/**
 * kanban.h - Core data structures for Kanban CLI
 * 
 * Defines Board, Column, and Task structures for GitHub-style
 * markdown persistence.
 */

#ifndef KANBAN_H
#define KANBAN_H

#include <stddef.h>
#include <time.h>

/**
 * Task structure with GitHub-style markdown syntax
 * Uses linked list for column tasks
 */
typedef struct Task {
    char id[37];           /* UUID v4 */
    char title[256];       /* Task title */
    int completed;         /* 0 = unchecked, 1 = checked */
    struct Task *next;     /* Linked list for column tasks */
} Task;

/**
 * Column structure representing kanban columns
 */
typedef struct Column {
    char name[64];         /* Column display name */
    Task *tasks;           /* Linked list of tasks */
    int task_count;        /* Number of tasks */
} Column;

/**
 * Board structure representing a kanban board
 */
typedef struct Board {
    Column columns[3];     /* To Do, In Progress, Done */
    char filename[512];    /* Markdown file path */
    time_t last_modified;  /* For change detection */
} Board;

/* Column names for default 3-column layout */
#define COLUMN_TODO "To Do"
#define COLUMN_IN_PROGRESS "In Progress"
#define COLUMN_DONE "Done"

/* Default column names array */
extern const char* DEFAULT_COLUMN_NAMES[3];

#endif /* KANBAN_H */