/**
 * NOUS Runtime Scheduler
 *
 * Intelligent work distribution across Apple Silicon cores:
 * - P-cores: User-facing, time-critical operations
 * - E-cores: Background maintenance, learning
 * - GPU: Batch operations, similarity search
 * - Neural Engine: Inference, embedding generation
 */

#include "nous/nous.h"
#include <arm_neon.h>
#include <dispatch/dispatch.h>
#include <mach/mach.h>
#include <mach/thread_policy.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// CORE TOPOLOGY
// ============================================================================

typedef enum {
    CORE_CLASS_PERFORMANCE, // P-cores: high-frequency, interactive
    CORE_CLASS_EFFICIENCY,  // E-cores: power-efficient, background
    CORE_CLASS_GPU,         // Metal GPU cores
    CORE_CLASS_NEURAL       // Neural Engine
} CoreClass;

typedef struct {
    CoreClass class;
    dispatch_queue_t queue;
    dispatch_group_t group;
    _Atomic size_t pending_tasks;
    _Atomic size_t completed_tasks;
    _Atomic uint64_t total_time_ns;
} CorePool;

typedef struct {
    CorePool performance;
    CorePool efficiency;
    CorePool gpu;
    CorePool neural;

    // Work stealing between P and E cores
    dispatch_semaphore_t work_available;

    // Metrics
    _Atomic uint64_t total_tasks_scheduled;
    _Atomic uint64_t tasks_stolen; // E-core took P-core work

    bool initialized;
} NousScheduler;

static NousScheduler g_scheduler = {0};

// ============================================================================
// QOS CLASS MAPPING
// ============================================================================

/*
 * Apple Silicon QoS to Core mapping:
 *
 * QOS_CLASS_USER_INTERACTIVE  -> P-cores only (highest priority)
 * QOS_CLASS_USER_INITIATED    -> P-cores preferred
 * QOS_CLASS_DEFAULT           -> P or E cores
 * QOS_CLASS_UTILITY           -> E-cores preferred
 * QOS_CLASS_BACKGROUND        -> E-cores only (lowest priority)
 */

__attribute__((unused)) static dispatch_qos_class_t core_class_to_qos(CoreClass class) {
    switch (class) {
    case CORE_CLASS_PERFORMANCE:
        return QOS_CLASS_USER_INTERACTIVE;
    case CORE_CLASS_EFFICIENCY:
        return QOS_CLASS_UTILITY;
    case CORE_CLASS_GPU:
        return QOS_CLASS_USER_INITIATED;
    case CORE_CLASS_NEURAL:
        return QOS_CLASS_USER_INITIATED;
    default:
        return QOS_CLASS_DEFAULT;
    }
}

// ============================================================================
// SCHEDULER INITIALIZATION
// ============================================================================

int nous_scheduler_init(void) {
    if (g_scheduler.initialized)
        return 0;

    // Performance cores queue (concurrent, high priority)
    dispatch_queue_attr_t p_attr = dispatch_queue_attr_make_with_qos_class(
        DISPATCH_QUEUE_CONCURRENT, QOS_CLASS_USER_INTERACTIVE, 0);
    g_scheduler.performance.queue = dispatch_queue_create("nous.perf", p_attr);
    g_scheduler.performance.group = dispatch_group_create();
    g_scheduler.performance.class = CORE_CLASS_PERFORMANCE;

    // Efficiency cores queue (concurrent, low priority)
    dispatch_queue_attr_t e_attr =
        dispatch_queue_attr_make_with_qos_class(DISPATCH_QUEUE_CONCURRENT, QOS_CLASS_UTILITY, 0);
    g_scheduler.efficiency.queue = dispatch_queue_create("nous.eff", e_attr);
    g_scheduler.efficiency.group = dispatch_group_create();
    g_scheduler.efficiency.class = CORE_CLASS_EFFICIENCY;

    // GPU queue (serial for Metal command buffer ordering)
    dispatch_queue_attr_t gpu_attr =
        dispatch_queue_attr_make_with_qos_class(DISPATCH_QUEUE_SERIAL, QOS_CLASS_USER_INITIATED, 0);
    g_scheduler.gpu.queue = dispatch_queue_create("nous.gpu", gpu_attr);
    g_scheduler.gpu.group = dispatch_group_create();
    g_scheduler.gpu.class = CORE_CLASS_GPU;

    // Neural Engine queue (serial for model inference)
    dispatch_queue_attr_t ne_attr =
        dispatch_queue_attr_make_with_qos_class(DISPATCH_QUEUE_SERIAL, QOS_CLASS_USER_INITIATED, 0);
    g_scheduler.neural.queue = dispatch_queue_create("nous.neural", ne_attr);
    g_scheduler.neural.group = dispatch_group_create();
    g_scheduler.neural.class = CORE_CLASS_NEURAL;

    // Work stealing semaphore
    g_scheduler.work_available = dispatch_semaphore_create(0);

    g_scheduler.initialized = true;
    return 0;
}

void nous_scheduler_shutdown(void) {
    if (!g_scheduler.initialized)
        return;

    // Wait for all work to complete
    dispatch_group_wait(g_scheduler.performance.group, DISPATCH_TIME_FOREVER);
    dispatch_group_wait(g_scheduler.efficiency.group, DISPATCH_TIME_FOREVER);
    dispatch_group_wait(g_scheduler.gpu.group, DISPATCH_TIME_FOREVER);
    dispatch_group_wait(g_scheduler.neural.group, DISPATCH_TIME_FOREVER);

    // Release queues and groups
    dispatch_release(g_scheduler.performance.queue);
    dispatch_release(g_scheduler.performance.group);
    dispatch_release(g_scheduler.efficiency.queue);
    dispatch_release(g_scheduler.efficiency.group);
    dispatch_release(g_scheduler.gpu.queue);
    dispatch_release(g_scheduler.gpu.group);
    dispatch_release(g_scheduler.neural.queue);
    dispatch_release(g_scheduler.neural.group);

    g_scheduler.initialized = false;
}

// ============================================================================
// TASK SCHEDULING
// ============================================================================

typedef enum {
    TASK_PRIORITY_CRITICAL,  // Must run on P-cores immediately
    TASK_PRIORITY_HIGH,      // P-cores preferred
    TASK_PRIORITY_NORMAL,    // P or E cores OK
    TASK_PRIORITY_LOW,       // E-cores preferred
    TASK_PRIORITY_BACKGROUND // E-cores only
} TaskPriority;

typedef struct {
    void (*function)(void* ctx);
    void* context;
    void (*cleanup)(void* ctx);
    TaskPriority priority;
    const char* name;
    uint64_t submit_time;
} ScheduledTask;

static CorePool* select_pool(TaskPriority priority) {
    switch (priority) {
    case TASK_PRIORITY_CRITICAL:
    case TASK_PRIORITY_HIGH:
        return &g_scheduler.performance;
    case TASK_PRIORITY_BACKGROUND:
    case TASK_PRIORITY_LOW:
        return &g_scheduler.efficiency;
    case TASK_PRIORITY_NORMAL:
    default:
        // Load balance: prefer less loaded pool
        if (atomic_load(&g_scheduler.performance.pending_tasks) <
            atomic_load(&g_scheduler.efficiency.pending_tasks)) {
            return &g_scheduler.performance;
        }
        return &g_scheduler.efficiency;
    }
}

void nous_schedule(void (*fn)(void*), void* ctx, TaskPriority priority) {
    if (!g_scheduler.initialized || !fn)
        return;

    CorePool* pool = select_pool(priority);
    atomic_fetch_add(&pool->pending_tasks, 1);
    atomic_fetch_add(&g_scheduler.total_tasks_scheduled, 1);

    dispatch_group_async(pool->group, pool->queue, ^{
        struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start);

        fn(ctx);

        clock_gettime(CLOCK_MONOTONIC, &end);
        uint64_t elapsed = (uint64_t)(end.tv_sec - start.tv_sec) * 1000000000ULL +
                           (uint64_t)(end.tv_nsec - start.tv_nsec);

        atomic_fetch_add(&pool->total_time_ns, elapsed);
        atomic_fetch_sub(&pool->pending_tasks, 1);
        atomic_fetch_add(&pool->completed_tasks, 1);
    });
}

void nous_schedule_gpu(void (*fn)(void*), void* ctx) {
    if (!g_scheduler.initialized || !fn)
        return;

    atomic_fetch_add(&g_scheduler.gpu.pending_tasks, 1);

    dispatch_group_async(g_scheduler.gpu.group, g_scheduler.gpu.queue, ^{
        fn(ctx);
        atomic_fetch_sub(&g_scheduler.gpu.pending_tasks, 1);
        atomic_fetch_add(&g_scheduler.gpu.completed_tasks, 1);
    });
}

void nous_schedule_neural(void (*fn)(void*), void* ctx) {
    if (!g_scheduler.initialized || !fn)
        return;

    atomic_fetch_add(&g_scheduler.neural.pending_tasks, 1);

    dispatch_group_async(g_scheduler.neural.group, g_scheduler.neural.queue, ^{
        fn(ctx);
        atomic_fetch_sub(&g_scheduler.neural.pending_tasks, 1);
        atomic_fetch_add(&g_scheduler.neural.completed_tasks, 1);
    });
}

// ============================================================================
// PARALLEL PRIMITIVES (SIMD-optimized for Apple Silicon)
// ============================================================================

typedef void (*ParallelBody)(size_t index, void* ctx);

/**
 * Parallel for loop with automatic core selection
 * Splits work across P-cores for small arrays, adds E-cores for large arrays
 */
void nous_parallel_for(size_t start, size_t end, ParallelBody body, void* ctx) {
    if (start >= end || !body)
        return;

    size_t count = end - start;

    // For small workloads, use dispatch_apply on P-cores
    if (count <= 1000) {
        dispatch_apply(count, g_scheduler.performance.queue, ^(size_t i) {
            body(start + i, ctx);
        });
        return;
    }

    // For large workloads, split between P and E cores
    size_t p_core_share = count * 7 / 10; // 70% to P-cores
    size_t e_core_share = count - p_core_share;

    dispatch_group_t group = dispatch_group_create();

    // P-cores handle the bulk
    dispatch_group_async(group, g_scheduler.performance.queue, ^{
        dispatch_apply(p_core_share, g_scheduler.performance.queue, ^(size_t i) {
            body(start + i, ctx);
        });
    });

    // E-cores handle the rest
    dispatch_group_async(group, g_scheduler.efficiency.queue, ^{
        dispatch_apply(e_core_share, g_scheduler.efficiency.queue, ^(size_t i) {
            body(start + p_core_share + i, ctx);
        });
    });

    dispatch_group_wait(group, DISPATCH_TIME_FOREVER);
    dispatch_release(group);
}

/**
 * SIMD-accelerated parallel reduction
 * Uses NEON for vectorized operations
 */
typedef float (*ReduceOp)(float a, float b);

float nous_parallel_reduce(const float* array, size_t count, float identity, ReduceOp op) {
    if (!array || count == 0)
        return identity;

    // Process 4 elements at a time with NEON
    size_t simd_count = count & ~3ULL;
    float32x4_t acc = vdupq_n_f32(identity);

    for (size_t i = 0; i < simd_count; i += 4) {
        float32x4_t v = vld1q_f32(&array[i]);

        // Horizontal reduction within vector
        float32x2_t sum1 = vadd_f32(vget_low_f32(v), vget_high_f32(v));
        float32x2_t sum2 = vpadd_f32(sum1, sum1);
        float partial = vget_lane_f32(sum2, 0);

        // Combine with accumulator (assuming op is addition)
        acc = vsetq_lane_f32(op(vgetq_lane_f32(acc, 0), partial), acc, 0);
    }

    // Extract final result
    float result = vgetq_lane_f32(acc, 0);

    // Handle remainder
    for (size_t i = simd_count; i < count; i++) {
        result = op(result, array[i]);
    }

    return result;
}

// ============================================================================
// WORK STEALING
// ============================================================================

/**
 * Allows E-cores to steal work from P-cores when they're idle
 * This improves overall throughput without affecting latency
 */
typedef struct {
    ScheduledTask* tasks;
    size_t capacity;
    _Atomic size_t head;
    _Atomic size_t tail;
    os_unfair_lock lock;
} WorkStealingQueue;

static WorkStealingQueue g_steal_queue = {
    .tasks = NULL, .capacity = 0, .head = 0, .tail = 0, .lock = OS_UNFAIR_LOCK_INIT};

int nous_init_work_stealing(size_t capacity) {
    g_steal_queue.tasks = calloc(capacity, sizeof(ScheduledTask));
    if (!g_steal_queue.tasks)
        return -1;
    g_steal_queue.capacity = capacity;
    return 0;
}

bool nous_try_steal_work(ScheduledTask* out_task) {
    if (!out_task)
        return false;

    os_unfair_lock_lock(&g_steal_queue.lock);

    size_t head = atomic_load(&g_steal_queue.head);
    size_t tail = atomic_load(&g_steal_queue.tail);

    if (head == tail) {
        os_unfair_lock_unlock(&g_steal_queue.lock);
        return false;
    }

    *out_task = g_steal_queue.tasks[head % g_steal_queue.capacity];
    atomic_store(&g_steal_queue.head, head + 1);

    os_unfair_lock_unlock(&g_steal_queue.lock);

    atomic_fetch_add(&g_scheduler.tasks_stolen, 1);
    return true;
}

// ============================================================================
// SCHEDULER METRICS
// ============================================================================

typedef struct {
    size_t p_core_tasks;
    size_t e_core_tasks;
    size_t gpu_tasks;
    size_t neural_tasks;
    double p_core_avg_time_ms;
    double e_core_avg_time_ms;
    size_t tasks_stolen;
} SchedulerMetrics;

void nous_scheduler_get_metrics(SchedulerMetrics* metrics) {
    if (!metrics)
        return;

    metrics->p_core_tasks = atomic_load(&g_scheduler.performance.completed_tasks);
    metrics->e_core_tasks = atomic_load(&g_scheduler.efficiency.completed_tasks);
    metrics->gpu_tasks = atomic_load(&g_scheduler.gpu.completed_tasks);
    metrics->neural_tasks = atomic_load(&g_scheduler.neural.completed_tasks);
    metrics->tasks_stolen = atomic_load(&g_scheduler.tasks_stolen);

    if (metrics->p_core_tasks > 0) {
        metrics->p_core_avg_time_ms = (double)atomic_load(&g_scheduler.performance.total_time_ns) /
                                      (double)metrics->p_core_tasks / 1e6;
    }

    if (metrics->e_core_tasks > 0) {
        metrics->e_core_avg_time_ms = (double)atomic_load(&g_scheduler.efficiency.total_time_ns) /
                                      (double)metrics->e_core_tasks / 1e6;
    }
}

void nous_scheduler_print_metrics(void) {
    SchedulerMetrics m;
    nous_scheduler_get_metrics(&m);

    printf("NOUS Scheduler Metrics:\n");
    printf("  P-core tasks: %zu (avg %.3f ms)\n", m.p_core_tasks, m.p_core_avg_time_ms);
    printf("  E-core tasks: %zu (avg %.3f ms)\n", m.e_core_tasks, m.e_core_avg_time_ms);
    printf("  GPU tasks: %zu\n", m.gpu_tasks);
    printf("  Neural tasks: %zu\n", m.neural_tasks);
    printf("  Work stolen: %zu\n", m.tasks_stolen);
}

// ============================================================================
// THREAD AFFINITY (for critical paths)
// ============================================================================

/**
 * Pin the current thread to P-cores for latency-sensitive work
 * Note: This is a hint to the scheduler, not a hard requirement
 */
int nous_pin_to_p_cores(void) {
    thread_t thread = mach_thread_self();

    thread_affinity_policy_data_t policy = {.affinity_tag = 1};
    kern_return_t result = thread_policy_set(
        thread, THREAD_AFFINITY_POLICY, (thread_policy_t)&policy, THREAD_AFFINITY_POLICY_COUNT);

    mach_port_deallocate(mach_task_self(), thread);
    return (result == KERN_SUCCESS) ? 0 : -1;
}

/**
 * Allow the thread to run on any core (default)
 */
int nous_unpin_thread(void) {
    thread_t thread = mach_thread_self();

    thread_affinity_policy_data_t policy = {.affinity_tag = 0};
    kern_return_t result = thread_policy_set(
        thread, THREAD_AFFINITY_POLICY, (thread_policy_t)&policy, THREAD_AFFINITY_POLICY_COUNT);

    mach_port_deallocate(mach_task_self(), thread);
    return (result == KERN_SUCCESS) ? 0 : -1;
}
