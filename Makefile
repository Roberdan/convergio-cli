# Convergio Kernel
# A semantic kernel for human-AI symbiosis
# Optimized for Apple Silicon M3 Max

# Compiler settings
CC = clang
OBJC = clang

# M3 Max optimizations
ARCH_FLAGS = -arch arm64 \
             -mcpu=apple-m3 \
             -mtune=apple-m3

# Compiler flags
CFLAGS = $(ARCH_FLAGS) \
         -std=c17 \
         -Wall -Wextra -Wpedantic \
         -Wno-unused-parameter \
         -ffast-math \
         -fvectorize \
         -I./include

OBJCFLAGS = $(CFLAGS) -fobjc-arc

# Release/Debug flags
ifeq ($(DEBUG),1)
    CFLAGS += -g -O0 -DDEBUG -fsanitize=address,undefined
    LDFLAGS += -fsanitize=address,undefined
else
    # Note: LTO disabled because it incorrectly eliminates tool functions
    # that are called through function pointers / dynamic paths
    CFLAGS += -O3 -DNDEBUG
    LDFLAGS +=
endif

# Frameworks
FRAMEWORKS = -framework Metal \
             -framework MetalKit \
             -framework MetalPerformanceShaders \
             -framework Accelerate \
             -framework Foundation \
             -framework CoreFoundation

# Libraries
LIBS = -lreadline -lcurl -lsqlite3

# Directories
SRC_DIR = src
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj
BIN_DIR = $(BUILD_DIR)/bin

# Source files
C_SOURCES = $(SRC_DIR)/core/fabric.c \
            $(SRC_DIR)/core/main.c \
            $(SRC_DIR)/core/ansi_md.c \
            $(SRC_DIR)/intent/parser.c \
            $(SRC_DIR)/intent/interpreter.c \
            $(SRC_DIR)/agents/agent.c \
            $(SRC_DIR)/spaces/space.c \
            $(SRC_DIR)/runtime/scheduler.c \
            $(SRC_DIR)/neural/claude.c \
            $(SRC_DIR)/orchestrator/cost.c \
            $(SRC_DIR)/orchestrator/registry.c \
            $(SRC_DIR)/orchestrator/msgbus.c \
            $(SRC_DIR)/orchestrator/orchestrator.c \
            $(SRC_DIR)/memory/persistence.c \
            $(SRC_DIR)/tools/tools.c

OBJC_SOURCES = $(SRC_DIR)/metal/gpu.m \
               $(SRC_DIR)/neural/mlx_embed.m

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

# Default target
all: dirs metal $(TARGET)
	@echo ""
	@echo "╔═══════════════════════════════════════════════════╗"
	@echo "║          CONVERGIO KERNEL                         ║"
	@echo "║  Build complete!                                  ║"
	@echo "║  Run with: $(TARGET)                 ║"
	@echo "╚═══════════════════════════════════════════════════╝"
	@echo ""

# Create directories
dirs:
	@mkdir -p $(OBJ_DIR)/core
	@mkdir -p $(OBJ_DIR)/intent
	@mkdir -p $(OBJ_DIR)/agents
	@mkdir -p $(OBJ_DIR)/spaces
	@mkdir -p $(OBJ_DIR)/runtime
	@mkdir -p $(OBJ_DIR)/metal
	@mkdir -p $(OBJ_DIR)/neural
	@mkdir -p $(OBJ_DIR)/orchestrator
	@mkdir -p $(OBJ_DIR)/memory
	@mkdir -p $(OBJ_DIR)/tools
	@mkdir -p $(BIN_DIR)
	@mkdir -p data

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

# Link
$(TARGET): $(OBJECTS)
	@echo "Linking $(TARGET)..."
	@$(CC) $(LDFLAGS) $(OBJECTS) -o $(TARGET) $(FRAMEWORKS) $(LIBS)

# Run
run: all
	@$(TARGET)

# Clean
clean:
	@rm -rf $(BUILD_DIR)
	@echo "Cleaned."

# Debug build
debug:
	@$(MAKE) DEBUG=1

# Install
install: all
	@echo "Installing to /usr/local/bin..."
	@sudo cp $(TARGET) /usr/local/bin/
	@sudo mkdir -p /usr/local/lib/convergio
	@sudo cp $(METAL_LIB) /usr/local/lib/convergio/
	@echo "Installed."

# Uninstall
uninstall:
	@sudo rm -f /usr/local/bin/convergio
	@sudo rm -rf /usr/local/lib/convergio
	@echo "Uninstalled."

# Show hardware info
hwinfo:
	@echo "=== M3 Max Hardware Info ==="
	@sysctl -n machdep.cpu.brand_string
	@echo "P-cores: $$(sysctl -n hw.perflevel0.physicalcpu)"
	@echo "E-cores: $$(sysctl -n hw.perflevel1.physicalcpu)"
	@echo "Memory: $$(( $$(sysctl -n hw.memsize) / 1024 / 1024 / 1024 )) GB"
	@system_profiler SPDisplaysDataType 2>/dev/null | grep -A2 "Chipset Model" | head -4

# Help
help:
	@echo "Convergio Kernel Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all       - Build Convergio (default)"
	@echo "  debug     - Build with debug symbols and sanitizers"
	@echo "  run       - Build and run Convergio"
	@echo "  clean     - Remove build artifacts"
	@echo "  install   - Install to /usr/local"
	@echo "  uninstall - Remove from /usr/local"
	@echo "  hwinfo    - Show M3 Max hardware info"
	@echo "  help      - Show this message"
	@echo ""
	@echo "Variables:"
	@echo "  DEBUG=1   - Enable debug build"

.PHONY: all dirs metal run clean debug install uninstall hwinfo help
