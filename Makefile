# Convergio Kernel
# A semantic kernel for human-AI symbiosis
# Optimized for Apple Silicon (M1, M2, M3, M4)

# Version management
VERSION := $(shell cat VERSION 2>/dev/null || echo "0.0.0")

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
# Use more jobs than cores to account for I/O wait and maximize throughput
# M3 Max: 14 cores (10P + 4E), use 18-20 jobs for optimal parallelism
CPU_CORES := $(shell sysctl -n hw.ncpu 2>/dev/null || echo 14)
P_CORES := $(shell sysctl -n hw.perflevel0.physicalcpu 2>/dev/null || echo 10)
# Use 1.5x cores for optimal parallelism (accounts for I/O wait)
PARALLEL_JOBS := $(shell echo $$(( $(CPU_CORES) * 3 / 2 )) || echo 20)
MAKEFLAGS += -j$(PARALLEL_JOBS) --load-average=$(CPU_CORES)

# Apple Silicon optimizations for M3 Max
# M3 Max specific: use latest architecture features
ARCH_FLAGS = -arch arm64 -mcpu=apple-m3

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
         -DCONVERGIO_VERSION=\"$(VERSION)\"

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
              -mllvm -enable-machine-outliner=never
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
            $(SRC_DIR)/core/repl.c \
            $(SRC_DIR)/core/commands/commands.c \
            $(SRC_DIR)/core/commands/setup_wizard.c \
            $(SRC_DIR)/core/ansi_md.c \
            $(SRC_DIR)/core/stream_md.c \
            $(SRC_DIR)/core/theme.c \
            $(SRC_DIR)/core/config.c \
            $(SRC_DIR)/core/updater.c \
            $(SRC_DIR)/core/safe_path.c \
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
            $(SRC_DIR)/agentic/tool_detector.c \
            $(SRC_DIR)/agentic/tool_installer.c \
            $(SRC_DIR)/agentic/approval.c \
            $(SRC_DIR)/projects/projects.c \
            $(SRC_DIR)/todo/todo.c \
            $(SRC_DIR)/notifications/notify.c \
            $(SRC_DIR)/mcp/mcp_client.c \
            $(SRC_DIR)/workflow/workflow_types.c

OBJC_SOURCES = $(SRC_DIR)/metal/gpu.m \
               $(SRC_DIR)/neural/mlx_embed.m \
               $(SRC_DIR)/auth/oauth.m \
               $(SRC_DIR)/auth/keychain.m \
               $(SRC_DIR)/core/hardware.m \
               $(SRC_DIR)/core/clipboard.m \
               $(SRC_DIR)/providers/mlx.m

METAL_SOURCES = shaders/similarity.metal

# Object files
C_OBJECTS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(C_SOURCES))
OBJC_OBJECTS = $(patsubst $(SRC_DIR)/%.m,$(OBJ_DIR)/%.o,$(OBJC_SOURCES))
OBJECTS = $(C_OBJECTS) $(OBJC_OBJECTS)

# Metal output
METAL_AIR = $(BUILD_DIR)/similarity.air
METAL_LIB = $(BUILD_DIR)/similarity.metallib

# Target
TARGET = $(BIN_DIR)/convergio

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
# Increase Swift build parallelism
export SWIFT_BUILD_JOBS=$(PARALLEL_JOBS)
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
	@mkdir -p $(BIN_DIR)
	@mkdir -p data

# Compile Swift package (for MLX integration)
# This builds the ConvergioMLX Swift library that provides LLM inference
# Strategy: Use swift build for library linking, but also run xcodebuild to compile Metal shaders
XCODE_BUILD_DIR = .build/xcode
XCODE_RELEASE_DIR = $(XCODE_BUILD_DIR)/Build/Products/Release

swift: $(SWIFT_LIB)

$(SWIFT_LIB): Package.swift Sources/ConvergioMLX/MLXBridge.swift
	@echo "Building Swift package (MLX integration, M3 Max optimized)..."
	@swift build -c release --product ConvergioMLX \
		-Xswiftc -O -Xswiftc -whole-module-optimization \
		--jobs $(PARALLEL_JOBS) 2>&1 | grep -v "^$$" || true
	@if [ -f "$(SWIFT_LIB)" ]; then \
		echo "Swift library built: $(SWIFT_LIB)"; \
		mkdir -p $(BIN_DIR); \
		echo "Setting up Metal shaders..."; \
		xcodebuild build -scheme ConvergioMLX -configuration Release -destination 'platform=macOS' \
			-derivedDataPath $(XCODE_BUILD_DIR) >/dev/null 2>&1 || true; \
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

# Compile C sources (with cache and parallelization)
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@echo "Compiling $<..."
	@$(CC) $(CFLAGS) -c $< -o $@

# Compile Objective-C sources (with cache and parallelization)
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.m
	@echo "Compiling $<..."
	@$(OBJC) $(OBJCFLAGS) -c $< -o $@

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

# Fuzz test target - tests security functions with malformed inputs
FUZZ_TEST = $(BIN_DIR)/fuzz_test
FUZZ_SOURCES = tests/fuzz_test.c $(TEST_STUBS)
# Exclude main.o since fuzz_test has its own main() and stubs provide globals
FUZZ_OBJECTS = $(filter-out $(OBJ_DIR)/core/main.o,$(OBJECTS))

fuzz_test: dirs swift $(OBJECTS) $(MLX_STUBS_OBJ) $(FUZZ_TEST)
	@echo "Running fuzz tests..."
	@$(FUZZ_TEST)

$(FUZZ_TEST): $(FUZZ_SOURCES) $(FUZZ_OBJECTS) $(SWIFT_LIB) $(MLX_STUBS_OBJ)
	@echo "Compiling fuzz tests..."
	@if [ -s "$(SWIFT_LIB)" ]; then \
		$(CC) $(CFLAGS) $(LDFLAGS) -o $(FUZZ_TEST) $(FUZZ_SOURCES) $(FUZZ_OBJECTS) $(SWIFT_LIB) $(FRAMEWORKS) $(LIBS) $(SWIFT_RUNTIME_LIBS); \
	else \
		$(CC) $(CFLAGS) $(LDFLAGS) -o $(FUZZ_TEST) $(FUZZ_SOURCES) $(FUZZ_OBJECTS) $(MLX_STUBS_OBJ) $(FRAMEWORKS) $(LIBS); \
	fi

# Unit test target - tests core components
UNIT_TEST = $(BIN_DIR)/unit_test
UNIT_SOURCES = tests/test_unit.c $(TEST_STUBS)
UNIT_OBJECTS = $(filter-out $(OBJ_DIR)/core/main.o,$(OBJECTS))

unit_test: dirs swift $(OBJECTS) $(MLX_STUBS_OBJ) $(UNIT_TEST)
	@echo "Running unit tests..."
	@$(UNIT_TEST)

$(UNIT_TEST): $(UNIT_SOURCES) $(UNIT_OBJECTS) $(SWIFT_LIB) $(MLX_STUBS_OBJ)
	@echo "Compiling unit tests..."
	@if [ -s "$(SWIFT_LIB)" ]; then \
		$(CC) $(CFLAGS) $(LDFLAGS) -o $(UNIT_TEST) $(UNIT_SOURCES) $(UNIT_OBJECTS) $(SWIFT_LIB) $(FRAMEWORKS) $(LIBS) $(SWIFT_RUNTIME_LIBS); \
	else \
		$(CC) $(CFLAGS) $(LDFLAGS) -o $(UNIT_TEST) $(UNIT_SOURCES) $(UNIT_OBJECTS) $(MLX_STUBS_OBJ) $(FRAMEWORKS) $(LIBS); \
	fi

# Anna Executive Assistant test target - tests todo, notify, mcp_client
ANNA_TEST = $(BIN_DIR)/anna_test
ANNA_SOURCES = tests/test_anna.c $(TEST_STUBS)
ANNA_OBJECTS = $(filter-out $(OBJ_DIR)/core/main.o,$(OBJECTS))

anna_test: dirs swift $(OBJECTS) $(MLX_STUBS_OBJ) $(ANNA_TEST)
	@echo "Running Anna Executive Assistant tests..."
	@$(ANNA_TEST)

$(ANNA_TEST): $(ANNA_SOURCES) $(ANNA_OBJECTS) $(SWIFT_LIB) $(MLX_STUBS_OBJ)
	@echo "Compiling Anna tests..."
	@if [ -s "$(SWIFT_LIB)" ]; then \
		$(CC) $(CFLAGS) $(LDFLAGS) -o $(ANNA_TEST) $(ANNA_SOURCES) $(ANNA_OBJECTS) $(SWIFT_LIB) $(FRAMEWORKS) $(LIBS) $(SWIFT_RUNTIME_LIBS); \
	else \
		$(CC) $(CFLAGS) $(LDFLAGS) -o $(ANNA_TEST) $(ANNA_SOURCES) $(ANNA_OBJECTS) $(MLX_STUBS_OBJ) $(FRAMEWORKS) $(LIBS); \
	fi

# Compaction test target - tests context compaction module
COMPACTION_TEST = $(BIN_DIR)/compaction_test
COMPACTION_SOURCES = tests/test_compaction.c
# Only need compaction.o and its minimal dependencies for this test
COMPACTION_OBJECTS = $(OBJ_DIR)/context/compaction.o

compaction_test: dirs $(OBJ_DIR)/context/compaction.o $(COMPACTION_TEST)
	@echo "Running compaction tests..."
	@$(COMPACTION_TEST)

$(COMPACTION_TEST): $(COMPACTION_SOURCES) $(COMPACTION_OBJECTS)
	@echo "Compiling compaction tests..."
	@$(CC) $(CFLAGS) $(LDFLAGS) -o $(COMPACTION_TEST) $(COMPACTION_SOURCES) $(COMPACTION_OBJECTS) $(FRAMEWORKS) $(LIBS)

# Plan database test target - tests SQLite-backed plan system
PLAN_DB_TEST = $(BIN_DIR)/plan_db_test
PLAN_DB_SOURCES = tests/test_plan_db.c
PLAN_DB_OBJECTS = $(OBJ_DIR)/orchestrator/plan_db.o

plan_db_test: dirs $(OBJ_DIR)/orchestrator/plan_db.o $(PLAN_DB_TEST)
	@echo "Running plan database tests..."
	@$(PLAN_DB_TEST)

$(PLAN_DB_TEST): $(PLAN_DB_SOURCES) $(PLAN_DB_OBJECTS)
	@echo "Compiling plan database tests..."
	@$(CC) $(CFLAGS) $(LDFLAGS) -o $(PLAN_DB_TEST) $(PLAN_DB_SOURCES) $(PLAN_DB_OBJECTS) -lsqlite3 -lpthread

# Output service test target - tests centralized document generation
OUTPUT_SERVICE_TEST = $(BIN_DIR)/output_service_test
OUTPUT_SERVICE_SOURCES = tests/test_output_service.c
OUTPUT_SERVICE_OBJECTS = $(OBJ_DIR)/tools/output_service.o $(OBJ_DIR)/ui/hyperlink.o

output_service_test: dirs $(OUTPUT_SERVICE_OBJECTS) $(OUTPUT_SERVICE_TEST)
	@echo "Running output service tests..."
	@$(OUTPUT_SERVICE_TEST)

$(OUTPUT_SERVICE_TEST): $(OUTPUT_SERVICE_SOURCES) $(OUTPUT_SERVICE_OBJECTS)
	@echo "Compiling output service tests..."
	@$(CC) $(CFLAGS) $(LDFLAGS) -o $(OUTPUT_SERVICE_TEST) $(OUTPUT_SERVICE_SOURCES) $(OUTPUT_SERVICE_OBJECTS)

# Tools test target - tests tools module including web search
TOOLS_TEST = $(BIN_DIR)/tools_test
TOOLS_SOURCES = tests/test_tools.c $(TEST_STUBS)
# Need most objects for full tools testing
TOOLS_OBJECTS = $(filter-out $(OBJ_DIR)/core/main.o,$(OBJECTS))

tools_test: dirs swift $(OBJECTS) $(MLX_STUBS_OBJ) $(TOOLS_TEST)
	@echo "Running tools tests..."
	@$(TOOLS_TEST)

$(TOOLS_TEST): $(TOOLS_SOURCES) $(TOOLS_OBJECTS) $(SWIFT_LIB) $(MLX_STUBS_OBJ)
	@echo "Compiling tools tests..."
	@if [ -s "$(SWIFT_LIB)" ]; then \
		$(CC) $(CFLAGS) $(LDFLAGS) -o $(TOOLS_TEST) $(TOOLS_SOURCES) $(TOOLS_OBJECTS) $(SWIFT_LIB) $(FRAMEWORKS) $(LIBS) $(SWIFT_RUNTIME_LIBS); \
	else \
		$(CC) $(CFLAGS) $(LDFLAGS) -o $(TOOLS_TEST) $(TOOLS_SOURCES) $(TOOLS_OBJECTS) $(MLX_STUBS_OBJ) $(FRAMEWORKS) $(LIBS); \
	fi

# Web search test target - tests web search across providers
WEBSEARCH_TEST = $(BIN_DIR)/websearch_test
WEBSEARCH_SOURCES = tests/test_websearch.c $(TEST_STUBS)
WEBSEARCH_OBJECTS = $(filter-out $(OBJ_DIR)/core/main.o,$(OBJECTS))

websearch_test: dirs swift $(OBJECTS) $(MLX_STUBS_OBJ) $(WEBSEARCH_TEST)
	@echo "Running web search tests..."
	@$(WEBSEARCH_TEST)

$(WEBSEARCH_TEST): $(WEBSEARCH_SOURCES) $(WEBSEARCH_OBJECTS) $(SWIFT_LIB) $(MLX_STUBS_OBJ)
	@echo "Compiling web search tests..."
	@if [ -s "$(SWIFT_LIB)" ]; then \
		$(CC) $(CFLAGS) $(LDFLAGS) -o $(WEBSEARCH_TEST) $(WEBSEARCH_SOURCES) $(WEBSEARCH_OBJECTS) $(SWIFT_LIB) $(FRAMEWORKS) $(LIBS) $(SWIFT_RUNTIME_LIBS); \
	else \
		$(CC) $(CFLAGS) $(LDFLAGS) -o $(WEBSEARCH_TEST) $(WEBSEARCH_SOURCES) $(WEBSEARCH_OBJECTS) $(MLX_STUBS_OBJ) $(FRAMEWORKS) $(LIBS); \
	fi

# Check help documentation coverage
check-docs:
	@echo "Checking help documentation coverage..."
	@./scripts/check_help_docs.sh

# Run all tests
test: fuzz_test unit_test anna_test compaction_test plan_db_test output_service_test tools_test websearch_test check-docs
	@echo "All tests completed!"

# Coverage target - builds with coverage and runs tests
coverage: clean
	@echo "Building with coverage instrumentation..."
	@$(MAKE) COVERAGE=1 all
	@$(MAKE) COVERAGE=1 fuzz_test unit_test anna_test compaction_test plan_db_test output_service_test tools_test websearch_test
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
	@echo "  check-docs - Verify all REPL commands are documented"
	@echo "  cache-stats - Show build cache statistics"
	@echo "  hwinfo     - Show Apple Silicon hardware info"
	@echo "  version    - Show version"
	@echo "  help       - Show this message"
	@echo ""
	@echo "Variables:"
	@echo "  DEBUG=1   - Enable debug build"
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

.PHONY: all dirs metal run clean debug install uninstall hwinfo help fuzz_test unit_test anna_test plan_db_test output_service_test check-docs test version dist release convergio-acp install-acp cache-stats
