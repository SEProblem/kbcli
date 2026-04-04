# Phase 2 Research: Input Handling & Modal Editing

**Phase:** 2
**Researched:** 2026-04-04
**Confidence:** HIGH

---

## Domain

Phase 2 adds vim-style navigation, mouse support, modal editing system, and terminal resize handling to the kanban CLI.

### Key Requirements

- NAV-01: hjkl navigation (h=left, l=right, j=down, k=up)
- NAV-02: Arrow keys as alternative
- NAV-03: gg/G for top/bottom jump
- NAV-04: Tab/Shift+Tab for column jumping
- NAV-05: Mouse click selects task
- NAV-06: Mouse scroll within column
- MOD-01: Normal mode (navigation/commands)
- MOD-02: Insert mode (text editing)
- MOD-03: 'i' or Enter to enter Insert mode
- MOD-04: Esc to exit Insert mode
- MOD-05: Visual hints in Normal mode
- APP-04: Terminal resize handling

### From CONTEXT.md Decisions

- **D-01:** Agent's discretion for hjkl conflict (hybrid approach)
- **D-02:** Arrow keys remain (already working)
- **D-03:** Tab/Shift+Tab remain (already working)
- **D-04/D-05:** i/Enter for Insert mode, Esc to exit
- **D-07/D-08:** Full mouse support per PITFALLS.md
- **D-10:** Soft resize using resizeterm()

---

## Validation Architecture

### What Must Be True (Goals)

1. **Vim Navigation Works:** User can navigate with hjkl in their familiar vim patterns
2. **Alternative Keys Work:** Arrow keys function as backup
3. **Jump Commands Work:** gg/G jump to ends, Tab/Shift+Tab switch columns
4. **Mouse Selection:** Click on any task selects it
5. **Mouse Scroll:** Wheel scrolls within column when overflowing
6. **Modal States:** App clearly distinguishes Normal vs Insert mode
7. **Mode Transitions:** i/Enter enter Insert, Esc returns to Normal
8. **Visual Feedback:** Status bar shows mode, column, position
9. **Resize Works:** Terminal resize triggers graceful redraw

### Verification Approach

| Requirement | Verification Method |
|-------------|---------------------|
| NAV-01 | Keypress test - press h/j/k/l verify cursor moves |
| NAV-02 | Press arrow keys verify cursor moves |
| NAV-03 | Press gg verify go top, G verify go bottom |
| NAV-04 | Press Tab verify next column, Shift+Tab prev |
| NAV-05 | Click on task verify selection highlight |
| NAV-06 | Scroll wheel verify content scrolls |
| MOD-01 | Check Normal mode active on startup |
| MOD-02 | Check Insert mode active when editing |
| MOD-03 | Press i/Enter verify Insert mode entered |
| MOD-04 | Press Esc verify Normal mode returned |
| MOD-05 | Check status bar shows actions |
| APP-04 | Resize terminal verify no crash, content redraws |

---

## Implementation Approach

### Mode State Machine

Two modes: Normal (default) and Insert.

```
Normal Mode:
  - hjkl: navigate tasks
  - h: move to previous column (or use </>)
  - l: move to next column
  - j/k: move up/down within column
  - gg/G: jump to top/bottom
  - Tab/Shift+Tab: switch columns
  - i/Enter: enter Insert mode
  - q: quit

Insert Mode:
  - Arrow keys: navigate (input)
  - Enter: confirm edit
  - Esc: return to Normal mode
```

### Input Handler Changes

1. Add `AppMode` enum: `MODE_NORMAL`, `MODE_INSERT`
2. Add mode state to Application struct
3. Modify input handler to check mode before routing keys
4. Route Normal mode keys to navigation handlers
5. Route Insert mode keys to text input handlers

### Mouse Integration

Per PITFALLS.md:
- Call `mousemask(ALL_MOUSE_EVENTS, NULL)` during init
- Check for `KEY_MOUSE` in input loop
- Use `getmouse()` to get coordinates
- Map coordinates to visible tasks

### Resize Handling

Per PITFALLS.md and D-10:
- Catch `KEY_RESIZE` in input loop
- Call `resizeterm(new_lines, new_cols)`
- Recalculate window positions
- Redraw all windows

### Status Bar

Per D-12/D-13:
- Show current mode: "NORMAL" or "-- INSERT --"
- Show current column name
- Show task position (e.g., "3/5" tasks)
- Position at bottom of screen

---

## Integration Points

### From Existing Code (CONTEXT.md)

- `src/input.c`: `handle_input()` function, Selection struct
- `src/main.c`: Event loop pattern, ESCDELAY set to 25ms
- `read_task_title()` in input.c - reusable for Insert mode
- Arrow navigation already works
- g/G already works

### What Needs Creating/Modifying

| File | Action |
|------|--------|
| `src/app.h` | Add AppMode enum, mode field |
| `src/app.c` | Handle mode transitions in event loop |
| `src/input.c` | Add mode-aware key routing |
| `src/ui/status_bar.c` | New file for status bar |
| `src/ui/renderer.c` | Add resize handler call |

---

## Pitfall Considerations

### From PITFALLS.md

- **Pitfall 3:** Resize handling - use resizeterm(), not full reinit
- **Pitfall 4:** Mouse events - enable mousemask, use getmouse()
- **Pitfall 6:** Escape key - reduce ESCDELAY to 25ms for vim feel

### Mitigation

- Resize: Call resizeterm() then redraw all windows
- Mouse: Test in xterm, tmux; document Shift+click for paste
- Escape: Already set to 25ms in Phase 1, should work

---

## Key Technical Details

### Key Bindings (per D-01)

Per CONTEXT.md D-01: Agent's discretion. I'll use:
- h: previous column (leftmost column does nothing)
- l: next column (rightmost column does nothing) 
- j/k: up/down within column
- Arrow keys: alternative (already works)
- Tab/Shift+Tab: column jumping (D-03, already works)
- </>: alternative column navigation

### Mode Indicator

Following vim convention:
- Normal mode: No prefix or "NORMAL"
- Insert mode: "-- INSERT --" in status bar

### File Format for Research Output

This research file covers Phase 2 implementation. All phase requirements are covered.

---

## Sources

- .planning/phases/02-input-handling-modal-editing/02-CONTEXT.md
- .planning/REQUIREMENTS.md
- .planning/research/PITFALLS.md (mouse, resize, escape key sections)
- .planning/research/ARCHITECTURE.md (input handling patterns)
- Prior phase: .planning/phases/01-core-kanban-foundation/01-CONTEXT.md