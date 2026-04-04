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

/* Global selection state */
static Selection global_selection = {0, 0};

/* Scroll offsets for each column (for column-local scrolling) */
static int scroll_offsets[3] = {0, 0, 0};

/* Minimum column width */
#define MIN_COLUMN_WIDTH 20

/* Border width between columns */
#define BORDER_WIDTH 1

/* Header height (for column title) */
#define HEADER_HEIGHT 2

/* Status bar height */
#define STATUS_BAR_HEIGHT 1

void renderer_init(void) {
    /* Initialize global selection */
    global_selection.column_index = 0;
    global_selection.task_index = 0;
    
    /* Initialize scroll offsets */
    scroll_offsets[0] = 0;
    scroll_offsets[1] = 0;
    scroll_offsets[2] = 0;
}

Selection* get_selection(void) {
    return &global_selection;
}

void set_selection(int column_index, int task_index) {
    if (column_index >= 0 && column_index < 3) {
        global_selection.column_index = column_index;
    }
    if (task_index >= 0) {
        global_selection.task_index = task_index;
    }
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

void render_task(Task *task, int y, int x, int width, int selected) {
    if (task == NULL) return;
    
    /* Calculate available width for title */
    int max_title_width = width - 8;  /* "- [ ] " prefix = 6 + safety margin */
    if (max_title_width < 1) max_title_width = 1;
    
    /* Create display string with checkbox */
    char display[512];
    char checkbox[8];
    
    if (task->completed) {
        snprintf(checkbox, sizeof(checkbox), "- [x] ");
    } else {
        snprintf(checkbox, sizeof(checkbox), "- [ ] ");
    }
    
    /* Copy title and truncate if needed */
    size_t title_len = strlen(task->title);
    if (title_len > (size_t)max_title_width) {
        /* Truncate title with ellipsis */
        snprintf(display, sizeof(display), "%.*s...", 
                 max_title_width - 3, task->title);
    } else {
        snprintf(display, sizeof(display), "%s", task->title);
    }
    
    /* Apply highlight if selected */
    if (selected) {
        attron(A_REVERSE);
    }
    
    /* Move to position and print */
    mvprintw(y, x, "%s%s", checkbox, display);
    
    /* Turn off highlight */
    if (selected) {
        attroff(A_REVERSE);
    }
}

void render_column(Column *col, int start_x, int width, 
                   int selected_col, int sel_task_index, int scroll_offset) {
    if (col == NULL) return;
    
    int height, max_y;
    getmaxyx(stdscr, height, max_y);
    max_y = max_y - STATUS_BAR_HEIGHT - 1;  /* Reserve space for status bar */
    
    int border_char = ACS_VLINE;
    if (border_char == 0) border_char = '|';  /* Fallback */
    
    /* Draw column border (vertical line on right side) */
    for (int y = 0; y < max_y; y++) {
        mvaddch(y, start_x + width - 1, border_char);
    }
    
    /* Draw column header */
    int header_y = 1;
    mvprintw(header_y, start_x + 1, "## %s", col->name);
    
    /* Draw tasks starting below header */
    int task_y = header_y + HEADER_HEIGHT;
    Task *task = col->tasks;
    int visible_index = 0;
    int rendered_count = 0;
    
    /* Skip tasks above scroll offset */
    while (task != NULL && visible_index < scroll_offset) {
        task = task->next;
        visible_index++;
    }
    
    /* Render visible tasks */
    while (task != NULL && task_y < max_y - 1) {
        /* Check if this task is selected */
        int is_selected = (selected_col == 1 && sel_task_index == visible_index);
        
        /* Render task */
        render_task(task, task_y, start_x + 2, width - 4, is_selected);
        
        task = task->next;
        task_y++;
        visible_index++;
        rendered_count++;
    }
    
    /* If no tasks, show placeholder */
    if (col->tasks == NULL) {
        mvprintw(task_y, start_x + 2, "(empty)");
    }
}

void render_board(Board *board) {
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
        int is_this_column_selected = (global_selection.column_index == i);
        
        render_column(&board->columns[i], col_start, col_width,
                      is_this_column_selected, 
                      is_this_column_selected ? global_selection.task_index : -1,
                      scroll_offsets[i]);
    }
    
    /* Draw status bar at bottom */
    int status_y = height - 1;
    mvhline(status_y, 0, 0, width);
    
    /* Show help text and selection info */
    char status_msg[256];
    Task *sel_task = NULL;
    if (global_selection.task_index < board->columns[global_selection.column_index].task_count) {
        sel_task = board->columns[global_selection.column_index].tasks;
        int idx = global_selection.task_index;
        while (sel_task != NULL && idx > 0) {
            sel_task = sel_task->next;
            idx--;
        }
    }
    
    if (sel_task != NULL) {
        snprintf(status_msg, sizeof(status_msg), 
                 "[%s] Selected: %s | 'o':new-below 'O':new-above 'd':delete | 'q':quit",
                 board->columns[global_selection.column_index].name,
                 sel_task->title);
    } else {
        snprintf(status_msg, sizeof(status_msg),
                 "[%s] No selection | 'o':new-below 'O':new-above 'd':delete | 'q':quit",
                 board->columns[global_selection.column_index].name);
    }
    
    /* Truncate status message if too long */
    if ((int)strlen(status_msg) > width - 1) {
        status_msg[width - 1] = '\0';
    }
    
    mvprintw(status_y, 0, "%s", status_msg);
    
    /* Show mode indicator per MOD-05 (Normal mode shows visual indicators) */
    if (board->app_mode == MODE_INSERT) {
        /* vim-style INSERT indicator at right side of status bar */
        int insert_x = width - 12;  /* "-- INSERT --" is 12 chars */
        if (insert_x > 0) {
            mvprintw(status_y, insert_x, "-- INSERT --");
        }
    }
    /* In Normal mode, show "NORMAL" indicator (optional - can be empty) */
    else {
        int normal_x = width - 8;
        if (normal_x > 0) {
            mvprintw(status_y, normal_x, "NORMAL");
        }
    }
    
    /* Refresh to show all changes (per PITFALLS.md) */
    refresh();
}

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