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
#include "utils.h"

#define GAMEPAD_IMPORT
#include "gamepad.h"

static bool gamepad_shortcut_prev[config_HotkeyIndex_COUNT] = { };

static bool gamepad_get_button(SDL_Gamepad* controller, int mapping);

bool gamepad_init(void)
{
    InitPointer(gamepad_controller);
    return true;
}

void gamepad_destroy(void)
{
    SDL_CloseGamepad(gamepad_controller);
}

void gamepad_load_mappings(void)
{
    std::string db_path;
    char exe_path[1024] = { };
    get_executable_path(exe_path, sizeof(exe_path));

    if (exe_path[0] != '\0')
    {
        db_path = std::string(exe_path) + "/gamecontrollerdb.txt";
    }
    else
    {
        db_path = "gamecontrollerdb.txt";
    }

    std::ifstream file;
    open_ifstream_utf8(file, db_path.c_str(), std::ios::in);
    if (!file.is_open())
    {
        open_ifstream_utf8(file, "gamecontrollerdb.txt", std::ios::in);
    }

    int added_mappings = 0;
    int updated_mappings = 0;
    int line_number = 0;
    char platform_field[64] = { };
    snprintf(platform_field, 64, "platform:%s", SDL_GetPlatform());
    if (file.is_open())
    {
        Debug("Loading gamecontrollerdb.txt file");
        std::string line;
        while (std::getline(file, line))
        {
            size_t comment = line.find_first_of('#');
            if (comment != std::string::npos)
                line = line.substr(0, comment);
            line = line.erase(0, line.find_first_not_of(" \t\r\n"));
            line = line.erase(line.find_last_not_of(" \t\r\n") + 1);
            while (line[0] == ' ')
                line = line.substr(1);
            if (line.empty())
                continue;
            if ((line.find("platform:") != std::string::npos) && (line.find(platform_field) == std::string::npos))
                continue;
            int result = SDL_AddGamepadMapping(line.c_str());
            if (result == 1)
                added_mappings++;
            else if (result == 0)
                updated_mappings++;
            else if (result == -1)
            {
                Error("Unable to load game controller mapping in line %d from gamecontrollerdb.txt", line_number);
                SDL_ERROR("SDL_AddGamepadMapping");
            }
            line_number++;
        }
        file.close();
    }
    else
    {
        Error("Game controller database not found (gamecontrollerdb.txt)!!");
        return;
    }
    Log("Added %d new game controller mappings from gamecontrollerdb.txt", added_mappings);
    Log("Updated %d game controller mappings from gamecontrollerdb.txt", updated_mappings);
    gamepad_added_mappings = added_mappings;
    gamepad_updated_mappings = updated_mappings;
}

void gamepad_add(void)
{
    if (IsValidPointer(gamepad_controller))
    {
        SDL_Joystick* js = SDL_GetGamepadJoystick(gamepad_controller);

        if (!IsValidPointer(js) || !SDL_JoystickConnected(js))
        {
            SDL_CloseGamepad(gamepad_controller);
            gamepad_controller = NULL;
            Debug("Game controller closed when adding a new gamepad");
        }
    }

    if (IsValidPointer(gamepad_controller))
        return;

    int count = 0;
    SDL_JoystickID* gamepads = SDL_GetGamepads(&count);
    if (gamepads)
    {
        for (int i = 0; i < count; i++)
        {
            SDL_Gamepad* controller = SDL_OpenGamepad(gamepads[i]);
            if (!IsValidPointer(controller))
            {
                Log("Warning: Unable to open game controller %d!", gamepads[i]);
                SDL_ERROR("SDL_OpenGamepad");
                continue;
            }

            gamepad_controller = controller;
            Debug("Game controller %d assigned to Player 1", gamepads[i]);
            break;
        }
        SDL_free(gamepads);
    }
}

void gamepad_remove(SDL_JoystickID instance_id)
{
    if (gamepad_controller != NULL)
    {
        SDL_JoystickID current_id = SDL_GetJoystickID(SDL_GetGamepadJoystick(gamepad_controller));
        if (current_id == instance_id)
        {
            SDL_CloseGamepad(gamepad_controller);
            gamepad_controller = NULL;
            Debug("Game controller %d disconnected", instance_id);
        }
    }
}

void gamepad_assign(SDL_JoystickID instance_id)
{
    if (instance_id == 0)
    {
        if (IsValidPointer(gamepad_controller))
        {
            SDL_CloseGamepad(gamepad_controller);
            gamepad_controller = NULL;
            Debug("Player controller set to None");
        }
        return;
    }

    if (IsValidPointer(gamepad_controller))
    {
        SDL_JoystickID current_id = SDL_GetJoystickID(SDL_GetGamepadJoystick(gamepad_controller));
        if (current_id == instance_id)
            return;
    }

    if (IsValidPointer(gamepad_controller))
    {
        SDL_CloseGamepad(gamepad_controller);
        gamepad_controller = NULL;
    }

    SDL_Gamepad* controller = SDL_OpenGamepad(instance_id);
    if (!IsValidPointer(controller))
    {
        Log("SDL_OpenGamepad failed for instance %d", instance_id);
        SDL_ERROR("SDL_OpenGamepad");
        return;
    }

    gamepad_controller = controller;
    Debug("Game controller %d assigned", instance_id);
}

void gamepad_check_shortcuts(void)
{
    SDL_Gamepad* sdl_controller = gamepad_controller;
    if (!IsValidPointer(sdl_controller))
        return;

    for (int i = 0; i < config_HotkeyIndex_COUNT; i++)
    {
        int button_mapping = config_input_gamepad_shortcuts.gamepad_shortcuts[i];
        if (button_mapping == SDL_GAMEPAD_BUTTON_INVALID)
            continue;

        bool button_pressed = gamepad_get_button(sdl_controller, button_mapping);

        if (button_pressed && !gamepad_shortcut_prev[i])
        {
            if (i >= config_HotkeyIndex_SelectSlot1 && i <= config_HotkeyIndex_SelectSlot5)
            {
                config_emulator.save_slot = i - config_HotkeyIndex_SelectSlot1;
            }
            else
            {
                for (int j = 0; j < GUI_HOTKEY_MAP_COUNT; j++)
                {
                    if (gui_hotkey_map[j].config_index == i)
                    {
                        gui_shortcut((gui_ShortCutEvent)gui_hotkey_map[j].shortcut);
                        break;
                    }
                }
            }
        }

        gamepad_shortcut_prev[i] = button_pressed;
    }
}

void gamepad_get_detected(Gamepad_Detected_Info* info)
{
    info->count = 0;
    info->current_index = -1;

    int num_gamepads = 0;
    SDL_JoystickID* gamepads = SDL_GetGamepads(&num_gamepads);

    SDL_JoystickID current_id = 0;
    if (IsValidPointer(gamepad_controller))
        current_id = SDL_GetJoystickID(SDL_GetGamepadJoystick(gamepad_controller));

    if (gamepads)
    {
        for (int i = 0; i < num_gamepads && info->count < GAMEPAD_MAX_DETECTED; i++)
        {
            SDL_JoystickID id = gamepads[i];

            const char* name = SDL_GetGamepadNameForID(id);
            if (!IsValidPointer(name))
                name = "Unknown Gamepad";

            info->ids[info->count] = id;

            if (current_id == id)
                info->current_index = info->count;

            char id_str[64];
            SDL_GUID guid = SDL_GetJoystickGUIDForID(id);
            SDL_GUIDToString(guid, id_str, sizeof(id_str));
            size_t len = strlen(id_str);
            const char* id_8 = id_str + (len > 8 ? len - 8 : 0);

            snprintf(info->labels[info->count], sizeof(info->labels[0]), "%s (ID: %s)", name, id_8);
            info->count++;
        }
        SDL_free(gamepads);
    }
}

static bool gamepad_get_button(SDL_Gamepad* controller, int mapping)
{
    if (!IsValidPointer(controller))
        return false;

    if (mapping >= 0 && mapping < SDL_GAMEPAD_BUTTON_COUNT)
    {
        return SDL_GetGamepadButton(controller, (SDL_GamepadButton)mapping) != 0;
    }
    else if (mapping >= GAMEPAD_VBTN_AXIS_BASE)
    {
        int axis = mapping - GAMEPAD_VBTN_AXIS_BASE;
        Sint16 value = SDL_GetGamepadAxis(controller, (SDL_GamepadAxis)axis);
        return value > GAMEPAD_VBTN_AXIS_THRESHOLD;
    }

    return false;
}
