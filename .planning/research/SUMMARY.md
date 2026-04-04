# Project Research Summary

**Project:** Kanban CLI - Terminal-based kanban board
**Domain:** TUI application with C/C++ and ncurses
**Researched:** 2026-04-04
**Confidence:** HIGH

## Executive Summary

This research covers building a terminal-based kanban board for personal task management using ncurses in C/C++. Based on analysis of existing tools like Taskell (1.8k stars) and taskwarrior-tui (2k stars), the recommended approach uses **ncurses 6.4+ with wide character support (ncursesw)** for cross-platform terminal UI rendering, paired with **C11** for simplicity or **C++17** if RAII and STL containers are preferred. The architecture should follow a strict Model-View separation with an event loop pattern, storing data in **Markdown format** for git compatibility and human readability.

The key differentiator opportunity is implementing **true vim modal editing** (normal/insert modes), which no existing TUI kanban tool offers—most use single-mode shortcuts. Combined with keyboard-first navigation (hjkl), mouse support as enhancement, and local-first Markdown storage, this creates a compelling developer-focused productivity tool. The main risks center on ncurses-specific pitfalls: terminal cleanup on exit, proper refresh() sequencing, resize handling, and file corruption prevention during saves.

## Key Findings

### Recommended Stack

Based on STACK.md analysis, the stack centers on proven Unix terminal libraries with minimal dependencies. ncurses is the standard choice for TUI applications, offering built-in mouse protocol support (xterm 1006), wide character handling for Unicode, and availability across all target platforms (Linux native, macOS via Homebrew, WSL compatible). CMake 3.16+ provides modern cross-platform build management superior to Make or Autotools for this scale of project.

**Core technologies:**
- **ncurses 6.4+ (ncursesw)**: Terminal UI rendering and input handling — standard on Unix systems, proven stability, wide character support required for Unicode
- **C11 or C++17**: Implementation language — C11 for simplicity and ncurses compatibility; C++17 if RAII and STL containers desired
- **CMake 3.16+**: Build system — cross-platform standard with better dependency management than Make
- **cmark 0.31.x** (optional): Markdown parsing for task descriptions — BSD-2 licensed reference CommonMark implementation

**Platform notes:**
- Linux: Native ncurses support, full mouse protocol support
- macOS: Use Homebrew ncurses; iTerm2 recommended over Terminal.app for full mouse support
- WSL: Works identically to native Linux

**What to avoid:** PDCurses (no mouse protocol on Unix), Autotools (overkill), SQLite (overkill for single-user files), Boost (heavy dependency)

### Expected Features

From FEATURES.md analysis of existing TUI kanban tools, clear patterns emerge. Users expect vim-style navigation as primary interaction, Markdown storage for version control friendliness, and keyboard-first workflows with mouse as optional enhancement.

**Must have (table stakes):**
- 3-column layout (To Do / In Progress / Done) — kanban convention since 1940s
- Task cards with titles — basic unit of work
- Move tasks between columns (hjkl or mouse) — core kanban mechanic
- Create (n), Delete (d) tasks — entry and correction
- hjkl navigation + arrow fallback — vim standard
- Visual selection indicator — know what's active
- Markdown persistence with auto-save — data survives restart
- Graceful quit (q key) — expected UX

**Should have (competitive):**
- **Vim modal editing** (normal/insert modes) — unique differentiator, no existing tool offers this
- Multiple board support (work/personal/side projects) — in-app switching
- Task descriptions with toggle view — more context than title
- Checklists/subtasks — break down complex work
- Status bar — context at a glance
- Configuration file (~/.config/kanban-cli/config) — personalize behavior
- Mouse click to select + scroll wheel support — best of both worlds

**Defer (v2+):**
- Mouse drag-and-drop — high complexity, keyboard works fine
- Custom column layouts — can start with standard 3-column
- Search/filtering — nice-to-have for personal use
- Recurring tasks — scheduling complexity
- Tags/labels — columns provide categorization

**Anti-features (explicitly NOT building):**
- Team collaboration, network/cloud sync, due dates/reminders
- Rich text/attachments, GUI or web interface, mobile app
- OAuth/authentication (single-user local files only)

### Architecture Approach

Per ARCHITECTURE.md, the standard architecture for ncurses TUI applications uses three clear layers: UI Layer (ncurses-specific), Business Logic Layer (application controller, board/task/column managers), and Data Layer (Markdown parser, file I/O, board registry). Strict Model-View separation is essential—business logic updates models, UI layer polls models for display. This enables testing and prevents the common anti-pattern of calling `mvprintw()` directly from task management code.

**Major components:**
1. **Application Controller** — Lifecycle, event loop, mode management (normal/insert)
2. **Input Handler** — Translate keys/mouse to actions, keymap configuration
3. **Board/Task/Column Managers** — CRUD operations, data integrity
4. **UI Renderer** — Panel-based architecture, responsive layout, dialog management
5. **Storage Layer** — Atomic file writes, Markdown parser/serializer

**Build order (considering dependencies):**
1. Foundation: models → utils → markdown parser/writer → storage
2. Core Logic: board_manager → task_manager → navigation
3. UI Layer: window → panel_manager → input_handler
4. Views: task_view → column_view → board_view → dialogs
5. Integration: app controller → main

### Critical Pitfalls

From PITFALLS.md, the top issues to address:

1. **Missing refresh() calls leading to blank screens** — ncurses uses virtual screen optimization; always call `refresh()` or `wrefresh()` after output operations. Use `wnoutrefresh()` + `doupdate()` for multiple windows to reduce flicker.

2. **Terminal not restored on exit (garbage display)** — Always call `endwin()` before exiting, even on crashes/signals. Install signal handlers for SIGINT/SIGTERM and use `atexit()` for cleanup. Without this, terminal shows garbled text and requires `reset` command.

3. **Terminal resize handling failure** — Handle `KEY_RESIZE` in input loop, call `resizeterm()` or `endwin()`+`refresh()` to reinitialize, then re-layout all windows. Content cut off or crashes occur without this.

4. **File corruption on crash (Markdown storage)** — Never write directly to main data file. Use atomic writes: write to temp file, `fsync()`, then `rename()` to replace original. Keep backup copies and validate on load.

5. **Escape key ambiguity (vim keybindings)** — Escape key and Alt+key send same initial byte. Reduce `ESCDELAY` environment variable (e.g., to 25ms) for responsive vim feel, or use `nodelay()` with `notimeout()`.

## Implications for Roadmap

Based on research, suggested phase structure:

### Phase 1: Core Kanban Foundation
**Rationale:** Must establish basic TUI rendering and data persistence before any features. Dependencies: ncurses setup must come before all UI; Markdown parser before persistence.
**Delivers:** Working 3-column kanban board with CRUD operations
**Addresses (from FEATURES.md):** 3-column layout, task cards, hjkl navigation, create/delete/move tasks, Markdown persistence, mouse click selection
**Avoids (from PITFALLS.md):** Missing refresh() calls, terminal cleanup issues, color pair limits
**Research needed:** No — well-documented ncurses patterns

### Phase 2: Input Handling & Window Management
**Rationale:** Mouse support and resize handling depend on core UI being stable. Modal editing state machine builds on existing input handler.
**Delivers:** Robust input system with mouse support, terminal resize handling, vim modal editing foundation
**Uses (from STACK.md):** ncurses mouse protocol (xterm 1006), panel library for z-order
**Implements (from ARCHITECTURE.md):** Input handler, panel manager, window hierarchy
**Avoids (from PITFALLS.md):** Mouse event complexity, escape key ambiguity, resize failures
**Research needed:** Minimal — standard ncurses mouse/keyboard patterns

### Phase 3: Data Persistence & File Management
**Rationale:** Atomic file operations and multiple board support require stable data models from Phase 1. File I/O patterns are well-understood but must be implemented carefully to avoid corruption.
**Delivers:** Atomic saves with crash safety, multiple board switching, board discovery
**Addresses (from FEATURES.md):** Multiple board support, configuration file
**Avoids (from PITFALLS.md):** File corruption on crash, concurrent file access issues, buffer overflows
**Research needed:** No — standard file I/O atomic write patterns

### Phase 4: Enhanced UX & Polish
**Rationale:** Advanced features depend on all core systems being stable. Can be developed in parallel once foundation is solid.
**Delivers:** Task descriptions, checklists/subtasks, status bar, custom keybindings
**Addresses (from FEATURES.md):** Task descriptions, checklists/subtasks, status bar, configuration
**Avoids (from PITFALLS.md):** Buffer overflows in input handling, UX pitfalls (feedback, help)
**Research needed:** No — established UI patterns

### Phase 5: Advanced Features (Optional)
**Rationale:** These features have high complexity and can be deferred. Only proceed if Phase 1-4 prove successful.
**Delivers:** Custom column layouts, mouse drag-and-drop, advanced vim features
**Addresses (from FEATURES.md):** Custom columns, mouse drag-and-drop
**Research needed:** Phase 5 only if implementing drag-and-drop — needs coordinate tracking research

### Phase Ordering Rationale

- **Foundation-first:** Phase 1 establishes rendering and persistence layers that all other phases depend on. Without stable ncurses initialization and Markdown storage, subsequent features have no base.
- **Input before polish:** Phase 2 handles the tricky ncurses input quirks (mouse, resize, escape timing) before adding complex features in Phases 3-4.
- **Data safety early:** Phase 3 implements atomic writes and file locking before users have real data to lose.
- **Incremental complexity:** Each phase builds capabilities while avoiding pitfalls identified in research. No phase depends on unbuilt future phases.

### Research Flags

Phases likely needing deeper research during planning:
- **Phase 5 (Advanced):** If implementing mouse drag-and-drop, needs research on coordinate tracking algorithms and visual feedback patterns

Phases with standard patterns (skip research-phase):
- **Phase 1 (Foundation):** ncurses initialization, basic window management — extensively documented in NCURSES HOWTO
- **Phase 2 (Input):** Mouse and keyboard handling — standard patterns from ncurses man pages
- **Phase 3 (Persistence):** Atomic file writes, INI config parsing — well-established patterns
- **Phase 4 (UX):** Dialog boxes, status bars — standard TUI patterns

## Confidence Assessment

| Area | Confidence | Notes |
|------|------------|-------|
| Stack | HIGH | ncurses is mature (decades), official documentation verified at invisible-island.net, CMake is standard |
| Features | HIGH | Clear patterns from Taskell and taskwarrior-tui analysis, user expectations well-documented in open source projects |
| Architecture | HIGH | NCURSES Programming HOWTO and man pages provide authoritative patterns, lazygit/gitui provide real-world C TUI examples |
| Pitfalls | HIGH | NCURSES FAQ extensively covers common issues, personal experience corroborates |

**Overall confidence:** HIGH

All four research areas draw from authoritative sources (official ncurses documentation, established open source projects) with strong consensus. The domain (TUI applications) is mature with well-established patterns.

### Gaps to Address

1. **Drag-and-drop complexity:** Research indicates this is high-complexity with unclear value. Decision needed in Phase 5 planning whether to implement or skip.

2. **Modal editing value proposition:** While research suggests this is a unique differentiator, user validation recommended before Phase 2 implementation—consider prototype or config option.

3. **Checklist nesting depth:** Architecture supports arbitrary nesting, but UX complexity increases. Decision needed: one level (tasks → subtasks) only, or true nesting?

4. **Terminal compatibility testing:** Research identifies multiple terminal types (xterm, rxvt, gnome-terminal, tmux, iTerm2). Need test matrix during Phase 2.

5. **Custom columns UX:** How to handle column creation without cluttering UI? Research deferred this to Phase 5, needs design exploration.

## Sources

### Primary (HIGH confidence)
- [NCURSES Programming HOWTO - TLDP](https://tldp.org/HOWTO/NCURSES-Programming-HOWTO/) — Comprehensive programming patterns, initialization sequences, window management
- [ncurses official documentation - invisible-island.net](https://invisible-island.net/ncurses/) — Mouse API verification, version 6.6 features, man pages
- [NCURSES FAQ](https://invisible-island.net/ncurses/ncurses.faq.html) — Extensive troubleshooting, common pitfalls, platform issues
- [Taskell GitHub repository](https://github.com/smallhadroncollider/taskell) — Direct competitor analysis, 1.8k stars, Markdown storage patterns, vim navigation
- [taskwarrior-tui GitHub repository](https://github.com/kdheepak/taskwarrior-tui) — 2k stars, active project, vim navigation patterns
- [cmark GitHub repository](https://github.com/commonmark/cmark) — v0.31.2 current release, BSD-2 license verification

### Secondary (MEDIUM confidence)
- [lazygit source](https://github.com/jesseduffield/lazygit) — Real-world Go TUI patterns (applicable concepts)
- [gitui source](https://github.com/extrawurst/gitui) — Rust TUI patterns, component architecture reference
- [cmatrix source](https://github.com/abishekvashok/cmatrix) — Real-world ncurses project using CMake
- [Brick library (Haskell)](https://github.com/jtdaugherty/brick) — Reference patterns for viewport and layout management
- [Bubbles (Go TUI)](https://github.com/charmbracelet/bubbles) — Component patterns, different language but applicable UX concepts

### Tertiary (LOW confidence)
- Personal experience with TUI application development — Corroborates ncurses pitfalls but limited scope
- System verification: ncurses headers present in `/usr/include/curses.h` — Local environment check only

---
*Research completed: 2026-04-04*
*Ready for roadmap: yes*
