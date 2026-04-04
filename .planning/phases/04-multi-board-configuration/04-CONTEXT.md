# Phase 4: Multi-Board & Configuration - Context

**Gathered:** 2026-04-04
**Status:** Ready for planning

<domain>
## Phase Boundary

Users can manage multiple boards and customize application behavior through configuration. This phase adds multi-board support (create, switch, list, delete) and configuration file for keybindings and default directory.

</domain>

<decisions>
## Implementation Decisions

### Board Switching Mechanism
- **D-01:** Command-based navigation using vim-style commands: `:bn` (next board), `:bp` (previous board)
- **D-02:** Board list menu available for mouse interaction — dedicated view showing all boards
- **D-03:** Both :bn/:bp and mouse-accessible board list supported (hybrid approach)

### Board Creation Workflow
- **D-04:** User prompted for board name only when creating new board
- **D-05:** New boards default to 3-column layout (To Do / In Progress / Done)

### Configuration File
- **D-06:** Configuration file format: JSON at `~/.config/kanban-cli/config`
- **D-07:** Configuration includes keybindings customization and default board directory

### Keybinding Customization
- **D-08:** Full replacement — config completely replaces default keybindings
- **D-09:** No additive/priority system — simpler logic for v1

### Board Storage
- **D-10:** Default board directory: `~/.config/kanban-cli/boards/`
- **D-11:** Per MBD-05: Boards stored in configurable directory, default as above

### the agent's Discretion
- Exact :bn/:bp implementation details — agent decides
- Board list menu visual design — agent decides
- Config file JSON schema — agent decides
- Default board on startup (loaded vs new) — agent decides

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Research Files
- `.planning/research/SUMMARY.md` — Project overview, stack recommendations
- `.planning/research/ARCHITECTURE.md` — ncurses patterns, Model-View separation
- `.planning/research/PITFALLS.md` — Terminal cleanup, refresh() calls, atomic writes

### Requirements
- `.planning/REQUIREMENTS.md` — MBD-01 through MBD-05, APP-05, APP-06, APP-07
- `.planning/ROADMAP.md` — Phase 4 success criteria

### Prior Phase Context
- `.planning/phases/01-core-kanban-foundation/01-CONTEXT.md` — Phase 1 decisions (vim keys, minimal UI)
- `.planning/phases/02-input-handling-modal-editing/02-CONTEXT.md` — Phase 2 decisions (modal editing)
- `.planning/phases/03-task-details-data-safety/03-CONTEXT.md` — Phase 3 decisions (description popup)

[If no external specs: "No external specs — requirements fully captured in decisions above"]

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `src/storage.c:get_default_board_path()` — Provides pattern for config directory paths
- `src/storage.c:board_save()` — Creates directory if needed (mkdir with 0755)
- Modal editing system from Phase 2 — Can be reused for command input (:bn, :bp)

### Established Patterns
- Markdown storage at `~/.config/kanban-cli/` per Phase 1
- Vim command mode (colon commands) — can extend for board switching
- Board struct already has `filename` field — can track which board file

### Integration Points
- Storage layer needs board list scanning function (read directory, list .md files)
- Input handler needs new commands: :bn, :bp, :b (switch), :bnew (create)
- Config file needs to be loaded on startup and keybindings applied
- Renderer needs board list menu view for mouse interaction

</code_context>

<specifics>
## Specific Ideas

- User specifically wants :bn and :bp for board navigation (vim-style)
- User wants board list menu for mouse users
- User wants board creation to prompt only for name
- User prefers JSON format for configuration file
- User wants full keybinding replacement (no additive)

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope

</deferred>

---

*Phase: 04-multi-board-configuration*
*Context gathered: 2026-04-04*