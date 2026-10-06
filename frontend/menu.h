#ifndef MENU_H
#define MENU_H

enum MenuResult
{
    MENU_RESUME,
    MENU_LOAD_NEW_GAME,
    MENU_EXIT
};

/* The in-game menu, laid out like lr-gpsp-nspire's. The game is paused while
 * it is open. */
enum MenuResult menu_run(void);

enum ListMenuResult
{
    LIST_MENU_BACK,
    LIST_MENU_START,
    LIST_MENU_START_WITHOUT_STATE,
    LIST_MENU_QUIT     /* PC build: the window was closed */
};

/* The game list's menu: the settings and keys of the game at rom_path
 * (opened with config_open_game), or for all games when it is NULL, and a
 * way to start the game without loading its newest state. */
enum ListMenuResult menu_run_list(const char *rom_path);

#endif
