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

/* Maximum description length per TSK-01 */
#define MAX_DESC_LEN 500

/**
 * Mode enumeration for vim-style modal editing
 * MOD-01, MOD-02 per requirements
 */
typedef enum {
    MODE_NORMAL = 0,
    MODE_INSERT,
    MODE_DESCRIPTION_VIEW,
    MODE_DESCRIPTION_EDIT,
    MODE_CHECKLIST,
    MODE_HELP,
    MODE_CARD_POPUP
} AppMode;

/**
 * ChecklistItem structure for task subtasks
 * Uses linked list for multiple checklist items per task
 */
typedef struct ChecklistItem {
    char text[256];
    int checked;                     /* 0 = unchecked, 1 = checked */
    struct ChecklistItem *next;      /* Linked list for checklist items */
} ChecklistItem;

/**
 * Task structure with GitHub-style markdown syntax
 * Uses linked list for column tasks
 */
typedef struct Task {
    char id[37];           /* UUID v4 */
    char title[256];       /* Task title */
    char description[MAX_DESC_LEN];  /* Task description */
    size_t desc_len;       /* Length of description */
    ChecklistItem *checklist;  /* Linked list of checklist items */
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
    AppMode app_mode;      /* Current mode (Normal/Insert) per MOD-01, MOD-02 */
    int detailed_view;     /* Toggle between compact (0) and detailed (1) view */
    int checklist_index;   /* Current position in checklist when in MODE_CHECKLIST */
} Board;

/* Column names for default 3-column layout */
#define COLUMN_TODO "To Do"
#define COLUMN_IN_PROGRESS "In Progress"
#define COLUMN_DONE "Done"

/* Default column names array */
extern const char* DEFAULT_COLUMN_NAMES[3];

#endif /* KANBAN_H */