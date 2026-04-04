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
 * Wait for a key press and return it
 * Used for input mode
 * 
 * @return The key pressed
 */
int wait_for_key(void);

/**
 * Read task title from user input
 * Uses ncurses input field with prompt
 * 
 * @param buffer Buffer to store the title
 * @param size Size of the buffer
 * @return 0 on success, -1 on cancel
 */
int read_task_title(char *buffer, size_t size);

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

#endif /* INPUT_H */