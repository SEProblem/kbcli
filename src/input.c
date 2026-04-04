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
#include "input.h"

/* Static error message for input failures */
static char input_error[256] = {0};

/* Timestamp for double-g detection (jump to top) */
static time_t last_g_press_time = 0;
#define G_DOUBLE_TAP_TIMEOUT_MS 300

/* External board filename for auto-save */
extern char global_board_filename[512];

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
    
    /* MODE_NORMAL: route keys for navigation and commands */
    
    /* 'i' - enter Insert mode (MOD-03) */
    if (key == 'i') {
        enter_insert_mode(board);
        return 0;
    }
    
    /* Enter key - enter Insert mode (MOD-03 alternative) */
    if (key == KEY_ENTER) {
        enter_insert_mode(board);
        return 0;
    }
    
    Column *col = &board->columns[selection->column_index];
    int task_count = col->task_count;
    
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
            break;
        }
        
        case KEY_UP:
        case 'k': {
            /* Move selection up */
            if (selection->task_index > 0) {
                selection->task_index--;
            }
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
            break;
        }
        
        /* 'g' - jump to first task (top of column), supports double-tap gg */
        case 'g': {
            /* Check for double-tap (gg) within timeout */
            time_t now = time(NULL);
            double elapsed = difftime(now, last_g_press_time);
            
            /* If second 'g' pressed within G_DOUBLE_TAP_TIMEOUT_MS ms, jump to top */
            if (elapsed < 0.3) {  /* 300ms timeout */
                selection->task_index = 0;
            }
            /* Store timestamp for next press */
            last_g_press_time = now;
            break;
        }
        
        /* 'G' - jump to last task (bottom of column) */
        case 'G': {
            selection->task_index = (task_count > 0) ? task_count - 1 : 0;
            break;
        }
        
        /* 'q' or 'Q' - quit */
        case 'q':
        case 'Q': {
            return 1;  /* Signal quit */
        }
        
        /* Handle resize */
        case KEY_RESIZE: {
            /* Will be handled by redraw in main loop */
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