# Technology Stack

**Project:** Kanban CLI - Terminal-based kanban board
**Domain:** TUI application with C/C++ and ncurses
**Researched:** 2026-04-04
**Confidence:** HIGH

## Recommended Stack

### Core Technologies

| Technology | Version | Purpose | Why Recommended |
|------------|---------|---------|-----------------|
| ncurses | 6.4+ | Terminal UI rendering, mouse/keyboard input | Standard on Unix-like systems, proven stability, built-in mouse protocol support (xterm 1006), wide character support via ncursesw |
| C11/C++17 | C11 or C++17 | Implementation language | C11 for simplicity and ncurses compatibility; C++17 if RAII and STL containers are desired |
| CMake | 3.16+ | Build system | Cross-platform, standard for C/C++ projects, better dependency management than Make |

### ncurses Components

| Component | Purpose | Notes |
|-----------|---------|-------|
| `ncursesw` | Wide-character ncurses | Required for Unicode support; link with `-lncursesw` not `-lncurses` |
| `panel` | Window layering | Allows overlapping windows (modal dialogs over board) |
| `menu` | Menu widgets | Optional - useful for board selection menus |
| `form` | Form input handling | Optional - useful for task editing forms |

### Supporting Libraries

| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| cmark | 0.31.x | Markdown parsing | Parse task descriptions with markdown; BSD-2 licensed, reference CommonMark implementation |
| cJSON | 1.7.x | JSON serialization | Alternative lightweight storage format; if markdown parsing is deferred |

### Development Tools

| Tool | Purpose | Notes |
|------|---------|-------|
| pkg-config | Dependency detection | Use `pkg-config --cflags --libs ncursesw` for compilation flags |
| valgrind | Memory debugging | Critical for C/C++ development; check for leaks with `valgrind --leak-check=full` |
| clang-format | Code formatting | Use with `.clang-format` config for consistency |
| gdb/lldb | Debugging | Standard Unix debuggers |

## Installation

### Ubuntu/Debian

```bash
# Core dependencies
sudo apt-get install libncursesw6 libncursesw5-dev
sudo apt-get install cmake pkg-config

# Optional panel/menu/form libraries
sudo apt-get install libpanelw5 libmenuw5 libformw5

# Markdown parsing (optional)
sudo apt-get install libcmark-dev

# Build tools
sudo apt-get install build-essential valgrind
```

### macOS

```bash
# Using Homebrew
brew install ncurses cmake pkg-config cmark

# Note: macOS ships with ncurses but may be outdated
# Force linking if needed: brew link ncurses --force
```

### Compile Example

```bash
# With pkg-config
gcc -o kanban main.c $(pkg-config --cflags --libs ncursesw)

# With additional libraries
gcc -o kanban main.c $(pkg-config --cflags --libs ncursesw cmark)

# CMake approach (recommended)
mkdir build && cd build
cmake ..
make
```

## CMake Configuration

```cmake
cmake_minimum_required(VERSION 3.16)
project(kanban VERSION 1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Find ncurses with wide character support
find_package(Curses REQUIRED)
if(NOT CURSES_HAVE_NCURSES_W)
    message(FATAL_ERROR "ncursesw (wide character) support required")
endif()

# Optional: Find cmark
find_package(cmark)

add_executable(kanban src/main.c src/board.c src/task.c src/ui.c)

target_include_directories(kanban PRIVATE ${CURSES_INCLUDE_DIRS})
target_link_libraries(kanban PRIVATE ${CURSES_LIBRARIES})

if(cmark_FOUND)
    target_link_libraries(kanban PRIVATE cmark::cmark)
    target_compile_definitions(kanban PRIVATE HAS_CMARK)
endif()
```

## Alternatives Considered

| Recommended | Alternative | When to Use Alternative |
|-------------|-------------|-------------------------|
| ncurses | termbox, notcurses | Use notcurses for modern C++ API or 24-bit color; use termbox for simpler embedded systems |
| cmark | hoedown, md4c | Use md4c for faster streaming parser if large markdown files expected |
| CMake | Makefile | Use Makefile only for very simple single-file projects |
| cmark | None (custom parser) | If only basic checklists needed, manual parsing may be simpler |

## What NOT to Use

| Avoid | Why | Use Instead |
|-------|-----|-------------|
| PDCurses | No mouse protocol support on Unix, color limitations | ncursesw |
| Autotools | Overkill for this project, harder to maintain | CMake |
| Heavy XML parsers (libxml2) | Unnecessary complexity for config files | cJSON or simple line parsing |
| Full database (SQLite) | Overkill for single-user local files | Plain text/markdown files |
| Boost (C++) | Heavy dependency, increases build time | Standard library only |

## Platform Compatibility

| Platform | ncurses | Mouse Support | Notes |
|----------|---------|---------------|-------|
| Linux | Native | Full via xterm 1006 protocol | Best supported platform |
| macOS | Homebrew or system | Limited in Terminal.app | iTerm2 recommended for full mouse support |
| WSL | Native | Full | Works identically to Linux |

## Mouse Support Implementation

ncurses 6.4+ supports xterm's extended mouse protocol (1006), enabling:
- Mouse position reporting beyond 223 columns
- Button press/release tracking
- Modifier key detection (Shift, Ctrl, Alt)

```c
// Enable mouse in ncurses
mmask_t old_mask, new_mask = ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION;
mousemask(new_mask, &old_mask);

// Process mouse events
MEVENT event;
if (getmouse(&event) == OK) {
    // event.x, event.y - coordinates
    // event.bstate - button state
}
```

## Version Compatibility

| Component | Minimum | Recommended | Notes |
|-----------|---------|-------------|-------|
| ncurses | 5.9 | 6.4+ | 6.0+ required for extended mouse protocol |
| CMake | 3.10 | 3.16+ | 3.16+ has better presets support |
| cmark | 0.30 | 0.31.2+ | Latest has security fixes |
| GCC | 7.0 | 11+ | C11 support guaranteed |
| Clang | 6.0 | 14+ | Better diagnostics |

## Integration Points

### Data Flow

```
User Input (keyboard/mouse)
    ↓
ncurses input handling (getch/getmouse)
    ↓
Application state update
    ↓
File I/O (markdown persistence)
    ↓
UI re-render via ncurses
```

### Storage Format

```markdown
# Board: Project Alpha

## To Do

- [ ] Task 1
  Description with **bold** and _italic_
  - [ ] Subtask A
  - [ ] Subtask B

## In Progress

- [ ] Task 2
  Working on this now

## Done

- [x] Task 0
  Completed yesterday
```

## Sources

- [ncurses official documentation](https://invisible-island.net/ncurses/) — Mouse API verification, version 6.6 features
- [cmark GitHub repository](https://github.com/commonmark/cmark) — v0.31.2 current release, BSD-2 license
- [ncurses HOWTO - TLDP](https://tldp.org/HOWTO/NCURSES-Programming-HOWTO/) — Programming patterns
- [cmatrix source](https://github.com/abishekvashok/cmatrix) — Real-world ncurses project using CMake
- [json-c GitHub](https://github.com/json-c/json-c) — Alternative storage format
- System verification: ncurses headers present in `/usr/include/curses.h`

---
*Stack research for: TUI kanban board with ncurses*
*Researched: 2026-04-04*
