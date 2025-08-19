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
#include "gui.h"
#include "config.h"
#include "emu.h"
#include "utils.h"

void gui_debug_window_m6502(void)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(3, 26), ImGuiCond_FirstUseEver);

    ImGui::Begin("Suzy 65C02", &config_debug.show_processor, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize);

    ImGui::PushFont(gui_default_font);

    GearlynxCore* core = emu_get_core();
    M6502::M6502_State* cpu = core->GetM6502()->GetState();
    Memory::Memory_State* mem = core->GetMemory()->GetState();
    Mikey::Mikey_State* mikey = core->GetMikey()->GetState();

    if (ImGui::BeginTable("m6502", 1, ImGuiTableFlags_BordersInnerH))
    {
        ImGui::TableNextColumn();
        ImGui::TextColored(orange, "   N V - B D I Z C");
        ImGui::Text("   " BYTE_TO_BINARY_PATTERN_ALL_SPACED, BYTE_TO_BINARY(cpu->P.GetValue()));

        ImGui::TableNextColumn();
        ImGui::TextColored(yellow, "     PC"); ImGui::SameLine();
        if (ImGui::IsItemClicked())
            gui_debug_memory_goto(MEMORY_EDITOR_RAM, cpu->PC.GetValue());
        ImGui::Text("= $%04X", cpu->PC.GetValue());
        if (ImGui::IsItemClicked())
            gui_debug_memory_goto(MEMORY_EDITOR_RAM, cpu->PC.GetValue());
        ImGui::Text(" " BYTE_TO_BINARY_PATTERN_SPACED " " BYTE_TO_BINARY_PATTERN_SPACED " ", BYTE_TO_BINARY(cpu->PC.GetHigh()), BYTE_TO_BINARY(cpu->PC.GetLow()));
        if (ImGui::IsItemClicked())
            gui_debug_memory_goto(MEMORY_EDITOR_RAM, cpu->PC.GetValue());

        ImGui::TableNextColumn();
        ImGui::TextColored(yellow, "     SP"); ImGui::SameLine();
        if (ImGui::IsItemClicked())
            gui_debug_memory_goto(MEMORY_EDITOR_STACK, STACK_ADDR | cpu->S.GetValue());
        ImGui::Text("= $%04X", STACK_ADDR | cpu->S.GetValue());
        if (ImGui::IsItemClicked())
            gui_debug_memory_goto(MEMORY_EDITOR_STACK, STACK_ADDR | cpu->S.GetValue());
        ImGui::Text(" " BYTE_TO_BINARY_PATTERN_SPACED " " BYTE_TO_BINARY_PATTERN_SPACED " ", BYTE_TO_BINARY(0x21), BYTE_TO_BINARY(cpu->S.GetValue()));
        if (ImGui::IsItemClicked())
            gui_debug_memory_goto(MEMORY_EDITOR_STACK, STACK_ADDR | cpu->S.GetValue());

        ImGui::TableNextColumn();

        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(2.0f, 2.0f));

        if (ImGui::BeginTable("regs", 2, ImGuiTableFlags_BordersInnerH |ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_NoPadOuterX))
        {
            ImGui::TableNextColumn();
            ImGui::TextColored(cyan, "  A"); ImGui::SameLine();
            ImGui::Text("   $%02X", cpu->A.GetValue());
            ImGui::Text(" " BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(cpu->A.GetValue()));

            ImGui::TableNextColumn();
            ImGui::TextColored(cyan, "  S"); ImGui::SameLine();
            ImGui::Text("   $%02X", cpu->S.GetValue());
            ImGui::Text(" " BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(cpu->S.GetValue()));

            ImGui::TableNextColumn();
            ImGui::TextColored(cyan, "  X"); ImGui::SameLine();
            ImGui::Text("   $%02X", cpu->X.GetValue());
            ImGui::Text(" " BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(cpu->X.GetValue()));

            ImGui::TableNextColumn();
            ImGui::TextColored(cyan, "  Y"); ImGui::SameLine();
            ImGui::Text("   $%02X", cpu->Y.GetValue());
            ImGui::Text(" " BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(cpu->Y.GetValue()));

            ImGui::TableNextColumn();
            ImGui::TextColored(violet, "MAPCTL"); ImGui::SameLine();
            ImGui::Text("$%02X", mem->MAPCTL);
            ImGui::Text(" " BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(mem->MAPCTL));

            ImGui::TableNextColumn();
            ImGui::TextColored(blue, "IRQPEN"); ImGui::SameLine();
            ImGui::Text("$%02X", mikey->irq_pending);
            ImGui::Text(" " BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(mikey->irq_pending));

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
