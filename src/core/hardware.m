/**
 * CONVERGIO HARDWARE DETECTION
 *
 * Auto-detection and optimization for Apple Silicon chips
 * Uses sysctl for CPU info and Metal API for GPU info
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#include "nous/hardware.h"
#include <sys/sysctl.h>
#include <string.h>
#include <stdio.h>

// ============================================================================
// GLOBAL HARDWARE INFO
// ============================================================================

AppleSiliconInfo g_hardware = {0};
static bool g_hardware_detected = false;

// ============================================================================
// CHIP DATABASE
// ============================================================================

typedef struct {
    const char* identifier;  // Substring to match in chip name
    ChipFamily family;
    ChipVariant variant;
    uint8_t neural_cores;    // Neural Engine cores (not directly queryable)
    uint32_t bandwidth_gbps; // Memory bandwidth estimate
} ChipProfile;

// Known Apple Silicon profiles
static const ChipProfile CHIP_PROFILES[] = {
    // M1 family
    {"M1 Ultra", CHIP_FAMILY_M1, CHIP_VARIANT_ULTRA, 32, 800},
    {"M1 Max",   CHIP_FAMILY_M1, CHIP_VARIANT_MAX,   16, 400},
    {"M1 Pro",   CHIP_FAMILY_M1, CHIP_VARIANT_PRO,   16, 200},
    {"M1",       CHIP_FAMILY_M1, CHIP_VARIANT_BASE,  16, 68},

    // M2 family
    {"M2 Ultra", CHIP_FAMILY_M2, CHIP_VARIANT_ULTRA, 32, 800},
    {"M2 Max",   CHIP_FAMILY_M2, CHIP_VARIANT_MAX,   16, 400},
    {"M2 Pro",   CHIP_FAMILY_M2, CHIP_VARIANT_PRO,   16, 200},
    {"M2",       CHIP_FAMILY_M2, CHIP_VARIANT_BASE,  16, 100},

    // M3 family
    {"M3 Ultra", CHIP_FAMILY_M3, CHIP_VARIANT_ULTRA, 32, 800},
    {"M3 Max",   CHIP_FAMILY_M3, CHIP_VARIANT_MAX,   16, 400},
    {"M3 Pro",   CHIP_FAMILY_M3, CHIP_VARIANT_PRO,   16, 200},
    {"M3",       CHIP_FAMILY_M3, CHIP_VARIANT_BASE,  16, 100},

    // M4 family
    {"M4 Ultra", CHIP_FAMILY_M4, CHIP_VARIANT_ULTRA, 32, 800},
    {"M4 Max",   CHIP_FAMILY_M4, CHIP_VARIANT_MAX,   16, 400},
    {"M4 Pro",   CHIP_FAMILY_M4, CHIP_VARIANT_PRO,   16, 200},
    {"M4",       CHIP_FAMILY_M4, CHIP_VARIANT_BASE,  16, 100},

    {NULL, CHIP_FAMILY_UNKNOWN, CHIP_VARIANT_BASE, 0, 0}
};

// ============================================================================
// SYSCTL HELPERS
// ============================================================================

static int get_sysctl_int(const char* name) {
    int value = 0;
    size_t size = sizeof(value);
    if (sysctlbyname(name, &value, &size, NULL, 0) == 0) {
        return value;
    }
    return -1;
}

static int64_t get_sysctl_int64(const char* name) {
    int64_t value = 0;
    size_t size = sizeof(value);
    if (sysctlbyname(name, &value, &size, NULL, 0) == 0) {
        return value;
    }
    return -1;
}

static bool get_sysctl_string(const char* name, char* buf, size_t bufsize) {
    size_t size = bufsize;
    if (sysctlbyname(name, buf, &size, NULL, 0) == 0) {
        return true;
    }
    return false;
}

// ============================================================================
// CHIP IDENTIFICATION
// ============================================================================

static void identify_chip(const char* brand_string) {
    // Match against known profiles (order matters - check specific first)
    for (const ChipProfile* profile = CHIP_PROFILES; profile->identifier != NULL; profile++) {
        if (strstr(brand_string, profile->identifier) != NULL) {
            g_hardware.family = profile->family;
            g_hardware.variant = profile->variant;
            g_hardware.neural_cores = profile->neural_cores;
            g_hardware.memory_bandwidth_gbps = profile->bandwidth_gbps;
            return;
        }
    }

    // Unknown chip - use conservative defaults
    g_hardware.family = CHIP_FAMILY_UNKNOWN;
    g_hardware.variant = CHIP_VARIANT_BASE;
    g_hardware.neural_cores = 8;
    g_hardware.memory_bandwidth_gbps = 68;
}

// ============================================================================
// OPTIMIZATION PARAMETERS
// ============================================================================

static void calculate_optimal_parameters(void) {
    // Fabric shards: scale with total cores and memory
    // Base: 32 shards, scale up for more powerful chips
    uint32_t core_factor = g_hardware.total_cores / 8;
    uint32_t memory_gb = (uint32_t)(g_hardware.memory_bytes / (1024ULL * 1024 * 1024));
    uint32_t memory_factor = memory_gb / 8;

    g_hardware.optimal_fabric_shards = 32 * (core_factor > 0 ? core_factor : 1);
    if (g_hardware.optimal_fabric_shards > 128) {
        g_hardware.optimal_fabric_shards = 128;
    }

    // GPU batch size: scale with GPU cores
    g_hardware.optimal_gpu_batch = g_hardware.gpu_cores * 256;
    if (g_hardware.optimal_gpu_batch < 1024) {
        g_hardware.optimal_gpu_batch = 1024;
    }
    if (g_hardware.optimal_gpu_batch > 16384) {
        g_hardware.optimal_gpu_batch = 16384;
    }

    // Embedding buffer: scale with memory
    // Base: 64K embeddings for 8GB, scale up
    g_hardware.optimal_embedding_buffer = 65536 * (memory_factor > 0 ? memory_factor : 1);
    if (g_hardware.optimal_embedding_buffer > 1048576) {
        g_hardware.optimal_embedding_buffer = 1048576;  // Cap at 1M embeddings
    }

    // Threadgroup size: use GPU's recommended max
    g_hardware.optimal_threadgroup_size = 256;  // Good default for Apple Silicon
    if (g_hardware.gpu_cores >= 30) {
        g_hardware.optimal_threadgroup_size = 512;
    }
    if (g_hardware.gpu_cores >= 60) {
        g_hardware.optimal_threadgroup_size = 1024;
    }
}

// ============================================================================
// PUBLIC FUNCTIONS
// ============================================================================

int convergio_detect_hardware(void) {
    if (g_hardware_detected) {
        return 0;  // Already detected
    }

    @autoreleasepool {
        // Get chip name
        if (!get_sysctl_string("machdep.cpu.brand_string", g_hardware.chip_name, sizeof(g_hardware.chip_name))) {
            strncpy(g_hardware.chip_name, "Unknown Apple Silicon", sizeof(g_hardware.chip_name) - 1);
        }

        // Identify chip family and variant
        identify_chip(g_hardware.chip_name);

        // Get core counts
        int p_cores = get_sysctl_int("hw.perflevel0.physicalcpu");
        int e_cores = get_sysctl_int("hw.perflevel1.physicalcpu");

        if (p_cores > 0) {
            g_hardware.p_cores = (uint8_t)p_cores;
        } else {
            // Fallback: estimate from total
            int total = get_sysctl_int("hw.physicalcpu");
            g_hardware.p_cores = (uint8_t)(total > 4 ? total - 4 : total);
        }

        if (e_cores > 0) {
            g_hardware.e_cores = (uint8_t)e_cores;
        } else {
            g_hardware.e_cores = 4;  // Default for Apple Silicon
        }

        g_hardware.total_cores = g_hardware.p_cores + g_hardware.e_cores;

        // Get memory
        int64_t memsize = get_sysctl_int64("hw.memsize");
        if (memsize > 0) {
            g_hardware.memory_bytes = (uint64_t)memsize;
        } else {
            g_hardware.memory_bytes = 8ULL * 1024 * 1024 * 1024;  // Default 8GB
        }

        // Get cache info
        int l2 = get_sysctl_int("hw.l2cachesize");
        if (l2 > 0) {
            g_hardware.l2_cache_size = (uint32_t)l2;
        } else {
            g_hardware.l2_cache_size = 16 * 1024 * 1024;  // Default 16MB
        }

        int pagesize = get_sysctl_int("hw.pagesize");
        if (pagesize > 0) {
            g_hardware.page_size = (uint32_t)pagesize;
        } else {
            g_hardware.page_size = 16384;  // Apple Silicon uses 16KB pages
        }

        // Get GPU info via Metal
        id<MTLDevice> device = MTLCreateSystemDefaultDevice();
        if (device) {
            // Unfortunately, Apple doesn't expose GPU core count directly
            // We estimate based on chip variant
            switch (g_hardware.variant) {
                case CHIP_VARIANT_ULTRA:
                    g_hardware.gpu_cores = 76;  // M1/M2/M3 Ultra max
                    break;
                case CHIP_VARIANT_MAX:
                    g_hardware.gpu_cores = 40;  // M3 Max has 40
                    break;
                case CHIP_VARIANT_PRO:
                    g_hardware.gpu_cores = 18;  // Typical Pro
                    break;
                case CHIP_VARIANT_BASE:
                default:
                    g_hardware.gpu_cores = 10;  // Base model typical
                    break;
            }

            // Refine based on family
            if (g_hardware.family == CHIP_FAMILY_M3 && g_hardware.variant == CHIP_VARIANT_MAX) {
                g_hardware.gpu_cores = 40;  // M3 Max specific
            }
            if (g_hardware.family == CHIP_FAMILY_M3 && g_hardware.variant == CHIP_VARIANT_BASE) {
                g_hardware.gpu_cores = 10;
            }
        } else {
            g_hardware.gpu_cores = 8;  // Conservative default
        }

        // Calculate optimal parameters
        calculate_optimal_parameters();

        g_hardware_detected = true;
    }

    return 0;
}

void convergio_print_hardware_info(void) {
    if (!g_hardware_detected) {
        printf("Hardware not detected. Call convergio_detect_hardware() first.\n");
        return;
    }

    // Pre-format strings with values to ensure proper alignment
    char cpu_line[64];
    snprintf(cpu_line, sizeof(cpu_line), "%d P-cores + %d E-cores = %d total",
             g_hardware.p_cores, g_hardware.e_cores, g_hardware.total_cores);

    char memory_line[32];
    snprintf(memory_line, sizeof(memory_line), "%llu GB",
             g_hardware.memory_bytes / (1024ULL * 1024 * 1024));

    char bandwidth_line[32];
    snprintf(bandwidth_line, sizeof(bandwidth_line), "~%d GB/s",
             g_hardware.memory_bandwidth_gbps);

    char pagesize_line[32];
    snprintf(pagesize_line, sizeof(pagesize_line), "%d KB",
             g_hardware.page_size / 1024);

    printf("\n");
    printf("╔══════════════════════════════════════════════════════╗\n");
    printf("║              APPLE SILICON HARDWARE INFO             ║\n");
    printf("╠══════════════════════════════════════════════════════╣\n");
    printf("║  Chip: %-46s ║\n", g_hardware.chip_name);
    printf("║  Family: %-44s ║\n", convergio_chip_family_name(g_hardware.family));
    printf("║  Variant: %-43s ║\n", convergio_chip_variant_name(g_hardware.variant));
    printf("╠══════════════════════════════════════════════════════╣\n");
    printf("║  CPU Cores: %-41s ║\n", cpu_line);
    printf("║  GPU Cores: %-41d ║\n", g_hardware.gpu_cores);
    printf("║  Neural Engine: %-37d ║\n", g_hardware.neural_cores);
    printf("╠══════════════════════════════════════════════════════╣\n");
    printf("║  Unified Memory: %-36s ║\n", memory_line);
    printf("║  Memory Bandwidth: %-34s ║\n", bandwidth_line);
    printf("║  Page Size: %-41s ║\n", pagesize_line);
    printf("╠══════════════════════════════════════════════════════╣\n");
    printf("║  Optimized Parameters:                               ║\n");
    printf("║    Fabric Shards: %-35d ║\n", g_hardware.optimal_fabric_shards);
    printf("║    GPU Batch Size: %-34d ║\n", g_hardware.optimal_gpu_batch);
    printf("║    Embedding Buffer: %-32d ║\n", g_hardware.optimal_embedding_buffer);
    printf("║    Threadgroup Size: %-32d ║\n", g_hardware.optimal_threadgroup_size);
    printf("╚══════════════════════════════════════════════════════╝\n");
    printf("\n");
}

const char* convergio_chip_family_name(ChipFamily family) {
    switch (family) {
        case CHIP_FAMILY_M1: return "Apple M1";
        case CHIP_FAMILY_M2: return "Apple M2";
        case CHIP_FAMILY_M3: return "Apple M3";
        case CHIP_FAMILY_M4: return "Apple M4";
        default: return "Unknown";
    }
}

const char* convergio_chip_variant_name(ChipVariant variant) {
    switch (variant) {
        case CHIP_VARIANT_BASE:  return "Base";
        case CHIP_VARIANT_PRO:   return "Pro";
        case CHIP_VARIANT_MAX:   return "Max";
        case CHIP_VARIANT_ULTRA: return "Ultra";
        default: return "Unknown";
    }
}

bool convergio_hardware_detected(void) {
    return g_hardware_detected;
}
