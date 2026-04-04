# Phase 2: Input Handling & Modal Editing - Context

**Gathered:** 2026-04-04
**Status:** Ready for planning

<domain>
## Phase Boundary

Users can navigate efficiently using vim keys and mouse with modal editing for text input. This phase adds proper vim-style navigation, mouse support, modal editing system, and terminal resize handling.

</domain>

<decisions>
## Implementation Decisions

### Navigation Keys (hjkl conflict resolution)
- **D-01:** Agent's Discretion — "Hybrid (agent decides)" per user request. Agent implements vim-style navigation with h/l for column navigation, uses appropriate keys for task movement (e.g., < / > or Shift+h/l) preserving Phase 1 functionality where possible.
- **D-02:** Arrow keys remain as alternative navigation (NAV-02) — already implemented
- **D-03:** Tab/Shift+Tab remain for column jumping (NAV-04) — already implemented

### Modal Editing System
- **D-04:** Press 'i' or Enter to enter Insert mode for editing task titles (MOD-03)
- **D-05:** Press Esc to exit Insert mode back to Normal mode (MOD-04)
- **D-06:** App has Normal mode (navigation/commands) and Insert mode (text editing) (MOD-01, MOD-02)

### Mouse Support
- **D-07:** Full mouse support — Click selects any task visible on screen (NAV-05)
- **D-08:** Scroll wheel scrolls within a column when content overflows (NAV-06)
- **D-09:** Mouse handling per PITFALLS.md: enable with mousemask(ALL_MOUSE_EVENTS), use getmouse() for coordinates

### Resize Handling
- **D-10:** Soft resize using resizeterm() — Update LINES/COLS without full reinitialization (APP-04)
- **D-11:** On resize: resizeterm() call, recalculate window positions, redraw all windows

### Visual Indicators
- **D-12:** Status bar at bottom shows: current mode (Normal/Insert), current column, task position (MOD-05)
- **D-13:** Mode indicator follows vim convention: "-- INSERT --" style when in Insert mode

### the agent's Discretion
- Specific key bindings for task movement (replacing h/l) — agent decides based on vim conventions
- Status bar exact content and formatting — agent decides
- Scroll wheel behavior details — agent decides

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Research Files
- `.planning/research/SUMMARY.md` — Project overview, stack recommendations
- `.planning/research/ARCHITECTURE.md` — ncurses patterns, Model-View separation
- `.planning/research/PITFALLS.md` — Terminal cleanup, refresh() calls, atomic writes, mouse handling, escape key ambiguity

### Requirements
- `.planning/REQUIREMENTS.md` — NAV-01 through NAV-06, MOD-01 through MOD-05, APP-04
- `.planning/ROADMAP.md` — Phase 2 success criteria

### Prior Phase Context
- `.planning/phases/01-core-kanban-foundation/01-CONTEXT.md` — Phase 1 decisions (D-09, D-10 use h/l for task move)

[If no external specs: "No external specs — requirements fully captured in decisions above"]

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `src/input.c` — Existing input handling with handle_input() function, Selection struct for tracking current position
- `src/main.c` — Event loop pattern, ESCDELAY set to 25ms for vim feel
- `read_task_title()` function in input.c — Can be reused/extended for Insert mode editing

### Established Patterns
- Mode handling not yet implemented — needs new mode state variable
- KEY_RESIZE already caught in switch but not handled (just breaks)
- Arrow key navigation already works
- g/G for top/bottom jump already works (NAV-03)

### Integration Points
- Input handler needs mode state — add to Selection struct or create AppState
- Renderer needs status bar rendering — add status_bar.c or extend renderer
- Mouse events need to be checked in input loop alongside keyboard

</code_context>

<specifics>
## Specific Ideas

- User prefers vim-style modal editing (i/Enter for insert, Esc to exit)
- User wants full mouse support with click and scroll
- User wants status bar showing mode and position

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope

</deferred>

---

*Phase: 02-input-handling-modal-editing*
*Context gathered: 2026-04-04*