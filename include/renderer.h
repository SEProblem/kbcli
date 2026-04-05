/**
 * renderer.h - Board rendering functions for Kanban CLI
 * 
 * Provides ncurses-based rendering for kanban boards with
 * terminal-adaptive column widths and reverse video selection.
 */

#ifndef RENDERER_H
#define RENDERER_H

#include "kanban.h"

/**
 * Selection state for navigation
 * Tracks which task is currently selected
 */
typedef struct Selection {
    int column_index;  /* 0=To Do, 1=In Progress, 2=Done */
    int task_index;    /* Position in column's task list */
} Selection;

/**
 * Render the entire kanban board
 * Clears screen and renders all 3 columns with terminal-adaptive widths
 * 
 * @param board Pointer to the board to render
 * @param sel   Pointer to the current selection state
 */
void render_board(Board *board, Selection *sel);

/**
 * Render a single column
 * Draws column border, header, and all tasks
 * 
 * @param col Pointer to column to render
 * @param start_x Starting x position for column
 * @param width Width of the column
 * @param selected_col Whether this column is currently selected
 * @param sel_task_index Index of selected task in this column
 * @param scroll_offset Number of tasks to skip (for scrolling)
 * @param detailed_view Whether to show detailed view with description/checklist
 */
void render_column(Column *col, int start_x, int width, 
                   int selected_col, int sel_task_index, int scroll_offset,
                   int detailed_view);

/**
 * Render a single task
 * Displays "- [ ] title" or "- [x] title" format
 * Applies reverse video highlight if selected
 * In detailed_view mode, also shows description and checklist
 * 
 * @param task Pointer to task to render
 * @param y Row position
 * @param x Column position
 * @param width Available width for task display
 * @param selected Whether this task is currently selected
 * @param detailed_view Whether to show detailed view
 */
void render_task(Task *task, int y, int x, int width, int selected, int detailed_view);

/**
 * Highlight the selected task using reverse video
 * Should be called before rendering a selected task
 * 
 * @param selected 1 if task is selected, 0 otherwise
 */
void highlight_selected(int selected);

/**
 * Initialize renderer state
 * Should be called after initscr()
 */
void renderer_init(void);

/**
 * Get scroll offset for a specific column
 * 
 * @param column_index The column index
 * @return Current scroll offset
 */
int get_scroll_offset(int column_index);

/**
 * Set scroll offset for a specific column
 * 
 * @param column_index The column index
 * @param offset New scroll offset
 */
void set_scroll_offset(int column_index, int offset);

/**
 * Recalculate window layout based on new terminal dimensions
 * Called after resizeterm() to recreate windows with new sizes
 * 
 * @param lines New number of rows
 * @param cols New number of columns
 */
void renderer_calculate_layout(int lines, int cols);

/**
 * Redraw all windows after resize or other events
 * Clears screen and renders all columns and status bar
 */
void renderer_redraw_all(void);

/**
 * Render description popup overlay
 * Displays task title and description in centered overlay
 * Per D-01, D-02, D-03: popup for viewing/editing descriptions
 * 
 * @param board Pointer to the board
 * @param selection Pointer to current selection state
 */
void render_description_popup(Board *board, Selection *selection);

/**
 * Render a scrollable list of available boards
 * Displays board names with current board highlighted
 * 
 * @param board_names Array of board names
 * @param count Number of boards
 * @param selected Currently selected board index
 */
void render_board_list(char **board_names, int count, int selected);

/**
 * Show board list menu and wait for user selection
 * Returns selected board name (caller must free), or NULL on cancel
 */
char* show_board_list_menu(void);

/**
 * Render help popup overlay
 * Displays all keybindings in a centered overlay
 * Press any key to close
 */
void render_help_popup(void);

#endif /* RENDERER_H */