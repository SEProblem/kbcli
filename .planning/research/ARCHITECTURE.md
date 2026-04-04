# Architecture Research: TUI Kanban Board

**Domain:** Terminal User Interface (TUI) application using ncurses
**Researched:** 2025-04-04
**Confidence:** HIGH (based on ncurses official documentation and established patterns)

## Standard Architecture for ncurses TUI Applications

### System Overview

```
┌─────────────────────────────────────────────────────────────────────┐
│                        Application Layer                             │
├─────────────────────────────────────────────────────────────────────┤
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐              │
│  │   UI Layer   │  │ Input Handler│  │    State     │              │
│  │  (ncurses)   │  │              │  │   Manager    │              │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘              │
│         │                 │                 │                       │
├─────────┴─────────────────┴─────────────────┴───────────────────────┤
│                        Business Logic Layer                          │
├─────────────────────────────────────────────────────────────────────┤
│  ┌───────────────────────────────────────────────────────────────┐ │
│  │                    Application Controller                      │ │
│  │         (Command handling, navigation, modal management)       │ │
│  └───────────────────────────────────────────────────────────────┘ │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐              │
│  │   Board      │  │    Task      │  │   Column     │              │
│  │   Manager    │  │   Manager    │  │   Manager    │              │
│  └──────────────┘  └──────────────┘  └──────────────┘              │
├─────────────────────────────────────────────────────────────────────┤
│                        Data Layer                                    │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐              │
│  │   Markdown   │  │   File I/O   │  │   Board      │              │
│  │   Parser     │  │   Handler    │  │   Registry   │              │
│  └──────────────┘  └──────────────┘  └──────────────┘              │
└─────────────────────────────────────────────────────────────────────┘
```

### ncurses Window Hierarchy

```
┌──────────────────────────────────────────────────────────────┐
│ stdscr (Standard Screen - Root Window)                       │
│ ┌──────────────────────────────────────────────────────────┐ │
│ │ Header Panel (Board name, status bar)                    │ │
│ ├──────────────────────────────────────────────────────────┤ │
│ │                                                          │ │
│ │  ┌─────────┐  ┌─────────┐  ┌─────────┐                  │ │
│ │  │ Column  │  │ Column  │  │ Column  │   ...            │ │
│ │  │ Panel 1 │  │ Panel 2 │  │ Panel N │                  │ │
│ │  │         │  │         │  │         │                  │ │
│ │  │ ┌─────┐ │  │ ┌─────┐ │  │ ┌─────┐ │                  │ │
│ │  │ │Task │ │  │ │Task │ │  │ │Task │ │                  │ │
│ │  │ │Sub- │ │  │ │Sub- │ │  │ │Sub- │ │                  │ │
│ │  │ │win  │ │  │ │win  │ │  │ │win  │ │                  │ │
│ │  │ └─────┘ │  │ └─────┘ │  │ └─────┘ │                  │ │
│ │  │ ┌─────┐ │  │         │  │ ┌─────┐ │                  │ │
│ │  │ │Task │ │  │         │  │ │Task │ │                  │ │
│ │  │ └─────┘ │  │         │  │ └─────┘ │                  │ │
│ │  └─────────┘  └─────────┘  └─────────┘                  │ │
│ │                                                          │ │
│ ├──────────────────────────────────────────────────────────┤ │
│ │ Footer Panel (Key hints, current mode)                   │ │
│ └──────────────────────────────────────────────────────────┘ │
└──────────────────────────────────────────────────────────────┘
```

## Recommended Project Structure

```
src/
├── main.c                  # Entry point, initialization
├── app.c/h                 # Application controller, main loop
├── ui/
│   ├── window.c/h          # Base window management
│   ├── panel_manager.c/h   # ncurses panel operations
│   ├── board_view.c/h      # Kanban board rendering
│   ├── column_view.c/h     # Column rendering
│   ├── task_view.c/h       # Task card rendering
│   ├── input_handler.c/h   # Keyboard and mouse input
│   └── dialogs.c/h         # Modal dialogs (edit, confirm)
├── models/
│   ├── board.c/h           # Board data structure
│   ├── column.c/h          # Column data structure
│   ├── task.c/h            # Task data structure
│   └── checklist.c/h       # Checklist/subtask structure
├── data/
│   ├── storage.c/h         # File I/O operations
│   ├── markdown_parser.c/h # Markdown read/parse
│   └── markdown_writer.c/h # Markdown generation
├── logic/
│   ├── board_manager.c/h   # Board CRUD operations
│   ├── task_manager.c/h    # Task CRUD operations
│   └── navigation.c/h      # Cursor/focus management
└── utils/
    ├── string_utils.c/h    # String helpers
    ├── config.c/h          # Configuration management
    └── error.c/h           # Error handling
```

### Structure Rationale

- **ui/**: All ncurses-specific code isolated here for easy porting/testing
- **models/**: Pure data structures, no UI dependencies
- **data/**: Persistence layer, markdown format handling
- **logic/**: Business logic independent of UI
- **utils/**: Shared utilities

## Architectural Patterns

### Pattern 1: Event Loop (Main Loop)

**What:** Central event loop that polls for input and dispatches to handlers
**When to use:** All ncurses applications require this pattern
**Trade-offs:** Simple but must handle all input cases explicitly

```c
// main application loop
void app_run(Application* app) {
    int ch;
    
    while (app->running) {
        // Update display
        ui_refresh(app->ui);
        
        // Get input (blocking or non-blocking)
        ch = getch();
        
        // Handle input
        if (ch == KEY_MOUSE) {
            MEVENT event;
            getmouse(&event);
            handle_mouse_event(app, &event);
        } else {
            handle_key_event(app, ch);
        }
        
        // Process any pending updates
        process_updates(app);
    }
}
```

### Pattern 2: Model-View Separation

**What:** Strict separation between data models and UI rendering
**When to use:** When data persistence and UI are independent concerns
**Trade-offs:** More boilerplate but enables testing and multiple UIs

```c
// models/task.h - Pure data
typedef struct {
    char* id;
    char* title;
    char* description;
    ChecklistItem* checklist;
    time_t created_at;
    time_t updated_at;
} Task;

// ui/task_view.h - UI rendering
void task_view_render(WINDOW* win, const Task* task, 
                      int y, int width, bool selected);
```

### Pattern 3: Panel Stack (Z-Order Management)

**What:** Use ncurses panel library to manage overlapping windows
**When to use:** When you have popup dialogs, menus, or overlapping elements
**Trade-offs:** Adds dependency on panel library (usually included)

```c
// Creating stacked UI elements
WINDOW* dialog_win = newwin(height, width, y, x);
PANEL* dialog_panel = new_panel(dialog_win);
top_panel(dialog_panel);  // Bring to front
update_panels();
doupdate();
```

### Pattern 4: Command Pattern for User Actions

**What:** Encapsulate user actions as objects for undo/redo and testing
**When to use:** When you need undo functionality or action logging
**Trade-offs:** More complex but enables powerful features

```c
typedef struct {
    const char* name;
    void (*execute)(void* context);
    void (*undo)(void* context);
} Command;

// Example commands
Command* create_move_task_command(Task* task, Column* from, Column* to);
Command* create_edit_task_command(Task* task, const char* new_title);
```

## Data Flow

### User Action to Persistence Flow

```
┌─────────────┐     ┌──────────────┐     ┌───────────────┐
│   User      │────▶│  Input       │────▶│   Command     │
│   Action    │     │  Handler     │     │   Handler     │
└─────────────┘     └──────────────┘     └───────┬───────┘
                                                 │
                       ┌──────────────────────────┘
                       ▼
┌─────────────┐     ┌──────────────┐     ┌───────────────┐
│   File      │◄────│   Markdown   │◄────│   Model       │
│   Saved     │     │   Writer     │     │   Updated     │
└─────────────┘     └──────────────┘     └───────────────┘
                                                 ▲
                       ┌──────────────────────────┘
                       ▼
              ┌────────────────┐
              │   UI Refresh   │
              │   (update_panels│
              │    doupdate)   │
              └────────────────┘
```

### Key Data Flows

1. **Task Creation Flow:**
   - User presses 'n' → Input handler
   - Show dialog → Get title input
   - Create Task model → Add to Column
   - Persist to markdown → Refresh UI

2. **Task Move Flow:**
   - User selects task + presses 'l' (or drags with mouse)
   - Navigation logic finds target column
   - Update task's column reference
   - Persist board state → Refresh column views

3. **Board Switch Flow:**
   - User presses 'b' for board menu
   - Load available boards from filesystem
   - Show selection dialog
   - Unload current board → Load selected board
   - Full UI refresh

## Integration Points

### ncurses Integration

| Component | ncurses Function | Purpose |
|-----------|------------------|---------|
| Initialization | `initscr()`, `cbreak()`, `noecho()` | Setup terminal mode |
| Input | `keypad()`, `mousemask()`, `getch()` | Enable input types |
| Windows | `newwin()`, `delwin()` | Window management |
| Panels | `new_panel()`, `update_panels()` | Z-order management |
| Colors | `start_color()`, `init_pair()` | Color scheme setup |
| Rendering | `wrefresh()`, `doupdate()` | Screen updates |

### Data Persistence Integration

| Component | Interface | Notes |
|-----------|-----------|-------|
| Markdown Parser | `Board* parse_markdown(const char* filepath)` | Parse .md to model |
| Markdown Writer | `void write_markdown(const Board* board, const char* path)` | Serialize to .md |
| File Watcher | `int watch_board_directory(const char* path)` | Detect external changes |

### Internal Boundaries

| Boundary | Communication | Notes |
|----------|---------------|-------|
| UI ↔ Logic | Event callbacks | UI triggers actions, logic updates models |
| Logic ↔ Data | Repository pattern | Logic requests persistence, data layer handles I/O |
| Models ↔ All | Direct access | Models are plain data structures |

## Component Responsibilities

| Component | Responsibility | Implementation Approach |
|-----------|---------------|------------------------|
| **App Controller** | Lifecycle, event loop, mode management | Main loop in `app.c`, state machine for modes |
| **Input Handler** | Translate keys/mouse to actions | Keymap configuration, mouse coordinate translation |
| **Board Manager** | CRUD operations on boards | Filesystem operations, board registry |
| **Task Manager** | Task CRUD, movement, ordering | Linked lists or arrays within columns |
| **Navigation** | Cursor position, focus management | Current column/task indices, visual feedback |
| **UI Renderer** | Draw all windows, handle resizing | Panel-based architecture, responsive layout |
| **Storage** | Save/load markdown files | Atomic writes, error handling, backup on corruption |

## Scaling Considerations

### Single User, Local Storage

This application is designed for single-user, local use. Scaling considerations are minimal:

| Scale Factor | Approach |
|--------------|----------|
| Many boards (100+) | Lazy loading, board list pagination |
| Many tasks per column (100+) | Virtual scrolling within columns |
| Large task descriptions | Truncated display with expand option |
| Large files | Incremental markdown parsing |

### Performance Priorities

1. **Startup time:** Keep under 100ms for typical boards
2. **Input latency:** Immediate response to keyboard navigation
3. **Save operations:** Non-blocking or fast enough to not delay UI

## Anti-Patterns to Avoid

### Anti-Pattern 1: Direct ncurses Calls in Business Logic

**What people do:** Call `mvprintw()` directly from task management code
**Why it's wrong:** Ties business logic to UI, makes testing impossible
**Do this instead:** Business logic updates models, UI layer polls models for display

### Anti-Pattern 2: Blocking I/O in Main Thread

**What people do:** Call `fwrite()` directly during save operations
**Why it's wrong:** Can freeze UI on slow filesystems
**Do this instead:** Use non-blocking I/O or ensure saves are fast and atomic

### Anti-Pattern 3: Mixing Window Management with Data

**What people do:** Store WINDOW pointers in Task structures
**Why it's wrong:** Ties data lifetime to UI lifetime, causes memory issues
**Do this instead:** Keep models pure, create/destroy windows during render phase

### Anti-Pattern 4: No Refresh Strategy

**What people do:** Call `refresh()` after every single change
**Why it's wrong:** Causes flicker and unnecessary terminal updates
**Do this instead:** Batch updates, use `wnoutrefresh()` + `doupdate()` pattern

## ncurses Specific Patterns

### Initialization Sequence

```c
// Standard ncurses initialization
void ncurses_init() {
    setlocale(LC_ALL, "");           // Enable Unicode support
    initscr();                        // Initialize ncurses
    cbreak();                         // Disable line buffering
    noecho();                         // Don't echo input
    keypad(stdscr, TRUE);            // Enable function keys
    mousemask(ALL_MOUSE_EVENTS, NULL); // Enable mouse
    start_color();                    // Enable colors
    use_default_colors();             // Use terminal's default colors
    
    // Initialize color pairs
    init_pair(1, COLOR_WHITE, COLOR_BLUE);   // Selected item
    init_pair(2, COLOR_BLACK, COLOR_WHITE);  // Normal text
    // ... more color pairs
}
```

### Window Update Strategy

```c
// Efficient screen updates using panel library
void refresh_screen() {
    // Update all panels to virtual screen
    update_panels();
    
    // Push virtual screen to physical screen in one operation
    doupdate();
}
```

### Mouse Event Handling

```c
void handle_mouse(MEVENT* event) {
    // Check which window was clicked
    WINDOW* clicked = panel_window(panel_below(NULL));
    
    while (clicked) {
        // Check if click is within window bounds
        int y, x, h, w;
        getbegyx(clicked, y, x);
        getmaxyx(clicked, h, w);
        
        if (event->y >= y && event->y < y + h &&
            event->x >= x && event->x < x + w) {
            // Convert to window-relative coordinates
            int win_y = event->y - y;
            int win_x = event->x - x;
            
            // Dispatch to window's click handler
            window_handle_click(clicked, win_y, win_x, event->bstate);
            return;
        }
        
        clicked = panel_window(panel_below(panel_above(clicked)));
    }
}
```

## Build Order Considering Dependencies

Based on the architecture, the recommended build order is:

### Phase 1: Foundation (No UI)
1. **models/** - Data structures (no dependencies)
2. **utils/** - String utilities, error handling
3. **data/markdown_parser** - Markdown parsing (depends on models)
4. **data/markdown_writer** - Markdown writing (depends on models)
5. **data/storage** - File I/O (depends on parser/writer)

### Phase 2: Core Logic (No UI)
6. **logic/board_manager** - Board operations (depends on models, storage)
7. **logic/task_manager** - Task operations (depends on models, storage)
8. **logic/navigation** - Focus management (depends on models)

### Phase 3: UI Layer
9. **ui/window** - Base window operations (ncurses)
10. **ui/panel_manager** - Panel stack management (ncurses + panel)
11. **ui/input_handler** - Input processing (ncurses)

### Phase 4: Views
12. **ui/task_view** - Task rendering (depends on window, models)
13. **ui/column_view** - Column rendering (depends on window, task_view)
14. **ui/board_view** - Board rendering (depends on window, column_view)
15. **ui/dialogs** - Modal dialogs (depends on window, input_handler)

### Phase 5: Application Integration
16. **app** - Main controller (depends on everything)
17. **main** - Entry point (depends on app)

## Configuration Storage

| Setting | Storage | Location |
|---------|---------|----------|
| Key bindings | Config file | `~/.config/kanban-cli/config` |
| Color scheme | Config file | `~/.config/kanban-cli/config` |
| Board files | Markdown files | `~/.local/share/kanban-cli/boards/` or user-specified |
| Last opened board | State file | `~/.local/share/kanban-cli/state` |

## Sources

- NCURSES Programming HOWTO: https://tldp.org/HOWTO/NCURSES-Programming-HOWTO/
- ncurses man pages (panel, mouse, keyboard): https://invisible-island.net/ncurses/man/
- lazygit architecture patterns (GitHub): https://github.com/jesseduffield/lazygit
- gitui architecture patterns (GitHub): https://github.com/extrawurst/gitui

---
*Architecture research for: Kanban CLI TUI Application*
*Researched: 2025-04-04*
