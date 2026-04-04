# Phase 3: Task Details & Data Safety - Context

**Gathered:** 2026-04-04
**Status:** Ready for planning

<domain>
## Phase Boundary

Users can add rich task details (descriptions, checklists) with guaranteed data safety through atomic saves. This phase adds task descriptions, detailed view toggle, checklist/subtask support, and ensures atomic file operations.

</domain>

<decisions>
## Implementation Decisions

### Task Description UX
- **D-01:** Descriptions displayed in popup overlay — pressing Enter or 'i' on a task opens a popup overlay showing the description
- **D-02:** Description can be edited in place within the popup
- **D-03:** Press Esc to close popup and return to board
- **D-04:** Descriptions stored as plain text only — no Markdown rendering (keeps it simple, avoids formatting artifacts)

### View Toggle
- **D-05:** Agent's Discretion — specific key for toggling compact/detailed view, agent decides based on consistency with vim conventions

### Checklist Model
- **D-06:** Agent's Discretion — checklist markdown format, interaction model (sub-mode vs inline), and add/edit/delete workflow

### Atomic File Operations
- **D-07:** STO-02 already implemented — storage.c uses temp file + fsync + rename pattern in write_markdown()
- **D-08:** No changes needed — atomic writes are working correctly per PITFALLS.md recommendations

### the agent's Discretion
- View toggle key binding — agent decides
- Checklist implementation details — agent decides

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Research Files
- `.planning/research/SUMMARY.md` — Project overview, stack recommendations
- `.planning/research/ARCHITECTURE.md` — ncurses patterns, Model-View separation
- `.planning/research/PITFALLS.md` — Terminal cleanup, refresh() calls, atomic writes

### Requirements
- `.planning/REQUIREMENTS.md` — TSK-01 through TSK-07, STO-02
- `.planning/ROADMAP.md` — Phase 3 success criteria

### Prior Phase Context
- `.planning/phases/01-core-kanban-foundation/01-CONTEXT.md` — Phase 1 decisions (minimal UI, vim keys)
- `.planning/phases/02-input-handling-modal-editing/02-CONTEXT.md` — Phase 2 decisions (modal editing, status bar)

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `src/storage.c:write_markdown()` — Already implements atomic writes (STO-02 done!)
- `Task` struct in `include/kanban.h` — needs extension with description and checklist fields
- `task_create()` in `src/models.c` — takes only title, needs to support description
- Input handling in `src/input.c` — can be extended for description popup interaction

### Established Patterns
- Modal editing system from Phase 2 — can reuse mode switching for description popup
- Status bar in renderer — can show description popup mode
- Popup/overlay pattern — none yet, needs new UI component

### Integration Points
- Task struct needs new fields: description[MAX_DESC_LEN], checklist (linked list or array)
- Renderer needs new function: render_description_popup()
- Input handler needs new key: Enter or 'i' triggers description popup
- Storage layer needs update: parse/write description and checklists to markdown

</code_context>

<specifics>
## Specific Ideas

- User specifically wants Enter and 'i' to both open the task description popup
- User wants plain text descriptions only (no Markdown rendering)
- User prefers popup overlay for description display/edit

</specifics>

<deferred>
## Deferred Ideas

### Not Discussed (Agent's Discretion)
- View toggle key — agent decides
- Checklist implementation — agent decides

</deferred>

---

*Phase: 03-task-details-data-safety*
*Context gathered: 2026-04-04*