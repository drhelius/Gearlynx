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
#include "gui_debug_widgets.h"
#include "gui.h"
#include "config.h"
#include "emu.h"
#include "renderer.h"
#include "utils.h"

static void SuzyWriteCallback8(u16 address, u8 value, void* user_data)
{
    Suzy* suzy = (Suzy*)user_data;
    suzy->Write<true>(address, value);
}

static void SuzyWriteCallback16(u16 address, u16 value, void* user_data)
{
    Suzy* suzy = (Suzy*)user_data;
    suzy->Write<true>(address, (u8)(value & 0xFF));
    suzy->Write<true>(address + 1, (u8)((value >> 8) & 0xFF));
}

static void InputWriteCallback8(u16 address, u8 value, void* user_data)
{
    Input* input = (Input*)user_data;
    if (address == SUZY_JOYSTICK)
        input->WriteJoystick(value);
    else if (address == SUZY_SWITCHES)
        input->WriteSwitches(value);
}

void gui_debug_window_suzy_regs(void)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(93, 79), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(328, 400), ImGuiCond_FirstUseEver);

    ImGui::Begin("Suzy Registers", &config_debug.show_suzy_regs);

    ImGui::PushFont(gui_default_font);

    GearlynxCore* core = emu_get_core();
    Suzy* suzy = core->GetSuzy();
    Suzy::Suzy_State* suzy_state = suzy->GetState();
    Input* input = core->GetInput();
    u8 joystick = input->ReadJoystick();
    u8 switches = input->ReadSwitches();
    u8 sprsys = suzy->Read<true>(SUZY_SPRSYS);

    EditableRegister16("TMPADR  ", "FC00", SUZY_TMPADRL, suzy_state->TMPADR.value, SuzyWriteCallback16, suzy);
    EditableRegister16("TILTACUM", "FC02", SUZY_TILTACUML, suzy_state->TILTACUM.value, SuzyWriteCallback16, suzy);
    EditableRegister16("HOFF    ", "FC04", SUZY_HOFFL, suzy_state->HOFF.value, SuzyWriteCallback16, suzy);
    EditableRegister16("VOFF    ", "FC06", SUZY_VOFFL, suzy_state->VOFF.value, SuzyWriteCallback16, suzy);
    EditableRegister16("VIDBAS  ", "FC08", SUZY_VIDBASL, suzy_state->VIDBAS.value, SuzyWriteCallback16, suzy);
    EditableRegister16("COLLBAS ", "FC0A", SUZY_COLLBASL, suzy_state->COLLBAS.value, SuzyWriteCallback16, suzy);
    EditableRegister16("VIDADR  ", "FC0C", SUZY_VIDADRL, suzy_state->VIDADR.value, SuzyWriteCallback16, suzy);
    EditableRegister16("COLLADR ", "FC0E", SUZY_COLLADRL, suzy_state->COLLADR.value, SuzyWriteCallback16, suzy);
    EditableRegister16("SCBNEXT ", "FC10", SUZY_SCBNEXTL, suzy_state->SCBNEXT.value, SuzyWriteCallback16, suzy);
    EditableRegister16("SPRDLINE", "FC12", SUZY_SPRDLINEL, suzy_state->SPRDLINE.value, SuzyWriteCallback16, suzy);
    EditableRegister16("HPOSSTRT", "FC14", SUZY_HPOSSTRTL, suzy_state->HPOSSTRT.value, SuzyWriteCallback16, suzy);
    EditableRegister16("VPOSSTRT", "FC16", SUZY_VPOSSTRTL, suzy_state->VPOSSTRT.value, SuzyWriteCallback16, suzy);
    EditableRegister16("SPRHSIZ ", "FC18", SUZY_SPRHSIZL, suzy_state->SPRHSIZ.value, SuzyWriteCallback16, suzy);
    EditableRegister16("SPRVSIZ ", "FC1A", SUZY_SPRVSIZL, suzy_state->SPRVSIZ.value, SuzyWriteCallback16, suzy);
    EditableRegister16("STRETCH ", "FC1C", SUZY_STRETCHL, suzy_state->STRETCH.value, SuzyWriteCallback16, suzy);
    EditableRegister16("TILT    ", "FC1E", SUZY_TILTL, suzy_state->TILT.value, SuzyWriteCallback16, suzy);
    EditableRegister16("SPRDOFF ", "FC20", SUZY_SPRDOFFL, suzy_state->SPRDOFF.value, SuzyWriteCallback16, suzy);
    EditableRegister16("SPRVPOS ", "FC22", SUZY_SPRVPOSL, suzy_state->SPRVPOS.value, SuzyWriteCallback16, suzy);
    EditableRegister16("COLLOFF ", "FC24", SUZY_COLLOFFL, suzy_state->COLLOFF.value, SuzyWriteCallback16, suzy);
    EditableRegister16("VSIZACUM", "FC26", SUZY_VSIZACUML, suzy_state->VSIZACUM.value, SuzyWriteCallback16, suzy);
    EditableRegister16("HSIZOFF ", "FC28", SUZY_HSIZOFFL, suzy_state->HSIZOFF.value, SuzyWriteCallback16, suzy);
    EditableRegister16("VSIZOFF ", "FC2A", SUZY_VSIZOFFL, suzy_state->VSIZOFF.value, SuzyWriteCallback16, suzy);
    EditableRegister16("SCBADR  ", "FC2C", SUZY_SCBADRL, suzy_state->SCBADR.value, SuzyWriteCallback16, suzy);
    EditableRegister16("PROCADR ", "FC2E", SUZY_PROCADRL, suzy_state->PROCADR.value, SuzyWriteCallback16, suzy);

    EditableRegister8("SPRCTL0  ", "FC80", SUZY_SPRCTL0, suzy_state->SPRCTL0, SuzyWriteCallback8, suzy);
    EditableRegister8("SPRCTL1  ", "FC81", SUZY_SPRCTL1, suzy_state->SPRCTL1, SuzyWriteCallback8, suzy);
    EditableRegister8("SPRCOLL  ", "FC82", SUZY_SPRCOLL, suzy_state->SPRCOLL, SuzyWriteCallback8, suzy);
    EditableRegister8("SPRINIT  ", "FC83", SUZY_SPRINIT, suzy_state->SPRINIT, SuzyWriteCallback8, suzy);
    EditableRegister8("SUZYBUSEN", "FC90", SUZY_SUZYBUSEN, suzy_state->SUZYBUSEN, SuzyWriteCallback8, suzy);
    EditableRegister8("SPRGO    ", "FC91", SUZY_SPRGO, suzy_state->SPRGO, SuzyWriteCallback8, suzy);
    EditableRegister8("SPRSYS   ", "FC92", SUZY_SPRSYS, sprsys, SuzyWriteCallback8, suzy);
    EditableRegister8("JOYSTICK ", "FCB0", SUZY_JOYSTICK, joystick, InputWriteCallback8, input);
    EditableRegister8("SWITCHES ", "FCB1", SUZY_SWITCHES, switches, InputWriteCallback8, input);

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
    Suzy* suzy = core->GetSuzy();
    Suzy::Suzy_State* suzy_state = suzy->GetState();

    EditableRegister8("MATHD", "FC52", SUZY_MATHD, suzy_state->MATHD, SuzyWriteCallback8, suzy);
    EditableRegister8("MATHC", "FC53", SUZY_MATHC, suzy_state->MATHC, SuzyWriteCallback8, suzy);
    EditableRegister8("MATHB", "FC54", SUZY_MATHB, suzy_state->MATHB, SuzyWriteCallback8, suzy);
    EditableRegister8("MATHA", "FC55", SUZY_MATHA, suzy_state->MATHA, SuzyWriteCallback8, suzy);
    EditableRegister8("MATHP", "FC56", SUZY_MATHP, suzy_state->MATHP, SuzyWriteCallback8, suzy);
    EditableRegister8("MATHN", "FC57", SUZY_MATHN, suzy_state->MATHN, SuzyWriteCallback8, suzy);
    EditableRegister8("MATHH", "FC60", SUZY_MATHH, suzy_state->MATHH, SuzyWriteCallback8, suzy);
    EditableRegister8("MATHG", "FC61", SUZY_MATHG, suzy_state->MATHG, SuzyWriteCallback8, suzy);
    EditableRegister8("MATHF", "FC62", SUZY_MATHF, suzy_state->MATHF, SuzyWriteCallback8, suzy);
    EditableRegister8("MATHE", "FC63", SUZY_MATHE, suzy_state->MATHE, SuzyWriteCallback8, suzy);
    EditableRegister8("MATHM", "FC6C", SUZY_MATHM, suzy_state->MATHM, SuzyWriteCallback8, suzy);
    EditableRegister8("MATHL", "FC6D", SUZY_MATHL, suzy_state->MATHL, SuzyWriteCallback8, suzy);
    EditableRegister8("MATHK", "FC6E", SUZY_MATHK, suzy_state->MATHK, SuzyWriteCallback8, suzy);
    EditableRegister8("MATHJ", "FC6F", SUZY_MATHJ, suzy_state->MATHJ, SuzyWriteCallback8, suzy);

    ImGui::PopFont();

    ImGui::End();
    ImGui::PopStyleVar();
}

void gui_debug_window_frame_buffers(void)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(59, 70), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(498, 426), ImGuiCond_FirstUseEver);
    ImGui::Begin("Framebuffers", &config_debug.show_frame_buffers);

    ImGui::PushFont(gui_default_font);

    GearlynxCore* core = emu_get_core();
    Suzy::Suzy_State* suzy_state = core->GetSuzy()->GetState();
    Mikey::Mikey_State* mikey_state = core->GetMikey()->GetState();

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
