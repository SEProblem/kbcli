/**
 * main.c - Application entry point for Kanban CLI
 * 
 * Initializes ncurses TUI and loads/displays the kanban board.
 */

#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <ncurses.h>

#include "kanban.h"
#include "models.h"
#include "storage.h"

/* Global board instance */
static Board global_board;

/* Signal handler for graceful exit */
static void handle_signal(int sig) {
    endwin();
    exit(sig);
}

/* Cleanup function registered with atexit() */
static void cleanup(void) {
    endwin();
}

/* Draw a simple board placeholder */
static void draw_board(void) {
    int height, width;
    getmaxyx(stdscr, height, width);
    
    /* Draw column headers */
    int col_width = width / 3;
    
    for (int i = 0; i < 3; i++) {
        int x = i * col_width;
        
        /* Draw column box */
        box(stdscr, 0, 0);
        
        /* Draw column name */
        mvprintw(2, x + 2, "%s", global_board.columns[i].name);
        
        /* Draw tasks */
        Task *task = global_board.columns[i].tasks;
        int y = 4;
        
        while (task != NULL) {
            mvprintw(y, x + 2, "- [%c] %s", 
                     task->completed ? 'x' : ' ',
                     task->title);
            task = task->next;
            y++;
        }
    }
    
    /* Draw status bar at bottom */
    mvhline(height - 2, 0, 0, width);
    mvprintw(height - 1, 0, "Press 'q' to quit");
    
    refresh();
}

/* Main input loop - placeholder for phase 2 */
static void input_loop(void) {
    int ch;
    
    while ((ch = getch()) != 'q') {
        /* Placeholder: just refresh */
        draw_board();
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
    
    /* Initialize board */
    board_init(&global_board);
    
    /* Load or create board */
    char filepath[512];
    if (get_default_board_path(filepath, sizeof(filepath)) == 0) {
        board_load(&global_board, filepath);
    }
    
    /* Draw initial board */
    draw_board();
    
    /* Enter input loop */
    input_loop();
    
    /* Save before exit */
    if (global_board.filename[0] != '\0') {
        board_save(&global_board, global_board.filename);
    }
    
    /* Cleanup */
    board_free(&global_board);
    endwin();
    
    return 0;
}