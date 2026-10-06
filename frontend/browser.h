#ifndef BROWSER_H
#define BROWSER_H

#include <stddef.h>

enum BrowserResult
{
    BROWSER_QUIT,
    BROWSER_START,                /* start the game in 'path' */
    BROWSER_START_WITHOUT_STATE   /* ... without loading its newest state */
};

/* Lets the player pick a ROM, starting in cfg.rom_dir (on the file 'path'
 * already names, if it is there). The menu key opens the settings for the
 * highlighted game (or for all games on a folder), where it can also be
 * started without loading a state. */
enum BrowserResult browser_run(char *path, size_t size);

#endif
