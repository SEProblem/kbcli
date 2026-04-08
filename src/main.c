/**
 * main.c - Application entry point for Kanban CLI
 * 
 * Initializes ncurses TUI and loads/displays the kanban board.
 * Integrates renderer and input modules for full functionality.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <ncurses.h>

#include "kanban.h"
#include "models.h"
#include "storage.h"
#include "renderer.h"
#include "input.h"
#include "config.h"

/* Global board instance */
static Board global_board;

/* Global selection state - tracks current task selection */
static Selection current_selection = {0, 0};

/* Restore the terminal cursor style we may have changed via DECSCUSR. */
static void restore_cursor(void) {
    fputs("\033[0 q", stdout);
    fflush(stdout);
}

/* Signal handler for graceful exit */
static void handle_signal(int sig) {
    endwin();
    restore_cursor();
    exit(sig);
}

/* Cleanup function registered with atexit() */
static void cleanup(void) {
    endwin();
    restore_cursor();
}

/* Main event loop integrating renderer and input */
static void event_loop(void) {
    int running = 1;
    
    while (running) {
        /* Render based on current mode */
        if (global_board.app_mode == MODE_HELP) {
            render_board(&global_board, &current_selection);
            render_help_popup();
        } else if (global_board.app_mode == MODE_CARD_POPUP) {
            render_card_popup(&global_board, &current_selection, card_popup_active_field());
        } else {
            /* Render the board (shows current state) */
            render_board(&global_board, &current_selection);
        }
        
        /* Get user input */
        int key = getch();
        
        /* Handle input - returns 1 if quit requested */
        running = !handle_input(&global_board, key, &current_selection);
    }
}

int main(int argc, char *argv[]) {
    (void)argc;  /* Unused */
    (void)argv;  /* Unused */
    
    /* Seed random for UUID generation */
    srand(time(NULL));
    
    /* Register cleanup handlers */
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    atexit(cleanup);
    
    /* Load configuration and apply keybindings */
    Config app_config;
    config_load(&app_config);
    apply_keybindings_from_config(app_config.keybindings);
    
    /* Initialize ncurses */
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    
    /* Reduce escape key delay for vim feel */
    set_escdelay(25);

    /* Hide the hardware cursor by default. The card popup re-enables it
     * (curs_set(1)) and parks it on the active text field while editing,
     * so the user can see exactly where their next keystroke will land. */
    curs_set(0);
    
    /* Enable mouse support - NAV-05, NAV-06 per D-09 */
    mousemask(ALL_MOUSE_EVENTS, NULL);
    mouseinterval(166);  /* double-click detection window in ms - NAV-05 */
    
    /* Initialize renderer state */
    renderer_init();
    renderer_init_colors();
    
    /* Initialize board */
    board_init(&global_board);
    
    /* Load or create board. Use the config's default_board (persisted across
     * launches by :b/:brename/:blist) so we reopen whatever the user last had
     * open, not always "default". */
    {
        Config startup_cfg;
        config_load(&startup_cfg);
        const char *board_dir = config_get_board_directory(&startup_cfg);
        const char *board_name = (startup_cfg.default_board[0] != '\0')
                                 ? startup_cfg.default_board : "default";

        char filepath[512];
        if (board_dir != NULL && board_dir[0] != '\0') {
            snprintf(filepath, sizeof(filepath), "%s%s.md", board_dir, board_name);
        } else if (get_default_board_path(filepath, sizeof(filepath)) != 0) {
            filepath[0] = '\0';
        }

        if (filepath[0] != '\0') {
            board_load(&global_board, filepath);
        }

        /* Sync the in-memory current-board name so the status bar reflects
         * what we actually loaded, not the hardcoded "default". */
        extern char global_current_board_name[256];
        strncpy(global_current_board_name, board_name,
                sizeof(global_current_board_name) - 1);
        global_current_board_name[sizeof(global_current_board_name) - 1] = '\0';
    }

    /* First-run welcome: open the help overlay automatically and drop a
     * sentinel file so we never do it again. */
    {
        const char *home = getenv("HOME");
        if (home != NULL) {
            char dir[512];
            char sentinel[600];
            snprintf(dir, sizeof(dir), "%s/.config/kanban-cli", home);
            snprintf(sentinel, sizeof(sentinel), "%s/.welcomed", dir);
            struct stat st;
            if (stat(sentinel, &st) != 0) {
                /* Best-effort mkdir; ignore failure (config_load already ran) */
                mkdir(dir, 0755);
                FILE *f = fopen(sentinel, "w");
                if (f) { fputs("1\n", f); fclose(f); }
                global_board.app_mode = MODE_HELP;
            }
        }
    }

    /* Enter event loop - handles all rendering and input */
    event_loop();
    
    /* Save before exit (auto-save is handled in handle_input, but do final save) */
    if (global_board.filename[0] != '\0') {
        board_save(&global_board, global_board.filename);
    }
    
    /* Cleanup */
    board_free(&global_board);
    endwin();
    
    return 0;
}