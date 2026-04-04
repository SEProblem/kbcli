/**
 * models.h - Data management for Task and Column structures
 * 
 * Provides CRUD operations for tasks and columns.
 */

#ifndef MODELS_H
#define MODELS_H

#include "kanban.h"

/**
 * Generate a UUID v4 string
 * Uses random bytes for UUID generation
 */
void generate_uuid(char *buffer, size_t size);

/**
 * Create a new task with the given title
 * Allocates memory and initializes task with UUID
 * 
 * @param title The task title string
 * @return Pointer to new Task, or NULL on failure
 */
Task* task_create(const char *title);

/**
 * Delete a task from a task list
 * Removes task from linked list and frees memory
 * 
 * @param head Pointer to task list head pointer
 * @param task_id UUID of task to delete
 * @return 0 on success, -1 if not found
 */
int task_delete(Task **head, const char *task_id);

/**
 * Move a task between columns
 * Removes task from source column and adds to target column
 * 
 * @param task Pointer to task to move
 * @param from Source column (task will be removed from here)
 * @param to Target column (task will be added to here)
 */
void task_move(Task *task, Column *from, Column *to);

/**
 * Reorder tasks within a column
 * Moves a task to a specific position in the column
 * 
 * @param head Pointer to task list head
 * @param task_id UUID of task to move
 * @param new_position New position index (0-based)
 * @return 0 on success, -1 if not found or invalid position
 */
int task_reorder(Task **head, const char *task_id, int new_position);

/**
 * Initialize a column with a name
 * 
 * @param col Pointer to column to initialize
 * @param name Column display name
 */
void column_init(Column *col, const char *name);

/**
 * Free all tasks in a column
 * 
 * @param col Pointer to column with tasks to free
 */
void column_free_tasks(Column *col);

/**
 * Initialize a board with default 3-column layout
 * Creates "To Do", "In Progress", "Done" columns
 * 
 * @param board Pointer to board to initialize
 */
void board_init(Board *board);

/**
 * Free all resources in a board
 * Frees all tasks in all columns
 * 
 * @param board Pointer to board to free
 */
void board_free(Board *board);

#endif /* MODELS_H */