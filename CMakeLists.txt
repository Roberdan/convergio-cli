cmake_minimum_required(VERSION 3.20)

# Read version from VERSION file (same as Makefile)
file(READ "${CMAKE_SOURCE_DIR}/VERSION" VERSION_FILE)
string(STRIP "${VERSION_FILE}" PROJECT_VERSION)

project(CONVERGIO
    VERSION ${PROJECT_VERSION}
    DESCRIPTION "Convergio Kernel - A semantic kernel for human-AI symbiosis"
    LANGUAGES C OBJC
)

# Require Apple Silicon
if(NOT APPLE)
    message(FATAL_ERROR "Convergio Kernel is designed for Apple Silicon Macs only")
endif()

# Apple Silicon optimizations (M1/M2/M3/M4 compatible)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_OBJC_STANDARD 17)

# Deployment target - match the native app (14.0)
set(CMAKE_OSX_DEPLOYMENT_TARGET "14.0" CACHE STRING "Minimum macOS version")

# Architecture-specific flags for Apple Silicon (generic for M1-M4 compatibility)
# Using armv8.4-a which is the baseline for all Apple Silicon chips
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=armv8.4-a+fp16+dotprod+crypto")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mtune=apple-m1")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffast-math")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvectorize")
# Note: LTO disabled due to function pointer issues with some toolchains
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -flto")

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
    /opt/homebrew/include
)

# Source files - all C sources
set(NOUS_SOURCES
    src/core/fabric.c
    src/core/main.c
    src/core/signals.c
    src/core/repl.c
    src/core/commands/command_dispatch.c
    src/core/commands/commands_help.c
    src/core/commands/commands_core.c
    src/core/commands/commands_agent.c
    src/core/commands/commands_system.c
    src/core/commands/commands_project.c
    src/core/commands/commands_memory.c
    src/core/commands/commands_todo.c
    src/core/commands/commands_plan.c
    src/core/commands/setup_wizard.c
    src/core/ansi_md.c
    src/core/stream_md.c
    src/core/theme.c
    src/core/config.c
    src/core/edition.c
    src/core/updater.c
    src/core/safe_path.c
    src/core/conversational_config.c
    src/core/config_orchestrator.c
    src/core/error.c
    src/intent/parser.c
    src/intent/interpreter.c
    src/agents/agent.c
    src/agents/agent_config.c
    src/agents/embedded_agents.c
    src/spaces/space.c
    src/runtime/scheduler.c
    src/neural/claude.c
    src/orchestrator/cost.c
    src/orchestrator/registry.c
    src/orchestrator/msgbus.c
    src/orchestrator/orchestrator.c
    src/orchestrator/delegation.c
    src/orchestrator/planning.c
    src/orchestrator/plan_db.c
    src/orchestrator/convergence.c
    src/memory/persistence.c
    src/memory/semantic_persistence.c
    src/memory/memory.c
    src/context/compaction.c
    src/tools/tools.c
    # Multi-provider support
    src/providers/provider.c
    src/providers/anthropic.c
    src/providers/openai.c
    src/providers/gemini.c
    src/providers/openrouter.c
    src/providers/ollama.c
    src/providers/retry.c
    src/providers/streaming.c
    src/providers/tools.c
    src/providers/tokens.c
    src/providers/model_loader.c
    src/router/model_router.c
    src/router/cost_optimizer.c
    # Synchronization
    src/sync/file_lock.c
    # UI
    src/ui/hyperlink.c
    src/ui/statusbar.c
    src/ui/terminal.c
    # Compare
    src/compare/compare.c
    src/compare/parallel.c
    src/compare/render.c
    src/compare/diff.c
    # Telemetry
    src/telemetry/telemetry.c
    src/telemetry/consent.c
    src/telemetry/export.c
    # Agentic
    src/agentic/tool_detector.c
    src/agentic/tool_installer.c
    src/agentic/approval.c
    # Projects
    src/projects/projects.c
    # Anna Executive Assistant (Phase 1-3)
    src/todo/todo.c
    src/notifications/notify.c
    src/mcp/mcp_client.c
    # Workflow Orchestration
    src/core/commands/workflow.c
    src/core/commands/telemetry.c
    src/workflow/checkpoint.c
    src/workflow/checkpoint_optimization.c
    src/workflow/error_handling.c
    src/workflow/group_chat.c
    src/workflow/patterns.c
    src/workflow/retry.c
    src/workflow/router.c
    src/workflow/task_decomposer.c
    src/workflow/workflow_engine.c
    src/workflow/workflow_observability.c
    src/workflow/workflow_types.c
    src/workflow/workflow_visualization.c
    src/workflow/workflow_monitor.c
    # Voice History
    src/voice/voice_history.c
    # Education Edition
    src/education/accessibility_runtime.c
    src/education/ali_onboarding.c
    src/education/ali_preside.c
    src/education/anna_integration.c
    src/education/document_upload.c
    src/education/education_db.c
    src/education/error_interpreter.c
    src/education/feature_flags.c
    src/education/features/homework.c
    src/education/features/progress.c
    src/education/features/study_session.c
    src/education/fsrs.c
    src/education/mastery_gate.c
    src/education/mastery_visualization.c
    src/education/mastery.c
    src/education/periodic_table.c
    src/education/setup_wizard.c
    src/education/storytelling.c
    src/education/tools/audio_tts.c
    src/education/tools/calculator.c
    src/education/tools/flashcards.c
    src/education/tools/html_generator.c
    src/education/tools/linguistic_tools.c
    src/education/tools/mindmap.c
    src/education/tools/quiz.c
    src/core/commands/education_commands.c
)

# Objective-C sources
set(NOUS_OBJC_SOURCES
    src/metal/gpu.m
    src/neural/mlx_embed.m
    src/auth/oauth.m
    src/auth/keychain.m
    src/core/hardware.m
    src/core/clipboard.m
    src/providers/mlx.m
    src/providers/apple_foundation.m
    src/education/camera.m
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

# ============================================================================
# STATIC LIBRARY FOR NATIVE APP
# ============================================================================

# Library sources (everything except main.c, plus native app bridge)
set(LIB_SOURCES ${NOUS_SOURCES})
list(REMOVE_ITEM LIB_SOURCES src/core/main.c)
list(APPEND LIB_SOURCES src/native/swift_bridge.c)

# Create static library for native macOS app
add_library(convergio_lib STATIC
    ${LIB_SOURCES}
    ${NOUS_OBJC_SOURCES}
)

# Set output name to just "convergio" (produces libconvergio.a)
set_target_properties(convergio_lib PROPERTIES
    OUTPUT_NAME convergio
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)

# Library needs same frameworks
target_link_libraries(convergio_lib
    ${METAL_FRAMEWORK}
    ${METALKIT_FRAMEWORK}
    ${MPS_FRAMEWORK}
    ${ACCELERATE_FRAMEWORK}
    ${FOUNDATION_FRAMEWORK}
    ${SECURITY_FRAMEWORK}
    ${APPKIT_FRAMEWORK}
    ${CURL_LIBRARY}
    ${SQLITE3_LIBRARY}
)

if(COREML_FRAMEWORK)
    target_link_libraries(convergio_lib ${COREML_FRAMEWORK})
    target_compile_definitions(convergio_lib PRIVATE CONVERGIO_HAS_COREML=1)
endif()

# Expose public headers
target_include_directories(convergio_lib PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

add_dependencies(convergio_lib metal_shaders)

# ============================================================================
# MAIN EXECUTABLE (CLI)
# ============================================================================

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

# Install static library for native app development
install(TARGETS convergio_lib
    ARCHIVE DESTINATION lib
)

# Install public headers for native app
install(DIRECTORY include/nous
    DESTINATION include
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

# Provider tests (mock provider only - registry tests are stubbed)
add_executable(test_providers
    tests/test_providers.c
    tests/mock_provider.c
)

target_link_libraries(test_providers
    ${FOUNDATION_FRAMEWORK}
)

add_test(NAME provider_test COMMAND test_providers)

# Model router unit tests
add_executable(test_model_router
    tests/unit/test_model_router.c
    tests/unit/test_stubs.c
    src/router/model_router.c
)

target_include_directories(test_model_router PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_model_router
    ${FOUNDATION_FRAMEWORK}
    pthread
)

add_test(NAME model_router_test COMMAND test_model_router)

# Multi-provider integration tests
add_executable(test_multi_provider
    tests/integration/test_multi_provider.c
    tests/mock_provider.c
    tests/mocks/mock_anthropic.c
    tests/mocks/mock_openai.c
    tests/mocks/mock_gemini.c
    tests/mocks/mock_openrouter.c
    tests/mocks/mock_ollama.c
)

target_include_directories(test_multi_provider PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/tests
)

target_link_libraries(test_multi_provider
    ${FOUNDATION_FRAMEWORK}
)

add_test(NAME multi_provider_test COMMAND test_multi_provider)

# Provider-specific mock tests (OpenRouter, Ollama)
add_executable(test_new_providers
    tests/test_providers.c
    tests/mock_provider.c
    tests/mocks/mock_openrouter.c
    tests/mocks/mock_ollama.c
)

target_include_directories(test_new_providers PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/tests
)

target_link_libraries(test_new_providers
    ${FOUNDATION_FRAMEWORK}
)

add_test(NAME new_provider_test COMMAND test_new_providers)

# Compare module tests
add_executable(test_compare
    tests/test_compare.c
    src/compare/render.c
)

target_include_directories(test_compare PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_compare
    ${FOUNDATION_FRAMEWORK}
)

add_test(NAME compare_test COMMAND test_compare)

# Projects module tests
add_executable(test_projects
    tests/test_projects.c
    src/projects/projects.c
)

target_include_directories(test_projects PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(test_projects
    ${FOUNDATION_FRAMEWORK}
)

add_test(NAME projects_test COMMAND test_projects)

# Anna Executive Assistant tests (reuse Make target to ensure full dependency graph)
add_test(NAME anna_test
    COMMAND ${CMAKE_MAKE_PROGRAM} anna_test
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

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
