/*
 * Gearlynx - Lynx Emulator
 * Copyright (C) 2025  Ignacio Sanchez

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see http://www.gnu.org/licenses/
 *
 */

#include <SDL3/SDL.h>
#include "gearlynx.h"
#include "config.h"
#include "gui.h"
#include "emu.h"
#include "application.h"
#include "gamepad.h"

#define EVENTS_IMPORT
#include "events.h"

static bool events_check_hotkey(const SDL_Event* event, const config_Hotkey& hotkey, bool allow_repeat);

void events_shortcuts(const SDL_Event* event)
{
    if (event->type != SDL_EVENT_KEY_DOWN)
        return;

    if (events_check_hotkey(event, config_hotkeys[config_HotkeyIndex_Quit], false))
    {
        application_trigger_quit();
        return;
    }

    if (events_check_hotkey(event, config_hotkeys[config_HotkeyIndex_Fullscreen], false))
    {
        config_emulator.fullscreen = !config_emulator.fullscreen;
        application_trigger_fullscreen(config_emulator.fullscreen);
        return;
    }

    for (int i = 0; i < 5; i++)
    {
        if (events_check_hotkey(event, config_hotkeys[config_HotkeyIndex_SelectSlot1 + i], false))
        {
            config_emulator.save_slot = i;
            return;
        }
    }

    for (int i = 0; i < GUI_HOTKEY_MAP_COUNT; i++)
    {
        if (gui_hotkey_map[i].shortcut >= 0 && events_check_hotkey(event, config_hotkeys[gui_hotkey_map[i].config_index], gui_hotkey_map[i].allow_repeat))
        {
            gui_shortcut((gui_ShortCutEvent)gui_hotkey_map[i].shortcut);
            return;
        }
    }

    int key = event->key.scancode;
    SDL_Keymod mods = event->key.mod;

    if (event->key.repeat == 0 && key == SDL_SCANCODE_A && (mods & SDL_KMOD_CTRL))
    {
        gui_shortcut(gui_ShortcutDebugSelectAll);
        return;
    }

    if (event->key.repeat == 0 && key == SDL_SCANCODE_C && (mods & SDL_KMOD_CTRL))
    {
        gui_shortcut(gui_ShortcutDebugCopy);
        return;
    }

    if (event->key.repeat == 0 && key == SDL_SCANCODE_V && (mods & SDL_KMOD_CTRL))
    {
        gui_shortcut(gui_ShortcutDebugPaste);
        return;
    }

    if (event->key.repeat == 0 && key == SDL_SCANCODE_ESCAPE)
    {
        if (config_emulator.fullscreen && !config_emulator.always_show_menu)
        {
            config_emulator.fullscreen = false;
            application_trigger_fullscreen(false);
        }
    }
}

void events_emu(const SDL_Event* event)
{
    switch(event->type)
    {
        case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
        {
            if (!IsValidPointer(gamepad_controller))
                break;

            SDL_JoystickID id = SDL_GetJoystickID(SDL_GetGamepadJoystick(gamepad_controller));

            if (!config_input.gamepad)
                break;

            if (event->gbutton.which != id)
                break;

            if (event->gbutton.button == config_input.gamepad_A)
                emu_key_pressed(GLYNX_KEY_A);
            else if (event->gbutton.button == config_input.gamepad_B)
                emu_key_pressed(GLYNX_KEY_B);
            else if (event->gbutton.button == config_input.gamepad_pause)
                emu_key_pressed(GLYNX_KEY_PAUSE);
            else if (event->gbutton.button == config_input.gamepad_option1)
                emu_key_pressed(GLYNX_KEY_OPTION1);
            else if (event->gbutton.button == config_input.gamepad_option2)
                emu_key_pressed(GLYNX_KEY_OPTION2);

            if (config_input.gamepad_directional == 1)
                break;

            if (event->gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_UP)
                emu_key_pressed(GLYNX_KEY_UP);
            else if (event->gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_DOWN)
                emu_key_pressed(GLYNX_KEY_DOWN);
            else if (event->gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_LEFT)
                emu_key_pressed(GLYNX_KEY_LEFT);
            else if (event->gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_RIGHT)
                emu_key_pressed(GLYNX_KEY_RIGHT);
        }
        break;

        case SDL_EVENT_GAMEPAD_BUTTON_UP:
        {
            if (!IsValidPointer(gamepad_controller))
                break;

            SDL_JoystickID id = SDL_GetJoystickID(SDL_GetGamepadJoystick(gamepad_controller));

            if (!config_input.gamepad)
                break;

            if (event->gbutton.which != id)
                break;

            if (event->gbutton.button == config_input.gamepad_A)
                emu_key_released(GLYNX_KEY_A);
            else if (event->gbutton.button == config_input.gamepad_B)
                emu_key_released(GLYNX_KEY_B);
            else if (event->gbutton.button == config_input.gamepad_pause)
                emu_key_released(GLYNX_KEY_PAUSE);
            else if (event->gbutton.button == config_input.gamepad_option1)
                emu_key_released(GLYNX_KEY_OPTION1);
            else if (event->gbutton.button == config_input.gamepad_option2)
                emu_key_released(GLYNX_KEY_OPTION2);

            if (config_input.gamepad_directional == 1)
                break;

            if (event->gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_UP)
                emu_key_released(GLYNX_KEY_UP);
            else if (event->gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_DOWN)
                emu_key_released(GLYNX_KEY_DOWN);
            else if (event->gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_LEFT)
                emu_key_released(GLYNX_KEY_LEFT);
            else if (event->gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_RIGHT)
                emu_key_released(GLYNX_KEY_RIGHT);
        }
        break;

        case SDL_EVENT_GAMEPAD_AXIS_MOTION:
        {
            if (!IsValidPointer(gamepad_controller))
                break;

            SDL_JoystickID id = SDL_GetJoystickID(SDL_GetGamepadJoystick(gamepad_controller));

            if (!config_input.gamepad)
                break;

            if (event->gaxis.which != id)
                break;

            if (config_input.gamepad_directional == 1)
            {
                const int STICK_DEAD_ZONE = 8000;

                if(event->gaxis.axis == config_input.gamepad_x_axis)
                {
                    int x_motion = event->gaxis.value * (config_input.gamepad_invert_x_axis ? -1 : 1);

                    if (x_motion < -STICK_DEAD_ZONE)
                        emu_key_pressed(GLYNX_KEY_LEFT);
                    else if (x_motion > STICK_DEAD_ZONE)
                        emu_key_pressed(GLYNX_KEY_RIGHT);
                    else
                    {
                        emu_key_released(GLYNX_KEY_LEFT);
                        emu_key_released(GLYNX_KEY_RIGHT);
                    }
                }
                else if(event->gaxis.axis == config_input.gamepad_y_axis)
                {
                    int y_motion = event->gaxis.value * (config_input.gamepad_invert_y_axis ? -1 : 1);

                    if (y_motion < -STICK_DEAD_ZONE)
                        emu_key_pressed(GLYNX_KEY_UP);
                    else if (y_motion > STICK_DEAD_ZONE)
                        emu_key_pressed(GLYNX_KEY_DOWN);
                    else
                    {
                        emu_key_released(GLYNX_KEY_UP);
                        emu_key_released(GLYNX_KEY_DOWN);
                    }
                }
            }

            if (event->gaxis.axis == SDL_GAMEPAD_AXIS_LEFT_TRIGGER || event->gaxis.axis == SDL_GAMEPAD_AXIS_RIGHT_TRIGGER)
            {
                int vbtn = GAMEPAD_VBTN_AXIS_BASE + event->gaxis.axis;
                bool pressed = event->gaxis.value > GAMEPAD_VBTN_AXIS_THRESHOLD;

                if (config_input.gamepad_A == vbtn)
                {
                    if (pressed)
                        emu_key_pressed(GLYNX_KEY_A);
                    else
                        emu_key_released(GLYNX_KEY_UP);
                }
                else if (config_input.gamepad_B == vbtn)
                {
                    if (pressed)
                        emu_key_pressed(GLYNX_KEY_B);
                    else
                        emu_key_released(GLYNX_KEY_B);
                }
                else if (config_input.gamepad_pause == vbtn)
                {
                    if (pressed)
                        emu_key_pressed(GLYNX_KEY_PAUSE);
                    else
                        emu_key_released(GLYNX_KEY_PAUSE);
                }
                else if (config_input.gamepad_option1 == vbtn)
                {
                    if (pressed)
                        emu_key_pressed(GLYNX_KEY_OPTION1);
                    else
                        emu_key_released(GLYNX_KEY_OPTION1);
                }
                else if (config_input.gamepad_option2 == vbtn)
                {
                    if (pressed)
                        emu_key_pressed(GLYNX_KEY_OPTION2);
                    else
                        emu_key_released(GLYNX_KEY_OPTION2);
                }
            }
        }
        break;

        case SDL_EVENT_KEY_DOWN:
        {
            if (event->key.repeat != 0)
                break;

            if (event->key.mod & SDL_KMOD_CTRL)
                break;
            if (event->key.mod & SDL_KMOD_SHIFT)
                break;

            int key = event->key.scancode;

            if (key == config_input.key_left)
                emu_key_pressed(GLYNX_KEY_LEFT);
            else if (key == config_input.key_right)
                emu_key_pressed(GLYNX_KEY_RIGHT);
            else if (key == config_input.key_up)
                emu_key_pressed(GLYNX_KEY_UP);
            else if (key == config_input.key_down)
                emu_key_pressed(GLYNX_KEY_DOWN);
            else if (key == config_input.key_A)
                emu_key_pressed(GLYNX_KEY_A);
            else if (key == config_input.key_B)
                emu_key_pressed(GLYNX_KEY_B);
            else if (key == config_input.key_pause)
                emu_key_pressed(GLYNX_KEY_PAUSE);
            else if (key == config_input.key_option1)
                emu_key_pressed(GLYNX_KEY_OPTION1);
            else if (key == config_input.key_option2)
                emu_key_pressed(GLYNX_KEY_OPTION2);
        }
        break;

        case SDL_EVENT_KEY_UP:
        {
            int key = event->key.scancode;

            if (key == config_input.key_left)
                emu_key_released(GLYNX_KEY_LEFT);
            else if (key == config_input.key_right)
                emu_key_released(GLYNX_KEY_RIGHT);
            else if (key == config_input.key_up)
                emu_key_released(GLYNX_KEY_UP);
            else if (key == config_input.key_down)
                emu_key_released(GLYNX_KEY_DOWN);
            else if (key == config_input.key_A)
                emu_key_released(GLYNX_KEY_A);
            else if (key == config_input.key_B)
                emu_key_released(GLYNX_KEY_B);
            else if (key == config_input.key_pause)
                emu_key_released(GLYNX_KEY_PAUSE);
            else if (key == config_input.key_option1)
                emu_key_released(GLYNX_KEY_OPTION1);
            else if (key == config_input.key_option2)
                emu_key_released(GLYNX_KEY_OPTION2);
        }
        break;
    }
}

static bool events_check_hotkey(const SDL_Event* event, const config_Hotkey& hotkey, bool allow_repeat)
{
    if (event->type != SDL_EVENT_KEY_DOWN)
        return false;

    if (!allow_repeat && event->key.repeat != 0)
        return false;

    if (event->key.scancode != hotkey.key)
        return false;

    SDL_Keymod mods = event->key.mod;
    SDL_Keymod expected = hotkey.mod;

    SDL_Keymod mods_normalized = (SDL_Keymod)0;
    if (mods & (SDL_KMOD_LCTRL | SDL_KMOD_RCTRL)) mods_normalized = (SDL_Keymod)(mods_normalized | SDL_KMOD_CTRL);
    if (mods & (SDL_KMOD_LSHIFT | SDL_KMOD_RSHIFT)) mods_normalized = (SDL_Keymod)(mods_normalized | SDL_KMOD_SHIFT);
    if (mods & (SDL_KMOD_LALT | SDL_KMOD_RALT)) mods_normalized = (SDL_Keymod)(mods_normalized | SDL_KMOD_ALT);
    if (mods & (SDL_KMOD_LGUI | SDL_KMOD_RGUI)) mods_normalized = (SDL_Keymod)(mods_normalized | SDL_KMOD_GUI);

    SDL_Keymod expected_normalized = (SDL_Keymod)0;
    if (expected & (SDL_KMOD_LCTRL | SDL_KMOD_RCTRL | SDL_KMOD_CTRL)) expected_normalized = (SDL_Keymod)(expected_normalized | SDL_KMOD_CTRL);
    if (expected & (SDL_KMOD_LSHIFT | SDL_KMOD_RSHIFT | SDL_KMOD_SHIFT)) expected_normalized = (SDL_Keymod)(expected_normalized | SDL_KMOD_SHIFT);
    if (expected & (SDL_KMOD_LALT | SDL_KMOD_RALT | SDL_KMOD_ALT)) expected_normalized = (SDL_Keymod)(expected_normalized | SDL_KMOD_ALT);
    if (expected & (SDL_KMOD_LGUI | SDL_KMOD_RGUI | SDL_KMOD_GUI)) expected_normalized = (SDL_Keymod)(expected_normalized | SDL_KMOD_GUI);

    return mods_normalized == expected_normalized;
}
