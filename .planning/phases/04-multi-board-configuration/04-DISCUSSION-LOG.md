# Phase 4: Multi-Board & Configuration - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-04-04
**Phase:** 4-multi-board-configuration
**Areas discussed:** Board switching mechanism, Board creation workflow, Configuration file format, Keybinding customization

---

## Board Switching Mechanism

| Option | Description | Selected |
|--------|-------------|----------|
| Command-based (:b, :board) | Type command to open board switcher - minimal UI, vim-style | |
| Board list menu | Dedicated view showing all boards - more visual, requires new screen | |
| Ctrl+n popup | Quick popup overlay listing boards - balanced approach | |

**User's choice:** I should be able to nav with :bn and :bp but also board list menu when interacting with mouse
**Notes:** Hybrid approach — both :bn/:bp commands AND board list menu for mouse users

---

## Board Creation Workflow

| Option | Description | Selected |
|--------|-------------|----------|
| Prompt for name only | Creates empty board with default 3 columns | ✓ |
| Prompt + layout selection | Asks for board name AND which columns layout | |
| Template-based | Choose from preset templates | |

**User's choice:** Prompt for name only
**Notes:** New boards get default 3-column layout (To Do/In Progress/Done)

---

## Configuration File Format

| Option | Description | Selected |
|--------|-------------|----------|
| INI-style | [section]\nkey=value — familiar, easy to parse | |
| JSON | {"key": "value"} — structured but more verbose | ✓ |
| Plain key=value | No sections — simplest format | |

**User's choice:** JSON
**Notes:** JSON format at ~/.config/kanban-cli/config

---

## Keybinding Customization

| Option | Description | Selected |
|--------|-------------|----------|
| Full replacement | Config completely replaces default keybindings | ✓ |
| Additive override | Config adds/overrides specific keys, rest use defaults | |
| Priority system | Config keys take priority, fallback to defaults | |

**User's choice:** Full replacement
**Notes:** Simpler logic for v1 — config fully replaces defaults

---

## the agent's Discretion

- Exact :bn/:bp implementation details
- Board list menu visual design
- Config file JSON schema
- Default board on startup behavior

---

## Deferred Ideas

None mentioned.