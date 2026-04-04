/**
 * storage.c - Implementation of Markdown persistence layer
 * 
 * Provides atomic file writes using temp file + fsync + rename pattern.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <libgen.h>

#include "kanban.h"
#include "models.h"
#include "storage.h"

/* Trim trailing whitespace from a string */
static void trim_trailing(char *str) {
    size_t len = strlen(str);
    while (len > 0 && (str[len - 1] == ' ' || str[len - 1] == '\t' ||
                       str[len - 1] == '\n' || str[len - 1] == '\r')) {
        str[--len] = '\0';
    }
}

/* Parse a task line: "- [ ] title" or "- [x] title" */
static Task* parse_task_line(const char *line) {
    if (line == NULL) return NULL;
    
    /* Look for "- [" pattern */
    const char *bracket = strstr(line, "- [");
    if (bracket == NULL) return NULL;
    
    /* Check if checked or unchecked */
    int completed = 0;
    if (strncmp(bracket, "- [x]", 5) == 0) {
        completed = 1;
    } else if (strncmp(bracket, "- [ ]", 5) != 0) {
        return NULL;  /* Not a valid task line */
    }
    
    /* Get task title after "- [ ] " or "- [x] " */
    const char *title_start = bracket + 5;
    while (*title_start == ' ') title_start++;
    
    if (*title_start == '\0') return NULL;
    
    /* Create task */
    Task *task = task_create(title_start);
    if (task != NULL) {
        task->completed = completed;
    }
    
    return task;
}

/* Get column index from H2 header "## Column Name" */
static int parse_column_header(const char *line) {
    if (line == NULL) return -1;
    
    /* Skip "## " prefix */
    if (strncmp(line, "## ", 3) != 0) return -1;
    
    const char *name = line + 3;
    trim_trailing((char*)name);
    
    /* Match against known column names */
    if (strcmp(name, COLUMN_TODO) == 0) return 0;
    if (strcmp(name, COLUMN_IN_PROGRESS) == 1) return 1;
    if (strcmp(name, COLUMN_DONE) == 2) return 2;
    if (strcmp(name, "To Do") == 0) return 0;
    if (strcmp(name, "In Progress") == 1) return 1;
    if (strcmp(name, "Done") == 2) return 2;
    
    return -1;  /* Unknown column */
}

/* Parse description from a task line following the title */
static void parse_task_description(Task *task, const char *description_line) {
    if (task == NULL || description_line == NULL) return;
    
    /* Skip "Description: " prefix (13 chars) */
    const char *desc_start = description_line + 13;
    trim_trailing((char*)desc_start);
    
    /* Copy description with bounds checking */
    size_t desc_len = strlen(desc_start);
    if (desc_len > 0) {
        strncpy(task->description, desc_start, sizeof(task->description) - 1);
        task->description[sizeof(task->description) - 1] = '\0';
        task->desc_len = strlen(task->description);
    }
}

/**
 * Parse a markdown file and populate a Board structure
 */
int parse_markdown(Board *board, const char *filepath) {
    if (board == NULL || filepath == NULL) return -1;
    
    FILE *fp = fopen(filepath, "r");
    if (fp == NULL) {
        /* File doesn't exist - return empty board */
        return 0;
    }
    
    char line[1024];
    int current_column = -1;
    Task *last_task = NULL;
    
    while (fgets(line, sizeof(line), fp) != NULL) {
        /* Check for column header */
        int col = parse_column_header(line);
        if (col >= 0 && col < 3) {
            current_column = col;
            last_task = NULL;
            continue;
        }
        
        /* If in a column, try to parse task */
        if (current_column >= 0) {
            /* Check for Description: line */
            if (last_task != NULL && strncmp(line, "Description: ", 13) == 0) {
                parse_task_description(last_task, line);
                continue;
            }
            
            Task *task = parse_task_line(line);
            if (task != NULL) {
                /* Add task to column */
                task->next = board->columns[current_column].tasks;
                board->columns[current_column].tasks = task;
                board->columns[current_column].task_count++;
                last_task = task;
            } else if (strncmp(line, "## ", 3) != 0 && line[0] != '\n' && line[0] != '\r') {
                /* Non-task, non-header line - reset last_task tracking */
                if (line[0] != '-') {
                    last_task = NULL;
                }
            }
        }
    }
    
    fclose(fp);
    return 0;
}

/**
 * Write a Board structure to a markdown file
 * Uses atomic write pattern
 */
int write_markdown(Board *board, const char *filepath) {
    if (board == NULL || filepath == NULL) return -1;
    
    /* Create temp file path */
    char tmpfile[512];
    snprintf(tmpfile, sizeof(tmpfile), "%s.XXXXXX", filepath);
    
    /* Create and open temp file */
    int fd = mkstemp(tmpfile);
    if (fd < 0) {
        return -1;
    }
    
    FILE *fp = fdopen(fd, "w");
    if (fp == NULL) {
        close(fd);
        unlink(tmpfile);
        return -1;
    }
    
    /* Write each column */
    for (int i = 0; i < 3; i++) {
        Column *col = &board->columns[i];
        
        /* Write column header */
        fprintf(fp, "## %s\n", col->name);
        
        /* Write tasks */
        Task *task = col->tasks;
        while (task != NULL) {
            if (task->completed) {
                fprintf(fp, "- [x] %s\n", task->title);
            } else {
                fprintf(fp, "- [ ] %s\n", task->title);
            }
            /* Write description if present */
            if (task->description[0] != '\0') {
                fprintf(fp, "Description: %s\n", task->description);
            }
            task = task->next;
        }
        
        /* Empty line between columns */
        fprintf(fp, "\n");
    }
    
    /* Flush and sync */
    fflush(fp);
    fsync(fd);
    fclose(fp);
    
    /* Atomic rename */
    if (rename(tmpfile, filepath) < 0) {
        unlink(tmpfile);
        return -1;
    }
    
    return 0;
}

/**
 * Load a board from a markdown file
 */
int board_load(Board *board, const char *filepath) {
    if (board == NULL || filepath == NULL) return -1;
    
    /* Store filename */
    strncpy(board->filename, filepath, sizeof(board->filename) - 1);
    board->filename[sizeof(board->filename) - 1] = '\0';
    
    /* Try to parse markdown file */
    if (parse_markdown(board, filepath) == 0) {
        /* Get file modification time */
        struct stat st;
        if (stat(filepath, &st) == 0) {
            board->last_modified = st.st_mtime;
        }
        return 0;
    }
    
    return -1;
}

/**
 * Save a board to a markdown file
 */
int board_save(Board *board, const char *filepath) {
    if (board == NULL || filepath == NULL) return -1;
    
    /* Ensure directory exists */
    char *path_copy = strdup(filepath);
    if (path_copy == NULL) return -1;
    
    char *dir = dirname(path_copy);
    
    /* Create directory if needed */
    struct stat st;
    if (stat(dir, &st) != 0) {
        mkdir(dir, 0755);
    }
    
    free(path_copy);
    
    /* Write markdown file */
    int result = write_markdown(board, filepath);
    
    if (result == 0) {
        /* Update modification time */
        struct stat st;
        if (stat(filepath, &st) == 0) {
            board->last_modified = st.st_mtime;
        }
    }
    
    return result;
}

/**
 * Get the default board file path
 */
int get_default_board_path(char *buffer, size_t size) {
    if (buffer == NULL || size == 0) return -1;
    
    const char *home = getenv("HOME");
    if (home == NULL) {
        return -1;
    }
    
    snprintf(buffer, size, "%s/.config/kanban-cli/boards/default.md", home);
    return 0;
}