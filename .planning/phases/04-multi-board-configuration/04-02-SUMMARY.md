---
phase: 04-multi-board-configuration
plan: 02
subsystem: configuration
tags: [config, json, keybindings]
dependency_graph:
  requires:
    - APP-05
    - APP-06
    - APP-07
  provides:
    - Configuration file at ~/.config/kanban-cli/config
    - Customizable keybindings via config
    - Configurable board directory
  affects:
    - src/main.c
    - src/input.c
    - src/storage.c
tech_stack:
  added:
    - JSON config parsing
    - Atomic file writes
    - Configurable keybindings
  patterns:
    - Config struct with JSON string for keybindings
    - Atomic write (temp file + fsync + rename)
    - Key mapping conversion at input handler
key_files:
  created:
    - src/config.h
    - src/config.c
  modified:
    - CMakeLists.txt
    - src/main.c
    - src/input.c
    - src/storage.c
decisions:
  - "D-08: Full replacement - config completely replaces default keybindings"
  - "JSON format for configuration file per user preference"
  - "Config loaded on startup before renderer_init"
---

# Phase 4 Plan 2: Configuration System Summary

## One-liner

JSON config file with customizable keybindings and configurable board directory.

## Objective

Implement configuration system: JSON config file, custom keybindings, configurable board directory.

## Tasks Completed

| Task | Name | Commit | Files |
|------|------|--------|-------|
| 1 | Create config module with JSON parsing | ecb9780 | src/config.c, src/config.h, CMakeLists.txt |
| 2 | Apply keybindings from config | 3090a58 | src/input.c, src/config.h |
| 3 | Use config board directory | ee800f5 | src/main.c, src/storage.c |

## Implementation Details

### Task 1: Config Module (ecb9780)

Created the configuration module with JSON parsing capabilities:

- **config.h**: Defines `Config` struct with `board_directory`, `auto_save`, `keybindings`, and `default_board` fields
- **config.c**: Implements:
  - `get_config_path()` - Returns `~/.config/kanban-cli/config`
  - `config_load()` - Loads JSON config, creates default dirs if needed
  - `config_save()` - Atomic JSON write using temp file + fsync + rename
  - `config_get_board_directory()` - Returns configured or default path
  - `apply_keybindings_from_config()` - Parses JSON keybindings object

- Default keybindings: `{"up":"k","down":"j","left":"h","right":"l","create":"n","delete":"d"}`

### Task 2: Keybindings Integration (3090a58)

Integrated config with input handling:

- Added `config.h` include in input.c
- Added key mapping conversion at start of `handle_input()`:
  - Converts key_char to map configured keys to default vim actions
  - Supports: `key_create`, `key_delete`, `key_up`, `key_down`, `key_left`, `key_right`
- Global key variables defined in config.c, declared extern in config.h

### Task 3: Board Directory Integration (ee800f5)

Integrated config with storage layer:

- Added `config.h` include in main.c and storage.c
- Load config and apply keybindings on startup in `main()`
- Modified `get_default_board_path()` to use config's `board_directory` and `default_board`
- Modified `get_boards_directory()` to check config first, fallback to default

## Verification

Build successful with no errors:
```
[100%] Built target kanban-cli
```

Grep verification for config functions and keybindings shows all expected functions present.

## Known Stubs

None - all functionality implemented.

## Deviations from Plan

None - plan executed exactly as written.

## Requirements Met

- APP-05: User can customize keybindings via configuration file ✓
- APP-06: User can set default board directory path via configuration ✓
- APP-07: Configuration file exists at ~/.config/kanban-cli/config ✓

## Duration

Tasks completed in single execution session.

---

*Plan 04-02 complete: Configuration system implemented with JSON config file, customizable keybindings, and configurable board directory.*