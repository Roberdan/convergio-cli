cmake_minimum_required(VERSION 3.20)

project(CONVERGIO
    VERSION 0.1.0
    DESCRIPTION "Convergio Kernel - A semantic kernel for human-AI symbiosis"
    LANGUAGES C OBJC
)

# Require Apple Silicon
if(NOT APPLE)
    message(FATAL_ERROR "Convergio Kernel is designed for Apple Silicon Macs only")
endif()

# M3 Max optimizations
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_OBJC_STANDARD 17)

# Architecture-specific flags for M3 Max
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=armv8.6-a+fp16+dotprod+crypto")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mtune=apple-m3")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffast-math")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvectorize")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -flto")

# Release optimizations
set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_C_FLAGS_DEBUG "-g -O0 -DDEBUG -fsanitize=address,undefined")

# Find required frameworks
find_library(METAL_FRAMEWORK Metal REQUIRED)
find_library(METALKIT_FRAMEWORK MetalKit REQUIRED)
find_library(MPS_FRAMEWORK MetalPerformanceShaders REQUIRED)
find_library(ACCELERATE_FRAMEWORK Accelerate REQUIRED)
find_library(FOUNDATION_FRAMEWORK Foundation REQUIRED)
find_library(COREML_FRAMEWORK CoreML)

# Find readline
find_path(READLINE_INCLUDE_DIR readline/readline.h)
find_library(READLINE_LIBRARY readline)

if(NOT READLINE_LIBRARY)
    message(STATUS "readline not found, using editline fallback")
    find_library(READLINE_LIBRARY edit)
endif()

# Find curl
find_library(CURL_LIBRARY curl)
find_path(CURL_INCLUDE_DIR curl/curl.h)

# Find sqlite3
find_library(SQLITE3_LIBRARY sqlite3)
find_path(SQLITE3_INCLUDE_DIR sqlite3.h)

# Find Security framework for keychain
find_library(SECURITY_FRAMEWORK Security)
find_library(APPKIT_FRAMEWORK AppKit)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${READLINE_INCLUDE_DIR}
    ${CURL_INCLUDE_DIR}
    ${SQLITE3_INCLUDE_DIR}
)

# Source files - all C sources
set(NOUS_SOURCES
    src/core/fabric.c
    src/core/main.c
    src/core/ansi_md.c
    src/core/stream_md.c
    src/core/theme.c
    src/core/config.c
    src/core/updater.c
    src/core/safe_path.c
    src/intent/parser.c
    src/intent/interpreter.c
    src/agents/agent.c
    src/agents/embedded_agents.c
    src/spaces/space.c
    src/runtime/scheduler.c
    src/neural/claude.c
    src/orchestrator/cost.c
    src/orchestrator/registry.c
    src/orchestrator/msgbus.c
    src/orchestrator/orchestrator.c
    src/memory/persistence.c
    src/tools/tools.c
)

# Objective-C sources
set(NOUS_OBJC_SOURCES
    src/metal/gpu.m
    src/neural/mlx_embed.m
    src/auth/oauth.m
    src/auth/keychain.m
    src/core/hardware.m
)

# Metal shaders
set(METAL_SHADERS
    shaders/similarity.metal
)

# Compile Metal shaders
add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/similarity.air
    COMMAND xcrun -sdk macosx metal
        -c ${CMAKE_SOURCE_DIR}/shaders/similarity.metal
        -o ${CMAKE_BINARY_DIR}/similarity.air
        -std=metal3.1
        -O3
    DEPENDS ${CMAKE_SOURCE_DIR}/shaders/similarity.metal
    COMMENT "Compiling Metal shaders"
)

add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/similarity.metallib
    COMMAND xcrun -sdk macosx metallib
        ${CMAKE_BINARY_DIR}/similarity.air
        -o ${CMAKE_BINARY_DIR}/similarity.metallib
    DEPENDS ${CMAKE_BINARY_DIR}/similarity.air
    COMMENT "Linking Metal library"
)

add_custom_target(metal_shaders
    DEPENDS ${CMAKE_BINARY_DIR}/similarity.metallib
)

# Main executable
add_executable(convergio
    ${NOUS_SOURCES}
    ${NOUS_OBJC_SOURCES}
)

add_dependencies(convergio metal_shaders)

# Link frameworks and libraries
target_link_libraries(convergio
    ${METAL_FRAMEWORK}
    ${METALKIT_FRAMEWORK}
    ${MPS_FRAMEWORK}
    ${ACCELERATE_FRAMEWORK}
    ${FOUNDATION_FRAMEWORK}
    ${SECURITY_FRAMEWORK}
    ${APPKIT_FRAMEWORK}
    ${READLINE_LIBRARY}
    ${CURL_LIBRARY}
    ${SQLITE3_LIBRARY}
    "-framework CoreFoundation"
)

if(COREML_FRAMEWORK)
    target_link_libraries(convergio ${COREML_FRAMEWORK})
    target_compile_definitions(convergio PRIVATE CONVERGIO_HAS_COREML=1)
endif()

# Objective-C ARC
set_source_files_properties(${NOUS_OBJC_SOURCES}
    PROPERTIES COMPILE_FLAGS "-fobjc-arc"
)

# Copy metal library to output directory
add_custom_command(TARGET convergio POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_BINARY_DIR}/similarity.metallib
        $<TARGET_FILE_DIR:convergio>/similarity.metallib
)

# Install target
install(TARGETS convergio
    RUNTIME DESTINATION bin
)

install(FILES ${CMAKE_BINARY_DIR}/similarity.metallib
    DESTINATION lib/convergio
)

install(DIRECTORY include/nous
    DESTINATION include/convergio
)

# Testing
enable_testing()

add_executable(test_fabric
    tests/test_fabric.c
    src/core/fabric.c
)

target_link_libraries(test_fabric
    ${ACCELERATE_FRAMEWORK}
    ${FOUNDATION_FRAMEWORK}
)

add_test(NAME fabric_test COMMAND test_fabric)

# Print configuration
message(STATUS "")
message(STATUS "=== Convergio Kernel Configuration ===")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "C Flags: ${CMAKE_C_FLAGS}")
message(STATUS "Metal Framework: ${METAL_FRAMEWORK}")
message(STATUS "Accelerate Framework: ${ACCELERATE_FRAMEWORK}")
message(STATUS "Readline: ${READLINE_LIBRARY}")
message(STATUS "")
