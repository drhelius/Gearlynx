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

#include "imgui.h"
#include "gearlynx.h"
#include "gui_debug_constants.h"
#include "gui_debug_memory.h"
#include "gui_debug_widgets.h"
#include "gui.h"
#include "config.h"
#include "emu.h"
#include "utils.h"

enum M6502RegId
{
    M6502RegId_A = 0,
    M6502RegId_S = 1,
    M6502RegId_X = 2,
    M6502RegId_Y = 3,
    M6502RegId_P = 4,
    M6502RegId_PC = 5,
    M6502RegId_SP = 6
};

static void M6502WriteCallback8(u16 reg_id, u8 value, void* user_data)
{
    M6502::M6502_State* cpu = (M6502::M6502_State*)user_data;
    switch (reg_id)
    {
        case M6502RegId_A: cpu->A.SetValue(value); break;
        case M6502RegId_S: cpu->S.SetValue(value); break;
        case M6502RegId_X: cpu->X.SetValue(value); break;
        case M6502RegId_Y: cpu->Y.SetValue(value); break;
        case M6502RegId_P: cpu->P.SetValue(value); break;
    }
}

static void M6502WriteCallback1(u16 reg_id, u8 bit_index, bool value, void* user_data)
{
    M6502::M6502_State* cpu = (M6502::M6502_State*)user_data;
    if (reg_id == M6502RegId_P)
    {
        u8 p = cpu->P.GetValue();
        if (value)
            p |= (1 << bit_index);
        else
            p &= ~(1 << bit_index);
        cpu->P.SetValue(p);
    }
}

static void MemoryWriteCallback8(u16 address, u8 value, void* user_data)
{
    Memory* memory = (Memory*)user_data;
    memory->Write<true>(address, value);
}

static void M6502WriteCallback16(u16 reg_id, u16 value, void* user_data)
{
    M6502::M6502_State* cpu = (M6502::M6502_State*)user_data;
    switch (reg_id)
    {
        case M6502RegId_PC: cpu->PC.SetValue(value); break;
        case M6502RegId_SP: cpu->S.SetValue(value & 0xFF); break;
    }
}

void gui_debug_window_m6502(void)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(3, 26), ImGuiCond_FirstUseEver);

    ImGui::Begin("Mikey 65C02", &config_debug.show_processor, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize);

    ImGui::PushFont(gui_default_font);

    GearlynxCore* core = emu_get_core();
    M6502::M6502_State* cpu = core->GetM6502()->GetState();
    Memory* memory = core->GetMemory();
    Memory::Memory_State* mem = memory->GetState();
    Mikey::Mikey_State* mikey = core->GetMikey()->GetState();

    if (ImGui::BeginTable("m6502", 1, ImGuiTableFlags_BordersInnerH))
    {
        ImGui::TableNextColumn();
        u8 p = cpu->P.GetValue();
        ImGui::Text("  ");
        ImGui::SameLine(0, 0); ImGui::TextColored(orange, "N");
        ImGui::SameLine(); ImGui::TextColored(orange, "V");
        ImGui::SameLine(); ImGui::TextColored(orange, "-");
        ImGui::SameLine(); ImGui::TextColored(orange, "B");
        ImGui::SameLine(); ImGui::TextColored(orange, "D");
        ImGui::SameLine(); ImGui::TextColored(orange, "I");
        ImGui::SameLine(); ImGui::TextColored(orange, "Z");
        ImGui::SameLine(); ImGui::TextColored(orange, "C");
        ImGui::Text("  ");
        ImGui::SameLine(0, 0); EditableRegister1(M6502RegId_P, 7, (p >> 7) & 1, M6502WriteCallback1, cpu);
        ImGui::SameLine(); EditableRegister1(M6502RegId_P, 6, (p >> 6) & 1, M6502WriteCallback1, cpu);
        ImGui::SameLine(); EditableRegister1(M6502RegId_P, 5, (p >> 5) & 1, nullptr, cpu, gray, gray);
        ImGui::SameLine(); EditableRegister1(M6502RegId_P, 4, (p >> 4) & 1, M6502WriteCallback1, cpu);
        ImGui::SameLine(); EditableRegister1(M6502RegId_P, 3, (p >> 3) & 1, M6502WriteCallback1, cpu);
        ImGui::SameLine(); EditableRegister1(M6502RegId_P, 2, (p >> 2) & 1, M6502WriteCallback1, cpu);
        ImGui::SameLine(); EditableRegister1(M6502RegId_P, 1, (p >> 1) & 1, M6502WriteCallback1, cpu);
        ImGui::SameLine(); EditableRegister1(M6502RegId_P, 0, p & 1, M6502WriteCallback1, cpu);

        ImGui::TableNextColumn();
        ImGui::TextColored(yellow, "     PC"); ImGui::SameLine();
        ImGui::Text("  "); ImGui::SameLine(0, 0);
        EditableRegister16(NULL, NULL, M6502RegId_PC, cpu->PC.GetValue(), M6502WriteCallback16, cpu, EditableRegisterFlags_None);
        if (ImGui::IsItemClicked())
            gui_debug_memory_goto(MEMORY_EDITOR_RAM, cpu->PC.GetValue());
        ImGui::TextColored(gray, " " BYTE_TO_BINARY_PATTERN_SPACED " " BYTE_TO_BINARY_PATTERN_SPACED " ", BYTE_TO_BINARY(cpu->PC.GetHigh()), BYTE_TO_BINARY(cpu->PC.GetLow()));
        if (ImGui::IsItemClicked())
            gui_debug_memory_goto(MEMORY_EDITOR_RAM, cpu->PC.GetValue());

        ImGui::TableNextColumn();
        ImGui::TextColored(yellow, "     SP"); ImGui::SameLine();
        ImGui::Text("  "); ImGui::SameLine(0, 0);
        EditableRegister16(NULL, NULL, M6502RegId_SP, STACK_ADDR | cpu->S.GetValue(), M6502WriteCallback16, cpu, EditableRegisterFlags_None);
        if (ImGui::IsItemClicked())
            gui_debug_memory_goto(MEMORY_EDITOR_STACK, STACK_ADDR | cpu->S.GetValue());
        ImGui::TextColored(gray, " " BYTE_TO_BINARY_PATTERN_SPACED " " BYTE_TO_BINARY_PATTERN_SPACED " ", BYTE_TO_BINARY(0x01), BYTE_TO_BINARY(cpu->S.GetValue()));
        if (ImGui::IsItemClicked())
            gui_debug_memory_goto(MEMORY_EDITOR_STACK, STACK_ADDR | cpu->S.GetValue());

        ImGui::TableNextColumn();

        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(2.0f, 2.0f));

        if (ImGui::BeginTable("regs", 2, ImGuiTableFlags_BordersInnerH |ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_NoPadOuterX))
        {
            ImGui::TableNextColumn();
            ImGui::TextColored(cyan, "  A"); ImGui::SameLine();
            ImGui::Text("  "); ImGui::SameLine(0, 0);
            EditableRegister8(NULL, NULL, M6502RegId_A, cpu->A.GetValue(), M6502WriteCallback8, cpu, EditableRegisterFlags_None);
            ImGui::TextColored(gray, " " BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(cpu->A.GetValue()));

            ImGui::TableNextColumn();
            ImGui::TextColored(cyan, "  S"); ImGui::SameLine();
            ImGui::Text("  "); ImGui::SameLine(0, 0);
            EditableRegister8(NULL, NULL, M6502RegId_S, cpu->S.GetValue(), M6502WriteCallback8, cpu, EditableRegisterFlags_None);
            ImGui::TextColored(gray, " " BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(cpu->S.GetValue()));

            ImGui::TableNextColumn();
            ImGui::TextColored(cyan, "  X"); ImGui::SameLine();
            ImGui::Text("  "); ImGui::SameLine(0, 0);
            EditableRegister8(NULL, NULL, M6502RegId_X, cpu->X.GetValue(), M6502WriteCallback8, cpu, EditableRegisterFlags_None);
            ImGui::TextColored(gray, " " BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(cpu->X.GetValue()));

            ImGui::TableNextColumn();
            ImGui::TextColored(cyan, "  Y"); ImGui::SameLine();
            ImGui::Text("  "); ImGui::SameLine(0, 0);
            EditableRegister8(NULL, NULL, M6502RegId_Y, cpu->Y.GetValue(), M6502WriteCallback8, cpu, EditableRegisterFlags_None);
            ImGui::TextColored(gray, " " BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(cpu->Y.GetValue()));

            ImGui::TableNextColumn();
            ImGui::TextColored(violet, "MAPCTL"); ImGui::SameLine();
            EditableRegister8(NULL, NULL, 0xFFF9, mem->MAPCTL, MemoryWriteCallback8, memory, EditableRegisterFlags_None);
            ImGui::TextColored(gray, " " BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(mem->MAPCTL));

            ImGui::TableNextColumn();
            ImGui::TextColored(blue, "IRQPEN"); ImGui::SameLine();
            ImGui::Text("$%02X", mikey->irq_pending);
            ImGui::TextColored(gray, " " BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(mikey->irq_pending));

            bool suzy_visible = !(mem->MAPCTL & 0x01);
            bool mikey_visible = !(mem->MAPCTL & 0x02);
            bool rom_visible = !(mem->MAPCTL & 0x04);
            bool vectors_visible = !(mem->MAPCTL & 0x08);

            ImGui::TableNextColumn();
            ImGui::TextColored(violet, "SUZY  "); ImGui::SameLine();
            ImGui::TextColored(suzy_visible ? green : gray, suzy_visible ? "ON" : "OFF");
            ImGui::TextColored(brown, " FC00-FCFF");

            ImGui::TableNextColumn();
            ImGui::TextColored(violet, "MIKEY "); ImGui::SameLine();
            ImGui::TextColored(mikey_visible ? green : gray, mikey_visible ? "ON" : "OFF");
            ImGui::TextColored(brown, " FD00-FDFF");

            ImGui::TableNextColumn();
            ImGui::TextColored(violet, "ROM   "); ImGui::SameLine();
            ImGui::TextColored(rom_visible ? green : gray, rom_visible ? "ON" : "OFF");
            ImGui::TextColored(brown, " FE00-FFF7");

            ImGui::TableNextColumn();
            ImGui::TextColored(violet, "VECTOR"); ImGui::SameLine();
            ImGui::TextColored(vectors_visible ? green : gray, vectors_visible ? "ON" : "OFF");
            ImGui::TextColored(brown, " FFFA-FFFF");

            for (int i = 0; i < 8; i++)
            {
                GLYNX_Mikey_Timer* timer = &mikey->timers[i];
                bool enabled = IS_SET_BIT(timer->control_a, 7);
                if (i == 4)
                    enabled = (mikey->uart.tx_int_en || mikey->uart.rx_int_en);

                bool asserted = mikey->irq_pending & (1 << i);
                ImGui::TableNextColumn();
                ImGui::TextColored(blue, "IRQ %d ", i); ImGui::SameLine();
                ImGui::TextColored(enabled ? green : gray, enabled ? "ON" : "OFF");
                ImGui::TextColored(asserted ? green : gray, " ASSERTED");
            }

            ImGui::EndTable();
        }

        ImGui::PopStyleVar();

        ImGui::TableNextColumn();

        ImGui::TextColored(magenta, " IRQ LINE: "); ImGui::SameLine();
        ImGui::TextColored(cpu->irq_asserted ? green : gray, "ASSERTED");

        ImGui::EndTable();
    }

    ImGui::PopFont();

    ImGui::End();
    ImGui::PopStyleVar();
}
