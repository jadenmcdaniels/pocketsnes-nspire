#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>

#include "browser.h"
#include "config.h"
#include "draw.h"
#include "gui.h"
#include "menu.h"

#define LIST_Y    40
#define LIST_ROWS 17
#define NAME_COLUMNS 50

struct Entry
{
    char name[256];
    int is_dir;
};

static struct Entry *entries;
static int entry_count, entry_capacity;
static int has_parent;   /* entries[0] is ".." */
static char current_dir[512];

static int ends_with(const char *text, const char *suffix)
{
    size_t text_len = strlen(text), suffix_len = strlen(suffix);
    return text_len >= suffix_len && strcasecmp(text + text_len - suffix_len, suffix) == 0;
}

/* ROMs are "*.tns" files; this hides PocketSNES's own files next to them. */
static int is_rom_name(const char *name)
{
    size_t len = strlen(name);

    if (ends_with(name, ".srm.tns") || ends_with(name, ".cfg.tns") ||
        strncasecmp(name, "pocketsnes", 10) == 0)
        return 0;
    if (len > 10 && strncasecmp(name + len - 10, ".sv", 3) == 0 && ends_with(name, ".tns"))
        return 0;
    if (ends_with(name, ".tns"))
        return 1;
#ifndef _TINSPIRE
    /* The PC build also takes plain ROM files. */
    if (ends_with(name, ".sfc") || ends_with(name, ".smc") || ends_with(name, ".swc") ||
        ends_with(name, ".fig"))
        return 1;
#endif
    return 0;
}

static int compare_entries(const void *a, const void *b)
{
    const struct Entry *x = (const struct Entry *) a, *y = (const struct Entry *) b;
    if (x->is_dir != y->is_dir)
        return y->is_dir - x->is_dir;
    return strcasecmp(x->name, y->name);
}

static void add_entry(const char *name, int is_dir)
{
    if (entry_count == entry_capacity)
    {
        int capacity = entry_capacity ? entry_capacity * 2 : 64;
        struct Entry *grown = (struct Entry *) realloc(entries, capacity * sizeof(*entries));
        if (!grown)
            return;
        entries = grown;
        entry_capacity = capacity;
    }
    snprintf(entries[entry_count].name, sizeof(entries[entry_count].name), "%s", name);
    entries[entry_count].is_dir = is_dir;
    entry_count++;
}

static void read_dir(void)
{
    entry_count = 0;
    has_parent = strcmp(current_dir, "/") != 0;
    if (has_parent)
        add_entry("..", 1);

    DIR *dir = opendir(current_dir);
    if (!dir)
        return;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        char path[800];
        struct stat st;

        if (entry->d_name[0] == '.')
            continue;
        snprintf(path, sizeof(path), "%s/%s", strcmp(current_dir, "/") == 0 ? "" : current_dir,
                 entry->d_name);
        int is_dir = stat(path, &st) == 0 && S_ISDIR(st.st_mode);
        if (is_dir || is_rom_name(entry->d_name))
            add_entry(entry->d_name, is_dir);
    }
    closedir(dir);

    qsort(entries + has_parent, entry_count - has_parent, sizeof(*entries), compare_entries);
}

static int dir_exists(const char *path)
{
    DIR *dir = opendir(path);
    if (!dir)
        return 0;
    closedir(dir);
    return 1;
}

static void draw_browser(int selected, int top)
{
    char line[NAME_COLUMNS + 8];
    size_t dir_len = strlen(current_dir);

    draw_clear(COLOR_BG);
    draw_text("PocketSNES - choose a game", COLOR_ROM_INFO, COLOR_BG, 10, 10, 0);
    if (dir_len > NAME_COLUMNS)
        snprintf(line, sizeof(line), "...%s", current_dir + dir_len - (NAME_COLUMNS - 3));
    else
        snprintf(line, sizeof(line), "%.*s", NAME_COLUMNS, current_dir);
    draw_text(line, COLOR_ROM_INFO, COLOR_BG, 10, 20, 0);

    for (int row = 0; row < LIST_ROWS && top + row < entry_count; row++)
    {
        const struct Entry *entry = &entries[top + row];
        snprintf(line, sizeof(line), "%.*s%s", NAME_COLUMNS - 1, entry->name, entry->is_dir ? "/" : "");
        draw_text(line, top + row == selected ? COLOR_ACTIVE_ITEM : COLOR_INACTIVE_ITEM, COLOR_BG,
                  10, LIST_Y + row * TEXT_H, 0);
    }
    if (entry_count == has_parent)
    {
        static const char *const hint[] =
        {
            "No games in this folder. Copy SNES ROMs to the",
            "calculator named like game.sfc.tns (it only takes",
            ".tns files), or pick another folder (.. goes up).",
        };
        for (int i = 0; i < 3; i++)
            draw_text(hint[i], COLOR_HELP_TEXT, COLOR_BG, 10, LIST_Y + (2 + i) * TEXT_H, 0);
    }

    draw_text("Enter: open   Left/Right: page   Esc: quit", COLOR_HELP_TEXT, COLOR_BG, 10, 215, 0);
    draw_text("Menu: settings (for the game, or all games)", COLOR_HELP_TEXT, COLOR_BG, 10, 225, 0);
}

/* The full path of an entry in the current folder. */
static void entry_path(const struct Entry *entry, char *path, size_t size)
{
    snprintf(path, size, "%s/%s", strcmp(current_dir, "/") == 0 ? "" : current_dir, entry->name);
}

enum BrowserResult browser_run(char *path, size_t size)
{
    int selected = 0, top = 0;
    enum BrowserResult chosen = BROWSER_QUIT;

    snprintf(current_dir, sizeof(current_dir), "%s",
             dir_exists(cfg.rom_dir) ? cfg.rom_dir : platform_exe_dir());
    read_dir();
    gui_wait_release();

    /* Coming back from a game, start on that game. */
    const char *last = strrchr(path, '/');
    if (last)
        for (int i = 0; i < entry_count; i++)
            if (strcmp(entries[i].name, last + 1) == 0)
                selected = i;

    for (;;)
    {
        if (selected >= entry_count)
            selected = entry_count ? entry_count - 1 : 0;
        if (selected < top)
            top = selected;
        if (selected >= top + LIST_ROWS)
            top = selected - LIST_ROWS + 1;

        draw_browser(selected, top);
        gui_present();

        enum GuiAction action = gui_input();
        if (action == GUI_BACK || action == GUI_QUIT)
            break;
        if (action == GUI_MENU)
        {
            /* The settings of the game under the cursor, or of all games. */
            char rom[800];
            int on_game = entry_count > 0 && !entries[selected].is_dir;
            if (on_game)
            {
                entry_path(&entries[selected], rom, sizeof(rom));
                config_open_game(rom);
            }
            enum ListMenuResult result = menu_run_list(on_game ? rom : NULL);
            if (on_game)
                config_close_game();
            if (result == LIST_MENU_QUIT)
                break;
            if (result == LIST_MENU_START || result == LIST_MENU_START_WITHOUT_STATE)
            {
                snprintf(path, size, "%s", rom);
                snprintf(cfg.rom_dir, sizeof(cfg.rom_dir), "%s", current_dir);
                chosen = result == LIST_MENU_START ? BROWSER_START : BROWSER_START_WITHOUT_STATE;
                break;
            }
            gui_wait_release();
            continue;
        }
        if (entry_count == 0)
            continue;

        if (action == GUI_UP)
            selected = (selected + entry_count - 1) % entry_count;
        else if (action == GUI_DOWN)
            selected = (selected + 1) % entry_count;
        else if (action == GUI_LEFT)
            selected = selected > LIST_ROWS ? selected - LIST_ROWS : 0;
        else if (action == GUI_RIGHT)
            selected = selected + LIST_ROWS < entry_count ? selected + LIST_ROWS : entry_count - 1;
        else if (action == GUI_SELECT)
        {
            struct Entry *entry = &entries[selected];

            if (entry->is_dir && strcmp(entry->name, "..") == 0)
            {
                /* Go up and put the cursor on the folder we came out of. */
                char child[256];
                char *slash = strrchr(current_dir, '/');
                snprintf(child, sizeof(child), "%s", slash ? slash + 1 : "");
                if (slash == current_dir)
                    current_dir[1] = 0;
                else if (slash)
                    *slash = 0;
                read_dir();
                selected = top = 0;
                for (int i = 0; i < entry_count; i++)
                    if (strcmp(entries[i].name, child) == 0)
                        selected = i;
            }
            else if (entry->is_dir)
            {
                size_t len = strlen(current_dir);
                snprintf(current_dir + len, sizeof(current_dir) - len, "%s%s",
                         len && current_dir[len - 1] == '/' ? "" : "/", entry->name);
                read_dir();
                selected = top = 0;
            }
            else
            {
                entry_path(entry, path, size);
                snprintf(cfg.rom_dir, sizeof(cfg.rom_dir), "%s", current_dir);
                chosen = BROWSER_START;
                break;
            }
        }
    }

    free(entries);
    entries = NULL;
    entry_count = entry_capacity = 0;
    gui_wait_release();
    return chosen;
}
