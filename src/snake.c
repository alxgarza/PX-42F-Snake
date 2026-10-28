/* Snake for the PX-42F. The snake turns left with the top-left key (Sigma+)
 * and right with the top-right key (XEQ), relative to where it's heading.
 * No walls: off one edge, on at the other. Apples make it longer; every
 * few apples it gets faster and more apples are out at once. Running into
 * itself ends the game. R/S pauses, ON goes back to the menu. The best
 * score is kept in the game's folder (/BIN/GAMES/SNAKE/BEST). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "px.h"

/* The field: cells of 5 x 5 pixels under a 12-pixel status bar. */
#define CELL    5
#define COLS    (PX_SCREEN_W / CELL)          /* 50 */
#define FIELD_Y 14
#define ROWS    ((PX_SCREEN_H - FIELD_Y) / CELL) /* 21 */
#define CELLS   (COLS * ROWS)

#define START_LENGTH  4
#define GROWTH        2   /* cells an apple adds */
#define APPLES_A_LEVEL 5  /* eaten to reach the next level */
#define START_TICK_MS 180 /* a step at level 0... */
#define TICK_STEP_MS  12  /* ...this much faster each level... */
#define FASTEST_MS    70  /* ...down to this */
#define MAX_APPLES    6   /* out at once: 1 at level 0, one more each level */

enum { EMPTY, BODY, APPLE };

/* Right, down, left, up: a right turn is +1, a left turn -1. */
static const int step_x[4] = {1, 0, -1, 0};
static const int step_y[4] = {0, 1, 0, -1};

static struct {
    uint8_t grid[ROWS][COLS];
    uint16_t body[CELLS]; /* ring buffer of cells (y * COLS + x), tail to head */
    int tail, length;     /* body[tail] is the tail; the head is length - 1 further */
    int heading;
    int turns[2], turn_count; /* pressed but not yet taken: one a step */
    int grow;                 /* cells still to add */
    int score, best, apples_out;
} g;

static const uint8_t apple_bits[CELL] = {0x20, 0x70, 0xF8, 0xF8, 0x70}; /* 5 x 5, top bit leftmost */

static int level(void) {
    return g.score / APPLES_A_LEVEL;
}

static int tick_ms(void) {
    int t = START_TICK_MS - TICK_STEP_MS * level();
    return t < FASTEST_MS ? FASTEST_MS : t;
}

static int apples_wanted(void) {
    int n = 1 + level();
    return n > MAX_APPLES ? MAX_APPLES : n;
}

static int head(void) {
    return g.body[(g.tail + g.length - 1) % CELLS];
}

static int load_best(void) {
    char text[12] = "";
    int n = px->load("BEST", text, sizeof(text) - 1);
    return n > 0 ? atoi(text) : 0;
}

static void save_best(int best) {
    char text[12];
    int n = snprintf(text, sizeof(text), "%d\n", best);
    px->save("BEST", text, n);
}

/* An apple on a random empty cell (a cell is nearly always free: if the
 * snake fills the field, there's simply no new apple). */
static void place_apple(void) {
    for (int tries = 0; tries < 200; tries++) {
        int c = (int) (px->random() % CELLS);
        if (g.grid[c / COLS][c % COLS] == EMPTY) {
            g.grid[c / COLS][c % COLS] = APPLE;
            g.apples_out++;
            return;
        }
    }
}

static void new_game(void) {
    memset(g.grid, EMPTY, sizeof(g.grid));
    g.tail = 0;
    g.length = START_LENGTH;
    for (int i = 0; i < START_LENGTH; i++) {
        int x = COLS / 2 - START_LENGTH + i, y = ROWS / 2;
        g.body[i] = (uint16_t) (y * COLS + x);
        g.grid[y][x] = BODY;
    }
    g.heading = 0;
    g.turn_count = 0;
    g.grow = 0;
    g.score = 0;
    g.apples_out = 0;
    place_apple();
}

static void turn(int by) {
    if (g.turn_count < 2)
        g.turns[g.turn_count++] = by;
}

/* One step. False when the snake has run into itself. */
static bool step(void) {
    if (g.turn_count > 0) {
        g.heading = (g.heading + g.turns[0] + 4) % 4;
        g.turns[0] = g.turns[1];
        g.turn_count--;
    }
    int h = head();
    int x = (h % COLS + step_x[g.heading] + COLS) % COLS;
    int y = (h / COLS + step_y[g.heading] + ROWS) % ROWS;

    /* The tail moves out of the way first, unless the snake is growing. */
    if (g.grow == 0) {
        int t = g.body[g.tail];
        g.grid[t / COLS][t % COLS] = EMPTY;
        g.tail = (g.tail + 1) % CELLS;
        g.length--;
    } else {
        g.grow--;
    }
    if (g.grid[y][x] == BODY)
        return false;
    if (g.grid[y][x] == APPLE) {
        px->tone_async(9); /* a short high tone; the game doesn't wait for it */
        g.score++;
        g.grow += GROWTH;
        g.apples_out--;
    }
    g.grid[y][x] = BODY;
    g.body[(g.tail + g.length) % CELLS] = (uint16_t) (y * COLS + x);
    g.length++;
    while (g.apples_out < apples_wanted() && g.length + g.apples_out < CELLS)
        place_apple();
    return true;
}

static void draw_status(void) {
    char line[40];
    px->fill(0, 0, PX_SCREEN_W, 12, 1);
    px->text(2, 0, "SNAKE", PX_FONT_SMALL, 1);
    snprintf(line, sizeof(line), "Score %d  Best %d", g.score, g.best);
    px->text(PX_SCREEN_W - 2 - px->text_width(line, PX_FONT_SMALL), 0, line, PX_FONT_SMALL, 1);
}

static void draw_field(void) {
    px->clear();
    draw_status();
    int h = head();
    for (int y = 0; y < ROWS; y++)
        for (int x = 0; x < COLS; x++) {
            int px0 = x * CELL, py0 = FIELD_Y + y * CELL;
            if (g.grid[y][x] == APPLE)
                px->bitmap(px0, py0, CELL, CELL, apple_bits, 1);
            else if (g.grid[y][x] == BODY && y * COLS + x == h)
                px->fill(px0, py0, CELL, CELL, 1); /* the head: a full cell */
            else if (g.grid[y][x] == BODY)
                px->fill(px0, py0, CELL - 1, CELL - 1, 1);
        }
}

/* A box in the middle of the screen, over the field. */
static void draw_box(const char *title, const char *line1, const char *line2) {
    int w = 170, h = 58, x = (PX_SCREEN_W - w) / 2, y = FIELD_Y + (ROWS * CELL - h) / 2;
    px->fill(x - 2, y - 2, w + 4, h + 4, 0);
    px->fill(x, y, w, h, 1);
    px->fill(x + 2, y + 2, w - 4, h - 4, 0);
    px->text(x + (w - px->text_width(title, PX_FONT_LARGE)) / 2, y + 6, title, PX_FONT_LARGE, 0);
    px->text(x + (w - px->text_width(line1, PX_FONT_SMALL)) / 2, y + 26, line1, PX_FONT_SMALL, 0);
    px->text(x + (w - px->text_width(line2, PX_FONT_SMALL)) / 2, y + 40, line2, PX_FONT_SMALL, 0);
}

/* The next key going down, waiting up to wait_ms; releases are skipped. */
static int key_down(int wait_ms) {
    uint32_t until = px->millis() + (uint32_t) wait_ms;
    while (1) {
        int32_t left = wait_ms < 0 ? -1 : (int32_t) (until - px->millis());
        int key = px->key(wait_ms < 0 ? -1 : left > 0 ? (int) left : 0);
        if (key == PX_KEY_NONE || key == PX_KEY_QUIT || !(key & PX_KEY_RELEASED))
            return key;
    }
}

/* Waits for ENTER (or R/S): false if the app is to end. */
static bool wait_for_start(void) {
    while (1) {
        int key = key_down(-1);
        if (key == PX_KEY_QUIT)
            return false;
        if (key == PX_KEY_ENTER || key == PX_KEY_RS)
            return true;
    }
}

static bool title_screen(void) {
    char best[24];
    px->clear();
    draw_status();
    px->text((PX_SCREEN_W - px->text_width("SNAKE", PX_FONT_LARGE)) / 2, 20, "SNAKE", PX_FONT_LARGE, 0);
    px->text(16, 44, "\x05+  turns left", PX_FONT_SMALL, 0); /* code 5: Sigma */
    px->text(16, 57, "XEQ turns right", PX_FONT_SMALL, 0);
    px->text(16, 70, "R/S pause   ON quit", PX_FONT_SMALL, 0);
    snprintf(best, sizeof(best), "Best: %d", g.best);
    px->text(16, 88, best, PX_FONT_SMALL, 0);
    px->text(16, 106, "ENTER to start", PX_FONT_SMALL, 1);
    px->show();
    return wait_for_start();
}

/* Paused until R/S again: false if the app is to end. */
static bool pause(void) {
    draw_box("PAUSED", "R/S to go on", "ON to quit");
    px->show();
    while (1) {
        int key = key_down(-1);
        if (key == PX_KEY_QUIT)
            return false;
        if (key == PX_KEY_RS)
            return true;
    }
}

/* One game. False if the app is to end. */
static bool play(void) {
    new_game();
    draw_field();
    px->show();
    uint32_t next_step = px->millis() + (uint32_t) tick_ms();
    while (1) {
        int32_t left = (int32_t) (next_step - px->millis());
        int key = key_down(left > 0 ? (int) left : 0);
        if (key == PX_KEY_QUIT)
            return false;
        if (key == PX_KEY_SIGMA)
            turn(-1);
        else if (key == PX_KEY_XEQ)
            turn(+1);
        else if (key == PX_KEY_RS) {
            if (!pause())
                return false;
            next_step = px->millis() + (uint32_t) tick_ms();
            draw_field();
            px->show();
        }
        if (key != PX_KEY_NONE)
            continue;

        if (!step())
            break;
        draw_field();
        px->show();
        next_step += (uint32_t) tick_ms();
    }

    /* Game over. */
    bool record = g.score > g.best;
    if (record) {
        g.best = g.score;
        save_best(g.best);
    }
    draw_field();
    char line[24];
    snprintf(line, sizeof(line), record ? "New best: %d!" : "Score: %d", g.score);
    draw_box("GAME OVER", line, "ENTER: again");
    px->show();
    px->tone(10);
    return wait_for_start();
}

int app_main(void) {
    g.best = load_best();
    if (!title_screen())
        return PX_EXIT_MENU;
    while (play())
        ;
    return PX_EXIT_MENU;
}
