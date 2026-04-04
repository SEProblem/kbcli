# Feature Landscape: TUI Kanban Board

**Domain:** Terminal-based kanban board for personal task tracking
**Researched:** 2026-04-04
**Overall confidence:** HIGH

## Executive Summary

TUI kanban boards for developers have matured with tools like **Taskell** (1.8k stars) and **taskwarrior-tui** (2k stars) establishing clear patterns. The personal productivity niche favors:

1. **Vim-style navigation** as the primary interaction model
2. **Markdown storage** for version control friendliness
3. **Minimal dependencies** with local-first storage
4. **Keyboard-first, mouse-optional** workflows

Key insight from Taskell: users prefer "per-project task lists" stored alongside code repositories, not a centralized database. This enables git-based workflow integration.

---

## Table Stakes

Features users expect in any kanban board. Missing these = product feels broken.

| Feature | Why Expected | Complexity | Notes |
|---------|--------------|------------|-------|
| **3-column layout** (To Do/In Progress/Done) | Kanban convention since 1940s Toyota | Low | Standard preset. Most users expect this as default. |
| **Task cards with title** | Basic unit of work | Low | Title truncation needed for narrow columns. |
| **Move tasks between columns** | Core kanban mechanic | Low | hjkl or mouse drag. Visual feedback essential. |
| **Create new task** | Entry point for all work | Low | `n` key in Taskell. Prompt for title. |
| **Delete task** | Mistake correction | Low | `d` with confirmation prevents accidents. |
| **Scroll columns independently** | Terminal height < content | Medium | Viewport pattern from Brick library. |
| **Visual selection indicator** | Know what's active | Low | Color + highlight (magenta in Taskell default). |
| **Save/load persistence** | Data survives restart | Low | Auto-save on every change. |
| **Quit gracefully** | Expected UX | Low | `q` to exit, preserve state. |

### Table Stakes Detail

**Navigation Patterns (from Taskell & taskwarrior-tui):**
- `hjkl` - directional movement (vim standard)
- `J/K` - move task up/down within column
- `H/L` - move task between columns
- `g/G` - jump to top/bottom
- Arrow keys as fallback

**Mouse Support Essentials:**
- Click to select task
- Click column header to scroll column
- Scroll wheel for vertical scrolling
- Click-drag between columns (optional, see complexity)

---

## Differentiators

Features that set this kanban board apart from basic implementations.

| Feature | Value Proposition | Complexity | Notes |
|---------|-------------------|------------|-------|
| **Vim-style modal editing** | Familiar to target audience | Medium | Normal mode (nav), Insert mode (edit). Different from Taskell's single-mode. |
| **Markdown native storage** | Git-friendly, human-readable | Low | Taskell pioneered this. Enables diff viewing. |
| **Multiple board support** | Separate work/personal/side projects | Medium | In-app switching (key or menu). Boards stored as separate .md files. |
| **Checklists/subtasks** | Break down complex work | Medium | Collapsible in UI. Stored as nested markdown lists. |
| **Task descriptions** | More context than title | Low | Toggle view or inline expansion. Indicator when description exists. |
| **Custom column layouts** | Adapt to personal workflow | Medium | Empty custom boards OR preset 3-column. Persist column names. |
| **Mouse + keyboard hybrid** | Best of both worlds | High | Mouse for quick moves, keyboard for power users. |
| **Status bar** | Context at a glance | Low | Current board, task count, key hints. |
| **Configuration file** | Personalize behavior | Medium | ~/.config/kanban-cli/config.ini pattern. |

### Differentiator Detail

**Vim Modal Editing (Unique Differentiator):**
Most TUI kanbans (Taskell, etc.) use single-mode with direct shortcuts. True vim modal editing (normal/insert) would be unique:
- Normal mode: navigation keys work directly
- Insert mode: typing edits task title
- `Esc` returns to normal mode
- More intuitive for heavy vim users

**Markdown Storage Format (Taskell-style):**
```markdown
## To Do

- Task title here
    Description line 1
    Description line 2
    * Subtask 1
    * Subtask 2

## In Progress

- Another task
```

Benefits:
- `git diff` shows meaningful changes
- Can edit in any text editor
- No lock-in, no binary format
- Works with existing sync tools (Dropbox, Syncthing)

**Multiple Board Switching:**
- Switch via key (e.g., `b` for board menu)
- List recent boards
- Each board = separate .md file
- Directory scanning for available boards

---

## Anti-Features

Explicitly NOT building these. Prevents scope creep.

| Anti-Feature | Why Avoid | What to Do Instead |
|--------------|-----------|-------------------|
| **Team collaboration** | Requires sync, auth, conflicts | Single user only. File-based enables external sync if needed. |
| **Network/cloud sync** | Complexity, privacy concerns | Users sync via Dropbox/rsync/git as they prefer. |
| **Due dates / reminders** | Requires background daemon | Keep simple. Due dates add scheduling complexity. |
| **Rich text / attachments** | Beyond TUI capabilities | Plain text + markdown only. Links can be pasted. |
| **Tags / labels** | Overkill for personal use | Columns provide categorization. Search could come later. |
| **Priority levels** | Complexity without clear benefit | Column position = priority (top = high). |
| **Recurring tasks** | Scheduling complexity | Out of scope for v1. Manual duplication. |
| **Search / filtering** | Nice-to-have, not essential | Defer to v2. Vim's `/` pattern could work. |
| **Archiving old tasks** | Premature optimization | Keep all tasks in Done column, delete when needed. |
| **GUI or web interface** | Dilutes focus | Terminal only. Maintains simplicity. |
| **Mobile app** | Different platform entirely | Mobile users have other kanban apps. |
| **OAuth / authentication** | No user accounts needed | Local file permissions only. |

---

## Feature Dependencies

```
Core Rendering
    └── ncurses initialization
    └── Window layout (columns)
    └── Drawing borders/headers

Task Display
    └── Core Rendering
    └── Task data model
    └── Title truncation
    └── Description indicator

Navigation
    └── Task Display
    └── Selection state
    └── hjkl handling
    └── Arrow key fallback

Task Movement
    └── Navigation
    └── Data model updates
    └── Visual feedback

Mouse Support
    └── Core Rendering
    └── ncurses mouse events
    └── Hit detection
    └── Click → action mapping

Persistence
    └── Task data model
    └── Markdown parser/serializer
    └── File I/O
    └── Auto-save trigger

Multiple Boards
    └── Persistence
    └── Board metadata
    └── Switching UI
    └── File discovery

Checklists
    └── Task Display
    └── Collapsible UI
    └── Nested data model
    └── Toggle interaction

Configuration
    └── Config file parser (ini)
    └── Default values
    └── Runtime overrides
```

---

## Complexity Assessment

| Feature | Implementation Complexity | Risk Level | Notes |
|---------|---------------------------|------------|-------|
| Basic 3-column UI | Low | Low | Standard ncurses patterns |
| Task CRUD | Low | Low | Data structure straightforward |
| hjkl navigation | Low | Low | Simple key mapping |
| Markdown storage | Low | Low | Simple format, no external lib needed |
| Mouse click selection | Medium | Low | ncurses mouse support is mature |
| Mouse drag-and-drop | High | Medium | Coordinate tracking, visual feedback |
| Multiple boards | Medium | Low | File management, state switching |
| Checklists/subtasks | Medium | Medium | Nested data, UI indentation |
| Vim modal editing | Medium | Medium | State machine, mode indicators |
| Configuration system | Medium | Low | INI parsing, defaults |
| Custom column layouts | Medium | Medium | Dynamic UI, column persistence |
| Description display | Low | Low | Toggle or inline expansion |
| Status bar | Low | Low | Static info display |

---

## Personal Use Case Considerations

Since this is explicitly **single-user personal productivity**, we optimize for:

**Workflows to Support:**
1. **Quick capture** - Add task without disrupting flow (minimal prompts)
2. **Daily standup** - View all columns at once, see progress
3. **Deep work** - Focus on single column, clear visual hierarchy
4. **Weekly review** - Archive/completed tasks visible but out of way

**Not Team Workflows:**
- No assignment to users
- No comments/discussion
- No workflow transitions with permissions
- No notifications
- No burndown charts or velocity

**Developer-Specific UX:**
- Vim keys are primary, not secondary
- Mouse as enhancement, not replacement
- Config lives in ~/.config/ (XDG standard)
- Works well in tmux/screen
- Respects terminal color schemes

---

## MVP Feature Set Recommendation

### Phase 1 (Core Kanban):
1. **3-column layout** (To Do / In Progress / Done)
2. **Task cards with titles**
3. **hjkl navigation** + arrows
4. **Create (n), Delete (d), Move (H/L)**
5. **Markdown persistence** (auto-save)
6. **Mouse click to select**

### Phase 2 (Enhanced UX):
7. **Multiple boards** (in-app switching)
8. **Task descriptions** (toggle view)
9. **Checklists/subtasks**
10. **Status bar**
11. **Configuration file**

### Phase 3 (Polish):
12. **Custom column layouts** (create/delete columns)
13. **Mouse drag-and-drop**
14. **Vim modal editing** (if proven valuable)

---

## Sources

| Tool | Source | Confidence |
|------|--------|------------|
| Taskell | https://github.com/smallhadroncollider/taskell | HIGH - Direct competitor, 1.8k stars, archived but mature |
| taskwarrior-tui | https://github.com/kdheepak/taskwarrior-tui | HIGH - 2k stars, active, vim navigation |
| Brick (TUI lib) | https://github.com/jtdaugherty/brick | MEDIUM - Reference patterns, Haskell not C++ |
| Bubbles (Go TUI) | https://github.com/charmbracelet/bubbles | MEDIUM - Component patterns, different language |

---

## Confidence Assessment

| Area | Confidence | Reasoning |
|------|------------|-----------|
| Table Stakes | HIGH | Clear conventions from existing tools |
| Differentiators | HIGH | User feedback on Taskell issues shows demand |
| Anti-Features | HIGH | Project.md explicitly scopes these out |
| Complexity | MEDIUM | Based on patterns, actual ncurses work may vary |
| Dependencies | MEDIUM | Logical analysis, implementation may reveal more |

## Open Questions for Phase Research

1. **Drag-and-drop complexity** - Is the effort worth it vs keyboard-only movement?
2. **Modal editing value** - Do users want true vim modes or just vim keys?
3. **Custom columns UX** - How to handle column creation without cluttering UI?
4. **Checklist depth** - One level only, or true nesting (tasks → subtasks → sub-subtasks)?
