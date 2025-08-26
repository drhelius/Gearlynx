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

#ifndef CONFIG_H
#define	CONFIG_H

#include <SDL.h>
#include "gearlynx.h"
#define MINI_CASE_SENSITIVE
#include "ini.h"
#include "imgui.h"

#ifdef CONFIG_IMPORT
    #define EXTERN
#else
    #define EXTERN extern
#endif

static const int config_max_recent_roms = 10;

struct config_Emulator
{
    bool maximized = false;
    bool fullscreen = false;
    bool always_show_menu = false;
    bool paused = false;
    int save_slot = 0;
    bool start_paused = false;
    bool ffwd = false;
    int ffwd_speed = 1;
    bool show_info = false;
    std::string recent_roms[config_max_recent_roms];
    std::string bios_path;
    int savefiles_dir_option = 0;
    std::string savefiles_path;
    int savestates_dir_option = 0;
    std::string savestates_path;
    int screenshots_dir_option = 0;
    std::string screenshots_path;
    std::string last_open_path;
    int window_width = 770;
    int window_height = 600;
    bool status_messages = false;
};

struct config_Video
{
    int scale = 0;
    int scale_manual = 1;
    int ratio = 1;
    bool fps = false;
    bool bilinear = false;
    bool mix_frames = true;
    float mix_frames_intensity = 0.60f;
    bool scanlines = true;
    bool scanlines_filter = true;
    float scanlines_intensity = 0.10f;
    bool sync = true;
};

struct config_Audio
{
    bool enable = true;
    bool sync = true;
};

struct config_Input
{
    SDL_Scancode key_left;
    SDL_Scancode key_right;
    SDL_Scancode key_up;
    SDL_Scancode key_down;
    SDL_Scancode key_start;
    SDL_Scancode key_option1;
    SDL_Scancode key_option2;
    SDL_Scancode key_A;
    SDL_Scancode key_B;
    bool gamepad;
    int gamepad_directional;
    bool gamepad_invert_x_axis;
    bool gamepad_invert_y_axis;
    int gamepad_start;
    int gamepad_option1;
    int gamepad_option2;
    int gamepad_A;
    int gamepad_B;
    int gamepad_x_axis;
    int gamepad_y_axis;
};

struct config_Debug
{
    bool debug = false;
    bool show_screen = true;
    bool show_disassembler = true;
    bool show_processor = true;
    bool show_call_stack = false;
    bool show_memory = false;
    bool show_psg = false;
    bool show_trace_logger = false;
    bool show_mikey_regs = false;
    bool show_mikey_timer_regs = false;
    bool show_mikey_timers = false;
    bool show_mikey_audio = false;
    bool show_mikey_colors = false;
    bool show_suzy_regs = false;
    bool show_suzy_math_regs = false;
    bool trace_counter = true;
    bool trace_registers = true;
    bool trace_flags = true;
    bool trace_bytes = true;
    bool dis_show_mem = true;
    bool dis_show_symbols = true;
    bool dis_show_segment = true;
    bool dis_show_auto_symbols = true;
    bool dis_replace_symbols = true;
    bool dis_replace_labels = true;
    int font_size = 0;
    bool multi_viewport = false;
};

EXTERN mINI::INIFile* config_ini_file;
EXTERN mINI::INIStructure config_ini_data;
EXTERN char* config_root_path;
EXTERN char config_emu_file_path[260];
EXTERN char config_imgui_file_path[260];
EXTERN config_Emulator config_emulator;
EXTERN config_Video config_video;
EXTERN config_Audio config_audio;
EXTERN config_Input config_input;
EXTERN config_Debug config_debug;

EXTERN void config_init(void);
EXTERN void config_destroy(void);
EXTERN void config_read(void);
EXTERN void config_write(void);

#undef CONFIG_IMPORT
#undef EXTERN
#endif	/* CONFIG_H */
