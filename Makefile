# Convergio Kernel
# A semantic kernel for human-AI symbiosis
# Optimized for Apple Silicon (M1, M2, M3, M4)

# Version management
VERSION := $(shell cat VERSION 2>/dev/null || echo "0.0.0")

# Compiler settings
CC = clang
OBJC = clang

# Apple Silicon optimizations (generic for all M-series chips)
ARCH_FLAGS = -arch arm64

# GNU Readline from Homebrew (NOT libedit - libedit doesn't support \001\002 markers for colors)
READLINE_PREFIX = $(shell brew --prefix readline)

# Compiler flags
CFLAGS = $(ARCH_FLAGS) \
         -std=c17 \
         -Wall -Wextra -Wpedantic \
         -Wno-unused-parameter \
         -Wno-overlength-strings \
         -ffast-math \
         -fvectorize \
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
    # Note: LTO disabled because it incorrectly eliminates tool functions
    # that are called through function pointers / dynamic paths
    CFLAGS += -O3 -DNDEBUG
    LDFLAGS +=
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

# Libraries (cJSON linked statically to avoid dylib signature issues)
# GNU readline linked explicitly from Homebrew (libedit doesn't support prompt color markers)
LIBS = -L$(READLINE_PREFIX)/lib -lreadline -lcurl -lsqlite3 /opt/homebrew/opt/cjson/lib/libcjson.a

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
            $(SRC_DIR)/orchestrator/convergence.c \
            $(SRC_DIR)/memory/persistence.c \
            $(SRC_DIR)/memory/semantic_persistence.c \
            $(SRC_DIR)/context/compaction.c \
            $(SRC_DIR)/tools/tools.c \
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
            $(SRC_DIR)/mcp/mcp_client.c

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

# Embedded agents (generated from .md files)
EMBEDDED_AGENTS = $(SRC_DIR)/agents/embedded_agents.c

# Default target - MUST be first target in file
.DEFAULT_GOAL := all

all: dirs metal swift $(TARGET)
	@echo ""
	@echo "╔═══════════════════════════════════════════════════╗"
	@echo "║          CONVERGIO KERNEL v$(VERSION)              "
	@echo "║  Build complete!                                  ║"
	@echo "║  Run with: $(TARGET)                              ║"
	@echo "╚═══════════════════════════════════════════════════╝"
	@echo ""

# Generate embedded agents if source .md files changed
$(EMBEDDED_AGENTS): $(wildcard $(SRC_DIR)/agents/definitions/*.md) scripts/embed_agents.sh
	@echo "Generating embedded agents..."
	@./scripts/embed_agents.sh

# Ensure embedded agents are generated before compiling
$(OBJ_DIR)/agents/embedded_agents.o: $(EMBEDDED_AGENTS)

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
	@mkdir -p $(BIN_DIR)
	@mkdir -p data

# Compile Swift package (for MLX integration)
# This builds the ConvergioMLX Swift library that provides LLM inference
# Strategy: Use swift build for library linking, but also run xcodebuild to compile Metal shaders
XCODE_BUILD_DIR = .build/xcode
XCODE_RELEASE_DIR = $(XCODE_BUILD_DIR)/Build/Products/Release

swift: $(SWIFT_LIB)

$(SWIFT_LIB): Package.swift Sources/ConvergioMLX/MLXBridge.swift
	@echo "Building Swift package (MLX integration)..."
	@swift build -c release --product ConvergioMLX 2>&1 | grep -v "^$$" || true
	@if [ -f "$(SWIFT_LIB)" ]; then \
		echo "Swift library built: $(SWIFT_LIB)"; \
		echo "Building Metal shaders via xcodebuild..."; \
		xcodebuild build -scheme ConvergioMLX -configuration Release -destination 'platform=macOS' \
			-derivedDataPath $(XCODE_BUILD_DIR) 2>&1 | grep -E "(BUILD SUCCEEDED|error:)" || true; \
		if [ -f "$(XCODE_RELEASE_DIR)/mlx-swift_Cmlx.bundle/Contents/Resources/default.metallib" ]; then \
			mkdir -p $(BIN_DIR); \
			cp "$(XCODE_RELEASE_DIR)/mlx-swift_Cmlx.bundle/Contents/Resources/default.metallib" "$(BIN_DIR)/"; \
			echo "Metal library copied to $(BIN_DIR)/default.metallib"; \
		else \
			echo "Warning: Metal shaders not compiled, MLX GPU acceleration may fail"; \
		fi; \
	else \
		echo "Warning: Swift build failed, MLX will be unavailable"; \
		mkdir -p $(SWIFT_BUILD_DIR); \
		touch $(SWIFT_LIB); \
	fi

# Compile Metal shaders (optional - requires Metal Toolchain)
metal: $(METAL_LIB)

$(METAL_AIR): $(METAL_SOURCES)
	@echo "Compiling Metal shaders..."
	@xcrun -sdk macosx metal -c $(METAL_SOURCES) -o $(METAL_AIR) \
		-std=metal3.1 -O3 2>/dev/null || \
		(echo "Warning: Metal Toolchain not available, skipping shader compilation" && touch $(METAL_AIR))

$(METAL_LIB): $(METAL_AIR)
	@if [ -s $(METAL_AIR) ]; then \
		echo "Linking Metal library..."; \
		xcrun -sdk macosx metallib $(METAL_AIR) -o $(METAL_LIB) && cp $(METAL_LIB) $(BIN_DIR)/; \
	else \
		echo "Skipping Metal library (shaders not compiled)"; \
		touch $(METAL_LIB); \
	fi

# Compile C sources
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@echo "Compiling $<..."
	@$(CC) $(CFLAGS) -c $< -o $@

# Compile Objective-C sources
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
	@echo "Linking $(TARGET)..."
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
	@# Install icon for notifications
	@mkdir -p ~/.convergio
	@if [ -f docs/logo/CovergioLogo.jpeg ]; then \
		cp docs/logo/CovergioLogo.jpeg ~/.convergio/icon.jpeg; \
		echo "  Notification icon installed"; \
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
	@tar -czvf dist/convergio-$(VERSION)-darwin-arm64.tar.gz \
		-C $(BIN_DIR) convergio \
		-C $(BUILD_DIR) similarity.metallib 2>/dev/null || \
		tar -czvf dist/convergio-$(VERSION)-darwin-arm64.tar.gz -C $(BIN_DIR) convergio
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

# Check help documentation coverage
check-docs:
	@echo "Checking help documentation coverage..."
	@./scripts/check_help_docs.sh

# Run all tests
test: fuzz_test unit_test anna_test compaction_test check-docs
	@echo "All tests completed!"

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
	@echo "  check-docs - Verify all REPL commands are documented"
	@echo "  hwinfo     - Show Apple Silicon hardware info"
	@echo "  version    - Show version"
	@echo "  help       - Show this message"
	@echo ""
	@echo "Variables:"
	@echo "  DEBUG=1   - Enable debug build"

.PHONY: all dirs metal run clean debug install uninstall hwinfo help fuzz_test unit_test anna_test check-docs test version dist release
