# Phase 2: Input Handling & Modal Editing - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-04-04
**Phase:** 02-input-handling-modal-editing
**Areas discussed:** Navigation keys, Modal editing system, Mouse support, Resize handling, Visual indicators

---

## Navigation keys (hjkl vs h/l task move conflict)

| Option | Description | Selected |
|--------|-------------|----------|
| Reassign task move to new keys | h/l become vim-style navigation, use < / > or H/L for task movement | |
| Keep h/l for task move | h/l stay as task movement keys, use arrows/Tab for navigation | |
| Hybrid (agent decides) | Agent decides based on vim conventions - h/l for nav, J/K for task reorder | ✓ |

**User's choice:** Hybrid (agent decides)
**Notes:** User delegated this decision to the agent. Agent will implement vim-style navigation with h/l for column navigation, reassign task movement to new keys (< > or Shift+h/l) while preserving Phase 1 functionality.

---

## Modal editing system

| Option | Description | Selected |
|--------|-------------|----------|
| Press 'i' or Enter, Esc to exit (vim-style) | Standard vim modal editing | ✓ |
| Enter only, Esc to exit | Only Enter enters edit mode | |
| Agent decides | User trusts agent to decide | |

**User's choice:** Press 'i' or Enter, Esc to exit (vim-style)
**Notes:** User wants familiar vim behavior - 'i' or Enter to enter Insert mode, Esc to exit back to Normal mode.

---

## Mouse support

| Option | Description | Selected |
|--------|-------------|----------|
| Full mouse support | Click selects any task, scroll wheel scrolls columns | ✓ |
| Selection + optional scroll | Click selects tasks, scroll optional | |
| Keyboard-first with mouse as enhancement | All features work without mouse | |

**User's choice:** Full mouse support
**Notes:** User wants full mouse capability - click to select any task, scroll wheel to scroll within columns.

---

## Resize handling

| Option | Description | Selected |
|--------|-------------|----------|
| Full reinitialize (endwin + refresh) | endwin() + refresh(), rebuild windows | |
| Soft resize (resizeterm) | Use resizeterm() to update LINES/COLS, adjust windows in place | ✓ |
| Simple clear + redraw | Just clear + redraw current layout | |

**User's choice:** Soft resize (resizeterm)
**Notes:** User prefers soft resize - less disruptive than full reinitialization, updates window positions in place.

---

## Visual indicators

| Option | Description | Selected |
|--------|-------------|----------|
| Status bar with mode + position | Bottom status bar shows mode, column, task count | ✓ |
| Inline hints | Show hints next to selected task | |
| Help overlay on '?' | Press '?' shows all keybindings | |
| Minimal mode indicator only | Just mode indicator | |

**User's choice:** Status bar with mode + position
**Notes:** User wants clear feedback - status bar at bottom showing current mode (Normal/Insert), current column, and task position.

---

## the agent's Discretion

- Specific key bindings for task movement (replacing h/l from Phase 1)
- Status bar exact content and formatting
- Scroll wheel behavior details
- Implementation of hybrid navigation approach

## Deferred Ideas

None — discussion stayed within phase scope