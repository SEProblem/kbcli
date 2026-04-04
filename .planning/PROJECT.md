# Kanban CLI

## What This Is

A terminal-based kanban board application using ncurses with mouse support and vim-style keyboard shortcuts. Built in C/C++ for personal task tracking with human-readable markdown storage.

## Core Value

Users can manage their tasks efficiently without leaving the terminal, using familiar vim shortcuts and optional mouse interactions.

## Requirements

### Validated

- [x] Build TUI interface with ncurses supporting mouse and keyboard input (Phase 1, 2)
- [x] Implement vim-style navigation (hjkl, d for delete, n for new) (Phase 2)
- [x] Support multiple boards with in-app switching capability (Phase 4)
- [x] Create task management (add, edit, delete, move between columns) (Phase 1)
- [x] Design markdown-based storage format for persistence (Phase 1)
- [x] Support both preset 3-column layout and empty custom boards (Phase 1, 4)
- [x] Implement task metadata - descriptions and checklists (Phase 3)
- [x] Configuration system with customizable keybindings (Phase 4)
- [x] Configurable board directory (Phase 4)

### Out of Scope

- Team collaboration features (real-time sync, multi-user) — single user tool only
- Network/cloud synchronization — local storage only
- GUI or web interface — terminal only
- Rich text formatting or attachments — plain text only
- Mobile or desktop apps — CLI/TUI only
- OAuth or authentication — local file-based

## Context

This is a personal productivity tool targeting developers who spend significant time in the terminal. The markdown storage format ensures:
- Portability across machines
- Version control friendly (git diffs are readable)
- No vendor lock-in
- Easy backup and sync via existing tools (rsync, Dropbox, etc.)

## Constraints

- **Tech stack**: C/C++ with ncurses library
- **Platform**: Unix-like systems (Linux, macOS)
- **Dependencies**: Minimal external dependencies, ncurses as primary library
- **Build system**: Simple Makefile or CMake for portability

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| Markdown storage | Human-readable, version-control friendly | — Pending |
| ncurses over alternatives | Widely available, proven, mouse support | — Pending |
| Vim keybindings | Target audience familiarity | — Pending |

## Evolution

This document evolves at phase transitions and milestone boundaries.

**After each phase transition** (via `/gsd-transition`):
1. Requirements invalidated? → Move to Out of Scope with reason
2. Requirements validated? → Move to Validated with phase reference
3. New requirements emerged? → Add to Active
4. Decisions to log? → Add to Key Decisions
5. "What This Is" still accurate? → Update if drifted

**After each milestone** (via `/gsd-complete-milestone`):
1. Full review of all sections
2. Core Value check — still the right priority?
3. Audit Out of Scope — reasons still valid?
4. Update Context with current state

## Current Milestone: v1.0 Kanban Board CLI with TUI

**Goal:** Build a terminal-based kanban board application using ncurses with mouse support and vim-style keyboard shortcuts for personal task tracking.

**Target features:**
- TUI interface using ncurses in C/C++ with mouse support for drag-and-drop
- Vim-style keyboard shortcuts (hjkl navigation, d for delete, n for new)
- Multiple board support with ability to switch between boards
- Standard 3-column layout (To Do / In Progress / Done) OR completely empty custom boards
- Tasks with title, description, and checklists (subtasks)
- Plain text/markdown-based data storage for human-readable persistence
- Create, edit, move, and delete tasks via keyboard or mouse

---
*Last updated: 2026-04-04 after Phase 4 complete*
