/*
 * Gearlynx - Lynx Emulator
 * Copyright (C) 2025  Ignacio Sanchez

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see http://www.gnu.org/licenses/
 *
 */

#ifndef CONFIG_DATA_H
#define CONFIG_DATA_H

#include <SDL3/SDL.h>
#include <string>
#include "gearlynx.h"

static const char* config_application_name = GLYNX_TITLE;
static const int config_version = 4;
static const int config_minimum_version = 2;
static const int config_max_recent_roms = 10;
static const int config_memory_editor_count = 9;

enum config_ShaderMode
{
    config_ShaderMode_PixelPerfect = 0,
    config_ShaderMode_External = 1
};

enum config_Theme
{
    config_Theme_Light = 0,
    config_Theme_Dark = 1,
    config_Theme_Count = 2
};

enum config_VideoSync
{
    config_VideoSync_Disabled = 0,
    config_VideoSync_Fixed = 1,
    config_VideoSync_VRR = 2
};

enum config_EEPROM
{
    config_EEPROM_Auto = 0,
    config_EEPROM_None,
    config_EEPROM_93C46_16Bit,
    config_EEPROM_93C46_8Bit,
    config_EEPROM_93C56_16Bit,
    config_EEPROM_93C56_8Bit,
    config_EEPROM_93C66_16Bit,
    config_EEPROM_93C66_8Bit,
    config_EEPROM_93C76_16Bit,
    config_EEPROM_93C76_8Bit,
    config_EEPROM_93C86_16Bit,
    config_EEPROM_93C86_8Bit,
    config_EEPROM_Count
};

enum config_CartridgeHardware
{
    config_CartridgeHardware_Auto = 0,
    config_CartridgeHardware_Standard,
    config_CartridgeHardware_GameDrive,
    config_CartridgeHardware_ElCheapoSD,
    config_CartridgeHardware_Count
};

struct config_Emulator
{
    bool maximized;
    bool fullscreen;
    int fullscreen_mode;
    bool always_show_menu;
    int theme;
    bool paused;
    int save_slot;
    bool start_paused;
    bool pause_when_inactive;
    bool ffwd;
    int ffwd_speed;
    int runahead;
    bool fast_sprite_rendering;
    bool show_info;
    std::string recent_roms[config_max_recent_roms];
    std::string bios_path;
    int savefiles_dir_option;
    std::string savefiles_path;
    int savestates_dir_option;
    std::string savestates_path;
    int screenshots_dir_option;
    std::string screenshots_path;
    std::string last_open_path;
    int window_width;
    int window_height;
    bool status_messages;
    bool allow_screensaver;
    int mcp_tcp_port;
    std::string mcp_http_address;
    int comlynx_session;
    int comlynx_stall_us;
    int console_type;
    int eeprom;
    int cartridge_hardware;
};

struct config_Video
{
    int scale;
    int scale_manual;
    int ratio;
    int rotation;
    bool fps;
    int sync_mode;
    float background_color[config_Theme_Count][3];
    float background_color_debugger[config_Theme_Count][3];
    int shader_mode;
    std::string shader_preset_path;
};

struct config_Audio
{
    bool enable;
    bool sync;
    float master_volume;
    float volume[4];
    int lowpass_cutoff;
    int buffer_count;
};

struct config_Rewind
{
    bool enabled;
    int buffer_seconds;
    int frames_per_snapshot;
    float speed;
};

struct config_Input
{
    SDL_Scancode key_left;
    SDL_Scancode key_right;
    SDL_Scancode key_up;
    SDL_Scancode key_down;
    SDL_Scancode key_pause;
    SDL_Scancode key_option1;
    SDL_Scancode key_option2;
    SDL_Scancode key_A;
    SDL_Scancode key_B;
    bool allow_up_down;
    bool gamepad;
    int gamepad_directional;
    bool gamepad_invert_x_axis;
    bool gamepad_invert_y_axis;
    int gamepad_pause;
    int gamepad_option1;
    int gamepad_option2;
    int gamepad_A;
    int gamepad_B;
    int gamepad_x_axis;
    int gamepad_y_axis;
};

enum config_HotkeyIndex
{
    config_HotkeyIndex_OpenROM = 0,
    config_HotkeyIndex_ReloadROM,
    config_HotkeyIndex_Quit,
    config_HotkeyIndex_Reset,
    config_HotkeyIndex_Pause,
    config_HotkeyIndex_FFWD,
    config_HotkeyIndex_Rewind,
    config_HotkeyIndex_SaveState,
    config_HotkeyIndex_LoadState,
    config_HotkeyIndex_Screenshot,
    config_HotkeyIndex_Fullscreen,
    config_HotkeyIndex_ShowMainMenu,
    config_HotkeyIndex_DebugStepInto,
    config_HotkeyIndex_DebugStepOver,
    config_HotkeyIndex_DebugStepOut,
    config_HotkeyIndex_DebugStepFrame,
    config_HotkeyIndex_DebugContinue,
    config_HotkeyIndex_DebugBreak,
    config_HotkeyIndex_DebugRunToCursor,
    config_HotkeyIndex_DebugBreakpoint,
    config_HotkeyIndex_DebugGoBack,
    config_HotkeyIndex_SelectSlot1,
    config_HotkeyIndex_SelectSlot2,
    config_HotkeyIndex_SelectSlot3,
    config_HotkeyIndex_SelectSlot4,
    config_HotkeyIndex_SelectSlot5,
    config_HotkeyIndex_Mute,
    config_HotkeyIndex_COUNT
};

struct config_Input_Gamepad_Shortcuts
{
    int gamepad_shortcuts[config_HotkeyIndex_COUNT];
};

struct config_Hotkey
{
    SDL_Scancode key;
    SDL_Keymod mod;
    char str[64];
};

struct config_Debug
{
    bool debug;
    bool show_screen;
    bool show_disassembler;
    bool show_processor;
    bool show_call_stack;
    bool show_breakpoints;
    bool show_symbols;
    bool show_memory;
    bool show_psg;
    bool show_trace_logger;
    bool show_mikey_regs;
    bool show_mikey_timers;
    bool show_mikey_colors;
    bool show_suzy_regs;
    bool show_suzy_math_regs;
    bool show_scb_viewer;
    int sprite_bounding_box_mode;
    int sprite_bounding_box_color;
    int sprite_bounding_box_decay;
    int scb_viewer_address;
    bool scb_viewer_auto;
    int scb_viewer_mode;
    bool show_frame_buffers;
    int frame_buffer_custom_address;
    bool show_lcd;
    bool show_uart;
    bool show_comlynx;
    bool show_eeprom;
    bool show_cart;
    bool show_rewind;
    bool trace_counter;
    bool trace_registers;
    bool trace_flags;
    bool trace_bytes;
    bool trace_cpu;
    bool trace_cpu_irq;
    bool trace_suzy_math;
    bool trace_suzy_sprites;
    bool trace_suzy_input;
    bool trace_mikey_timers;
    bool trace_mikey_uart;
    bool trace_redeye;
    bool trace_mikey_audio;
    bool trace_cart;
    bool trace_debug_messages;
    bool debug_output_enabled;
    bool dis_show_mem;
    bool dis_show_symbols;
    bool dis_show_segment;
    bool dis_show_auto_symbols;
    bool dis_dim_auto_symbols;
    bool dis_replace_symbols;
    bool dis_replace_labels;
    int dis_syntax;
    int dis_look_ahead_count;
    bool step_skip_interrupts;
    bool pause_on_brk;
    int pause_on_brk_value;
    bool pause_on_brk_trigger_irq;
    int font_size;
    int scale;
    bool multi_viewport;
    bool single_instance;
    bool auto_debug_settings;
    int mem_editor_bytes_per_row[config_memory_editor_count];
    int mem_editor_preview_data_type[config_memory_editor_count];
    int mem_editor_preview_endianess[config_memory_editor_count];
    bool mem_editor_uppercase_hex[config_memory_editor_count];
    bool mem_editor_gray_out_zeros[config_memory_editor_count];
};

EXTERN config_Emulator config_emulator;
EXTERN config_Video config_video;
EXTERN config_Audio config_audio;
EXTERN config_Rewind config_rewind;
EXTERN config_Input config_input;
EXTERN config_Input_Gamepad_Shortcuts config_input_gamepad_shortcuts;
EXTERN config_Hotkey config_hotkeys[config_HotkeyIndex_COUNT];
EXTERN config_Debug config_debug;

#endif /* CONFIG_DATA_H */
