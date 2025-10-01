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

#define GUI_DEBUG_MIKEY_IMPORT
#include "gui_debug_mikey.h"

#include "imgui.h"
#include "gearlynx.h"
#include "gui_debug_constants.h"
#include "gui_debug_memory.h"
#include "gui.h"
#include "config.h"
#include "emu.h"
#include "utils.h"

static ImVec4 color_444_to_float(u16 color);

void gui_debug_window_mikey_regs(void)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(210, 162), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(328, 340), ImGuiCond_FirstUseEver);

    ImGui::Begin("Mikey Registers", &config_debug.show_mikey_regs);

    ImGui::PushFont(gui_default_font);

    GearlynxCore* core = emu_get_core();
    Mikey* mikey = core->GetMikey();
    Mikey::Mikey_State* mikey_state = mikey->GetState();
    u8 iodat = mikey->Read(MIKEY_IODAT);

    struct {
        const char* name;
        const char* addr;
        u8* reg;
    } regs8[] = {
        {"ATTEN_A ", "FD40", &mikey_state->ATTEN_A},
        {"ATTEN_B ", "FD41", &mikey_state->ATTEN_B},
        {"ATTEN_C ", "FD42", &mikey_state->ATTEN_C},
        {"ATTEN_D ", "FD43", &mikey_state->ATTEN_D},
        {"MPAN    ", "FD44", &mikey_state->MPAN},
        {"MSTEREO ", "FD50", &mikey_state->MSTEREO},
        {"INTRST  ", "FD80", &mikey_state->irq_pending},
        {"INTSET  ", "FD81", &mikey_state->irq_pending},
        {"SYSCTL1 ", "FD87", &mikey_state->SYSCTL1},
        {"IODIR   ", "FD8A", &mikey_state->IODIR},
        {"IODAT   ", "FD8B", &iodat},
        {"SERCTL  ", "FD8C", &mikey_state->SERCTL},
        {"SERDAT  ", "FD8D", &mikey_state->SERDAT},
        {"SDONEACK", "FD90", &mikey_state->SDONEACK},
        {"CPUSLEEP", "FD91", &mikey_state->CPUSLEEP},
        {"DISPCTL ", "FD92", &mikey_state->DISPCTL},
        {"PBKUP   ", "FD93", &mikey_state->PBKUP},
        {0, 0, 0}
    };

    int i = 0;
    while (regs8[i].name != 0)
    {
        ImGui::TextColored(cyan, "%s ", regs8[i].addr); ImGui::SameLine();
        ImGui::TextColored(orange, "%s ", regs8[i].name); ImGui::SameLine();
        ImGui::Text("$%02X (" BYTE_TO_BINARY_PATTERN_SPACED ")", *regs8[i].reg, BYTE_TO_BINARY(*regs8[i].reg));
        i++;
    }

    struct {
        const char* name;
        const char* addr;
        u16_union* reg;
    } regs16[] = {
        {"DISPADR ", "FD94", &mikey_state->DISPADR},
        {0, 0, 0}
    };
    i = 0;
    while (regs16[i].name != 0)
    {
        ImGui::TextColored(cyan, "%s ", regs16[i].addr); ImGui::SameLine();
        ImGui::TextColored(orange, "%s ", regs16[i].name); ImGui::SameLine();
        ImGui::Text("$%04X (" BYTE_TO_BINARY_PATTERN_SPACED " " BYTE_TO_BINARY_PATTERN_SPACED ")", regs16[i].reg->value, BYTE_TO_BINARY(regs16[i].reg->high), BYTE_TO_BINARY(regs16[i].reg->low));
        i++;
    }

    ImGui::PopFont();

    ImGui::End();
    ImGui::PopStyleVar();
}

void gui_debug_window_mikey_colors(void)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(212, 128), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(338, 400), ImGuiCond_FirstUseEver);

    ImGui::Begin("Mikey Color Registers", &config_debug.show_mikey_colors);
    ImGui::PushFont(gui_default_font);

    GearlynxCore* core = emu_get_core();
    Mikey::Mikey_State* mikey_state = core->GetMikey()->GetState();

    const u16 base = 0xFDA0;

    for (int line = 0; line < 4; line++)
    {
        for (int c = 0; c < 4; c++)
        {
            int idx = (line * 4) + c;
            ImGui::TextColored(orange, "   %02d      ", idx);

            if (c < 3)
            {
                ImGui::SameLine();
                ImGui::SameLine();
            }
        }

        for (int c = 0; c < 4; c++)
        {
            u16 addr = (line * 4) + c + base;
            ImGui::TextColored(cyan, "%04X,%04X  ", addr , addr + 0x10);

            if (c < 3)
            {
                ImGui::SameLine();
                ImGui::SameLine();
            }
        }

        ImGui::Text("   "); ImGui::SameLine(0,0);

        for (int c = 0; c < 4; c++)
        {
            int idx = (line * 4) + c;
            u16 green = mikey_state->colors[idx].green;
            u16 bluered = mikey_state->colors[idx].bluered;
            u16 color = (green << 8) | bluered;
            ImVec4 float_color = color_444_to_float(color);

            if (c > 0)
            ImGui::Text("      "); ImGui::SameLine(0,0);

            char id[16];
            snprintf(id, sizeof(id), "##pal_%d_%d", line, c);

            ImGui::ColorEdit3(id, (float*)&float_color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoPicker);

            if (c < 3)
            {
                ImGui::SameLine(); ImGui::Dummy(ImVec2(8.0f, 0.0f));
                ImGui::SameLine();
            }
        }

        ImGui::Text("  "); ImGui::SameLine(0,0);

        for (int c = 0; c < 4; c++)
        {
            int idx = (line * 4) + c;

            u8 color_green = mikey_state->colors[idx].green & 0x0F;
            u8 color_blue = (mikey_state->colors[idx].bluered >> 4) & 0x0F;
            u8 color_red = mikey_state->colors[idx].bluered & 0x0F;

            ImGui::TextColored(green, " %01X", color_green); ImGui::SameLine(0,0);
            ImGui::TextColored(blue, "%01X", color_blue); ImGui::SameLine(0,0);
            ImGui::TextColored(red, "%01X       ", color_red);

            if (c < 3)
            {   
                ImGui::SameLine();
                ImGui::SameLine();
            }
        }

        if (line < 3)
        {
            ImGui::NewLine();
            ImGui::Separator();

        }

        //ImGui::Text("     "); ImGui::SameLine(0,0);

    }

    ImGui::PopFont();
    ImGui::End();
    ImGui::PopStyleVar();
}

static ImVec4 color_444_to_float(u16 color)
{
    ImVec4 ret;
    ret.w = 0;
    ret.x = (1.0f / 15.0f) * (color & 0xF);
    ret.z = (1.0f / 15.0f) * ((color >> 4) & 0xF);
    ret.y = (1.0f / 15.0f) * ((color >> 8) & 0xF);
    return ret;
}
