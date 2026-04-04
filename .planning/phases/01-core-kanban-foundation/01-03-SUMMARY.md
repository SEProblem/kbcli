---
phase: 01-core-kanban-foundation
plan: "03"
subsystem: data
tags: [c, ncurses, movement, reordering, persistence]

# Dependency graph
requires:
  - 01-02 (renderer, input)
provides:
  - Task movement between columns ('h' left, 'l' right)
  - Task reordering within column ('J' up, 'K' down)
  - Board persistence across app restarts
affects: [phase-2]

# Tech tracking
tech-stack:
  added: [task movement, reordering]
  patterns: [linked-list manipulation, auto-save on change]

key-files:
  created: []
  modified: [include/models.h, src/models.c, src/input.c]

key-decisions:
  - 'h' moves selected task to left column per D-09
  - 'l' moves selected task to right column per D-10
  - 'J' (Shift+j) reorders task up within column per D-11
  - 'K' (Shift+k) reorders task down within column per D-11
  - Arrow keys for navigation, auto-save after every change

patterns-established:
  - "Movement functions return success/failure for boundary handling"
  - "Selection updates to follow moved task to new column"

requirements-completed: [BRD-04, BRD-05, BRD-07, BRD-08, APP-02, APP-03]

# Metrics
duration: 2min
completed: 2026-04-04
---

# Phase 1 Plan 3: Task Movement & Persistence Summary

**Task movement between columns with reordering and markdown persistence**

## Performance

- **Duration:** 2 min
- **Started:** 2026-04-04T07:48:15Z
- **Completed:** 2026-04-04T07:50:00Z
- **Tasks:** 3
- **Files modified:** 3

## Accomplishments

- Task movement between columns ('h' to left, 'l' to right) per D-09/D-10
- Task reordering within column ('J' up, 'K' down) per D-11
- Arrow keys restored for column navigation
- Auto-save on every change per D-14 requirement
- Board loads from markdown file on startup (~/.config/kanban-cli/boards/default.md)
- Terminal properly restored on quit via signal handlers and atexit()

## Task Commits

Each task was committed atomically:

1. **Task 1: Implement task movement between columns** - `27efdee` (feat)
2. **Task 2: Add arrow key navigation fix** - `87b42f9` (fix)
3. **Task 3: Fix reordering logic** - `6005c49` (fix)

## Files Created/Modified

- `include/models.h` - Added task_move_to_column, move_left, move_right, move_up, move_down declarations
- `src/models.c` - Implementation of task movement and reordering functions
- `src/input.c` - Updated key handlers for 'h', 'l', 'J', 'K' with auto-save

## Decisions Made

- Used 'h'/'l' for task movement (not navigation) as per D-09/D-10
- Arrow keys provide column navigation alternative
- 'J'/'K' use vim-style shift-letter convention for "bigger" action (reordering)
- Boundary handling: can't move beyond first/last column (returns -1)
- Auto-save triggers after successful move/reorder per D-14

## Deviations from Plan

None - plan executed as specified. All movement keys implemented per design decisions.

## Issues Encountered

- Selection struct duplication between models.h and renderer.h - fixed by using renderer.h in models.h
- Unused variable warning in move_up - fixed with cleaner swap logic
- Arrow key navigation removed when implementing 'h'/'l' movement - restored

## Next Phase Readiness

- Phase 1 core kanban functionality complete
- All 14 Phase 1 requirements satisfied
- Ready for Phase 2: Input Handling & Modal Editing (NAV-01 through NAV-06)
- No blockers identified

---
*Phase: 01-core-kanban-foundation*
*Completed: 2026-04-04*