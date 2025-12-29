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

#define GUI_DEBUG_SUZY_IMPORT
#include "gui_debug_suzy.h"

#include "imgui.h"
#include "gearlynx.h"
#include "gui_debug_constants.h"
#include "gui_debug_memory.h"
#include "gui.h"
#include "config.h"
#include "emu.h"
#include "renderer.h"
#include "utils.h"

void gui_debug_window_suzy_regs(void)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(93, 79), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(284, 400), ImGuiCond_FirstUseEver);

    ImGui::Begin("Suzy Registers", &config_debug.show_suzy_regs);

    ImGui::PushFont(gui_default_font);

    GearlynxCore* core = emu_get_core();
    Suzy* suzy = core->GetSuzy();
    Suzy::Suzy_State* suzy_state = suzy->GetState();
    Input* input = core->GetInput();
    u8 joystick = input->ReadJoystick();
    u8 switches = input->ReadSwitches();
    u8 sprsys = suzy->Read<true>(SUZY_SPRSYS);

    struct
    {
        const char* name;
        const char* addr;
        u16_union* reg;
    } regs16[] = {
        {"TMPADR  ", "FC00", &suzy_state->TMPADR},
        {"TILTACUM", "FC02", &suzy_state->TILTACUM},
        {"HOFF    ", "FC04", &suzy_state->HOFF},
        {"VOFF    ", "FC06", &suzy_state->VOFF},
        {"VIDBAS  ", "FC08", &suzy_state->VIDBAS},
        {"COLLBAS ", "FC0A", &suzy_state->COLLBAS},
        {"VIDADR  ", "FC0C", &suzy_state->VIDADR},
        {"COLLADR ", "FC0E", &suzy_state->COLLADR},
        {"SCBNEXT ", "FC10", &suzy_state->SCBNEXT},
        {"SPRDLINE", "FC12", &suzy_state->SPRDLINE},
        {"HPOSSTRT", "FC14", &suzy_state->HPOSSTRT},
        {"VPOSSTRT", "FC16", &suzy_state->VPOSSTRT},
        {"SPRHSIZ ", "FC18", &suzy_state->SPRHSIZ},
        {"SPRVSIZ ", "FC1A", &suzy_state->SPRVSIZ},
        {"STRETCH ", "FC1C", &suzy_state->STRETCH},
        {"TILT    ", "FC1E", &suzy_state->TILT},
        {"SPRDOFF ", "FC20", &suzy_state->SPRDOFF},
        {"SPRVPOS ", "FC22", &suzy_state->SPRVPOS},
        {"COLLOFF ", "FC24", &suzy_state->COLLOFF},
        {"VSIZACUM", "FC26", &suzy_state->VSIZACUM},
        {"HSIZOFF ", "FC28", &suzy_state->HSIZOFF},
        {"VSIZOFF ", "FC2A", &suzy_state->VSIZOFF},
        {"SCBADR  ", "FC2C", &suzy_state->SCBADR},
        {"PROCADR ", "FC2E", &suzy_state->PROCADR},
        { 0, 0, 0 }
    };
    
    int i = 0;
    while (regs16[i].name != 0)
    {
        ImGui::TextColored(cyan, "%s ", regs16[i].addr); ImGui::SameLine();
        ImGui::TextColored(orange, "%s ", regs16[i].name); ImGui::SameLine();
        ImGui::Text("$%04X (" BYTE_TO_BINARY_PATTERN_SPACED " " BYTE_TO_BINARY_PATTERN_SPACED ")", regs16[i].reg->value, BYTE_TO_BINARY(regs16[i].reg->high), BYTE_TO_BINARY(regs16[i].reg->low));
        i++;
    }

    struct {
        const char* name;
        const char* addr;
        u8* reg;
    } regs8[] = {
        {"SPRCTL0  ", "FC80", &suzy_state->SPRCTL0},
        {"SPRCTL1  ", "FC81", &suzy_state->SPRCTL1},
        {"SPRCOLL  ", "FC82", &suzy_state->SPRCOLL},
        {"SPRINIT  ", "FC83", &suzy_state->SPRINIT},
        {"SUZYBUSEN", "FC90", &suzy_state->SUZYBUSEN},
        {"SPRGO    ", "FC91", &suzy_state->SPRGO},
        {"SPRSYS   ", "FC92", &sprsys},
        {"JOYSTICK ", "FCB0", &joystick},
        {"SWITCHES ", "FCB1", &switches},
        { 0, 0, 0 }
    };

    i = 0;
    while (regs8[i].name != 0)
    {
        ImGui::TextColored(cyan, "%s ", regs8[i].addr); ImGui::SameLine();
        ImGui::TextColored(orange, "%s  ", regs8[i].name); ImGui::SameLine();
        ImGui::Text("$%02X (" BYTE_TO_BINARY_PATTERN_SPACED ")", *regs8[i].reg, BYTE_TO_BINARY(*regs8[i].reg));
        i++;
    }

    ImGui::PopFont();

    ImGui::End();
    ImGui::PopStyleVar();
}

void gui_debug_window_suzy_math_regs(void)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(444, 218), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(216, 276), ImGuiCond_FirstUseEver);

    ImGui::Begin("Suzy Math Registers", &config_debug.show_suzy_math_regs);

    ImGui::PushFont(gui_default_font);

    GearlynxCore* core = emu_get_core();
    Suzy::Suzy_State* suzy_state = core->GetSuzy()->GetState();

    struct {
        const char* name;
        const char* addr;
        u8* reg;
    } regs8[] = {
        {"MATHD", "FC52", &suzy_state->MATHD},
        {"MATHC", "FC53", &suzy_state->MATHC},
        {"MATHB", "FC54", &suzy_state->MATHB},
        {"MATHA", "FC55", &suzy_state->MATHA},
        {"MATHP", "FC56", &suzy_state->MATHP},
        {"MATHN", "FC57", &suzy_state->MATHN},
        {"MATHH", "FC60", &suzy_state->MATHH},
        {"MATHG", "FC61", &suzy_state->MATHG},
        {"MATHF", "FC62", &suzy_state->MATHF},
        {"MATHE", "FC63", &suzy_state->MATHE},
        {"MATHM", "FC6C", &suzy_state->MATHM},
        {"MATHL", "FC6D", &suzy_state->MATHL},
        {"MATHK", "FC6E", &suzy_state->MATHK},
        {"MATHJ", "FC6F", &suzy_state->MATHJ},
        { 0, 0, 0 }
    };

    int i = 0;
    while (regs8[i].name != 0)
    {
        ImGui::TextColored(cyan, "%s ", regs8[i].addr); ImGui::SameLine();
        ImGui::TextColored(orange, "%s ", regs8[i].name); ImGui::SameLine();
        ImGui::Text("$%02X (" BYTE_TO_BINARY_PATTERN_SPACED ")", *regs8[i].reg, BYTE_TO_BINARY(*regs8[i].reg));
        i++;
    }

    ImGui::PopFont();

    ImGui::End();
    ImGui::PopStyleVar();
}

void gui_debug_window_frame_buffers(void)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(59, 70), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(498, 440), ImGuiCond_FirstUseEver);
    ImGui::Begin("Framebuffers", &config_debug.show_frame_buffers);

    ImGui::PushFont(gui_default_font);

    GearlynxCore* core = emu_get_core();
    Suzy::Suzy_State* suzy_state = core->GetSuzy()->GetState();
    Mikey::Mikey_State* mikey_state = core->GetMikey()->GetState();

    u32 scanline = mikey_state->render_line;
    ImGui::TextColored(orange, "SCAN LINE "); ImGui::SameLine();
    ImGui::TextColored(white, "$%02X (%03d)", scanline, scanline);

    ImGui::TextColored(orange, "VIDBAS    "); ImGui::SameLine();
    ImGui::Text("$%04X (" BYTE_TO_BINARY_PATTERN_SPACED " " BYTE_TO_BINARY_PATTERN_SPACED ")", suzy_state->VIDBAS.value, BYTE_TO_BINARY(suzy_state->VIDBAS.high), BYTE_TO_BINARY(suzy_state->VIDBAS.low));

    ImGui::TextColored(orange, "DISPADR   "); ImGui::SameLine();
    ImGui::Text("$%04X (" BYTE_TO_BINARY_PATTERN_SPACED " " BYTE_TO_BINARY_PATTERN_SPACED ")", mikey_state->DISPADR.value, BYTE_TO_BINARY(mikey_state->DISPADR.high), BYTE_TO_BINARY(mikey_state->DISPADR.low));

    ImGui::NewLine();

    ImGui::PopFont();

    const float scale = 3.0f;

    if (ImGui::BeginTabBar("##frame_tabs", ImGuiTabBarFlags_None))
    {
        if (ImGui::BeginTabItem("Suzy VIDBAS", NULL, ImGuiTabItemFlags_None))
        {
            ImGui::Image((ImTextureID)(intptr_t)renderer_emu_debug_framebuffer[0], ImVec2(GLYNX_SCREEN_WIDTH, GLYNX_SCREEN_HEIGHT) * scale, ImVec2(0, 0), ImVec2(GLYNX_SCREEN_WIDTH / 256.0f, GLYNX_SCREEN_HEIGHT / 128.0f));

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Mikey DISPADR", NULL, ImGuiTabItemFlags_None))
        {
            ImGui::Image((ImTextureID)(intptr_t)renderer_emu_debug_framebuffer[1], ImVec2(GLYNX_SCREEN_WIDTH, GLYNX_SCREEN_HEIGHT) * scale, ImVec2(0, 0), ImVec2(GLYNX_SCREEN_WIDTH / 256.0f, GLYNX_SCREEN_HEIGHT / 128.0f));

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
    ImGui::PopStyleVar();
}
