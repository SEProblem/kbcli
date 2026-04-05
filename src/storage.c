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
#include "config.h"

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
    if (strcmp(name, COLUMN_IN_PROGRESS) == 0) return 1;
    if (strcmp(name, COLUMN_DONE) == 0) return 2;
    if (strcmp(name, "To Do") == 0) return 0;
    if (strcmp(name, "In Progress") == 0) return 1;
    if (strcmp(name, "Done") == 0) return 2;
    
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

/* Parse a checklist item line: "- [ ] text" or "- [x] text" */
static ChecklistItem* parse_checklist_line(const char *line) {
    if (line == NULL) return NULL;
    
    /* Look for "- [" pattern */
    const char *bracket = strstr(line, "- [");
    if (bracket == NULL) return NULL;
    
    /* Check if checked or unchecked */
    int checked = 0;
    if (strncmp(bracket, "- [x]", 5) == 0) {
        checked = 1;
    } else if (strncmp(bracket, "- [ ]", 5) != 0) {
        return NULL;  /* Not a valid checklist line */
    }
    
    /* Get checklist text after "- [ ] " or "- [x] " */
    const char *text_start = bracket + 5;
    while (*text_start == ' ') text_start++;
    
    if (*text_start == '\0') return NULL;
    
    trim_trailing((char*)text_start);
    
    /* Create checklist item */
    ChecklistItem *item = checklist_item_create(text_start);
    if (item != NULL) {
        item->checked = checked;
    }
    
    return item;
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
            
            /* Check for checklist item line (only after a task exists) */
            if (last_task != NULL && strncmp(line, "- [", 4) == 0) {
                ChecklistItem *item = parse_checklist_line(line);
                if (item != NULL) {
                    /* Append to end of checklist list (preserves file order) */
                    if (last_task->checklist == NULL) {
                        last_task->checklist = item;
                    } else {
                        ChecklistItem *ctail = last_task->checklist;
                        while (ctail->next != NULL) ctail = ctail->next;
                        ctail->next = item;
                    }
                    continue;
                }
            }
            
            Task *task = parse_task_line(line);
            if (task != NULL) {
                /* Append task to end of column list (preserves file order) */
                if (board->columns[current_column].tasks == NULL) {
                    board->columns[current_column].tasks = task;
                } else {
                    Task *tail = board->columns[current_column].tasks;
                    while (tail->next != NULL) tail = tail->next;
                    tail->next = task;
                }
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
            /* Write checklist items if present */
            ChecklistItem *item = task->checklist;
            while (item != NULL) {
                if (item->checked) {
                    fprintf(fp, "- [x] %s\n", item->text);
                } else {
                    fprintf(fp, "- [ ] %s\n", item->text);
                }
                item = item->next;
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
 * Uses config's default_board and board_directory if available
 */
int get_default_board_path(char *buffer, size_t size) {
    if (buffer == NULL || size == 0) return -1;
    
    /* Try to get board directory from config */
    Config config;
    config_load(&config);
    const char *board_dir = config_get_board_directory(&config);
    
    if (board_dir != NULL && board_dir[0] != '\0') {
        /* Use configured board directory with default board name */
        snprintf(buffer, size, "%s%s.md", board_dir, config.default_board);
        return 0;
    }
    
    /* Fallback to default */
    const char *home = getenv("HOME");
    if (home == NULL) {
        return -1;
    }
    
    snprintf(buffer, size, "%s/.config/kanban-cli/boards/default.md", home);
    return 0;
}

/**
 * Get the boards directory path
 * Returns from config if configured, otherwise default path
 */
int get_boards_directory(char *buffer, size_t size) {
    if (buffer == NULL || size == 0) return -1;
    
    /* Try to get board directory from config */
    Config config;
    config_load(&config);
    const char *board_dir = config_get_board_directory(&config);
    
    if (board_dir != NULL && board_dir[0] != '\0') {
        /* Use configured board directory */
        strncpy(buffer, board_dir, size - 1);
        buffer[size - 1] = '\0';
        return 0;
    }
    
    /* Fallback to default */
    const char *home = getenv("HOME");
    if (home == NULL) {
        return -1;
    }
    
    snprintf(buffer, size, "%s/.config/kanban-cli/boards/", home);
    return 0;
}

/**
 * List all available boards in the boards directory
 * Returns array of board names (without .md extension)
 * Caller must free the returned array and each board name
 * 
 * @param boards Pointer to array that receives board names
 * @param count Pointer to integer that receives board count
 * @return 0 on success, -1 on error
 */
int board_list_boards(char ***boards, int *count) {
    if (boards == NULL || count == NULL) return -1;
    
    *boards = NULL;
    *count = 0;
    
    /* Get boards directory */
    char dir_path[512];
    if (get_boards_directory(dir_path, sizeof(dir_path)) != 0) {
        return -1;
    }
    
    /* Open directory */
    DIR *dir = opendir(dir_path);
    if (dir == NULL) {
        /* Directory might not exist yet - return empty list */
        return 0;
    }
    
    /* First pass: count .md files */
    struct dirent *entry;
    int md_count = 0;
    
    while ((entry = readdir(dir)) != NULL) {
        /* Check for .md extension */
        size_t name_len = strlen(entry->d_name);
        if (name_len > 3) {
            if (strcmp(entry->d_name + name_len - 3, ".md") == 0) {
                md_count++;
            }
        }
    }
    
    /* Allocate array */
    if (md_count > 0) {
        *boards = (char**)malloc(sizeof(char*) * md_count);
        if (*boards == NULL) {
            closedir(dir);
            return -1;
        }
        
        /* Initialize all pointers to NULL for cleanup on error */
        for (int i = 0; i < md_count; i++) {
            (*boards)[i] = NULL;
        }
    }
    
    /* Second pass: collect board names */
    rewinddir(dir);
    int idx = 0;
    
    while ((entry = readdir(dir)) != NULL) {
        /* Check for .md extension */
        size_t name_len = strlen(entry->d_name);
        if (name_len > 3) {
            if (strcmp(entry->d_name + name_len - 3, ".md") == 0) {
                /* Copy name without .md extension */
                size_t base_len = name_len - 3;  /* Remove .md */
                (*boards)[idx] = (char*)malloc(base_len + 1);
                if ((*boards)[idx] != NULL) {
                    memcpy((*boards)[idx], entry->d_name, base_len);
                    (*boards)[idx][base_len] = '\0';
                    idx++;
                }
            }
        }
    }
    
    closedir(dir);
    *count = idx;
    
    return 0;
}

/**
 * Free board list allocated by board_list_boards
 */
void board_list_free(char **boards, int count) {
    if (boards == NULL) return;
    
    for (int i = 0; i < count; i++) {
        if (boards[i] != NULL) {
            free(boards[i]);
        }
    }
    free(boards);
}

/**
 * Create a new board file with 3-column template
 * Creates board at ~/.config/kanban-cli/boards/<name>.md
 * 
 * @param name Board name (without .md extension)
 * @return 0 on success, -1 on error
 */
int board_create(const char *name) {
    if (name == NULL || name[0] == '\0') return -1;
    
    /* Get boards directory */
    char dir_path[512];
    if (get_boards_directory(dir_path, sizeof(dir_path)) != 0) {
        return -1;
    }
    
    /* Create directory if needed */
    struct stat st;
    if (stat(dir_path, &st) != 0) {
        if (mkdir(dir_path, 0755) != 0) {
            return -1;
        }
    }
    
    /* Build full file path */
    char file_path[512];
    snprintf(file_path, sizeof(file_path), "%s%s.md", dir_path, name);
    
    /* Check if already exists */
    if (stat(file_path, &st) == 0) {
        /* Board already exists */
        return -1;
    }
    
    /* Create board with 3-column template using atomic write */
    char tmpfile[512];
    snprintf(tmpfile, sizeof(tmpfile), "%s.XXXXXX", file_path);
    
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
    
    /* Write 3-column template */
    fprintf(fp, "## To Do\n");
    fprintf(fp, "\n");
    fprintf(fp, "## In Progress\n");
    fprintf(fp, "\n");
    fprintf(fp, "## Done\n");
    
    /* Flush and sync */
    fflush(fp);
    fsync(fd);
    fclose(fp);
    
    /* Atomic rename */
    if (rename(tmpfile, file_path) < 0) {
        unlink(tmpfile);
        return -1;
    }
    
    return 0;
}

/**
 * Delete a board file
 * Deletes board at ~/.config/kanban-cli/boards/<name>.md
 * 
 * @param name Board name (without .md extension)
 * @return 0 on success, -1 on error
 */
int board_delete(const char *name) {
    if (name == NULL || name[0] == '\0') return -1;
    
    /* Get boards directory */
    char dir_path[512];
    if (get_boards_directory(dir_path, sizeof(dir_path)) != 0) {
        return -1;
    }
    
    /* Build full file path */
    char file_path[512];
    snprintf(file_path, sizeof(file_path), "%s%s.md", dir_path, name);
    
    /* Delete the file */
    if (unlink(file_path) != 0) {
        return -1;
    }
    
    return 0;
}

/**
 * Check if a board file exists
 * 
 * @param name Board name (without .md extension)
 * @return 1 if exists, 0 if not
 */
int board_exists(const char *name) {
    if (name == NULL || name[0] == '\0') return 0;
    
    /* Get boards directory */
    char dir_path[512];
    if (get_boards_directory(dir_path, sizeof(dir_path)) != 0) {
        return 0;
    }
    
    /* Build full file path */
    char file_path[512];
    snprintf(file_path, sizeof(file_path), "%s%s.md", dir_path, name);
    
    /* Check if file exists */
    struct stat st;
    if (stat(file_path, &st) == 0) {
        return 1;
    }
    
    return 0;
}