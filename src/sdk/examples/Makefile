# Makefile for ADU SDK Examples
# This Makefile demonstrates how to build applications using the ADU SDK

# Compiler and flags
CC := gcc
CFLAGS := -Wall -Wextra -std=c99 -g
LDFLAGS :=

# Use pkg-config to get SDK flags (if available)
PKG_CONFIG_EXISTS := $(shell pkg-config --exists aducsdk && echo yes)

ifeq ($(PKG_CONFIG_EXISTS),yes)
    # Use pkg-config if available
    CFLAGS += $(shell pkg-config --cflags aducsdk)
    LDFLAGS += $(shell pkg-config --libs aducsdk)
    SDK_AVAILABLE := yes
else
    # Fallback: try to use local build
    SDK_INCLUDE := ../inc
    SDK_LIB := ../../../out/src/sdk/libaducsdk.a

    ifneq ($(wildcard $(SDK_INCLUDE)/aduc/aducsdk.h),)
        ifneq ($(wildcard $(SDK_LIB)),)
            CFLAGS += -I$(SDK_INCLUDE)
            LDFLAGS += $(SDK_LIB)
            SDK_AVAILABLE := yes
        endif
    endif
endif

# Source files and targets
SOURCES := simple_status_check.c status_monitor.c
TARGETS := simple_status_check status_monitor

# Default target
.PHONY: all clean check help install

all: check $(TARGETS)

# Check if SDK is available
check:
ifndef SDK_AVAILABLE
	@echo "Error: ADU SDK not found!"
	@echo ""
	@echo "Please either:"
	@echo "1. Install the SDK system-wide:"
	@echo "   sudo cmake --build ../../out --target install"
	@echo ""
	@echo "2. Or build the SDK first:"
	@echo "   cd ../.. && ./scripts/build.sh -c"
	@echo ""
	@exit 1
endif
	@echo "ADU SDK found - building examples..."

# Individual targets
simple_status_check: simple_status_check.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

status_monitor: status_monitor.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

# Install examples (optional)
install: $(TARGETS)
	@echo "Installing examples to /usr/local/bin/"
	@sudo cp $(TARGETS) /usr/local/bin/
	@echo "Examples installed successfully"

# Clean build artifacts
clean:
	rm -f $(TARGETS)

# Show help
help:
	@echo "ADU SDK Examples Makefile"
	@echo "========================"
	@echo ""
	@echo "Targets:"
	@echo "  all                 Build all examples"
	@echo "  simple_status_check Build simple status checker"
	@echo "  status_monitor      Build continuous status monitor"
	@echo "  install             Install examples to /usr/local/bin"
	@echo "  clean               Remove built binaries"
	@echo "  check               Check if SDK is available"
	@echo "  help                Show this help message"
	@echo ""
	@echo "Usage examples:"
	@echo "  make all                    # Build all examples"
	@echo "  make simple_status_check    # Build just one example"
	@echo "  make clean all              # Clean and rebuild"
	@echo ""
ifeq ($(PKG_CONFIG_EXISTS),yes)
	@echo "SDK Status: Found via pkg-config"
	@echo "SDK Version: $(shell pkg-config --modversion aducsdk 2>/dev/null || echo 'unknown')"
else ifdef SDK_AVAILABLE
	@echo "SDK Status: Found in local build"
else
	@echo "SDK Status: NOT FOUND"
endif
