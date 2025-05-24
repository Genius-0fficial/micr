# 📝 Micrn Editor

A lightweight, terminal-based text editor with syntax highlighting. Built in C for a fast and efficient editing experience.

## ✨ Features

- 🎨 **Syntax Highlighting** - Support for HTML, CSS, C/C++, and Python
- 🔄 **Dual Buffer System** - Work with two files simultaneously
- ↩️ **Undo System** - Comprehensive undo functionality
- 🔍 **Search Functionality** - Real-time incremental search
- 📋 **Kill Ring** - Cut/copy/paste with kill ring support
- 🎯 **Mark and Region** - Select and manipulate text regions
- ⚡ **Fast Performance** - Lightweight and responsive
- 🖥️ **Cross-Platform** - Works on Linux, macOS, and Windows

##  Installation

### Prerequisites

You'll need a C compiler and the ncurses library installed on your system. Then just clone this repo with `git clone https://github.com/Genius-0fficial/micr.git`


### 🐧 Linux or  🍎 macOS

```bash
# Make the install script executable
chmod +x install.sh

# Full installation (requires sudo for system-wide install)
./install.sh

# User installation (no sudo required)
./install.sh --user

# Skip dependency installation
./install.sh --no-deps
```


### 🪟 Windows

#### Installation

```bash
# Run in MinGW/MSYS2/Cygwin terminal
install.bat

# User installation (no admin required)
install.bat --user
```

### Manual Installation

#### Prerequisites

##### 🐧Linux:

- GCC compiler
- ncurses development library
- Make utility

##### Ubuntu/Debian:

```bash
sudo apt-get install build-essential libncurses5-dev libncursesw5-dev
```

##### CentOS/RHEL/Fedora:
```bash
sudo yum install gcc ncurses-devel make
# or for newer systems:
sudo dnf install gcc ncurses-devel make
```

##### Arch Linux:

```bash
sudo pacman -S base-devel ncurses
```

##### 🍎MacOS:

```bash
# Install Xcode command line tools
xcode-select --install

# Install ncurses via Homebrew (optional)
brew install ncurses
```

##### 🪟Windows:

- Install MSYS2, MinGW-w64, or Cygwin
- Install GCC and ncurses development libraries

##### 🛠️Build and install 
1. Build the program
```bash
make
```
2. Install system-wide (requires sudo on Linux/macOS):
```bash
make install
```
3. Install to user directory (no sudo required):
```bash
make install-user
```
4. Check installation:
```bash
micrn 
```

#### Path configuration

If you used `make install-user`, ensure `~/.local/bin` is in your PATH:
```bash
# Add to ~/.bashrc, ~/.zshrc, or ~/.profile
export PATH="$HOME/.local/bin:$PATH"

# Reload your shell configuration
source ~/.bashrc
```


## 🚀 Usage

### Opening the Editor

```bash
# Start with empty buffer
micrn

# Open a specific file
micrn filename.txt

# Open with full path
micrn /path/to/your/file.py
```

### 📁 Supported File Types

The editor automatically detects syntax highlighting based on file extensions:

- 🌐 **HTML** - `.html`
- 🎨 **CSS** - `.css`
- ⚙️ **C/C++** - `.c`, `.cpp`
- 🐍 **Python** - `.py`

But for regural editing you can use any file type.

## ⌨️ Keybindings

### 📝 Basic Editing

| Key Combination    | Action                 |
| ------------------ | ---------------------- |
| `Printable chars`  | Insert character       |
| `Enter` / `Ctrl+J` | Insert newline         |
| `Backspace`        | Delete character left  |
| `Ctrl+D` / `Del`   | Delete character right |

### 🧭 Navigation

|Key Combination|Action|
|---|---|
|`Arrow Keys`|Move cursor|
|`Ctrl+P`|Move up ⬆️|
|`Ctrl+N`|Move down ⬇️|
|`Ctrl+B`|Move left ⬅️|
|`Ctrl+F`|Move right ➡️|
|`Ctrl+A`|Beginning of line 🏠|
|`Ctrl+E`|End of line 🔚|
|`Alt+B`|Backward word 📤|
|`Alt+F`|Forward word 📥|
|`Alt+{`|Backward paragraph ⬆️📄|
|`Alt+}`|Forward paragraph ⬇️📄|

### ✂️ Cut, Copy, Paste

|Key Combination|Action|
|---|---|
|`Ctrl+K`|Kill line (cut to kill-ring) ✂️|
|`Ctrl+W`|Kill region (cut selection) 🔥|
|`Ctrl+Y`|Yank (paste from kill-ring) 📋|
|`Ctrl+Space`|Set mark (start selection) 📍|

### 🗑️ Advanced Deletion

|Key Combination|Action|
|---|---|
|`Alt+Backspace`|Delete word left 🔙|
|`Alt+Delete`|Delete word right 🔜|

### 🔍 Search

|Key Combination|Action|
|---|---|
|`Ctrl+S`|Start incremental search 🔍|
|`Esc` / `Enter`|End search|
|`Backspace`|Remove last search character|

### 💾 File Operations

| Key Combination | Action         |
| --------------- | -------------- |
| `Ctrl+X Ctrl+S` | Save file 💾   |
| `Ctrl+X Ctrl+C` | Exit editor 🚪 |

### 🔧 Other Commands

|Key Combination|Action|
|---|---|
|`Ctrl+U`|Undo last action ↩️|
|`Ctrl+I`|Show editor info ℹ️|

## 🎯 Working with Regions

1. **Set Mark**: Press `Ctrl+Space` to set the mark at current cursor position 📍
2. **Move Cursor**: Navigate to the end of your desired selection 🎯
3. **Kill Region**: Press `Ctrl+W` to cut the selected region ✂️
4. **Yank**: Press `Ctrl+Y` to paste the content 📋


## 🔍 Search Feature

The incremental search allows real-time searching:

1. Press `Ctrl+S` to start search 🔍
2. Type your search query - matches are highlighted in real-time ⚡
3. Press `Esc` or `Enter` to end search 🏁
4. Use `Backspace` to modify the search query ⌫

## 🎨 Syntax Highlighting

The editor provides color-coded syntax highlighting:

- 🔵 **Keywords** - Language-specific keywords in cyan
- 🟢 **Strings** - String literals in green
- 🟡 **Comments** - Comments in yellow
- 🟣 **Numbers** - Numeric literals in magenta
- 🔴 **Preprocessor** - Preprocessor directives in red (C/C++)

## 🐛 Troubleshooting

### Common Issues

**Terminal doesn't support colors:**

```bash
# Check if your terminal supports colors
echo $TERM
# Try setting a compatible terminal
export TERM=xterm-256color
```

**Ncurses library not found:**

```bash
# Make sure ncurses development headers are installed
# Ubuntu/Debian: sudo apt install libncurses5-dev
# CentOS/RHEL: sudo yum install ncurses-devel
```

**Compilation errors:**

```bash
# Try specifying the ncurses library explicitly
gcc -o micrn main1.c -lncurses -ltinfo
```


## 🤝 Contributing

We welcome contributions! Please feel free to:

1. 🍴 Fork the repository
2. 🌿 Create a feature branch
3. 💻 Make your changes
4. 🧪 Test thoroughly
5. 📤 Submit a pull request

## 📄 License

This project is open source. Please check the LICENSE file for details.

## 👨‍💻 Author

Created by Genius, 2025

---

**Happy editing!** 🎉✨
