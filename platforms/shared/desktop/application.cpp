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

#include <SDL.h>
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl2.h"
#include "../../../src/gearlynx.h"
#include "config.h"
#include "gui.h"
#include "gui_debug_disassembler.h"
#include "renderer.h"
#include "emu.h"

#define APPLICATION_IMPORT
#include "application.h"

#if defined(GLYNX_DEBUG)
#define WINDOW_TITLE GLYNX_TITLE " " GLYNX_VERSION " (DEBUG)"
#else
#define WINDOW_TITLE GLYNX_TITLE " " GLYNX_VERSION
#endif

static SDL_GLContext gl_context;
static bool running = true;
static bool paused_when_focus_lost = false;
static Uint64 frame_time_start;
static Uint64 frame_time_end;

static int sdl_init(void);
static void sdl_destroy(void);
static void sdl_load_gamepad_mappings(void);
static void sdl_events(void);
static void sdl_events_app(const SDL_Event* event);
static void sdl_events_emu(const SDL_Event* event);
static void sdl_shortcuts_gui(const SDL_Event* event);
static void sdl_add_gamepads(void);
static void sdl_remove_gamepad(SDL_JoystickID instance_id);
static void handle_mouse_cursor(void);
static void run_emulator(void);
static void render(void);
static void frame_throttle(void);
static void save_window_size(void);

int application_init(const char* rom_file, const char* symbol_file)
{
    Log("\n%s", GLYNX_TITLE_ASCII);
    Log("%s %s Desktop App", GLYNX_TITLE, GLYNX_VERSION);

    config_init();
    config_read();

    int ret = sdl_init();
    emu_init();

    gui_init();

    ImGui_ImplSDL2_InitForOpenGL(application_sdl_window, gl_context);

    renderer_init();

    SDL_GL_SetSwapInterval(config_video.sync ? 1 : 0);

    if (config_emulator.fullscreen)
        application_trigger_fullscreen(true);

    if (IsValidPointer(rom_file) && (strlen(rom_file) > 0))
    {
        Debug("Rom file argument: %s", rom_file);
        gui_load_rom(rom_file);
    }
    if (IsValidPointer(symbol_file) && (strlen(symbol_file) > 0))
    {
        Debug("Symbol file argument: %s", symbol_file);
        gui_debug_reset_symbols();
        gui_debug_load_symbols_file(symbol_file);
    }

    return ret;
}

void application_destroy(void)
{
    save_window_size();
    config_write();
    config_destroy();
    renderer_destroy();
    ImGui_ImplSDL2_Shutdown();
    gui_destroy();
    emu_destroy();
    sdl_destroy();
}

void application_mainloop(void)
{
    while (running)
    {
        frame_time_start = SDL_GetPerformanceCounter();
        sdl_events();
        handle_mouse_cursor();
        run_emulator();
        render();
        frame_time_end = SDL_GetPerformanceCounter();
        frame_throttle();
    }
}

void application_trigger_quit(void)
{
    SDL_Event event;
    event.type = SDL_QUIT;
    SDL_PushEvent(&event);
}

void application_trigger_fullscreen(bool fullscreen)
{
    SDL_SetWindowFullscreen(application_sdl_window, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
}

void application_trigger_fit_to_content(int width, int height)
{
    SDL_SetWindowSize(application_sdl_window, width, height);
}

void application_update_title_with_rom(const char* rom)
{
    char final_title[256];
    snprintf(final_title, 256, "%s - %s", WINDOW_TITLE, rom);
    SDL_SetWindowTitle(application_sdl_window, final_title);
}

static int sdl_init(void)
{
    Debug("Initializing SDL...");

    InitPointer(application_gamepad);

#if defined(_WIN32)
    SDL_SetHint(SDL_HINT_WINDOWS_DPI_SCALING, "1");
#endif
    
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        Log("ERROR: %s\n", SDL_GetError());
        return 1;
    }

    SDL_VERSION(&application_sdl_build_version);
    SDL_GetVersion(&application_sdl_link_version);

    Log("Using SDL %d.%d.%d (build)", application_sdl_build_version.major, application_sdl_build_version.minor, application_sdl_build_version.patch);
    Log("Using SDL %d.%d.%d (link) ", application_sdl_link_version.major, application_sdl_link_version.minor, application_sdl_link_version.patch);

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

    if (config_emulator.maximized)
        window_flags = (SDL_WindowFlags)(window_flags | SDL_WINDOW_MAXIMIZED);

    application_sdl_window = SDL_CreateWindow(WINDOW_TITLE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, config_emulator.window_width, config_emulator.window_height, window_flags);
    gl_context = SDL_GL_CreateContext(application_sdl_window);
    SDL_GL_MakeCurrent(application_sdl_window, gl_context);
    SDL_GL_SetSwapInterval(0);

    SDL_SetWindowMinimumSize(application_sdl_window, 500, 300);

    sdl_load_gamepad_mappings();
    sdl_add_gamepads();

    int w, h;
    int display_w, display_h;
    SDL_GetWindowSize(application_sdl_window, &w, &h);
    SDL_GL_GetDrawableSize(application_sdl_window, &display_w, &display_h);
    
    if (w > 0 && h > 0)
    {
        float scale_w = (float)display_w / w;
        float scale_h = (float)display_h / h;

        application_display_scale = (scale_w > scale_h) ? scale_w : scale_h;
    }

    SDL_EventState(SDL_DROPFILE, SDL_ENABLE);

    return 0;
}

static void sdl_destroy(void)
{
    SDL_GameControllerClose(application_gamepad);
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(application_sdl_window);
    SDL_Quit();
}

static void sdl_load_gamepad_mappings(void)
{
    std::ifstream file("gamecontrollerdb.txt");

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

            int result = SDL_GameControllerAddMapping(line.c_str());
            if (result == 1)
                added_mappings++;
            else if (result == 0)
                updated_mappings++;
            else if (result == -1)
            {
                Log("ERROR: Unable to load game controller mapping in line %d from gamecontrollerdb.txt", line_number);
                Log("SDL: %s", SDL_GetError());
            }

            line_number++;
        }

        file.close();
    }
    else
    {
        Log("ERROR: Game controller database not found (gamecontrollerdb.txt)!!");
        return;
    }

    Log("Added %d new game controller mappings from gamecontrollerdb.txt", added_mappings);
    Log("Updated %d game controller mappings from gamecontrollerdb.txt", updated_mappings);

    application_added_gamepad_mappings = added_mappings;
    application_updated_gamepad_mappings = updated_mappings;
}

static void handle_mouse_cursor(void)
{
    bool hide_cursor = false;

    if (gui_main_window_hovered && !config_debug.debug)
        hide_cursor = true;

    if (!config_emulator.show_menu && !config_debug.debug)
        hide_cursor = true;

    if (hide_cursor)
        ImGui::SetMouseCursor(ImGuiMouseCursor_None);
    else
        ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
}

static void sdl_events(void)
{
    SDL_Event event;
        
    while (SDL_PollEvent(&event))
    {
        sdl_events_app(&event);

        if (running)
        {
            ImGui_ImplSDL2_ProcessEvent(&event);

            if (!gui_in_use)
            {
                sdl_events_emu(&event);
                sdl_shortcuts_gui(&event);
            }
        }
    }
}

static void sdl_events_app(const SDL_Event* event)
{
    if (event->type == SDL_QUIT)
    {
        running = false;
        return;
    }

    if (event->type == SDL_WINDOWEVENT && event->window.event == SDL_WINDOWEVENT_CLOSE && event->window.windowID == SDL_GetWindowID(application_sdl_window))
    {
        running = false;
        return;
    }

    switch (event->type)
    {
        case SDL_CONTROLLERDEVICEADDED:
        {
            sdl_add_gamepads();
            break;
        }
        case SDL_CONTROLLERDEVICEREMOVED:
        {
            sdl_remove_gamepad(event->cdevice.which);
            break;
        }
    }
}

static void sdl_events_emu(const SDL_Event* event)
{
    switch(event->type)
    {
         case (SDL_DROPFILE):
        {
            char* dropped_filedir = event->drop.file;
            gui_load_rom(dropped_filedir);
            SDL_free(dropped_filedir);
            SDL_SetWindowInputFocus(application_sdl_window);
        }
        break;

        case SDL_WINDOWEVENT:
        {
            switch (event->window.event)
            {
                case SDL_WINDOWEVENT_FOCUS_GAINED:
                {
                    if (!paused_when_focus_lost)
                        emu_resume();
                }
                break;

                case SDL_WINDOWEVENT_FOCUS_LOST:
                {
                    paused_when_focus_lost = emu_is_paused();
                    emu_pause();
                }
                break;
            }
        }
        break;

        case SDL_CONTROLLERBUTTONDOWN:
        {
            if (!IsValidPointer(application_gamepad))
                break;

            SDL_JoystickID id = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(application_gamepad));

            if (!config_input.gamepad)
                break;

            if (event->cbutton.which != id)
                break;

            if (event->cbutton.button == config_input.gamepad_A)
                emu_key_pressed(GLYNX_KEY_A);
            else if (event->cbutton.button == config_input.gamepad_B)
                emu_key_pressed(GLYNX_KEY_B);
            else if (event->cbutton.button == config_input.gamepad_start)
                emu_key_pressed(GLYNX_KEY_START);
            else if (event->cbutton.button == config_input.gamepad_option1)
                emu_key_pressed(GLYNX_KEY_OPTION1);
            else if (event->cbutton.button == config_input.gamepad_option2)
                emu_key_pressed(GLYNX_KEY_OPTION2);

            if (config_input.gamepad_directional == 1)
                break;

            if (event->cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_UP)
                emu_key_pressed(GLYNX_KEY_UP);
            else if (event->cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_DOWN)
                emu_key_pressed(GLYNX_KEY_DOWN);
            else if (event->cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_LEFT)
                emu_key_pressed(GLYNX_KEY_LEFT);
            else if (event->cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_RIGHT)
                emu_key_pressed(GLYNX_KEY_RIGHT);
        }
        break;

        case SDL_CONTROLLERBUTTONUP:
        {
            if (!IsValidPointer(application_gamepad))
                break;

            SDL_JoystickID id = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(application_gamepad));

            if (!config_input.gamepad)
                break;

            if (event->cbutton.which != id)
                break;

            if (event->cbutton.button == config_input.gamepad_A)
                emu_key_released(GLYNX_KEY_A);
            else if (event->cbutton.button == config_input.gamepad_B)
                emu_key_released(GLYNX_KEY_B);
            else if (event->cbutton.button == config_input.gamepad_start)
                emu_key_released(GLYNX_KEY_START);
            else if (event->cbutton.button == config_input.gamepad_option1)
                emu_key_released(GLYNX_KEY_OPTION1);
            else if (event->cbutton.button == config_input.gamepad_option2)
                emu_key_released(GLYNX_KEY_OPTION2);

            if (config_input.gamepad_directional == 1)
                break;

            if (event->cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_UP)
                emu_key_released(GLYNX_KEY_UP);
            else if (event->cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_DOWN)
                emu_key_released(GLYNX_KEY_DOWN);
            else if (event->cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_LEFT)
                emu_key_released(GLYNX_KEY_LEFT);
            else if (event->cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_RIGHT)
                emu_key_released(GLYNX_KEY_RIGHT);
        }
        break;

        case SDL_CONTROLLERAXISMOTION:
        {
            if (!IsValidPointer(application_gamepad))
                break;

            SDL_JoystickID id = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(application_gamepad));

            if (!config_input.gamepad)
                break;

            if (config_input.gamepad_directional == 0)
                break;

            if (event->caxis.which != id)
                break;

            const int STICK_DEAD_ZONE = 8000;

            if(event->caxis.axis == config_input.gamepad_x_axis)
            {
                int x_motion = event->caxis.value * (config_input.gamepad_invert_x_axis ? -1 : 1);

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
            else if(event->caxis.axis == config_input.gamepad_y_axis)
            {
                int y_motion = event->caxis.value * (config_input.gamepad_invert_y_axis ? -1 : 1);

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
        break;

        case SDL_KEYDOWN:
        {
            if (event->key.repeat != 0)
                break;

            if (event->key.keysym.mod & KMOD_CTRL)
                break;
            if (event->key.keysym.mod & KMOD_SHIFT)
                break;

            int key = event->key.keysym.scancode;

            if (key == SDL_SCANCODE_ESCAPE)
            {
                application_trigger_quit();
                break;
            }

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
            else if (key == config_input.key_start)
                emu_key_pressed(GLYNX_KEY_START);
            else if (key == config_input.key_option1)
                emu_key_pressed(GLYNX_KEY_OPTION1);
            else if (key == config_input.key_option2)
                emu_key_pressed(GLYNX_KEY_OPTION2);
        }
        break;

        case SDL_KEYUP:
        {
            int key = event->key.keysym.scancode;

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
            else if (key == config_input.key_start)
                emu_key_released(GLYNX_KEY_START);
            else if (key == config_input.key_option1)
                emu_key_released(GLYNX_KEY_OPTION1);
            else if (key == config_input.key_option2)
                emu_key_released(GLYNX_KEY_OPTION2);
        }
        break;
    }
}

static void sdl_shortcuts_gui(const SDL_Event* event)
{
    if (event->type == SDL_KEYDOWN)
    {
        int key = event->key.keysym.scancode;

        switch (key)
        {
            case SDL_SCANCODE_A:
                if (event->key.keysym.mod & KMOD_CTRL)
                    gui_shortcut(gui_ShortcutDebugSelectAll);
                break;
            case SDL_SCANCODE_C:
                if (event->key.keysym.mod & KMOD_CTRL)
                    gui_shortcut(gui_ShortcutDebugCopy);
                break;
            case SDL_SCANCODE_V:
                if (event->key.keysym.mod & KMOD_CTRL)
                    gui_shortcut(gui_ShortcutDebugPaste);
                break;
            case SDL_SCANCODE_O:
                if (event->key.keysym.mod & KMOD_CTRL)
                    gui_shortcut(gui_ShortcutOpenROM);
                break;
            case SDL_SCANCODE_R:
                if (event->key.keysym.mod & KMOD_CTRL)
                    gui_shortcut(gui_ShortcutReset);
                break;
            case SDL_SCANCODE_P:
                if (event->key.keysym.mod & KMOD_CTRL)
                    gui_shortcut(gui_ShortcutPause);
                break;
            case SDL_SCANCODE_F:
                if (event->key.keysym.mod & KMOD_CTRL)
                    gui_shortcut(gui_ShortcutFFWD);
                break;
            case SDL_SCANCODE_L:
                if (event->key.keysym.mod & KMOD_CTRL)
                    gui_shortcut(gui_ShortcutLoadState);
                break;
            case SDL_SCANCODE_S:
                if (event->key.keysym.mod & KMOD_CTRL)
                    gui_shortcut(gui_ShortcutSaveState);
                break;
            case SDL_SCANCODE_X:
                if (event->key.keysym.mod & KMOD_CTRL)
                    gui_shortcut(gui_ShortcutScreenshot);
                break;
            case SDL_SCANCODE_M:
                if (event->key.keysym.mod & KMOD_CTRL)
                    gui_shortcut(gui_ShortcutShowMainMenu);
                break;
            case SDL_SCANCODE_F5:
                gui_shortcut(gui_ShortcutDebugContinue);
                break;
            case SDL_SCANCODE_F6:
                gui_shortcut(gui_ShortcutDebugStepFrame);
                break;
            case SDL_SCANCODE_F7:
                gui_shortcut(gui_ShortcutDebugBreak);
                break;
            case SDL_SCANCODE_F8:
                gui_shortcut(gui_ShortcutDebugRuntocursor);
                break;
            case SDL_SCANCODE_F9:
                gui_shortcut(gui_ShortcutDebugBreakpoint);
                break;
            case SDL_SCANCODE_F10:
                gui_shortcut(gui_ShortcutDebugStepOver);
                break;
            case SDL_SCANCODE_F11:
                if (event->key.keysym.mod & KMOD_SHIFT)
                    gui_shortcut(gui_ShortcutDebugStepOut);
                else
                    gui_shortcut(gui_ShortcutDebugStepInto);
                break;
            case SDL_SCANCODE_F12:
                config_emulator.fullscreen = !config_emulator.fullscreen;
                application_trigger_fullscreen(config_emulator.fullscreen);
                break;
            case SDL_SCANCODE_BACKSPACE:
                if (event->key.keysym.mod & KMOD_CTRL)
                    gui_shortcut(gui_ShortcutDebugGoBack);
                break;
        }
    }
}

static void sdl_add_gamepads(void)
{
    bool connected = IsValidPointer(application_gamepad);

    if (connected)
        return;

    for (int i = 0; i < SDL_NumJoysticks(); i++)
    {
        if (!SDL_IsGameController(i))
            continue;

        SDL_GameController* controller = SDL_GameControllerOpen(i);
        if (!IsValidPointer(controller))
        {
            Log("Warning: Unable to open game controller %d! SDL Error: %s\n", i, SDL_GetError());
            continue;
        }

        if (!connected)
        {
            application_gamepad = controller;
            connected = true;
            Debug("Game controller %d assigned to Player 1", i);
        }
        else
        {
            SDL_GameControllerClose(controller);
            Debug("Game controller %d detected but all player slots are full", i);
        }

        if (connected)
            break;
    }
}

static void sdl_remove_gamepad(SDL_JoystickID instance_id)
{
    if (application_gamepad != NULL)
    {
        SDL_JoystickID current_id = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(application_gamepad));
        if (current_id == instance_id)
        {
            SDL_GameControllerClose(application_gamepad);
            application_gamepad = NULL;
            Debug("Game controller %d disconnected", instance_id);
        }
    }
}

static void run_emulator(void)
{
    config_emulator.paused = emu_is_paused();
    emu_audio_sync = config_audio.sync;
    emu_update();
}

static void render(void)
{
    renderer_begin_render();
    ImGui_ImplSDL2_NewFrame();
    gui_render();
    renderer_render();
    renderer_end_render();

    SDL_GL_SwapWindow(application_sdl_window);
}

static void frame_throttle(void)
{
    if (emu_is_empty() || emu_is_paused() || emu_is_debug_idle() || !emu_is_audio_open() || config_emulator.ffwd)
    {
        Uint64 count_per_sec = SDL_GetPerformanceFrequency();
        float elapsed = (float)(frame_time_end - frame_time_start) / (float)count_per_sec;
        elapsed *= 1000.0f;

        float min = 16.666f;

        if (config_emulator.ffwd)
        {
            switch (config_emulator.ffwd_speed)
            {
                case 0:
                    min = 16.666f / 1.5f;
                    break;
                case 1: 
                    min = 16.666f / 2.0f;
                    break;
                case 2:
                    min = 16.666f / 2.5f;
                    break;
                case 3:
                    min = 16.666f / 3.0f;
                    break;
                default:
                    min = 0.0f;
            }
        }

        if (elapsed < min)
            SDL_Delay((Uint32)(min - elapsed));
    }
}

static void save_window_size(void)
{
    if (!config_emulator.fullscreen)
    {
        int width, height;
        SDL_GetWindowSize(application_sdl_window, &width, &height);
        config_emulator.window_width = width;
        config_emulator.window_height = height;
        config_emulator.maximized = (SDL_GetWindowFlags(application_sdl_window) & SDL_WINDOW_MAXIMIZED);
    }
}
