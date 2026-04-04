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

void render_task(Task *task, int y, int x, int width, int selected, int detailed_view) {
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
    
    /* In detailed view, show description and checklist below task */
    if (detailed_view) {
        int desc_y = y + 1;
        
        /* Show description if present */
        if (task->description[0] != '\0') {
            mvprintw(desc_y, x + 2, "  %s", task->description);
            desc_y++;
        }
        
        /* Show checklist items */
        ChecklistItem *item = task->checklist;
        while (item != NULL) {
            char check_display[264];
            if (item->checked) {
                snprintf(check_display, sizeof(check_display), "  ☑ %s", item->text);
            } else {
                snprintf(check_display, sizeof(check_display), "  ☐ %s", item->text);
            }
            mvprintw(desc_y, x, "%s", check_display);
            desc_y++;
            item = item->next;
        }
    }
}

void render_column(Column *col, int start_x, int width, 
                   int selected_col, int sel_task_index, int scroll_offset,
                   int detailed_view) {
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
        render_task(task, task_y, start_x + 2, width - 4, is_selected, detailed_view);
        
        /* In detailed view, count lines for description + checklist */
        if (detailed_view) {
            task_y++;
            /* Add line for description if present */
            if (task->description[0] != '\0') {
                task_y++;
            }
            /* Add lines for each checklist item */
            ChecklistItem *item = task->checklist;
            while (item != NULL) {
                task_y++;
                item = item->next;
            }
        }
        
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
                      scroll_offsets[i],
                      board->detailed_view);
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
    
    /* Show view mode indicator (Compact or Detailed) */
    int view_mode_x = width - 18;
    if (view_mode_x > 0) {
        if (board->detailed_view) {
            mvprintw(status_y, view_mode_x, "[Detailed View]");
        } else {
            mvprintw(status_y, view_mode_x, "[Compact View]");
        }
    }
    
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
 * Render a description popup overlay
 * Displays task title and description in a centered overlay window
 * Per D-01, D-02, D-03: popup for viewing/editing descriptions
 */
void render_description_popup(Board *board, Selection *selection) {
    if (board == NULL || selection == NULL) return;
    
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
    
    if (task == NULL) return;
    
    /* Calculate popup dimensions - 60% width */
    int popup_width = (int)(width * 0.6);
    if (popup_width < 40) popup_width = 40;
    if (popup_width > width - 4) popup_width = width - 4;
    
    /* Calculate popup height based on content */
    int desc_lines = 1;
    if (task->description[0] != '\0') {
        desc_lines = (int)(strlen(task->description) / (popup_width - 10)) + 2;
    }
    int popup_height = 6 + desc_lines;  /* Title + Description lines + border */
    if (popup_height > height - 4) popup_height = height - 4;
    
    /* Calculate centered position */
    int popup_y = (height - popup_height) / 2;
    int popup_x = (width - popup_width) / 2;
    if (popup_y < 1) popup_y = 1;
    if (popup_x < 1) popup_x = 1;
    
    /* Draw popup border */
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
    
    /* Draw title bar */
    int title_y = popup_y + 1;
    char title_bar[256];
    snprintf(title_bar, sizeof(title_bar), " Description: %s ", task->title);
    mvprintw(title_y, popup_x + 2, "%s", title_bar);
    
    /* Draw description content */
    int desc_y = popup_y + 3;
    if (task->description[0] != '\0') {
        /* Word-wrap description */
        char *desc_copy = strdup(task->description);
        if (desc_copy) {
            char *word = strtok(desc_copy, "\n");
            int y = desc_y;
            int x = popup_x + 2;
            int max_x = popup_x + popup_width - 3;
            
            while (word && y < popup_y + popup_height - 2) {
                int word_len = strlen(word);
                if (x + word_len > max_x) {
                    y++;
                    x = popup_x + 2;
                }
                if (y >= popup_y + popup_height - 2) break;
                mvprintw(y, x, "%s ", word);
                x += word_len + 1;
                word = strtok(NULL, "\n");
            }
            free(desc_copy);
        }
    } else {
        mvprintw(desc_y, popup_x + 2, "(No description - press Enter to edit)");
    }
    
    /* Draw help text at bottom */
    int help_y = popup_y + popup_height - 2;
    char *mode_text = (board->app_mode == MODE_DESCRIPTION_EDIT) ? 
        "EDITING: Type to edit | Enter: save | Esc: cancel" :
        "VIEWING: Enter: edit | Esc: close";
    mvprintw(help_y, popup_x + 2, "%s", mode_text);
    
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
    
    /* Calculate dimensions */
    int menu_height = height - 4;
    int menu_width = 40;
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
    
    /* Help text */
    mvwprintw(stdscr, start_y + menu_height - 2, start_x + 2, 
              "j/k: navigate | Enter: select | Esc: cancel");
    
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