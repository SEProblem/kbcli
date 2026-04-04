/**
 * config.h - Configuration management for Kanban CLI
 * 
 * Provides configuration loading from JSON config file,
 * keybindings customization, and default board directory.
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <stddef.h>
#include <ncurses.h>

/* Maximum path length */
#define MAX_PATH 512

/* Maximum name length */
#define MAX_NAME 64

/* Default keybindings per D-08: full replacement */
#define DEFAULT_KEYBINDINGS "{\"up\":\"k\",\"down\":\"j\",\"left\":\"h\",\"right\":\"l\",\"create\":\"n\",\"delete\":\"d\"}"
#define DEFAULT_KEY_UP 'k'
#define DEFAULT_KEY_DOWN 'j'
#define DEFAULT_KEY_LEFT 'h'
#define DEFAULT_KEY_RIGHT 'l'
#define DEFAULT_KEY_CREATE 'n'
#define DEFAULT_KEY_DELETE 'd'

/**
 * Config structure holding application configuration
 */
typedef struct {
    char board_directory[MAX_PATH];    /* Configured boards directory */
    int auto_save;                     /* Auto-save on changes */
    char keybindings[256];             /* JSON string of keybindings */
    char default_board[MAX_NAME];      /* Default board name */
} Config;

/**
 * Get the config file path
 * Returns ~/.config/kanban-cli/config
 * 
 * @param buffer Buffer to store the path
 * @param size Size of the buffer
 * @return 0 on success, -1 on error
 */
int get_config_path(char *buffer, size_t size);

/**
 * Load configuration from JSON config file
 * Applies defaults if config file is missing or corrupt
 * 
 * @param config Config structure to populate
 * @return 0 on success, -1 on error
 */
int config_load(Config *config);

/**
 * Save configuration to JSON config file
 * Uses atomic write pattern (temp file + fsync + rename)
 * 
 * @param config Config structure to save
 * @return 0 on success, -1 on error
 */
int config_save(const Config *config);

/**
 * Get the configured board directory
 * Returns configured path or default if not set
 * 
 * @param config Pointer to Config structure
 * @return Pointer to board directory string (do not free)
 */
const char* config_get_board_directory(const Config *config);

/**
 * Apply keybindings from JSON config string
 * Parses JSON keybindings object and updates global key mappings
 * 
 * @param json JSON string containing keybindings object
 */
void apply_keybindings_from_config(const char *json);

/**
 * Get the default keybindings JSON string
 * 
 * @return Pointer to default keybindings string
 */
const char* config_get_default_keybindings(void);

/**
 * Initialize config with defaults
 * 
 * @param config Config structure to initialize
 */
void config_init_defaults(Config *config);

/* External key mappings - used by input handler */
extern char key_up;
extern char key_down;
extern char key_left;
extern char key_right;
extern char key_create;
extern char key_delete;

#endif /* CONFIG_H */