# kbcli

> A tiny vim-flavored kanban board that lives in your terminal and stores
> everything as plain Markdown on your own disk.

`kbcli` is a single-binary, ncurses-based kanban tool. Three columns
(To Do / In Progress / Done), cards with descriptions and checklists,
multiple boards, vim keybindings, and a markdown file you can `cat`,
`grep`, and check into git like a normal human being.

No daemon. No sync server. No telemetry. No account. No web view. No
electron. No "AI assist" sidebar trying to summarize your TODOs.

## Why

Because every other kanban tool wants to be a SaaS, and the one good
self-hosted one wants to be a database. I wanted something I could
launch in under 100ms, edit with `hjkl`, and check into the same repo
as the project it's tracking.

## Install

Requires a C11 compiler, CMake ≥ 3.16, and ncurses.

```sh
git clone git@github.com:SEProblem/kbcli.git
cd kbcli
cmake -B build
cmake --build build
./build/kanban-cli
```

Drop the resulting binary anywhere on your `$PATH` and you're done.

## Where stuff lives

- Boards:    `~/.local/share/kanban-cli/<board-name>.md`
- Config:    `~/.config/kanban-cli/config`

The board files are plain GitHub-flavored Markdown — checklists,
headers, the works. You can edit them in any text editor, commit them
to a repo, diff them, whatever.

## Keys

`?` inside the app shows the full keymap. The shortlist:

### Board navigation
| key            | does                              |
|----------------|-----------------------------------|
| `h` `j` `k` `l`| move selection                    |
| `H` `L`        | shift card to prev/next column    |
| `J` `K`        | reorder card down/up              |
| `gg` / `G`     | top / bottom of column            |
| `Tab`          | next column                       |
| `o` / `O`      | new card below / above            |
| `i` / `Enter`  | open card popup                   |
| `c`            | open card popup, focus checklist  |
| `d`            | delete card                       |
| `?`            | help overlay                      |
| `q`            | quit                              |

### Inside the card popup
| key                          | does                              |
|------------------------------|-----------------------------------|
| `Tab` / `Shift+Tab`          | next / previous field             |
| `←` `→`                      | move cursor                       |
| `Ctrl+←` `Ctrl+→`            | jump by word                      |
| `Home` `End`                 | line start / line end             |
| `↑` `↓` *(description only)* | move between visual lines         |
| `Enter` *(description only)* | insert newline                    |
| `Backspace` `Delete`         | edit at cursor                    |
| `Esc`                        | save and close                    |

### Checklist field
| key       | does                                       |
|-----------|--------------------------------------------|
| `j` `k`   | navigate items                             |
| `Space`   | toggle checked                             |
| `o` `O`   | new item below / above (enters edit mode)  |
| `J` `K`   | move item down / up                        |
| `d`       | delete item                                |
| `Enter`   | commit current edit                        |

### Boards & commands
| command           | does                                  |
|-------------------|---------------------------------------|
| `:b <name>`       | switch to board `<name>`              |
| `:bn` / `:bp`     | next / previous board                 |
| `:bnew <name>`    | create new board                      |
| `:brename <name>` | rename current board                  |

The active board persists across launches. Empty cards (no title, no
description, no checklist) are auto-discarded on close, so `o` is safe
to mash by accident.

## Storage format

```markdown
## To Do
- [ ] Buy milk
Description: Whole, not 2%. The 2% is a betrayal.
  - [ ] Walk to store
  - [ ] Try not to forget like last time
- [ ] Refactor the auth middleware

## In Progress
- [ ] Write the README

## Done
- [x] Ship it
```

Newlines in descriptions are escaped as `\n` so the parser can stay
single-line-simple. Atomic writes via `mkstemp` + `fsync` + `rename`,
so a crash mid-save won't corrupt your board.

## License

[VIBESHIELD LICENSE v1](./LICENSE) — free for personal and commercial
use, fork it, sell it, embed it, do whatever you want with it. There is
exactly **one** restriction: you may not feed this codebase to an AI to
generate further code for it. The AI budget on this project is spent.
Human PRs only.

(Yes, this README mentions that an AI helped write the original. That's
the budget. Clause 1 of the license explains it. Read it, it's short
and only mildly insufferable.)
