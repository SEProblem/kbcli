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
        /* Render the board (shows current state) */
        render_board(&global_board);
        
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
    
    /* Initialize ncurses */
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    
    /* Reduce escape key delay for vim feel */
    set_escdelay(25);
    
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