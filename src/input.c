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

int read_task_title(char *buffer, size_t size) {
    if (buffer == NULL || size == 0) return -1;
    
    int height, width;
    getmaxyx(stdscr, height, width);
    
    /* Clear input buffer */
    buffer[0] = '\0';
    
    /* Create input prompt */
    int prompt_y = height - 2;
    int prompt_x = 0;
    
    /* Show prompt */
    mvprintw(prompt_y, prompt_x, "Task title: ");
    clrtoeol();
    refresh();
    
    /* Move cursor to input position */
    move(prompt_y, prompt_x + 12);
    
    /* Read input with bounds checking (per PITFALLS.md) */
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
 * Read task description from user input
 * Uses ncurses input field with prompt
 * 
 * @param buffer Buffer to store the description
 * @param size Size of the buffer
 * @return 0 on success, -1 on cancel
 */
int read_task_description(char *buffer, size_t size) {
    if (buffer == NULL || size == 0) return -1;
    
    int height, width;
    getmaxyx(stdscr, height, width);
    
    /* Clear input buffer */
    buffer[0] = '\0';
    
    /* Create input prompt */
    int prompt_y = height - 2;
    int prompt_x = 0;
    
    /* Show prompt */
    mvprintw(prompt_y, prompt_x, "Description: ");
    clrtoeol();
    refresh();
    
    /* Move cursor to input position */
    move(prompt_y, prompt_x + 13);
    
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
 * Enter Insert mode - allows text editing
 * MOD-03: User can enter Insert mode to edit task title
 */
void enter_insert_mode(Board *board) {
    if (board == NULL) return;
    board->app_mode = MODE_INSERT;
}

/**
 * Exit Insert mode - returns to Normal mode for navigation
 * MOD-04: User can exit Insert mode back to Normal mode (Esc)
 */
void exit_insert_mode(Board *board) {
    if (board == NULL) return;
    board->app_mode = MODE_NORMAL;
}

/**
 * Enter Checklist mode - allows navigating checklist items
 * Activated when user presses 'c' in detailed view
 */
void enter_checklist_mode(Board *board) {
    if (board == NULL) return;
    board->app_mode = MODE_CHECKLIST;
    board->checklist_index = 0;
}

/**
 * Exit Checklist mode - returns to Normal mode
 */
void exit_checklist_mode(Board *board) {
    if (board == NULL) return;
    board->app_mode = MODE_NORMAL;
    board->checklist_index = 0;
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
 * Handle terminal resize event using soft resize
 * Per D-10, D-11: uses resizeterm() not full reinitialization
 */
void handle_resize(Board *board) {
    if (board == NULL) return;
    
    /* Get new terminal dimensions */
    int new_lines = LINES;
    int new_cols = COLS;
    
    /* Use soft resize - resizeterm updates without full reinit */
    resizeterm(new_lines, new_cols);
    
    /* Recalculate all window positions */
    renderer_calculate_layout(new_lines, new_cols);
    
    /* Redraw everything */
    renderer_redraw_all();
}

/**
 * Clamp the scroll offset for the current column so that sel->task_index
 * remains visible on screen.
 *
 * visible_height = height - 5 because render_column starts tasks at task_y=3
 * and stops before max_y-1 (= height-2), giving (height-2) - 3 = height-5
 * visible rows.  Keeping this comment here avoids re-deriving the geometry.
 */
static void clamp_scroll(Selection *sel) {
    int height, width;
    getmaxyx(stdscr, height, width);
    (void)width;  /* unused but getmaxyx requires both */
    int visible_height = height - 5; /* matches render_column: task_y=3, max_y=height-2, stop < max_y-1 */
    if (visible_height < 1) visible_height = 1;
    int col_i = sel->column_index;
    int offset = get_scroll_offset(col_i);
    if (sel->task_index < offset) {
        set_scroll_offset(col_i, sel->task_index);
    } else if (sel->task_index >= offset + visible_height) {
        set_scroll_offset(col_i, sel->task_index - visible_height + 1);
    }
}

int handle_input(Board *board, int key, Selection *selection) {
    if (board == NULL || selection == NULL) return 0;
    
    /* Check current mode and route accordingly */
    if (board->app_mode == MODE_INSERT) {
        /* Insert mode: Escape returns to Normal mode */
        if (key == 27) {  /* Escape */
            exit_insert_mode(board);
            return 0;
        }
        /* In Insert mode, Enter returns to Normal mode */
        if (key == '\n' || key == KEY_ENTER) {
            exit_insert_mode(board);
            return 0;
        }
        /* Arrow keys could move cursor in text editing mode */
        if (key == KEY_UP || key == KEY_DOWN || key == KEY_LEFT || key == KEY_RIGHT) {
            return 0;
        }
        /* Pass through other keys for potential text input */
        return 0;
    }
    
    /* MODE_DESCRIPTION_VIEW: show description popup */
    if (board->app_mode == MODE_DESCRIPTION_VIEW) {
        /* Esc closes popup */
        if (key == 27) {  /* Escape */
            board->app_mode = MODE_NORMAL;
            return 0;
        }
        /* Enter enters edit mode */
        if (key == '\n' || key == KEY_ENTER) {
            board->app_mode = MODE_DESCRIPTION_EDIT;
            return 0;
        }
        /* 'i' also enters edit mode */
        if (key == 'i') {
            board->app_mode = MODE_DESCRIPTION_EDIT;
            return 0;
        }
        return 0;
    }
    
    /* MODE_DESCRIPTION_EDIT: prompt for description immediately */
    if (board->app_mode == MODE_DESCRIPTION_EDIT) {
        Column *col = &board->columns[selection->column_index];
        Task *task = col->tasks;
        int idx = selection->task_index;
        while (task != NULL && idx > 0) {
            task = task->next;
            idx--;
        }

        if (task != NULL) {
            char desc[MAX_DESC_LEN];
            strncpy(desc, task->description, sizeof(desc) - 1);
            desc[sizeof(desc) - 1] = '\0';

            if (read_task_description(desc, sizeof(desc)) == 0) {
                strncpy(task->description, desc, sizeof(task->description) - 1);
                task->description[sizeof(task->description) - 1] = '\0';
                task->desc_len = strlen(task->description);
                if (board->filename[0] != '\0') {
                    board_save(board, board->filename);
                }
            }
        }

        board->app_mode = MODE_NORMAL;
        return 0;
    }
    
    /* MODE_CHECKLIST: navigate and manage checklist items */
    if (board->app_mode == MODE_CHECKLIST) {
        /* Get the selected task */
        Column *col = &board->columns[selection->column_index];
        Task *task = col->tasks;
        int idx = selection->task_index;
        while (task != NULL && idx > 0) {
            task = task->next;
            idx--;
        }
        
        if (task == NULL) {
            exit_checklist_mode(board);
            return 0;
        }
        
        /* Get checklist count */
        int checklist_count_val = checklist_count(task);
        
        /* Esc exits checklist mode */
        if (key == 27) {  /* Escape */
            exit_checklist_mode(board);
            return 0;
        }
        
        /* Arrow keys navigate checklist items */
        if (key == KEY_DOWN || key == 'j') {
            if (board->checklist_index < checklist_count_val - 1) {
                board->checklist_index++;
            }
            return 0;
        }
        
        if (key == KEY_UP || key == 'k') {
            if (board->checklist_index > 0) {
                board->checklist_index--;
            }
            return 0;
        }
        
        /* Space toggles checked state of current item */
        if (key == ' ') {
            ChecklistItem *item = task->checklist;
            int item_idx = board->checklist_index;
            while (item != NULL && item_idx > 0) {
                item = item->next;
                item_idx--;
            }
            
            if (item != NULL) {
                checklist_item_toggle(item);
                /* Auto-save after toggling */
                if (board->filename[0] != '\0') {
                    board_save(board, board->filename);
                }
            }
            return 0;
        }
        
        /* 'N' (Shift+N) adds new checklist item */
        if (key == 'N') {
            char item_text[256] = {0};
            if (read_checklist_item(item_text, sizeof(item_text)) == 0) {
                checklist_item_add(task, item_text);
                /* Auto-save after adding */
                if (board->filename[0] != '\0') {
                    board_save(board, board->filename);
                }
            }
            return 0;
        }
        
        /* 'd' (double tap dd) deletes current checklist item */
        if (key == 'd') {
            /* Get current checklist item */
            ChecklistItem *item = task->checklist;
            int item_idx = board->checklist_index;
            while (item != NULL && item_idx > 0) {
                item = item->next;
                item_idx--;
            }
            
            if (item != NULL) {
                checklist_item_delete(task, item);
                /* Adjust index if needed */
                if (board->checklist_index >= checklist_count_val - 1) {
                    board->checklist_index = 0;
                }
                /* Auto-save after deleting */
                if (board->filename[0] != '\0') {
                    board_save(board, board->filename);
                }
            }
            return 0;
        }
        
        return 0;
    }
    
    /* MODE_HELP: any key closes the help overlay */
    if (board->app_mode == MODE_HELP) {
        board->app_mode = MODE_NORMAL;
        return 0;
    }

    /* MODE_NORMAL: route keys for navigation and commands */

    /* 'i' - also open description popup (D-01) */
    if (key == 'i') {
        /* Check if there's a selected task */
        Column *col = &board->columns[selection->column_index];
        if (col->task_count > 0) {
            board->app_mode = MODE_DESCRIPTION_VIEW;
        }
        return 0;
    }
    
    /* 'c' - enter checklist mode when in detailed view */
    if (key == 'c') {
        /* Only enter checklist mode if detailed view is enabled */
        if (board->detailed_view) {
            Column *col = &board->columns[selection->column_index];
            if (col->task_count > 0) {
                board->checklist_index = 0;
                board->app_mode = MODE_CHECKLIST;
            }
        }
        return 0;
    }
    
    /* Enter key - open description popup (D-01) */
    if (key == KEY_ENTER) {
        Column *col = &board->columns[selection->column_index];
        if (col->task_count > 0) {
            board->app_mode = MODE_DESCRIPTION_VIEW;
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
    
    /* Convert key to char for configurable keybindings */
    char key_char = (char)key;
    
    /* Check for configurable keybindings */
    int is_configurable_key = 0;
    
    /* Map configurable keys to their actions */
    if (key_char == key_create) {
        key = 'o';  /* Remap to create action */
        is_configurable_key = 1;
    } else if (key_char == key_delete) {
        key = 'd';  /* Remap to delete action */
        is_configurable_key = 1;
    } else if (key_char == key_up) {
        key = 'k';  /* Remap to up action */
        is_configurable_key = 1;
    } else if (key_char == key_down) {
        key = 'j';  /* Remap to down action */
        is_configurable_key = 1;
    } else if (key_char == key_left) {
        key = 'h';  /* Remap to left action */
        is_configurable_key = 1;
    } else if (key_char == key_right) {
        key = 'l';  /* Remap to right action */
        is_configurable_key = 1;
    }
    
    switch (key) {
        /* 'o' - create task below current position (D-05) */
        case 'o': {
            char title[256] = {0};
            if (read_task_title(title, sizeof(title)) == 0) {
                Task *new_task = task_create(title);
                if (new_task != NULL) {
                    /* Insert below current position */
                    int insert_pos = selection->task_index + 1;
                    task_insert_at(col, new_task, insert_pos);
                    selection->task_index = insert_pos;
                    
                    /* Auto-save per D-14 and STO-03 */
                    if (board->filename[0] != '\0') {
                        board_save(board, board->filename);
                    }
                }
            }
            break;
        }
        
        /* 'O' (Shift+o) - create task above current position (D-06) */
        case 'O': {
            char title[256] = {0};
            if (read_task_title(title, sizeof(title)) == 0) {
                Task *new_task = task_create(title);
                if (new_task != NULL) {
                    /* Insert above current position */
                    int insert_pos = selection->task_index;
                    task_insert_at(col, new_task, insert_pos);
                    /* Selection stays at same visual position, but now points to new task */
                    
                    /* Auto-save per D-14 and STO-03 */
                    if (board->filename[0] != '\0') {
                        board_save(board, board->filename);
                    }
                }
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
        /* Handle button 1 press - click to select task */
        if (event.bstate & BUTTON1_PRESSED || 
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
    
    /* Calculate which column was clicked */
    int col_width = COLS / 3;
    int col = screen_x / col_width;
    if (col > 2) col = 2;
    if (col < 0) col = 0;
    
    /* Get the column */
    Column *column = &board->columns[col];
    
    /* Calculate which task in the column based on screen position */
    int header_height = 2;  /* Column header rows */
    int task_idx = (screen_y - header_height);
    
    /* Clamp to valid range */
    if (task_idx < 0) task_idx = 0;
    if (task_idx >= column->task_count) task_idx = column->task_count - 1;
    if (column->task_count == 0) task_idx = 0;
    
    /* Update selection */
    selection->column_index = col;
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
    }
    
    return 0;
}