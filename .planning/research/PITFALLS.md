# Pitfalls Research: TUI Kanban Board with ncurses

**Domain:** Terminal User Interface (TUI) application using ncurses  
**Project:** Kanban CLI  
**Researched:** 2025-04-04  
**Confidence:** HIGH

---

## Critical Pitfalls

### Pitfall 1: Missing refresh() Calls Leading to Blank Screens

**What goes wrong:**  
The screen appears blank or doesn't update after calling `printw()`, `mvaddch()`, or other output functions. The application seems to hang or display nothing.

**Why it happens:**  
ncurses uses a "virtual screen" optimization. All output functions write to an internal buffer (stdscr or window buffers), not directly to the terminal. The actual screen update only happens when `refresh()` or `wrefresh()` is called. Beginners (and even experienced developers) often forget this crucial call.

**How to avoid:**
- Always follow output operations with `refresh()` when using stdscr functions
- Use `wrefresh(win)` after window-specific output operations
- Consider using `wnoutrefresh()` followed by `doupdate()` for multiple window updates to reduce flicker
- Add `refresh()` calls at the end of any screen update sequence

**Warning signs:**
- Screen is blank after program starts
- Output appears only after pressing a key (if getch() triggers a refresh)
- Partial screen updates where some changes show but others don't

**Phase to address:** Phase 1 - Basic TUI Setup

---

### Pitfall 2: Terminal Not Restored on Exit (Garbage Display)

**What goes wrong:**  
After the application exits, the terminal shows garbled text, cursor is invisible, keyboard input isn't echoed, or colors are wrong. The terminal behaves strangely until `reset` command is run.

**Why it happens:**  
If `endwin()` is not called (due to crashes, signals, or premature exits), ncurses doesn't restore the terminal to its original state. The terminal remains in curses mode with modified settings.

**How to avoid:**
- Always call `endwin()` before exiting, even on error conditions
- Install signal handlers for SIGINT, SIGTERM that call `endwin()` before exiting
- Use `atexit()` to register cleanup functions
- Consider using `newterm()` instead of `initscr()` for better control over cleanup
- Wrap critical sections in try/catch (C++) or use setjmp/longjmp carefully

```c
// Signal handler example
void cleanup_handler(int sig) {
    endwin();
    exit(sig);
}

// Installation
signal(SIGINT, cleanup_handler);
signal(SIGTERM, cleanup_handler);
```

**Warning signs:**
- Terminal doesn't show typed commands after program exits
- Cursor is invisible in shell
- Colors/formatting from program persist in shell output
- Must run `reset` or `tput reset` to fix terminal

**Phase to address:** Phase 1 - Basic TUI Setup

---

### Pitfall 3: Terminal Resize Handling Failure

**What goes wrong:**  
When the terminal window is resized, the application doesn't adapt. Content is cut off, cursor positioning is wrong, or the application crashes when trying to draw outside the new bounds.

**Why it happens:**  
Standard SVr4 curses doesn't have built-in resize handling. ncurses added `SIGWINCH` support as an extension, but applications must explicitly handle it. The `LINES` and `COLS` global variables don't update automatically without action.

**How to avoid:**
- Enable `KEY_RESIZE` by checking for it in the input loop
- Use `is_term_resized()` to detect resize events
- Call `resizeterm(new_lines, new_cols)` or `endwin()` followed by `refresh()` to reinitialize
- Re-layout all windows after detecting resize
- Use `getmaxyx(stdscr, rows, cols)` instead of relying on `LINES`/`COLS` directly

```c
// Resize handling pattern
if (ch == KEY_RESIZE) {
    endwin();
    refresh();
    // Re-create or resize windows here
    clear();
    redraw_all_windows();
}
```

**Warning signs:**
- Content disappears when terminal is made smaller
- Drawing outside visible area after resize
- Crash on window operations after resize

**Phase to address:** Phase 2 - Window Management & Layout

---

### Pitfall 4: Mouse Event Handling Complexity

**What goes wrong:**  
Mouse clicks aren't detected, coordinates are wrong, or mouse events interfere with keyboard input. X11 terminal cut/paste stops working.

**Why it happens:**  
Mouse support requires:
1. Calling `mousemask()` to enable mouse events
2. Checking for `KEY_MOUSE` in the input loop
3. Calling `getmouse()` to retrieve event details
4. Understanding coordinate systems (ncurses coordinates vs screen coordinates)

Xterm reserves unshifted mouse buttons for applications when mouse tracking is enabled, breaking normal cut/paste.

**How to avoid:**
- Call `mousemask(ALL_MOUSE_EVENTS, NULL)` during initialization
- Always check if `getmouse(&event)` returns OK before processing
- Remember coordinates are 0-based (y, x) not (x, y)
- Use `wmouse_trafo()` to convert between coordinate systems if using subwindows
- Document that users should Shift+click for cut/paste in xterm
- Handle `BUTTON1_PRESSED`, `BUTTON1_RELEASED`, and `BUTTON1_CLICKED` appropriately

```c
MEVENT event;
ch = getch();
if (ch == KEY_MOUSE) {
    if (getmouse(&event) == OK) {
        if (event.bstate & BUTTON1_PRESSED) {
            // event.y, event.x are the coordinates
            handle_click(event.y, event.x);
        }
    }
}
```

**Warning signs:**
- Mouse clicks produce escape sequences on screen instead of being captured
- Coordinates don't match expected positions
- Can't select/copy text from application in xterm

**Phase to address:** Phase 2 - Input Handling (Mouse & Keyboard)

---

### Pitfall 5: Terminal Description Mismatches

**What goes wrong:**  
Colors don't work, line-drawing characters show as 'x' and 'q', function keys send wrong codes, or terminal capabilities aren't recognized.

**Why it happens:**  
ncurses relies on the `TERM` environment variable to find the terminal description. If `TERM` is set incorrectly (e.g., `xterm` when using a different emulator, or `xterm-color` on a 256-color terminal), capabilities won't match reality.

**How to avoid:**
- Don't hardcode terminal types - respect user's `TERM` setting
- Document supported terminals (xterm, rxvt, gnome-terminal, etc.)
- Test with multiple terminal emulators
- Use `infocmp` to verify terminal descriptions
- Handle missing capabilities gracefully (check return values)
- For kanban boards, provide ASCII fallback if ACS characters unavailable

**Warning signs:**
- Box-drawing borders show as letters (q, x, m, j, etc.)
- Colors don't display or display wrong
- Function keys send "[A", "[B" instead of being recognized
- "Error opening terminal: xterm-256color" on remote systems

**Phase to address:** Phase 1 - Basic TUI Setup (ongoing through all phases)

---

### Pitfall 6: Escape Key Ambiguity (Vim Keybindings)

**What goes wrong:**  
Implementing vim-style keybindings is problematic because the Escape key and Alt+key combinations send the same initial byte (ESC). There's a delay when pressing Escape, or Alt combinations are misinterpreted.

**Why it happens:**  
- Escape is sent as a single byte (0x1B)
- Function keys and Alt combinations send escape sequences starting with ESC
- The terminal can't distinguish between "Escape pressed" and "start of escape sequence"
- ncurses uses a timeout (`ESCDELAY` environment variable, default 1000ms) to decide

**How to avoid:**
- For vim-style apps, reduce `ESCDELAY` (e.g., `setenv("ESCDELAY", "25", 1)`)
- Use `nodelay()` or `timeout()` with `notimeout()` for finer control
- Implement your own key parsing for Alt combinations if needed
- Document the Escape delay behavior to users
- Consider using `keypad(stdscr, TRUE)` to get KEY_LEFT, KEY_RIGHT, etc.

```c
// Reduce escape delay for vim-like feel
setenv("ESCDELAY", "25", 1);
initscr();
keypad(stdscr, TRUE);
```

**Warning signs:**
- Lag when pressing Escape to exit modes
- Alt+h interpreted as Escape then 'h'
- Function keys require precise timing to register

**Phase to address:** Phase 2 - Input Handling (Mouse & Keyboard)

---

### Pitfall 7: File Corruption on Crash (Markdown Storage)

**What goes wrong:**  
When the application crashes during file write or the system loses power, the kanban board data file becomes corrupted, partially written, or truncated.

**Why it happens:**  
Writing directly to the main data file means any interruption leaves the file in an inconsistent state. Partial writes, buffer flush failures, or write errors can corrupt the entire board.

**How to avoid:**
- **Atomic writes:** Write to a temporary file, then `rename()` to replace the original
- Use `fsync()` on the file descriptor before closing
- Keep a backup of the previous version
- Implement file format versioning for forward compatibility
- Validate file on load - if corrupt, try backup
- Use structured format (YAML front matter + markdown) for easier recovery

```c
// Safe write pattern
void save_board_atomic(const char* filename, Board* board) {
    char tmpfile[256];
    snprintf(tmpfile, sizeof(tmpfile), "%s.tmp", filename);
    
    FILE* fp = fopen(tmpfile, "w");
    if (!fp) return;
    
    write_board_to_file(fp, board);
    fflush(fp);
    fsync(fileno(fp));  // Ensure data hits disk
    fclose(fp);
    
    rename(tmpfile, filename);  // Atomic operation
}
```

**Warning signs:**
- Zero-byte files after crash
- Truncated JSON/markdown
- Missing tasks or columns after restart
- Parser errors on load

**Phase to address:** Phase 3 - Data Persistence

---

### Pitfall 8: Color Pair Limitations

**What goes wrong:**  
Colors don't display as expected, or the application crashes with "color pair out of range" errors. Background colors show incorrectly when clearing or scrolling.

**Why it happens:**  
- ncurses uses "color pairs" (foreground + background) rather than independent colors
- Default limit is COLOR_PAIRS (usually 64 or 256 depending on ABI)
- Color pair 0 is special (default terminal colors)
- The "back color erase" (bce) capability affects how clearing fills with background color

**How to avoid:**
- Initialize colors early with `start_color()`
- Use `use_default_colors()` to enable transparent/default terminal colors
- Manage color pairs carefully - reuse pairs for same fg/bg combinations
- Test on terminals with and without bce capability
- Check `has_colors()` and `can_change_color()` before using advanced features
- Be aware that different terminals handle colors differently

```c
// Proper color initialization
if (has_colors()) {
    start_color();
    use_default_colors();  // Allow -1 for default colors
    
    // Define color pairs
    init_pair(1, COLOR_RED, -1);     // Red on default bg
    init_pair(2, COLOR_WHITE, COLOR_BLUE);  // White on blue
}
```

**Warning signs:**
- `init_pair()` returns ERR
- Colors appear black-on-black or invisible
- Clearing screen erases background colors
- Different behavior between terminals

**Phase to address:** Phase 1 - Basic TUI Setup

---

### Pitfall 9: Buffer Overflows in Input Handling

**What goes wrong:**  
When editing task titles or descriptions, input buffer overflows cause crashes, data corruption, or security vulnerabilities.

**Why it happens:**  
Using `getstr()` or `wgetstr()` without bounds checking can overflow fixed-size buffers. ncurses' `getstr()` has no length limit by default.

**How to avoid:**
- Always use `getnstr()` or `wgetnstr()` with explicit length limits
- Validate input length before processing
- Allocate buffers dynamically if needed
- Don't trust user input - always bounds-check
- For form-like input, consider using the ncurses Form library

```c
// Safe input
char buffer[256];
getnstr(buffer, sizeof(buffer) - 1);
buffer[sizeof(buffer) - 1] = '\0';  // Ensure null termination
```

**Warning signs:**
- Crash when entering long text
- Stack corruption symptoms
- Overwriting adjacent variables

**Phase to address:** Phase 4 - Task Editing Features

---

### Pitfall 10: Concurrent File Access Issues

**What goes wrong:**  
If the user runs multiple instances of the kanban board or edits the file externally, changes are lost or the file becomes corrupted.

**Why it happens:**  
Plain file I/O has no built-in locking mechanism. Multiple processes writing simultaneously can corrupt the file.

**How to avoid:**
- Use file locking (`flock()` or `lockf()`) when opening files
- Check for file modification time changes before saving
- Implement conflict detection (compare in-memory state with disk)
- Warn user if file was modified externally
- Consider "last write wins" vs "merge changes" strategy
- Document that only one instance should run per board file

```c
// Simple file locking
int fd = open(filename, O_RDWR);
if (flock(fd, LOCK_EX | LOCK_NB) == -1) {
    // File is locked by another process
    warn_user("Another instance may be running");
}
```

**Warning signs:**
- Changes disappear between sessions
- File corruption with multiple instances
- Lost updates when editing externally

**Phase to address:** Phase 3 - Data Persistence

---

## Technical Debt Patterns

| Shortcut | Immediate Benefit | Long-term Cost | When Acceptable |
|----------|-------------------|----------------|-----------------|
| Use global variables for window handles | Simpler code, less passing around | Testing is hard, can't have multiple boards, threading issues | Only in early Phase 1 prototyping |
| Ignore `refresh()` optimization | Less thinking about update logic | Flickering, poor performance over SSH | Never for production |
| Hardcode terminal as "xterm" | Works on your machine | Breaks on rxvt, gnome-terminal, tmux, etc. | Never |
| Skip `endwin()` on error paths | Simpler error handling | Terminal left in unusable state | Never |
| Use fixed-size arrays for tasks | Simple memory management | Board size limits, potential overflows | Early prototyping only |
| Write directly to data file | One less file operation | Corruption risk on crash | Never |

---

## Integration Gotchas

| Integration | Common Mistake | Correct Approach |
|-------------|----------------|------------------|
| Vim/nvim | Conflicting keybindings | Document differences, allow config file for custom bindings |
| tmux/screen | Mouse events intercepted | Document Shift+click for selection, test in multiplexer |
| SSH remote | Terminal type not recognized | Bundle minimal terminfo, fallback to vt100 basics |
| Git version control | Binary files in repo | Ensure markdown output is deterministic (sorted keys, stable format) |
| Editor (vim/emacs) opening tasks | Temp file not cleaned up | Use proper temp file APIs, clean up in signal handlers |

---

## Performance Traps

| Trap | Symptoms | Prevention | When It Breaks |
|------|----------|------------|----------------|
| Redraw entire screen on every change | Lag on large boards, high CPU over SSH | Use `wnoutrefresh()` + `doupdate()`, only redraw changed regions | Boards with 50+ tasks |
| Parse entire markdown file on every save | Slow save operations | Keep in-memory representation, write-only on changes | 100+ tasks |
| No buffering on file writes | Slow saves, disk thrashing | Use stdio buffering, batch updates | Frequent edits |
| Calling `refresh()` in a loop | Flickering, slow rendering | Collect updates, single refresh at end | Any loop doing UI updates |
| Creating windows for every task | Memory exhaustion | Reuse window objects, use pads for large content | 100+ visible tasks |

---

## Security Considerations

| Concern | Risk | Mitigation |
|---------|------|------------|
| Path traversal in board filenames | Read/write arbitrary files | Validate and sanitize filenames, use whitelist |
| Symlink attacks on data files | Write to wrong location | Use `O_NOFOLLOW` when opening, check file ownership |
| Buffer overflow in task content | Code execution | Use bounded input functions, validate lengths |
| World-readable board files | Information disclosure | Set appropriate umask, document file permissions |
| Shell injection in external editor | Arbitrary command execution | Use `execve()` with args array, never shell interpolation |

---

## UX Pitfalls

| Pitfall | User Impact | Better Approach |
|---------|-------------|-----------------|
| No visual feedback on actions | Users don't know if command worked | Status bar messages, flash screen, or beep |
| Inconsistent vim keybindings | Muscle memory conflicts | Strict vim compatibility or clear documentation of differences |
| Mouse required for some operations | Inaccessible in non-mouse terminals | All features available via keyboard |
| No help screen | Users forget keybindings | '?' key shows context-sensitive help |
| Losing work on accidental quit | Data loss | Confirm quit if unsaved changes |
| No indication of modified state | Unsure if save needed | Show "*" or similar indicator when unsaved |
| Cursor invisible in input mode | Disorienting | Always show cursor position, use distinct cursor shapes |

---

## "Looks Done But Isn't" Checklist

- [ ] **File saving:** Often missing atomic writes or fsync — verify crash safety
- [ ] **Terminal cleanup:** Often missing `endwin()` in signal handlers — verify Ctrl+C safety
- [ ] **Resize handling:** Often works only at startup — verify runtime resize
- [ ] **Mouse support:** Often works in xterm but not elsewhere — test in rxvt, gnome-terminal, tmux
- [ ] **Error handling:** Often exits without cleanup — verify all error paths call `endwin()`
- [ ] **Large boards:** Often performs poorly — test with 100+ tasks
- [ ] **File locking:** Often missing — verify behavior with multiple instances
- [ ] **Unicode handling:** Often assumes ASCII — test with non-ASCII task names
- [ ] **Color handling:** Often assumes color support — verify on monochrome terminal
- [ ] **Help documentation:** Often incomplete — ensure all keys documented

---

## Recovery Strategies

| Pitfall | Recovery Cost | Recovery Steps |
|---------|---------------|----------------|
| Terminal corruption | LOW | Run `reset` command, or `stty sane` + `tput reset` |
| Data file corruption | MEDIUM | Restore from backup, implement auto-backup rotation |
| Lost work (crash) | HIGH | Implement auto-save to temp file, recovery on restart |
| Mouse not working | LOW | Fall back to keyboard, show help for keyboard shortcuts |
| Colors wrong | LOW | Check TERM variable, try different terminal emulator |
| Resize broke layout | LOW | Press key to force redraw, or exit and restart |

---

## Pitfall-to-Phase Mapping

| Pitfall | Prevention Phase | Verification |
|---------|------------------|--------------|
| Missing refresh() | Phase 1: Basic TUI Setup | Visual inspection of all screen updates |
| Terminal cleanup | Phase 1: Basic TUI Setup | Test Ctrl+C, SIGTERM handling |
| Color pair limits | Phase 1: Basic TUI Setup | Test on various terminals |
| Resize handling | Phase 2: Window Management | Resize window during operation |
| Mouse handling | Phase 2: Input Handling | Test clicks, verify coordinates |
| Escape key ambiguity | Phase 2: Input Handling | Verify vim-style bindings feel responsive |
| File corruption | Phase 3: Data Persistence | Simulate crashes during save |
| Concurrent access | Phase 3: Data Persistence | Run two instances simultaneously |
| Buffer overflow | Phase 4: Task Editing | Try entering very long text |
| Performance issues | Phase 4-5: Optimization | Test with large boards |

---

## Sources

- [NCURSES FAQ](https://invisible-island.net/ncurses/ncurses.faq.html) — Official FAQ with extensive troubleshooting (HIGH confidence)
- [NCURSES Programming HOWTO](https://tldp.org/HOWTO/NCURSES-Programming-HOWTO/) — Pradeep Padala's comprehensive guide (HIGH confidence)
- ncurses man pages: `curs_initscr`, `curs_refresh`, `curs_mouse`, `curs_getch`, `curs_color` (HIGH confidence)
- Personal experience with TUI application development (MEDIUM confidence)

---

*Pitfalls research for: TUI Kanban Board CLI*  
*Researched: 2025-04-04*
