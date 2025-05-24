# Makefile for Micrn Editor
# Cross-platform build system for Linux, macOS, and Windows

# Program name
PROGRAM = micrn

# Compiler
CC = gcc

# Source files
SOURCES = main.c

# Object files
OBJECTS = $(SOURCES:.c=.o)

# Compiler flags
CFLAGS = -lncurses -I/usr/include/ncurses -L/usr/lib

# Platform detection
UNAME_S := $(shell uname -s 2>/dev/null || echo "Windows")

# Platform-specific settings
ifeq ($(UNAME_S),Linux)
    # Linux
    LIBS = -lncurses
    INSTALL_DIR = /usr/local/bin
    EXECUTABLE = $(PROGRAM)
endif

ifeq ($(UNAME_S),Darwin)
    # macOS
    LIBS = -lncurses
    INSTALL_DIR = /usr/local/bin
    EXECUTABLE = $(PROGRAM)
    # Homebrew ncurses path (if needed)
    ifneq ($(wildcard /opt/homebrew/include),)
        CFLAGS += -I/opt/homebrew/include
        LDFLAGS += -L/opt/homebrew/lib
    endif
endif

ifeq ($(findstring MINGW,$(UNAME_S)),MINGW)
    # Windows (MinGW/MSYS2)
    LIBS = -lncurses
    EXECUTABLE = $(PROGRAM).exe
    INSTALL_DIR = /usr/local/bin
endif

ifeq ($(findstring CYGWIN,$(UNAME_S)),CYGWIN)
    # Windows (Cygwin)
    LIBS = -lncurses
    EXECUTABLE = $(PROGRAM).exe
    INSTALL_DIR = /usr/local/bin
endif

# Default target
all: $(EXECUTABLE)

# Build the executable
$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

# Compile source files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Install system-wide
install: $(EXECUTABLE)
	@echo "Installing $(PROGRAM) to $(INSTALL_DIR)..."
	@if [ "$(UNAME_S)" = "Darwin" ] || [ "$(UNAME_S)" = "Linux" ]; then \
		sudo cp $(EXECUTABLE) $(INSTALL_DIR)/; \
		sudo chmod +x $(INSTALL_DIR)/$(EXECUTABLE); \
		echo "Installation complete. You can now run '$(PROGRAM)' from anywhere."; \
	elif [ "$(findstring MINGW,$(UNAME_S))" != "" ] || [ "$(findstring CYGWIN,$(UNAME_S))" != "" ]; then \
		cp $(EXECUTABLE) $(INSTALL_DIR)/; \
		chmod +x $(INSTALL_DIR)/$(EXECUTABLE); \
		echo "Installation complete. You can now run '$(PROGRAM)' from anywhere."; \
	else \
		echo "Unsupported platform for automatic installation."; \
		echo "Please manually copy $(EXECUTABLE) to a directory in your PATH."; \
	fi

# Install to user's local bin (doesn't require sudo)
install-user: $(EXECUTABLE)
	@echo "Installing $(PROGRAM) to user's local bin..."
	@mkdir -p $$HOME/.local/bin
	@cp $(EXECUTABLE) $$HOME/.local/bin/
	@chmod +x $$HOME/.local/bin/$(EXECUTABLE)
	@echo "Installation complete to $$HOME/.local/bin/"
	@echo "Make sure $$HOME/.local/bin is in your PATH."
	@echo "Add this line to your ~/.bashrc or ~/.zshrc:"
	@echo "export PATH=\"\$$HOME/.local/bin:\$$PATH\""

# Uninstall system-wide
uninstall:
	@echo "Uninstalling $(PROGRAM)..."
	@if [ -f "$(INSTALL_DIR)/$(EXECUTABLE)" ]; then \
		if [ "$(UNAME_S)" = "Darwin" ] || [ "$(UNAME_S)" = "Linux" ]; then \
			sudo rm -f $(INSTALL_DIR)/$(EXECUTABLE); \
		else \
			rm -f $(INSTALL_DIR)/$(EXECUTABLE); \
		fi; \
		echo "$(PROGRAM) uninstalled successfully."; \
	else \
		echo "$(PROGRAM) not found in $(INSTALL_DIR)."; \
	fi

# Uninstall from user's local bin
uninstall-user:
	@echo "Uninstalling $(PROGRAM) from user's local bin..."
	@if [ -f "$$HOME/.local/bin/$(EXECUTABLE)" ]; then \
		rm -f $$HOME/.local/bin/$(EXECUTABLE); \
		echo "$(PROGRAM) uninstalled successfully from $$HOME/.local/bin/"; \
	else \
		echo "$(PROGRAM) not found in $$HOME/.local/bin/"; \
	fi

# Clean build files
clean:
	rm -f $(OBJECTS) $(EXECUTABLE)

# Create distribution package
dist: clean
	@echo "Creating distribution package..."
	@mkdir -p dist/$(PROGRAM)
	@cp $(SOURCES) Makefile README.md install.sh install.bat dist/$(PROGRAM)/
	@cd dist && tar -czf $(PROGRAM)-source.tar.gz $(PROGRAM)/
	@echo "Distribution package created: dist/$(PROGRAM)-source.tar.gz"

# Run the program (for testing)
run: $(EXECUTABLE)
	./$(EXECUTABLE)

# Check dependencies
check-deps:
	@echo "Checking dependencies..."
	@which $(CC) > /dev/null 2>&1 || (echo "Error: $(CC) not found. Please install GCC." && exit 1)
	@echo "Checking for ncurses..."
	@if [ "$(UNAME_S)" = "Linux" ]; then \
		if ! pkg-config --exists ncurses; then \
			echo "Error: ncurses development library not found."; \
			echo "On Ubuntu/Debian: sudo apt-get install libncurses5-dev"; \
			echo "On CentOS/RHEL/Fedora: sudo yum install ncurses-devel"; \
			echo "On Arch Linux: sudo pacman -S ncurses"; \
			exit 1; \
		fi; \
	elif [ "$(UNAME_S)" = "Darwin" ]; then \
		if ! brew list ncurses > /dev/null 2>&1; then \
			echo "Warning: ncurses might not be installed via Homebrew."; \
			echo "If build fails, run: brew install ncurses"; \
		fi; \
	fi
	@echo "Dependencies check passed."

# Help target
help:
	@echo "Micrn Editor Build System"
	@echo "========================="
	@echo ""
	@echo "Available targets:"
	@echo "  all           - Build the program (default)"
	@echo "  install       - Install system-wide (requires sudo on Linux/macOS)"
	@echo "  install-user  - Install to user's local bin (no sudo required)"
	@echo "  uninstall     - Uninstall system-wide"
	@echo "  uninstall-user- Uninstall from user's local bin"
	@echo "  clean         - Remove build files"
	@echo "  dist          - Create distribution package"
	@echo "  run           - Build and run the program"
	@echo "  check-deps    - Check for required dependencies"
	@echo "  help          - Show this help message"
	@echo ""
	@echo "Platform: $(UNAME_S)"
	@echo "Compiler: $(CC)"
	@echo "Target executable: $(EXECUTABLE)"

# Phony targets
.PHONY: all install install-user uninstall uninstall-user clean dist run check-deps help

# Default target
.DEFAULT_GOAL := all
