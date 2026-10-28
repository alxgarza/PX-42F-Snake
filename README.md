# Snake for the PX-42F

The classic game, as a native app for the PX-42F calculator. It's built on the
PX-42F App Template: the framework's files (`px42f/`, `sim/`) and its README
come from there.

## Playing

| Key | Does |
|---|---|
| ENTER | start, and play again after a game |
| Σ+ (top left) | turn left |
| XEQ (top right) | turn right |
| R/S | pause, and go on |
| ON | back to the menu (held 2 s: off) |

- **Turns.** They're relative to where the snake is heading. Two quick presses
  are both kept, so Σ+ Σ+ makes a tight U-turn.
- **No walls.** Off one edge, the snake comes back at the other.
- **Apples.** Each apple makes the snake 2 cells longer, with a short high tone.
  The tone plays on the calculator's second processor, so the game doesn't wait for it.
- **Levels.** Every 5 apples is a level: the snake gets faster, and one more
  apple is out at a time (up to 6).
- **Game over.** Running into itself ends the game. The best score is kept on
  the calculator, in `/BIN/GAMES/SNAKE/BEST`.

## Build and install

It needs the PX-42F-Plus firmware with app API version 2 or later.


- `make`: builds `build/SNAKE.bin`.
- `make sim`: plays a scripted game on your computer. Screenshots are saved in
  `build/sim-out`.
- `make install`: copies the game to the calculator. First open Settings → USB
  Drive → Connect to Computer.

To install by hand, copy `build/SNAKE.bin` to `BIN/GAMES` on the `PX-42F` drive.
The game then appears under Settings → Games & Apps → Games.

The VS Code tasks (Terminal → Run Task) run the same commands.

## Tuning

The numbers at the top of `src/snake.c`:
- `START_TICK_MS`, `TICK_STEP_MS`, `FASTEST_MS`: speed;
- `GROWTH`: cells an apple adds;
- `APPLES_A_LEVEL`: apples to the next level;
- `MAX_APPLES`: the most apples out at once.
