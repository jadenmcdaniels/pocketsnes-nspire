PocketSNES for TI-Nspire
========================

A Super Nintendo emulator for the TI-Nspire CX and CX II calculators (it
runs under [Ndless](https://ndless.me)). It is gameblabla's PocketSNES port
of Snes9x 1.43 with a new front end: a menu like gpSP's, save states with
999 slots, settings per game, a game list, fast tear-free drawing on the
CX II, and an optional higher CPU speed.

There is no sound: the calculator has no speaker.

* [Quick start](#quick-start)
* [Buttons](#buttons)
* [The menu, setting by setting](#the-menu-setting-by-setting)
* [Saving](#saving)
* [Speed](#speed)
* [What runs and what doesn't](#what-runs-and-what-doesnt)
* [Troubleshooting](#troubleshooting)
* [For developers](#for-developers)
* [Credits and license](#credits-and-license)


Quick start
-----------

1. **Install Ndless** on the calculator (see [ndless.me](https://ndless.me)
   for your OS version). On a CX II, Ndless has to be switched on again
   after every reset: open the Ndless installer document.
2. **Copy `pocketsnes.tns`** to the calculator, in any folder (for example
   a folder called `snes`), with TI-Nspire Computer Link / Student Software,
   TiLP or similar. Keep the name `pocketsnes.tns`: that lets ROMs open in it
   straight from the calculator's documents (step 5).
3. **Copy your ROMs**, renamed so they end in `.tns`: the calculator only
   takes `.tns` files. `Super Mario World.sfc` becomes
   `Super Mario World.sfc.tns`. Any folder works.
4. **Open `pocketsnes.tns`** on the calculator. The first time, a welcome
   screen shows the keys. Then the game list opens: pick a game and press
   enter.
5. After the first run, opening a ROM in the calculator's documents starts
   PocketSNES with it (Ndless file association for `.sfc`, `.smc`, `.fig`
   and `.swc`).

Leaving a game with Q (or Exit in the menu) saves where you were, and the
game picks up there next time. See [Saving](#saving).


Buttons
-------

These are the defaults. Every one can be changed (esc, then Configure SNES
buttons / Configure hotkeys), and each action has room for two keys or key
combinations. *Controls and about* in the menu always shows the keys as they
are set.

| SNES          | Calculator                                   |
|---------------|----------------------------------------------|
| D-pad         | arrows, or 8 4 6 5 (7 9 1 3 for diagonals)   |
| A / B         | ctrl / shift                                 |
| X / Y         | var / del                                    |
| L / R         | tab / menu                                   |
| Start / Select| enter / minus (-)                            |

| Hotkey        | Default | Does                                              |
|---------------|---------|---------------------------------------------------|
| Menu          | esc     | opens the menu (the game is paused meanwhile)     |
| Quit          | Q       | leaves the game; saves a state first (unless that's switched off) |
| Fast forward  | F       | on/off: runs as fast as it can                    |
| Save state    | S       | saves (to a new slot, with auto-increment on)     |
| Load state    | L       | loads the selected slot                           |
| Load newest   | none    | loads the newest state                            |
| Next / previous slot | none | picks the slot S and L use                   |
| FPS counter   | none    | shows/hides the FPS counter                       |
| Restart game  | none    | resets the SNES                                   |

On touchpad calculators, the touchpad's arrows work as the d-pad.

**In menus and the game list:** arrows (or 8 5 4 6) move, enter or click
selects, esc goes back, left/right change a setting. In the game list,
left/right turn a page and the menu key opens the settings.


The menu, setting by setting
----------------------------

Press **esc** in a game. Settings can be **for all games** or **for this game
only** (*Settings for* and *Keys for* in the menu). A game's own settings
are used whenever that game runs. The game list has the same settings: press
**menu** on a game for that game's settings (and to start it without loading
a state), or on a folder for the settings for all games.

### Graphics/performance

| Setting | Choices (default first) | What it does |
|---|---|---|
| Frameskip type | **automatic**, manual, off | Automatic skips drawing a frame only when the game falls behind, so the game itself always runs at full speed. Manual always draws 1 frame in N+1. Off draws every frame (smoothest, but the game slows down when drawing can't keep up). |
| Frameskip value | **5** (0-9) | Automatic: the most frames skipped in a row. Manual: N. |
| Show FPS counter | **off**, on, detailed | Frames drawn per second / the most there can be, e.g. 60/60 is full speed and 49/60 means some frames weren't drawn. Detailed adds how long each part takes (game, drawing, output) and logs it to `pocketsnes_perf.txt.tns`. |
| Screen output (CX II) | **no tearing**, DMA | No tearing: every frame shows whole. DMA: the DMA chip copies frames, which is a little faster, but fast scrolling can show a split line. |
| CPU speed (CX II) | **normal**, 432, 456, 480 MHz | Raises the CPU clock while a game runs (menus and saving stay at normal speed). Normal is 396 MHz, or 288 MHz while USB is plugged in. The game shows the speed it got when it starts. Uses more battery. If a speed freezes the calculator, the next start goes back to normal and says so. **Experimental:** on some calculators the clock doesn't change yet; the message then says "CPU speed not raised". |
| Run speed test | | In a game only: runs it from where you are in five ways for about 35 seconds and shows frames per second and where the time goes. The game is put back where it was. Results are also written to `pocketsnes_results.txt.tns`. |

### Save state options

| Setting | Default | What it does |
|---|---|---|
| Auto-increment slot on save | on | Every save goes to a new slot after your highest one, so nothing is ever overwritten. Off: saves go to the selected slot. |
| Save a state when leaving | on | Q, Exit or Load new game save a state first. |
| Load newest state on start | on | Starting a game loads its newest state. To start without it once, press menu on the game in the game list and pick *Start without loading a state*. |
| Write in-game saves | automatically | When the save you make inside the game (its battery save) is written to the calculator: a few seconds after the game saves, or only when you leave. |

### The rest of the in-game menu

* **Configure SNES buttons / Configure hotkeys**: left/right pick key 1 or
  2, enter sets it (press a key, or hold one key and press another for a
  combination), del clears it. *Reset to defaults* is at the bottom.
* **Load state from slot / Save state to slot**: left/right pick the slot
  (hold to go faster).
* **Settings for / Keys for**: all games, or this game only.
* **Load new game**, **Restart game**, **Return to game**, **Controls and
  about** (the keys as set now), **Exit PocketSNES**.


Saving
------

There are two kinds of saves:

* **In-game saves** (the game's own save, its "battery save") work as on a
  real SNES. They're written to the calculator a few seconds after the game
  saves.
* **Save states** freeze the whole game at any moment: S saves, L loads,
  and leaving a game saves one so you can carry on next time. There are 999
  slots per game. With auto-increment on (the default), each save goes to a
  new slot, so an old save is never overwritten. Each one takes a few KB to
  a few dozen KB.

Everything goes in a `.pocketsnes` folder next to the ROM:

| File | What |
|---|---|
| `game.sfc.srm.tns` | the in-game save |
| `game.sfc.sv001.tns` ... `game.sfc.sv999.tns` | save states (compressed) |
| `game.sfc.cfg.tns` | the game's own settings or keys, if it has any |

Next to `pocketsnes.tns`:

| File | What |
|---|---|
| `pocketsnes.cfg.tns` | settings and keys for all games (delete it to start fresh) |
| `pocketsnes_results.txt.tns` | the last speed test's results |
| `pocketsnes_perf.txt.tns` | the detailed FPS log, if you turned it on |
| `pocketsnes_clock.tns` | only while a game runs at a raised CPU speed: if it's still there at the next start, that speed froze the calculator |


Speed
-----

On a CX II at its normal 396 MHz, Super Mario World runs at a full 60
frames per second with every frame drawn; without the speed limit it would
reach about 66. Games with extra chips (Super FX such as Yoshi's Island and
Star Fox, or SA-1) are much heavier.

Tips:

* Keep **frameskip automatic** (the default): the game keeps full speed and
  only skips drawing when it has to. Use **off** only for games that
  already reach 60/60.
* Turn on **Show FPS counter** to see how a game does: 60/60 is perfect.
* On a CX II, **Screen output: DMA** gains a few percent, at the cost of
  tearing.
* Leave the calculator unplugged while playing: on USB the OS runs the CPU
  at 288 MHz instead of 396.

### What made it faster

Measured on a CX II at 396 MHz, Super Mario World, every frame drawn:

| Version | Frames per second |
|---|---:|
| The original port (lcd_blit, two copies of each frame) | 40 |
| The game drawn straight into the frame, -O3 | 48 |
| Frames handed to the LCD instead of copied ("flip") | 49 |
| Frames copied by the DMA chip | 50 (no speed limit) |
| Faster tile drawing and a shortcut for SMW's backdrop | 59 (no speed limit) |
| Only the 256x224 picture turned for the portrait LCD, tear-free (now) | 66 (no speed limit) |

Where a frame's time went, before and now (ms, no speed limit): running the
game 5.7 → 6.0, drawing 11.9 → 7.2, getting it on screen 2.7 → 1.8.

The changes, roughly in order (details and measurements in
[FINDINGS.md](FINDINGS.md)):

* **Drawing straight into the frame.** The original drew into one buffer,
  copied it to another, and had Ndless's `lcd_blit` copy it again.
* **The CX II's screen is portrait.** Its LCD reads 320 lines of 240
  pixels, so a landscape frame has to be turned. Ndless's `lcd_blit` does
  this through the OS's special LCD memory, which is slow (4 ms a frame).
  PocketSNES turns only the 256x224 SNES picture (and the text over it) into
  one of three buffers of its own, in 8-line strips written with burst
  stores, and points the LCD at that buffer. The LCD switches buffers only
  between refreshes, so there is no tearing.
* **DMA output** (optional): the calculator's DMA controller copies frames
  into the OS's LCD memory while the next frame is drawn.
* **Tile drawing**: `DrawTile16` / `DrawClippedTile16` keep the renderer's
  state in registers instead of reloading it for every pixel (14 → 9
  instructions per pixel), and skip transparent tile lines in one test.
* **Backdrop shortcut**: games like Super Mario World put their background
  on the sub screen with colour maths against a black backdrop; that is now
  drawn directly instead of being checked pixel by pixel afterwards.
* **-O3** beat -O2 and -Os.
* **Timer fix** (2026-10-05): the frame timer's rate used to be "calibrated"
  while ROMs loaded, which could make games run 5-20% too slow while the FPS
  counter still showed 60. It's now fixed at the calculator's 32768 Hz.

Everything draws exactly the same pixels as the original renderer
(`tests/render_compare.sh` checks that on 16 games and demos).


What runs and what doesn't
--------------------------

* Most SNES games run, including ones with DSP-1, DSP-2, Super FX, SA-1,
  C4, OBC1 and Seta chips (the heavy ones slowly).
* **S-DD1 and SPC7110 games don't run**: Star Ocean and Street Fighter
  Alpha 2 (S-DD1; Snes9x 1.43 needs graphics packs for them), Far East of
  Eden Zero and Momotarou Dentetsu Happy (SPC7110).
* There is no sound emulation at all, which also saves time.
* Yoshi's Island shows glitches in the top rows of the picture (from the
  emulator core; the original port has them too).
* ROMs can be up to 8 MB, as long as the calculator has the memory free.
* Calculators without a colour screen aren't supported.


Troubleshooting
---------------

* **My ROM isn't in the game list.** Its name must end in `.tns`
  (`game.sfc.tns`). Use `..` at the top of the list to go to other folders.
* **"Couldn't load ..."**: the file isn't a SNES ROM (or is damaged), or
  it's too big.
* **The game is slow.** Set frameskip to automatic, unplug USB, and look at
  the FPS counter. A raised CPU speed can help.
* **The calculator froze.** Press the reset button on the back (a paper
  clip), then open the Ndless installer again. If it froze at a raised CPU
  speed, PocketSNES puts the speed back to normal at the next start.
* **A save state is broken.** In the game list, press menu on the game and
  choose *Start without loading a state*.
* **I want my settings back to the defaults.** Delete `pocketsnes.cfg.tns`
  next to `pocketsnes.tns` (and `.pocketsnes/game.sfc.cfg.tns` for one
  game's own settings).
* **Opening a ROM from the documents doesn't start PocketSNES.** The
  program has to be called `pocketsnes.tns` and have run once. The
  association lives in `/documents/ndless/ndless.cfg.tns` (`ext.sfc=pocketsnes`).


For developers
--------------

### Building

The calculator build needs the [Ndless SDK](https://github.com/ndless-nspire/Ndless)
toolchain on PATH (`nspire-g++`, `nspire-ld`, `genzehn`, `make-prg`):

    make -f Makefile.nspire              # makes pocketsnes.tns

Variants each need their own `BUILD` folder (make doesn't notice changed
flags):

    make -f Makefile.nspire OPT_LEVEL=-O2 BUILD=build/o2
    make -f Makefile.nspire BENCH=1 BUILD_NAME=mine BUILD=build/bench TARGET=pocketsnes_speedtest

`BENCH=1` makes a speed test build: it plays `Super Mario World (USA).sfc.tns`
(next to it) from its newest state for 5 seconds, writes
`pocketsnes_diag.txt.tns` (clock, timer, LCD and DMA registers, memory
mapping and speeds, a clock switch test), runs the speed test, writes
`pocketsnes_results.txt.tns` and closes.

The PC test build needs g++ and zlib (and SDL2's headers for a window):

    make                                 # makes ./pocketsnes-host
    ./pocketsnes-host                    # the game list, in a window
    tests/run.sh                         # the test suite (39 checks)

[TESTING.md](TESTING.md) explains the PC build, its script mode and the
tests.

### Source layout

| Path | What |
|---|---|
| `pocketsnes/snes9x/`, `pocketsnes/include/` | the Snes9x 1.43 core (65c816 CPU, PPU, renderer in `gfx.cpp` and `tile.cpp`, chips) |
| `frontend/main.cpp` | startup: welcome, game list, game, repeat |
| `frontend/emu.cpp` | runs the core: frame pacing and frameskip, hotkeys, FPS counter and messages, save states and in-game saves, the speed test, CPU speed changes |
| `frontend/menu.cpp` | the menus, built from tables of options; the controls page and the welcome screen |
| `frontend/browser.cpp` | the game list |
| `frontend/config.cpp` | settings and keys: the file for all games and each game's own |
| `frontend/states.cpp` | save state slots and file names |
| `frontend/gui.cpp`, `draw.cpp`, `font.h`, `keys.cpp` | menu input with key repeat, message screens, text drawing, key names and bindings |
| `frontend/rotate.cpp` | turning frames for the CX II's portrait LCD |
| `frontend/platform.h` | everything hardware: screen, keys, timer, CPU clock |
| `frontend/platform_nspire.cpp` | ... on the calculator (Ndless) |
| `frontend/platform_host.cpp` | ... on a PC, for testing (SDL2 window or scripted, in virtual time) |
| `tests/` | `run.sh`, its scripts, `unit_tests.cpp`, `render_compare.sh` |
| `FINDINGS.md` | what was measured and learned about the hardware |
| `menu/`, `sal/`, `sdl/`, `unused/` | the original port's front end, not built any more |

### How it works

**A frame.** The core draws the 256x224 picture straight into a 320x240
RGB565 buffer (`platform_screen()`, with spare lines around it because the
renderer writes whole 8x8 tiles). On a CX II the frontend then turns the
picture into one of three portrait buffers (`platform_frame_begin/copy/
present`, `rotate_block`) and writes that buffer's address to the LCD
controller (a PL111 at 0xC0000000); with DMA output, the DMA controller (an
FTDMAC020 at 0xBC000000) copies the landscape frame to the OS's LCD memory
at 0xA8000000 instead. Other calculators use Ndless's `lcd_blit`. Menus use
the same buffers, which take turns, so every screen redraws everything each
frame.

**Timing.** The first SP804 timer (0x900C0000) runs free at 32768 Hz;
`emu.cpp` paces frames in 16.16 fixed-point ticks (60 or 50 per second) and
decides per frame whether to draw it. Interrupts stay off while PocketSNES
runs.

**CPU speed.** The CX II's power controller (0x90140000) holds the clock
multiplier in bits 24-29 of 0x90140030 (12 MHz × 33 = 396 MHz). PocketSNES
writes it the way NoverII does, idles the CPU for a millisecond, then times a
loop of known length against the 32 kHz timer to see what clock it really
got. A marker file guards against freezes. See `platform_nspire.cpp`
("CPU clock") and FINDINGS.md.

**Settings file.** `name=value` lines (`config_version=2`,
`frameskip_type`, `cpu_speed`, ... `key_a=122`, `key_a_2=121+20` for shift+3).
Key numbers are lr-gpsp-nspire's: 1 + the bit index in the keypad
registers. A game's own file adds `own_settings=1` / `own_keys=1`. Unknown
lines are ignored, so older and newer builds can share the file.

**Save states** are Snes9x snapshots compressed with zlib, written
through a buffer in memory.

### On the calculator

* *Run speed test* (in a game's Graphics/performance menu) is the quickest
  way to see what a change does; the `BENCH=1` build does the same unattended
  and adds the hardware report.
* Any USB transfer tool works (TI's software, TiLP, or one built on
  libnspire). Files sent to the calculator must end in `.tns`, which is why
  the result files are called `*.txt.tns`.
* FINDINGS.md lists what's known about the LCD, DMA, memory speeds, the
  on-chip SRAM and the clock, and ideas not tried yet (ARM assembly for tile
  drawing, profile-guided optimisation).


Credits and license
-------------------

* Snes9x 1.43 by the Snes9x team; PocketSNES by Nebuleon; the TI-Nspire port
  by gameblabla.
* The menu follows lr-gpsp-nspire (gpSP by Exophase, Nspire port by
  andymcca).
* Ndless by the Ndless team. The CPU speed method is NoverII's, by Xavier
  Andreani. Hardware details from Hackspire and the Firebird emulator.

Snes9x is free for non-commercial use: "Permission to use, copy, modify and
distribute Snes9x in both binary and source form, for non-commercial
purposes, is hereby granted without fee, providing that this license
information and copyright notice appear with all copies and any derived
work." The full notice is at the top of `pocketsnes/include/snes9x.h`.
Super NES and Super Nintendo Entertainment System are trademarks of
Nintendo.
