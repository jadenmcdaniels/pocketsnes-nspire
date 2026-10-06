#include "config.h"
#include "draw.h"
#include "gui.h"
#include "bindings.h"
#include "platform.h"

static enum GuiAction held_direction;
static uint32_t repeat_at;
static int repeats;
static uint32_t frame_end;

enum GuiAction gui_input(void)
{
    platform_poll_keys();
    if (platform_quit_requested())
        return GUI_QUIT;

    if (platform_key_pressed(KEY_ENTER) || platform_key_pressed(KEY_CLICK) ||
        platform_key_pressed(KEY_RETURN))
        return GUI_SELECT;
    if (platform_key_pressed(KEY_MENU))
        return GUI_MENU;
    if (platform_key_pressed(KEY_ESC))
        return GUI_BACK;
    for (int slot = 0; slot < KEY_SLOTS; slot++)
        if (binding_pressed(cfg.keys[ACTION_MENU][slot]))
            return GUI_BACK;
    if (platform_key_pressed(KEY_DEL))
        return GUI_CLEAR;

    enum GuiAction direction = GUI_NONE;
    if (platform_key_down(KEY_UP) || platform_key_down(KEY_8))
        direction = GUI_UP;
    else if (platform_key_down(KEY_DOWN) || platform_key_down(KEY_2) || platform_key_down(KEY_5))
        direction = GUI_DOWN;
    else if (platform_key_down(KEY_LEFT) || platform_key_down(KEY_4))
        direction = GUI_LEFT;
    else if (platform_key_down(KEY_RIGHT) || platform_key_down(KEY_6))
        direction = GUI_RIGHT;

    uint32_t now = platform_ticks();
    uint32_t hz = platform_tick_hz();
    if (direction != held_direction)
    {
        held_direction = direction;
        repeat_at = now + hz * 2 / 5;
        repeats = 0;
        return direction;
    }
    if (direction != GUI_NONE && (int32_t) (now - repeat_at) >= 0)
    {
        repeat_at = now + hz / 15;
        repeats++;
        return direction;
    }
    return GUI_NONE;
}

int gui_repeat_count(void)
{
    return repeats;
}

void gui_present(void)
{
    platform_present();

    uint32_t now = platform_ticks();
    if ((int32_t) (frame_end - now) > (int32_t) platform_tick_hz() || (int32_t) (frame_end - now) < 0)
        frame_end = now;
    frame_end += platform_tick_hz() / 60;
    platform_wait_until(frame_end);
}

void gui_wait_release(void)
{
    do
    {
        platform_poll_keys();
        platform_wait_until(platform_ticks() + platform_tick_hz() / 100);
    } while (platform_any_key_down() && !platform_quit_requested());
    held_direction = GUI_NONE;
}

void gui_message(const char *line1, const char *line2)
{
    draw_clear(COLOR_BG);
    draw_text(line1, COLOR_ACTIVE_ITEM, COLOR_BG, 10, 100, 0);
    if (line2)
        draw_text(line2, COLOR_ROM_INFO, COLOR_BG, 10, 112, 0);
    draw_text("Press any key.", COLOR_HELP_TEXT, COLOR_BG, 10, 140, 0);
    gui_present();

    gui_wait_release();
    do
    {
        platform_poll_keys();
        gui_present();
    } while (!platform_any_key_down() && !platform_quit_requested());
    gui_wait_release();
}
