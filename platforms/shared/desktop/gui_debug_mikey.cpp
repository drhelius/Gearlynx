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
#include "gui_debug_widgets.h"
#include "gui.h"
#include "config.h"
#include "emu.h"
#include "utils.h"

static void MikeyWriteCallback8(u16 address, u8 value, void* user_data)
{
    Mikey* mikey = (Mikey*)user_data;
    mikey->Write<true>(address, value);
}

static void MikeyWriteCallback16(u16 address, u16 value, void* user_data)
{
    Mikey* mikey = (Mikey*)user_data;
    mikey->Write<true>(address, (u8)(value & 0xFF));
    mikey->Write<true>(address + 1, (u8)((value >> 8) & 0xFF));
}

void gui_debug_window_mikey_regs(void)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(210, 162), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(322, 344), ImGuiCond_FirstUseEver);

    ImGui::Begin("Mikey Registers", &config_debug.show_mikey_regs);

    ImGui::PushFont(gui_default_font);

    GearlynxCore* core = emu_get_core();
    Mikey* mikey = core->GetMikey();
    Mikey::Mikey_State* mikey_state = mikey->GetState();

    EditableRegister8("ATTEN_A ", "FD40", MIKEY_ATTEN_A, mikey_state->ATTEN_A, MikeyWriteCallback8, mikey);
    EditableRegister8("ATTEN_B ", "FD41", MIKEY_ATTEN_B, mikey_state->ATTEN_B, MikeyWriteCallback8, mikey);
    EditableRegister8("ATTEN_C ", "FD42", MIKEY_ATTEN_C, mikey_state->ATTEN_C, MikeyWriteCallback8, mikey);
    EditableRegister8("ATTEN_D ", "FD43", MIKEY_ATTEN_D, mikey_state->ATTEN_D, MikeyWriteCallback8, mikey);
    EditableRegister8("MPAN    ", "FD44", MIKEY_MPAN, mikey_state->MPAN, MikeyWriteCallback8, mikey);
    EditableRegister8("MSTEREO ", "FD50", MIKEY_MSTEREO, mikey_state->MSTEREO, MikeyWriteCallback8, mikey);
    EditableRegister8("INTRST  ", "FD80", MIKEY_INTRST, mikey_state->irq_pending, MikeyWriteCallback8, mikey);
    EditableRegister8("INTSET  ", "FD81", MIKEY_INTSET, mikey_state->irq_pending, MikeyWriteCallback8, mikey);
    EditableRegister8("SYSCTL1 ", "FD87", MIKEY_SYSCTL1, mikey_state->SYSCTL1, MikeyWriteCallback8, mikey);
    EditableRegister8("IODIR   ", "FD8A", MIKEY_IODIR, mikey_state->IODIR, MikeyWriteCallback8, mikey);

    u8 iodat = mikey->Read<true>(MIKEY_IODAT);
    EditableRegister8("IODAT   ", "FD8B", MIKEY_IODAT, iodat, MikeyWriteCallback8, mikey);

    EditableRegister8("SERCTL  ", "FD8C", MIKEY_SERCTL, mikey_state->SERCTL, MikeyWriteCallback8, mikey);
    EditableRegister8("SERDAT  ", "FD8D", MIKEY_SERDAT, mikey_state->SERDAT, MikeyWriteCallback8, mikey);
    EditableRegister8("SDONEACK", "FD90", MIKEY_SDONEACK, mikey_state->SDONEACK, MikeyWriteCallback8, mikey);
    EditableRegister8("CPUSLEEP", "FD91", MIKEY_CPUSLEEP, mikey_state->CPUSLEEP, MikeyWriteCallback8, mikey);
    EditableRegister8("DISPCTL ", "FD92", MIKEY_DISPCTL, mikey_state->DISPCTL, MikeyWriteCallback8, mikey);
    EditableRegister8("PBKUP   ", "FD93", MIKEY_PBKUP, mikey_state->PBKUP, MikeyWriteCallback8, mikey);

    EditableRegister16("DISPADR ", "FD94", MIKEY_DISPADRL, mikey_state->DISPADR.value, MikeyWriteCallback16, mikey);

    ImGui::PopFont();

    ImGui::End();
    ImGui::PopStyleVar();
}

static bool EditableColor12(int color_index, u8* out_green, u8* out_bluered)
{
    static ImGuiID editing_id = 0;
    static int frames_editing = 0;
    static char edit_buffer[8] = {0};
    bool modified = false;

    u8 g = *out_green & 0x0F;
    u8 b = (*out_bluered >> 4) & 0x0F;
    u8 r = *out_bluered & 0x0F;

    ImGui::PushID(color_index);
    ImGuiID widget_id = ImGui::GetID("##edit_color");

    float item_width = ImGui::CalcTextSize(" FFF ").x;

    if (editing_id == widget_id)
    {
        float text_height = ImGui::GetTextLineHeight();
        float frame_height = ImGui::GetFrameHeight();
        float padding_reduction = (frame_height - text_height) * 0.5f;
        ImVec2 original_padding = ImGui::GetStyle().FramePadding;
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(original_padding.x, original_padding.y - padding_reduction));

        ImVec2 cursor = ImGui::GetCursorPos();
        ImGui::SetCursorPos(ImVec2(cursor.x, cursor.y + padding_reduction));

        ImGui::PushItemWidth(item_width);
        ImGuiInputTextFlags flags = ImGuiInputTextFlags_CharsHexadecimal |
                                    ImGuiInputTextFlags_CharsUppercase |
                                    ImGuiInputTextFlags_EnterReturnsTrue |
                                    ImGuiInputTextFlags_AutoSelectAll;

        if (frames_editing == 0)
        {
            ImGui::SetKeyboardFocusHere();
        }
        bool enter_pressed = ImGui::InputText("##edit_color", edit_buffer, sizeof(edit_buffer), flags);
        bool lost_focus = (frames_editing > 1) && !ImGui::IsItemActive();
        frames_editing++;

        if (enter_pressed)
        {
            u16 gbr_value = 0;
            if (parse_hex_string(edit_buffer, strlen(edit_buffer), &gbr_value) && strlen(edit_buffer) == 3)
            {
                u8 new_g = (gbr_value >> 8) & 0x0F;
                u8 new_b = (gbr_value >> 4) & 0x0F;
                u8 new_r = gbr_value & 0x0F;
                *out_green = new_g;
                *out_bluered = (new_b << 4) | new_r;
                modified = true;
            }
            editing_id = 0;
        }

        if (ImGui::IsKeyPressed(ImGuiKey_Escape) || lost_focus)
        {
            editing_id = 0;
        }

        ImGui::PopItemWidth();
        ImGui::PopStyleVar();
    }
    else
    {
        ImGui::AlignTextToFramePadding();
        ImVec2 pos = ImGui::GetCursorScreenPos();
        float frame_height = ImGui::GetFrameHeight();

        bool clicked = ImGui::InvisibleButton("##color_btn", ImVec2(item_width, frame_height));
        bool hovered = ImGui::IsItemHovered();
        if (clicked)
        {
            editing_id = widget_id;
            frames_editing = 0;
            snprintf(edit_buffer, sizeof(edit_buffer), "%01X%01X%01X", g, b, r);
        }

        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        if (hovered)
        {
            ImVec2 p_min = pos;
            ImVec2 p_max = ImVec2(pos.x + item_width, pos.y + frame_height);
            draw_list->AddRectFilled(p_min, p_max, ImGui::GetColorU32(ImGuiCol_HeaderHovered));
        }

        char nibble[2] = {0, 0};
        float x = pos.x + ImGui::GetStyle().FramePadding.x;
        float y = pos.y + ImGui::GetStyle().FramePadding.y;
        static const char* const k_hex_digits = "0123456789ABCDEF";

        nibble[0] = k_hex_digits[g];
        draw_list->AddText(ImVec2(x, y), ImGui::ColorConvertFloat4ToU32(green), nibble);
        x += ImGui::CalcTextSize(nibble).x;

        nibble[0] = k_hex_digits[b];
        draw_list->AddText(ImVec2(x, y), ImGui::ColorConvertFloat4ToU32(blue), nibble);
        x += ImGui::CalcTextSize(nibble).x;

        nibble[0] = k_hex_digits[r];
        draw_list->AddText(ImVec2(x, y), ImGui::ColorConvertFloat4ToU32(red), nibble);
    }

    ImGui::PopID();
    return modified;
}

void gui_debug_window_mikey_colors(void)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(212, 128), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(334, 224), ImGuiCond_FirstUseEver);

    ImGui::Begin("Mikey Color Registers", &config_debug.show_mikey_colors);
    ImGui::PushFont(gui_default_font);

    GearlynxCore* core = emu_get_core();
    Mikey::Mikey_State* mikey_state = core->GetMikey()->GetState();

    const u16 base = 0xFDA0;

    ImGuiTableFlags table_flags = ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_BordersInnerV;

    if (ImGui::BeginTable("##colors_table", 2, table_flags))
    {
        for (int line = 0; line < 8; line++)
        {
            ImGui::TableNextRow();

            for (int c = 0; c < 2; c++)
            {
                ImGui::TableNextColumn();

                int idx = (line * 2) + c;
                u16 addr = idx + base;

                ImGui::AlignTextToFramePadding();
                ImGui::TextColored(orange, "%02d", idx);
                ImGui::SameLine();
                ImGui::TextColored(cyan, "%04X,%04X", addr, addr + 0x10);
                ImGui::SameLine();

                u8 g = mikey_state->colors[idx].green;
                u8 br = mikey_state->colors[idx].bluered;
                u16 color = (g << 8) | br;
                ImVec4 float_color = color_444_to_float(color);

                char id[16];
                snprintf(id, sizeof(id), "##pal_%d", idx);
                ImGui::ColorEdit3(id, (float*)&float_color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoPicker);

                ImGui::SameLine();
                EditableColor12(idx, &mikey_state->colors[idx].green, &mikey_state->colors[idx].bluered);
            }
        }

        ImGui::EndTable();
    }

    ImGui::PopFont();
    ImGui::End();
    ImGui::PopStyleVar();
}
