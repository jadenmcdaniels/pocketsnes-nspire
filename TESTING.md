Testing without a calculator
============================

`make` builds `pocketsnes-host`: the same core, menu, save states and frame
pacing as the calculator build, with `frontend/platform_host.cpp` standing in
for the calculator's screen, keys and timer. What it can't tell you is how fast
the calculator is, or anything about its screen and keypad hardware.

Playing it in a window
----------------------

    make
    ./pocketsnes-host                      # starts in the game list
    ./pocketsnes-host path/to/game.sfc     # or straight into a game

Settings and the game list start from the program's folder (`--exe-dir DIR`
picks another one). Plain `.sfc`/`.smc` files work as well as `.tns` ones.
Calculator keys on the PC keyboard:

| PC key                 | Calculator key  | Default use           |
|------------------------|-----------------|-----------------------|
| Ctrl / Shift           | ctrl / shift    | SNES A / B            |
| F2 / Backspace         | var / del       | SNES X / Y            |
| Tab / F1               | tab / menu      | SNES L / R            |
| Enter / -              | enter / -       | SNES Start / Select   |
| arrows, 8 5 4 6        | same            | d-pad                 |
| Esc                    | esc             | menu                  |
| Q / F / S / L          | same            | quit / fast forward / save / load |

F3 is doc, F4 scratchpad, F5 cat, Home the on key; letters and digits are
themselves.

Scripted runs
-------------

`--script FILE` runs without a window, drives the keys from FILE and uses
virtual time, so every run is the same. `--shots DIR` is where `shot`
screenshots go. The commands are listed at the top of
`frontend/platform_host.cpp`; for example:

    wait 120
    press esc
    wait 5
    shot menu.png
    press S 2
    quit

`PSNES_TRACE=1` prints the keys seen at every poll, which helps when a script
doesn't do what you expect.

Two more options are for the tests: `--log FILE` appends a line to FILE for
every CPU clock change asked for (`clock x40`) and every message shown in the
game (`message: CPU 480 MHz`), and `--stuck-clock` acts like a calculator
whose clock doesn't change when asked (the PC build otherwise reports the
clock asked for as the one it got).

The test suite
--------------

    tests/run.sh

Downloads a few free homebrew demos (PeterLemon's SNES test programs) into
`tests/roms`, then checks that loading a state gives back exactly the same
game (pixel for pixel, 150 frames later), auto-increment, saving when leaving
and loading on start, old settings files, key combinations, settings for one
game, the CPU speed setting (the menu's choice reaches the clock when the
game resumes; a clock that doesn't move goes back to normal and says so; a
freeze at a raised speed is undone at the next start), that in-game saves
are written without quitting, the menu, the welcome screen on the first
start, the game list and its menu (number keys starting games, moving a game
with tab or the menu, the order kept, esc putting it back, sorting A to Z
again). Results and screenshots go to
`tests/out/`. Every test starts with a settings file (so not on a "first
start") unless it removes it.

Unit tests
----------

    make unit-tests

`tests/unit_tests.cpp`: the tile drawers against the original Snes9x 1.43
ones on 20,000 random tiles (flips, clipping, depths, line ranges), the
frame turning for the CX II LCD, the settings files (defaults, old files,
key combinations, settings for one game, the CPU speed reset after a
freeze) and the key bindings. tests/run.sh runs them first.

Drawing the same pixels as before
---------------------------------

    tests/render_compare.sh [ROM...]

Plays each ROM with the same keys in `./pocketsnes-host-ref` (a PC build of
the renderer before the speed work of October 2026; keep it) and in the
current build, with a screenshot every 2 to 4 frames, and checks they are
identical. Without arguments it uses the test ROMs, Super Mario World with
its save state, and the games in `~/nspire/build/testroms/calc` (with their
saves). The reference build crashes in Chrono Trigger's intro (a 64-bit PC
bug, see FINDINGS.md), so that game only compares up to there.
