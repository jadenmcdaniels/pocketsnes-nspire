PocketSNES for TI-Nspire CX / CX II, with a menu
================================================

This is gameblabla's PocketSNES 2.0 source (the 3.0 source was lost) with a new
frontend in `frontend/` that adds the features of lr-gpsp-nspire:

* An in-game menu laid out like gpSP's. The game is paused while it is open.
  In the game list, the menu key opens the settings of the highlighted game
  (or of all games, on a folder), where a game can also be started without
  loading its newest state (handy if that state is broken).
* Settings and keys for all games, or for one game only ("Settings for" /
  "Keys for" in the menu; a game's own are kept in its .pocketsnes folder).
* Save states in slots 1 to 999. Each of these can be switched off:
  auto-increment (every save goes to a new slot instead of overwriting one),
  saving a state when leaving the game (Q, Exit or Load new game), and
  loading the newest state when a game starts. All three are on by default.
* Speed limiting to 60 fps (50 for PAL games) with gpSP's frameskip choices
  (automatic, manual, off), fast forward, and an FPS counter showing frames
  drawn / expected per second (e.g. "49/60").
* On the CX II, only the parts of a frame that changed (the picture and the
  text over it) are turned into the buffer the screen shows next, which the
  LCD switches to at its next refresh: no tearing (see FINDINGS.md). Menu >
  Graphics/performance > Screen output can switch to the DMA controller
  instead, a little faster but it can tear.
* CPU speed (Graphics/performance, CX II): normal (396 MHz), 432, 456 or
  480 MHz while a game runs; menus, the game list and file writes stay at
  normal speed. Too fast a speed freezes the calculator (it needs a reset);
  the next start then puts the setting back to normal and says so. Raising
  the clock speeds up memory too and uses more battery. The way the clock
  is set comes from NoverII by Xavier Andreani.
* Every SNES button and hotkey has two key slots, so two keys can do the same
  thing, and a key can be put on several buttons (one key pressing A and B).
  A slot can also hold a key combination: hold one key and press another.
* Hotkeys for the menu, quit, fast forward, save, load, load newest, next and
  previous slot, the FPS counter and restarting the game.
* In-game (battery) saves are written a few seconds after the game saves,
  not only when you quit.

CONTROLS
========

These are the defaults; every one can be changed in Menu > Configure SNES
buttons and Configure hotkeys (each has a second, empty key slot too).

| Calculator key            | Does                                    |
|---------------------------|-----------------------------------------|
| CTRL / SHIFT / VAR / DEL  | SNES A / B / X / Y                      |
| TAB / MENU                | SNES L / R                              |
| ENTER / MINUS             | SNES Start / Select                     |
| arrows or 8 5 4 6         | SNES d-pad (7 9 1 3 for diagonals)      |
| ESC                       | open the menu                           |
| Q                         | quit (saves a state first, unless that's switched off) |
| F                         | fast forward on/off                     |
| S / L                     | save state / load the selected slot     |

Load newest, Next slot, Previous slot, FPS counter and Restart game have no
key until you give them one.

In the menu: up/down (or 8/5/2) move, left/right (or 4/6) change a value or
the slot (hold to go faster), enter or click selects, ESC goes back. On the
key screens, left/right pick key 1 or 2, enter sets it (press a key, or hold
one and press another for a combination) and del clears it. In the game
list, menu opens the settings.

FILES
=====

* `pocketsnes.tns` and its settings, `pocketsnes.cfg.tns`, in the same folder.
* ROMs anywhere, named like `game.sfc.tns` or `game.smc.tns`. The game list
  starts in the folder you used last.
* Saves go in a `.pocketsnes` folder next to the ROM:
  `game.sfc.srm.tns` is the in-game save (same place and name as PocketSNES
  3.0 uses), and `game.sfc.sv001.tns` to `game.sfc.sv999.tns` are save states
  (compressed, a few dozen KB each). A game's own settings or keys, if it has
  any, are in `game.sfc.cfg.tns` there.

BUILDING
========

    make -f Makefile.nspire          # makes pocketsnes.tns

This needs the Ndless SDK's toolchain on PATH (nspire-g++, nspire-ld,
genzehn, make-prg).

`make` (without `-f`) builds a PC version for testing; see TESTING.md.

Original notes
==============

Port done by gameblabla

PocketSNES was originally a SNES emulator by Nebuleon for GCW0.

I then took his emulator and removed the spu & apu code. (since the ti nspire
doesn't officially support sound)

Incompatible games are: Far East of Eden Zero, Street Fighter Alpha 2 and
Star Ocean. These 3 games use the SDD1 chip and Snes9x 1.43 requires a
graphics pack for them to be emulated.
