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
    
    /* Initialize description fields */
    task->description[0] = '\0';
    task->desc_len = 0;
    
    /* Initialize checklist */
    task->checklist = NULL;
    
    task->completed = 0;
    task->next = NULL;
    
    return task;
}

/**
 * Create a new checklist item with the given text
 */
ChecklistItem* checklist_item_create(const char *text) {
    if (text == NULL) return NULL;
    
    ChecklistItem *item = (ChecklistItem*)malloc(sizeof(ChecklistItem));
    if (item == NULL) return NULL;
    
    /* Copy text with bounds checking */
    strncpy(item->text, text, sizeof(item->text) - 1);
    item->text[sizeof(item->text) - 1] = '\0';
    
    item->checked = 0;
    item->next = NULL;
    
    return item;
}

/**
 * Add a new checklist item to a task
 */
int checklist_item_add(Task *task, const char *text) {
    if (task == NULL || text == NULL) return -1;
    
    ChecklistItem *new_item = checklist_item_create(text);
    if (new_item == NULL) return -1;
    
    /* Append to end of list (preserves insertion order) */
    if (task->checklist == NULL) {
        task->checklist = new_item;
    } else {
        ChecklistItem *tail = task->checklist;
        while (tail->next != NULL) tail = tail->next;
        tail->next = new_item;
    }
    
    return 0;
}

/**
 * Toggle the checked state of a checklist item
 */
void checklist_item_toggle(ChecklistItem *item) {
    if (item == NULL) return;
    item->checked = !item->checked;
}

/**
 * Delete a checklist item from a task
 */
int checklist_item_delete(Task *task, ChecklistItem *item) {
    if (task == NULL || item == NULL) return -1;
    
    /* Handle item at head of list */
    if (task->checklist == item) {
        task->checklist = item->next;
        free(item);
        return 0;
    }
    
    /* Find and remove item from list */
    ChecklistItem *current = task->checklist;
    while (current != NULL && current->next != item) {
        current = current->next;
    }
    
    if (current != NULL && current->next == item) {
        current->next = item->next;
        free(item);
        return 0;
    }
    
    return -1;  /* Not found */
}

/**
 * Free all checklist items for a task
 */
void checklist_free(Task *task) {
    if (task == NULL) return;
    
    ChecklistItem *current = task->checklist;
    while (current != NULL) {
        ChecklistItem *next = current->next;
        free(current);
        current = next;
    }
    
    task->checklist = NULL;
}

/**
 * Get the count of checklist items
 */
int checklist_count(Task *task) {
    if (task == NULL) return 0;
    
    int count = 0;
    ChecklistItem *current = task->checklist;
    while (current != NULL) {
        count++;
        current = current->next;
    }
    
    return count;
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
            /* Free checklist before freeing task */
            checklist_free(current);
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
    board->app_mode = MODE_NORMAL;  /* Default to Normal mode per MOD-01 */
    board->detailed_view = 0;       /* Default to compact view */
    board->checklist_index = 0;     /* Reset checklist position */
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

/**
 * Move a task to a specific position in a column
 */
void task_move_to_column(Task *task, Column *from, Column *to, int position) {
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
    
    /* Insert at position in target column */
    if (position <= 0 || to->tasks == NULL) {
        /* Insert at head */
        task->next = to->tasks;
        to->tasks = task;
        to->task_count++;
        return;
    }
    
    /* Find position */
    Task *prev = to->tasks;
    int idx = position - 1;
    while (prev->next != NULL && idx > 0) {
        prev = prev->next;
        idx--;
    }
    
    /* Insert after prev */
    task->next = prev->next;
    prev->next = task;
    to->task_count++;
}

/**
 * Move selected task to the column on the left
 */
int move_left(Board *board, Selection *selection) {
    if (board == NULL || selection == NULL) return -1;
    
    /* Can't move left from column 0 */
    if (selection->column_index <= 0) return -1;
    
    Column *from = &board->columns[selection->column_index];
    Column *to = &board->columns[selection->column_index - 1];
    
    /* Get task at current selection */
    Task *task = from->tasks;
    int idx = selection->task_index;
    while (task != NULL && idx > 0) {
        task = task->next;
        idx--;
    }
    
    if (task == NULL) return -1;
    
    /* Move task to left column at end */
    task_move_to_column(task, from, to, -1);
    
    /* Update selection to track new position in new column */
    selection->column_index--;
    selection->task_index = (to->task_count > 0) ? to->task_count - 1 : 0;
    
    return 0;
}

/**
 * Move selected task to the column on the right
 */
int move_right(Board *board, Selection *selection) {
    if (board == NULL || selection == NULL) return -1;
    
    /* Can't move right from column 2 */
    if (selection->column_index >= 2) return -1;
    
    Column *from = &board->columns[selection->column_index];
    Column *to = &board->columns[selection->column_index + 1];
    
    /* Get task at current selection */
    Task *task = from->tasks;
    int idx = selection->task_index;
    while (task != NULL && idx > 0) {
        task = task->next;
        idx--;
    }
    
    if (task == NULL) return -1;
    
    /* Move task to right column at end */
    task_move_to_column(task, from, to, -1);
    
    /* Update selection to track new position in new column */
    selection->column_index++;
    selection->task_index = (to->task_count > 0) ? to->task_count - 1 : 0;
    
    return 0;
}

/**
 * Move selected task up within current column
 */
int move_up(Board *board, Selection *selection) {
    if (board == NULL || selection == NULL) return -1;
    
    Column *col = &board->columns[selection->column_index];
    
    /* Can't move up if at position 0 */
    if (selection->task_index <= 0) return -1;
    
    /* Need at least 2 tasks to reorder */
    if (col->task_count < 2) return -1;
    
    /* Get task at current position and previous */
    Task *prev = NULL;
    Task *current = col->tasks;
    int idx = selection->task_index;
    
    while (current != NULL && idx > 0) {
        prev = current;
        current = current->next;
        idx--;
    }
    
    if (current == NULL || prev == NULL) return -1;
    
    /* Get task before prev (for swap) — walk from head until walker reaches prev */
    Task *before_prev = NULL;
    Task *walker = col->tasks;
    while (walker != NULL && walker != prev) {
        before_prev = walker;
        walker = walker->next;
    }
    
    /* Swap current and prev */
    if (before_prev == NULL) {
        /* prev is at head */
        col->tasks = current;
        prev->next = current->next;
        current->next = prev;
    } else {
        before_prev->next = current;
        prev->next = current->next;
        current->next = prev;
    }
    
    /* Update selection */
    selection->task_index--;
    
    return 0;
}

/**
 * Move selected task down within current column
 */
int move_down(Board *board, Selection *selection) {
    if (board == NULL || selection == NULL) return -1;
    
    Column *col = &board->columns[selection->column_index];
    
    /* Can't move down if at last position */
    if (selection->task_index >= col->task_count - 1) return -1;
    
    /* Need at least 2 tasks to reorder */
    if (col->task_count < 2) return -1;
    
    /* Get the task after the selected one */
    Task *selected_task = col->tasks;
    Task *next_task = NULL;
    int idx = selection->task_index;
    
    while (selected_task != NULL && idx > 0) {
        selected_task = selected_task->next;
        idx--;
    }
    
    if (selected_task == NULL || selected_task->next == NULL) return -1;
    
    next_task = selected_task->next;
    
    /* Find node before selected_task — walk from head until walker reaches selected_task */
    Task *before_selected = NULL;
    Task *walker = col->tasks;
    while (walker != NULL && walker != selected_task) {
        before_selected = walker;
        walker = walker->next;
    }
    
    if (before_selected == NULL) {
        /* Selected task is at head */
        Task *temp = next_task->next;
        col->tasks = next_task;
        next_task->next = selected_task;
        selected_task->next = temp;
    } else {
        /* Swap positions */
        Task *temp = next_task->next;
        next_task->next = selected_task;
        selected_task->next = temp;
        before_selected->next = next_task;
    }
    
    /* Update selection */
    selection->task_index++;
    
    return 0;
}