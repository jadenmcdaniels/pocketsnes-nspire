/* Save state slots. A game's states live next to its in-game save, in
 * "<ROM folder>/.pocketsnes/<ROM name>.svNNN.tns" for slots 1 to 999. */
#ifndef STATES_H
#define STATES_H

#include <stddef.h>

#define MAX_STATE_SLOTS 999

/* Points the slot functions at a ROM and scans its existing states. */
void states_open_game(const char *rom_path);

/* "<ROM folder>/.pocketsnes", created on first use. */
const char *states_dir(void);
void states_make_dir(void);
/* In-game (battery) save, named the way PocketSNES 3.0 names it. */
void states_sram_path(char *path, size_t size);

int states_exists(int slot);
int states_count(void);
/* The most recently saved slot, or 0 if the game has none. */
int states_newest(void);
/* Where an auto-increment save goes: the slot after the highest one used,
 * or the first free slot once 999 is taken. 0 when every slot is used. */
int states_next_new_slot(void);

/* Return 1 on success. states_load returns 0 for an empty slot and -1 when
 * the file can't be read or isn't a PocketSNES state. */
int states_save(int slot);
int states_load(int slot);

#endif
