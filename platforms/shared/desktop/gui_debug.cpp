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

#include <fstream>
#include "gearlynx.h"
#include "imgui.h"
#include "gui_debug_disassembler.h"
#include "gui_debug_m6502.h"
#include "gui_debug_memory.h"
#include "gui_debug_psg.h"
#include "gui_debug_trace_logger.h"
#include "gui_debug_mikey.h"
#include "gui_debug_suzy.h"
#include "gui_debug_timers.h"
#include "gui_debug_uart.h"
#include "gui_debug_lcd.h"
#include "gui_debug_eeprom.h"
#include "gui_debug_cart.h"
#include "emu.h"
#include "config.h"

void gui_debug_init(void)
{
    gui_debug_disassembler_init();
    gui_debug_psg_init();
    gui_debug_memory_init();
    gui_debug_timers_init();
    gui_debug_uart_init();
}

void gui_debug_destroy(void)
{
    gui_debug_disassembler_destroy();
    gui_debug_psg_destroy();
    gui_debug_memory_destroy();
    gui_debug_timers_destroy();
    gui_debug_uart_destroy();
}

void gui_debug_reset(void)
{
    gui_debug_disassembler_reset();
    gui_debug_memory_reset();
    gui_debug_reset_breakpoints();
    gui_debug_reset_symbols();
    gui_debug_reset_disassembler_bookmarks();
    gui_debug_reset_memory_bookmarks();
    gui_debug_reset_memory_watches();
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
        if (config_debug.show_breakpoints)
            gui_debug_window_breakpoints();
        if (config_debug.show_symbols)
            gui_debug_window_symbols();
        if (config_debug.show_psg)
            gui_debug_window_psg();
        if (config_debug.show_trace_logger)
            gui_debug_window_trace_logger();
        if (config_debug.show_mikey_regs)
            gui_debug_window_mikey_regs();
        if (config_debug.show_mikey_timers)
            gui_debug_window_timers();
        if (config_debug.show_mikey_colors)
            gui_debug_window_mikey_colors();
        if (config_debug.show_suzy_regs)
            gui_debug_window_suzy_regs();
        if (config_debug.show_suzy_math_regs)
            gui_debug_window_suzy_math_regs();
        if (config_debug.show_frame_buffers)
            gui_debug_window_frame_buffers();
        if (config_debug.show_lcd)
            gui_debug_window_lcd();
        if (config_debug.show_uart)
            gui_debug_window_uart();
        if (config_debug.show_eeprom)
            gui_debug_window_eeprom();
        if (config_debug.show_cart)
            gui_debug_window_cart();

        gui_debug_memory_watches_window();
        gui_debug_memory_search_window();
    }
}

static const char* GLDEBUG_MAGIC = "GLDEBUG1";
static const int GLDEBUG_MAGIC_LEN = 8;

void gui_debug_save_settings(const char* file_path)
{
    std::ofstream file(file_path, std::ios::binary);
    if (!file.is_open())
    {
        Log("Failed to open debug settings file for writing: %s", file_path);
        return;
    }

    file.write(GLDEBUG_MAGIC, GLDEBUG_MAGIC_LEN);

    GearlynxCore* core = emu_get_core();
    M6502* processor = core->GetM6502();

    std::vector<M6502::GLYNX_Breakpoint>* breakpoints = processor->GetBreakpoints();
    int bp_count = (int)breakpoints->size();
    file.write((const char*)&bp_count, sizeof(int));
    for (int i = 0; i < bp_count; i++)
    {
        M6502::GLYNX_Breakpoint& bp = (*breakpoints)[i];
        file.write((const char*)&bp.enabled, sizeof(bool));
        file.write((const char*)&bp.address1, sizeof(u16));
        file.write((const char*)&bp.address2, sizeof(u16));
        file.write((const char*)&bp.read, sizeof(bool));
        file.write((const char*)&bp.write, sizeof(bool));
        file.write((const char*)&bp.execute, sizeof(bool));
        file.write((const char*)&bp.range, sizeof(bool));
    }

    file.write((const char*)emu_debug_irq_breakpoints, sizeof(bool) * 8);

    void* bookmarks_ptr = NULL;
    int bookmark_count = gui_debug_get_disassembler_bookmarks(&bookmarks_ptr);
    file.write((const char*)&bookmark_count, sizeof(int));
    if (bookmark_count > 0 && bookmarks_ptr != NULL)
    {
        struct DasmBookmark { u16 address; char name[32]; };
        std::vector<DasmBookmark>* bm_vec = (std::vector<DasmBookmark>*)bookmarks_ptr;
        for (int i = 0; i < bookmark_count; i++)
        {
            file.write((const char*)&(*bm_vec)[i].address, sizeof(u16));
            file.write((*bm_vec)[i].name, 32);
        }
    }

    gui_debug_memory_save_settings(file);

    file.close();

    Log("Debug settings saved to: %s", file_path);
}

void gui_debug_load_settings(const char* file_path)
{
    std::ifstream file(file_path, std::ios::binary);
    if (!file.is_open())
    {
        Log("Failed to open debug settings file for reading: %s", file_path);
        return;
    }

    char magic[8];
    file.read(magic, GLDEBUG_MAGIC_LEN);
    if (memcmp(magic, GLDEBUG_MAGIC, GLDEBUG_MAGIC_LEN) != 0)
    {
        Log("Invalid debug settings file: %s", file_path);
        file.close();
        return;
    }

    GearlynxCore* core = emu_get_core();
    M6502* processor = core->GetM6502();

    processor->ResetBreakpoints();
    int bp_count = 0;
    file.read((char*)&bp_count, sizeof(int));
    std::vector<M6502::GLYNX_Breakpoint>* breakpoints = processor->GetBreakpoints();
    for (int i = 0; i < bp_count; i++)
    {
        M6502::GLYNX_Breakpoint bp;
        file.read((char*)&bp.enabled, sizeof(bool));
        file.read((char*)&bp.address1, sizeof(u16));
        file.read((char*)&bp.address2, sizeof(u16));
        file.read((char*)&bp.read, sizeof(bool));
        file.read((char*)&bp.write, sizeof(bool));
        file.read((char*)&bp.execute, sizeof(bool));
        file.read((char*)&bp.range, sizeof(bool));
        breakpoints->push_back(bp);
    }

    file.read((char*)emu_debug_irq_breakpoints, sizeof(bool) * 8);

    gui_debug_reset_disassembler_bookmarks();
    int bookmark_count = 0;
    file.read((char*)&bookmark_count, sizeof(int));
    for (int i = 0; i < bookmark_count; i++)
    {
        u16 address;
        char name[32];
        file.read((char*)&address, sizeof(u16));
        file.read(name, 32);
        gui_debug_add_disassembler_bookmark(address, name);
    }

    gui_debug_memory_load_settings(file);

    file.close();

    Log("Debug settings loaded from: %s", file_path);
}
