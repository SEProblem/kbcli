# Phase 1: Core Kanban Foundation - Context

**Gathered:** 2025-04-04
**Status:** Ready for planning

<domain>
## Phase Boundary

Users can view, create, delete, and move tasks on a 3-column kanban board (To Do / In Progress / Done) with automatic markdown persistence.

</domain>

<decisions>
## Implementation Decisions

### Board Layout & Rendering
- **D-01:** Columns use terminal-adaptive widths — each column fills available space with minimum widths for readability
- **D-02:** Scrolling is column-local — each column scrolls independently when content overflows

### Visual Styling
- **D-03:** Board uses minimal monochrome style — borders only, uses terminal default colors
- **D-04:** Selected task uses reverse video highlight — inverted foreground/background colors

### Task Creation Flow
- **D-05:** Press 'o' to create a new task below the current position
- **D-06:** Press Shift+O to create a new task above the current position
- **D-07:** New tasks are placed below the current task (inserts at cursor position)

### Task Deletion
- **D-08:** Task deletion is instant — no confirmation dialog, relies on undo if mistake

### Task Movement
- **D-09:** Use 'h' key to move selected task to the column on the left
- **D-10:** Use 'l' key to move selected task to the column on the right
- **D-11:** Tasks can be reordered within a column via movement keys

### Markdown Storage
- **D-12:** Board stored in GitHub-style Markdown format using `- [ ]` task syntax
- **D-13:** Columns represented as H2 headers (`## Column Name`)
- **D-14:** Auto-save triggers on every change (create, delete, move)

### Agent's Discretion
- Column minimum widths — agent decides based on terminal compatibility
- Specific key for delete — agent decides (standard is 'd' or 'x')
- Default board filename — agent decides
- File location defaults — agent decides per platform conventions

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Research Files
- `.planning/research/SUMMARY.md` — Project overview, stack recommendations
- `.planning/research/ARCHITECTURE.md` — ncurses patterns, Model-View separation
- `.planning/research/PITFALLS.md` — Terminal cleanup, refresh() calls, atomic writes
- `.planning/research/STACK.md` — ncurses 6.4+, C/C++ recommendations

### Requirements
- `.planning/REQUIREMENTS.md` — BRD-01 through BRD-08, STO-01/03/04, APP-01/02/03

[If no external specs: "No external specs — requirements fully captured in decisions above"]

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- No source code exists yet (greenfield project)

### Established Patterns
- Model-View separation per ARCHITECTURE.md
- ncurses initialization sequence from research
- Event loop pattern for main application
- Panel-based window hierarchy

### Integration Points
- Storage layer writes to `~/.config/kanban-cli/boards/` or user-specified directory
- Application controller manages event loop and mode state
- Input handler processes keyboard/mouse events

</code_context>

<specifics>
## Specific Ideas

- User specifically wants 'o' and Shift+O for task creation (vim-style "open below/above")
- User prefers instant deletion for fast workflow
- User wants GitHub-style markdown for portability and git-friendliness

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope

</deferred>

---

*Phase: 01-core-kanban-foundation*
*Context gathered: 2025-04-04*