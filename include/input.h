/**
 * input.h - Input handling for Kanban CLI
 * 
 * Provides keyboard input handling for task create/delete/navigation
 * following vim-style keybindings per D-05, D-06, D-08.
 */

#ifndef INPUT_H
#define INPUT_H

#include "kanban.h"
#include "renderer.h"

/**
 * Handle keyboard input
 * Processes key presses for task creation, deletion, and navigation
 * 
 * @param board Pointer to the board (for modifications)
 * @param key The key that was pressed
 * @param selection Pointer to current selection state
 * @return 0 if handled normally, 1 if quit requested
 */
int handle_input(Board *board, int key, Selection *selection);

/**
 * Get the active field index for the card popup
 * 0 = title, 1 = description, 2 = checklist
 * 
 * @return active field index
 */
int card_popup_active_field(void);

/**
 * Whether the popup checklist field is currently in inline-edit mode
 * (typing into a checklist item) vs. navigation mode (j/k/Space/etc).
 * Renderer uses this to park the cursor at the end of the item's text
 * instead of just at the start of its row.
 */
int card_popup_checklist_editing(void);

/**
 * Wait for a key press and return it
 * Used for input mode
 * 
 * @return The key pressed
 */
int wait_for_key(void);

/**
 * Get the last input error message
 * Returns NULL if no error
 *
 * @return Error message string or NULL
 */
const char* input_get_error(void);

/**
 * Clear any input error state
 */
void input_clear_error(void);

/**
 * Handle terminal resize event
 * Uses soft resize (resizeterm) to update terminal dimensions
 * then recalculates layout and redraws all windows
 * 
 * @param board Pointer to the board
 */
void handle_resize(Board *board);

/**
 * Switch to next board in list
 * 
 * @param board Pointer to the board
 * @param selection Pointer to current selection
 */
void switch_to_next_board(Board *board, Selection *selection);

/**
 * Switch to previous board in list
 * 
 * @param board Pointer to the board
 * @param selection Pointer to current selection
 */
void switch_to_previous_board(Board *board, Selection *selection);

/**
 * Create new board and switch to it
 * 
 * @param board Pointer to the board
 * @param selection Pointer to current selection
 */
void create_new_board(Board *board, Selection *selection);

/**
 * Parse and execute colon commands
 * Handles :bn, :bp, :b <name>, :bnew, :bcreate
 * 
 * @param board Pointer to the board
 * @param selection Pointer to current selection
 * @return 0 if handled normally
 */
int handle_colon_command(Board *board, Selection *selection);

#endif /* INPUT_H */