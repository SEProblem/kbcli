/**
 * storage.h - Markdown persistence layer for Kanban CLI
 * 
 * Provides functions for reading and writing boards in
 * GitHub-style Markdown format.
 */

#ifndef STORAGE_H
#define STORAGE_H

#include "kanban.h"
#include <dirent.h>

/**
 * Parse a markdown file and populate a Board structure
 * 
 * Expected format:
 * ## To Do
 * - [ ] Task 1
 * - [x] Task 2
 * ## In Progress
 * ...
 * 
 * @param board Pointer to board to populate
 * @param filepath Path to markdown file
 * @return 0 on success, -1 on error
 */
int parse_markdown(Board *board, const char *filepath);

/**
 * Write a Board structure to a markdown file
 * Uses atomic write pattern: temp file -> fsync -> rename
 * 
 * @param board Pointer to board to write
 * @param filepath Path to markdown file
 * @return 0 on success, -1 on error
 */
int write_markdown(Board *board, const char *filepath);

/**
 * Load a board from a markdown file
 * Creates empty board if file doesn't exist
 * 
 * @param board Pointer to board to load into
 * @param filepath Path to markdown file
 * @return 0 on success, -1 on error
 */
int board_load(Board *board, const char *filepath);

/**
 * Save a board to a markdown file
 * Auto-save triggered by create/delete/move operations
 * 
 * @param board Pointer to board to save
 * @param filepath Path to markdown file
 * @return 0 on success, -1 on error
 */
int board_save(Board *board, const char *filepath);

/**
 * Get the default board file path
 * Creates ~/.config/kanban-cli/boards/ directory if needed
 * 
 * @param buffer Buffer to store path
 * @param size Buffer size
 * @return 0 on success, -1 on error
 */
int get_default_board_path(char *buffer, size_t size);

/**
 * Get the boards directory path
 * Returns ~/.config/kanban-cli/boards/ or from config if available
 * 
 * @param buffer Buffer to store path
 * @param size Buffer size
 * @return 0 on success, -1 on error
 */
int get_boards_directory(char *buffer, size_t size);

/**
 * List all available boards in the boards directory
 * Returns array of board names (without .md extension)
 * Caller must free the returned array and each board name
 * 
 * @param boards Pointer to array that receives board names
 * @param count Pointer to integer that receives board count
 * @return 0 on success, -1 on error
 */
int board_list_boards(char ***boards, int *count);

/**
 * Free board list allocated by board_list_boards
 * 
 * @param boards Board array to free
 * @param count Number of boards in array
 */
void board_list_free(char **boards, int count);

/**
 * Create a new board file with 3-column template
 * Creates board at ~/.config/kanban-cli/boards/<name>.md
 * 
 * @param name Board name (without .md extension)
 * @return 0 on success, -1 on error
 */
int board_create(const char *name);

/**
 * Delete a board file
 * Deletes board at ~/.config/kanban-cli/boards/<name>.md
 * 
 * @param name Board name (without .md extension)
 * @return 0 on success, -1 on error
 */
int board_delete(const char *name);

/**
 * Rename a board file from old_name.md to new_name.md.
 *
 * @param old_name Current board name (without .md extension)
 * @param new_name Desired board name (without .md extension)
 * @return 0 on success, -1 on error (missing source, target exists, etc.)
 */
int board_rename(const char *old_name, const char *new_name);

/**
 * Check if a board file exists
 * 
 * @param name Board name (without .md extension)
 * @return 1 if exists, 0 if not
 */
int board_exists(const char *name);

#endif /* STORAGE_H */