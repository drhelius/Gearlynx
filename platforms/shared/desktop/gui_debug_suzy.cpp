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
#include "ogl_renderer.h"
#include "utils.h"

// Convert collision palette to ImGui format
static ImVec4 collision_palette_imgui[16];
static bool collision_palette_initialized = false;
static bool collision_overlay_enabled = false;
static float collision_overlay_alpha = 0.75f;

static void init_collision_palette()
{
    if (collision_palette_initialized)
        return;

    for (int i = 0; i < 16; i++)
    {
        u32 color = emu_collision_palette[i];
        float r = ((color >> 0) & 0xFF) / 255.0f;
        float g = ((color >> 8) & 0xFF) / 255.0f;
        float b = ((color >> 16) & 0xFF) / 255.0f;
        float a = ((color >> 24) & 0xFF) / 255.0f;
        collision_palette_imgui[i] = ImVec4(r, g, b, a);
    }

    collision_palette_initialized = true;
}

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

    u8 mathd = suzy->Read<true>(SUZY_MATHD);
    u8 mathc = suzy->Read<true>(SUZY_MATHC);
    u8 mathb = suzy->Read<true>(SUZY_MATHB);
    u8 matha = suzy->Read<true>(SUZY_MATHA);
    u8 mathp = suzy->Read<true>(SUZY_MATHP);
    u8 mathn = suzy->Read<true>(SUZY_MATHN);
    u8 mathh = suzy->Read<true>(SUZY_MATHH);
    u8 mathg = suzy->Read<true>(SUZY_MATHG);
    u8 mathf = suzy->Read<true>(SUZY_MATHF);
    u8 mathe = suzy->Read<true>(SUZY_MATHE);
    u8 mathm = suzy->Read<true>(SUZY_MATHM);
    u8 mathl = suzy->Read<true>(SUZY_MATHL);
    u8 mathk = suzy->Read<true>(SUZY_MATHK);
    u8 mathj = suzy->Read<true>(SUZY_MATHJ);

    EditableRegister8("MATHD", "FC52", SUZY_MATHD, mathd, SuzyWriteCallback8, suzy);
    EditableRegister8("MATHC", "FC53", SUZY_MATHC, mathc, SuzyWriteCallback8, suzy);
    EditableRegister8("MATHB", "FC54", SUZY_MATHB, mathb, SuzyWriteCallback8, suzy);
    EditableRegister8("MATHA", "FC55", SUZY_MATHA, matha, SuzyWriteCallback8, suzy);
    EditableRegister8("MATHP", "FC56", SUZY_MATHP, mathp, SuzyWriteCallback8, suzy);
    EditableRegister8("MATHN", "FC57", SUZY_MATHN, mathn, SuzyWriteCallback8, suzy);
    EditableRegister8("MATHH", "FC60", SUZY_MATHH, mathh, SuzyWriteCallback8, suzy);
    EditableRegister8("MATHG", "FC61", SUZY_MATHG, mathg, SuzyWriteCallback8, suzy);
    EditableRegister8("MATHF", "FC62", SUZY_MATHF, mathf, SuzyWriteCallback8, suzy);
    EditableRegister8("MATHE", "FC63", SUZY_MATHE, mathe, SuzyWriteCallback8, suzy);
    EditableRegister8("MATHM", "FC6C", SUZY_MATHM, mathm, SuzyWriteCallback8, suzy);
    EditableRegister8("MATHL", "FC6D", SUZY_MATHL, mathl, SuzyWriteCallback8, suzy);
    EditableRegister8("MATHK", "FC6E", SUZY_MATHK, mathk, SuzyWriteCallback8, suzy);
    EditableRegister8("MATHJ", "FC6F", SUZY_MATHJ, mathj, SuzyWriteCallback8, suzy);

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

    GearlynxCore* core = emu_get_core();
    Suzy::Suzy_State* suzy_state = core->GetSuzy()->GetState();
    Mikey::Mikey_State* mikey_state = core->GetMikey()->GetState();

    const float scale = 3.0f;

    if (ImGui::BeginTabBar("##frame_tabs", ImGuiTabBarFlags_None))
    {
        if (ImGui::BeginTabItem("Suzy VIDBAS", NULL, ImGuiTabItemFlags_None))
        {
            ImGui::PushFont(gui_default_font);

            ImGui::TextColored(orange, "VIDBAS  "); ImGui::SameLine();
            ImGui::Text("$%04X ", suzy_state->VIDBAS.value); ImGui::SameLine(0, 0);
            ImGui::TextColored(gray, "(" BYTE_TO_BINARY_PATTERN_SPACED " " BYTE_TO_BINARY_PATTERN_SPACED ")", BYTE_TO_BINARY(suzy_state->VIDBAS.high), BYTE_TO_BINARY(suzy_state->VIDBAS.low));

            ImGui::Checkbox("Collision Overlay", &collision_overlay_enabled);
            if (collision_overlay_enabled)
            {
                ImGui::SameLine();
                ImGui::PushItemWidth(100.0f);
                ImGui::SliderFloat("##coll_alpha_vidbas", &collision_overlay_alpha, 0.0f, 1.0f, "Alpha %.2f");
                ImGui::PopItemWidth();
            }

            ImVec2 img_pos = ImGui::GetCursorScreenPos();
            ImGui::Image((ImTextureID)(intptr_t)ogl_renderer_emu_debug_framebuffer[0], ImVec2(GLYNX_SCREEN_WIDTH, GLYNX_SCREEN_HEIGHT) * scale, ImVec2(0, 0), ImVec2(GLYNX_SCREEN_WIDTH / 256.0f, GLYNX_SCREEN_HEIGHT / 128.0f));

            if (collision_overlay_enabled)
            {
                ImVec2 img_max = ImVec2(img_pos.x + GLYNX_SCREEN_WIDTH * scale, img_pos.y + GLYNX_SCREEN_HEIGHT * scale);
                ImU32 overlay_col = IM_COL32(255, 255, 255, (int)(collision_overlay_alpha * 255));
                ImGui::GetWindowDrawList()->AddImage((ImTextureID)(intptr_t)ogl_renderer_emu_debug_framebuffer[4], img_pos, img_max, ImVec2(0, 0), ImVec2(GLYNX_SCREEN_WIDTH / 256.0f, GLYNX_SCREEN_HEIGHT / 128.0f), overlay_col);
            }

            ImGui::PopFont();

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Mikey DISPADR", NULL, ImGuiTabItemFlags_None))
        {
            ImGui::PushFont(gui_default_font);

            ImGui::TextColored(orange, "DISPADR "); ImGui::SameLine();
            ImGui::Text("$%04X ", mikey_state->DISPADR.value); ImGui::SameLine(0, 0);
            ImGui::TextColored(gray, "(" BYTE_TO_BINARY_PATTERN_SPACED " " BYTE_TO_BINARY_PATTERN_SPACED ")", BYTE_TO_BINARY(mikey_state->DISPADR.high), BYTE_TO_BINARY(mikey_state->DISPADR.low));

            ImGui::Checkbox("Collision Overlay", &collision_overlay_enabled);
            if (collision_overlay_enabled)
            {
                ImGui::SameLine();
                ImGui::PushItemWidth(100.0f);
                ImGui::SliderFloat("##coll_alpha_dispadr", &collision_overlay_alpha, 0.0f, 1.0f, "Alpha %.2f");
                ImGui::PopItemWidth();
            }

            ImVec2 img_pos = ImGui::GetCursorScreenPos();
            ImGui::Image((ImTextureID)(intptr_t)ogl_renderer_emu_debug_framebuffer[1], ImVec2(GLYNX_SCREEN_WIDTH, GLYNX_SCREEN_HEIGHT) * scale, ImVec2(0, 0), ImVec2(GLYNX_SCREEN_WIDTH / 256.0f, GLYNX_SCREEN_HEIGHT / 128.0f));

            if (collision_overlay_enabled)
            {
                ImVec2 img_max = ImVec2(img_pos.x + GLYNX_SCREEN_WIDTH * scale, img_pos.y + GLYNX_SCREEN_HEIGHT * scale);
                ImU32 overlay_col = IM_COL32(255, 255, 255, (int)(collision_overlay_alpha * 255));
                ImGui::GetWindowDrawList()->AddImage((ImTextureID)(intptr_t)ogl_renderer_emu_debug_framebuffer[4], img_pos, img_max, ImVec2(0, 0), ImVec2(GLYNX_SCREEN_WIDTH / 256.0f, GLYNX_SCREEN_HEIGHT / 128.0f), overlay_col);
            }

            ImGui::PopFont();

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Suzy COLLBAS", NULL, ImGuiTabItemFlags_None))
        {
            ImGui::PushFont(gui_default_font);

            ImGui::TextColored(orange, "COLLBAS "); ImGui::SameLine();
            ImGui::Text("$%04X ", suzy_state->COLLBAS.value); ImGui::SameLine(0, 0);
            ImGui::TextColored(gray, "(" BYTE_TO_BINARY_PATTERN_SPACED " " BYTE_TO_BINARY_PATTERN_SPACED ")", BYTE_TO_BINARY(suzy_state->COLLBAS.high), BYTE_TO_BINARY(suzy_state->COLLBAS.low));

            init_collision_palette();

            const char* hex_labels[16] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "A", "B", "C", "D", "E", "F"};
            const float button_size = ImGui::GetFrameHeight() * 0.9f;

            for (int i = 0; i < 16; i++)
            {
                if (i > 0) ImGui::SameLine(0, 2.0f);
                ImVec2 pos = ImGui::GetCursorScreenPos();
                ImGui::ColorButton(hex_labels[i], collision_palette_imgui[i], ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoBorder, ImVec2(button_size, button_size));

                ImDrawList* draw_list = ImGui::GetWindowDrawList();
                ImVec2 text_size = ImGui::CalcTextSize(hex_labels[i]);
                ImVec2 text_pos = ImVec2(pos.x + (button_size - text_size.x) * 0.5f, pos.y + (button_size - text_size.y) * 0.5f);

                draw_list->AddText(ImVec2(text_pos.x + 1, text_pos.y + 1), IM_COL32(0, 0, 0, 200), hex_labels[i]);
                draw_list->AddText(text_pos, IM_COL32(255, 255, 255, 255), hex_labels[i]);
            }

            ImGui::Dummy(ImVec2(0, 0));

            ImGui::Image((ImTextureID)(intptr_t)ogl_renderer_emu_debug_framebuffer[2], ImVec2(GLYNX_SCREEN_WIDTH, GLYNX_SCREEN_HEIGHT) * scale, ImVec2(0, 0), ImVec2(GLYNX_SCREEN_WIDTH / 256.0f, GLYNX_SCREEN_HEIGHT / 128.0f));

            ImGui::PopFont();

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Custom", NULL, ImGuiTabItemFlags_None))
        {
            ImGui::PushFont(gui_default_font);

            ImGui::AlignTextToFramePadding();
            ImGui::TextColored(orange, "ADDRESS: ");
            ImGui::SameLine();

            u16 custom_addr = (u16)config_debug.frame_buffer_custom_address;
            u16 step = 1;
            u16 step_fast = 16;
            ImVec2 character_size = ImGui::CalcTextSize("X");
            ImGui::PushItemWidth((6.0f * character_size.x) + (2 * ImGui::GetFrameHeight()));
            if (ImGui::InputScalar("##custom_address", ImGuiDataType_U16, &custom_addr, &step, &step_fast, "%04X", ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase))
                config_debug.frame_buffer_custom_address = custom_addr;
            ImGui::PopItemWidth();

            ImGui::SameLine();
            ImGui::TextColored(gray, "($0000-$FFFF)");

            ImGui::NewLine();

            ImGui::Image((ImTextureID)(intptr_t)ogl_renderer_emu_debug_framebuffer[3], ImVec2(GLYNX_SCREEN_WIDTH, GLYNX_SCREEN_HEIGHT) * scale, ImVec2(0, 0), ImVec2(GLYNX_SCREEN_WIDTH / 256.0f, GLYNX_SCREEN_HEIGHT / 128.0f));

            ImGui::PopFont();

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
    ImGui::PopStyleVar();
}
