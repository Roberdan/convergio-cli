/**
 * NOUS Metal GPU Integration
 *
 * Leverages Apple Silicon GPU cores for:
 * - Batch similarity computations
 * - Embedding generation
 * - Graph propagation
 */

#import <Metal/Metal.h>
#import <MetalPerformanceShaders/MetalPerformanceShaders.h>
#include "nous/nous.h"
#include <stdlib.h>

// ============================================================================
// GPU CONTEXT
// ============================================================================

typedef struct {
    id<MTLDevice> device;
    id<MTLCommandQueue> commandQueue;

    // Compute pipelines
    id<MTLComputePipelineState> similarityPipeline;
    id<MTLComputePipelineState> topKPipeline;
    id<MTLComputePipelineState> propagatePipeline;
    id<MTLComputePipelineState> kmeansAssignPipeline;

    // Persistent buffers for embedding storage
    id<MTLBuffer> embeddingBuffer;      // All embeddings
    size_t embeddingCapacity;           // Number of embeddings allocated
    size_t embeddingCount;              // Current count

    // MPS for optimized operations
    MPSMatrixMultiplication* matmul;

    // Thread management
    dispatch_queue_t gpuQueue;
    os_unfair_lock bufferLock;

} NousGPUContext;

static NousGPUContext* g_gpu = NULL;

// ============================================================================
// INITIALIZATION
// ============================================================================

int nous_gpu_init(void) {
    if (g_gpu) return 0;  // Already initialized

    g_gpu = calloc(1, sizeof(NousGPUContext));
    if (!g_gpu) return -1;

    @autoreleasepool {
        // Get the Apple Silicon GPU
        g_gpu->device = MTLCreateSystemDefaultDevice();
        if (!g_gpu->device) {
            free(g_gpu);
            g_gpu = NULL;
            return -1;
        }

        // Silent initialization - info available via nous_gpu_print_stats()

        // Create command queue with max concurrent execution
        g_gpu->commandQueue = [g_gpu->device newCommandQueueWithMaxCommandBufferCount:64];

        // Load shader library
        NSError* error = nil;
        NSString* shaderPath = [[NSBundle mainBundle] pathForResource:@"similarity"
                                                              ofType:@"metallib"];
        id<MTLLibrary> library;

        if (shaderPath) {
            // Use newLibraryWithURL instead of deprecated newLibraryWithFile
            NSURL* shaderURL = [NSURL fileURLWithPath:shaderPath];
            library = [g_gpu->device newLibraryWithURL:shaderURL error:&error];
        } else {
            // Try to load from source
            NSString* sourcePath = @"shaders/similarity.metal";
            NSString* source = [NSString stringWithContentsOfFile:sourcePath
                                                         encoding:NSUTF8StringEncoding
                                                            error:&error];
            if (source) {
                MTLCompileOptions* options = [[MTLCompileOptions alloc] init];
#if defined(__MAC_OS_X_VERSION_MAX_ALLOWED) && __MAC_OS_X_VERSION_MAX_ALLOWED >= 140000
                if (@available(macOS 14.0, *)) {
                    options.mathMode = MTLMathModeFast;
                }
#endif
                options.languageVersion = MTLLanguageVersion3_0;
                library = [g_gpu->device newLibraryWithSource:source
                                                      options:options
                                                        error:&error];
            }
        }

        if (!library) {
            // Shaders not available - using MPS fallback (silent)
        } else {
            // Create pipelines
            MTLComputePipelineDescriptor* desc = [[MTLComputePipelineDescriptor alloc] init];
            desc.threadGroupSizeIsMultipleOfThreadExecutionWidth = YES;

            // Similarity pipeline
            id<MTLFunction> simFunc = [library newFunctionWithName:@"batch_cosine_similarity"];
            if (simFunc) {
                desc.computeFunction = simFunc;
                g_gpu->similarityPipeline = [g_gpu->device newComputePipelineStateWithDescriptor:desc
                                                                                         options:0
                                                                                      reflection:nil
                                                                                           error:&error];
            }

            // Top-K pipeline
            id<MTLFunction> topkFunc = [library newFunctionWithName:@"top_k_similarity"];
            if (topkFunc) {
                desc.computeFunction = topkFunc;
                g_gpu->topKPipeline = [g_gpu->device newComputePipelineStateWithDescriptor:desc
                                                                                   options:0
                                                                                reflection:nil
                                                                                     error:&error];
            }

            // Propagate pipeline
            id<MTLFunction> propFunc = [library newFunctionWithName:@"propagate_activation"];
            if (propFunc) {
                desc.computeFunction = propFunc;
                g_gpu->propagatePipeline = [g_gpu->device newComputePipelineStateWithDescriptor:desc
                                                                                        options:0
                                                                                     reflection:nil
                                                                                          error:&error];
            }

            // KMeans pipeline
            id<MTLFunction> kmeansFunc = [library newFunctionWithName:@"kmeans_assign"];
            if (kmeansFunc) {
                desc.computeFunction = kmeansFunc;
                g_gpu->kmeansAssignPipeline = [g_gpu->device newComputePipelineStateWithDescriptor:desc
                                                                                          options:0
                                                                                       reflection:nil
                                                                                            error:&error];
            }
        }

        // Initialize embedding buffer (start with 64K embeddings)
        g_gpu->embeddingCapacity = 65536;
        size_t bufferSize = g_gpu->embeddingCapacity * NOUS_EMBEDDING_DIM * sizeof(_Float16);

        // Use shared memory mode for unified memory architecture
        MTLResourceOptions options = MTLResourceStorageModeShared |
                                     MTLResourceCPUCacheModeWriteCombined |
                                     MTLResourceHazardTrackingModeTracked;

        g_gpu->embeddingBuffer = [g_gpu->device newBufferWithLength:bufferSize
                                                            options:options];

        // Create dispatch queue for GPU operations
        dispatch_queue_attr_t attr = dispatch_queue_attr_make_with_qos_class(
            DISPATCH_QUEUE_SERIAL, QOS_CLASS_USER_INITIATED, 0);
        g_gpu->gpuQueue = dispatch_queue_create("nous.gpu", attr);

        g_gpu->bufferLock = OS_UNFAIR_LOCK_INIT;
    }

    return 0;
}

void nous_gpu_shutdown(void) {
    if (!g_gpu) return;

    @autoreleasepool {
        // Release all Metal objects (ARC handles this)
        g_gpu->device = nil;
        g_gpu->commandQueue = nil;
        g_gpu->similarityPipeline = nil;
        g_gpu->topKPipeline = nil;
        g_gpu->propagatePipeline = nil;
        g_gpu->embeddingBuffer = nil;

        // dispatch queues are automatically released under ARC
        g_gpu->gpuQueue = nil;

        free(g_gpu);
        g_gpu = NULL;
    }
}

// ============================================================================
// EMBEDDING MANAGEMENT (Unified Memory)
// ============================================================================

size_t nous_gpu_add_embedding(const NousEmbedding* embedding) {
    if (!g_gpu || !embedding) return SIZE_MAX;

    os_unfair_lock_lock(&g_gpu->bufferLock);

    // Grow buffer if needed
    if (g_gpu->embeddingCount >= g_gpu->embeddingCapacity) {
        size_t newCapacity = g_gpu->embeddingCapacity * 2;
        size_t newSize = newCapacity * NOUS_EMBEDDING_DIM * sizeof(_Float16);

        @autoreleasepool {
            id<MTLBuffer> newBuffer = [g_gpu->device newBufferWithLength:newSize
                                       options:MTLResourceStorageModeShared |
                                               MTLResourceCPUCacheModeWriteCombined];
            if (!newBuffer) {
                os_unfair_lock_unlock(&g_gpu->bufferLock);
                return SIZE_MAX;
            }

            // Copy existing data
            memcpy(newBuffer.contents, g_gpu->embeddingBuffer.contents,
                   g_gpu->embeddingCount * NOUS_EMBEDDING_DIM * sizeof(_Float16));

            g_gpu->embeddingBuffer = newBuffer;
            g_gpu->embeddingCapacity = newCapacity;
        }
    }

    // Copy new embedding
    _Float16* dest = (_Float16*)g_gpu->embeddingBuffer.contents;
    dest += g_gpu->embeddingCount * NOUS_EMBEDDING_DIM;
    memcpy(dest, embedding->values, NOUS_EMBEDDING_DIM * sizeof(_Float16));

    size_t index = g_gpu->embeddingCount++;

    os_unfair_lock_unlock(&g_gpu->bufferLock);

    return index;
}

// ============================================================================
// BATCH SIMILARITY (GPU-Accelerated)
// ============================================================================

void nous_metal_batch_similarity(const NousEmbedding* query,
                                 const NousEmbedding* candidates,
                                 size_t count,
                                 float* results) {
    if (!g_gpu || !query || !candidates || !results || count == 0) return;

    @autoreleasepool {
        // Create buffers
        id<MTLBuffer> queryBuffer = [g_gpu->device newBufferWithBytes:query->values
                                     length:NOUS_EMBEDDING_DIM * sizeof(_Float16)
                                     options:MTLResourceStorageModeShared];

        id<MTLBuffer> candidatesBuffer = [g_gpu->device newBufferWithBytes:candidates
                                          length:count * NOUS_EMBEDDING_DIM * sizeof(_Float16)
                                          options:MTLResourceStorageModeShared];

        id<MTLBuffer> resultsBuffer = [g_gpu->device newBufferWithLength:count * sizeof(float)
                                       options:MTLResourceStorageModeShared];

        // Create command buffer
        id<MTLCommandBuffer> commandBuffer = [g_gpu->commandQueue commandBuffer];
        id<MTLComputeCommandEncoder> encoder = [commandBuffer computeCommandEncoder];

        if (g_gpu->similarityPipeline) {
            [encoder setComputePipelineState:g_gpu->similarityPipeline];
            [encoder setBuffer:queryBuffer offset:0 atIndex:0];
            [encoder setBuffer:candidatesBuffer offset:0 atIndex:1];
            [encoder setBuffer:resultsBuffer offset:0 atIndex:2];

            uint32_t candidateCount = (uint32_t)count;
            [encoder setBytes:&candidateCount length:sizeof(uint32_t) atIndex:3];

            // Optimal threadgroup size for Apple Silicon
            NSUInteger threadgroupSize = MIN(256, g_gpu->similarityPipeline.maxTotalThreadsPerThreadgroup);
            MTLSize threadgroupsPerGrid = MTLSizeMake((count + threadgroupSize - 1) / threadgroupSize, 1, 1);
            MTLSize threadsPerThreadgroup = MTLSizeMake(threadgroupSize, 1, 1);

            [encoder dispatchThreadgroups:threadgroupsPerGrid
                    threadsPerThreadgroup:threadsPerThreadgroup];
        }

        [encoder endEncoding];

        // Execute and wait
        [commandBuffer commit];
        [commandBuffer waitUntilCompleted];

        // Copy results back
        memcpy(results, resultsBuffer.contents, count * sizeof(float));
    }
}

// ============================================================================
// GRAPH PROPAGATION (GPU-Accelerated)
// ============================================================================

typedef struct {
    uint32_t* offsets;      // CSR row offsets
    uint32_t* indices;      // CSR column indices
    _Float16* weights;      // Edge weights
    size_t nodeCount;
    size_t edgeCount;
} GraphCSR;

void nous_gpu_propagate(GraphCSR* graph, float* activations, float decay) {
    if (!g_gpu || !graph || !activations) return;

    @autoreleasepool {
        // Create buffers
        id<MTLBuffer> offsetsBuffer = [g_gpu->device newBufferWithBytes:graph->offsets
                                       length:(graph->nodeCount + 1) * sizeof(uint32_t)
                                       options:MTLResourceStorageModeShared];

        id<MTLBuffer> indicesBuffer = [g_gpu->device newBufferWithBytes:graph->indices
                                       length:graph->edgeCount * sizeof(uint32_t)
                                       options:MTLResourceStorageModeShared];

        id<MTLBuffer> weightsBuffer = [g_gpu->device newBufferWithBytes:graph->weights
                                       length:graph->edgeCount * sizeof(_Float16)
                                       options:MTLResourceStorageModeShared];

        id<MTLBuffer> activationsBuffer = [g_gpu->device newBufferWithBytes:activations
                                           length:graph->nodeCount * sizeof(float)
                                           options:MTLResourceStorageModeShared];

        id<MTLBuffer> newActivationsBuffer = [g_gpu->device newBufferWithLength:graph->nodeCount * sizeof(float)
                                              options:MTLResourceStorageModeShared];

        if (!g_gpu->propagatePipeline) return;

        id<MTLCommandBuffer> commandBuffer = [g_gpu->commandQueue commandBuffer];
        id<MTLComputeCommandEncoder> encoder = [commandBuffer computeCommandEncoder];

        [encoder setComputePipelineState:g_gpu->propagatePipeline];
        [encoder setBuffer:offsetsBuffer offset:0 atIndex:0];
        [encoder setBuffer:indicesBuffer offset:0 atIndex:1];
        [encoder setBuffer:weightsBuffer offset:0 atIndex:2];
        [encoder setBuffer:activationsBuffer offset:0 atIndex:3];
        [encoder setBuffer:newActivationsBuffer offset:0 atIndex:4];
        [encoder setBytes:&decay length:sizeof(float) atIndex:5];

        uint32_t nodeCount = (uint32_t)graph->nodeCount;
        [encoder setBytes:&nodeCount length:sizeof(uint32_t) atIndex:6];

        NSUInteger threadgroupSize = MIN(256, g_gpu->propagatePipeline.maxTotalThreadsPerThreadgroup);
        MTLSize threadgroupsPerGrid = MTLSizeMake((graph->nodeCount + threadgroupSize - 1) / threadgroupSize, 1, 1);
        MTLSize threadsPerThreadgroup = MTLSizeMake(threadgroupSize, 1, 1);

        [encoder dispatchThreadgroups:threadgroupsPerGrid
                threadsPerThreadgroup:threadsPerThreadgroup];

        [encoder endEncoding];
        [commandBuffer commit];
        [commandBuffer waitUntilCompleted];

        // Copy results back
        memcpy(activations, newActivationsBuffer.contents, graph->nodeCount * sizeof(float));
    }
}

// ============================================================================
// MPS MATRIX OPERATIONS (for large-scale embedding ops)
// ============================================================================

void nous_gpu_embedding_matmul(const _Float16* A, const _Float16* B,
                                float* C,
                                size_t M, size_t K, size_t N) {
    if (!g_gpu || !A || !B || !C) return;

    @autoreleasepool {
        // Create matrix descriptors
        MPSMatrixDescriptor* descA = [MPSMatrixDescriptor matrixDescriptorWithRows:M
                                      columns:K
                                      rowBytes:K * sizeof(_Float16)
                                      dataType:MPSDataTypeFloat16];

        MPSMatrixDescriptor* descB = [MPSMatrixDescriptor matrixDescriptorWithRows:K
                                      columns:N
                                      rowBytes:N * sizeof(_Float16)
                                      dataType:MPSDataTypeFloat16];

        MPSMatrixDescriptor* descC = [MPSMatrixDescriptor matrixDescriptorWithRows:M
                                      columns:N
                                      rowBytes:N * sizeof(float)
                                      dataType:MPSDataTypeFloat32];

        // Create buffers
        id<MTLBuffer> bufferA = [g_gpu->device newBufferWithBytes:A
                                 length:M * K * sizeof(_Float16)
                                 options:MTLResourceStorageModeShared];

        id<MTLBuffer> bufferB = [g_gpu->device newBufferWithBytes:B
                                 length:K * N * sizeof(_Float16)
                                 options:MTLResourceStorageModeShared];

        id<MTLBuffer> bufferC = [g_gpu->device newBufferWithLength:M * N * sizeof(float)
                                 options:MTLResourceStorageModeShared];

        // Create matrices
        MPSMatrix* matrixA = [[MPSMatrix alloc] initWithBuffer:bufferA descriptor:descA];
        MPSMatrix* matrixB = [[MPSMatrix alloc] initWithBuffer:bufferB descriptor:descB];
        MPSMatrix* matrixC = [[MPSMatrix alloc] initWithBuffer:bufferC descriptor:descC];

        // Create and execute multiplication
        MPSMatrixMultiplication* matmul = [[MPSMatrixMultiplication alloc]
                                           initWithDevice:g_gpu->device
                                           transposeLeft:NO
                                           transposeRight:NO
                                           resultRows:M
                                           resultColumns:N
                                           interiorColumns:K
                                           alpha:1.0
                                           beta:0.0];

        id<MTLCommandBuffer> commandBuffer = [g_gpu->commandQueue commandBuffer];
        [matmul encodeToCommandBuffer:commandBuffer
                           leftMatrix:matrixA
                          rightMatrix:matrixB
                         resultMatrix:matrixC];

        [commandBuffer commit];
        [commandBuffer waitUntilCompleted];

        // Copy results
        memcpy(C, bufferC.contents, M * N * sizeof(float));
    }
}

// ============================================================================
// GPU DIAGNOSTICS
// ============================================================================

void nous_gpu_print_stats(void) {
    if (!g_gpu) {
        printf("NOUS GPU: Not initialized\n");
        return;
    }

    @autoreleasepool {
        printf("NOUS GPU Statistics:\n");
        printf("  Device: %s\n", [g_gpu->device.name UTF8String]);
        printf("  Unified Memory: %s\n", g_gpu->device.hasUnifiedMemory ? "Yes" : "No");
        printf("  Max Threadgroup Memory: %lu bytes\n",
               (unsigned long)g_gpu->device.maxThreadgroupMemoryLength);
        printf("  Embeddings stored: %zu / %zu\n",
               g_gpu->embeddingCount, g_gpu->embeddingCapacity);
        printf("  Buffer size: %.2f MB\n",
               (double)(g_gpu->embeddingCapacity * NOUS_EMBEDDING_DIM * sizeof(_Float16)) / (1024 * 1024));
    }
}
