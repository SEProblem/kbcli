/**
 * renderer.c - Board rendering implementation for Kanban CLI
 * 
 * Implements panel-based architecture for rendering kanban boards
 * with terminal-adaptive column widths and reverse video selection.
 */

#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <panel.h>

#include "kanban.h"
#include "renderer.h"
#include "storage.h"
#include "models.h"
#include "input.h"

/* Access current board name and error state from input.c */
extern char global_current_board_name[256];
extern const char* input_get_error(void);
extern void input_clear_error(void);

/* Scroll offsets for each column (for column-local scrolling) */
static int scroll_offsets[3] = {0, 0, 0};

/* Minimum column width */
#define MIN_COLUMN_WIDTH 22

/* Gap between columns. Zero so each column's right border sits flush
 * against the next column's left border (two distinct colored vlines). */
#define BORDER_WIDTH 0

/* Header height (for column title) */
#define HEADER_HEIGHT 2

/* Bottom chrome: hint strip + status bar */
#define HINT_BAR_HEIGHT   1
#define STATUS_BAR_HEIGHT 1
#define BOTTOM_CHROME     (HINT_BAR_HEIGHT + STATUS_BAR_HEIGHT)

/* Color pair IDs (only used when has_colors()). 0 = default. */
#define PAIR_TODO         1
#define PAIR_INPROG       2
#define PAIR_DONE         3
#define PAIR_STATUS       4
#define PAIR_HINT         5
#define PAIR_MODE_NORMAL  6
#define PAIR_MODE_POPUP   7
#define PAIR_MODE_HELP    8

static int colors_enabled = 0;

void renderer_init_colors(void) {
    if (!has_colors()) {
        colors_enabled = 0;
        return;
    }
    start_color();
    use_default_colors();
    init_pair(PAIR_TODO,        COLOR_CYAN,   -1);
    init_pair(PAIR_INPROG,      COLOR_YELLOW, -1);
    init_pair(PAIR_DONE,        COLOR_GREEN,  -1);
    init_pair(PAIR_STATUS,      COLOR_WHITE,  COLOR_BLUE);
    init_pair(PAIR_HINT,        COLOR_CYAN,   -1);
    init_pair(PAIR_MODE_NORMAL, COLOR_BLACK,  COLOR_GREEN);
    init_pair(PAIR_MODE_POPUP,  COLOR_BLACK,  COLOR_YELLOW);
    init_pair(PAIR_MODE_HELP,   COLOR_BLACK,  COLOR_CYAN);
    colors_enabled = 1;
}

static int column_pair(int column_index) {
    switch (column_index) {
        case 0: return PAIR_TODO;
        case 1: return PAIR_INPROG;
        case 2: return PAIR_DONE;
        default: return 0;
    }
}

static void k_attr_on(int pair, int extra) {
    if (colors_enabled && pair) attron(COLOR_PAIR(pair));
    if (extra) attron(extra);
}

static void k_attr_off(int pair, int extra) {
    if (extra) attroff(extra);
    if (colors_enabled && pair) attroff(COLOR_PAIR(pair));
}

void renderer_init(void) {
    /* Initialize scroll offsets */
    scroll_offsets[0] = 0;
    scroll_offsets[1] = 0;
    scroll_offsets[2] = 0;
}

int get_scroll_offset(int column_index) {
    if (column_index >= 0 && column_index < 3) {
        return scroll_offsets[column_index];
    }
    return 0;
}

void set_scroll_offset(int column_index, int offset) {
    if (column_index >= 0 && column_index < 3 && offset >= 0) {
        scroll_offsets[column_index] = offset;
    }
}

void highlight_selected(int selected) {
    if (selected) {
        attron(A_REVERSE);
    }
}

/* How many display rows a task will consume when rendered with the given
 * inner width and view mode. Mirrors the layout in render_task. */
static int task_total_rows(const Task *task, int inner_w, int detailed_view) {
    if (task == NULL) return 0;
    int body_rows = 1;  /* title */
    if (detailed_view) {
        if (task->description[0] != '\0') {
            int dlen = (int)strlen(task->description);
            int desc_rows = (dlen + inner_w - 1) / inner_w;
            if (desc_rows < 1) desc_rows = 1;
            body_rows += desc_rows;
        }
        const ChecklistItem *it = task->checklist;
        while (it != NULL) { body_rows++; it = it->next; }
    }
    return body_rows + 2;  /* + top/bottom border */
}

int render_task(Task *task, int y, int x, int width, int selected,
                int detailed_view, int column_index) {
    if (task == NULL || width < 4) return 1;

    int is_done = (column_index == 2);
    int pair    = column_pair(column_index);

    /* Inner content width = total - 2 borders - 2 padding */
    int inner_w = width - 4;
    if (inner_w < 1) inner_w = 1;

    /* Compute body rows (between top and bottom border) */
    int body_rows = 1;  /* title row */
    int desc_rows = 0;
    int cl_rows   = 0;
    if (detailed_view) {
        if (task->description[0] != '\0') {
            int dlen = (int)strlen(task->description);
            desc_rows = (dlen + inner_w - 1) / inner_w;
            if (desc_rows < 1) desc_rows = 1;
            body_rows += desc_rows;
        }
        ChecklistItem *it = task->checklist;
        while (it != NULL) { cl_rows++; it = it->next; }
        body_rows += cl_rows;
    }

    int total_rows = body_rows + 2;  /* +top +bottom border */

    /* Render order: clear → inner content → borders LAST. Drawing borders
     * after all the inner writes makes it impossible for a stray inner write
     * (UTF-8 width mismatch, off-by-one truncation) to clobber a pipe. */

    /* 1. Clear the entire card cell range (including border cells) */
    for (int r = 0; r < total_rows; r++) {
        for (int c = 0; c < width; c++) {
            mvaddch(y + r, x + c, ' ');
        }
    }

    /* Title row coordinates */
    int title_x = x + 2;
    int title_y = y + 1;

    /* Truncate title to inner_w (leave room for done checkmark if applicable) */
    int title_max = inner_w;
    if (is_done) title_max -= 2;  /* room for " ✓" */
    if (title_max < 1) title_max = 1;

    char title_buf[512];
    int tlen = (int)strlen(task->title);
    if (tlen > title_max) {
        if (title_max >= 4) {
            snprintf(title_buf, sizeof(title_buf), "%.*s...", title_max - 3, task->title);
        } else {
            snprintf(title_buf, sizeof(title_buf), "%.*s", title_max, task->title);
        }
    } else {
        snprintf(title_buf, sizeof(title_buf), "%s", task->title);
    }

    int title_extra = 0;
    if (selected) title_extra |= A_REVERSE;
    if (is_done)  title_extra |= A_DIM;
    if (title_extra) attron(title_extra);
    mvprintw(title_y, title_x, "%s", title_buf);
    if (title_extra) attroff(title_extra);

    /* Done checkmark on the right of the title row. Plain ASCII so we don't
     * depend on wide-char ncurses (we link against plain ncurses, which would
     * write each UTF-8 byte as a separate cell and overflow the title area). */
    if (is_done) {
        k_attr_on(pair, A_BOLD);
        mvaddch(title_y, x + width - 3, '*');
        k_attr_off(pair, A_BOLD);
    }

    /* Detailed view body */
    if (detailed_view) {
        int row = title_y + 1;

        /* Description (wrapped) */
        if (desc_rows > 0) {
            const char *src = task->description;
            int src_len = (int)strlen(src);
            int pos = 0;
            for (int dr = 0; dr < desc_rows && pos < src_len; dr++) {
                int chunk = inner_w;
                if (pos + chunk > src_len) chunk = src_len - pos;
                char buf[256];
                if (chunk >= (int)sizeof(buf)) chunk = (int)sizeof(buf) - 1;
                strncpy(buf, src + pos, chunk);
                buf[chunk] = '\0';
                if (is_done) attron(A_DIM);
                mvprintw(row, title_x, "%s", buf);
                if (is_done) attroff(A_DIM);
                row++;
                pos += chunk;
            }
        }

        /* Checklist items. ASCII-only marks for the same single-byte ncurses
         * reason as the done checkmark above. */
        ChecklistItem *it = task->checklist;
        while (it != NULL) {
            const char *mark = it->checked ? "[x]" : "[ ]";
            int text_max = inner_w - 4;  /* "[x] " = 4 cols */
            if (text_max < 0) text_max = 0;
            char buf[280];
            snprintf(buf, sizeof(buf), "%s %.*s", mark, text_max, it->text);
            int dim_on = is_done || it->checked;
            if (dim_on) attron(A_DIM);
            mvprintw(row, title_x, "%s", buf);
            if (dim_on) attroff(A_DIM);
            row++;
            it = it->next;
        }
    }

    /* ----- Borders LAST so nothing can clobber them ----- */
    int border_extra = 0;
    if (selected) border_extra |= A_BOLD;
    else if (is_done) border_extra |= A_DIM;

    k_attr_on(pair, border_extra);

    /* Top */
    mvaddch(y, x, ACS_ULCORNER);
    for (int i = 1; i < width - 1; i++) mvaddch(y, x + i, ACS_HLINE);
    mvaddch(y, x + width - 1, ACS_URCORNER);

    /* Sides */
    for (int r = 1; r <= body_rows; r++) {
        mvaddch(y + r, x,             ACS_VLINE);
        mvaddch(y + r, x + width - 1, ACS_VLINE);
    }

    /* Bottom */
    mvaddch(y + body_rows + 1, x, ACS_LLCORNER);
    for (int i = 1; i < width - 1; i++) mvaddch(y + body_rows + 1, x + i, ACS_HLINE);
    mvaddch(y + body_rows + 1, x + width - 1, ACS_LRCORNER);

    k_attr_off(pair, border_extra);

    /* Chevron overdraw on the title row when selected */
    if (selected) {
        k_attr_on(pair, A_BOLD);
        mvaddch(title_y, x,             '>');
        mvaddch(title_y, x + width - 1, '<');
        k_attr_off(pair, A_BOLD);
    }

    return total_rows;
}

void render_column(Column *col, int column_index, int start_x, int width,
                   int selected_col, int sel_task_index, int scroll_offset,
                   int detailed_view) {
    if (col == NULL) return;

    int height, max_x_unused;
    getmaxyx(stdscr, height, max_x_unused);
    (void)max_x_unused;
    /* Reserve: 2 rows of bottom chrome (hint + status) AND 1 row for the
     * column bottom border. Cards may occupy rows up to but not including
     * card_max_y. */
    int bottom_border_y = height - BOTTOM_CHROME - 1;
    int card_max_y      = bottom_border_y;

    int pair = column_pair(column_index);

    /* Inner content lives between the two side borders. */
    int inner_x     = start_x + 2;
    int inner_width = width - 4;
    if (inner_width < 1) inner_width = 1;

    /* Column header: bold colored name + count badge */
    int header_y = 0;
    char header_buf[96];
    snprintf(header_buf, sizeof(header_buf), "%s  %d", col->name, col->task_count);
    int hlen = (int)strlen(header_buf);
    int hx = inner_x + (inner_width - hlen) / 2;
    if (hx < inner_x) hx = inner_x;
    k_attr_on(pair, A_BOLD);
    mvprintw(header_y, hx, "%s", header_buf);
    k_attr_off(pair, A_BOLD);

    /* Header underline rule */
    k_attr_on(pair, A_DIM);
    for (int i = inner_x; i < inner_x + inner_width; i++) {
        mvaddch(header_y + 1, i, ACS_HLINE);
    }
    k_attr_off(pair, A_DIM);

    /* Render tasks first; column side borders are drawn AFTER so nothing
     * a card writes can clobber them. */
    int task_y = HEADER_HEIGHT + 1;  /* leave a gap below header */
    Task *task = col->tasks;
    int visible_index = 0;

    while (task != NULL && visible_index < scroll_offset) {
        task = task->next;
        visible_index++;
    }

    while (task != NULL) {
        int rows = task_total_rows(task, inner_width - 4, detailed_view);
        if (task_y + rows > card_max_y) {
            /* Card would overflow the visible area — drawing it now would
             * leave its lower borders un-rendered (silent mvaddch failure
             * for off-screen rows), producing the "missing |" artifact. */
            break;
        }
        int is_selected = (selected_col && sel_task_index == visible_index);
        render_task(task, task_y, inner_x, inner_width,
                    is_selected, detailed_view, column_index);
        task_y += rows + 1;  /* +1 gap between cards */
        task = task->next;
        visible_index++;
    }

    /* Counts for overflow indicators */
    int hidden_above = scroll_offset;
    int hidden_below = col->task_count - visible_index;
    if (hidden_above < 0) hidden_above = 0;
    if (hidden_below < 0) hidden_below = 0;

    /* Empty-column placeholder: a dashed card-shaped slot. When this column
     * is the active selection, it lights up so the user can see "the cursor
     * lives here" even though there are no cards. */
    if (col->tasks == NULL) {
        int slot_y = HEADER_HEIGHT + 1;
        int slot_h = 3;
        int extra  = selected_col ? A_BOLD : A_DIM;

        k_attr_on(pair, extra);
        /* Top */
        mvaddch(slot_y, inner_x, ACS_ULCORNER);
        for (int i = 1; i < inner_width - 1; i++)
            mvaddch(slot_y, inner_x + i, ACS_HLINE);
        mvaddch(slot_y, inner_x + inner_width - 1, ACS_URCORNER);
        /* Sides */
        mvaddch(slot_y + 1, inner_x, ACS_VLINE);
        mvaddch(slot_y + 1, inner_x + inner_width - 1, ACS_VLINE);
        /* Bottom */
        mvaddch(slot_y + slot_h - 1, inner_x, ACS_LLCORNER);
        for (int i = 1; i < inner_width - 1; i++)
            mvaddch(slot_y + slot_h - 1, inner_x + i, ACS_HLINE);
        mvaddch(slot_y + slot_h - 1, inner_x + inner_width - 1, ACS_LRCORNER);
        k_attr_off(pair, extra);

        /* Centered placeholder text inside the slot. Reverse-video on the
         * active column makes it unmistakable. */
        const char *hint = selected_col ? "press o to add the first card"
                                        : "no cards";
        int hl = (int)strlen(hint);
        if (hl > inner_width - 2) hl = inner_width - 2;
        int hxp = inner_x + (inner_width - hl) / 2;
        int text_extra = selected_col ? (A_BOLD | A_REVERSE) : A_DIM;
        k_attr_on(pair, text_extra);
        mvprintw(slot_y + 1, hxp, "%.*s", hl, hint);
        k_attr_off(pair, text_extra);
    }

    /* Column chrome LAST — side pipes, bottom border, and overflow badges
     * all go after the cards so nothing the cards write can clobber them. */
    if (colors_enabled) attron(COLOR_PAIR(pair) | A_DIM);
    else                attron(A_DIM);

    /* Side pipes from row 0 down through the bottom border row */
    for (int yy = 0; yy <= bottom_border_y; yy++) {
        mvaddch(yy, start_x,             ACS_VLINE);
        mvaddch(yy, start_x + width - 1, ACS_VLINE);
    }

    /* Bottom border: corners + horizontal rule across the column interior */
    mvaddch(bottom_border_y, start_x,             ACS_LLCORNER);
    mvaddch(bottom_border_y, start_x + width - 1, ACS_LRCORNER);
    for (int i = start_x + 1; i < start_x + width - 1; i++) {
        mvaddch(bottom_border_y, i, ACS_HLINE);
    }

    if (colors_enabled) attroff(COLOR_PAIR(pair) | A_DIM);
    else                attroff(A_DIM);

    /* Overflow indicators: when cards are scrolled out of view, show a
     * compact "[+N]" badge inline with the column's top or bottom rule so
     * the user knows there's more in that direction. */
    if (hidden_above > 0) {
        char badge[16];
        snprintf(badge, sizeof(badge), "[+%d]", hidden_above);
        int blen = (int)strlen(badge);
        int bx = start_x + width - 2 - blen;  /* right-aligned on header rule */
        if (bx < inner_x) bx = inner_x;
        k_attr_on(pair, A_BOLD);
        mvprintw(header_y + 1, bx, "%s", badge);
        k_attr_off(pair, A_BOLD);
    }
    if (hidden_below > 0) {
        char badge[16];
        snprintf(badge, sizeof(badge), "[+%d]", hidden_below);
        int blen = (int)strlen(badge);
        int bx = start_x + width - 2 - blen;  /* right-aligned on bottom rule */
        if (bx < inner_x) bx = inner_x;
        k_attr_on(pair, A_BOLD);
        mvprintw(bottom_border_y, bx, "%s", badge);
        k_attr_off(pair, A_BOLD);
    }
}

void render_board(Board *board, Selection *sel) {
    if (board == NULL) return;
    
    int height, width;
    getmaxyx(stdscr, height, width);
    
    /* Clear screen */
    clear();
    
    /* Calculate terminal-adaptive column widths (D-01) */
    int available_width = width;
    int num_columns = 3;
    int num_borders = num_columns - 1;
    int border_total = num_borders * BORDER_WIDTH;
    
    int min_col_width = MIN_COLUMN_WIDTH;
    int col_width = (available_width - border_total) / num_columns;
    
    /* Enforce minimum width */
    if (col_width < min_col_width) {
        col_width = min_col_width;
    }
    
    /* Calculate starting x position to center columns */
    int total_width = (col_width * num_columns) + border_total;
    int start_x = (width - total_width) / 2;
    if (start_x < 0) start_x = 0;
    
    /* Render each column (D-02 - column-local scrolling) */
    for (int i = 0; i < 3; i++) {
        int col_start = start_x + (i * (col_width + BORDER_WIDTH));
        
        /* Get selection for this column */
        int is_this_column_selected = (sel->column_index == i);
        
        render_column(&board->columns[i], i, col_start, col_width,
                      is_this_column_selected,
                      is_this_column_selected ? sel->task_index : -1,
                      scroll_offsets[i],
                      board->detailed_view);
    }
    
    /* ----- Hint strip (one row above status bar) ----- */
    int hint_y = height - 2;
    int has_card = (board->columns[sel->column_index].task_count > 0);
    const char *hint_text;
    switch (board->app_mode) {
        case MODE_CARD_POPUP:
            hint_text = " Tab next field  |  j/k item  |  Space toggle  |  a add  |  d del  |  Esc close";
            break;
        case MODE_HELP:
            hint_text = " any key to close help";
            break;
        default:
            /* Only advertise card-targeted actions when a card actually exists
             * in the current column — otherwise i/d/H/L/J/K do nothing. */
            if (has_card) {
                hint_text = " hjkl move  |  o new  |  i edit  |  d del  |  H/L shift col  |  J/K reorder  |  v details  |  ? help  |  q quit";
            } else {
                hint_text = " hjkl move  |  o new card  |  ? help  |  q quit";
            }
            break;
    }
    mvhline(hint_y, 0, ' ', width);
    if (colors_enabled) attron(COLOR_PAIR(PAIR_HINT));
    attron(A_DIM);
    char hint_buf[512];
    snprintf(hint_buf, sizeof(hint_buf), "%s", hint_text);
    if ((int)strlen(hint_buf) > width - 1) hint_buf[width - 1] = '\0';
    mvprintw(hint_y, 0, "%s", hint_buf);
    attroff(A_DIM);
    if (colors_enabled) attroff(COLOR_PAIR(PAIR_HINT));

    /* ----- Status bar (bottom row) ----- */
    int status_y = height - 1;
    if (colors_enabled) attron(COLOR_PAIR(PAIR_STATUS));
    else                attron(A_REVERSE);
    mvhline(status_y, 0, ' ', width);

    /* Mode pill (right-aligned) */
    const char *mode_str;
    int mode_pair;
    switch (board->app_mode) {
        case MODE_CARD_POPUP: mode_str = " POPUP "; mode_pair = PAIR_MODE_POPUP;  break;
        case MODE_HELP:       mode_str = " HELP ";  mode_pair = PAIR_MODE_HELP;   break;
        default:              mode_str = " NORMAL ";mode_pair = PAIR_MODE_NORMAL; break;
    }
    int mode_len = (int)strlen(mode_str);
    int mode_x = width - mode_len;
    if (mode_x < 0) mode_x = 0;

    /* Compose left side: [board] · column · task */
    Task *sel_task = NULL;
    if (sel->task_index < board->columns[sel->column_index].task_count) {
        sel_task = board->columns[sel->column_index].tasks;
        int idx = sel->task_index;
        while (sel_task != NULL && idx > 0) { sel_task = sel_task->next; idx--; }
    }

    const char *err = input_get_error();
    char status_msg[512];
    if (err != NULL) {
        snprintf(status_msg, sizeof(status_msg), " ! %s", err);
        input_clear_error();
    } else if (sel_task != NULL) {
        snprintf(status_msg, sizeof(status_msg),
                 " %s  >  %s  >  %s",
                 global_current_board_name,
                 board->columns[sel->column_index].name,
                 sel_task->title);
    } else {
        snprintf(status_msg, sizeof(status_msg),
                 " %s  >  %s  >  (empty)",
                 global_current_board_name,
                 board->columns[sel->column_index].name);
    }
    int max_left = mode_x - 1;
    if (max_left > 0 && (int)strlen(status_msg) > max_left)
        status_msg[max_left] = '\0';
    mvprintw(status_y, 0, "%s", status_msg);

    if (colors_enabled) attroff(COLOR_PAIR(PAIR_STATUS));
    else                attroff(A_REVERSE);

    /* Mode pill with its own color */
    if (colors_enabled) attron(COLOR_PAIR(mode_pair) | A_BOLD);
    else                attron(A_REVERSE | A_BOLD);
    mvprintw(status_y, mode_x, "%s", mode_str);
    if (colors_enabled) attroff(COLOR_PAIR(mode_pair) | A_BOLD);
    else                attroff(A_REVERSE | A_BOLD);

    /* Normal-mode rendering: keep the hardware cursor hidden. The popup
     * re-enables it (curs_set + DECSCUSR blink) only while editing. */
    curs_set(0);

    refresh();

    /* Restore the terminal's default cursor style (undo the blinking-block
     * DECSCUSR the popup may have set). */
    fputs("\033[0 q", stdout);
    fflush(stdout);
}

/**
 * Render a description popup overlay
 * Displays task title and description in a centered overlay window
 * Per D-01, D-02, D-03: popup for viewing/editing descriptions
 */
/**
 * Recalculate window layout based on new terminal dimensions
 * Per D-10: soft resize using resizeterm() - update LINES/COLS without full reinit
 * This function recalculates column positions but does NOT recreate windows
 * since ncurses handles this automatically after resizeterm()
 */
void renderer_calculate_layout(int lines, int cols) {
    (void)lines;
    (void)cols;
    /* 
     * The actual window positions will be recalculated in render_board()
     * which is called via renderer_redraw_all() after resizeterm()
     * 
     * This function exists as a hook for potential future window recreation
     * if needed for more complex layouts
     */
}

/**
 * Redraw all windows after resize or other events
 * Clears the virtual screen and redraws all content
 * Per D-11: must redraw all windows after resize
 */
void renderer_redraw_all(void) {
    /* Clear virtual screen */
    clear();
    
    /* Update panels and refresh */
    update_panels();
    doupdate();
}

/**
 * Render a scrollable list of available boards
 * Displays board names with current board highlighted
 * 
 * @param board_names Array of board names
 * @param count Number of boards
 * @param selected Currently selected board index
 */
void render_board_list(char **board_names, int count, int selected) {
    if (board_names == NULL || count <= 0) return;
    
    int height, width;
    getmaxyx(stdscr, height, width);
    
    /* Clear screen for board list */
    clear();
    
    /* Calculate dimensions. Width must be at least wide enough for the
     * footer hint (~46 cols including borders/padding); previously it was
     * 40 and the hint overran the right border. */
    int menu_height = height - 4;
    int menu_width = 50;
    if (menu_width > width - 4) menu_width = width - 4;
    
    /* Center position */
    int start_y = (height - menu_height) / 2;
    int start_x = (width - menu_width) / 2;
    if (start_y < 1) start_y = 1;
    if (start_x < 1) start_x = 1;
    
    /* Draw border */
    mvaddch(start_y, start_x, ACS_ULCORNER);
    mvaddch(start_y, start_x + menu_width - 1, ACS_URCORNER);
    mvaddch(start_y + menu_height - 1, start_x, ACS_LLCORNER);
    mvaddch(start_y + menu_height - 1, start_x + menu_width - 1, ACS_LRCORNER);
    
    for (int x = start_x + 1; x < start_x + menu_width - 1; x++) {
        mvaddch(start_y, x, ACS_HLINE);
        mvaddch(start_y + menu_height - 1, x, ACS_HLINE);
    }
    for (int y = start_y + 1; y < start_y + menu_height - 1; y++) {
        mvaddch(y, start_x, ACS_VLINE);
        mvaddch(y, start_x + menu_width - 1, ACS_VLINE);
    }
    
    /* Title */
    mvwprintw(stdscr, start_y + 1, start_x + 2, " Board List ");
    
    /* Get current board name */
    extern char global_current_board_name[];
    
    /* Draw board names */
    int list_start_y = start_y + 3;
    int max_display = menu_height - 5;
    
    for (int i = 0; i < count && i < max_display; i++) {
        int y = list_start_y + i;
        
        /* Highlight current board */
        int is_current = 0;
        if (board_names[i] != NULL && 
            strcmp(board_names[i], global_current_board_name) == 0) {
            is_current = 1;
        }
        
        /* Highlight selected */
        if (i == selected) {
            attron(A_REVERSE);
        }
        
        /* Mark current board with * */
        if (is_current) {
            mvwprintw(stdscr, y, start_x + 2, "* %s", board_names[i]);
        } else {
            mvwprintw(stdscr, y, start_x + 2, "  %s", board_names[i]);
        }
        
        if (i == selected) {
            attroff(A_REVERSE);
        }
    }
    
    /* Help text — truncated to fit inside the box. */
    int hint_max = menu_width - 4;
    char hint[64];
    snprintf(hint, sizeof(hint), "j/k:nav | Enter:select | Esc:cancel");
    if ((int)strlen(hint) > hint_max && hint_max > 0) hint[hint_max] = '\0';
    mvwprintw(stdscr, start_y + menu_height - 2, start_x + 2, "%s", hint);

    refresh();
}

/**
 * Show board list menu and wait for user selection
 * Returns selected board name (caller must free), or NULL on cancel
 */
char* show_board_list_menu(void) {
    /* Get available boards */
    char **boards = NULL;
    int count = 0;
    
    if (board_list_boards(&boards, &count) != 0 || count == 0) {
        return NULL;
    }
    
    /* Get current board name */
    extern char global_current_board_name[];
    
    /* Find current board index */
    int selected = 0;
    for (int i = 0; i < count; i++) {
        if (boards[i] != NULL && 
            strcmp(boards[i], global_current_board_name) == 0) {
            selected = i;
            break;
        }
    }
    
    /* Render initial list */
    render_board_list(boards, count, selected);
    
    /* Input loop */
    int done = 0;
    char *result = NULL;
    
    while (!done) {
        int key = getch();
        
        switch (key) {
            case KEY_DOWN:
            case 'j':
                if (selected < count - 1) {
                    selected++;
                    render_board_list(boards, count, selected);
                }
                break;
                
            case KEY_UP:
            case 'k':
                if (selected > 0) {
                    selected--;
                    render_board_list(boards, count, selected);
                }
                break;
                
            case '\n':
            case KEY_ENTER:
                /* Select current board */
                if (boards[selected] != NULL) {
                    result = strdup(boards[selected]);
                }
                done = 1;
                break;
                
            case 27:  /* Escape */
                done = 1;
                break;
                
            default:
                break;
        }
    }
    
    /* Cleanup */
    board_list_free(boards, count);

    return result;
}

/**
 * Render help popup with all keybindings.
 *
 * Two-column layout: the full keymap doesn't fit in a 30-row terminal as a
 * single column, so we split sections across two side-by-side columns. The
 * popup width is sized to fit both columns + padding.
 *
 * Press any key to close.
 */
void render_help_popup(void) {
    int height, width;
    getmaxyx(stdscr, height, width);

    /* type 0 = section header (bold), 1 = key row, 2 = blank */
    typedef struct { int type; const char *text; } HelpLine;

    /* LEFT column: things you do on the board */
    static const HelpLine left[] = {
        {0, "NAVIGATION"},
        {1, "  h j k l         move L/D/U/R"},
        {1, "  arrows          same as hjkl"},
        {1, "  Tab / S-Tab     next/prev column"},
        {1, "  gg              jump to top"},
        {1, "  G               jump to bottom"},
        {2, ""},
        {0, "CARDS"},
        {1, "  o / O           new card below/above"},
        {1, "  d  or  x        delete card"},
        {1, "  H / L           shift to other col"},
        {1, "  J / K           reorder card down/up"},
        {2, ""},
        {0, "VIEW"},
        {1, "  v               toggle detailed view"},
        {1, "  i  or  Enter    open card popup"},
        {1, "  c               focus checklist field"},
    };
    int left_n = (int)(sizeof(left) / sizeof(left[0]));

    /* RIGHT column: popup edit + boards + global */
    static const HelpLine right[] = {
        {0, "INSIDE CARD POPUP"},
        {1, "  Tab / S-Tab     cycle field"},
        {1, "  Esc             close (drops empty)"},
        {1, "  type to edit title/description"},
        {2, ""},
        {0, "CHECKLIST FIELD"},
        {1, "  j / k           navigate items"},
        {1, "  Space           toggle"},
        {1, "  o / O           add below/above+edit"},
        {1, "  J / K           pull item down/up"},
        {1, "  d               delete item"},
        {1, "  Enter           commit edit"},
        {2, ""},
        {0, "BOARDS"},
        {1, "  :bn / :bp       next / prev board"},
        {1, "  :bnew           create board"},
        {1, "  :brename <n>    rename current"},
        {1, "  :blist          board picker"},
        {1, "  :b <name>       switch by name"},
        {2, ""},
        {0, "GLOBAL"},
        {1, "  ?               toggle this help"},
        {1, "  q               quit"},
    };
    int right_n = (int)(sizeof(right) / sizeof(right[0]));

    int rows = (left_n > right_n ? left_n : right_n);

    /* Geometry: each column ~40 chars wide; 2 cols + padding + borders */
    int col_w = 40;
    int popup_width = (col_w * 2) + 5;  /* 2 borders + 1 col gap + 2 padding */
    if (popup_width > width - 4) popup_width = width - 4;

    int popup_height = rows + 3;  /* +2 border, +1 title row */
    if (popup_height > height - 2) popup_height = height - 2;

    int popup_y = (height - popup_height) / 2;
    int popup_x = (width - popup_width) / 2;
    if (popup_y < 1) popup_y = 1;
    if (popup_x < 1) popup_x = 1;

    /* Clear popup area */
    for (int y = popup_y; y < popup_y + popup_height; y++) {
        mvhline(y, popup_x, ' ', popup_width);
    }

    /* Border */
    mvaddch(popup_y, popup_x, ACS_ULCORNER);
    mvaddch(popup_y, popup_x + popup_width - 1, ACS_URCORNER);
    mvaddch(popup_y + popup_height - 1, popup_x, ACS_LLCORNER);
    mvaddch(popup_y + popup_height - 1, popup_x + popup_width - 1, ACS_LRCORNER);
    for (int x = popup_x + 1; x < popup_x + popup_width - 1; x++) {
        mvaddch(popup_y, x, ACS_HLINE);
        mvaddch(popup_y + popup_height - 1, x, ACS_HLINE);
    }
    for (int y = popup_y + 1; y < popup_y + popup_height - 1; y++) {
        mvaddch(y, popup_x, ACS_VLINE);
        mvaddch(y, popup_x + popup_width - 1, ACS_VLINE);
    }

    /* Title */
    const char *title = " Keybindings ";
    int title_x = popup_x + (popup_width - (int)strlen(title)) / 2;
    attron(A_BOLD);
    mvprintw(popup_y, title_x, "%s", title);
    attroff(A_BOLD);

    /* Render a single row of one column. */
    int content_top = popup_y + 1;
    int content_max = popup_y + popup_height - 1;
    int left_x  = popup_x + 2;
    int right_x = popup_x + 2 + col_w + 1;

    /* Helper macro to draw one line of one column at the given screen y.
     * Defined inline to keep render_help_popup self-contained. */
    #define DRAW_HELP_LINE(arr, idx, n, col_x) do { \
        if ((idx) < (n) && y < content_max) {                              \
            const HelpLine *L = &(arr)[idx];                               \
            if (L->type == 0) {                                            \
                if (colors_enabled) attron(COLOR_PAIR(PAIR_HINT));         \
                attron(A_BOLD);                                            \
                mvprintw(y, col_x, "%.*s", col_w, L->text);                \
                attroff(A_BOLD);                                           \
                if (colors_enabled) attroff(COLOR_PAIR(PAIR_HINT));        \
            } else if (L->type == 1) {                                     \
                mvprintw(y, col_x, "%.*s", col_w, L->text);                \
            }                                                              \
        }                                                                  \
    } while (0)

    int y = content_top;
    for (int i = 0; i < rows; i++, y++) {
        if (y >= content_max) break;
        DRAW_HELP_LINE(left,  i, left_n,  left_x);
        DRAW_HELP_LINE(right, i, right_n, right_x);
    }

    #undef DRAW_HELP_LINE

    refresh();
}

/**
 * Render card popup overlay for in-place editing
 * Displays title, description, and checklist in a centered overlay.
 * active_field: 0=title, 1=description, 2=checklist
 */
void render_card_popup(Board *board, Selection *selection, int active_field) {
    if (board == NULL || selection == NULL) return;

    /* Do NOT render the board behind the popup. The card being edited lives
     * on that board, and since we mutate task->title / task->description in
     * place per keystroke, render_board would echo the live edits onto the
     * underlying card — which sticks out from under the popup, producing the
     * "text appears in two places" artifact. Just paint a clean background. */
    erase();

    int height, width;
    getmaxyx(stdscr, height, width);

    /* Get the selected task */
    Column *col = &board->columns[selection->column_index];
    Task *task = col->tasks;
    int idx = selection->task_index;
    while (task != NULL && idx > 0) {
        task = task->next;
        idx--;
    }
    if (task == NULL) {
        /* Selection points past the end of this column — fall back to the
         * normal board view rather than showing a blank screen. */
        board->app_mode = MODE_NORMAL;
        render_board(board, selection);
        return;
    }

    /* Calculate popup dimensions */
    int popup_width = (int)(width * 0.7);
    if (popup_width < 44)        popup_width = 44;
    if (popup_width > width - 4) popup_width = width - 4;

    int cl_count = checklist_count(task);
    int cl_rows  = (cl_count > 0) ? cl_count : 1;

    /*
     * Height breakdown:
     *   1  top border
     *   1  title row
     *   1  blank gap
     *   1  desc header
     *   3  desc content rows
     *   1  divider
     *   1  checklist header
     *   cl_rows  checklist items (min 1 for "(no items)")
     *   2  checklist nav hint area
     *   1  key hint footer
     *   1  bottom border
     * = 13 + cl_rows
     */
    int popup_height = 13 + cl_rows;
    if (popup_height > height - 4) popup_height = height - 4;

    int popup_y = (height - popup_height) / 2;
    int popup_x = (width  - popup_width)  / 2;
    if (popup_y < 1) popup_y = 1;
    if (popup_x < 1) popup_x = 1;

    /* Clear popup background area */
    for (int r = popup_y; r < popup_y + popup_height; r++) {
        mvhline(r, popup_x, ' ', popup_width);
    }

    /* Draw border */
    mvaddch(popup_y, popup_x, ACS_ULCORNER);
    mvaddch(popup_y, popup_x + popup_width - 1, ACS_URCORNER);
    mvaddch(popup_y + popup_height - 1, popup_x, ACS_LLCORNER);
    mvaddch(popup_y + popup_height - 1, popup_x + popup_width - 1, ACS_LRCORNER);
    for (int x = popup_x + 1; x < popup_x + popup_width - 1; x++) {
        mvaddch(popup_y, x, ACS_HLINE);
        mvaddch(popup_y + popup_height - 1, x, ACS_HLINE);
    }
    for (int y = popup_y + 1; y < popup_y + popup_height - 1; y++) {
        mvaddch(y, popup_x, ACS_VLINE);
        mvaddch(y, popup_x + popup_width - 1, ACS_VLINE);
    }

    /* Row 1: Title. Like the description and checklist sections below, the
     * only highlight is on the section LABEL ("Card:") when the field is
     * active. The value itself stays plain — the blinking cursor parked at
     * the end of the text is the "you are here" indicator. */
    int title_row = popup_y + 1;
    if (active_field == 0) attron(A_REVERSE);
    mvprintw(title_row, popup_x + 1, " Card:");
    if (active_field == 0) attroff(A_REVERSE);
    int title_val_x = popup_x + 8;
    int title_val_w = popup_width - 10;
    if (title_val_w < 1) title_val_w = 1;
    char title_display[256];
    strncpy(title_display, task->title, sizeof(title_display) - 1);
    title_display[sizeof(title_display) - 1] = '\0';
    if ((int)strlen(title_display) > title_val_w)
        title_display[title_val_w] = '\0';
    if (title_display[0] == '\0') {
        /* Empty title: show a dim "(empty)" hint, but ONLY when this field
         * isn't active. When it's active, the blinking cursor parked here
         * is the indicator — a placeholder would just be noise. */
        if (active_field != 0) {
            attron(A_DIM);
            mvprintw(title_row, title_val_x, "(empty)");
            attroff(A_DIM);
        }
    } else {
        mvprintw(title_row, title_val_x, "%s", title_display);
    }

    /* Row 3: Description header */
    int desc_hdr_row = popup_y + 3;
    if (active_field == 1) attron(A_REVERSE);
    mvprintw(desc_hdr_row, popup_x + 1, " Description:");
    if (active_field == 1) attroff(A_REVERSE);

    /* Rows 4-6: Description content (up to 3 lines, wrapped) */
    int desc_content_start = popup_y + 4;
    int inner_w = popup_width - 6;  /* indent 3 each side */
    if (inner_w < 1) inner_w = 1;

    if (task->description[0] != '\0') {
        const char *src = task->description;
        int src_len = (int)strlen(src);
        int line = 0;
        int pos = 0;
        while (pos < src_len && line < 3) {
            int chunk = inner_w;
            if (pos + chunk > src_len) chunk = src_len - pos;
            /* break at newline */
            for (int ci = 0; ci < chunk; ci++) {
                if (src[pos + ci] == '\n') { chunk = ci; break; }
            }
            char linebuf[256];
            if (chunk >= (int)sizeof(linebuf)) chunk = (int)sizeof(linebuf) - 1;
            strncpy(linebuf, src + pos, chunk);
            linebuf[chunk] = '\0';
            mvprintw(desc_content_start + line, popup_x + 3, "%s", linebuf);
            pos += chunk;
            if (pos < src_len && src[pos] == '\n') pos++;  /* skip newline */
            line++;
        }
    } else if (active_field != 1) {
        /* Empty description placeholder is hidden while editing — the
         * cursor parked at the start of the desc area is the indicator. */
        attron(A_DIM);
        mvprintw(desc_content_start, popup_x + 3, "(empty)");
        attroff(A_DIM);
    }

    /* Row 7: Divider */
    int divider_row = popup_y + 7;
    for (int x = popup_x + 1; x < popup_x + popup_width - 1; x++) {
        mvaddch(divider_row, x, ACS_HLINE);
    }

    /* Row 8: Checklist header */
    int cl_hdr_row = popup_y + 8;
    if (active_field == 2) attron(A_REVERSE);
    mvprintw(cl_hdr_row, popup_x + 1, " Checklist:");
    if (active_field == 2) attroff(A_REVERSE);

    /* Rows 9+: Checklist items */
    int cl_start_row = popup_y + 9;
    int cl_max_rows  = popup_height - (cl_start_row - popup_y) - 3; /* leave hint + border */
    if (cl_max_rows < 1) cl_max_rows = 1;

    if (task->checklist == NULL) {
        if (active_field != 2) {
            attron(A_DIM);
            mvprintw(cl_start_row, popup_x + 3, "(no items)");
            attroff(A_DIM);
        }
    } else {
        ChecklistItem *item = task->checklist;
        int ci = 0;
        while (item != NULL && ci < cl_max_rows) {
            int row = cl_start_row + ci;
            char item_buf[280];
            snprintf(item_buf, sizeof(item_buf), "%s %s",
                     item->checked ? "[x]" : "[ ]", item->text);
            /* Truncate to fit */
            int max_item_w = popup_width - 6;
            if (max_item_w > 0 && (int)strlen(item_buf) > max_item_w)
                item_buf[max_item_w] = '\0';
            /* Like the title and description fields, the only highlight is
             * on the "Checklist:" label above. The cursor parked on the
             * current row is the "you are here" indicator. */
            mvprintw(row, popup_x + 3, "%s", item_buf);
            item = item->next;
            ci++;
        }
    }

    /* Second-to-last row: key hints */
    int hint_row = popup_y + popup_height - 2;
    char hint_buf[128];
    snprintf(hint_buf, sizeof(hint_buf),
             " Tab:next | j/k:nav | Sp:toggle | o/O:add | J/K:move | d:del | Esc:close");
    int hint_max = popup_width - 2;
    if (hint_max > 0 && (int)strlen(hint_buf) > hint_max)
        hint_buf[hint_max] = '\0';
    mvprintw(hint_row, popup_x + 1, "%s", hint_buf);

    /* Park the hardware cursor on the active field and make it visible.
     * This is the user-facing "you are here" indicator — much louder than
     * reverse-video alone, and crucial when the field is empty. */
    int cursor_y = title_row;
    int cursor_x = title_val_x;
    if (active_field == 0) {
        cursor_y = title_row;
        cursor_x = title_val_x + (int)strlen(task->title);
        if (cursor_x > popup_x + popup_width - 2)
            cursor_x = popup_x + popup_width - 2;
    } else if (active_field == 1) {
        /* Cursor at the end of the description text (last rendered line). */
        if (task->description[0] == '\0') {
            cursor_y = desc_content_start;
            cursor_x = popup_x + 3;
        } else {
            int dlen = (int)strlen(task->description);
            int line = 0;
            int pos = 0;
            int col_in_line = 0;
            while (pos < dlen && line < 3) {
                int chunk = inner_w;
                if (pos + chunk > dlen) chunk = dlen - pos;
                for (int ci = 0; ci < chunk; ci++) {
                    if (task->description[pos + ci] == '\n') { chunk = ci; break; }
                }
                col_in_line = chunk;
                pos += chunk;
                if (pos < dlen && task->description[pos] == '\n') { pos++; line++; col_in_line = 0; }
                else if (pos < dlen) line++;
            }
            cursor_y = desc_content_start + line;
            cursor_x = popup_x + 3 + col_in_line;
            if (cursor_y >= desc_content_start + 3) cursor_y = desc_content_start + 2;
        }
    } else if (active_field == 2) {
        if (task->checklist == NULL) {
            /* No items: park cursor where the first item would land,
             * inside the [ ] position. */
            cursor_y = popup_y + 9;
            cursor_x = popup_x + 3 + 1;  /* "  [ ]" — inside the brackets */
        } else if (card_popup_checklist_editing()) {
            /* Editing: park cursor at the end of the item's text. */
            cursor_y = popup_y + 9 + board->checklist_index;
            ChecklistItem *cit = task->checklist;
            int ci2 = board->checklist_index;
            while (cit != NULL && ci2 > 0) { cit = cit->next; ci2--; }
            cursor_x = popup_x + 3 + 4 + (cit ? (int)strlen(cit->text) : 0);
            if (cursor_x > popup_x + popup_width - 2)
                cursor_x = popup_x + popup_width - 2;
        } else {
            /* Navigating: cursor sits INSIDE the [ ] of the current item.
             * Each item is rendered as "[ ] text" starting at popup_x + 3,
             * so the space inside the brackets is at popup_x + 3 + 1. */
            cursor_y = popup_y + 9 + board->checklist_index;
            cursor_x = popup_x + 3 + 1;
        }
    }
    move(cursor_y, cursor_x);
    curs_set(1);

    refresh();

    /* Force a blinking block cursor via DECSCUSR. ncurses' curs_set() only
     * picks "invisible / normal / very visible" — actual blink behavior is a
     * separate terminal mode that most modern terminals leave OFF by default.
     * We write the escape after refresh() so it doesn't get clobbered by the
     * ncurses output. ESC[1 q = blinking block. */
    fputs("\033[1 q", stdout);
    fflush(stdout);
}