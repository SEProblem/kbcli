/**
 * config.c - Configuration management implementation for Kanban CLI
 * 
 * Implements JSON config loading/saving, keybindings parsing,
 * and default board directory management.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "config.h"

/* Global key mappings - used by input handler */
char key_up = DEFAULT_KEY_UP;
char key_down = DEFAULT_KEY_DOWN;
char key_left = DEFAULT_KEY_LEFT;
char key_right = DEFAULT_KEY_RIGHT;
char key_create = DEFAULT_KEY_CREATE;
char key_delete = DEFAULT_KEY_DELETE;

/* Static error message */
static char config_error[256] = {0};

const char* config_get_error(void) {
    if (config_error[0] == '\0') {
        return NULL;
    }
    return config_error;
}

void config_clear_error(void) {
    config_error[0] = '\0';
}

/**
 * Get the config file path
 * Returns ~/.config/kanban-cli/config
 */
int get_config_path(char *buffer, size_t size) {
    if (buffer == NULL || size == 0) return -1;
    
    const char *home = getenv("HOME");
    if (home == NULL) {
        snprintf(config_error, sizeof(config_error), "HOME environment variable not set");
        return -1;
    }
    
    snprintf(buffer, size, "%s/.config/kanban-cli/config", home);
    return 0;
}

/**
 * Initialize config with defaults
 */
void config_init_defaults(Config *config) {
    if (config == NULL) return;
    
    const char *home = getenv("HOME");
    if (home != NULL) {
        snprintf(config->board_directory, sizeof(config->board_directory),
                 "%s/.config/kanban-cli/boards/", home);
    } else {
        config->board_directory[0] = '\0';
    }
    
    config->auto_save = 1;
    
    strncpy(config->keybindings, DEFAULT_KEYBINDINGS, sizeof(config->keybindings) - 1);
    config->keybindings[sizeof(config->keybindings) - 1] = '\0';
    
    strncpy(config->default_board, "default", sizeof(config->default_board) - 1);
    config->default_board[sizeof(config->default_board) - 1] = '\0';
    
    /* Reset key mappings to defaults */
    key_up = DEFAULT_KEY_UP;
    key_down = DEFAULT_KEY_DOWN;
    key_left = DEFAULT_KEY_LEFT;
    key_right = DEFAULT_KEY_RIGHT;
    key_create = DEFAULT_KEY_CREATE;
    key_delete = DEFAULT_KEY_DELETE;
}

/**
 * Trim trailing whitespace from a string
 */
static void trim_trailing(char *str) {
    size_t len = strlen(str);
    while (len > 0 && (str[len - 1] == ' ' || str[len - 1] == '\t' ||
                       str[len - 1] == '\n' || str[len - 1] == '\r')) {
        str[--len] = '\0';
    }
}

/**
 * Extract string value from JSON key-value pair
 * Simple parser - finds "key":"value" pattern
 */
static int parse_json_string(const char *json, const char *key, char *out, size_t out_size) {
    if (json == NULL || key == NULL || out == NULL || out_size == 0) return -1;
    
    /* Build search pattern: "key":" */
    char search[128];
    snprintf(search, sizeof(search), "\"%s\":\"", key);
    
    const char *key_pos = strstr(json, search);
    if (key_pos == NULL) {
        /* Try without colon: "key": "value" */
        snprintf(search, sizeof(search), "\"%s\": \"", key);
        key_pos = strstr(json, search);
    }
    
    if (key_pos == NULL) return -1;
    
    /* Find the start of value */
    const char *value_start = strchr(key_pos, '"');
    if (value_start == NULL) return -1;
    value_start++;  /* Skip opening quote */
    
    /* Find closing quote */
    const char *value_end = strchr(value_start, '"');
    if (value_end == NULL || value_end <= value_start) return -1;
    
    /* Copy value with bounds checking */
    size_t value_len = value_end - value_start;
    if (value_len > out_size - 1) {
        value_len = out_size - 1;
    }
    
    memcpy(out, value_start, value_len);
    out[value_len] = '\0';
    
    trim_trailing(out);
    
    return 0;
}

/**
 * Extract boolean value from JSON key-value pair
 */
static int parse_json_bool(const char *json, const char *key, int *out) {
    if (json == NULL || key == NULL || out == NULL) return -1;
    
    /* Build search pattern: "key":value or "key": true/false */
    char search[128];
    snprintf(search, sizeof(search), "\"%s\":", key);
    
    const char *key_pos = strstr(json, search);
    if (key_pos == NULL) {
        /* Try with space: "key": */
        snprintf(search, sizeof(search), "\"%s\": ", key);
        key_pos = strstr(json, search);
    }
    
    if (key_pos == NULL) return -1;
    
    /* Find the start of value */
    const char *value_start = key_pos + strlen(search);
    while (*value_start == ' ' || *value_start == '\t') value_start++;
    
    /* Check for true/false or 1/0 */
    if (strncmp(value_start, "true", 4) == 0 || strncmp(value_start, "1", 1) == 0) {
        *out = 1;
        return 0;
    }
    if (strncmp(value_start, "false", 5) == 0 || strncmp(value_start, "0", 1) == 0) {
        *out = 0;
        return 0;
    }
    
    return -1;
}

/**
 * Load configuration from JSON config file
 */
int config_load(Config *config) {
    if (config == NULL) return -1;
    
    /* Initialize with defaults first */
    config_init_defaults(config);
    
    /* Get config file path */
    char config_path[MAX_PATH];
    if (get_config_path(config_path, sizeof(config_path)) != 0) {
        return -1;
    }
    
    /* Check if config file exists */
    struct stat st;
    if (stat(config_path, &st) != 0) {
        /* Config file doesn't exist - use defaults, but create config dir if needed */
        char dir_path[MAX_PATH];
        const char *home = getenv("HOME");
        if (home != NULL) {
            snprintf(dir_path, sizeof(dir_path), "%s/.config/kanban-cli", home);
            if (stat(dir_path, &st) != 0) {
                mkdir(dir_path, 0755);
            }
            snprintf(dir_path, sizeof(dir_path), "%s/.config/kanban-cli/boards", home);
            if (stat(dir_path, &st) != 0) {
                mkdir(dir_path, 0755);
            }
        }
        return 0;  /* Use defaults */
    }
    
    /* Read config file */
    FILE *fp = fopen(config_path, "r");
    if (fp == NULL) {
        return 0;  /* Use defaults */
    }
    
    /* Get file size */
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    if (file_size <= 0 || file_size > 4096) {
        fclose(fp);
        return 0;  /* Use defaults - file too large or empty */
    }
    
    /* Read file contents */
    char *json = (char*)malloc(file_size + 1);
    if (json == NULL) {
        fclose(fp);
        return -1;
    }
    
    size_t bytes_read = fread(json, 1, file_size, fp);
    json[bytes_read] = '\0';
    fclose(fp);
    
    /* Parse JSON values */
    parse_json_string(json, "board_directory", config->board_directory, sizeof(config->board_directory));
    parse_json_bool(json, "auto_save", &config->auto_save);
    parse_json_string(json, "keybindings", config->keybindings, sizeof(config->keybindings));
    parse_json_string(json, "default_board", config->default_board, sizeof(config->default_board));
    
    free(json);
    
    return 0;
}

/**
 * Save configuration to JSON config file
 * Uses atomic write pattern
 */
int config_save(const Config *config) {
    if (config == NULL) return -1;
    
    /* Get config file path */
    char config_path[MAX_PATH];
    if (get_config_path(config_path, sizeof(config_path)) != 0) {
        return -1;
    }
    
    /* Ensure directory exists */
    char dir_path[MAX_PATH];
    const char *home = getenv("HOME");
    if (home != NULL) {
        snprintf(dir_path, sizeof(dir_path), "%s/.config/kanban-cli", home);
        struct stat st;
        if (stat(dir_path, &st) != 0) {
            mkdir(dir_path, 0755);
        }
    }
    
    /* Create temp file */
    char tmpfile[MAX_PATH];
    snprintf(tmpfile, sizeof(tmpfile), "%s.XXXXXX", config_path);
    
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
    
    /* Write JSON */
    fprintf(fp, "{\n");
    fprintf(fp, "  \"board_directory\": \"%s\",\n", config->board_directory);
    fprintf(fp, "  \"auto_save\": %s,\n", config->auto_save ? "true" : "false");
    fprintf(fp, "  \"keybindings\": %s,\n", config->keybindings);
    fprintf(fp, "  \"default_board\": \"%s\"\n", config->default_board);
    fprintf(fp, "}\n");
    
    /* Flush and sync */
    fflush(fp);
    fsync(fd);
    fclose(fp);
    
    /* Atomic rename */
    if (rename(tmpfile, config_path) < 0) {
        unlink(tmpfile);
        return -1;
    }
    
    return 0;
}

/**
 * Get the configured board directory
 * Returns configured path or default if not set
 */
const char* config_get_board_directory(const Config *config) {
    if (config == NULL) return NULL;
    
    if (config->board_directory[0] != '\0') {
        return config->board_directory;
    }
    
    /* Return default */
    static char default_dir[MAX_PATH];
    const char *home = getenv("HOME");
    if (home != NULL) {
        snprintf(default_dir, sizeof(default_dir), "%s/.config/kanban-cli/boards/", home);
        return default_dir;
    }
    
    return NULL;
}

/**
 * Get the default keybindings JSON string
 */
const char* config_get_default_keybindings(void) {
    return DEFAULT_KEYBINDINGS;
}

/**
 * Extract individual keybinding from JSON object
 * Returns the key name mapped to the action
 */
static int extract_keybinding(const char *json, const char *action, char *out, size_t out_size) {
    if (json == NULL || action == NULL || out == NULL || out_size == 0) return -1;
    
    /* Find action in keybindings: "action":"key" */
    char search[128];
    snprintf(search, sizeof(search), "\"%s\":\"", action);
    
    const char *action_pos = strstr(json, search);
    if (action_pos == NULL) {
        /* Try alternate format: "action": "key" */
        snprintf(search, sizeof(search), "\"%s\": \"", action);
        action_pos = strstr(json, search);
    }
    
    if (action_pos == NULL) return -1;
    
    /* Find the start of key value */
    const char *key_start = strchr(action_pos, '"');
    if (key_start == NULL) return -1;
    key_start++;  /* Skip opening quote */
    
    /* Find closing quote */
    const char *key_end = strchr(key_start, '"');
    if (key_end == NULL || key_end <= key_start) return -1;
    
    /* Copy key with bounds checking */
    size_t key_len = key_end - key_start;
    if (key_len > out_size - 1) {
        key_len = out_size - 1;
    }
    
    memcpy(out, key_start, key_len);
    out[key_len] = '\0';
    
    return 0;
}

/**
 * Apply keybindings from JSON config string
 * Parses JSON keybindings object and updates global key mappings
 */
void apply_keybindings_from_config(const char *json) {
    if (json == NULL) return;
    
    char key_buf[16];
    
    /* Parse each keybinding */
    if (extract_keybinding(json, "up", key_buf, sizeof(key_buf)) == 0 && key_buf[0] != '\0') {
        key_up = key_buf[0];
    }
    if (extract_keybinding(json, "down", key_buf, sizeof(key_buf)) == 0 && key_buf[0] != '\0') {
        key_down = key_buf[0];
    }
    if (extract_keybinding(json, "left", key_buf, sizeof(key_buf)) == 0 && key_buf[0] != '\0') {
        key_left = key_buf[0];
    }
    if (extract_keybinding(json, "right", key_buf, sizeof(key_buf)) == 0 && key_buf[0] != '\0') {
        key_right = key_buf[0];
    }
    if (extract_keybinding(json, "create", key_buf, sizeof(key_buf)) == 0 && key_buf[0] != '\0') {
        key_create = key_buf[0];
    }
    if (extract_keybinding(json, "delete", key_buf, sizeof(key_buf)) == 0 && key_buf[0] != '\0') {
        key_delete = key_buf[0];
    }
}