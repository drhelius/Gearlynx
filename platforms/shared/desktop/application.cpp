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
static void sdl_events(void);
static void sdl_events_emu(const SDL_Event* event);
static void sdl_shortcuts_gui(const SDL_Event* event);
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

    application_gamepad_mappings = SDL_GameControllerAddMappingsFromRW(SDL_RWFromFile("gamecontrollerdb.txt", "rb"), 1);

    if (application_gamepad_mappings > 0)
    {
        Log("Succesfuly loaded %d game controller mappings from gamecontrollerdb.txt", application_gamepad_mappings);
    }
    else
    {
        Log("ERROR: Game controller database not found (gamecontrollerdb.txt)!!");
    }

    int gamepads_found = 0;

    for (int i = 0; i < SDL_NumJoysticks(); ++i)
    {
        if (SDL_IsGameController(i))
        {
            application_gamepad = SDL_GameControllerOpen(i);
            if(!application_gamepad)
            {
                Log("ERROR: Unable to open game controller %d! SDL Error: %s\n", gamepads_found, SDL_GetError());
            }
            else
            {
                Debug("Game controller %d correctly detected", i);
                gamepads_found++;

                if (gamepads_found > 1)
                    break;
            }
        }
    }

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
        if (event.type == SDL_QUIT)
        {
            running = false;
            break;
        }

        if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(application_sdl_window))
        {
            running = false;
            break;
        }

        ImGui_ImplSDL2_ProcessEvent(&event);

        if (!gui_in_use)
        {
            sdl_events_emu(&event);
            sdl_shortcuts_gui(&event);
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

        // case SDL_CONTROLLERBUTTONDOWN:
        // {
        //     for (int i = 0; i < 2; i++)
        //     {
        //         GC_Controllers controller = (i == 0) ? Controller_1 : Controller_2;
        //         SDL_JoystickID id = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(application_gamepad[i]));

        //         if (!config_input[i].gamepad)
        //             continue;

        //         if (event->cbutton.which != id)
        //             continue;
                
        //         if (event->cbutton.button == config_input[i].gamepad_left_button)
        //             emu_key_pressed(controller, Key_Left_Button);
        //         else if (event->cbutton.button == config_input[i].gamepad_right_button)
        //             emu_key_pressed(controller, Key_Right_Button);
        //         else if (event->jbutton.button == config_input[i].gamepad_0)
        //             emu_key_pressed(controller, Keypad_0);
        //         else if (event->jbutton.button == config_input[i].gamepad_1)
        //             emu_key_pressed(controller, Keypad_1);
        //         else if (event->jbutton.button == config_input[i].gamepad_2)
        //             emu_key_pressed(controller, Keypad_2);
        //         else if (event->jbutton.button == config_input[i].gamepad_3)
        //             emu_key_pressed(controller, Keypad_3);
        //         else if (event->jbutton.button == config_input[i].gamepad_4)
        //             emu_key_pressed(controller, Keypad_4);
        //         else if (event->jbutton.button == config_input[i].gamepad_5)
        //             emu_key_pressed(controller, Keypad_5);
        //         else if (event->jbutton.button == config_input[i].gamepad_6)
        //             emu_key_pressed(controller, Keypad_6);
        //         else if (event->jbutton.button == config_input[i].gamepad_7)
        //             emu_key_pressed(controller, Keypad_7);
        //         else if (event->jbutton.button == config_input[i].gamepad_8)
        //             emu_key_pressed(controller, Keypad_8);
        //         else if (event->jbutton.button == config_input[i].gamepad_9)
        //             emu_key_pressed(controller, Keypad_9);
        //         else if (event->jbutton.button == config_input[i].gamepad_asterisk)
        //             emu_key_pressed(controller, Keypad_Asterisk);
        //         else if (event->jbutton.button == config_input[i].gamepad_hash)
        //             emu_key_pressed(controller, Keypad_Hash);

        //         if (config_input[i].gamepad_directional == 1)
        //             continue;
                
        //         if (event->cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_UP)
        //             emu_key_pressed(controller, Key_Up);
        //         else if (event->cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_DOWN)
        //             emu_key_pressed(controller, Key_Down);
        //         else if (event->cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_LEFT)
        //             emu_key_pressed(controller, Key_Left);
        //         else if (event->cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_RIGHT)
        //             emu_key_pressed(controller, Key_Right);
        //     }
        // }
        // break;

        // case SDL_CONTROLLERBUTTONUP:
        // {
        //     for (int i = 0; i < 2; i++)
        //     {
        //         GC_Controllers controller = (i == 0) ? Controller_1 : Controller_2;
        //         SDL_JoystickID id = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(application_gamepad[i]));

        //         if (!config_input[i].gamepad)
        //             continue;

        //         if (event->jbutton.which != id)
        //             continue;

        //         if (event->jbutton.button == config_input[i].gamepad_left_button)
        //             emu_key_released(controller, Key_Left_Button);
        //         else if (event->jbutton.button == config_input[i].gamepad_right_button)
        //             emu_key_released(controller, Key_Right_Button);
        //         else if (event->jbutton.button == config_input[i].gamepad_0)
        //             emu_key_released(controller, Keypad_0);
        //         else if (event->jbutton.button == config_input[i].gamepad_1)
        //             emu_key_released(controller, Keypad_1);
        //         else if (event->jbutton.button == config_input[i].gamepad_2)
        //             emu_key_released(controller, Keypad_2);
        //         else if (event->jbutton.button == config_input[i].gamepad_3)
        //             emu_key_released(controller, Keypad_3);
        //         else if (event->jbutton.button == config_input[i].gamepad_4)
        //             emu_key_released(controller, Keypad_4);
        //         else if (event->jbutton.button == config_input[i].gamepad_5)
        //             emu_key_released(controller, Keypad_5);
        //         else if (event->jbutton.button == config_input[i].gamepad_6)
        //             emu_key_released(controller, Keypad_6);
        //         else if (event->jbutton.button == config_input[i].gamepad_7)
        //             emu_key_released(controller, Keypad_7);
        //         else if (event->jbutton.button == config_input[i].gamepad_8)
        //             emu_key_released(controller, Keypad_8);
        //         else if (event->jbutton.button == config_input[i].gamepad_9)
        //             emu_key_released(controller, Keypad_9);
        //         else if (event->jbutton.button == config_input[i].gamepad_asterisk)
        //             emu_key_released(controller, Keypad_Asterisk);
        //         else if (event->jbutton.button == config_input[i].gamepad_hash)
        //             emu_key_released(controller, Keypad_Hash);

        //         if (config_input[i].gamepad_directional == 1)
        //             continue;
                
        //         if (event->cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_UP)
        //             emu_key_released(controller, Key_Up);
        //         else if (event->cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_DOWN)
        //             emu_key_released(controller, Key_Down);
        //         else if (event->cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_LEFT)
        //             emu_key_released(controller, Key_Left);
        //         else if (event->cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_RIGHT)
        //             emu_key_released(controller, Key_Right);
        //     }
        // }
        // break;

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
