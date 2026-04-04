# Phase 1: Core Kanban Foundation - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2025-04-04
**Phase:** 01-core-kanban-foundation
**Areas discussed:** Board layout & rendering, Visual styling, Task creation flow, Task deletion, Task movement, Markdown format

---

## Board Layout & Rendering

| Option | Description | Selected |
|--------|-------------|----------|
| Even 1/3 split | Each column gets equal width. Simplest to implement. | |
| Flexible columns | Columns adapt based on content width or terminal size | ✓ |
| You decide | Agent decides based on standard ncurses patterns | |

**User's choice:** Flexible columns
**Notes:** User wants terminal-adaptive columns that fill available space with minimum widths

### Follow-up: What should drive flexible column widths?

| Option | Description | Selected |
|--------|-------------|----------|
| Content-based | Column width grows with longest task title | |
| Terminal-adaptive | Columns fill available space, with minimum widths | ✓ |
| You decide | Agent decides based on best TUI practices | |

**User's choice:** Terminal-adaptive

### Follow-up: How should scrolling work?

| Option | Description | Selected |
|--------|-------------|----------|
| Column-local scroll | Each column scrolls independently | ✓ |
| Global scroll | All columns scroll together | |
| You decide | Agent decides | |

**User's choice:** Column-local scroll

---

## Visual Styling

| Option | Description | Selected |
|--------|-------------|----------|
| Minimal monochrome | Borders only, uses terminal's default colors | ✓ |
| Color-coded columns | Each column has subtle background color | |
| You decide | Agent decides | |

**User's choice:** Minimal monochrome

### Follow-up: How should selected task be highlighted?

| Option | Description | Selected |
|--------|-------------|----------|
| Reverse video | Inverted foreground/background | ✓ |
| Underline only | Subtle underline | |
| You decide | Agent decides | |

**User's choice:** Reverse video

---

## Task Creation Flow

| Option | Description | Selected |
|--------|-------------|----------|
| n key | Press 'n' to create new task in current column | |
| o key | Press 'o' for insert/new | ✓ (with shift+o for above) |
| You decide | Agent decides | |

**User's choice:** 'o' to add a new card underneath current, and 'Shift+O' to add above the current
**Notes:** User clarified: o creates below current, Shift+O creates above current

### Follow-up: Where should new tasks be placed?

| Option | Description | Selected |
|--------|-------------|----------|
| Below current | New task inserted below currently selected task | ✓ |
| At top of column | Always goes to top | |
| At bottom of column | Always goes to bottom | |
| You decide | Agent decides | |

**User's choice:** Below current

---

## Task Deletion

| Option | Description | Selected |
|--------|-------------|----------|
| Instant delete | Delete immediately, rely on undo | ✓ |
| Confirm dialog | Popup confirmation | |
| You decide | Agent decides | |

**User's choice:** Instant delete

---

## Task Movement

| Option | Description | Selected |
|--------|-------------|----------|
| hl to move left/right | h moves left, l moves right | ✓ |
| Arrow keys | Left/right arrows to move between columns | |
| You decide | Agent decides | |

**User's choice:** hl to move left/right

---

## Markdown Format

| Option | Description | Selected |
|--------|-------------|----------|
| GitHub-style task lists | Uses - [ ] syntax for tasks, # for columns | ✓ |
| Custom format | Own format with specific delimiters | |
| You decide | Agent decides | |

**User's choice:** GitHub-style task lists

### Follow-up: How should columns be represented?

| Option | Description | Selected |
|--------|-------------|----------|
| H2 headers (## Column Name) | Each column is a ## H2 section | ✓ |
| H1 only | Single H1 for board, columns as bullets | |
| You decide | Agent decides | |

**User's choice:** H2 headers

---

## Agent's Discretion

- Column minimum widths
- Specific key for delete
- Default board filename
- File location defaults

---

## Deferred Ideas

No deferred ideas captured — discussion stayed within phase scope.