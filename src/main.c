/**
 * main.c - Application entry point for Kanban CLI
 * 
 * Initializes ncurses TUI and loads/displays the kanban board.
 * Integrates renderer and input modules for full functionality.
 */

#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
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

/* Signal handler for graceful exit */
static void handle_signal(int sig) {
    endwin();
    exit(sig);
}

/* Cleanup function registered with atexit() */
static void cleanup(void) {
    endwin();
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
            /* Render card popup (T02 will wire the real active_field accessor) */
            render_card_popup(&global_board, &current_selection, 0);
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
    
    /* Enable mouse support - NAV-05, NAV-06 per D-09 */
    mousemask(ALL_MOUSE_EVENTS, NULL);
    
    /* Initialize renderer state */
    renderer_init();
    
    /* Initialize board */
    board_init(&global_board);
    
    /* Load or create board */
    char filepath[512];
    if (get_default_board_path(filepath, sizeof(filepath)) == 0) {
        board_load(&global_board, filepath);
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