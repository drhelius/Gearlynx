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

#include "config_macros.h"
#include "shader_preset.h"

static inline void process(config_Operation operation)
{
    //**************************************
    // Debug
    //**************************************

    // Debugger windows
    CONFIG_BOOL("Debug", "Debug", config_debug.debug, false);
    CONFIG_BOOL("Debug", "Disassembler", config_debug.show_disassembler, true);
    CONFIG_BOOL("Debug", "Screen", config_debug.show_screen, true);
    CONFIG_BOOL("Debug", "Memory", config_debug.show_memory, false);
    CONFIG_BOOL("Debug", "Processor", config_debug.show_processor, true);
    CONFIG_BOOL("Debug", "CallStack", config_debug.show_call_stack, false);
    CONFIG_BOOL("Debug", "Breakpoints", config_debug.show_breakpoints, false);
    CONFIG_BOOL("Debug", "Symbols", config_debug.show_symbols, false);
    CONFIG_BOOL("Debug", "PSG", config_debug.show_psg, false);
    CONFIG_BOOL("Debug", "TraceLogger", config_debug.show_trace_logger, false);
    CONFIG_BOOL("Debug", "MikeyRegs", config_debug.show_mikey_regs, false);
    CONFIG_BOOL("Debug", "MikeyTimers", config_debug.show_mikey_timers, false);
    CONFIG_BOOL("Debug", "MikeyColors", config_debug.show_mikey_colors, false);
    CONFIG_BOOL("Debug", "SuzyRegs", config_debug.show_suzy_regs, false);
    CONFIG_BOOL("Debug", "SuzyMathRegs", config_debug.show_suzy_math_regs, false);
    CONFIG_BOOL("Debug", "SCBViewer", config_debug.show_scb_viewer, false);

    // Sprite bounding boxes and viewers
    CONFIG_INT_RANGE("Debug", "SpriteBoundingBoxMode", config_debug.sprite_bounding_box_mode, GLYNX_SPRITE_BOUNDING_BOX_DISABLED, GLYNX_SPRITE_BOUNDING_BOX_DISABLED, GLYNX_SPRITE_BOUNDING_BOX_SPRCOLL_BIT_7);
    CONFIG_INT_RANGE("Debug", "SpriteBoundingBoxColor", config_debug.sprite_bounding_box_color, 0, 0, 7);
    CONFIG_INT_RANGE("Debug", "SpriteBoundingBoxDecay", config_debug.sprite_bounding_box_decay, 0, 0, 10);
    CONFIG_INT("Debug", "SCBViewerAddress", config_debug.scb_viewer_address, 0x0000);
    CONFIG_BOOL("Debug", "SCBViewerAuto", config_debug.scb_viewer_auto, true);
    CONFIG_INT("Debug", "SCBViewerMode", config_debug.scb_viewer_mode, 1);
    CONFIG_BOOL("Debug", "FrameBuffers", config_debug.show_frame_buffers, false);
    CONFIG_INT("Debug", "FrameBufferCustomAddress", config_debug.frame_buffer_custom_address, 0x0000);
    CONFIG_BOOL("Debug", "LCD", config_debug.show_lcd, false);
    CONFIG_BOOL("Debug", "UART", config_debug.show_uart, false);
    CONFIG_BOOL("Debug", "ComLynx", config_debug.show_comlynx, false);
    CONFIG_BOOL("Debug", "EEPROM", config_debug.show_eeprom, false);
    CONFIG_BOOL("Debug", "Cart", config_debug.show_cart, false);
    CONFIG_BOOL("Debug", "Rewind", config_debug.show_rewind, false);

    // Trace logger
    CONFIG_BOOL("Debug", "TraceCounter", config_debug.trace_counter, true);
    CONFIG_BOOL("Debug", "TraceCycles", config_debug.trace_cycles, false);
    CONFIG_BOOL("Debug", "TraceRegisters", config_debug.trace_registers, true);
    CONFIG_BOOL("Debug", "TraceFlags", config_debug.trace_flags, true);
    CONFIG_BOOL("Debug", "TraceBytes", config_debug.trace_bytes, true);
    CONFIG_BOOL("Debug", "TraceCpuEnabled", config_debug.trace_cpu_enabled, true);
    CONFIG_BOOL("Debug", "TraceCpu", config_debug.trace_cpu, true);
    CONFIG_BOOL("Debug", "TraceCpuIrq", config_debug.trace_cpu_irq, true);
    CONFIG_BOOL("Debug", "TraceSuzyMath", config_debug.trace_suzy_math, false);
    CONFIG_BOOL("Debug", "TraceSuzySprites", config_debug.trace_suzy_sprites, false);
    CONFIG_BOOL("Debug", "TraceSuzyInput", config_debug.trace_suzy_input, false);
    CONFIG_BOOL("Debug", "TraceMikeyTimers", config_debug.trace_mikey_timers, false);
    CONFIG_BOOL("Debug", "TraceMikeyInterrupts", config_debug.trace_mikey_interrupts, false);
    CONFIG_BOOL("Debug", "TraceMikeyDisplay", config_debug.trace_mikey_display, false);
    CONFIG_BOOL("Debug", "TraceMikeyUart", config_debug.trace_mikey_uart, false);
    CONFIG_BOOL("Debug", "TraceRedEye", config_debug.trace_redeye, false);
    CONFIG_BOOL("Debug", "TraceMikeyAudio", config_debug.trace_mikey_audio, false);
    CONFIG_BOOL("Debug", "TraceCart", config_debug.trace_cart, false);
    CONFIG_BOOL("Debug", "TraceDebugMessages", config_debug.trace_debug_messages, false);
    CONFIG_INT_RANGE("Debug", "TraceSuzyMathEvents", config_debug.trace_suzy_math_events, TRACE_SUZY_MATH_FILTER_ALL, 0, TRACE_SUZY_MATH_FILTER_ALL);
    CONFIG_INT_RANGE("Debug", "TraceSuzySpriteEvents", config_debug.trace_suzy_sprite_events, TRACE_SUZY_SPRITE_FILTER_ALL, 0, TRACE_SUZY_SPRITE_FILTER_ALL);
    CONFIG_INT_RANGE("Debug", "TraceSuzyInputEvents", config_debug.trace_suzy_input_events, TRACE_SUZY_INPUT_FILTER_ALL, 0, TRACE_SUZY_INPUT_FILTER_ALL);
    CONFIG_INT_RANGE("Debug", "TraceMikeyTimerEvents", config_debug.trace_mikey_timer_events, TRACE_MIKEY_TIMER_FILTER_ALL, 0, TRACE_MIKEY_TIMER_FILTER_ALL);
    CONFIG_INT_RANGE("Debug", "TraceMikeyInterruptEvents", config_debug.trace_mikey_interrupt_events, TRACE_MIKEY_INTERRUPT_FILTER_ALL, 0, TRACE_MIKEY_INTERRUPT_FILTER_ALL);
    CONFIG_INT_RANGE("Debug", "TraceMikeyDisplayEvents", config_debug.trace_mikey_display_events, TRACE_MIKEY_DISPLAY_FILTER_ALL, 0, TRACE_MIKEY_DISPLAY_FILTER_ALL);
    CONFIG_INT_RANGE("Debug", "TraceMikeyUartEvents", config_debug.trace_mikey_uart_events, TRACE_MIKEY_UART_FILTER_ALL, 0, TRACE_MIKEY_UART_FILTER_ALL);
    CONFIG_INT_RANGE("Debug", "TraceRedEyeEvents", config_debug.trace_redeye_events, TRACE_REDEYE_FILTER_ALL, 0, TRACE_REDEYE_FILTER_ALL);
    CONFIG_INT_RANGE("Debug", "TraceMikeyAudioEvents", config_debug.trace_mikey_audio_events, TRACE_MIKEY_AUDIO_FILTER_ALL, 0, TRACE_MIKEY_AUDIO_FILTER_ALL);
    CONFIG_INT_RANGE("Debug", "TraceCartridgeEvents", config_debug.trace_cartridge_events, TRACE_CARTRIDGE_FILTER_ALL, 0, TRACE_CARTRIDGE_FILTER_ALL);
    CONFIG_INT_RANGE("Debug", "TraceDebugEvents", config_debug.trace_debug_events, TRACE_DEBUG_FILTER_MESSAGES, 0, TRACE_DEBUG_FILTER_MESSAGES);
    CONFIG_BOOL("Debug", "DebugOutputEnabled", config_debug.debug_output_enabled, false);
    CONFIG_INT_RANGE("Debug", "TraceOutput", config_debug.trace_output, 0, 0, 1);
    CONFIG_INT_RANGE("Debug", "TraceCapacity", config_debug.trace_capacity, 0, 0, 4);
    CONFIG_INT_RANGE("Debug", "TraceDiskDirOption", config_debug.trace_disk_dir_option, 0, 0, 2);
    CONFIG_INT_RANGE("Debug", "TraceDiskSize", config_debug.trace_disk_size, 2, 0, 6);
    CONFIG_STRING_NOT_EMPTY("Debug", "TraceDiskPath", config_debug.trace_disk_path, config_root_path);

    // Disassembler
    CONFIG_BOOL("Debug", "DisMem", config_debug.dis_show_mem, true);
    CONFIG_BOOL("Debug", "DisSymbols", config_debug.dis_show_symbols, true);
    CONFIG_BOOL("Debug", "DisSegment", config_debug.dis_show_segment, true);
    CONFIG_BOOL("Debug", "DisAutoSymbols", config_debug.dis_show_auto_symbols, true);
    CONFIG_BOOL("Debug", "DisDimAutoSymbols", config_debug.dis_dim_auto_symbols, false);
    CONFIG_BOOL("Debug", "DisReplaceSymbols", config_debug.dis_replace_symbols, true);
    CONFIG_BOOL("Debug", "DisReplaceLabels", config_debug.dis_replace_labels, true);
    CONFIG_INT_RANGE("Debug", "DisSyntax", config_debug.dis_syntax, GLYNX_Disassembler_Syntax_Gearlynx, GLYNX_Disassembler_Syntax_Gearlynx, GLYNX_Disassembler_Syntax_Count - 1);
    CONFIG_INT("Debug", "DisLookAheadCount", config_debug.dis_look_ahead_count, 20);
    CONFIG_BOOL("Debug", "StepSkipInterrupts", config_debug.step_skip_interrupts, false);
    CONFIG_BOOL("Debug", "PauseOnBRK", config_debug.pause_on_brk, false);
    CONFIG_INT_RANGE("Debug", "PauseOnBRKValue", config_debug.pause_on_brk_value, 0x42, 0, 0xFF);
    CONFIG_BOOL("Debug", "PauseOnBRKTriggerIRQ", config_debug.pause_on_brk_trigger_irq, false);

    // Interface
    CONFIG_INT_RANGE("Debug", "FontSize", config_debug.font_size, 0, 0, 3);
    CONFIG_INT_RANGE("Debug", "Scale", config_debug.scale, 2, 1, 20);
    CONFIG_BOOL("Debug", "MultiViewport", config_debug.multi_viewport, false);
    CONFIG_BOOL("Debug", "SingleInstance", config_debug.single_instance, false);
    CONFIG_BOOL("Debug", "AutoDebugSettings", config_debug.auto_debug_settings, false);

    // Memory editors
    for (int i = 0; i < config_memory_editor_count; i++)
    {
        char section[32];
        snprintf(section, sizeof(section), "MemEditor_%d", i);
        CONFIG_INT(section, "BytesPerRow", config_debug.mem_editor_bytes_per_row[i], 16);
        CONFIG_INT(section, "PreviewDataType", config_debug.mem_editor_preview_data_type[i], 0);
        CONFIG_INT(section, "PreviewEndianess", config_debug.mem_editor_preview_endianess[i], 0);
        CONFIG_BOOL(section, "UppercaseHex", config_debug.mem_editor_uppercase_hex[i], true);
        CONFIG_BOOL(section, "GrayOutZeros", config_debug.mem_editor_gray_out_zeros[i], true);
    }

    //**************************************
    // Emulator
    //**************************************

    // Window and interface
    CONFIG_BOOL("Emulator", "Maximized", config_emulator.maximized, false);
    CONFIG_BOOL("Emulator", "FullScreen", config_emulator.fullscreen, false);
    CONFIG_INT("Emulator", "FullScreenMode", config_emulator.fullscreen_mode, 0);
    CONFIG_BOOL("Emulator", "AlwaysShowMenu", config_emulator.always_show_menu, false);
    CONFIG_INT_RANGE("Emulator", "Theme", config_emulator.theme, config_Theme_Dark, config_Theme_Light, config_Theme_Dark);

    // Emulation
    CONFIG_INT("Emulator", "FFWD", config_emulator.ffwd_speed, 1);
    CONFIG_INT_RANGE("Emulator", "RunAhead", config_emulator.runahead, 0, 0, 3);
    CONFIG_INT_RANGE("Emulator", "SaveSlot", config_emulator.save_slot, 0, 0, 4);
    CONFIG_BOOL("Emulator", "LegacySpriteRendering", config_emulator.fast_sprite_rendering, false);
    CONFIG_BOOL("Emulator", "StartPaused", config_emulator.start_paused, false);
    CONFIG_BOOL("Emulator", "PauseWhenInactive", config_emulator.pause_when_inactive, true);
    CONFIG_STRING("Emulator", "BiosPath", config_emulator.bios_path, "");

    // Files and paths
    CONFIG_INT("Emulator", "SaveFilesDirOption", config_emulator.savefiles_dir_option, 0);
    CONFIG_STRING_NOT_EMPTY("Emulator", "SaveFilesPath", config_emulator.savefiles_path, config_root_path);
    CONFIG_INT("Emulator", "SaveStatesDirOption", config_emulator.savestates_dir_option, 0);
    CONFIG_STRING_NOT_EMPTY("Emulator", "SaveStatesPath", config_emulator.savestates_path, config_root_path);
    CONFIG_INT("Emulator", "ScreenshotDirOption", config_emulator.screenshots_dir_option, 0);
    CONFIG_STRING_NOT_EMPTY("Emulator", "ScreenshotPath", config_emulator.screenshots_path, config_root_path);
    CONFIG_STRING("Emulator", "LastOpenPath", config_emulator.last_open_path, "");
    CONFIG_INT("Emulator", "WindowWidth", config_emulator.window_width, 770);
    CONFIG_INT("Emulator", "WindowHeight", config_emulator.window_height, 600);
    CONFIG_BOOL("Emulator", "StatusMessages", config_emulator.status_messages, false);
    CONFIG_BOOL("Emulator", "AllowScreenSaver", config_emulator.allow_screensaver, false);

    // Services and hardware
    CONFIG_INT("Emulator", "MCPTCPPort", config_emulator.mcp_tcp_port, 7777);
    CONFIG_STRING_NOT_EMPTY("Emulator", "MCPHTTPAddress", config_emulator.mcp_http_address, "127.0.0.1");
    CONFIG_INT_RANGE("Emulator", "ComLynxSession", config_emulator.comlynx_session, 1, 1, 255);
#if defined(_WIN32)
    CONFIG_INT_RANGE("Emulator", "ComLynxStallUs", config_emulator.comlynx_stall_us, 5000, 1000, 10000);
#elif defined(__APPLE__)
    CONFIG_INT_RANGE("Emulator", "ComLynxStallUs", config_emulator.comlynx_stall_us, 100, 50, 1000);
#else
    CONFIG_INT_RANGE("Emulator", "ComLynxStallUs", config_emulator.comlynx_stall_us, 250, 50, 2000);
#endif
    CONFIG_INT("Emulator", "ConsoleType", config_emulator.console_type, 0);
    CONFIG_INT_RANGE("Emulator", "EEPROM", config_emulator.eeprom, config_EEPROM_Auto, config_EEPROM_Auto, config_EEPROM_Count - 1);
    CONFIG_INT_RANGE("Emulator", "CartridgeHardware", config_emulator.cartridge_hardware, config_CartridgeHardware_Auto, config_CartridgeHardware_Auto, config_CartridgeHardware_Count - 1);
    CONFIG_STRING_ARRAY("Emulator", "RecentROM%d", config_emulator.recent_roms, config_max_recent_roms, "");

    //**************************************
    // Video
    //**************************************

    // Display
    CONFIG_INT("Video", "Scale", config_video.scale, 0);
    CONFIG_INT_RANGE("Video", "ScaleManual", config_video.scale_manual, 1, 1, 20);
    CONFIG_INT("Video", "AspectRatio", config_video.ratio, 0);
    CONFIG_INT_RANGE("Video", "Rotation", config_video.rotation, GLYNX_ROTATION_AUTO, GLYNX_ROTATION_AUTO, GLYNX_ROTATION_180);
    CONFIG_BOOL("Video", "FPS", config_video.fps, false);
    CONFIG_INT_RANGE("Video", "ShaderMode", config_video.shader_mode, config_ShaderMode_PixelPerfect, config_ShaderMode_PixelPerfect, config_ShaderMode_External);

    if (operation == config_Operation_Write)
    {
        std::string preset_file = get_filename(config_video.shader_preset_path.c_str());
        CONFIG_STRING("Video", "ShaderPresetFile", preset_file, "");
    }
    else
    {
        CONFIG_STRING("Video", "ShaderPresetFile", config_video.shader_preset_path, "");
    }

    CONFIG_INT_RANGE("Video", "SyncMode", config_video.sync_mode, config_VideoSync_Disabled, config_VideoSync_Disabled, config_VideoSync_VRR);

    // Background colors
    CONFIG_FLOAT("Video", "BackgroundColorR", config_video.background_color[config_Theme_Dark][0], 0.1f);
    CONFIG_FLOAT("Video", "BackgroundColorG", config_video.background_color[config_Theme_Dark][1], 0.1f);
    CONFIG_FLOAT("Video", "BackgroundColorB", config_video.background_color[config_Theme_Dark][2], 0.1f);
    CONFIG_FLOAT("Video", "BackgroundColorDebuggerR", config_video.background_color_debugger[config_Theme_Dark][0], 0.2f);
    CONFIG_FLOAT("Video", "BackgroundColorDebuggerG", config_video.background_color_debugger[config_Theme_Dark][1], 0.2f);
    CONFIG_FLOAT("Video", "BackgroundColorDebuggerB", config_video.background_color_debugger[config_Theme_Dark][2], 0.2f);
    CONFIG_FLOAT("Video", "BackgroundColorLightR", config_video.background_color[config_Theme_Light][0], 128.0f / 255.0f);
    CONFIG_FLOAT("Video", "BackgroundColorLightG", config_video.background_color[config_Theme_Light][1], 128.0f / 255.0f);
    CONFIG_FLOAT("Video", "BackgroundColorLightB", config_video.background_color[config_Theme_Light][2], 128.0f / 255.0f);
    CONFIG_FLOAT("Video", "BackgroundColorDebuggerLightR", config_video.background_color_debugger[config_Theme_Light][0], 160.0f / 255.0f);
    CONFIG_FLOAT("Video", "BackgroundColorDebuggerLightG", config_video.background_color_debugger[config_Theme_Light][1], 160.0f / 255.0f);
    CONFIG_FLOAT("Video", "BackgroundColorDebuggerLightB", config_video.background_color_debugger[config_Theme_Light][2], 160.0f / 255.0f);

    //**************************************
    // Audio
    //**************************************

    CONFIG_BOOL("Audio", "Enable", config_audio.enable, true);
    CONFIG_BOOL("Audio", "Sync", config_audio.sync, true);
    CONFIG_FLOAT_RANGE("Audio", "MasterVolume", config_audio.master_volume, 1.0f, 0.0f, 2.0f);
    CONFIG_FLOAT_ARRAY("Audio", "Channel%dVolume", config_audio.volume, 4, 1.0f);
    CONFIG_INT("Audio", "LowpassCutoff", config_audio.lowpass_cutoff, 3000);
    CONFIG_INT("Audio", "BufferCount", config_audio.buffer_count, 3);

    //**************************************
    // Rewind
    //**************************************

    CONFIG_BOOL("Rewind", "Enabled", config_rewind.enabled, true);
    CONFIG_INT_RANGE("Rewind", "BufferSeconds", config_rewind.buffer_seconds, 10, 1, 10);
    CONFIG_INT_MIN("Rewind", "FramesPerSnapshot", config_rewind.frames_per_snapshot, 1, 1);
    CONFIG_FLOAT_RANGE("Rewind", "Speed", config_rewind.speed, 2.0f, 1.0f, 8.0f);

    //**************************************
    // Input
    //**************************************

    // Keyboard
    CONFIG_SCANCODE("Input", "KeyLeft", config_input.key_left, SDL_SCANCODE_LEFT);
    CONFIG_SCANCODE("Input", "KeyRight", config_input.key_right, SDL_SCANCODE_RIGHT);
    CONFIG_SCANCODE("Input", "KeyUp", config_input.key_up, SDL_SCANCODE_UP);
    CONFIG_SCANCODE("Input", "KeyDown", config_input.key_down, SDL_SCANCODE_DOWN);
    CONFIG_SCANCODE("Input", "KeyPause", config_input.key_pause, SDL_SCANCODE_S);
    CONFIG_SCANCODE("Input", "KeyOption1", config_input.key_option1, SDL_SCANCODE_A);
    CONFIG_SCANCODE("Input", "KeyOption2", config_input.key_option2, SDL_SCANCODE_D);
    CONFIG_SCANCODE("Input", "KeyA", config_input.key_A, SDL_SCANCODE_Z);
    CONFIG_SCANCODE("Input", "KeyB", config_input.key_B, SDL_SCANCODE_X);
    CONFIG_BOOL("Input", "AllowUpDown", config_input.allow_up_down, false);

    // Gamepad
    CONFIG_BOOL("Input", "Gamepad", config_input.gamepad, true);
    CONFIG_INT("Input", "GamepadDirectional", config_input.gamepad_directional, 0);
    CONFIG_BOOL("Input", "GamepadInvertX", config_input.gamepad_invert_x_axis, false);
    CONFIG_BOOL("Input", "GamepadInvertY", config_input.gamepad_invert_y_axis, false);
    CONFIG_INT("Input", "GamepadPause", config_input.gamepad_pause, SDL_GAMEPAD_BUTTON_START);
    CONFIG_INT("Input", "GamepadOption1", config_input.gamepad_option1, SDL_GAMEPAD_BUTTON_WEST);
    CONFIG_INT("Input", "GamepadOption2", config_input.gamepad_option2, SDL_GAMEPAD_BUTTON_NORTH);
    CONFIG_INT("Input", "GamepadX", config_input.gamepad_x_axis, SDL_GAMEPAD_AXIS_LEFTX);
    CONFIG_INT("Input", "GamepadY", config_input.gamepad_y_axis, SDL_GAMEPAD_AXIS_LEFTY);
    CONFIG_INT("Input", "GamepadA", config_input.gamepad_A, SDL_GAMEPAD_BUTTON_SOUTH);
    CONFIG_INT("Input", "GamepadB", config_input.gamepad_B, SDL_GAMEPAD_BUTTON_EAST);
    CONFIG_INT_ARRAY("InputGamepadShortcuts", "Shortcut%d", config_input_gamepad_shortcuts.gamepad_shortcuts, config_HotkeyIndex_COUNT, SDL_GAMEPAD_BUTTON_INVALID);

    // Hotkeys
    CONFIG_HOTKEY("OpenROM", config_hotkeys[config_HotkeyIndex_OpenROM], SDL_SCANCODE_O, SDL_KMOD_CTRL);
    CONFIG_HOTKEY("ReloadROM", config_hotkeys[config_HotkeyIndex_ReloadROM], SDL_SCANCODE_D, SDL_KMOD_CTRL);
    CONFIG_HOTKEY("Quit", config_hotkeys[config_HotkeyIndex_Quit], SDL_SCANCODE_Q, SDL_KMOD_CTRL);
    CONFIG_HOTKEY("Reset", config_hotkeys[config_HotkeyIndex_Reset], SDL_SCANCODE_R, SDL_KMOD_CTRL);
    CONFIG_HOTKEY("Pause", config_hotkeys[config_HotkeyIndex_Pause], SDL_SCANCODE_P, SDL_KMOD_CTRL);
    CONFIG_HOTKEY("FFWD", config_hotkeys[config_HotkeyIndex_FFWD], SDL_SCANCODE_F, SDL_KMOD_CTRL);
    CONFIG_HOTKEY("Rewind", config_hotkeys[config_HotkeyIndex_Rewind], SDL_SCANCODE_BACKSPACE, SDL_KMOD_NONE);
    CONFIG_HOTKEY("SaveState", config_hotkeys[config_HotkeyIndex_SaveState], SDL_SCANCODE_S, SDL_KMOD_CTRL);
    CONFIG_HOTKEY("LoadState", config_hotkeys[config_HotkeyIndex_LoadState], SDL_SCANCODE_L, SDL_KMOD_CTRL);
    CONFIG_HOTKEY("Screenshot", config_hotkeys[config_HotkeyIndex_Screenshot], SDL_SCANCODE_X, SDL_KMOD_CTRL);
    CONFIG_HOTKEY("Fullscreen", config_hotkeys[config_HotkeyIndex_Fullscreen], SDL_SCANCODE_F12, SDL_KMOD_NONE);
    CONFIG_HOTKEY("ShowMainMenu", config_hotkeys[config_HotkeyIndex_ShowMainMenu], SDL_SCANCODE_M, SDL_KMOD_CTRL);
    CONFIG_HOTKEY("DebugStepInto", config_hotkeys[config_HotkeyIndex_DebugStepInto], SDL_SCANCODE_F11, SDL_KMOD_NONE);
    CONFIG_HOTKEY("DebugStepOver", config_hotkeys[config_HotkeyIndex_DebugStepOver], SDL_SCANCODE_F10, SDL_KMOD_NONE);
    CONFIG_HOTKEY("DebugStepOut", config_hotkeys[config_HotkeyIndex_DebugStepOut], SDL_SCANCODE_F11, SDL_KMOD_SHIFT);
    CONFIG_HOTKEY("DebugStepFrame", config_hotkeys[config_HotkeyIndex_DebugStepFrame], SDL_SCANCODE_F6, SDL_KMOD_NONE);
    CONFIG_HOTKEY("DebugContinue", config_hotkeys[config_HotkeyIndex_DebugContinue], SDL_SCANCODE_F5, SDL_KMOD_NONE);
    CONFIG_HOTKEY("DebugBreak", config_hotkeys[config_HotkeyIndex_DebugBreak], SDL_SCANCODE_F7, SDL_KMOD_NONE);
    CONFIG_HOTKEY("DebugRunToCursor", config_hotkeys[config_HotkeyIndex_DebugRunToCursor], SDL_SCANCODE_F8, SDL_KMOD_NONE);
    CONFIG_HOTKEY("DebugBreakpoint", config_hotkeys[config_HotkeyIndex_DebugBreakpoint], SDL_SCANCODE_F9, SDL_KMOD_NONE);
    CONFIG_HOTKEY("DebugGoBack", config_hotkeys[config_HotkeyIndex_DebugGoBack], SDL_SCANCODE_BACKSPACE, SDL_KMOD_CTRL);
    CONFIG_HOTKEY("SelectSlot1", config_hotkeys[config_HotkeyIndex_SelectSlot1], SDL_SCANCODE_1, SDL_KMOD_CTRL);
    CONFIG_HOTKEY("SelectSlot2", config_hotkeys[config_HotkeyIndex_SelectSlot2], SDL_SCANCODE_2, SDL_KMOD_CTRL);
    CONFIG_HOTKEY("SelectSlot3", config_hotkeys[config_HotkeyIndex_SelectSlot3], SDL_SCANCODE_3, SDL_KMOD_CTRL);
    CONFIG_HOTKEY("SelectSlot4", config_hotkeys[config_HotkeyIndex_SelectSlot4], SDL_SCANCODE_4, SDL_KMOD_CTRL);
    CONFIG_HOTKEY("SelectSlot5", config_hotkeys[config_HotkeyIndex_SelectSlot5], SDL_SCANCODE_5, SDL_KMOD_CTRL);
    CONFIG_HOTKEY("Mute", config_hotkeys[config_HotkeyIndex_Mute], SDL_SCANCODE_U, SDL_KMOD_CTRL);
}

//**************************************
// Emulator-specific behavior
//**************************************

static void before_read(int file_version);
static void after_read(int file_version);
static void before_write(void);
static void after_write(void);
static void before_defaults(void);
static void after_defaults(void);
static void normalize(void);
static void migrate(int file_version);
static void sync_shader_preset_parameter_defaults(void);

static void before_read(int file_version)
{
    migrate(file_version);
}

static void after_read(int file_version)
{
    UNUSED(file_version);
    sync_shader_preset_parameter_defaults();
}

static void before_write(void)
{
    if (config_emulator.ffwd)
        config_audio.sync = true;
}

static void after_write(void)
{
    sync_shader_preset_parameter_defaults();
}

static void before_defaults(void)
{
}

static void after_defaults(void)
{
    config_emulator.paused = false;
    config_emulator.ffwd = false;
    config_emulator.show_info = false;
}

static void normalize(void)
{
#if defined(GLYNX_DISABLE_DISASSEMBLER)
    config_debug.debug = false;
#endif
#if !defined(_WIN32)
    if (config_video.sync_mode == config_VideoSync_VRR)
        config_video.sync_mode = config_VideoSync_Fixed;
#endif
}

static void migrate(int file_version)
{
    int sync_mode = -1;
    std::string stored;
    bool valid_sync_mode = get_setting("Video", "SyncMode", &stored) &&
        parse_int_string(stored, &sync_mode) && sync_mode >= config_VideoSync_Disabled &&
        sync_mode <= config_VideoSync_VRR;

    if (file_version < 4 || !valid_sync_mode)
    {
        bool sync = read_bool("Video", "Sync", true);
        bool vrr = read_bool("Video", "VRR", false);
        sync_mode = sync ? (vrr ? config_VideoSync_VRR : config_VideoSync_Fixed) : config_VideoSync_Disabled;
        write_int("Video", "SyncMode", sync_mode);
    }

    if (file_version < 5)
    {
        bool cpu = read_bool("Debug", "TraceCpu", true);
        bool cpu_irq = read_bool("Debug", "TraceCpuIrq", true);
        bool suzy_math = read_bool("Debug", "TraceSuzyMath", true);
        bool suzy_sprites = read_bool("Debug", "TraceSuzySprites", true);
        bool suzy_input = read_bool("Debug", "TraceSuzyInput", true);
        bool mikey_timers = read_bool("Debug", "TraceMikeyTimers", true);
        bool mikey_uart = read_bool("Debug", "TraceMikeyUart", true);
        bool redeye = read_bool("Debug", "TraceRedEye", true);
        bool mikey_audio = read_bool("Debug", "TraceMikeyAudio", true);
        bool cartridge = read_bool("Debug", "TraceCart", true);
        bool debug_messages = read_bool("Debug", "TraceDebugMessages", true);
        bool old_default = cpu && cpu_irq && suzy_math && suzy_sprites && suzy_input &&
            mikey_timers && mikey_uart && redeye && mikey_audio && cartridge && debug_messages;

        write_bool("Debug", "TraceCpu", cpu);
        write_bool("Debug", "TraceCpuIrq", cpu_irq);
        write_bool("Debug", "TraceSuzyMath", old_default ? false : suzy_math);
        write_bool("Debug", "TraceSuzySprites", old_default ? false : suzy_sprites);
        write_bool("Debug", "TraceSuzyInput", old_default ? false : suzy_input);
        write_bool("Debug", "TraceMikeyTimers", old_default ? false : mikey_timers);
        write_bool("Debug", "TraceMikeyInterrupts", false);
        write_bool("Debug", "TraceMikeyDisplay", false);
        write_bool("Debug", "TraceMikeyUart", old_default ? false : mikey_uart);
        write_bool("Debug", "TraceRedEye", old_default ? false : redeye);
        write_bool("Debug", "TraceMikeyAudio", old_default ? false : mikey_audio);
        write_bool("Debug", "TraceCart", old_default ? false : cartridge);
        write_bool("Debug", "TraceDebugMessages", old_default ? false : debug_messages);
        write_bool("Debug", "TraceCycles", false);
        write_int("Debug", "TraceSuzyMathEvents", TRACE_SUZY_MATH_FILTER_ALL);
        write_int("Debug", "TraceSuzySpriteEvents", TRACE_SUZY_SPRITE_FILTER_ALL);
        write_int("Debug", "TraceSuzyInputEvents", TRACE_SUZY_INPUT_FILTER_ALL);
        write_int("Debug", "TraceMikeyTimerEvents", TRACE_MIKEY_TIMER_FILTER_ALL);
        write_int("Debug", "TraceMikeyInterruptEvents", TRACE_MIKEY_INTERRUPT_FILTER_ALL);
        write_int("Debug", "TraceMikeyDisplayEvents", TRACE_MIKEY_DISPLAY_FILTER_ALL);
        write_int("Debug", "TraceMikeyUartEvents", TRACE_MIKEY_UART_FILTER_ALL);
        write_int("Debug", "TraceRedEyeEvents", TRACE_REDEYE_FILTER_ALL);
        write_int("Debug", "TraceMikeyAudioEvents", TRACE_MIKEY_AUDIO_FILTER_ALL);
        write_int("Debug", "TraceCartridgeEvents", TRACE_CARTRIDGE_FILTER_ALL);
        write_int("Debug", "TraceDebugEvents", TRACE_DEBUG_FILTER_MESSAGES);
    }

    int scale = 0;
    if (get_setting("Video", "Scale", &stored) && parse_int_string(stored, &scale) && scale > 3)
        write_int("Video", "Scale", scale - 2);
}

static void sync_shader_preset_parameter_defaults(void)
{
    ShaderPresetInfo presets[SHADER_PRESET_MAX_DISCOVERED];
    int preset_count = shader_preset_scan_bundled(presets, SHADER_PRESET_MAX_DISCOVERED);

    for (int i = 0; i < preset_count; i++)
    {
        ShaderPreset preset;
        char error[512];
        if (!shader_preset_load(presets[i].path, &preset, error, sizeof(error)))
            continue;

        char preset_file[SHADER_PRESET_MAX_PATH];
        if (!shader_preset_get_config_path(preset.preset_path, preset_file, sizeof(preset_file)))
            continue;

        std::string section = shader_preset_section_name(preset_file);
        for (int j = 0; j < preset.parameter_count; j++)
        {
            ShaderPresetParameter* parameter = &preset.parameters[j];
            if (config_ini_data[section].has(parameter->name))
                continue;

            write_float(section.c_str(), parameter->name, parameter->default_value);
        }
    }
}
