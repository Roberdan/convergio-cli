/**
 * CONVERGIO HARDWARE DETECTION
 *
 * Auto-detection and optimization for Apple Silicon chips
 * Supports M1, M2, M3, M4, M5 and all variants (Pro, Max, Ultra)
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

#ifndef CONVERGIO_HARDWARE_H
#define CONVERGIO_HARDWARE_H

#include <stdint.h>
#include <stdbool.h>

// ============================================================================
// CHIP IDENTIFICATION
// ============================================================================

typedef enum {
    CHIP_VARIANT_BASE = 0,
    CHIP_VARIANT_PRO = 1,
    CHIP_VARIANT_MAX = 2,
    CHIP_VARIANT_ULTRA = 3
} ChipVariant;

typedef enum {
    CHIP_FAMILY_UNKNOWN = 0,
    CHIP_FAMILY_M1 = 1,
    CHIP_FAMILY_M2 = 2,
    CHIP_FAMILY_M3 = 3,
    CHIP_FAMILY_M4 = 4,
    CHIP_FAMILY_M5 = 5   // Nov 2025 - Neural Accelerators in GPU
} ChipFamily;

// ============================================================================
// HARDWARE INFO STRUCTURE
// ============================================================================

typedef struct {
    // Identification
    char chip_name[64];           // "Apple M3 Max"
    ChipFamily family;            // M1, M2, M3, M4
    ChipVariant variant;          // base, Pro, Max, Ultra

    // Core counts (detected via sysctl)
    uint8_t p_cores;              // Performance cores
    uint8_t e_cores;              // Efficiency cores
    uint8_t total_cores;          // p_cores + e_cores

    // GPU (detected via Metal API)
    uint8_t gpu_cores;            // Metal GPU cores

    // Neural Engine (estimated from chip family/variant)
    uint8_t neural_cores;         // Neural Engine cores

    // Memory
    uint64_t memory_bytes;        // Unified memory total
    uint32_t memory_bandwidth_gbps; // Estimated bandwidth

    // Optimized parameters (calculated based on hardware)
    uint32_t optimal_fabric_shards;    // For SemanticFabric
    uint32_t optimal_gpu_batch;        // For GPU operations
    uint32_t optimal_embedding_buffer; // Embedding buffer size
    uint32_t optimal_threadgroup_size; // For Metal compute

    // Cache sizes
    uint32_t l2_cache_size;       // L2 cache in bytes
    uint32_t page_size;           // Memory page size

} AppleSiliconInfo;

// ============================================================================
// GLOBAL HARDWARE INFO
// ============================================================================

extern AppleSiliconInfo g_hardware;

// ============================================================================
// FUNCTIONS
// ============================================================================

/**
 * Detect hardware and populate g_hardware struct
 * Must be called once at startup before using g_hardware
 * Returns 0 on success, -1 on failure
 */
int convergio_detect_hardware(void);

/**
 * Print hardware info to stdout
 */
void convergio_print_hardware_info(void);

/**
 * Get human-readable chip family name
 */
const char* convergio_chip_family_name(ChipFamily family);

/**
 * Get human-readable chip variant name
 */
const char* convergio_chip_variant_name(ChipVariant variant);

/**
 * Check if hardware detection was successful
 */
bool convergio_hardware_detected(void);

#endif // CONVERGIO_HARDWARE_H
