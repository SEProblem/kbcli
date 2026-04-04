/**
 * models.c - Implementation of Task and Column data management
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#include "kanban.h"
#include "models.h"

/* Default column names for 3-column layout */
const char* DEFAULT_COLUMN_NAMES[3] = {
    COLUMN_TODO,
    COLUMN_IN_PROGRESS,
    COLUMN_DONE
};

/**
 * Generate a UUID v4 string
 */
void generate_uuid(char *buffer, size_t size) {
    if (buffer == NULL || size < 37) return;
    
    /* UUID v4 format: xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx */
    snprintf(buffer, size, 
             "%08x-%04x-4%03x-%02x%02x-%012lx",
             (unsigned int)rand(),
             (unsigned int)rand() & 0xFFFF,
             (unsigned int)rand() & 0x0FFF,
             ((unsigned int)rand() & 0x3F) | 0x80,
             (unsigned int)rand() & 0xFF,
             (unsigned long)rand());
}

/**
 * Create a new task with the given title
 */
Task* task_create(const char *title) {
    if (title == NULL) return NULL;
    
    Task *task = (Task*)malloc(sizeof(Task));
    if (task == NULL) return NULL;
    
    /* Generate UUID */
    generate_uuid(task->id, sizeof(task->id));
    
    /* Copy title with bounds checking */
    strncpy(task->title, title, sizeof(task->title) - 1);
    task->title[sizeof(task->title) - 1] = '\0';
    
    task->completed = 0;
    task->next = NULL;
    
    return task;
}

/**
 * Delete a task from a task list
 */
int task_delete(Task **head, const char *task_id) {
    if (head == NULL || task_id == NULL) return -1;
    
    Task *current = *head;
    Task *prev = NULL;
    
    while (current != NULL) {
        if (strcmp(current->id, task_id) == 0) {
            /* Found the task */
            if (prev == NULL) {
                /* Task is at head */
                *head = current->next;
            } else {
                prev->next = current->next;
            }
            free(current);
            return 0;
        }
        prev = current;
        current = current->next;
    }
    
    return -1;  /* Not found */
}

/**
 * Move a task between columns
 */
void task_move(Task *task, Column *from, Column *to) {
    if (task == NULL || from == NULL || to == NULL) return;
    
    /* Remove from source column */
    Task **current = &from->tasks;
    while (*current != NULL) {
        if (*current == task) {
            *current = task->next;
            from->task_count--;
            break;
        }
        current = &(*current)->next;
    }
    
    /* Add to target column */
    task->next = to->tasks;
    to->tasks = task;
    to->task_count++;
}

/**
 * Reorder tasks within a column
 */
int task_reorder(Task **head, const char *task_id, int new_position) {
    if (head == NULL || task_id == NULL || new_position < 0) return -1;
    
    /* Find the task */
    Task *current = *head;
    Task *prev = NULL;
    Task *target_task = NULL;
    int pos = 0;
    
    while (current != NULL) {
        if (strcmp(current->id, task_id) == 0) {
            target_task = current;
            break;
        }
        prev = current;
        current = current->next;
        pos++;
    }
    
    if (target_task == NULL) return -1;  /* Not found */
    
    /* Don't reorder if already at position */
    if (pos == new_position) return 0;
    
    /* Remove from current position */
    if (prev == NULL) {
        *head = target_task->next;
    } else {
        prev->next = target_task->next;
    }
    
    /* Insert at new position */
    current = *head;
    prev = NULL;
    pos = 0;
    
    while (current != NULL && pos < new_position) {
        prev = current;
        current = current->next;
        pos++;
    }
    
    /* Insert */
    if (prev == NULL) {
        target_task->next = *head;
        *head = target_task;
    } else {
        target_task->next = current;
        prev->next = target_task;
    }
    
    return 0;
}

/**
 * Initialize a column with a name
 */
void column_init(Column *col, const char *name) {
    if (col == NULL) return;
    
    if (name != NULL) {
        strncpy(col->name, name, sizeof(col->name) - 1);
        col->name[sizeof(col->name) - 1] = '\0';
    } else {
        col->name[0] = '\0';
    }
    
    col->tasks = NULL;
    col->task_count = 0;
}

/**
 * Free all tasks in a column
 */
void column_free_tasks(Column *col) {
    if (col == NULL) return;
    
    Task *current = col->tasks;
    while (current != NULL) {
        Task *next = current->next;
        free(current);
        current = next;
    }
    
    col->tasks = NULL;
    col->task_count = 0;
}

/**
 * Initialize a board with default 3-column layout
 */
void board_init(Board *board) {
    if (board == NULL) return;
    
    /* Initialize 3 default columns */
    for (int i = 0; i < 3; i++) {
        column_init(&board->columns[i], DEFAULT_COLUMN_NAMES[i]);
    }
    
    board->filename[0] = '\0';
    board->last_modified = 0;
}

/**
 * Free all resources in a board
 */
void board_free(Board *board) {
    if (board == NULL) return;
    
    /* Free all tasks in all columns */
    for (int i = 0; i < 3; i++) {
        column_free_tasks(&board->columns[i]);
    }
}