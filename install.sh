#!/bin/bash
# Micrn Editor Installation Script for Linux and macOS
# This script automates the build and installation process

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to detect the operating system
detect_os() {
    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        OS="linux"
        if command -v apt-get >/dev/null 2>&1; then
            DISTRO="debian"
        elif command -v yum >/dev/null 2>&1; then
            DISTRO="rhel"
        elif command -v pacman >/dev/null 2>&1; then
            DISTRO="arch"
        elif command -v zypper >/dev/null 2>&1; then
            DISTRO="suse"
        else
            DISTRO="unknown"
        fi
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        OS="macos"
        DISTRO="macos"
    else
        OS="unknown"
        DISTRO="unknown"
    fi
}

# Function to install dependencies
install_dependencies() {
    print_status "Installing dependencies for $OS ($DISTRO)..."
    
    case $DISTRO in
        "debian")
            print_status "Updating package list..."
            sudo apt-get update
            print_status "Installing build essentials and ncurses..."
            sudo apt-get install -y build-essential libncurses5-dev libncursesw5-dev
            ;;
        "rhel")
            print_status "Installing development tools and ncurses..."
            if command -v dnf >/dev/null 2>&1; then
                sudo dnf groupinstall -y "Development Tools"
                sudo dnf install -y ncurses-devel
            else
                sudo yum groupinstall -y "Development Tools"
                sudo yum install -y ncurses-devel
            fi
            ;;
        "arch")
            print_status "Installing base-devel and ncurses..."
            sudo pacman -S --needed --noconfirm base-devel ncurses
            ;;
        "suse")
            print_status "Installing development tools and ncurses..."
            sudo zypper install -y -t pattern devel_basis
            sudo zypper install -y ncurses-devel
            ;;
        "macos")
            if ! command -v brew >/dev/null 2>&1; then
                print_error "Homebrew not found. Please install Homebrew first:"
                print_error "https://brew.sh/"
                exit 1
            fi
            print_status "Installing ncurses via Homebrew..."
            brew install ncurses
            ;;
        *)
            print_warning "Unknown distribution. Please install GCC and ncurses development libraries manually."
            ;;
    esac
}

# Function to check if dependencies are installed
check_dependencies() {
    print_status "Checking dependencies..."
    
    # Check for GCC
    if ! command -v gcc >/dev/null 2>&1; then
        print_error "GCC not found."
        return 1
    fi
    
    # Check for make
    if ! command -v make >/dev/null 2>&1; then
        print_error "Make not found."
        return 1
    fi
    
    # Check for ncurses (try to compile a simple test)
    if ! echo '#include <ncurses.h>' | gcc -E - >/dev/null 2>&1; then
        print_error "ncurses development library not found."
        return 1
    fi
    
    print_success "All dependencies are satisfied."
    return 0
}

# Function to build the program
build_program() {
    print_status "Building Micrn Editor..."
    
    if ! make clean >/dev/null 2>&1; then
        print_warning "Clean failed, continuing anyway..."
    fi
    
    if make; then
        print_success "Build completed successfully."
    else
        print_error "Build failed."
        exit 1
    fi
}

# Function to install the program
install_program() {
    local install_type=$1
    
    if [[ "$install_type" == "user" ]]; then
        print_status "Installing to user's local bin directory..."
        make install-user
        
        # Check if ~/.local/bin is in PATH
        if [[ ":$PATH:" != *":$HOME/.local/bin:"* ]]; then
            print_warning "~/.local/bin is not in your PATH."
            print_status "Add the following line to your shell configuration file:"
            print_status "  ~/.bashrc, ~/.zshrc, or ~/.profile"
            echo ""
            echo "export PATH=\"\$HOME/.local/bin:\$PATH\""
            echo ""
            print_status "Then restart your terminal or run: source ~/.bashrc"
        fi
    else
        print_status "Installing system-wide..."
        make install
    fi
}

# Function to test the installation
test_installation() {
    print_status "Testing installation..."
    
    if command -v micrn >/dev/null 2>&1; then
        print_success "Installation successful! You can now run 'micrn' from anywhere."
        print_status "Try: micrn --help"
    else
        print_warning "Command 'micrn' not found in PATH."
        print_status "You may need to restart your terminal or update your PATH."
    fi
}

# Main installation function
main() {
    echo "=================================="
    echo "  Micrn Editor Installation Script"
    echo "=================================="
    echo ""
    
    # Detect OS
    detect_os
    print_status "Detected OS: $OS ($DISTRO)"
    
    # Parse command line arguments
    INSTALL_DEPS=true
    INSTALL_TYPE="system"
    
    while [[ $# -gt 0 ]]; do
        case $1 in
            --no-deps)
                INSTALL_DEPS=false
                shift
                ;;
            --user)
                INSTALL_TYPE="user"
                shift
                ;;
            --help|-h)
                echo "Usage: $0 [OPTIONS]"
                echo ""
                echo "Options:"
                echo "  --no-deps    Skip dependency installation"
                echo "  --user       Install to user's local bin (no sudo required)"
                echo "  --help, -h   Show this help message"
                echo ""
                echo "Examples:"
                echo "  $0                 # Full system installation"
                echo "  $0 --user          # User installation"
                echo "  $0 --no-deps      # Skip dependency check/installation"
                exit 0
                ;;
            *)
                print_error "Unknown option: $1"
                print_status "Use --help for usage information."
                exit 1
                ;;
        esac
    done
    
    # Install dependencies if requested
    if [[ "$INSTALL_DEPS" == true ]]; then
        if ! check_dependencies; then
            print_status "Installing missing dependencies..."
            install_dependencies
            
            # Check again after installation
            if ! check_dependencies; then
                print_error "Dependency installation failed."
                exit 1
            fi
        fi
    else
        print_status "Skipping dependency check as requested."
    fi
    
    # Build the program
    build_program
    
    # Install the program
    install_program "$INSTALL_TYPE"
    
    # Test installation
    test_installation
    
    print_success "Installation complete!"
    echo ""
    print_status "You can now use the Micrn Editor by typing 'micrn [filename]'"
    
    if [[ "$INSTALL_TYPE" == "system" ]]; then
        print_status "To uninstall: make uninstall"
    else
        print_status "To uninstall: make uninstall-user"
    fi
}

# Run main function with all arguments
main "$@"
