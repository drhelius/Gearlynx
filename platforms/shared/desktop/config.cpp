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
#include <iomanip>
#include "gearlynx.h"

#define MINI_CASE_SENSITIVE
#include "ini.h"

#define CONFIG_IMPORT
#include "config.h"

static bool check_portable(void);
static int read_int(const char* group, const char* key, int default_value);
static void write_int(const char* group, const char* key, int integer);
static float read_float(const char* group, const char* key, float default_value);
static void write_float(const char* group, const char* key, float value);
static bool read_bool(const char* group, const char* key, bool default_value);
static void write_bool(const char* group, const char* key, bool boolean);
static std::string read_string(const char* group, const char* key);
static void write_string(const char* group, const char* key, std::string value);

void config_init(void)
{
    if (check_portable())
        config_root_path = SDL_GetBasePath();
    else
        config_root_path = SDL_GetPrefPath("Geardome", GLYNX_TITLE);

    strncpy_fit(config_emu_file_path, config_root_path, sizeof(config_emu_file_path));
    strncat_fit(config_emu_file_path, "config.ini", sizeof(config_emu_file_path));

    strncpy_fit(config_imgui_file_path, config_root_path, sizeof(config_imgui_file_path));
    strncat_fit(config_imgui_file_path, "imgui.ini", sizeof(config_imgui_file_path));

    config_input.key_left = SDL_SCANCODE_LEFT;
    config_input.key_right = SDL_SCANCODE_RIGHT;
    config_input.key_up = SDL_SCANCODE_UP;
    config_input.key_down = SDL_SCANCODE_DOWN;
    config_input.key_pause = SDL_SCANCODE_A;
    config_input.key_option1 = SDL_SCANCODE_S;
    config_input.key_option2 = SDL_SCANCODE_D;
    config_input.key_A = SDL_SCANCODE_Z;
    config_input.key_B = SDL_SCANCODE_X;
    config_input.gamepad = true;
    config_input.gamepad_invert_x_axis = false;
    config_input.gamepad_invert_y_axis = false;
    config_input.gamepad_pause = SDL_CONTROLLER_BUTTON_START;
    config_input.gamepad_option1 = SDL_CONTROLLER_BUTTON_X;
    config_input.gamepad_option2 = SDL_CONTROLLER_BUTTON_Y;
    config_input.gamepad_A = SDL_CONTROLLER_BUTTON_A;
    config_input.gamepad_B = SDL_CONTROLLER_BUTTON_B;
    config_input.gamepad_x_axis = SDL_CONTROLLER_AXIS_LEFTX;
    config_input.gamepad_y_axis = SDL_CONTROLLER_AXIS_LEFTY;

    config_ini_file = new mINI::INIFile(config_emu_file_path);
}

void config_destroy(void)
{
    SafeDelete(config_ini_file)
    SDL_free(config_root_path);
}

void config_read(void)
{
    if (!config_ini_file->read(config_ini_data))
    {
        Log("Unable to load settings from %s", config_emu_file_path);
        return;
    }

    Log("Loading settings from %s", config_emu_file_path);

#if defined(GLYNX_DISABLE_DISASSEMBLER)
        config_debug.debug = false;
#else
        config_debug.debug = read_bool("Debug", "Debug", false);
#endif
    config_debug.show_disassembler = read_bool("Debug", "Disassembler", true);
    config_debug.show_screen = read_bool("Debug", "Screen", true);
    config_debug.show_memory = read_bool("Debug", "Memory", false);
    config_debug.show_processor = read_bool("Debug", "Processor", true);
    config_debug.show_call_stack = read_bool("Debug", "CallStack", false);
    config_debug.show_psg = read_bool("Debug", "PSG", false);
    config_debug.show_trace_logger = read_bool("Debug", "TraceLogger", false);
    config_debug.show_mikey_regs = read_bool("Debug", "MikeyRegs", false);
    config_debug.show_mikey_timer_regs = read_bool("Debug", "MikeyTimerRegs", false);
    config_debug.show_mikey_timers = read_bool("Debug", "MikeyTimers", false);
    config_debug.show_mikey_audio = read_bool("Debug", "MikeyAudio", false);
    config_debug.show_mikey_colors = read_bool("Debug", "MikeyColors", false);
    config_debug.show_suzy_regs = read_bool("Debug", "SuzyRegs", false);
    config_debug.show_suzy_math_regs = read_bool("Debug", "SuzyMathRegs", false);
    config_debug.show_frame_buffers = read_bool("Debug", "FrameBuffers", false);
    config_debug.trace_counter = read_bool("Debug", "TraceCounter", true);
    config_debug.trace_registers = read_bool("Debug", "TraceRegisters", true);
    config_debug.trace_flags = read_bool("Debug", "TraceFlags", true);
    config_debug.trace_bytes = read_bool("Debug", "TraceBytes", true);
    config_debug.dis_show_mem = read_bool("Debug", "DisMem", true);
    config_debug.dis_show_symbols = read_bool("Debug", "DisSymbols", true);
    config_debug.dis_show_segment = read_bool("Debug", "DisSegment", true);
    config_debug.dis_show_auto_symbols = read_bool("Debug", "DisAutoSymbols", true);
    config_debug.dis_replace_symbols = read_bool("Debug", "DisReplaceSymbols", true);
    config_debug.dis_replace_labels = read_bool("Debug", "DisReplaceLabels", true);
    config_debug.font_size = read_int("Debug", "FontSize", 0);
    config_debug.multi_viewport = read_bool("Debug", "MultiViewport", false);

    config_emulator.maximized = read_bool("Emulator", "Maximized", false);
    config_emulator.fullscreen = read_bool("Emulator", "FullScreen", false);
    config_emulator.always_show_menu = read_bool("Emulator", "AlwaysShowMenu", false);
    config_emulator.ffwd_speed = read_int("Emulator", "FFWD", 1);
    config_emulator.save_slot = read_int("Emulator", "SaveSlot", 0);
    config_emulator.start_paused = read_bool("Emulator", "StartPaused", false);
    config_emulator.bios_path = read_string("Emulator", "BiosPath");
    config_emulator.savefiles_dir_option = read_int("Emulator", "SaveFilesDirOption", 0);
    config_emulator.savefiles_path = read_string("Emulator", "SaveFilesPath");
    config_emulator.savestates_dir_option = read_int("Emulator", "SaveStatesDirOption", 0);
    config_emulator.savestates_path = read_string("Emulator", "SaveStatesPath");
    config_emulator.screenshots_dir_option = read_int("Emulator", "ScreenshotDirOption", 0);
    config_emulator.screenshots_path = read_string("Emulator", "ScreenshotPath");
    config_emulator.last_open_path = read_string("Emulator", "LastOpenPath");
    config_emulator.window_width = read_int("Emulator", "WindowWidth", 770);
    config_emulator.window_height = read_int("Emulator", "WindowHeight", 600);
    config_emulator.status_messages = read_bool("Emulator", "StatusMessages", false);

    if (config_emulator.savefiles_path.empty())
    {
        config_emulator.savefiles_path = config_root_path;
    }
    if (config_emulator.savestates_path.empty())
    {
        config_emulator.savestates_path = config_root_path;
    }
    if (config_emulator.screenshots_path.empty())
    {
        config_emulator.screenshots_path = config_root_path;
    }

    for (int i = 0; i < config_max_recent_roms; i++)
    {
        std::string item = "RecentROM" + std::to_string(i);
        config_emulator.recent_roms[i] = read_string("Emulator", item.c_str());
    }

    config_video.scale = read_int("Video", "Scale", 0);
    if (config_video.scale > 3)
        config_video.scale -= 2;
    config_video.scale_manual = read_int("Video", "ScaleManual", 1);
    config_video.ratio = read_int("Video", "AspectRatio", 1);
    config_video.fps = read_bool("Video", "FPS", false);
    config_video.bilinear = read_bool("Video", "Bilinear", false);
    config_video.mix_frames = read_bool("Video", "MixFrames", true);
    config_video.mix_frames_intensity = read_float("Video", "MixFramesIntensity", 0.60f);
    config_video.scanlines_type = read_int("Video", "ScanlinesType", 2);
    config_video.scanlines_filter = read_bool("Video", "ScanlinesFilter", false);
    config_video.scanlines_intensity = read_float("Video", "ScanlinesIntensity", 0.80f);

    config_video.sync = read_bool("Video", "Sync", true);
    
    config_audio.enable = read_bool("Audio", "Enable", true);
    config_audio.sync = read_bool("Audio", "Sync", true);

    config_input.key_left = (SDL_Scancode)read_int("Input", "KeyLeft", SDL_SCANCODE_LEFT);
    config_input.key_right = (SDL_Scancode)read_int("Input", "KeyRight", SDL_SCANCODE_RIGHT);
    config_input.key_up = (SDL_Scancode)read_int("Input", "KeyUp", SDL_SCANCODE_UP);
    config_input.key_down = (SDL_Scancode)read_int("Input", "KeyDown", SDL_SCANCODE_DOWN);
    config_input.key_pause = (SDL_Scancode)read_int("Input", "KeyPause", SDL_SCANCODE_A);
    config_input.key_option1 = (SDL_Scancode)read_int("Input", "KeyOption1", SDL_SCANCODE_S);
    config_input.key_option2 = (SDL_Scancode)read_int("Input", "KeyOption2", SDL_SCANCODE_D);
    config_input.key_A = (SDL_Scancode)read_int("Input", "KeyA", SDL_SCANCODE_Z);
    config_input.key_B = (SDL_Scancode)read_int("Input", "KeyB", SDL_SCANCODE_X);

    config_input.gamepad = read_bool("Input", "Gamepad", true);
    config_input.gamepad_directional = read_int("Input", "GamepadDirectional", 0);
    config_input.gamepad_invert_x_axis = read_bool("Input", "GamepadInvertX", false);
    config_input.gamepad_invert_y_axis = read_bool("Input", "GamepadInvertY", false);
    config_input.gamepad_pause = read_int("Input", "GamepadPause", SDL_CONTROLLER_BUTTON_START);
    config_input.gamepad_option1 = read_int("Input", "GamepadOption1", SDL_CONTROLLER_BUTTON_X);
    config_input.gamepad_option2 = read_int("Input", "GamepadOption2", SDL_CONTROLLER_BUTTON_Y);
    config_input.gamepad_x_axis = read_int("Input", "GamepadX", SDL_CONTROLLER_AXIS_LEFTX);
    config_input.gamepad_y_axis = read_int("Input", "GamepadY", SDL_CONTROLLER_AXIS_LEFTY);
    config_input.gamepad_A = read_int("Input", "GamepadA", SDL_CONTROLLER_BUTTON_A);
    config_input.gamepad_B = read_int("Input", "GamepadB", SDL_CONTROLLER_BUTTON_B);

    Debug("Settings loaded");
}

void config_write(void)
{
    Log("Saving settings to %s", config_emu_file_path);

    if (config_emulator.ffwd)
        config_audio.sync = true;

    write_bool("Debug", "Debug", config_debug.debug);
    write_bool("Debug", "Disassembler", config_debug.show_disassembler);
    write_bool("Debug", "Screen", config_debug.show_screen);
    write_bool("Debug", "Memory", config_debug.show_memory);
    write_bool("Debug", "Processor", config_debug.show_processor);
    write_bool("Debug", "CallStack", config_debug.show_call_stack);
    write_bool("Debug", "PSG", config_debug.show_psg);
    write_bool("Debug", "MikeyRegs", config_debug.show_mikey_regs);
    write_bool("Debug", "MikeyTimerRegs", config_debug.show_mikey_timer_regs);
    write_bool("Debug", "MikeyTimers", config_debug.show_mikey_timers);
    write_bool("Debug", "MikeyAudio", config_debug.show_mikey_audio);
    write_bool("Debug", "MikeyColors", config_debug.show_mikey_colors);
    write_bool("Debug", "SuzyRegs", config_debug.show_suzy_regs);
    write_bool("Debug", "SuzyMathRegs", config_debug.show_suzy_math_regs);
    write_bool("Debug", "TraceLogger", config_debug.show_trace_logger);
    write_bool("Debug", "FrameBuffers", config_debug.show_frame_buffers);
    write_bool("Debug", "TraceCounter", config_debug.trace_counter);
    write_bool("Debug", "TraceRegisters", config_debug.trace_registers);
    write_bool("Debug", "TraceFlags", config_debug.trace_flags);
    write_bool("Debug", "TraceBytes", config_debug.trace_bytes);
    write_bool("Debug", "DisMem", config_debug.dis_show_mem);
    write_bool("Debug", "DisSymbols", config_debug.dis_show_symbols);
    write_bool("Debug", "DisSegment", config_debug.dis_show_segment);
    write_bool("Debug", "DisAutoSymbols", config_debug.dis_show_auto_symbols);
    write_bool("Debug", "DisReplaceSymbols", config_debug.dis_replace_symbols);
    write_bool("Debug", "DisReplaceLabels", config_debug.dis_replace_labels);
    write_int("Debug", "FontSize", config_debug.font_size);
    write_bool("Debug", "MultiViewport", config_debug.multi_viewport);

    write_bool("Emulator", "Maximized", config_emulator.maximized);
    write_bool("Emulator", "FullScreen", config_emulator.fullscreen);
    write_bool("Emulator", "AlwaysShowMenu", config_emulator.always_show_menu);
    write_int("Emulator", "FFWD", config_emulator.ffwd_speed);
    write_int("Emulator", "SaveSlot", config_emulator.save_slot);
    write_bool("Emulator", "StartPaused", config_emulator.start_paused);
    write_string("Emulator", "BiosPath", config_emulator.bios_path);
    write_int("Emulator", "SaveFilesDirOption", config_emulator.savefiles_dir_option);
    write_string("Emulator", "SaveFilesPath", config_emulator.savefiles_path);
    write_int("Emulator", "SaveStatesDirOption", config_emulator.savestates_dir_option);
    write_string("Emulator", "SaveStatesPath", config_emulator.savestates_path);
    write_int("Emulator", "ScreenshotDirOption", config_emulator.screenshots_dir_option);
    write_string("Emulator", "ScreenshotPath", config_emulator.screenshots_path);
    write_string("Emulator", "LastOpenPath", config_emulator.last_open_path);
    write_int("Emulator", "WindowWidth", config_emulator.window_width);
    write_int("Emulator", "WindowHeight", config_emulator.window_height);
    write_bool("Emulator", "StatusMessages", config_emulator.status_messages);

    for (int i = 0; i < config_max_recent_roms; i++)
    {
        std::string item = "RecentROM" + std::to_string(i);
        write_string("Emulator", item.c_str(), config_emulator.recent_roms[i]);
    }

    write_int("Video", "Scale", config_video.scale);
    write_int("Video", "ScaleManual", config_video.scale_manual);
    write_int("Video", "AspectRatio", config_video.ratio);
    write_bool("Video", "FPS", config_video.fps);
    write_bool("Video", "Bilinear", config_video.bilinear);
    write_bool("Video", "MixFrames", config_video.mix_frames);
    write_float("Video", "MixFramesIntensity", config_video.mix_frames_intensity);
    write_int("Video", "ScanlinesType", config_video.scanlines_type);
    write_bool("Video", "ScanlinesFilter", config_video.scanlines_filter);
    write_float("Video", "ScanlinesIntensity", config_video.scanlines_intensity);
    write_bool("Video", "Sync", config_video.sync);

    write_bool("Audio", "Enable", config_audio.enable);
    write_bool("Audio", "Sync", config_audio.sync);

    write_int("Input", "KeyLeft", config_input.key_left);
    write_int("Input", "KeyRight", config_input.key_right);
    write_int("Input", "KeyUp", config_input.key_up);
    write_int("Input", "KeyDown", config_input.key_down);
    write_int("Input", "KeyPause", config_input.key_pause);
    write_int("Input", "KeyOption1", config_input.key_option1);
    write_int("Input", "KeyOption2", config_input.key_option2);
    write_int("Input", "KeyA", config_input.key_A);
    write_int("Input", "KeyB", config_input.key_B);

    write_bool("Input", "Gamepad", config_input.gamepad);
    write_int("Input", "GamepadDirectional", config_input.gamepad_directional);
    write_bool("Input", "GamepadInvertX", config_input.gamepad_invert_x_axis);
    write_bool("Input", "GamepadInvertY", config_input.gamepad_invert_y_axis);
    write_int("Input", "GamepadPause", config_input.gamepad_pause);
    write_int("Input", "GamepadOption1", config_input.gamepad_option1);
    write_int("Input", "GamepadOption2", config_input.gamepad_option2);
    write_int("Input", "GamepadX", config_input.gamepad_x_axis);
    write_int("Input", "GamepadY", config_input.gamepad_y_axis);
    write_int("Input", "GamepadA", config_input.gamepad_A);
    write_int("Input", "GamepadB", config_input.gamepad_B);

    if (config_ini_file->write(config_ini_data, true))
    {
        Debug("Settings saved");
    }
}

static bool check_portable(void)
{
    char* base_path;
    char portable_file_path[260];
    
    base_path = SDL_GetBasePath();
    
    strcpy(portable_file_path, base_path);
    strcat(portable_file_path, "portable.ini");

    FILE* file = fopen(portable_file_path, "r");
    
    if (IsValidPointer(file))
    {
        fclose(file);
        return true;
    }

    return false;
}

static int read_int(const char* group, const char* key, int default_value)
{
    int ret = 0;

    std::string value = config_ini_data[group][key];

    if(value.empty())
        ret = default_value;
    else
        ret = std::stoi(value);

    Debug("Load integer setting: [%s][%s]=%d", group, key, ret);
    return ret;
}

static void write_int(const char* group, const char* key, int integer)
{
    std::string value = std::to_string(integer);
    config_ini_data[group][key] = value;
    Debug("Save integer setting: [%s][%s]=%s", group, key, value.c_str());
}

static float read_float(const char* group, const char* key, float default_value)
{
    float ret = 0.0f;

    std::string value = config_ini_data[group][key];

    if(value.empty())
        ret = default_value;
    else
        ret = strtof(value.c_str(), NULL);

    Debug("Load float setting: [%s][%s]=%.2f", group, key, ret);
    return ret;
}

static void write_float(const char* group, const char* key, float value)
{
    std::ostringstream oss;
    oss.imbue(std::locale::classic());
    oss << std::fixed << std::setprecision(2) << value;
    std::string value_str = oss.str();
    config_ini_data[group][key] = oss.str();
    Debug("Save float setting: [%s][%s]=%s", group, key, value_str.c_str());
}

static bool read_bool(const char* group, const char* key, bool default_value)
{
    bool ret;

    std::string value = config_ini_data[group][key];

    if(value.empty())
        ret = default_value;
    else
        std::istringstream(value) >> std::boolalpha >> ret;

    Debug("Load bool setting: [%s][%s]=%s", group, key, ret ? "true" : "false");
    return ret;
}

static void write_bool(const char* group, const char* key, bool boolean)
{
    std::stringstream converter;
    converter << std::boolalpha << boolean;
    std::string value;
    value = converter.str();
    config_ini_data[group][key] = value;
    Debug("Save bool setting: [%s][%s]=%s", group, key, value.c_str());
}

static std::string read_string(const char* group, const char* key)
{
    std::string ret = config_ini_data[group][key];
    Debug("Load string setting: [%s][%s]=%s", group, key, ret.c_str());
    return ret;
}

static void write_string(const char* group, const char* key, std::string value)
{
    config_ini_data[group][key] = value;
    Debug("Save string setting: [%s][%s]=%s", group, key, value.c_str());
}
