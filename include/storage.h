/**
 * storage.h - Markdown persistence layer for Kanban CLI
 * 
 * Provides functions for reading and writing boards in
 * GitHub-style Markdown format.
 */

#ifndef STORAGE_H
#define STORAGE_H

#include "kanban.h"

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

#endif /* STORAGE_H */