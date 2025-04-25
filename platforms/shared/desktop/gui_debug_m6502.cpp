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

#define GUI_DEBUG_M6502_IMPORT
#include "gui_debug_m6502.h"

#include "imgui/imgui.h"
#include "../../../src/gearlynx.h"
#include "gui_debug_constants.h"
#include "gui_debug_memory.h"
#include "gui.h"
#include "config.h"
#include "emu.h"
#include "utils.h"

static void get_bank_name(u8 mpr, u8 mpr_value, char *name, char* tooltip);
static void goto_address(u8 mpr_value);

void gui_debug_window_m6502(void)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(3, 26), ImGuiCond_FirstUseEver);

    ImGui::Begin("65C02", &config_debug.show_processor, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize);

    ImGui::PushFont(gui_default_font);

    GearlynxCore* core = emu_get_core();
    M6502* processor = core->GetM6502();
    M6502::M6502_State* proc_state = processor->GetState();

    if (ImGui::BeginTable("m6502", 1, ImGuiTableFlags_BordersInnerH))
    {
        ImGui::TableNextColumn();
        ImGui::TextColored(cyan, "      STATUS");
        ImGui::TextColored(magenta, "  N V - B D I Z C");
        ImGui::Text("  " BYTE_TO_BINARY_PATTERN_ALL_SPACED, BYTE_TO_BINARY(proc_state->P->GetValue()));

        ImGui::TableNextColumn();
        ImGui::TextColored(yellow, "    PC"); ImGui::SameLine();
        ImGui::Text("= $%04X", proc_state->PC->GetValue());
        ImGui::Text(BYTE_TO_BINARY_PATTERN_SPACED " " BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(proc_state->PC->GetHigh()), BYTE_TO_BINARY(proc_state->PC->GetLow()));

        ImGui::TableNextColumn();
        ImGui::TextColored(yellow, "    SP"); ImGui::SameLine();
        if (ImGui::IsItemClicked())
            gui_debug_memory_goto(MEMORY_EDITOR_RAM, (STACK_ADDR - 0x2000) | proc_state->S->GetValue());
        ImGui::Text("= $%04X", STACK_ADDR | proc_state->S->GetValue());
        if (ImGui::IsItemClicked())
            gui_debug_memory_goto(MEMORY_EDITOR_RAM, (STACK_ADDR - 0x2000) | proc_state->S->GetValue());
        ImGui::Text(BYTE_TO_BINARY_PATTERN_SPACED " " BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(0x21), BYTE_TO_BINARY(proc_state->S->GetValue()));
        if (ImGui::IsItemClicked())
            gui_debug_memory_goto(MEMORY_EDITOR_RAM, (STACK_ADDR - 0x2000) | proc_state->S->GetValue());

        ImGui::TableNextColumn();

        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(2.0f, 2.0f));

        if (ImGui::BeginTable("regs", 2, ImGuiTableFlags_BordersInnerH |ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_NoPadOuterX))
        {
            ImGui::TableNextColumn();
            ImGui::TextColored(cyan, " A"); ImGui::SameLine();
            ImGui::Text("   $%02X", proc_state->A->GetValue());
            ImGui::Text(BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(proc_state->A->GetValue()));

            ImGui::TableNextColumn();
            ImGui::TextColored(cyan, " S"); ImGui::SameLine();
            ImGui::Text("   $%02X", proc_state->S->GetValue());
            ImGui::Text(BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(proc_state->S->GetValue()));

            ImGui::TableNextColumn();
            ImGui::TextColored(cyan, " X"); ImGui::SameLine();
            ImGui::Text("   $%02X", proc_state->X->GetValue());
            ImGui::Text(BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(proc_state->X->GetValue()));

            ImGui::TableNextColumn();
            ImGui::TextColored(cyan, " Y"); ImGui::SameLine();
            ImGui::Text("   $%02X", proc_state->Y->GetValue());
            ImGui::Text(BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(proc_state->Y->GetValue()));

            ImGui::EndTable();
        }

        ImGui::PopStyleVar();

        ImGui::TableNextColumn();

        ImGui::TextColored(magenta, "IRQ1:"); ImGui::SameLine();
        ImGui::TextColored(*proc_state->IDR & 0x02 ? gray : green, *proc_state->IDR & 0x02 ? "OFF" : "ON "); ImGui::SameLine();
        ImGui::TextColored(*proc_state->IRR & 0x02 ? green : gray, "ASSERTED");

        ImGui::TextColored(magenta, "IRQ2:"); ImGui::SameLine();
        ImGui::TextColored(*proc_state->IDR & 0x01 ? gray : green, *proc_state->IDR & 0x01 ? "OFF" : "ON "); ImGui::SameLine();
        ImGui::TextColored(*proc_state->IRR & 0x01 ? green : gray, "ASSERTED");

        ImGui::TextColored(magenta, "TIQ: "); ImGui::SameLine();
        ImGui::TextColored(*proc_state->IDR & 0x04 ? gray : green, *proc_state->IDR & 0x04 ? "OFF" : "ON "); ImGui::SameLine();
        ImGui::TextColored(*proc_state->IRR & 0x04 ? green : gray, "ASSERTED");

        ImGui::EndTable();
    }

    ImGui::PopFont();

    ImGui::End();
    ImGui::PopStyleVar();
}

static void get_bank_name(u8 mpr, u8 mpr_value, char *name, char* tooltip)
{
    u16 cpu_address = mpr << 13;

    // 0x00 - 0x7F
    if (mpr_value < 0x80)
    {
        u32 rom_address = mpr_value << 13;
        snprintf(name, 16, "ROM $%02X", mpr_value);
        snprintf(tooltip, 128, "Range (CPU) $%04X-$%04X \nRange (ROM) $%06X-$%06X",
            cpu_address, cpu_address + 0x1FFF,  rom_address,  rom_address + 0x1FFF);
    }
    // 0x80 - 0xF6
    else if (mpr_value < 0xF7)
    {
        snprintf(name, 16, "UNUSED");
        snprintf(tooltip, 128, "Range (CPU) $%04X-$%04X", cpu_address, cpu_address + 0x1FFF);
    }
    // 0xF7
    else if (mpr_value < 0xF8)
    {
        snprintf(name, 16, "BRAM");
        snprintf(tooltip, 128, "Range (CPU) $%04X-$%04X", cpu_address, cpu_address + 0x1FFF);
    }
    // 0xF8 - 0xFB
    else if (mpr_value < 0xFC)
    {
        u8 ram_bank = mpr_value - 0xF8;
        u16 ram_address = ram_bank << 13;
        snprintf(name, 16, "WRAM $%02X", ram_bank);
        snprintf(tooltip, 128, "Range (CPU) $%04X-$%04X \nRange (WRAM) $%04X-$%04X",
            cpu_address, cpu_address + 0x1FFF,  ram_address,  ram_address + 0x1FFF);
    }
    // 0xFC - 0xFE
    else if (mpr_value < 0xFF)
    {
        snprintf(name, 16, "UNUSED");
        snprintf(tooltip, 128, "Range (CPU) $%04X-$%04X", cpu_address, cpu_address + 0x1FFF);
    }
    // 0xFF
    else
    {
        snprintf(name, 16, "HARDWARE");
        snprintf(tooltip, 128, "Range (CPU) $%04X-$%04X", cpu_address, cpu_address + 0x1FFF);
    }
}

static void goto_address(u8 mpr_value)
{
    // 0x00 - 0x7F
    if (mpr_value < 0x80)
    {
        u32 rom_address = mpr_value << 13;
        gui_debug_memory_goto(MEMORY_EDITOR_ROM, rom_address);
    }
    // 0xF8 - 0xFB
    else if (mpr_value < 0xFC)
    {
        u8 ram_bank = mpr_value - 0xF8;
        u16 ram_address = ram_bank << 13;
        gui_debug_memory_goto(MEMORY_EDITOR_RAM, ram_address);
    }
}
