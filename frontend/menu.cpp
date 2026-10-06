#include <stdio.h>
#include <string.h>

#include "config.h"
#include "draw.h"
#include "emu.h"
#include "gui.h"
#include "bindings.h"
#include "menu.h"
#include "platform.h"
#include "states.h"

#define HELP_Y       210
#define HELP_LINES   3
#define ITEM_COLUMNS 52

/* The key screens: an action's name, then its two key slots. */
#define BINDING_NAME_COLUMNS 14
#define BINDING_COLUMNS      18
#define BINDING_X(column)    (10 + (BINDING_NAME_COLUMNS + (column) * BINDING_COLUMNS) * TEXT_W)

enum OptionType
{
    OPTION_ACTION,
    OPTION_SUBMENU,    /* no submenu: back to the menu this one was opened from */
    OPTION_CHOICE,     /* value indexes 'choices' */
    OPTION_NUMBER,     /* value is 0..max */
    OPTION_BINDING,    /* the two key slots of action 'action_id' */
    OPTION_LOAD_SLOT,  /* left/right pick the slot, select loads it */
    OPTION_SAVE_SLOT   /* left/right pick the slot, select saves */
};

/* What an action asks the menu to do next. */
enum Step
{
    STEP_STAY,
    STEP_RESUME,
    STEP_NEW_GAME,
    STEP_EXIT,
    STEP_BACK_TO_LIST,
    STEP_START,
    STEP_START_WITHOUT_STATE
};

struct Menu;

struct Option
{
    enum OptionType type;
    const char *label;
    const char *help;
    int line;
    uint32_t *value;
    const char *const *choices;
    int max;                       /* last choice / largest number */
    enum Step (*action)(void);
    struct Menu *submenu;
    void (*changed)(void);         /* called after a choice or number changes */
    int action_id;                 /* OPTION_BINDING */
};

struct Menu
{
    struct Option *options;
    int count;
    int first_y;
    const char *title;             /* NULL: a main menu, which shows the game */
};

static char status[64];
static uint32_t status_until;
static int key_column;            /* the key slot picked on the key screens */
static int in_game;               /* the in-game menu, not the game list's */
static const char *list_rom;      /* the game list's menu: the game, or NULL for all games */

static void set_status(const char *format, int slot)
{
    snprintf(status, sizeof(status), format, slot);
    status_until = platform_ticks() + platform_tick_hz() * 3;
}

/* ---- Actions ---- */

static enum Step load_state(void)
{
    int slot = emu_slot();
    int result = emu_load_state(slot);
    if (result == 1)
    {
        char text[40];
        snprintf(text, sizeof(text), "Loaded slot %03d", slot);
        emu_show_message(text);
        return STEP_RESUME;
    }
    set_status(result == 0 ? "Slot %03d is empty." : "Couldn't load slot %03d.", slot);
    return STEP_STAY;
}

static enum Step save_state(void)
{
    int slot = emu_save_target();
    if (!slot)
        set_status("All 999 slots are used.", 0);
    else if (emu_save_state(slot))
        set_status("Saved slot %03d.", slot);
    else
        set_status("Couldn't save slot %03d. Out of space?", slot);
    return STEP_STAY;
}

/* Puts one group of actions back to the default keys. */
static void reset_actions(int first, int last)
{
    struct Binding kept[NUM_ACTIONS][KEY_SLOTS];
    memcpy(kept, cfg.keys, sizeof(kept));
    config_default_keys();
    for (int i = 0; i < NUM_ACTIONS; i++)
        if (i < first || i > last)
            memcpy(cfg.keys[i], kept[i], sizeof(kept[i]));
}

static enum Step reset_buttons(void)
{
    reset_actions(0, FIRST_HOTKEY - 1);
    set_status("SNES buttons reset to the defaults.", 0);
    return STEP_STAY;
}

static enum Step reset_hotkeys(void)
{
    reset_actions(FIRST_HOTKEY, NUM_ACTIONS - 1);
    set_status("Hotkeys reset to the defaults.", 0);
    return STEP_STAY;
}

static enum Step run_speed_test(void)
{
    emu_speed_test(0);
    return STEP_STAY;
}

static enum Step load_new_game(void) { return STEP_NEW_GAME; }
static enum Step return_to_game(void) { return STEP_RESUME; }
static enum Step exit_app(void) { return STEP_EXIT; }
static enum Step back_to_list(void) { return STEP_BACK_TO_LIST; }
static enum Step start_game(void) { return STEP_START; }
static enum Step start_without_state(void) { return STEP_START_WITHOUT_STATE; }

static enum Step restart_game(void)
{
    emu_reset_game();
    emu_show_message("Game restarted");
    return STEP_RESUME;
}

/* Whether the game's settings and keys are its own (config.h). */
static uint32_t own_settings, own_keys;
static void own_settings_changed(void) { config_set_game_settings((int) own_settings); }
static void own_keys_changed(void) { config_set_game_keys((int) own_keys); }

/* ---- Menus ---- */

static const char *const frameskip_types[] = { "automatic", "manual", "off" };
static const char *const off_on[] = { "off", "on" };
static const char *const fps_modes[] = { "off", "on", "detailed" };
static const char *const sram_write_modes[] = { "on exit only", "automatically" };
static const char *const scopes[] = { "all games", "this game only" };
static const char *const screen_outputs[] = { "no tearing", "DMA (faster, can tear)" };
static const char *const cpu_speeds[] = { "normal (396 MHz)", "432 MHz", "456 MHz", "480 MHz" };

#define FRAMESKIP_OPTIONS \
    { OPTION_CHOICE, "Frameskip type: %s", \
      "Automatic: skip drawing frames only when the game falls behind. " \
      "Manual: always draw 1 of every N+1 frames. Off: draw every frame.", \
      0, &cfg.frameskip_type, frameskip_types, NUM_FRAMESKIP_TYPES - 1, NULL, NULL, NULL, 0 }, \
    { OPTION_NUMBER, "Frameskip value: %d", \
      "Automatic: the most frames skipped in a row. Manual: N, the frames " \
      "skipped for every frame drawn.", \
      1, &cfg.frameskip_value, NULL, MAX_FRAMESKIP_VALUE, NULL, NULL, NULL, 0 }, \
    { OPTION_CHOICE, "Show FPS counter: %s", \
      "Frames drawn / expected per second (like lr-gpsp-nspire). Detailed adds " \
      "the game's real speed and ms per frame, logged to pocketsnes_perf.txt.", \
      2, &cfg.show_fps, fps_modes, 2, NULL, NULL, NULL, 0 }, \
    { OPTION_CHOICE, "Screen output: %s", \
      "No tearing: frames go to the buffer the screen shows next. DMA: " \
      "copied by the DMA chip, a little faster, but fast scrolling can " \
      "show a split line.", \
      3, &cfg.screen_dma, screen_outputs, 1, NULL, NULL, NULL, 0 }, \
    { OPTION_CHOICE, "CPU speed: %s", \
      "Raises the clock while a game runs; menus stay normal. Uses more " \
      "battery. If it freezes the calculator, the next start is back at " \
      "normal.", \
      4, &cfg.cpu_speed, cpu_speeds, NUM_CPU_SPEEDS - 1, NULL, NULL, NULL, 0 }

#define BACK_OPTION(line) \
    { OPTION_SUBMENU, "Back", "Return to the menu before.", line, NULL, NULL, 0, NULL, NULL, NULL, 0 }

static struct Option graphics_options[] =
{
    FRAMESKIP_OPTIONS,
    { OPTION_ACTION, "Run speed test (about 35 seconds)",
      "Runs the game from here in a few speed modes, at the CPU speed set "
      "above, and shows the results. Esc stops it. The game is put back where it was.",
      6, NULL, NULL, 0, run_speed_test, NULL, NULL, 0 },
    BACK_OPTION(8),
};

/* The same from the game list, which has no game running to test. */
static struct Option list_graphics_options[] =
{
    FRAMESKIP_OPTIONS,
    BACK_OPTION(6),
};

static struct Option savestate_options[] =
{
    { OPTION_CHOICE, "Auto-increment slot on save: %s",
      "On: every save (S, or the menu) goes to a new slot after your highest "
      "one, so nothing gets overwritten. Off: saves go to the selected slot.",
      0, &cfg.auto_increment, off_on, 1, NULL, NULL, NULL, 0 },
    { OPTION_CHOICE, "Save a state when leaving: %s",
      "On: leaving the game (Q, Exit or Load new game) saves a state first, "
      "to a new slot with auto-increment on.",
      1, &cfg.save_on_exit, off_on, 1, NULL, NULL, NULL, 0 },
    { OPTION_CHOICE, "Load newest state on start: %s",
      "On: starting a game loads its newest state. To start a game without "
      "it once, press menu on the game in the game list.",
      2, &cfg.load_on_start, off_on, 1, NULL, NULL, NULL, 0 },
    { OPTION_CHOICE, "Write in-game saves: %s",
      "When the game's own battery save (SRAM) is written to the calculator. "
      "Automatically: a few seconds after the game saves.",
      3, &cfg.sram_autosave, sram_write_modes, 1, NULL, NULL, NULL, 0 },
    BACK_OPTION(5),
};

#define BINDING_HELP "Left/right: key 1 or 2. Enter: set it (hold a key and press another for a combination). Del: clear it."

#define BINDING_OPTION(action, line) \
    { OPTION_BINDING, NULL, BINDING_HELP, line, NULL, NULL, 0, NULL, NULL, NULL, action }

static struct Option button_options[] =
{
    BINDING_OPTION(ACTION_UP, 0),
    BINDING_OPTION(ACTION_DOWN, 1),
    BINDING_OPTION(ACTION_LEFT, 2),
    BINDING_OPTION(ACTION_RIGHT, 3),
    BINDING_OPTION(ACTION_A, 4),
    BINDING_OPTION(ACTION_B, 5),
    BINDING_OPTION(ACTION_X, 6),
    BINDING_OPTION(ACTION_Y, 7),
    BINDING_OPTION(ACTION_L, 8),
    BINDING_OPTION(ACTION_R, 9),
    BINDING_OPTION(ACTION_START, 10),
    BINDING_OPTION(ACTION_SELECT, 11),
    { OPTION_ACTION, "Reset to defaults",
      "Puts the SNES buttons back to the default keys. The arrows and 7 9 1 3 "
      "(diagonals, unless bound) always work as the d-pad.",
      13, NULL, NULL, 0, reset_buttons, NULL, NULL, 0 },
    BACK_OPTION(14),
};

static struct Option hotkey_options[] =
{
    BINDING_OPTION(ACTION_MENU, 0),
    BINDING_OPTION(ACTION_QUIT, 1),
    BINDING_OPTION(ACTION_FAST_FORWARD, 2),
    BINDING_OPTION(ACTION_SAVE_STATE, 3),
    BINDING_OPTION(ACTION_LOAD_STATE, 4),
    BINDING_OPTION(ACTION_LOAD_NEWEST, 5),
    BINDING_OPTION(ACTION_NEXT_SLOT, 6),
    BINDING_OPTION(ACTION_PREVIOUS_SLOT, 7),
    BINDING_OPTION(ACTION_SHOW_FPS, 8),
    BINDING_OPTION(ACTION_RESTART, 9),
    { OPTION_ACTION, "Reset to defaults",
      "Puts the hotkeys back to the defaults: esc menu, Q quit, F fast "
      "forward, S save, L load; the others have no key.",
      11, NULL, NULL, 0, reset_hotkeys, NULL, NULL, 0 },
    BACK_OPTION(12),
};

static struct Option about_options[] =
{
    BACK_OPTION(16),
};

#define MENU(name, options, first_y, title) \
    static struct Menu name = { options, sizeof(options) / sizeof(options[0]), first_y, title }

MENU(graphics_menu, graphics_options, 40, "Graphics and performance");
MENU(list_graphics_menu, list_graphics_options, 40, "Graphics and performance");
MENU(savestate_menu, savestate_options, 40, "Save state options");
MENU(button_menu, button_options, 30, "SNES buttons");
MENU(hotkey_menu, hotkey_options, 30, "Hotkeys");
MENU(about_menu, about_options, 40, "About");

#define SCOPE_OPTIONS(line) \
    { OPTION_CHOICE, "Settings for: %s", \
      "This game only: the graphics/performance and save state options set here are kept " \
      "for this game alone. All games: it uses the shared ones again.", \
      line, &own_settings, scopes, 1, NULL, NULL, own_settings_changed, 0 }, \
    { OPTION_CHOICE, "Keys for: %s", \
      "This game only: the SNES buttons and hotkeys set here are kept for " \
      "this game alone. All games: it uses the shared ones again.", \
      line + 1, &own_keys, scopes, 1, NULL, NULL, own_keys_changed, 0 }

static struct Option main_options[] =
{
    { OPTION_SUBMENU, "Graphics/performance",
      "Frameskip, the FPS counter, screen output, CPU speed and the speed test.",
      0, NULL, NULL, 0, NULL, &graphics_menu, NULL, 0 },
    { OPTION_SUBMENU, "Save state options",
      "Auto-increment, saving when leaving, loading on start and in-game saves.",
      1, NULL, NULL, 0, NULL, &savestate_menu, NULL, 0 },
    { OPTION_SUBMENU, "Configure SNES buttons", "Which keys press the SNES buttons, two each.",
      2, NULL, NULL, 0, NULL, &button_menu, NULL, 0 },
    { OPTION_SUBMENU, "Configure hotkeys",
      "Keys or key combinations for the menu, saving, loading, fast forward and more.",
      3, NULL, NULL, 0, NULL, &hotkey_menu, NULL, 0 },
    { OPTION_LOAD_SLOT, NULL,
      "Loads the state in the selected slot. Left/right pick the slot (1-999); hold to go faster.",
      5, NULL, NULL, 0, load_state, NULL, NULL, 0 },
    { OPTION_SAVE_SLOT, NULL,
      "Saves the game's state. Left/right pick the slot; with auto-increment on, saves always go to a new slot.",
      6, NULL, NULL, 0, save_state, NULL, NULL, 0 },
    SCOPE_OPTIONS(8),
    { OPTION_ACTION, "Load new game", "Leave this game and pick another ROM.",
      11, NULL, NULL, 0, load_new_game, NULL, NULL, 0 },
    { OPTION_ACTION, "Restart game", "Reset the SNES with this game.",
      12, NULL, NULL, 0, restart_game, NULL, NULL, 0 },
    { OPTION_ACTION, "Return to game", "Close this menu and keep playing.",
      13, NULL, NULL, 0, return_to_game, NULL, NULL, 0 },
    { OPTION_SUBMENU, "About", "Credits.", 14, NULL, NULL, 0, NULL, &about_menu, NULL, 0 },
    { OPTION_ACTION, "Exit PocketSNES", "Quit to the calculator.",
      16, NULL, NULL, 0, exit_app, NULL, NULL, 0 },
};

/* The game list's menu on a game. */
static struct Option list_game_options[] =
{
    { OPTION_ACTION, "Start game", "Starts the game (with its newest state if Load newest state on start is on).",
      0, NULL, NULL, 0, start_game, NULL, NULL, 0 },
    { OPTION_ACTION, "Start without loading a state",
      "Starts the game from its in-game save only, whatever Load newest state on start says.",
      1, NULL, NULL, 0, start_without_state, NULL, NULL, 0 },
    { OPTION_SUBMENU, "Graphics/performance",
      "Frameskip, the FPS counter, screen output and CPU speed.",
      3, NULL, NULL, 0, NULL, &list_graphics_menu, NULL, 0 },
    { OPTION_SUBMENU, "Save state options",
      "Auto-increment, saving when leaving, loading on start and in-game saves.",
      4, NULL, NULL, 0, NULL, &savestate_menu, NULL, 0 },
    { OPTION_SUBMENU, "Configure SNES buttons", "Which keys press the SNES buttons, two each.",
      5, NULL, NULL, 0, NULL, &button_menu, NULL, 0 },
    { OPTION_SUBMENU, "Configure hotkeys",
      "Keys or key combinations for the menu, saving, loading, fast forward and more.",
      6, NULL, NULL, 0, NULL, &hotkey_menu, NULL, 0 },
    SCOPE_OPTIONS(8),
    { OPTION_ACTION, "Back to the game list", "Close this menu.",
      11, NULL, NULL, 0, back_to_list, NULL, NULL, 0 },
};

/* ... and on a folder: the settings for all games. */
static struct Option list_options[] =
{
    { OPTION_SUBMENU, "Graphics/performance",
      "Frameskip, the FPS counter, screen output and CPU speed.",
      0, NULL, NULL, 0, NULL, &list_graphics_menu, NULL, 0 },
    { OPTION_SUBMENU, "Save state options",
      "Auto-increment, saving when leaving, loading on start and in-game saves.",
      1, NULL, NULL, 0, NULL, &savestate_menu, NULL, 0 },
    { OPTION_SUBMENU, "Configure SNES buttons", "Which keys press the SNES buttons, two each.",
      2, NULL, NULL, 0, NULL, &button_menu, NULL, 0 },
    { OPTION_SUBMENU, "Configure hotkeys",
      "Keys or key combinations for the menu, saving, loading, fast forward and more.",
      3, NULL, NULL, 0, NULL, &hotkey_menu, NULL, 0 },
    { OPTION_ACTION, "Back to the game list", "Close this menu.",
      5, NULL, NULL, 0, back_to_list, NULL, NULL, 0 },
};

MENU(main_menu, main_options, 40, NULL);
MENU(list_game_menu, list_game_options, 40, NULL);
MENU(list_menu, list_options, 40, NULL);

/* ---- Drawing ---- */

static void format_option(const struct Option *option, char *text, size_t size)
{
    switch (option->type)
    {
    case OPTION_CHOICE:
        snprintf(text, size, option->label, option->choices[*option->value]);
        break;
    case OPTION_NUMBER:
        snprintf(text, size, option->label, (int) *option->value);
        break;
    case OPTION_BINDING:
    {
        char first[24], second[24];
        snprintf(text, size, "%-*s%-*s%s", BINDING_NAME_COLUMNS, action_names[option->action_id],
                 BINDING_COLUMNS, binding_name(cfg.keys[option->action_id][0], first, sizeof(first)),
                 binding_name(cfg.keys[option->action_id][1], second, sizeof(second)));
        break;
    }
    case OPTION_LOAD_SLOT:
        snprintf(text, size, "Load state from slot %03d", emu_slot());
        break;
    case OPTION_SAVE_SLOT:
        if (!cfg.auto_increment)
            snprintf(text, size, "Save state to slot %03d", emu_slot());
        else if (emu_save_target())
            snprintf(text, size, "Save state to new slot (%03d)", emu_save_target());
        else
            snprintf(text, size, "Save state (all 999 slots used)");
        break;
    default:
        snprintf(text, size, "%s", option->label);
        break;
    }
}

/* Word-wraps the help text into the bottom lines, like lr-gpsp-nspire. */
static void draw_help(const char *text)
{
    for (int line = 0; line < HELP_LINES && *text; line++)
    {
        int len = (int) strlen(text);
        if (len > ITEM_COLUMNS)
        {
            len = ITEM_COLUMNS;
            while (len > 0 && text[len] != ' ')
                len--;
            if (len == 0)
                len = ITEM_COLUMNS;
        }
        char buffer[ITEM_COLUMNS + 1];
        snprintf(buffer, sizeof(buffer), "%.*s", len, text);
        draw_text(buffer, COLOR_HELP_TEXT, COLOR_BG, 8, HELP_Y + line * TEXT_H, 0);
        text += len;
        while (*text == ' ')
            text++;
    }
}

static const char *file_name(const char *path)
{
    const char *slash = strrchr(path, '/');
    return slash ? slash + 1 : path;
}

static void draw_header(const struct Menu *menu)
{
    char text[ITEM_COLUMNS + 1];
    int show_status = status[0] && (int32_t) (status_until - platform_ticks()) > 0;

    if (menu->first_y < 40)
    {
        /* The key screens: the status takes the column titles' place. */
        draw_text(menu->title, COLOR_ACTIVE_ITEM, COLOR_BG, 10, 5, 0);
        if (show_status)
            draw_text(status, COLOR_WARNING, COLOR_BG, 10, 17, 0);
        else
        {
            snprintf(text, sizeof(text), "%-*s%-*s%s", BINDING_NAME_COLUMNS,
                     config_game_has_keys() ? "(this game)" : "", BINDING_COLUMNS, "Key 1", "Key 2");
            draw_text(text, COLOR_HELP_TEXT, COLOR_BG, 10, 17, 0);
        }
        return;
    }

    if (menu->title)
        draw_text(menu->title, COLOR_ACTIVE_ITEM, COLOR_BG, 10, 10, 0);
    else if (in_game)
    {
        snprintf(text, sizeof(text), "%s", file_name(emu_rom_path()));
        draw_text(text, COLOR_ROM_INFO, COLOR_BG, 10, 10, 0);
        snprintf(text, sizeof(text), "%s", emu_game_title());
        draw_text(text, COLOR_ROM_INFO, COLOR_BG, 10, 20, 0);
    }
    else
    {
        draw_text(list_rom ? "Settings for this game:" : "Settings for all games",
                  COLOR_ROM_INFO, COLOR_BG, 10, 10, 0);
        if (list_rom)
        {
            snprintf(text, sizeof(text), "%s", file_name(list_rom));
            draw_text(text, COLOR_ROM_INFO, COLOR_BG, 10, 20, 0);
        }
    }

    if (show_status)
        draw_text(status, COLOR_WARNING, COLOR_BG, 10, 30, 0);
    else if (!menu->title && in_game)
    {
        int slot = emu_slot();
        if (states_count())
            snprintf(text, sizeof(text), "Slot %03d: %s   (%d saved, newest %03d)", slot,
                     states_exists(slot) ? "saved" : "empty", states_count(), states_newest());
        else
            snprintf(text, sizeof(text), "Slot %03d: empty   (no saved states yet)", slot);
        draw_text(text, COLOR_HELP_TEXT, COLOR_BG, 10, 30, 0);
    }
}

static void draw_about(void)
{
    static const char *const lines[] =
    {
        "PocketSNES for TI-Nspire",
        "",
        "Snes9x 1.43 core by the Snes9x team",
        "PocketSNES port by gameblabla",
        "Menu modeled on lr-gpsp-nspire (andymcca)",
        "and gpSP by Exophase",
        "Ndless by the Ndless team",
        "",
        "Hotkeys (change them in Configure hotkeys):",
        "  esc  menu          Q  quit",
        "  S    save state    L  load state",
        "  F    fast forward",
        "In the game list, menu opens the settings.",
    };
    for (size_t i = 0; i < sizeof(lines) / sizeof(lines[0]); i++)
        draw_text(lines[i], i == 0 ? COLOR_ACTIVE_ITEM : COLOR_INACTIVE_ITEM, COLOR_BG,
                  10, 30 + (int) i * TEXT_H, 0);
}

static void draw_menu(const struct Menu *menu, int selected)
{
    char text[ITEM_COLUMNS + 40];

    draw_clear(COLOR_BG);
    draw_header(menu);
    if (menu == &about_menu)
        draw_about();

    for (int i = 0; i < menu->count; i++)
    {
        const struct Option *option = &menu->options[i];
        int y = menu->first_y + option->line * TEXT_H;
        format_option(option, text, sizeof(text));
        draw_text(text, i == selected ? COLOR_ACTIVE_ITEM : COLOR_INACTIVE_ITEM, COLOR_BG,
                  10, y, ITEM_COLUMNS);
        if (i == selected && option->type == OPTION_BINDING)
        {
            /* The picked key slot stands out. */
            char cell[24];
            binding_name(cfg.keys[option->action_id][key_column], cell, sizeof(cell));
            snprintf(text, sizeof(text), ">%s<", cell);
            draw_text(text, COLOR_WARNING, COLOR_BG, BINDING_X(key_column) - TEXT_W, y, 0);
        }
    }
    draw_help(menu->options[selected].help);
}

/* ---- Input ---- */

static int bindable(int key)
{
    return key && key_name(key) && !key_is_reserved(key);
}

/* Sets the picked key slot to the next key pressed, or to a combination:
 * a key held while another is pressed. */
static void choose_binding(const struct Menu *menu, const struct Option *option)
{
    int y = menu->first_y + option->line * TEXT_H;
    int x = BINDING_X(key_column);
    int held = 0;
    struct Binding chosen = { 0, 0 };

    draw_rect(x - TEXT_W, y, SCREEN_W - (x - TEXT_W), TEXT_H, COLOR_BG);
    draw_text("<press keys>", COLOR_WARNING, COLOR_BG, x - TEXT_W, y, 0);
    gui_present();
    gui_wait_release();

    while (!chosen.key)
    {
        platform_poll_keys();
        if (platform_quit_requested())
            return;
        if (!held)
        {
            int key = platform_first_key_down();
            if (bindable(key))
                held = key;
        }
        else if (!platform_key_down(held))
            chosen.key = (uint8_t) held;   /* let go alone: just this key */
        else
            for (int key = 1; key <= NUM_KEYS; key++)
                if (key != held && bindable(key) && platform_key_down(key))
                {
                    chosen.key = (uint8_t) key;
                    chosen.with = (uint8_t) held;
                    break;
                }
        gui_present();
    }
    cfg.keys[option->action_id][key_column] = chosen;
    gui_wait_release();
}

static void change_value(const struct Option *option, int delta)
{
    if (option->type == OPTION_CHOICE || option->type == OPTION_NUMBER)
    {
        int count = option->max + 1;
        *option->value = (uint32_t) (((int) *option->value + delta % count + count) % count);
        if (option->changed)
            option->changed();
    }
    else if (option->type == OPTION_BINDING)
        key_column = delta > 0 ? 1 : 0;
    else if (option->type == OPTION_LOAD_SLOT || option->type == OPTION_SAVE_SLOT)
    {
        int step = gui_repeat_count() > 20 ? 10 : 1;
        int slot = emu_slot() - 1 + delta * step;
        slot = (slot % MAX_STATE_SLOTS + MAX_STATE_SLOTS) % MAX_STATE_SLOTS;
        emu_set_slot(slot + 1);
    }
}

/* The entry of 'top' that opens 'menu', to land on when coming back. */
static int entry_of(const struct Menu *top, const struct Menu *menu)
{
    for (int i = 0; i < top->count; i++)
        if (top->options[i].submenu == menu)
            return i;
    return 0;
}

static enum Step run(struct Menu *top)
{
    struct Menu *menu = top;
    int selected = 0;
    enum Step step = STEP_STAY;

    status[0] = 0;
    key_column = 0;
    own_settings = (uint32_t) config_game_has_settings();
    own_keys = (uint32_t) config_game_has_keys();

    gui_wait_release();
    while (step == STEP_STAY)
    {
        draw_menu(menu, selected);
        gui_present();

        struct Option *option = &menu->options[selected];
        switch (gui_input())
        {
        case GUI_UP:
            selected = (selected + menu->count - 1) % menu->count;
            break;
        case GUI_DOWN:
            selected = (selected + 1) % menu->count;
            break;
        case GUI_LEFT:
            change_value(option, -1);
            break;
        case GUI_RIGHT:
            change_value(option, 1);
            break;
        case GUI_CLEAR:
            if (option->type == OPTION_BINDING)
                cfg.keys[option->action_id][key_column].key = cfg.keys[option->action_id][key_column].with = 0;
            break;
        case GUI_SELECT:
            if (option->type == OPTION_SUBMENU && option->submenu)
            {
                menu = option->submenu;
                selected = 0;
            }
            else if (option->type == OPTION_SUBMENU)
            {
                selected = entry_of(top, menu);
                menu = top;
            }
            else if (option->type == OPTION_BINDING)
                choose_binding(menu, option);
            else if (option->type == OPTION_CHOICE)
                change_value(option, 1);
            else if (option->action)
                step = option->action();
            break;
        case GUI_BACK:
        case GUI_MENU:
            if (menu == top)
                step = in_game ? STEP_RESUME : STEP_BACK_TO_LIST;
            else
            {
                selected = entry_of(top, menu);
                menu = top;
            }
            break;
        case GUI_QUIT:
            step = STEP_EXIT;
            break;
        default:
            break;
        }
    }

    config_save();
    return step;
}

enum MenuResult menu_run(void)
{
    in_game = 1;
    list_rom = NULL;
    enum Step step = run(&main_menu);
    return step == STEP_NEW_GAME ? MENU_LOAD_NEW_GAME : step == STEP_EXIT ? MENU_EXIT : MENU_RESUME;
}

enum ListMenuResult menu_run_list(const char *rom_path)
{
    in_game = 0;
    list_rom = rom_path;
    enum Step step = run(rom_path ? &list_game_menu : &list_menu);
    switch (step)
    {
    case STEP_START:
        return LIST_MENU_START;
    case STEP_START_WITHOUT_STATE:
        return LIST_MENU_START_WITHOUT_STATE;
    case STEP_EXIT:
        return LIST_MENU_QUIT;
    default:
        return LIST_MENU_BACK;
    }
}
