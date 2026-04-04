# Requirements: Kanban CLI

**Defined:** 2025-04-04
**Core Value:** Users can manage their tasks efficiently without leaving the terminal, using familiar vim shortcuts and optional mouse interactions.

## v1 Requirements

Requirements for Milestone v1.0 — complete kanban board with TUI, vim navigation, and markdown storage.

### Core Board

- [x] **BRD-01**: User can view a 3-column kanban board (To Do / In Progress / Done)
- [x] **BRD-02**: User can create a new task with a title
- [x] **BRD-03**: User can delete an existing task
- [x] **BRD-04**: User can move tasks between columns
- [x] **BRD-05**: User can reorder tasks within a column
- [x] **BRD-06**: Visual indicator shows which task is currently selected
- [x] **BRD-07**: Board state persists to markdown file automatically
- [x] **BRD-08**: User can load an existing board from markdown file

### Navigation

- [x] **NAV-01**: User can navigate tasks using hjkl keys (vim-style)
- [x] **NAV-02**: User can navigate using arrow keys as alternative
- [x] **NAV-03**: User can jump to top/bottom of column (gg/G)
- [x] **NAV-04**: User can jump between columns (H/L or Tab/Shift+Tab)
- [x] **NAV-05**: Mouse click selects a task
- [x] **NAV-06**: Scroll wheel scrolls within a column

### Vim Modal Editing

- [x] **MOD-01**: Application has Normal mode for navigation and commands
- [x] **MOD-02**: Application has Insert mode for editing text
- [x] **MOD-03**: User can enter Insert mode to edit task title (i or Enter)
- [x] **MOD-04**: User can exit Insert mode back to Normal mode (Esc)
- [x] **MOD-05**: Normal mode shows visual indicators for available actions

### Task Details

- [x] **TSK-01**: User can view task description in addition to title
- [x] **TSK-02**: User can toggle between title-only and detailed view
- [x] **TSK-03**: User can edit task description
- [x] **TSK-04**: Tasks can have checklists (subtasks)
- [x] **TSK-05**: User can toggle checklist items (complete/incomplete)
- [x] **TSK-06**: User can add checklist items to a task
- [x] **TSK-07**: User can delete checklist items from a task

### Multiple Boards

- [ ] **MBD-01**: User can create a new board
- [ ] **MBD-02**: User can switch between boards without exiting
- [ ] **MBD-03**: User can list available boards
- [ ] **MBD-04**: User can delete a board
- [ ] **MBD-05**: Boards are stored in configurable directory (default: ~/.config/kanban-cli/boards/)

### Storage & Persistence

- [x] **STO-01**: Board data stored in human-readable markdown format
- [x] **STO-02**: Storage uses atomic writes (write temp, then rename)
- [x] **STO-03**: Application auto-saves on changes
- [x] **STO-04**: Markdown format compatible with GitHub-style task lists

### Application

- [x] **APP-01**: Application initializes ncurses TUI on launch
- [x] **APP-02**: User can quit gracefully from Normal mode (q or :q)
- [x] **APP-03**: Terminal is restored to original state on exit
- [x] **APP-04**: Application handles terminal resize events
- [x] **APP-05**: Configuration file at ~/.config/kanban-cli/config
- [x] **APP-06**: User can customize keybindings via configuration
- [x] **APP-07**: User can set default board directory via configuration

## v2 Requirements

Deferred to future release. Tracked but not in current roadmap.

### Advanced Features

- **ADV-01**: Mouse drag-and-drop to move tasks between columns
- **ADV-02**: Custom column layouts (not just 3 columns)
- **ADV-03**: Search/filter tasks across all boards
- **ADV-04**: Tags/labels for tasks
- **ADV-05**: Due dates for tasks

### UI Enhancements

- **UI-01**: Color themes/customization
- **UI-02**: Status bar with keyboard shortcut hints
- **UI-03**: Help screen showing all keybindings

## Out of Scope

Explicitly excluded. Documented to prevent scope creep.

| Feature | Reason |
|---------|--------|
| Team collaboration | Single-user tool by design |
| Network/cloud sync | File-based storage, users can sync via rsync/Dropbox if needed |
| Real-time sync | Not applicable for single-user local tool |
| GUI or web interface | Terminal-only focus |
| Mobile app | CLI/TUI only |
| OAuth/authentication | Local file-based, no accounts needed |
| Rich text formatting | Plain text/markdown only |
| File attachments | Keep storage simple and portable |
| Recurring tasks | Scheduling complexity, defer to v2+ |
| Import/export to other formats | Markdown is standard, can add later |

## Traceability

Which phases cover which requirements. Updated during roadmap creation.

| Requirement | Phase | Status |
|-------------|-------|--------|
| BRD-01 | Phase 1 | Complete |
| BRD-02 | Phase 1 | Complete |
| BRD-03 | Phase 1 | Complete |
| BRD-04 | Phase 1 | Complete |
| BRD-05 | Phase 1 | Complete |
| BRD-06 | Phase 1 | Complete |
| BRD-07 | Phase 1 | Complete |
| BRD-08 | Phase 1 | Complete |
| STO-01 | Phase 1 | Complete |
| STO-03 | Phase 1 | Complete |
| STO-04 | Phase 1 | Complete |
| APP-01 | Phase 1 | Complete |
| APP-02 | Phase 1 | Complete |
| APP-03 | Phase 1 | Complete |
| NAV-01 | Phase 2 | Complete |
| NAV-02 | Phase 2 | Complete |
| NAV-03 | Phase 2 | Complete |
| NAV-04 | Phase 2 | Complete |
| NAV-05 | Phase 2 | Complete |
| NAV-06 | Phase 2 | Complete |
| MOD-01 | Phase 2 | Complete |
| MOD-02 | Phase 2 | Complete |
| MOD-03 | Phase 2 | Complete |
| MOD-04 | Phase 2 | Complete |
| MOD-05 | Phase 2 | Complete |
| APP-04 | Phase 2 | Complete |
| TSK-01 | Phase 3 | Complete |
| TSK-02 | Phase 3 | Complete |
| TSK-03 | Phase 3 | Complete |
| TSK-04 | Phase 3 | Complete |
| TSK-05 | Phase 3 | Complete |
| TSK-06 | Phase 3 | Complete |
| TSK-07 | Phase 3 | Complete |
| STO-02 | Phase 3 | Complete |
| MBD-01 | Phase 4 | Pending |
| MBD-02 | Phase 4 | Pending |
| MBD-03 | Phase 4 | Pending |
| MBD-04 | Phase 4 | Pending |
| MBD-05 | Phase 4 | Pending |
| APP-05 | Phase 4 | Complete |
| APP-06 | Phase 4 | Complete |
| APP-07 | Phase 4 | Complete |

**Coverage:**
- v1 requirements: 40 total
- Phase 1 (Core Kanban Foundation): 14 requirements
- Phase 2 (Input Handling & Modal Editing): 12 requirements
- Phase 3 (Task Details & Data Safety): 8 requirements
- Phase 4 (Multi-Board & Configuration): 6 requirements
- Mapped to phases: 40
- Unmapped: 0 ✓

---
*Requirements defined: 2025-04-04*
*Last updated: 2025-04-04 after roadmap creation*
