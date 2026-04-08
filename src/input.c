/**
 * input.c - Input handling implementation for Kanban CLI
 * 
 * Implements keyboard handling for task create/delete/navigation
 * following vim-style keybindings per D-05, D-06, D-08.
 */

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ncurses.h>

#include "kanban.h"
#include "models.h"
#include "storage.h"
#include "renderer.h"
#include "input.h"
#include "config.h"

/* External board management state - stores current board name */
extern char global_current_board_name[256];

/* Forward declarations */
static void switch_to_board(Board *board, Selection *selection, const char *board_name);

/* Static error message for input failures */
static char input_error[256] = {0};

/* Timestamp for double-g detection (jump to top) */
/* Uses CLOCK_MONOTONIC via timespec for sub-second precision — time_t has only
 * 1-second resolution, which caused false triggers within the same second. */
static struct timespec last_g_press_time = {0, 0};
#define G_DOUBLE_TAP_TIMEOUT_MS 300

/* Active field for card popup: 0=title, 1=description, 2=checklist */
static int popup_active_field = 0;

/* When in checklist field: 0 = navigating items, 1 = inline-editing the
 * item at board->checklist_index. Reset whenever we leave the popup or
 * switch fields, so the checklist field always opens in nav mode. */
static int checklist_editing = 0;

/* In-place text editing cursors for the title and description fields.
 * These are byte offsets into task->title / task->description. They get
 * reset to the end-of-string each time the popup opens (so if the user
 * is opening an existing card to keep typing, the cursor lands where
 * they'd expect — past the existing text). */
static int title_cursor = 0;
static int desc_cursor = 0;

/* For description: which visual line is at the top of the rendered
 * 3-row desc area. Auto-adjusted as the cursor moves so the cursor
 * always remains visible. */
static int desc_scroll_line = 0;

/* Lazily-resolved key codes for Ctrl+Left and Ctrl+Right. ncurses doesn't
 * give them a stable KEY_* constant — we have to ask terminfo for the
 * sequence and look up the code ncurses assigned to it. -1 = not present
 * in this terminal's terminfo (we'll just ignore the binding). */
static int key_ctrl_left  = -1;
static int key_ctrl_right = -1;
static int special_keys_inited = 0;

static void init_special_keys_lazy(void) {
    if (special_keys_inited) return;
    special_keys_inited = 1;
    char *s;
    s = tigetstr("kLFT5");
    if (s != NULL && s != (char*)-1) {
        int kc = key_defined(s);
        if (kc > 0) key_ctrl_left = kc;
    }
    s = tigetstr("kRIT5");
    if (s != NULL && s != (char*)-1) {
        int kc = key_defined(s);
        if (kc > 0) key_ctrl_right = kc;
    }
}

int card_popup_active_field(void) {
    return popup_active_field;
}

int card_popup_checklist_editing(void) {
    return checklist_editing;
}

int card_popup_title_cursor(void) { return title_cursor; }
int card_popup_desc_cursor(void)  { return desc_cursor; }
int card_popup_desc_scroll(void)  { return desc_scroll_line; }

/* ---------- in-place text editing helpers ---------- */

static int is_word_char(unsigned char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9') || c == '_';
}

/* vim/readline-style word-left: skip non-word chars, then skip word chars. */
static int word_left(const char *s, int cursor) {
    if (cursor <= 0) return 0;
    cursor--;
    while (cursor > 0 && !is_word_char((unsigned char)s[cursor])) cursor--;
    while (cursor > 0 && is_word_char((unsigned char)s[cursor - 1])) cursor--;
    return cursor;
}

static int word_right(const char *s, int cursor) {
    int len = (int)strlen(s);
    if (cursor >= len) return len;
    while (cursor < len && is_word_char((unsigned char)s[cursor])) cursor++;
    while (cursor < len && !is_word_char((unsigned char)s[cursor])) cursor++;
    return cursor;
}

/* Insert one character at cursor. Returns 1 on success, 0 if buffer full. */
static int text_insert_char(char *s, size_t cap, int *cursor, char ch) {
    size_t len = strlen(s);
    if (len + 1 >= cap) return 0;
    if (*cursor < 0) *cursor = 0;
    if (*cursor > (int)len) *cursor = (int)len;
    /* Shift right (including the trailing NUL) */
    memmove(s + *cursor + 1, s + *cursor, len - *cursor + 1);
    s[*cursor] = ch;
    (*cursor)++;
    return 1;
}

static void text_delete_back(char *s, int *cursor) {
    if (*cursor <= 0) return;
    size_t len = strlen(s);
    memmove(s + *cursor - 1, s + *cursor, len - *cursor + 1);
    (*cursor)--;
}

static void text_delete_forward(char *s, int *cursor) {
    size_t len = strlen(s);
    if ((size_t)*cursor >= len) return;
    memmove(s + *cursor, s + *cursor + 1, len - *cursor);
}

/* ---------- description wrapping layout (shared with renderer) ---------- */

/*
 * Compute the starts of each visual line for `text` wrapped at inner_w
 * columns, with hard breaks at '\n'. Fills line_starts[0..count-1].
 * Returns the visual line count (>= 1 even for empty text).
 *
 * The renderer uses this to know where each visual line begins so it can
 * print substrings; the input handler uses it to translate the cursor
 * byte offset into a (line, column) pair for vertical navigation.
 */
int desc_compute_lines(const char *text, int inner_w, int *line_starts, int max_lines) {
    if (line_starts == NULL || max_lines <= 0 || inner_w < 1) return 0;
    line_starts[0] = 0;
    int count = 1;
    int len = (text != NULL) ? (int)strlen(text) : 0;
    int pos = 0;
    while (pos < len && count < max_lines) {
        int max_end = pos + inner_w;
        if (max_end > len) max_end = len;
        int line_end = pos;
        while (line_end < max_end && text[line_end] != '\n') line_end++;
        if (line_end < len && text[line_end] == '\n') {
            /* Hard break — next visual line starts AFTER the newline */
            pos = line_end + 1;
        } else if (line_end == max_end && line_end < len) {
            /* Soft wrap — next visual line starts at line_end (no \n eaten) */
            pos = line_end;
        } else {
            break;
        }
        line_starts[count++] = pos;
    }
    return count;
}

/* Locate (line, col) for a cursor byte position given a precomputed layout. */
void desc_cursor_locate(int cursor, const int *line_starts, int line_count,
                        int *out_line, int *out_col) {
    if (line_count <= 0) { *out_line = 0; *out_col = 0; return; }
    int line = 0;
    for (int i = 1; i < line_count; i++) {
        if (line_starts[i] <= cursor) line = i;
        else break;
    }
    *out_line = line;
    *out_col = cursor - line_starts[line];
}

/* After moving the description cursor, scroll the desc viewport so the
 * cursor's visual line stays inside the 3 visible rows. */
static void desc_clamp_scroll(const char *text, int inner_w) {
    int line_starts[64];
    int n = desc_compute_lines(text, inner_w, line_starts, 64);
    int cl, cc;
    desc_cursor_locate(desc_cursor, line_starts, n, &cl, &cc);
    (void)cc;
    if (cl < desc_scroll_line) desc_scroll_line = cl;
    if (cl >= desc_scroll_line + 3) desc_scroll_line = cl - 2;
    if (desc_scroll_line < 0) desc_scroll_line = 0;
}

/* Reset cursor state to the end of each field's current text. Called
 * once per popup-open transition (not on Tab between fields). */
static void popup_reset_cursors(Task *task) {
    if (task == NULL) return;
    title_cursor = (int)strlen(task->title);
    desc_cursor  = (int)strlen(task->description);
    desc_scroll_line = 0;
}

/* Swap two ChecklistItems' data in place. The pointers stay put, so we
 * don't need a prev-pointer walk on the singly-linked list. */
static void checklist_swap_data(ChecklistItem *a, ChecklistItem *b) {
    if (a == NULL || b == NULL || a == b) return;
    char tmp_text[256];
    int tmp_checked = a->checked;
    strncpy(tmp_text, a->text, sizeof(tmp_text) - 1);
    tmp_text[sizeof(tmp_text) - 1] = '\0';
    a->checked = b->checked;
    strncpy(a->text, b->text, sizeof(a->text) - 1);
    a->text[sizeof(a->text) - 1] = '\0';
    b->checked = tmp_checked;
    strncpy(b->text, tmp_text, sizeof(b->text) - 1);
    b->text[sizeof(b->text) - 1] = '\0';
}

/* Walk the checklist to the item at the given index, or NULL. */
static ChecklistItem *checklist_at(Task *task, int index) {
    if (task == NULL || index < 0) return NULL;
    ChecklistItem *it = task->checklist;
    while (it != NULL && index > 0) { it = it->next; index--; }
    return it;
}

/* Insert a new (empty) checklist item at the given index. Index 0 means
 * at the head; an index >= count appends. Returns the inserted item. */
static ChecklistItem *checklist_insert_empty_at(Task *task, int index) {
    if (task == NULL) return NULL;
    ChecklistItem *new_item = checklist_item_create("");
    if (new_item == NULL) return NULL;
    if (index <= 0 || task->checklist == NULL) {
        new_item->next = task->checklist;
        task->checklist = new_item;
        return new_item;
    }
    ChecklistItem *prev = task->checklist;
    while (prev->next != NULL && index > 1) { prev = prev->next; index--; }
    new_item->next = prev->next;
    prev->next = new_item;
    return new_item;
}

/* External board filename for auto-save */
extern char global_board_filename[512];

/* Forward declarations for mouse handling - NAV-05, NAV-06 */
void handle_mouse_event(Board *board, Selection *selection);
void navigation_select_task_at(Board *board, Selection *selection, int screen_y, int screen_x);
void navigation_scroll_up(Selection *selection);
void navigation_scroll_down(Selection *selection, Board *board);

const char* input_get_error(void) {
    if (input_error[0] == '\0') {
        return NULL;
    }
    return input_error;
}

void input_clear_error(void) {
    input_error[0] = '\0';
}

/**
 * Insert a task at a specific position in a column
 * Used by 'o' (below) and 'O' (above) commands
 */
static void task_insert_at(Column *col, Task *new_task, int position) {
    if (col == NULL || new_task == NULL) return;
    
    /* If position is 0 or negative, insert at head */
    if (position <= 0 || col->tasks == NULL) {
        new_task->next = col->tasks;
        col->tasks = new_task;
        col->task_count++;
        return;
    }
    
    /* Find the task at position-1 */
    Task *prev = col->tasks;
    int idx = position - 1;
    
    while (prev->next != NULL && idx > 0) {
        prev = prev->next;
        idx--;
    }
    
    /* Insert after prev */
    new_task->next = prev->next;
    prev->next = new_task;
    col->task_count++;
}

/**
 * Get task at a specific index in a column
 */
static Task* task_at_index(Column *col, int index) {
    if (col == NULL || index < 0) return NULL;
    
    Task *task = col->tasks;
    int idx = index;
    
    while (task != NULL && idx > 0) {
        task = task->next;
        idx--;
    }
    
    return task;
}

/**
 * Read checklist item text from user
 */
int read_checklist_item(char *buffer, size_t size) {
    if (buffer == NULL || size == 0) return -1;
    
    int height, width;
    getmaxyx(stdscr, height, width);
    
    /* Clear input buffer */
    buffer[0] = '\0';
    
    /* Create input prompt */
    int prompt_y = height - 2;
    int prompt_x = 0;
    
    /* Show prompt */
    mvprintw(prompt_y, prompt_x, "Checklist item: ");
    clrtoeol();
    refresh();
    
    /* Move cursor to input position */
    move(prompt_y, prompt_x + 17);
    
    /* Read input with bounds checking */
    echo();
    int ch;
    size_t pos = 0;
    
    while ((ch = getch()) != '\n' && ch != KEY_ENTER) {
        if (ch == 27) {  /* Escape to cancel */
            buffer[0] = '\0';
            noecho();
            return -1;
        }
        if (ch == KEY_BACKSPACE || ch == 127) {  /* Backspace */
            if (pos > 0) {
                pos--;
                buffer[pos] = '\0';
                delch();
            }
        } else if (ch >= 32 && ch <= 126 && pos < size - 1) {
            buffer[pos++] = (char)ch;
            buffer[pos] = '\0';
        }
    }
    
    noecho();
    
    /* Clear the input line */
    mvprintw(prompt_y, prompt_x, "%*s", width, " ");
    
    return (buffer[0] == '\0') ? -1 : 0;
}

/**
 * Handle terminal resize event.
 *
 * By the time KEY_RESIZE is delivered, ncurses has already updated LINES /
 * COLS via SIGWINCH. We just need to wipe the curscr/stdscr backing buffers
 * so cells from the *old* geometry don't bleed through, and force a full
 * repaint on the next event-loop iteration. The loop's call to render_board()
 * will pick up the new dimensions via getmaxyx().
 *
 * Calling endwin() + refresh() is the canonical "the terminal changed under
 * us, re-enter cleanly" pattern; it also fixes cases where the underlying
 * terminal was resized while we weren't actively reading input.
 */
void handle_resize(Board *board) {
    (void)board;
    endwin();
    refresh();
    clear();
}

/**
 * Clamp the scroll offset for the current column so sel->task_index remains
 * visible. Geometry must mirror render_column:
 *   - bottom chrome (hint+status) = 2 rows
 *   - column bottom border        = 1 row
 *   - column header (title+rule)  = 2 rows
 *   - blank gap below header      = 1 row
 *   - each compact card           = 3 rows + 1 gap = 4 rows
 * So available card rows = height - 2 - 1 - 2 - 1 = height - 6, and the
 * number of compact cards that fit is (rows + 1) / 4 (the trailing gap of
 * the last card isn't required).
 */
static void clamp_scroll(Selection *sel) {
    int height, width;
    getmaxyx(stdscr, height, width);
    (void)width;
    int card_rows = height - 6;
    int visible_cards = (card_rows + 1) / 4;
    if (visible_cards < 1) visible_cards = 1;
    int col_i = sel->column_index;
    int offset = get_scroll_offset(col_i);
    if (sel->task_index < offset) {
        set_scroll_offset(col_i, sel->task_index);
    } else if (sel->task_index >= offset + visible_cards) {
        set_scroll_offset(col_i, sel->task_index - visible_cards + 1);
    }
}

int handle_input(Board *board, int key, Selection *selection) {
    if (board == NULL || selection == NULL) return 0;

    /* MODE_CARD_POPUP: centered overlay — handles title/description/checklist editing */
    if (board->app_mode == MODE_CARD_POPUP) {
        init_special_keys_lazy();
        /* Get the selected task */
        Column *col = &board->columns[selection->column_index];
        Task *task = col->tasks;
        int idx = selection->task_index;
        while (task != NULL && idx > 0) { task = task->next; idx--; }
        if (task == NULL) { board->app_mode = MODE_NORMAL; return 0; }

        int checklist_count_val = checklist_count(task);

        if (key == 27 && !checklist_editing) { /* Esc: save and close popup */
            /* If the user closes a card with no title, no description, and
             * no checklist items, treat it as a cancel: delete the card.
             * This makes 'o' on an empty column safe — press 'o' by mistake,
             * Esc, no garbage card is left behind. Cards that were already
             * non-empty before editing won't trigger this because the user
             * would have to backspace away every field to land here. */
            int is_empty = (task->title[0] == '\0' &&
                            task->description[0] == '\0' &&
                            task->checklist == NULL);
            if (is_empty) {
                task_delete(&col->tasks, task->id);
                col->task_count--;
                /* Clamp selection back to a valid card (or 0 if column is
                 * now empty). */
                if (selection->task_index >= col->task_count)
                    selection->task_index = col->task_count - 1;
                if (selection->task_index < 0)
                    selection->task_index = 0;
            }
            if (board->filename[0] != '\0') board_save(board, board->filename);
            board->app_mode = MODE_NORMAL;
            popup_active_field = 0;
            board->checklist_index = 0;
            checklist_editing = 0;
            return 0;
        }

        if (key == '\t' && !checklist_editing) { /* Tab: advance field */
            popup_active_field = (popup_active_field + 1) % 3;
            checklist_editing = 0;
            return 0;
        }

        if (key == KEY_BTAB && !checklist_editing) { /* Shift+Tab: prev field */
            popup_active_field = (popup_active_field + 2) % 3;
            checklist_editing = 0;
            return 0;
        }

        /* Per-field routing */
        if (popup_active_field == 0) { /* Title field — in-place editor */
            int tlen = (int)strlen(task->title);
            if (title_cursor > tlen) title_cursor = tlen;
            if (title_cursor < 0)    title_cursor = 0;

            if (key == KEY_LEFT) {
                if (title_cursor > 0) title_cursor--;
            } else if (key == KEY_RIGHT) {
                if (title_cursor < tlen) title_cursor++;
            } else if (key == key_ctrl_left || key == KEY_SLEFT) {
                title_cursor = word_left(task->title, title_cursor);
            } else if (key == key_ctrl_right || key == KEY_SRIGHT) {
                title_cursor = word_right(task->title, title_cursor);
            } else if (key == KEY_HOME || key == 1 /* Ctrl-A */) {
                title_cursor = 0;
            } else if (key == KEY_END  || key == 5 /* Ctrl-E */) {
                title_cursor = tlen;
            } else if (key == KEY_BACKSPACE || key == 127 || key == 8) {
                text_delete_back(task->title, &title_cursor);
            } else if (key == KEY_DC) {
                text_delete_forward(task->title, &title_cursor);
            } else if (key >= 32 && key <= 126) {
                text_insert_char(task->title, sizeof(task->title), &title_cursor, (char)key);
            }
            return 0;
        }

        if (popup_active_field == 1) { /* Description field — in-place editor with newlines */
            int dlen = (int)strlen(task->description);
            if (desc_cursor > dlen) desc_cursor = dlen;
            if (desc_cursor < 0)    desc_cursor = 0;
            int inner_w = renderer_card_popup_inner_width();

            if (key == KEY_LEFT) {
                if (desc_cursor > 0) desc_cursor--;
            } else if (key == KEY_RIGHT) {
                if (desc_cursor < dlen) desc_cursor++;
            } else if (key == key_ctrl_left || key == KEY_SLEFT) {
                desc_cursor = word_left(task->description, desc_cursor);
            } else if (key == key_ctrl_right || key == KEY_SRIGHT) {
                desc_cursor = word_right(task->description, desc_cursor);
            } else if (key == KEY_HOME || key == 1 /* Ctrl-A */) {
                /* Jump to start of current visual line */
                int ls[64];
                int n = desc_compute_lines(task->description, inner_w, ls, 64);
                int cl, cc; desc_cursor_locate(desc_cursor, ls, n, &cl, &cc);
                desc_cursor = ls[cl];
            } else if (key == KEY_END || key == 5 /* Ctrl-E */) {
                /* Jump to end of current visual line */
                int ls[64];
                int n = desc_compute_lines(task->description, inner_w, ls, 64);
                int cl, cc; desc_cursor_locate(desc_cursor, ls, n, &cl, &cc);
                int end = (cl + 1 < n) ? ls[cl + 1] : dlen;
                /* Don't include the trailing \n in the line */
                if (end > 0 && end <= dlen && task->description[end - 1] == '\n') end--;
                desc_cursor = end;
            } else if (key == KEY_UP) {
                int ls[64];
                int n = desc_compute_lines(task->description, inner_w, ls, 64);
                int cl, cc; desc_cursor_locate(desc_cursor, ls, n, &cl, &cc);
                if (cl > 0) {
                    int prev_start = ls[cl - 1];
                    int prev_end   = ls[cl] - 1;
                    if (prev_end < prev_start) prev_end = prev_start;
                    /* Strip a trailing newline that belongs to the prev line */
                    if (prev_end >= 0 && task->description[prev_end] == '\n') prev_end--;
                    int prev_len = prev_end - prev_start + 1;
                    if (prev_len < 0) prev_len = 0;
                    int target_col = (cc < prev_len) ? cc : prev_len;
                    desc_cursor = prev_start + target_col;
                }
            } else if (key == KEY_DOWN) {
                int ls[64];
                int n = desc_compute_lines(task->description, inner_w, ls, 64);
                int cl, cc; desc_cursor_locate(desc_cursor, ls, n, &cl, &cc);
                if (cl + 1 < n) {
                    int next_start = ls[cl + 1];
                    int next_end   = (cl + 2 < n) ? ls[cl + 2] - 1 : dlen - 1;
                    if (next_end >= 0 && next_end < dlen &&
                        task->description[next_end] == '\n') next_end--;
                    int next_len = next_end - next_start + 1;
                    if (next_len < 0) next_len = 0;
                    int target_col = (cc < next_len) ? cc : next_len;
                    desc_cursor = next_start + target_col;
                }
            } else if (key == KEY_BACKSPACE || key == 127 || key == 8) {
                text_delete_back(task->description, &desc_cursor);
            } else if (key == KEY_DC) {
                text_delete_forward(task->description, &desc_cursor);
            } else if (key == '\n' || key == KEY_ENTER) {
                /* Insert literal newline. Description supports multi-line. */
                text_insert_char(task->description, MAX_DESC_LEN, &desc_cursor, '\n');
            } else if (key >= 32 && key <= 126) {
                text_insert_char(task->description, MAX_DESC_LEN, &desc_cursor, (char)key);
            }
            task->desc_len = strlen(task->description);
            desc_clamp_scroll(task->description, inner_w);
            return 0;
        }

        if (popup_active_field == 2) { /* Checklist field */
            /* ----- Inline edit mode: typing into the text of the current item ----- */
            if (checklist_editing) {
                ChecklistItem *item = checklist_at(task, board->checklist_index);
                if (item == NULL) { checklist_editing = 0; return 0; }
                if (key == 27 || key == '\n' || key == KEY_ENTER) {
                    /* Esc / Enter: commit and go back to nav mode */
                    checklist_editing = 0;
                    if (board->filename[0] != '\0') board_save(board, board->filename);
                    return 0;
                }
                size_t ilen = strlen(item->text);
                if ((key == KEY_BACKSPACE || key == 127) && ilen > 0) {
                    item->text[ilen - 1] = '\0';
                } else if (key >= 32 && key <= 126 && ilen < sizeof(item->text) - 1) {
                    item->text[ilen] = (char)key;
                    item->text[ilen + 1] = '\0';
                }
                return 0;
            }

            /* ----- Navigation mode ----- */
            if (key == 'j' || key == KEY_DOWN) {
                if (board->checklist_index < checklist_count_val - 1)
                    board->checklist_index++;
                return 0;
            }
            if (key == 'k' || key == KEY_UP) {
                if (board->checklist_index > 0)
                    board->checklist_index--;
                return 0;
            }
            if (key == ' ') { /* Toggle current item */
                ChecklistItem *item = checklist_at(task, board->checklist_index);
                if (item != NULL) {
                    checklist_item_toggle(item);
                    if (board->filename[0] != '\0') board_save(board, board->filename);
                }
                return 0;
            }
            /* 'o' — insert new empty item AFTER the current one (or at end if
             * the list is empty), enter inline edit mode on it. Vim semantics. */
            if (key == 'o') {
                int new_idx = (checklist_count_val == 0) ? 0
                                                         : board->checklist_index + 1;
                if (checklist_insert_empty_at(task, new_idx) != NULL) {
                    board->checklist_index = new_idx;
                    checklist_editing = 1;
                }
                return 0;
            }
            /* 'O' — insert new empty item BEFORE the current one, enter edit. */
            if (key == 'O') {
                int new_idx = board->checklist_index;
                if (checklist_count_val == 0) new_idx = 0;
                if (checklist_insert_empty_at(task, new_idx) != NULL) {
                    board->checklist_index = new_idx;
                    checklist_editing = 1;
                }
                return 0;
            }
            /* 'J' — pull current item DOWN one slot (swap with next). */
            if (key == 'J') {
                ChecklistItem *cur  = checklist_at(task, board->checklist_index);
                ChecklistItem *next = (cur != NULL) ? cur->next : NULL;
                if (next != NULL) {
                    checklist_swap_data(cur, next);
                    board->checklist_index++;
                    if (board->filename[0] != '\0') board_save(board, board->filename);
                }
                return 0;
            }
            /* 'K' — pull current item UP one slot (swap with previous). */
            if (key == 'K') {
                if (board->checklist_index > 0) {
                    ChecklistItem *prev = checklist_at(task, board->checklist_index - 1);
                    ChecklistItem *cur  = (prev != NULL) ? prev->next : NULL;
                    if (prev != NULL && cur != NULL) {
                        checklist_swap_data(prev, cur);
                        board->checklist_index--;
                        if (board->filename[0] != '\0') board_save(board, board->filename);
                    }
                }
                return 0;
            }
            if (key == 'd') { /* Delete current checklist item */
                if (checklist_count_val > 0) {
                    ChecklistItem *item = checklist_at(task, board->checklist_index);
                    if (item != NULL) {
                        checklist_item_delete(task, item);
                        if (board->checklist_index >= checklist_count_val - 1 && board->checklist_index > 0)
                            board->checklist_index--;
                        if (board->filename[0] != '\0') board_save(board, board->filename);
                    }
                }
                return 0;
            }
        }

        return 0;
    }

    /* MODE_HELP: any key closes the help overlay */
    if (board->app_mode == MODE_HELP) {
        board->app_mode = MODE_NORMAL;
        return 0;
    }

    /* MODE_NORMAL: route keys for navigation and commands */

    /* 'i' - open card popup (D-01) */
    if (key == 'i') {
        Column *col = &board->columns[selection->column_index];
        if (col->task_count > 0) {
            board->app_mode = MODE_CARD_POPUP;
            popup_active_field = 0;
            board->checklist_index = 0;
            checklist_editing = 0;
            popup_reset_cursors(task_at_index(col, selection->task_index));
        }
        return 0;
    }

    /* 'c' - open card popup focused on checklist field */
    if (key == 'c') {
        Column *col = &board->columns[selection->column_index];
        if (col->task_count > 0) {
            board->app_mode = MODE_CARD_POPUP;
            popup_active_field = 2;
            board->checklist_index = 0;
            checklist_editing = 0;
            popup_reset_cursors(task_at_index(col, selection->task_index));
        }
        return 0;
    }

    /* Enter key - open card popup (D-01) */
    if (key == KEY_ENTER || key == '\n') {
        Column *col = &board->columns[selection->column_index];
        if (col->task_count > 0) {
            board->app_mode = MODE_CARD_POPUP;
            popup_active_field = 0;
            board->checklist_index = 0;
            checklist_editing = 0;
            popup_reset_cursors(task_at_index(col, selection->task_index));
        }
        return 0;
    }
    
    /* '?' - show help overlay */
    if (key == '?') {
        board->app_mode = MODE_HELP;
        return 0;
    }

    /* Colon command mode - enter command input */
    if (key == ':') {
        handle_colon_command(board, selection);
        return 0;
    }
    
    Column *col = &board->columns[selection->column_index];
    int task_count = col->task_count;
    
    /* Map configurable keybindings (from ~/.config/kanban-cli/config.json)
     * onto the canonical action keys used by the switch below. */
    char key_char = (char)key;
    if      (key_char == key_create) key = 'o';
    else if (key_char == key_delete) key = 'd';
    else if (key_char == key_up)     key = 'k';
    else if (key_char == key_down)   key = 'j';
    else if (key_char == key_left)   key = 'h';
    else if (key_char == key_right)  key = 'l';
    
    switch (key) {
        /* 'o' / 'O' - create a new card and immediately enter the popup
         * focused on the title field. We deliberately do NOT use a bottom-bar
         * prompt: that put text on the hint-strip row and made the typed
         * title appear far away from the actual card. Editing in the popup
         * keeps the user's eyes on the card they just made. */
        case 'o':
        case 'O': {
            Task *new_task = task_create("");
            if (new_task != NULL) {
                /* Compute the desired insert slot, then clamp to the column's
                 * new size. The clamp matters when the column was empty:
                 * task_index defaults to 0 and 'o' would otherwise compute
                 * insert_pos=1, leaving the selection pointing past the only
                 * card and rendering a blank popup. */
                int insert_pos = (key == 'o') ? selection->task_index + 1
                                              : selection->task_index;
                task_insert_at(col, new_task, insert_pos);
                if (insert_pos >= col->task_count) insert_pos = col->task_count - 1;
                if (insert_pos < 0) insert_pos = 0;
                selection->task_index = insert_pos;
                clamp_scroll(selection);
                if (board->filename[0] != '\0') {
                    board_save(board, board->filename);
                }
                /* Hand off to the popup, focused on the title field. */
                board->app_mode = MODE_CARD_POPUP;
                popup_active_field = 0;
                board->checklist_index = 0;
                checklist_editing = 0;
                popup_reset_cursors(new_task);
            }
            break;
        }
        
        /* 'd' or 'x' - delete selected task (D-08) */
        case 'd':
        case 'x': {
            Task *sel_task = task_at_index(col, selection->task_index);
            if (sel_task != NULL) {
                /* Delete the task */
                task_delete(&col->tasks, sel_task->id);
                col->task_count--;
                
                /* Adjust selection if needed */
                if (selection->task_index >= col->task_count && col->task_count > 0) {
                    selection->task_index = col->task_count - 1;
                }
                if (col->task_count == 0) {
                    selection->task_index = 0;
                }
                
                /* Auto-save per D-14 and STO-03 */
                if (board->filename[0] != '\0') {
                    board_save(board, board->filename);
                }
            }
            break;
        }
        
        /* Arrow keys - navigation */
        case KEY_DOWN:
        case 'j': {
            /* Move selection down */
            if (selection->task_index < task_count - 1) {
                selection->task_index++;
            }
            clamp_scroll(selection);
            break;
        }
        
        case KEY_UP:
        case 'k': {
            /* Move selection up */
            if (selection->task_index > 0) {
                selection->task_index--;
            }
            clamp_scroll(selection);
            break;
        }
        
        /* Arrow keys - column navigation */
        case KEY_RIGHT: {
            /* Move to next column */
            if (selection->column_index < 2) {
                selection->column_index++;
                /* Clamp task index to valid range */
                int new_count = board->columns[selection->column_index].task_count;
                if (selection->task_index >= new_count && new_count > 0) {
                    selection->task_index = new_count - 1;
                } else if (new_count == 0) {
                    selection->task_index = 0;
                }
            }
            clamp_scroll(selection);
            break;
        }
        
        case KEY_LEFT: {
            /* Move to previous column */
            if (selection->column_index > 0) {
                selection->column_index--;
                /* Clamp task index to valid range */
                int new_count = board->columns[selection->column_index].task_count;
                if (selection->task_index >= new_count && new_count > 0) {
                    selection->task_index = new_count - 1;
                } else if (new_count == 0) {
                    selection->task_index = 0;
                }
            }
            clamp_scroll(selection);
            break;
        }
        
        case 'l': {
            /* Navigate to next column (right) - vim-style navigation */
            if (selection->column_index < 2) {
                selection->column_index++;
                /* Clamp task index to valid range */
                int new_count = board->columns[selection->column_index].task_count;
                if (selection->task_index >= new_count && new_count > 0) {
                    selection->task_index = new_count - 1;
                } else if (new_count == 0) {
                    selection->task_index = 0;
                }
            }
            clamp_scroll(selection);
            break;
        }
        
        case 'h': {
            /* Navigate to previous column (left) - vim-style navigation */
            if (selection->column_index > 0) {
                selection->column_index--;
                /* Clamp task index to valid range */
                int new_count = board->columns[selection->column_index].task_count;
                if (selection->task_index >= new_count && new_count > 0) {
                    selection->task_index = new_count - 1;
                } else if (new_count == 0) {
                    selection->task_index = 0;
                }
            }
            clamp_scroll(selection);
            break;
        }
        
        /* Tab - jump to next column */
        case '\t': {
            selection->column_index = (selection->column_index + 1) % 3;
            int new_count = board->columns[selection->column_index].task_count;
            if (selection->task_index >= new_count && new_count > 0) {
                selection->task_index = new_count - 1;
            } else if (new_count == 0) {
                selection->task_index = 0;
            }
            clamp_scroll(selection);
            break;
        }
        
        /* Shift+Tab (KEY_BTAB) - jump to previous column */
        case KEY_BTAB: {
            selection->column_index = (selection->column_index + 2) % 3;
            int new_count = board->columns[selection->column_index].task_count;
            if (selection->task_index >= new_count && new_count > 0) {
                selection->task_index = new_count - 1;
            } else if (new_count == 0) {
                selection->task_index = 0;
            }
            clamp_scroll(selection);
            break;
        }
        
        /* 'g' - jump to first task (top of column), supports double-tap gg */
        case 'g': {
            /* Use CLOCK_MONOTONIC for millisecond precision; time_t has only
             * 1-second resolution and caused false triggers / missed presses. */
            struct timespec now;
            clock_gettime(CLOCK_MONOTONIC, &now);
            long elapsed_ms = (now.tv_sec - last_g_press_time.tv_sec) * 1000L
                            + (now.tv_nsec - last_g_press_time.tv_nsec) / 1000000L;
            /* elapsed_ms > 0 guards the pathological same-nanosecond case */
            if (elapsed_ms > 0 && elapsed_ms < G_DOUBLE_TAP_TIMEOUT_MS) {
                selection->task_index = 0;
                clamp_scroll(selection);
            }
            /* Store timestamp for next press */
            last_g_press_time = now;
            /* Re-validate offset after any prior resize even for a single 'g' */
            clamp_scroll(selection);
            break;
        }
        
        /* 'G' - jump to last task (bottom of column) */
        case 'G': {
            selection->task_index = (task_count > 0) ? task_count - 1 : 0;
            clamp_scroll(selection);
            break;
        }
        
        /* 'v' - toggle between compact and detailed view */
        case 'v': {
            board->detailed_view = !board->detailed_view;
            /* Reset checklist index when toggling view */
            board->checklist_index = 0;
            break;
        }
        
        /* 'q' or 'Q' - quit */
        case 'q':
        case 'Q': {
            return 1;  /* Signal quit */
        }
        
        /* Mouse event handling - NAV-05, NAV-06 */
        case KEY_MOUSE: {
            handle_mouse_event(board, selection);
            break;
        }
        
        /* Handle resize - soft resize with resizeterm */
        case KEY_RESIZE: {
            handle_resize(board);
            break;
        }
        
        /* 'H' (Shift+h) - move task to left column */
        case 'H': {
            if (move_left(board, selection) == 0) {
                if (board->filename[0] != '\0') {
                    board_save(board, board->filename);
                }
            }
            break;
        }

        /* 'L' (Shift+l) - move task to right column */
        case 'L': {
            if (move_right(board, selection) == 0) {
                if (board->filename[0] != '\0') {
                    board_save(board, board->filename);
                }
            }
            break;
        }

        /* 'J' (Shift+j) - move task up within column (D-11) */
        case 'J': {
            if (move_up(board, selection) == 0) {
                /* Auto-save per D-14 and STO-03 */
                if (board->filename[0] != '\0') {
                    board_save(board, board->filename);
                }
            }
            break;
        }

        /* 'K' (Shift+k) - move task down within column (D-11) */
        case 'K': {
            if (move_down(board, selection) == 0) {
                /* Auto-save per D-14 and STO-03 */
                if (board->filename[0] != '\0') {
                    board_save(board, board->filename);
                }
            }
            break;
        }
        
        default:
            break;
    }
    
    return 0;  /* Continue running */
}

int wait_for_key(void) {
    return getch();
}

/**
 * Handle mouse events - NAV-05, NAV-06
 * Processes mouse clicks and scroll wheel events
 * 
 * @param board Pointer to the board
 * @param selection Pointer to current selection state
 */
void handle_mouse_event(Board *board, Selection *selection) {
    MEVENT event;
    
    if (getmouse(&event) == OK) {
        /* Handle button 1 double-click - open card popup (NAV-05); tested first
         * because ncurses may set both DOUBLE and PRESSED on a double-click */
        if (event.bstate & BUTTON1_DOUBLE_CLICKED) {
            navigation_select_task_at(board, selection, event.y, event.x);
            board->app_mode = MODE_CARD_POPUP;
            popup_active_field = 0;
            checklist_editing = 0;
            {
                Column *mcol = &board->columns[selection->column_index];
                popup_reset_cursors(task_at_index(mcol, selection->task_index));
            }
        } else if (event.bstate & BUTTON1_PRESSED ||
                   event.bstate & BUTTON1_CLICKED) {
            /* Navigate to task at clicked position */
            navigation_select_task_at(board, selection, event.y, event.x);
        }
        
        /* Handle scroll wheel up - NAV-06 */
        if (event.bstate & BUTTON4_PRESSED) {
            navigation_scroll_up(selection);
        }
        
        /* Handle scroll wheel down - NAV-06 */
        if (event.bstate & BUTTON5_PRESSED) {
            navigation_scroll_down(selection, board);
        }
    }
}

/**
 * Navigate to task at mouse coordinates - NAV-05
 * Maps screen coordinates to task selection
 * 
 * @param board Pointer to the board
 * @param selection Pointer to current selection state
 * @param screen_y Screen row coordinate
 * @param screen_x Screen column coordinate
 */
void navigation_select_task_at(Board *board, Selection *selection, int screen_y, int screen_x) {
    if (board == NULL || selection == NULL) return;
    
    /* Mirror render_board geometry exactly - NAV-05 */
    int num_columns = 3;
    int border_total = (num_columns - 1) * 1;  /* BORDER_WIDTH=1 */
    int col_width = (COLS - border_total) / num_columns;
    if (col_width < 20) col_width = 20;  /* MIN_COLUMN_WIDTH */
    int total_width = (col_width * num_columns) + border_total;
    int start_x = (COLS - total_width) / 2;
    if (start_x < 0) start_x = 0;
    
    /* Detect which column was clicked */
    int clicked_col = -1;
    for (int i = 0; i < num_columns; i++) {
        int col_start = start_x + i * (col_width + 1);
        if (screen_x >= col_start && screen_x < col_start + col_width) {
            clicked_col = i;
            break;
        }
    }
    if (clicked_col < 0) return;  /* click landed on a border or outside */
    
    /* Task area starts at row 3 (header_y=1 + HEADER_HEIGHT=2) */
    int task_y_base = 3;
    if (screen_y < task_y_base) return;  /* click in header area */
    int task_idx = get_scroll_offset(clicked_col) + (screen_y - task_y_base);
    
    /* Get the column and clamp task index */
    Column *column = &board->columns[clicked_col];
    if (column->task_count == 0) return;
    if (task_idx < 0) task_idx = 0;
    if (task_idx >= column->task_count) task_idx = column->task_count - 1;
    
    selection->column_index = clicked_col;
    selection->task_index = task_idx;
}

/**
 * Scroll up within current column - NAV-06
 * Moves selection up by 3 tasks
 * 
 * @param selection Pointer to current selection state
 */
void navigation_scroll_up(Selection *selection) {
    if (selection == NULL) return;
    
    /* Move up by 3 tasks per NAV-06 */
    selection->task_index -= 3;
    if (selection->task_index < 0) {
        selection->task_index = 0;
    }
    clamp_scroll(selection);
}

/**
 * Scroll down within current column - NAV-06
 * Moves selection down by 3 tasks
 * 
 * @param selection Pointer to current selection state
 * @param board Pointer to the board (for task count)
 */
void navigation_scroll_down(Selection *selection, Board *board) {
    if (selection == NULL || board == NULL) return;
    
    Column *col = &board->columns[selection->column_index];
    int max_index = (col->task_count > 0) ? col->task_count - 1 : 0;
    
    /* Move down by 3 tasks per NAV-06 */
    selection->task_index += 3;
    if (selection->task_index > max_index) {
        selection->task_index = max_index;
    }
    clamp_scroll(selection);
}

/* Current board name for multi-board support */
char global_current_board_name[256] = "default";

/* Board list cache for :bn/:bp navigation */
static char** cached_board_list = NULL;
static int cached_board_count = 0;
static int cached_current_index = -1;

/**
 * Refresh the board list cache
 */
static void refresh_board_list_cache(void) {
    /* Free old cache */
    if (cached_board_list != NULL) {
        board_list_free(cached_board_list, cached_board_count);
        cached_board_list = NULL;
        cached_board_count = 0;
        cached_current_index = -1;
    }
    
    /* Get new list */
    board_list_boards(&cached_board_list, &cached_board_count);
    
    /* Find current board index */
    if (global_current_board_name[0] != '\0') {
        for (int i = 0; i < cached_board_count; i++) {
            if (cached_board_list[i] != NULL && 
                strcmp(cached_board_list[i], global_current_board_name) == 0) {
                cached_current_index = i;
                break;
            }
        }
    }
}

/**
 * Persist `name` as the default board in the user's config file so the next
 * launch reopens it instead of always falling back to "default". Best-effort:
 * we don't surface errors here because the in-memory switch already happened
 * and the user shouldn't be blocked.
 */
static void persist_active_board(const char *name) {
    if (name == NULL || name[0] == '\0') return;
    Config cfg;
    config_load(&cfg);
    strncpy(cfg.default_board, name, sizeof(cfg.default_board) - 1);
    cfg.default_board[sizeof(cfg.default_board) - 1] = '\0';
    config_save(&cfg);
}

/**
 * Switch to a board by name
 * Saves current board and loads new one
 */
static void switch_to_board(Board *board, Selection *selection, const char *board_name) {
    if (board == NULL || board_name == NULL) return;

    /* Save current board first */
    if (board->filename[0] != '\0') {
        board_save(board, board->filename);
    }

    /* Get boards directory and build new path */
    char dir_path[512];
    if (get_boards_directory(dir_path, sizeof(dir_path)) != 0) {
        return;
    }

    char file_path[512];
    snprintf(file_path, sizeof(file_path), "%s%s.md", dir_path, board_name);

    /* Check if board exists */
    if (!board_exists(board_name)) {
        return;
    }

    /* Free current board tasks */
    board_free(board);

    /* Reinitialize board */
    board_init(board);

    /* Load new board */
    board_load(board, file_path);

    /* Update current board name */
    strncpy(global_current_board_name, board_name, sizeof(global_current_board_name) - 1);
    global_current_board_name[sizeof(global_current_board_name) - 1] = '\0';

    /* Reset selection */
    selection->column_index = 0;
    selection->task_index = 0;

    /* Refresh cache */
    refresh_board_list_cache();

    /* Persist this as the active board so the next launch reopens it. */
    persist_active_board(board_name);
}

/**
 * Switch to next board in list
 */
void switch_to_next_board(Board *board, Selection *selection) {
    /* Refresh cache if needed */
    if (cached_board_list == NULL) {
        refresh_board_list_cache();
    }
    
    if (cached_board_count <= 1) return;
    
    int next_index = (cached_current_index + 1) % cached_board_count;
    if (cached_board_list[next_index] != NULL) {
        switch_to_board(board, selection, cached_board_list[next_index]);
    }
}

/**
 * Switch to previous board in list
 */
void switch_to_previous_board(Board *board, Selection *selection) {
    /* Refresh cache if needed */
    if (cached_board_list == NULL) {
        refresh_board_list_cache();
    }
    
    if (cached_board_count <= 1) return;
    
    int prev_index = (cached_current_index - 1 + cached_board_count) % cached_board_count;
    if (cached_board_list[prev_index] != NULL) {
        switch_to_board(board, selection, cached_board_list[prev_index]);
    }
}

/**
 * Read board name from user input
 * Uses ncurses input field with prompt
 */
static int read_board_name(char *buffer, size_t size) {
    if (buffer == NULL || size == 0) return -1;
    
    int height, width;
    getmaxyx(stdscr, height, width);
    
    buffer[0] = '\0';
    
    int prompt_y = height - 2;
    int prompt_x = 0;
    
    mvprintw(prompt_y, prompt_x, "Board name: ");
    clrtoeol();
    refresh();
    
    move(prompt_y, prompt_x + 12);
    
    echo();
    int ch;
    size_t pos = 0;
    
    while ((ch = getch()) != '\n' && ch != KEY_ENTER) {
        if (ch == 27) {
            buffer[0] = '\0';
            noecho();
            return -1;
        }
        if (ch == KEY_BACKSPACE || ch == 127) {
            if (pos > 0) {
                pos--;
                buffer[pos] = '\0';
                delch();
            }
        } else if (ch >= 32 && ch <= 126 && pos < size - 1) {
            buffer[pos++] = (char)ch;
            buffer[pos] = '\0';
        }
    }
    
    noecho();
    mvprintw(prompt_y, prompt_x, "%*s", width, " ");
    
    return (buffer[0] == '\0') ? -1 : 0;
}

/**
 * Create new board and switch to it
 */
void create_new_board(Board *board, Selection *selection) {
    char board_name[256] = {0};
    
    if (read_board_name(board_name, sizeof(board_name)) != 0) {
        return;
    }
    
    /* Validate board name */
    if (board_name[0] == '\0') return;
    
    /* Create the board */
    if (board_create(board_name) != 0) {
        /* Board might already exist */
        return;
    }
    
    /* Switch to the new board */
    switch_to_board(board, selection, board_name);
}

/**
 * Parse and execute colon commands
 * Returns 1 if quit requested, 0 otherwise
 */
int handle_colon_command(Board *board, Selection *selection) {
    /* Read command into buffer */
    char command[256] = {0};
    int height, width;
    getmaxyx(stdscr, height, width);
    
    int prompt_y = height - 2;
    int prompt_x = 0;
    
    mvprintw(prompt_y, prompt_x, ":");
    clrtoeol();
    refresh();
    
    move(prompt_y, prompt_x + 1);
    
    echo();
    int ch;
    size_t pos = 0;
    
    while ((ch = getch()) != '\n' && ch != KEY_ENTER) {
        if (ch == 27) {
            noecho();
            return 0;
        }
        if (ch == KEY_BACKSPACE || ch == 127) {
            if (pos > 0) {
                pos--;
                command[pos] = '\0';
                delch();
            }
        } else if (ch >= 32 && ch <= 126 && pos < sizeof(command) - 1) {
            command[pos++] = (char)ch;
            command[pos] = '\0';
        }
    }
    
    noecho();
    mvprintw(prompt_y, prompt_x, "%*s", width, " ");
    
    /* Parse commands */
    if (strcmp(command, "bn") == 0 || strcmp(command, "bnext") == 0) {
        /* Switch to next board */
        switch_to_next_board(board, selection);
    } else if (strcmp(command, "bp") == 0 || strcmp(command, "bprev") == 0) {
        /* Switch to previous board */
        switch_to_previous_board(board, selection);
    } else if (strncmp(command, "b ", 2) == 0) {
        /* Switch to board by name */
        char *board_name = command + 2;
        if (board_exists(board_name)) {
            switch_to_board(board, selection, board_name);
        } else {
            snprintf(input_error, sizeof(input_error), "No board named \"%s\"", board_name);
        }
    } else if (strcmp(command, "bnew") == 0 || strcmp(command, "bcreate") == 0) {
        /* Create new board */
        create_new_board(board, selection);
    } else if (strcmp(command, "blist") == 0 || strcmp(command, "boards") == 0) {
        /* Show board list menu */
        char *selected_board = show_board_list_menu();
        if (selected_board != NULL) {
            switch_to_board(board, selection, selected_board);
            free(selected_board);
        }
    } else if (strncmp(command, "brename ", 8) == 0) {
        /* :brename <new_name> — rename the current board file on disk and
         * point this in-memory Board at the new path so subsequent saves
         * land in the right place. */
        const char *new_name = command + 8;
        while (*new_name == ' ') new_name++;
        if (*new_name == '\0') {
            snprintf(input_error, sizeof(input_error), "Usage: :brename <new_name>");
        } else if (board_exists(new_name)) {
            snprintf(input_error, sizeof(input_error),
                     "Board \"%s\" already exists", new_name);
        } else if (board_rename(global_current_board_name, new_name) != 0) {
            snprintf(input_error, sizeof(input_error),
                     "Could not rename to \"%s\"", new_name);
        } else {
            /* Update in-memory state to point at the new file. */
            char dir_path[512];
            if (get_boards_directory(dir_path, sizeof(dir_path)) == 0) {
                snprintf(board->filename, sizeof(board->filename),
                         "%s%s.md", dir_path, new_name);
            }
            strncpy(global_current_board_name, new_name,
                    sizeof(global_current_board_name) - 1);
            global_current_board_name[sizeof(global_current_board_name) - 1] = '\0';
            refresh_board_list_cache();
            /* Persist so the next launch reopens the renamed board, not
             * the old (now-missing) name. */
            persist_active_board(new_name);
        }
    } else if (command[0] != '\0') {
        snprintf(input_error, sizeof(input_error),
                 "Unknown command: :%s", command);
    }

    return 0;
}