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

#define GUI_DEBUG_IMPORT
#include "gui_debug.h"

#include "gearlynx.h"
#include "imgui.h"
#include "gui_debug_disassembler.h"
#include "gui_debug_m6502.h"
#include "gui_debug_memory.h"
#include "gui_debug_psg.h"
#include "gui_debug_trace_logger.h"
#include "gui_debug_mikey.h"
#include "gui_debug_suzy.h"
#include "emu.h"
#include "config.h"

void gui_debug_init(void)
{
    gui_debug_disassembler_init();
    gui_debug_psg_init();
    gui_debug_memory_init();
}

void gui_debug_destroy(void)
{
    gui_debug_disassembler_destroy();
    gui_debug_psg_destroy();
    gui_debug_memory_destroy();
}

void gui_debug_reset(void)
{
    gui_debug_disassembler_reset();
    gui_debug_memory_reset();
    gui_debug_reset_breakpoints();
    gui_debug_reset_symbols();
}

void gui_debug_callback(void)
{
    gui_debug_trace_logger_update();
}

void gui_debug_windows(void)
{
    if (config_debug.debug)
    {
        if (config_debug.show_processor)
            gui_debug_window_m6502();
        if (config_debug.show_memory)
            gui_debug_window_memory();
        if (config_debug.show_disassembler)
            gui_debug_window_disassembler();
        if (config_debug.show_call_stack)
            gui_debug_window_call_stack();
        if (config_debug.show_psg)
            gui_debug_window_psg();
        if (config_debug.show_trace_logger)
            gui_debug_window_trace_logger();
        if (config_debug.show_mikey_regs)
            gui_debug_window_mikey_regs();
        if (config_debug.show_mikey_timers)
            gui_debug_window_mikey_timers();
        if (config_debug.show_mikey_audio)
            gui_debug_window_mikey_audio();
        if (config_debug.show_mikey_colors)
            gui_debug_window_mikey_colors();
        if (config_debug.show_suzy_regs)
            gui_debug_window_suzy_regs();
        if (config_debug.show_suzy_math_regs)
            gui_debug_window_suzy_math_regs();
        gui_debug_memory_watches_window();
        gui_debug_memory_search_window();
    }
}
