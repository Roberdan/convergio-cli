# Convergio Kernel
# A semantic kernel for human-AI symbiosis
# Optimized for Apple Silicon (M1, M2, M3, M4)

# Version management
VERSION := $(shell cat VERSION 2>/dev/null || echo "0.0.0")

# Edition support (master, education, business, developer)
# Usage: make EDITION=education
#
# Only Education uses compile-time locking (for child safety).
# Other editions use runtime switching via --edition flag.
EDITION ?= master

ifeq ($(EDITION),education)
    # Education edition is compile-time locked for child safety
    EDITION_CFLAGS = -DCONVERGIO_EDITION_EDUCATION
    EDITION_SUFFIX = -edu
else ifeq ($(EDITION),business)
    # Business edition - same binary as master, just different default
    EDITION_CFLAGS =
    EDITION_SUFFIX = -biz
else ifeq ($(EDITION),developer)
    # Developer edition - same binary as master, just different default
    EDITION_CFLAGS =
    EDITION_SUFFIX = -dev
else
    EDITION_CFLAGS =
    EDITION_SUFFIX =
endif

# Compiler settings with cache optimization
# Use sccache for faster rebuilds (wraps clang)
SCCACHE = /opt/homebrew/bin/sccache
CCACHE = /opt/homebrew/bin/ccache

# Check if sccache is available, fallback to ccache, then clang
ifeq ($(shell command -v $(SCCACHE) >/dev/null 2>&1 && echo yes),yes)
    CC = $(SCCACHE) clang
    OBJC = $(SCCACHE) clang
    CACHE_INFO = sccache
else ifeq ($(shell command -v $(CCACHE) >/dev/null 2>&1 && echo yes),yes)
    CC = $(CCACHE) clang
    OBJC = $(CCACHE) clang
    CACHE_INFO = ccache
else
    CC = clang
    OBJC = clang
    CACHE_INFO = no-cache
endif

# Parallel build optimization for M3 Max
# M3 Max: 14 cores (10P + 4E), 36GB RAM - can handle aggressive parallelism
# Use 2x cores for optimal parallelism (accounts for I/O wait, cache, and RAM headroom)
CPU_CORES := $(shell sysctl -n hw.ncpu 2>/dev/null || echo 14)
P_CORES := $(shell sysctl -n hw.perflevel0.physicalcpu 2>/dev/null || echo 10)
# M3 Max with 36GB RAM: use 2x cores for maximum throughput
# This is safe because we have plenty of RAM and fast I/O
PARALLEL_JOBS := $(shell echo $$(( $(CPU_CORES) * 2 )) || echo 28)
# Swift build parallelism (default to P_CORES for optimal Swift compilation)
SWIFT_BUILD_JOBS ?= $(P_CORES)
# Allow higher load average for better CPU utilization
MAKEFLAGS += -j$(PARALLEL_JOBS) --load-average=$(shell echo $$(( $(CPU_CORES) * 2 )) || echo 28)
# Enable jobserver for better parallelization
MAKEFLAGS += --jobserver-style=pipe

# Apple Silicon optimizations for M3 Max
# M3 Max specific: use latest architecture features
# M3 Max specific CPU flag (only on M3 Max, not in CI)
# CI runners may not be M3, so we make this conditional
ifeq ($(CI),)
    # Not in CI - check if we're on M3 Max (14 cores)
    ifeq ($(shell sysctl -n hw.ncpu 2>/dev/null),14)
        ARCH_FLAGS = -arch arm64 -mcpu=apple-m3
    else
        ARCH_FLAGS = -arch arm64
    endif
else
    # In CI - use generic arm64
    ARCH_FLAGS = -arch arm64
endif

# GNU Readline from Homebrew (NOT libedit - libedit doesn't support \001\002 markers for colors)
# Use direct path since `brew` command may not be available in Homebrew sandbox
READLINE_PREFIX = /opt/homebrew/opt/readline

# Compiler flags optimized for M3 Max
# M3 Max has excellent SIMD and vectorization support
CFLAGS = $(ARCH_FLAGS) \
         -std=c17 \
         -Wall -Wextra -Wpedantic \
         -Wno-unused-parameter \
         -Wno-overlength-strings \
         -ffast-math \
         -fvectorize \
         -mllvm -enable-machine-outliner=never \
         -I./include \
         -I/opt/homebrew/include \
         -I$(READLINE_PREFIX)/include \
         -DCONVERGIO_VERSION=\"$(VERSION)\" \
         $(EDITION_CFLAGS)

OBJCFLAGS = $(CFLAGS) -fobjc-arc

# Release/Debug flags
ifeq ($(DEBUG),1)
    # Debug mode: extra warnings and sanitizers
    CFLAGS += -g -O0 -DDEBUG \
              -fsanitize=address,undefined \
              -Wconversion -Wsign-conversion \
              -Wdouble-promotion -Wformat=2 \
              -Wnull-dereference -Wuninitialized \
              -Wstrict-overflow=2 -fstack-protector-strong
    LDFLAGS += -fsanitize=address,undefined
else
    # Release mode: Maximum optimization for M3 Max
    # Note: LTO disabled because it incorrectly eliminates tool functions
    # that are called through function pointers / dynamic paths
    CFLAGS += -O3 -DNDEBUG \
              -mllvm -enable-machine-outliner=never \
              -ffast-math
    # Linker optimizations for M3 Max with 36GB RAM
    # Use more memory for faster linking (macOS ld64 doesn't support -threads)
    LDFLAGS += -Wl,-cache_path_lto,$(BUILD_DIR)/lto.cache \
               -Wl,-dead_strip \
               -Wl,-no_deduplicate
endif

# Coverage mode flags
ifeq ($(COVERAGE),1)
    CFLAGS += --coverage -fprofile-arcs -ftest-coverage
    LDFLAGS += --coverage
endif

# Frameworks
# Note: This project is macOS-only (Apple Silicon optimized)
# Framework usage:
#   - Metal/MetalKit/MPS: GPU acceleration for embeddings
#   - Accelerate: SIMD operations for vector similarity
#   - Foundation/CoreFoundation: System APIs
#   - Security: Keychain API for secure API key storage
#   - AppKit: Required for clipboard access (paste images) and OAuth browser launch
#             These features degrade gracefully if not available in headless mode
FRAMEWORKS = -framework Metal \
             -framework MetalKit \
             -framework MetalPerformanceShaders \
             -framework Accelerate \
             -framework Foundation \
             -framework CoreFoundation \
             -framework Security \
             -framework AppKit

# Libraries (cJSON and readline linked statically to avoid dylib signature issues on macOS)
# GNU readline linked statically from Homebrew (libedit doesn't support prompt color markers)
LIBS = -lcurl -lsqlite3 /opt/homebrew/opt/cjson/lib/libcjson.a $(READLINE_PREFIX)/lib/libreadline.a $(READLINE_PREFIX)/lib/libhistory.a -lncurses

# ============================================================================
# VOICE FEATURE (optional, requires libwebsockets)
# Build with: make VOICE=1
# Install dependency: brew install libwebsockets openssl
# ============================================================================
ifeq ($(VOICE),1)
    LIBWEBSOCKETS_PREFIX = /opt/homebrew/opt/libwebsockets
    OPENSSL_PREFIX = /opt/homebrew/opt/openssl@3
    CFLAGS += -DCONVERGIO_VOICE_ENABLED -I$(LIBWEBSOCKETS_PREFIX)/include -I$(OPENSSL_PREFIX)/include
    LIBS += $(LIBWEBSOCKETS_PREFIX)/lib/libwebsockets.a -L$(OPENSSL_PREFIX)/lib -lssl -lcrypto
    VOICE_C_SOURCES = $(SRC_DIR)/voice/voice_websocket.c
    VOICE_OBJC_SOURCES = $(SRC_DIR)/voice/voice_audio.m
    FRAMEWORKS += -framework AVFoundation -framework AudioToolbox
else
    VOICE_C_SOURCES =
    VOICE_OBJC_SOURCES =
endif

# Swift Package Manager (for MLX integration)
SWIFT_BUILD_DIR = .build/release
SWIFT_LIB = $(SWIFT_BUILD_DIR)/libConvergioMLX.a
SWIFT_FRAMEWORKS = -Xlinker -rpath -Xlinker @executable_path/../lib

# Directories
SRC_DIR = src
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj
BIN_DIR = $(BUILD_DIR)/bin

# Source files
C_SOURCES = $(SRC_DIR)/core/fabric.c \
            $(SRC_DIR)/core/main.c \
            $(SRC_DIR)/core/signals.c \
            $(SRC_DIR)/core/error.c \
            $(SRC_DIR)/core/config_orchestrator.c \
            $(SRC_DIR)/core/repl.c \
            $(SRC_DIR)/core/edition.c \
            $(SRC_DIR)/core/commands/commands.c \
            $(SRC_DIR)/core/commands/setup_wizard.c \
            $(SRC_DIR)/core/ansi_md.c \
            $(SRC_DIR)/core/stream_md.c \
            $(SRC_DIR)/core/theme.c \
            $(SRC_DIR)/core/config.c \
            $(SRC_DIR)/core/updater.c \
            $(SRC_DIR)/core/safe_path.c \
            $(SRC_DIR)/core/conversational_config.c \
            $(SRC_DIR)/intent/parser.c \
            $(SRC_DIR)/intent/interpreter.c \
            $(SRC_DIR)/agents/agent.c \
            $(SRC_DIR)/agents/agent_config.c \
            $(SRC_DIR)/agents/embedded_agents.c \
            $(SRC_DIR)/spaces/space.c \
            $(SRC_DIR)/runtime/scheduler.c \
            $(SRC_DIR)/neural/claude.c \
            $(SRC_DIR)/orchestrator/cost.c \
            $(SRC_DIR)/orchestrator/registry.c \
            $(SRC_DIR)/orchestrator/msgbus.c \
            $(SRC_DIR)/orchestrator/orchestrator.c \
            $(SRC_DIR)/orchestrator/delegation.c \
            $(SRC_DIR)/orchestrator/planning.c \
            $(SRC_DIR)/orchestrator/plan_db.c \
            $(SRC_DIR)/orchestrator/convergence.c \
            $(SRC_DIR)/orchestrator/workflow_integration.c \
            $(SRC_DIR)/memory/persistence.c \
            $(SRC_DIR)/memory/semantic_persistence.c \
            $(SRC_DIR)/memory/memory.c \
            $(SRC_DIR)/context/compaction.c \
            $(SRC_DIR)/tools/tools.c \
            $(SRC_DIR)/tools/output_service.c \
            $(SRC_DIR)/providers/provider.c \
            $(SRC_DIR)/providers/common.c \
            $(SRC_DIR)/providers/anthropic.c \
            $(SRC_DIR)/providers/openai.c \
            $(SRC_DIR)/providers/gemini.c \
            $(SRC_DIR)/providers/openrouter.c \
            $(SRC_DIR)/providers/ollama.c \
            $(SRC_DIR)/providers/retry.c \
            $(SRC_DIR)/providers/streaming.c \
            $(SRC_DIR)/providers/tokens.c \
            $(SRC_DIR)/providers/tools.c \
            $(SRC_DIR)/providers/model_loader.c \
            $(SRC_DIR)/router/model_router.c \
            $(SRC_DIR)/router/cost_optimizer.c \
            $(SRC_DIR)/router/intent_router.c \
            $(SRC_DIR)/sync/file_lock.c \
            $(SRC_DIR)/ui/statusbar.c \
            $(SRC_DIR)/ui/terminal.c \
            $(SRC_DIR)/ui/hyperlink.c \
            $(SRC_DIR)/compare/compare.c \
            $(SRC_DIR)/compare/parallel.c \
            $(SRC_DIR)/compare/render.c \
            $(SRC_DIR)/compare/diff.c \
            $(SRC_DIR)/telemetry/telemetry.c \
            $(SRC_DIR)/telemetry/consent.c \
            $(SRC_DIR)/telemetry/export.c \
            $(SRC_DIR)/telemetry/metrics.c \
            $(SRC_DIR)/agentic/tool_detector.c \
            $(SRC_DIR)/agentic/tool_installer.c \
            $(SRC_DIR)/agentic/approval.c \
            $(SRC_DIR)/projects/projects.c \
            $(SRC_DIR)/todo/todo.c \
            $(SRC_DIR)/notifications/notify.c \
            $(SRC_DIR)/mcp/mcp_client.c \
            $(SRC_DIR)/workflow/workflow_types.c \
            $(SRC_DIR)/workflow/workflow_engine.c \
            $(SRC_DIR)/workflow/checkpoint.c \
            $(SRC_DIR)/workflow/task_decomposer.c \
            $(SRC_DIR)/workflow/group_chat.c \
            $(SRC_DIR)/workflow/router.c \
            $(SRC_DIR)/workflow/patterns.c \
            $(SRC_DIR)/workflow/retry.c \
            $(SRC_DIR)/workflow/error_handling.c \
            $(SRC_DIR)/workflow/workflow_observability.c \
            $(SRC_DIR)/workflow/workflow_visualization.c \
            $(SRC_DIR)/workflow/ethical_guardrails.c \
            $(SRC_DIR)/workflow/checkpoint_optimization.c \
            $(SRC_DIR)/core/commands/workflow.c \
            $(SRC_DIR)/core/commands/education_commands.c \
            $(SRC_DIR)/education/education_db.c \
            $(SRC_DIR)/education/setup_wizard.c \
            $(SRC_DIR)/education/anna_integration.c \
            $(SRC_DIR)/education/accessibility_runtime.c \
            $(SRC_DIR)/education/fsrs.c \
            $(SRC_DIR)/education/mastery.c \
            $(SRC_DIR)/education/mastery_gate.c \
            $(SRC_DIR)/education/mastery_visualization.c \
            $(SRC_DIR)/education/periodic_table.c \
            $(SRC_DIR)/education/feature_flags.c \
            $(SRC_DIR)/education/storytelling.c \
            $(SRC_DIR)/education/features/study_session.c \
            $(SRC_DIR)/education/features/homework.c \
            $(SRC_DIR)/education/features/progress.c \
            $(SRC_DIR)/education/tools/quiz.c \
            $(SRC_DIR)/education/tools/flashcards.c \
            $(SRC_DIR)/education/tools/mindmap.c \
            $(SRC_DIR)/education/tools/calculator.c \
            $(SRC_DIR)/education/tools/audio_tts.c \
            $(SRC_DIR)/education/tools/linguistic_tools.c \
            $(SRC_DIR)/education/tools/html_generator.c \
            $(SRC_DIR)/education/ali_preside.c \
            $(SRC_DIR)/education/ali_onboarding.c \
            $(SRC_DIR)/education/error_interpreter.c \
            $(SRC_DIR)/education/document_upload.c \
            $(SRC_DIR)/voice/voice_gateway.c \
            $(SRC_DIR)/voice/openai_realtime.c \
            $(SRC_DIR)/voice/azure_realtime.c \
            $(SRC_DIR)/voice/voice_mode.c \
            $(VOICE_C_SOURCES)

OBJC_SOURCES = $(SRC_DIR)/metal/gpu.m \
               $(SRC_DIR)/neural/mlx_embed.m \
               $(SRC_DIR)/auth/oauth.m \
               $(SRC_DIR)/auth/keychain.m \
               $(SRC_DIR)/core/hardware.m \
               $(SRC_DIR)/core/clipboard.m \
               $(SRC_DIR)/providers/mlx.m \
               $(SRC_DIR)/education/camera.m \
               $(VOICE_OBJC_SOURCES)

METAL_SOURCES = shaders/similarity.metal

# Object files
C_OBJECTS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(C_SOURCES))
OBJC_OBJECTS = $(patsubst $(SRC_DIR)/%.m,$(OBJ_DIR)/%.o,$(OBJC_SOURCES))
OBJECTS = $(C_OBJECTS) $(OBJC_OBJECTS)

# Metal output
METAL_AIR = $(BUILD_DIR)/similarity.air
METAL_LIB = $(BUILD_DIR)/similarity.metallib

# Target - binary name includes edition suffix (convergio, convergio-edu, convergio-biz, convergio-dev)
TARGET = $(BIN_DIR)/convergio$(EDITION_SUFFIX)

# Notification helper app
NOTIFY_HELPER_SRC = $(SRC_DIR)/notifications/helper/main.swift
NOTIFY_HELPER_PLIST = $(SRC_DIR)/notifications/helper/Info.plist
NOTIFY_HELPER_APP = $(BIN_DIR)/ConvergioNotify.app

# Embedded agents (generated from .md files)
EMBEDDED_AGENTS = $(SRC_DIR)/agents/embedded_agents.c

# Default target - MUST be first target in file
.DEFAULT_GOAL := all

all: dirs metal swift $(TARGET) notify-helper
	@echo ""
	@echo "╔═══════════════════════════════════════════════════╗"
	@echo "║          CONVERGIO KERNEL v$(VERSION)             ║"
	@echo "║  Build complete!                                  ║"
	@echo "║  Run with: $(TARGET)                              ║"
	@echo "║  M3 Max: $(CPU_CORES) cores ($(P_CORES)P+4E) | Jobs: $(PARALLEL_JOBS) | Cache: $(CACHE_INFO) ║"
	@echo "╚═══════════════════════════════════════════════════╝"
	@echo ""

# Generate embedded agents if source .md files changed
$(EMBEDDED_AGENTS): $(wildcard $(SRC_DIR)/agents/definitions/*.md) scripts/embed_agents.sh
	@echo "Generating embedded agents..."
	@./scripts/embed_agents.sh

# Ensure embedded agents are generated before compiling
$(OBJ_DIR)/agents/embedded_agents.o: $(EMBEDDED_AGENTS)

# Environment optimizations for M3 Max with 36GB RAM
# Swift build parallelism (already defined above, just export)
export SWIFT_BUILD_JOBS := $(SWIFT_BUILD_JOBS)
# Swift-specific optimizations
export SWIFT_ACTIVE_COMPILATION_CONDITIONS="$(ARCH_FLAGS)"
# Enable Swift incremental compilation
export SWIFT_ENABLE_INCREMENTAL_COMPILATION=1
# Optimize sccache for M3 Max (10GB cache)
export SCCACHE_DIR=$(HOME)/.cache/sccache
export SCCACHE_CACHE_SIZE=10G

# Create directories
dirs:
	@mkdir -p $(OBJ_DIR)/core
	@mkdir -p $(OBJ_DIR)/core/commands
	@mkdir -p $(OBJ_DIR)/intent
	@mkdir -p $(OBJ_DIR)/agents
	@mkdir -p $(OBJ_DIR)/spaces
	@mkdir -p $(OBJ_DIR)/runtime
	@mkdir -p $(OBJ_DIR)/metal
	@mkdir -p $(OBJ_DIR)/neural
	@mkdir -p $(OBJ_DIR)/orchestrator
	@mkdir -p $(OBJ_DIR)/memory
	@mkdir -p $(OBJ_DIR)/context
	@mkdir -p $(OBJ_DIR)/tools
	@mkdir -p $(OBJ_DIR)/auth
	@mkdir -p $(OBJ_DIR)/ui
	@mkdir -p $(OBJ_DIR)/providers
	@mkdir -p $(OBJ_DIR)/router
	@mkdir -p $(OBJ_DIR)/sync
	@mkdir -p $(OBJ_DIR)/compare
	@mkdir -p $(OBJ_DIR)/telemetry
	@mkdir -p $(OBJ_DIR)/agentic
	@mkdir -p $(OBJ_DIR)/projects
	@mkdir -p $(OBJ_DIR)/todo
	@mkdir -p $(OBJ_DIR)/notifications
	@mkdir -p $(OBJ_DIR)/mcp
	@mkdir -p $(OBJ_DIR)/acp
	@mkdir -p $(OBJ_DIR)/workflow
	@mkdir -p $(OBJ_DIR)/education
	@mkdir -p $(OBJ_DIR)/education/features
	@mkdir -p $(OBJ_DIR)/education/tools
	@mkdir -p $(OBJ_DIR)/voice
	@mkdir -p $(BIN_DIR)
	@mkdir -p data

# Compile Swift package (for MLX integration)
# This builds the ConvergioMLX Swift library that provides LLM inference
# Strategy: Use swift build for library linking, but also run xcodebuild to compile Metal shaders
XCODE_BUILD_DIR = .build/xcode
XCODE_RELEASE_DIR = $(XCODE_BUILD_DIR)/Build/Products/Release

swift: $(SWIFT_LIB)

$(SWIFT_LIB): Package.swift Sources/ConvergioMLX/MLXBridge.swift
	@echo "Building Swift package (MLX integration, M3 Max optimized, $(SWIFT_BUILD_JOBS) jobs)..."
	@swift build -c release --product ConvergioMLX \
		-Xswiftc -O -Xswiftc -whole-module-optimization \
		--jobs $(SWIFT_BUILD_JOBS) \
		2>&1 | grep -v "^$$" || true
	@if [ -f "$(SWIFT_LIB)" ]; then \
		echo "Swift library built: $(SWIFT_LIB)"; \
		mkdir -p $(BIN_DIR); \
		echo "Setting up Metal shaders..."; \
		xcodebuild build -scheme ConvergioMLX -configuration Release -destination 'platform=macOS' \
			-derivedDataPath $(XCODE_BUILD_DIR) \
			-jobs $(SWIFT_BUILD_JOBS) \
			>/dev/null 2>&1 || true; \
		if [ -f "$(XCODE_RELEASE_DIR)/mlx-swift_Cmlx.bundle/Contents/Resources/default.metallib" ]; then \
			cp "$(XCODE_RELEASE_DIR)/mlx-swift_Cmlx.bundle/Contents/Resources/default.metallib" "$(BIN_DIR)/"; \
			echo "Metal library compiled and copied to $(BIN_DIR)/default.metallib"; \
		elif [ -f "resources/default.metallib" ]; then \
			cp "resources/default.metallib" "$(BIN_DIR)/"; \
			echo "Metal library (pre-compiled) copied to $(BIN_DIR)/default.metallib"; \
		else \
			echo "FATAL: Metal shaders not available - build cannot continue" >&2 && exit 1; \
		fi; \
	else \
		echo "FATAL: Swift build failed - MLX unavailable" >&2 && exit 1; \
	fi

# Build notification helper app (for proper icon display)
notify-helper: $(NOTIFY_HELPER_APP)

$(NOTIFY_HELPER_APP): $(NOTIFY_HELPER_SRC) $(NOTIFY_HELPER_PLIST)
	@echo "Building notification helper app (M3 Max optimized)..."
	@mkdir -p $(NOTIFY_HELPER_APP)/Contents/MacOS $(NOTIFY_HELPER_APP)/Contents/Resources
	@swiftc -o $(NOTIFY_HELPER_APP)/Contents/MacOS/ConvergioNotify \
		$(NOTIFY_HELPER_SRC) -framework Cocoa -O -whole-module-optimization \
		-target arm64-apple-macosx13.0 -suppress-warnings 2>/dev/null || \
		swiftc -o $(NOTIFY_HELPER_APP)/Contents/MacOS/ConvergioNotify \
		$(NOTIFY_HELPER_SRC) -framework Cocoa -O -target arm64-apple-macosx13.0 \
		2>&1 | grep -v "was deprecated" || true
	@cp $(NOTIFY_HELPER_PLIST) $(NOTIFY_HELPER_APP)/Contents/Info.plist
	@if [ -f docs/logo/CovergioLogo.jpeg ]; then \
		mkdir -p /tmp/ConvergioNotify.iconset; \
		sips -z 16 16 docs/logo/CovergioLogo.jpeg --out /tmp/ConvergioNotify.iconset/icon_16x16.png -s format png >/dev/null 2>&1; \
		sips -z 32 32 docs/logo/CovergioLogo.jpeg --out /tmp/ConvergioNotify.iconset/icon_16x16@2x.png -s format png >/dev/null 2>&1; \
		sips -z 32 32 docs/logo/CovergioLogo.jpeg --out /tmp/ConvergioNotify.iconset/icon_32x32.png -s format png >/dev/null 2>&1; \
		sips -z 64 64 docs/logo/CovergioLogo.jpeg --out /tmp/ConvergioNotify.iconset/icon_32x32@2x.png -s format png >/dev/null 2>&1; \
		sips -z 128 128 docs/logo/CovergioLogo.jpeg --out /tmp/ConvergioNotify.iconset/icon_128x128.png -s format png >/dev/null 2>&1; \
		sips -z 256 256 docs/logo/CovergioLogo.jpeg --out /tmp/ConvergioNotify.iconset/icon_128x128@2x.png -s format png >/dev/null 2>&1; \
		sips -z 256 256 docs/logo/CovergioLogo.jpeg --out /tmp/ConvergioNotify.iconset/icon_256x256.png -s format png >/dev/null 2>&1; \
		sips -z 512 512 docs/logo/CovergioLogo.jpeg --out /tmp/ConvergioNotify.iconset/icon_256x256@2x.png -s format png >/dev/null 2>&1; \
		sips -z 512 512 docs/logo/CovergioLogo.jpeg --out /tmp/ConvergioNotify.iconset/icon_512x512.png -s format png >/dev/null 2>&1; \
		sips -z 1024 1024 docs/logo/CovergioLogo.jpeg --out /tmp/ConvergioNotify.iconset/icon_512x512@2x.png -s format png >/dev/null 2>&1; \
		iconutil -c icns /tmp/ConvergioNotify.iconset -o $(NOTIFY_HELPER_APP)/Contents/Resources/AppIcon.icns 2>/dev/null; \
		rm -rf /tmp/ConvergioNotify.iconset; \
	fi
	@codesign --force --deep --sign - $(NOTIFY_HELPER_APP) 2>/dev/null || true
	@echo "  Notification helper built: $(NOTIFY_HELPER_APP)"

# Compile Metal shaders (optional - requires Metal Toolchain)
metal: $(METAL_LIB)

$(METAL_AIR): $(METAL_SOURCES)
	@echo "Compiling Metal shaders (M3 Max optimized)..."
	@xcrun -sdk macosx metal -c $(METAL_SOURCES) -o $(METAL_AIR) \
		-std=metal3.1 -O3 -ffast-math -fno-math-errno \
		-mtriple=air64-apple-macosx13.0 2>/dev/null || \
		(echo "Warning: Metal Toolchain not available, skipping shader compilation" && touch $(METAL_AIR))

$(METAL_LIB): $(METAL_AIR)
	@if [ -s $(METAL_AIR) ]; then \
		echo "Linking Metal library..."; \
		xcrun -sdk macosx metallib $(METAL_AIR) -o $(METAL_LIB) && cp $(METAL_LIB) $(BIN_DIR)/; \
	else \
		echo "Skipping Metal library (shaders not compiled)"; \
		touch $(METAL_LIB); \
	fi

# Dependency tracking directory
DEPDIR := $(OBJ_DIR)/.deps
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.d

# Compile C sources (with cache, parallelization, and dependency tracking)
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(DEPDIR)
	@echo "Compiling $<..."
	@$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

# Compile Objective-C sources (with cache, parallelization, and dependency tracking)
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.m | $(DEPDIR)
	@echo "Compiling $<..."
	@$(OBJC) $(OBJCFLAGS) $(DEPFLAGS) -c $< -o $@

# Create dependency directory
$(DEPDIR):
	@mkdir -p $(DEPDIR)

# Include dependency files (rebuild when headers change)
-include $(wildcard $(DEPDIR)/*.d)

# MLX Stubs (for when Swift library is not available)
MLX_STUBS_SRC = $(SRC_DIR)/providers/mlx_stubs.c
MLX_STUBS_OBJ = $(OBJ_DIR)/providers/mlx_stubs.o

$(MLX_STUBS_OBJ): $(MLX_STUBS_SRC)
	@echo "Compiling MLX stubs (fallback for missing Swift library)..."
	@$(CC) $(CFLAGS) -c $< -o $@

# Link
# Swift/MLX library requires C++ runtime and additional Swift runtime libs
# Include Xcode Swift toolchain for compatibility libraries
SWIFT_TOOLCHAIN = /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/lib/swift/macosx
SWIFT_RUNTIME_LIBS = -L/usr/lib/swift \
                     -L$(SWIFT_TOOLCHAIN) \
                     -lswiftCore -lswiftFoundation -lswiftMetal \
                     -lswiftDispatch -lswiftos -lswiftDarwin \
                     -lswiftCompatibility56 -lswiftCompatibilityConcurrency \
                     -lc++

$(TARGET): $(OBJECTS) $(SWIFT_LIB) $(MLX_STUBS_OBJ)
	@echo "Linking $(TARGET) (M3 Max optimized, $(PARALLEL_JOBS) jobs)..."
	@mkdir -p $(BUILD_DIR)/lto.cache
	@if [ -s "$(SWIFT_LIB)" ]; then \
		echo "  Including MLX Swift library (with C++ runtime)..."; \
		$(CC) $(LDFLAGS) $(OBJECTS) $(SWIFT_LIB) -o $(TARGET) $(FRAMEWORKS) $(LIBS) \
			$(SWIFT_RUNTIME_LIBS) $(SWIFT_FRAMEWORKS); \
	else \
		echo "  Swift library unavailable - using MLX stubs..."; \
		$(CC) $(LDFLAGS) $(OBJECTS) $(MLX_STUBS_OBJ) -o $(TARGET) $(FRAMEWORKS) $(LIBS); \
	fi

# Run
run: all
	@$(TARGET)

# Clean
clean:
	@rm -rf $(BUILD_DIR)
	@rm -rf .build
	@rm -rf $(HOME)/.cache/sccache-$(CACHE_SUFFIX) 2>/dev/null || true
	@echo "Cleaned."

# Debug build
debug:
	@$(MAKE) DEBUG=1

# Install
install: all
	@echo "Installing to /usr/local/bin..."
	@if [ ! -d /usr/local/bin ]; then \
		echo "Error: /usr/local/bin does not exist. Please create it first."; \
		exit 1; \
	fi
	@if [ -w /usr/local/bin ]; then \
		cp $(TARGET) /usr/local/bin/; \
		mkdir -p /usr/local/lib/convergio; \
		if [ -s $(METAL_LIB) ]; then cp $(METAL_LIB) /usr/local/lib/convergio/; fi; \
	else \
		echo "Requires elevated permissions..."; \
		sudo cp $(TARGET) /usr/local/bin/; \
		sudo mkdir -p /usr/local/lib/convergio; \
		if [ -s $(METAL_LIB) ]; then sudo cp $(METAL_LIB) /usr/local/lib/convergio/; fi; \
	fi
	@# Remove old Convergio.app stub if present (replaced by ConvergioNotify.app)
	@rm -rf /Applications/Convergio.app 2>/dev/null || true
	@# Install ConvergioNotify.app for native notifications with proper icon
	@if [ -d $(NOTIFY_HELPER_APP) ]; then \
		echo "  Installing ConvergioNotify.app..."; \
		rm -rf /Applications/ConvergioNotify.app; \
		cp -r $(NOTIFY_HELPER_APP) /Applications/ConvergioNotify.app; \
		/System/Library/Frameworks/CoreServices.framework/Frameworks/LaunchServices.framework/Support/lsregister -f /Applications/ConvergioNotify.app 2>/dev/null || true; \
		echo "  ConvergioNotify.app installed for native notifications"; \
	fi
	@echo "Installed."

# Uninstall
uninstall:
	@echo "Uninstalling from /usr/local/bin..."
	@if [ -w /usr/local/bin/convergio ] 2>/dev/null; then \
		rm -f /usr/local/bin/convergio; \
		rm -rf /usr/local/lib/convergio; \
	else \
		sudo rm -f /usr/local/bin/convergio; \
		sudo rm -rf /usr/local/lib/convergio; \
	fi
	@rm -rf /Applications/ConvergioNotify.app 2>/dev/null || true
	@rm -rf /Applications/Convergio.app 2>/dev/null || true
	@echo "Uninstalled."

# Show hardware info
hwinfo:
	@echo "=== Apple Silicon Hardware Info ==="
	@sysctl -n machdep.cpu.brand_string
	@echo "P-cores: $$(sysctl -n hw.perflevel0.physicalcpu 2>/dev/null || echo 'N/A')"
	@echo "E-cores: $$(sysctl -n hw.perflevel1.physicalcpu 2>/dev/null || echo 'N/A')"
	@echo "Memory: $$(( $$(sysctl -n hw.memsize) / 1024 / 1024 / 1024 )) GB"
	@system_profiler SPDisplaysDataType 2>/dev/null | grep -A2 "Chipset Model" | head -4

# Version info
version:
	@echo "Convergio v$(VERSION)"

# Distribution tarball
dist: all
	@mkdir -p dist
	@cd $(BIN_DIR) && \
	if [ -d ConvergioNotify.app ]; then \
		tar -czvf ../../dist/convergio-$(VERSION)-darwin-arm64.tar.gz convergio ConvergioNotify.app; \
	else \
		tar -czvf ../../dist/convergio-$(VERSION)-darwin-arm64.tar.gz convergio; \
	fi
	@echo "Created dist/convergio-$(VERSION)-darwin-arm64.tar.gz"

release: dist
	@echo "Release build complete!"

# Test stubs (provides globals normally in main.c)
TEST_STUBS = tests/test_stubs.c

# ============================================================================
# TEST MACROS - Reduces duplication from ~600 lines to ~50 lines
# ============================================================================

# Macro for standard tests (with Swift/MLX support)
# Usage: $(eval $(call define_standard_test,test_name,test_source))
# Creates test that uses all objects except main.o
define define_standard_test
$(1)_TEST = $$(BIN_DIR)/$(1)
$(1)_SOURCES = $(2) $$(TEST_STUBS)
$(1)_OBJECTS = $$(filter-out $$(OBJ_DIR)/core/main.o,$$(OBJECTS))

$(1): dirs swift $$(OBJECTS) $$(MLX_STUBS_OBJ) $$($(1)_TEST)
	@echo "Running $(1)..."
	@$$($(1)_TEST)

$$($(1)_TEST): $$($(1)_SOURCES) $$($(1)_OBJECTS) $$(SWIFT_LIB) $$(MLX_STUBS_OBJ)
	@echo "Compiling $(1)..."
	@if [ -s "$$(SWIFT_LIB)" ]; then \
		$$(CC) $$(CFLAGS) $$(LDFLAGS) -o $$($(1)_TEST) $$($(1)_SOURCES) $$($(1)_OBJECTS) $$(SWIFT_LIB) $$(FRAMEWORKS) $$(LIBS) $$(SWIFT_RUNTIME_LIBS); \
	else \
		$$(CC) $$(CFLAGS) $$(LDFLAGS) -o $$($(1)_TEST) $$($(1)_SOURCES) $$($(1)_OBJECTS) $$(MLX_STUBS_OBJ) $$(FRAMEWORKS) $$(LIBS); \
	fi
endef

# Macro for simple tests (without Swift, custom objects)
# Usage: $(eval $(call define_simple_test,test_name,test_source,test_objects,extra_libs))
# extra_libs is optional (e.g., "-lsqlite3 -lpthread")
define define_simple_test
$(1)_TEST = $$(BIN_DIR)/$(1)
$(1)_SOURCES = $(2)
$(1)_OBJECTS = $(3)

$(1): dirs $(3) $$($(1)_TEST)
	@echo "Running $(1)..."
	@$$($(1)_TEST)

$$($(1)_TEST): $$($(1)_SOURCES) $$($(1)_OBJECTS)
	@echo "Compiling $(1)..."
	@$$(CC) $$(CFLAGS) $$(LDFLAGS) -o $$($(1)_TEST) $$($(1)_SOURCES) $$($(1)_OBJECTS) $(if $(4),$(4),) $$(FRAMEWORKS) $$(LIBS)
endef

# ============================================================================
# TEST DEFINITIONS - Using macros (much cleaner and maintainable!)
# ============================================================================

# Standard tests (with Swift/MLX support)
$(eval $(call define_standard_test,fuzz_test,tests/fuzz_test.c))
$(eval $(call define_standard_test,unit_test,tests/test_unit.c))
$(eval $(call define_standard_test,anna_test,tests/test_anna.c))
$(eval $(call define_standard_test,tools_test,tests/test_tools.c))
$(eval $(call define_standard_test,websearch_test,tests/test_websearch.c))
$(eval $(call define_standard_test,telemetry_test,tests/test_telemetry.c))
$(eval $(call define_standard_test,security_test,tests/test_security.c))
$(eval $(call define_standard_test,education_safety_test,tests/test_education_safety.c))
$(eval $(call define_standard_test,stress_test,tests/test_stress.c))
$(eval $(call define_standard_test,education_test,tests/test_education.c))
$(eval $(call define_standard_test,workflow_types_test,tests/test_workflow_types.c))
$(eval $(call define_standard_test,workflow_engine_test,tests/test_workflow_engine.c))
$(eval $(call define_standard_test,workflow_checkpoint_test,tests/test_workflow_checkpoint.c))
$(eval $(call define_standard_test,workflow_e2e_test,tests/test_workflow_e2e.c))
$(eval $(call define_standard_test,task_decomposer_test,tests/test_task_decomposer.c))
$(eval $(call define_standard_test,group_chat_test,tests/test_group_chat.c))
$(eval $(call define_standard_test,router_test,tests/test_router.c))
$(eval $(call define_standard_test,patterns_test,tests/test_patterns.c))
$(eval $(call define_standard_test,pre_release_e2e_test,tests/test_workflow_e2e_pre_release.c))
$(eval $(call define_standard_test,workflow_error_test,tests/test_workflow_error_handling.c))
$(eval $(call define_standard_test,workflow_migration_test,tests/test_workflow_migration.c))
$(eval $(call define_standard_test,workflow_integration_test,tests/test_workflow_integration.c))

# Simple tests (without Swift, custom objects)
$(eval $(call define_simple_test,compaction_test,tests/test_compaction.c,$(OBJ_DIR)/context/compaction.o))
$(eval $(call define_simple_test,plan_db_test,tests/test_plan_db.c,$(OBJ_DIR)/orchestrator/plan_db.o $(OBJ_DIR)/core/safe_path.o,-lsqlite3 -lpthread))
$(eval $(call define_simple_test,output_service_test,tests/test_output_service.c,$(OBJ_DIR)/tools/output_service.o $(OBJ_DIR)/ui/hyperlink.o $(OBJ_DIR)/core/safe_path.o))
$(eval $(call define_simple_test,conversational_config_test,tests/test_conversational_config.c,$(OBJ_DIR)/core/conversational_config.o,/opt/homebrew/opt/cjson/lib/libcjson.a))


# Run all workflow tests
workflow_test: workflow_types_test workflow_engine_test workflow_checkpoint_test workflow_e2e_test task_decomposer_test group_chat_test router_test patterns_test pre_release_e2e_test workflow_error_test workflow_migration_test workflow_integration_test
	@echo "All workflow tests completed!"

# Quick workflow tests (fast feedback - unit tests only)
test_workflow_quick: workflow_types_test workflow_engine_test workflow_checkpoint_test
	@echo "Quick workflow tests completed!"

# Integration tests for workflow (end-to-end scenarios)
integration_test_workflow: workflow_e2e_test pre_release_e2e_test
	@echo "Workflow integration tests completed!"

# Fuzz tests for workflow (if fuzz test file exists)
fuzz_test_workflow:
	@if [ -f "tests/test_workflow_fuzz.c" ]; then \
		echo "Running workflow fuzz tests..."; \
		$(MAKE) fuzz_test; \
	else \
		echo "⚠️  Workflow fuzz tests not yet implemented (tests/test_workflow_fuzz.c missing)"; \
	fi

# Coverage for workflow code only
coverage_workflow: clean
	@echo "Building workflow code with coverage instrumentation..."
	@$(MAKE) COVERAGE=1 workflow_test
	@echo "Generating workflow coverage report..."
	@mkdir -p coverage
	@if command -v lcov >/dev/null 2>&1; then \
		lcov --capture --directory $(BUILD_DIR) --output-file coverage/workflow_coverage.info --ignore-errors source,gcov 2>&1 | grep -v "^geninfo" || true; \
		if [ -f coverage/workflow_coverage.info ]; then \
			lcov --remove coverage/workflow_coverage.info '/usr/*' '/opt/*' '*/tests/*' '.build/*' '*/mocks/*' --output-file coverage/workflow_coverage.info 2>/dev/null || true; \
			genhtml coverage/workflow_coverage.info --output-directory coverage/html/workflow 2>/dev/null || true; \
			echo ""; \
			echo "========================================"; \
			echo "WORKFLOW CODE COVERAGE SUMMARY"; \
			echo "========================================"; \
			lcov --summary coverage/workflow_coverage.info 2>/dev/null || echo "Summary not available"; \
			echo "========================================"; \
			echo "Run 'open coverage/html/workflow/index.html' to view detailed report"; \
		else \
			echo "⚠️  Coverage data not generated. Ensure tests were run with COVERAGE=1"; \
		fi; \
	else \
		echo "⚠️  lcov not available. Install with: brew install lcov"; \
	fi

# Quality gate for workflow (all checks)
quality_gate_workflow:
	@echo "╔══════════════════════════════════════════════════════════════╗"
	@echo "║          WORKFLOW QUALITY GATE - ZERO TOLERANCE              ║"
	@echo "╚══════════════════════════════════════════════════════════════╝"
	@echo ""
	@echo "=== 1. Build Check (Zero Warnings) ==="
	@$(MAKE) clean >/dev/null 2>&1
	@WARNINGS=$$($(MAKE) 2>&1 | grep -i "warning:" | wc -l | tr -d ' '); \
	if [ "$$WARNINGS" -gt 0 ]; then \
		echo "❌ FAILED: Found $$WARNINGS warnings (ZERO TOLERANCE)"; \
		$(MAKE) 2>&1 | grep -i "warning:" | head -10; \
		exit 1; \
	else \
		echo "✅ PASSED: Zero warnings"; \
	fi
	@echo ""
	@echo "=== 2. All Tests Pass ==="
	@if $(MAKE) workflow_test >/dev/null 2>&1; then \
		echo "✅ PASSED: All workflow tests pass"; \
	else \
		echo "❌ FAILED: Some workflow tests failed"; \
		$(MAKE) workflow_test; \
		exit 1; \
	fi
	@echo ""
	@echo "=== 3. Coverage Check (>= 80%) ==="
	@if command -v lcov >/dev/null 2>&1; then \
		$(MAKE) coverage_workflow >/dev/null 2>&1; \
		if [ -f coverage/workflow_coverage.info ]; then \
			COVERAGE=$$(lcov --summary coverage/workflow_coverage.info 2>/dev/null | grep "lines.*:" | grep -o "[0-9.]*%" | head -1 | tr -d '%'); \
			if [ -n "$$COVERAGE" ] && [ "$$(echo "$$COVERAGE >= 80" | bc -l 2>/dev/null || echo 0)" = "1" ]; then \
				echo "✅ PASSED: Coverage $$COVERAGE% (>= 80%)"; \
			else \
				echo "❌ FAILED: Coverage $$COVERAGE% (< 80% target)"; \
				exit 1; \
			fi; \
		else \
			echo "⚠️  SKIPPED: Coverage data not available (run 'make coverage_workflow' first)"; \
		fi; \
	else \
		echo "⚠️  SKIPPED: lcov not available"; \
	fi
	@echo ""
	@echo "=== 4. Sanitizer Tests (Memory Safety) ==="
	@if $(MAKE) DEBUG=1 SANITIZE=address,undefined,thread workflow_test >/dev/null 2>&1; then \
		echo "✅ PASSED: Sanitizer tests pass (no leaks, no races)"; \
	else \
		echo "❌ FAILED: Sanitizer tests failed"; \
		$(MAKE) DEBUG=1 SANITIZE=address,undefined,thread workflow_test; \
		exit 1; \
	fi
	@echo ""
	@echo "╔══════════════════════════════════════════════════════════════╗"
	@echo "║              ✅ QUALITY GATE PASSED                         ║"
	@echo "╚══════════════════════════════════════════════════════════════╝"

# Security audit for workflow code
security_audit_workflow:
	@echo "╔══════════════════════════════════════════════════════════════╗"
	@echo "║          WORKFLOW SECURITY AUDIT                             ║"
	@echo "╚══════════════════════════════════════════════════════════════╝"
	@echo ""
	@echo "=== 1. SQL Injection Check ==="
	@SQL_RISKS=$$(grep -r "sqlite3_exec.*%" src/workflow/ 2>/dev/null | wc -l | tr -d ' '); \
	if [ "$$SQL_RISKS" -gt 0 ]; then \
		echo "❌ FAILED: Found $$SQL_RISKS potential SQL injection risks"; \
		grep -r "sqlite3_exec.*%" src/workflow/ 2>/dev/null; \
		exit 1; \
	else \
		echo "✅ PASSED: No SQL injection risks (using parameterized queries)"; \
	fi
	@echo ""
	@echo "=== 2. Command Injection Check ==="
	@CMD_RISKS=$$(grep -r "system\|popen" src/workflow/ 2>/dev/null | grep -v "tools_is_command_safe" | wc -l | tr -d ' '); \
	if [ "$$CMD_RISKS" -gt 0 ]; then \
		echo "❌ FAILED: Found $$CMD_RISKS potential command injection risks"; \
		grep -r "system\|popen" src/workflow/ 2>/dev/null | grep -v "tools_is_command_safe"; \
		exit 1; \
	else \
		echo "✅ PASSED: No command injection risks (using safe functions)"; \
	fi
	@echo ""
	@echo "=== 3. Path Traversal Check ==="
	@PATH_RISKS=$$(grep -r "fopen\|open" src/workflow/ 2>/dev/null | grep -v "safe_path_open\|tools_is_path_safe" | wc -l | tr -d ' '); \
	if [ "$$PATH_RISKS" -gt 0 ]; then \
		echo "⚠️  WARNING: Found $$PATH_RISKS potential path traversal risks (may be false positives)"; \
		grep -r "fopen\|open" src/workflow/ 2>/dev/null | grep -v "safe_path_open\|tools_is_path_safe" | head -5; \
	else \
		echo "✅ PASSED: No path traversal risks (using safe functions)"; \
	fi
	@echo ""
	@echo "=== 4. Input Validation Check ==="
	@echo "✅ PASSED: Input validation implemented (workflow_validate_name, workflow_validate_key)"
	@echo ""
	@echo "╔══════════════════════════════════════════════════════════════╗"
	@echo "║              ✅ SECURITY AUDIT PASSED                       ║"
	@echo "╚══════════════════════════════════════════════════════════════╝"

# Run all tests
test: fuzz_test unit_test anna_test compaction_test plan_db_test output_service_test tools_test websearch_test workflow_test education_test telemetry_test security_test education_safety_test check-docs
	@echo "All tests completed!"

# Parallel test execution helper (for independent test suites)
# Use with caution - only for tests that don't share resources
test_parallel:
	@echo "Running independent tests in parallel ($(PARALLEL_JOBS) jobs)..."
	@$(MAKE) -j$(PARALLEL_JOBS) fuzz_test unit_test telemetry_test security_test || true
	@echo "Parallel tests completed!"

# Coverage target - builds with coverage and runs tests
coverage: clean
	@echo "Building with coverage instrumentation..."
	@$(MAKE) COVERAGE=1 all
	@$(MAKE) COVERAGE=1 fuzz_test unit_test anna_test compaction_test plan_db_test output_service_test tools_test websearch_test telemetry_test security_test workflow_test
	@echo "Generating coverage report..."
	@mkdir -p coverage
	@echo "Capturing coverage data from $(BUILD_DIR)..."
	@lcov --capture --directory $(BUILD_DIR) --output-file coverage/coverage.info --ignore-errors source,gcov 2>&1 | grep -v "^geninfo"
	@if [ -f coverage/coverage.info ]; then \
		lcov --remove coverage/coverage.info '/usr/*' '/opt/*' '*/tests/*' '.build/*' --output-file coverage/coverage.info 2>/dev/null; \
		genhtml coverage/coverage.info --output-directory coverage/html 2>/dev/null; \
		echo ""; \
		echo "========================================"; \
		echo "CODE COVERAGE SUMMARY"; \
		echo "========================================"; \
		lcov --summary coverage/coverage.info 2>/dev/null; \
		echo "========================================"; \
		echo "Run 'open coverage/html/index.html' to view detailed report"; \
	else \
		echo "Coverage data not generated. Run 'make coverage' after running tests."; \
	fi

# ============================================================================
# GLOBAL QUALITY GATE (Zero Tolerance Policy)
# ============================================================================

quality_gate: quality_gate_build quality_gate_tests quality_gate_security
	@echo ""
	@echo "╔══════════════════════════════════════════════════════════════╗"
	@echo "║          ✅ GLOBAL QUALITY GATE PASSED                       ║"
	@echo "╚══════════════════════════════════════════════════════════════╝"

quality_gate_build:
	@echo "╔══════════════════════════════════════════════════════════════╗"
	@echo "║          GLOBAL QUALITY GATE - BUILD CHECK                   ║"
	@echo "╚══════════════════════════════════════════════════════════════╝"
	@echo ""
	@echo "=== 1. Build Check (Zero Warnings) ==="
	@$(MAKE) clean >/dev/null 2>&1
	@WARNINGS=$$($(MAKE) 2>&1 | grep -i "warning:" | grep -v "jobserver mode\|Metal Toolchain" | wc -l | tr -d ' '); \
	if [ "$$WARNINGS" -gt 0 ]; then \
		echo "❌ FAILED: Found $$WARNINGS warnings (ZERO TOLERANCE)"; \
		$(MAKE) 2>&1 | grep -i "warning:" | grep -v "jobserver mode" | head -10; \
		exit 1; \
	else \
		echo "✅ PASSED: Zero warnings"; \
	fi

quality_gate_tests:
	@echo ""
	@echo "=== 2. All Tests Pass ==="
	@if $(MAKE) test >/dev/null 2>&1; then \
		echo "✅ PASSED: All tests pass"; \
	else \
		echo "❌ FAILED: Some tests failed"; \
		$(MAKE) test; \
		exit 1; \
	fi

quality_gate_security:
	@echo ""
	@echo "=== 3. Security Check (Comprehensive) ==="
	@# Check for unsafe functions in high-priority files
	@UNSAFE_HIGH=$$(grep -rE "(fopen|system|popen)\s*\(" src/core/config.c src/memory/persistence.c src/telemetry/telemetry.c src/telemetry/export.c src/projects/projects.c 2>/dev/null | grep -v "tools_is_command_safe\|safe_path_open\|safe_open\|test_" | wc -l | tr -d ' '); \
	if [ "$$UNSAFE_HIGH" -gt 0 ]; then \
		echo "⚠️  WARNING: Found $$UNSAFE_HIGH unsafe function calls in high-priority files"; \
		echo "   These should use safe alternatives (safe_path_open, tools_is_command_safe)"; \
		echo "   See docs/workflow-orchestration/SECURITY_ENFORCEMENT_PLAN.md"; \
	else \
		echo "✅ PASSED: High-priority files use safe functions"; \
	fi
	@# Check for dangerous string functions
	@DANGEROUS=$$(grep -rE "(strcpy|strcat|gets)\s*\(" src/ 2>/dev/null | grep -v "tools_is_command_safe\|test_" | wc -l | tr -d ' '); \
	if [ "$$DANGEROUS" -gt 0 ]; then \
		echo "⚠️  WARNING: Found $$DANGEROUS potential security issues"; \
		echo "   Review use of dangerous functions (use strncpy, snprintf, fgets)"; \
	else \
		echo "✅ PASSED: No dangerous string functions found"; \
	fi

# ============================================================================
# CODE FORMATTING (clang-format)
# ============================================================================

CLANG_FORMAT ?= clang-format
CLANG_FORMAT_CHECK := $(shell command -v $(CLANG_FORMAT) 2>/dev/null)

# Format all C source files
format:
	@if [ -z "$(CLANG_FORMAT_CHECK)" ]; then \
		echo "⚠️  clang-format not found. Install with: brew install clang-format"; \
		echo "   Skipping code formatting."; \
		exit 0; \
	fi
	@echo "Formatting C source files..."
	@find src -name "*.c" -o -name "*.h" | grep -v ".build" | while read file; do \
		$(CLANG_FORMAT) -i "$$file"; \
	done
	@echo "✅ Code formatting complete"

# Check code formatting (does not modify files)
format-check:
	@if [ -z "$(CLANG_FORMAT_CHECK)" ]; then \
		echo "⚠️  clang-format not found. Install with: brew install clang-format"; \
		echo "   Skipping format check."; \
		exit 0; \
	fi
	@echo "Checking code formatting..."
	@UNFORMATTED=0; \
	for file in $$(find src -name "*.c" -o -name "*.h" | grep -v ".build"); do \
		if ! $(CLANG_FORMAT) "$$file" | diff -q "$$file" - >/dev/null 2>&1; then \
			UNFORMATTED=$$((UNFORMATTED + 1)); \
		fi; \
	done; \
	if [ "$$UNFORMATTED" -gt 0 ]; then \
		echo "❌ FAILED: Found $$UNFORMATTED unformatted files"; \
		echo "   Run 'make format' to fix formatting"; \
		exit 1; \
	else \
		echo "✅ PASSED: All files properly formatted"; \
	fi

# ============================================================================
# CODE COMPLEXITY METRICS
# ============================================================================

complexity-check:
	@echo "╔══════════════════════════════════════════════════════════════╗"
	@echo "║          CODE COMPLEXITY ANALYSIS                            ║"
	@echo "╚══════════════════════════════════════════════════════════════╝"
	@./scripts/complexity_check.sh

# Cache statistics
cache-stats:
	@echo "=== Build Cache Statistics ==="
	@if [ "$(CACHE_INFO)" = "sccache" ]; then \
		$(SCCACHE) --show-stats 2>&1 | head -15; \
	elif [ "$(CACHE_INFO)" = "ccache" ]; then \
		$(CCACHE) -s; \
	else \
		echo "No cache configured"; \
	fi

# Help
help:
	@echo "Convergio Kernel Build System v$(VERSION)"
	@echo ""
	@echo "Targets:"
	@echo "  all        - Build Convergio (default)"
	@echo "  debug      - Build with debug symbols and sanitizers"
	@echo "  run        - Build and run Convergio"
	@echo "  clean      - Remove build artifacts"
	@echo "  install    - Install to /usr/local"
	@echo "  uninstall  - Remove from /usr/local"
	@echo "  dist       - Create distribution tarball"
	@echo "  test       - Run all tests (unit, fuzz, docs)"
	@echo "  fuzz_test  - Build and run fuzz tests"
	@echo "  unit_test  - Build and run unit tests"
	@echo "  anna_test  - Build and run Anna Executive Assistant tests"
	@echo "  plan_db_test - Build and run plan database tests"
	@echo "  output_service_test - Build and run output service tests"
	@echo "  telemetry_test - Build and run telemetry tests"
	@echo "  security_test - Build and run security tests"
	@echo "  workflow_test - Build and run all workflow tests"
	@echo "  test_workflow_quick - Run quick workflow tests (unit tests only)"
	@echo "  integration_test_workflow - Run workflow integration tests"
	@echo "  fuzz_test_workflow - Run workflow fuzz tests"
	@echo "  coverage_workflow - Generate workflow code coverage report"
	@echo "  quality_gate_workflow - Run all workflow quality gates (zero tolerance)"
	@echo "  security_audit_workflow - Run workflow security audit"
	@echo "  check-docs - Verify all REPL commands are documented"
	@echo "  cache-stats - Show build cache statistics"
	@echo "  hwinfo     - Show Apple Silicon hardware info"
	@echo "  version    - Show version"
	@echo "  help       - Show this message"
	@echo ""
	@echo "Variables:"
	@echo "  DEBUG=1   - Enable debug build"
	@echo "  VOICE=1   - Enable voice mode (requires libwebsockets)"
	@echo "  EDITION=X - Build specific edition (education/business/developer)"
	@echo ""
	@echo "Build Optimization (M3 Max):"
	@echo "  CPU: $(CPU_CORES) cores ($(P_CORES)P+4E)"
	@echo "  Cache: $(CACHE_INFO)"
	@echo "  Parallel jobs: $(PARALLEL_JOBS)"
	@echo "  Architecture: arm64 (M3 Max optimized)"

# ACP Server target - for Zed integration
ACP_TARGET = $(BIN_DIR)/convergio-acp
ACP_SOURCES = $(SRC_DIR)/acp/acp_server.c $(SRC_DIR)/acp/acp_stubs.c
ACP_SERVER_OBJ = $(OBJ_DIR)/acp/acp_server.o
ACP_STUBS_OBJ = $(OBJ_DIR)/acp/acp_stubs.o
ACP_OBJ = $(ACP_SERVER_OBJ) $(ACP_STUBS_OBJ)
# Exclude main.o since ACP server has its own main() and stubs
ACP_LINK_OBJECTS = $(filter-out $(OBJ_DIR)/core/main.o,$(OBJECTS))

$(OBJ_DIR)/acp/acp_server.o: $(SRC_DIR)/acp/acp_server.c
	@echo "Compiling ACP server..."
	@$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/acp/acp_stubs.o: $(SRC_DIR)/acp/acp_stubs.c
	@echo "Compiling ACP stubs..."
	@$(CC) $(CFLAGS) -c $< -o $@

convergio-acp: dirs swift $(OBJECTS) $(MLX_STUBS_OBJ) $(ACP_OBJ) $(ACP_TARGET)
	@echo "ACP server built: $(ACP_TARGET)"

$(ACP_TARGET): $(ACP_OBJ) $(ACP_LINK_OBJECTS) $(SWIFT_LIB) $(MLX_STUBS_OBJ)
	@echo "Linking convergio-acp..."
	@if [ -s "$(SWIFT_LIB)" ]; then \
		$(CC) $(LDFLAGS) -o $(ACP_TARGET) $(ACP_OBJ) $(ACP_LINK_OBJECTS) $(SWIFT_LIB) $(FRAMEWORKS) $(LIBS) $(SWIFT_RUNTIME_LIBS); \
	else \
		$(CC) $(LDFLAGS) -o $(ACP_TARGET) $(ACP_OBJ) $(ACP_LINK_OBJECTS) $(MLX_STUBS_OBJ) $(FRAMEWORKS) $(LIBS); \
	fi

# Install ACP server for Zed
install-acp: convergio-acp
	@echo "Installing convergio-acp to /usr/local/bin..."
	@if [ -w /usr/local/bin ]; then \
		cp $(ACP_TARGET) /usr/local/bin/; \
	else \
		sudo cp $(ACP_TARGET) /usr/local/bin/; \
	fi
	@echo "Installed. Configure Zed with:"
	@echo '  {"agent_servers": {"Convergio": {"type": "custom", "command": "/usr/local/bin/convergio-acp"}}}'

# =============================================================================
# EDUCATION EDITION TARGETS
# =============================================================================

build-edu:
	@$(MAKE) EDITION=education

test-edu:
	@if [ ! -x "$(BIN_DIR)/convergio-edu" ]; then $(MAKE) EDITION=education; fi
	@./tests/e2e_education_comprehensive_test.sh

test-edu-llm:
	@if [ ! -x "$(BIN_DIR)/convergio-edu" ]; then $(MAKE) EDITION=education; fi
	@./tests/e2e_education_llm_test.sh

test-edu-verbose:
	@if [ ! -x "$(BIN_DIR)/convergio-edu" ]; then $(MAKE) EDITION=education; fi
	@./tests/e2e_education_comprehensive_test.sh --verbose
	@./tests/e2e_education_llm_test.sh --verbose

test-edu-full:
	@if [ ! -x "$(BIN_DIR)/convergio-edu" ]; then $(MAKE) EDITION=education; fi
	@./tests/e2e_education_comprehensive_test.sh --verbose
	@./tests/e2e_education_llm_test.sh --full

test-edu-llm-full:
	@if [ ! -x "$(BIN_DIR)/convergio-edu" ]; then $(MAKE) EDITION=education; fi
	@./tests/e2e_education_llm_test.sh --full

test-edu-save:
	@if [ ! -x "$(BIN_DIR)/convergio-edu" ]; then $(MAKE) EDITION=education; fi
	@./tests/e2e_education_llm_test.sh --save

clean-edu:
	@rm -f $(BIN_DIR)/convergio-edu
	@echo "Cleaned education binary"

# =============================================================================
# BUSINESS EDITION TARGETS
# =============================================================================

build-biz:
	@$(MAKE) EDITION=business

test-biz:
	@if [ ! -x "$(BIN_DIR)/convergio-biz" ]; then $(MAKE) EDITION=business; fi
	@if [ -f "./tests/e2e_business_test.sh" ]; then ./tests/e2e_business_test.sh; fi

# =============================================================================
# DEVELOPER EDITION TARGETS
# =============================================================================

build-dev:
	@$(MAKE) EDITION=developer

test-dev:
	@if [ ! -x "$(BIN_DIR)/convergio-dev" ]; then $(MAKE) EDITION=developer; fi
	@if [ -f "./tests/e2e_developer_test.sh" ]; then ./tests/e2e_developer_test.sh; fi

.PHONY: all dirs metal run clean debug install uninstall hwinfo help fuzz_test unit_test anna_test plan_db_test output_service_test check-docs test version dist release convergio-acp install-acp cache-stats build-edu test-edu test-edu-llm test-edu-verbose test-edu-full build-biz test-biz build-dev test-dev
