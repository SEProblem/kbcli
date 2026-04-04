# Roadmap: Kanban CLI v1.0

**Milestone:** v1.0 — Kanban Board CLI with TUI  
**Created:** 2025-04-04  
**Requirements:** 40 total across 7 categories  
**Phases:** 4

## Overview

This roadmap delivers a terminal-based kanban board using C/C++ and ncurses with vim-style navigation, mouse support, and Markdown persistence. Phases are designed to build incrementally: foundation first, then input handling, then data management, finally polish.

## Phases

- [ ] **Phase 1: Core Kanban Foundation** — Working 3-column board with basic CRUD and persistence
- [ ] **Phase 2: Input Handling & Modal Editing** — Vim navigation, mouse support, resize handling
- [ ] **Phase 3: Task Details & Data Safety** — Descriptions, checklists, atomic file operations
- [ ] **Phase 4: Multi-Board & Configuration** — Multiple boards, settings, customization

## Phase Details

### Phase 1: Core Kanban Foundation
**Goal**: Users can view, create, delete, and move tasks on a 3-column kanban board with automatic persistence

**Depends on**: Nothing (first phase)

**Requirements**: BRD-01, BRD-02, BRD-03, BRD-04, BRD-05, BRD-06, BRD-07, BRD-08, STO-01, STO-03, STO-04, APP-01, APP-02, APP-03

**Success Criteria** (what must be TRUE):
1. User launches app and sees 3-column kanban board (To Do / In Progress / Done)
2. User can create a new task by pressing a key and typing a title
3. User can delete a task with a single keypress
4. User can move tasks between columns and reorder within columns
5. Selected task has clear visual highlight
6. Board automatically saves to Markdown file on every change
7. User can quit gracefully with 'q' and terminal is restored to original state
8. User can relaunch app and see previous board state restored from file

**Plans**: 3 plans in 3 waves

**Plan list:**
- [ ] 01-01-PLAN.md — Data models and storage layer
- [ ] 01-02-PLAN.md — Board rendering and task CRUD
- [ ] 01-03-PLAN.md — Task movement and final integration

### Phase 2: Input Handling & Modal Editing
**Goal**: Users can navigate efficiently using vim keys and mouse with modal editing for text input

**Depends on**: Phase 1

**Requirements**: NAV-01, NAV-02, NAV-03, NAV-04, NAV-05, NAV-06, MOD-01, MOD-02, MOD-03, MOD-04, MOD-05, APP-04

**Success Criteria** (what must be TRUE):
1. User can navigate tasks using hjkl keys (h=left column, l=right column, j/k=down/up)
2. Arrow keys work as alternative navigation method
3. User can jump to top (gg) or bottom (G) of current column
4. User can jump between columns using Tab/Shift+Tab or H/L
5. Mouse click selects any task visible on screen
6. Mouse scroll wheel scrolls within a column when content overflows
7. Application has clear Normal mode (navigation/commands) and Insert mode (text editing)
8. User can press 'i' or Enter to enter Insert mode for editing task titles
9. User can press Esc to return to Normal mode from Insert mode
10. Normal mode displays visual hints for available actions
11. Terminal resize is handled gracefully without crashing or garbled display

**Plans**: TBD

### Phase 3: Task Details & Data Safety
**Goal**: Users can add rich task details (descriptions, checklists) with guaranteed data safety through atomic saves

**Depends on**: Phase 2

**Requirements**: TSK-01, TSK-02, TSK-03, TSK-04, TSK-05, TSK-06, TSK-07, STO-02

**Success Criteria** (what must be TRUE):
1. User can view task description in addition to title
2. User can toggle between compact (title-only) and detailed views
3. User can edit task descriptions in Insert mode
4. Tasks support checklists with multiple subtasks
5. User can toggle checklist items between complete/incomplete states
6. User can add new checklist items to any task
7. User can delete checklist items from tasks
8. File writes use atomic operations (temp file + fsync + rename) to prevent corruption

**Plans**: TBD

### Phase 4: Multi-Board & Configuration
**Goal**: Users can manage multiple boards and customize application behavior through configuration

**Depends on**: Phase 3

**Requirements**: MBD-01, MBD-02, MBD-03, MBD-04, MBD-05, APP-05, APP-06, APP-07

**Success Criteria** (what must be TRUE):
1. User can create new boards from within the application
2. User can switch between boards without exiting the app
3. User can view list of all available boards
4. User can delete boards they no longer need
5. Boards are stored in configurable directory (default: ~/.config/kanban-cli/boards/)
6. Configuration file exists at ~/.config/kanban-cli/config
7. User can customize keybindings via configuration file
8. User can set default board directory path via configuration

**Plans**: TBD

## Progress

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 1. Core Kanban Foundation | 0/3 | ✓ Planned | - |
| 2. Input Handling & Modal Editing | 0/3 | Not started | - |
| 3. Task Details & Data Safety | 0/2 | Not started | - |
| 4. Multi-Board & Configuration | 0/2 | Not started | - |

## Requirement Coverage

### Coverage by Phase

| Phase | Requirement Count | Categories |
|-------|-------------------|------------|
| Phase 1 | 14 | BRD (8), STO (3), APP (3) |
| Phase 2 | 12 | NAV (6), MOD (5), APP (1) |
| Phase 3 | 8 | TSK (7), STO (1) |
| Phase 4 | 6 | MBD (5), APP (3) |
| **Total** | **40** | **7 categories** |

### Traceability Summary

**Phase 1: Core Kanban Foundation**
- BRD-01, BRD-02, BRD-03, BRD-04, BRD-05, BRD-06, BRD-07, BRD-08
- STO-01, STO-03, STO-04
- APP-01, APP-02, APP-03

**Phase 2: Input Handling & Modal Editing**
- NAV-01, NAV-02, NAV-03, NAV-04, NAV-05, NAV-06
- MOD-01, MOD-02, MOD-03, MOD-04, MOD-05
- APP-04

**Phase 3: Task Details & Data Safety**
- TSK-01, TSK-02, TSK-03, TSK-04, TSK-05, TSK-06, TSK-07
- STO-02

**Phase 4: Multi-Board & Configuration**
- MBD-01, MBD-02, MBD-03, MBD-04, MBD-05
- APP-05, APP-06, APP-07

**Coverage Validation**: ✓ All 40 v1 requirements mapped to exactly one phase

## Dependencies

```
Phase 1 (Foundation)
    ↓
Phase 2 (Input & Modal)
    ↓
Phase 3 (Task Details)
    ↓
Phase 4 (Multi-Board)
```

**Key Dependencies:**
- Phase 2 requires Phase 1 (must have UI before adding navigation)
- Phase 3 requires Phase 2 (task editing needs input handling)
- Phase 4 requires Phase 3 (boards need tasks to be useful)

## Notes

### Phase Ordering Rationale

**Foundation-first approach**: Phase 1 establishes rendering and persistence layers that all other phases depend on. Without stable ncurses initialization and Markdown storage, subsequent features have no base.

**Input before polish**: Phase 2 handles the tricky ncurses input quirks (mouse, resize, escape timing) before adding complex features in Phases 3-4.

**Data safety early**: Phase 3 implements atomic writes before users have real data to lose.

**Incremental complexity**: Each phase builds capabilities while avoiding pitfalls identified in research. No phase depends on unbuilt future phases.

### Research-Informed Decisions

From research/SUMMARY.md:
- **Modal editing differentiator**: True vim modal editing (normal/insert) is a unique feature not found in existing TUI kanban tools (Phase 2)
- **Atomic file operations**: Critical to prevent data corruption on crash (Phase 3)
- **Mouse as enhancement**: Keyboard-first with mouse support as optional (Phase 2)
- **Markdown storage**: Human-readable, git-friendly format (Phase 1)

### Risks & Mitigations

| Risk | Phase | Mitigation |
|------|-------|------------|
| Terminal cleanup failure | 1 | Install signal handlers, use atexit() for cleanup |
| Escape key latency | 2 | Reduce ESCDELAY environment variable |
| File corruption | 3 | Atomic writes: temp → fsync → rename |
| Resize crashes | 2 | Handle KEY_RESIZE, re-layout all windows |

### Future Considerations (v2+)

Features intentionally deferred:
- Mouse drag-and-drop for moving tasks
- Custom column layouts (beyond 3 columns)
- Search/filter across boards
- Tags/labels for tasks
- Due dates and reminders
- Color themes
- Help screen with keybindings

These are tracked in REQUIREMENTS.md v2 section but not in this roadmap.

---
*Created by GSD Roadmap Agent*  
*Last updated: 2025-04-04*
