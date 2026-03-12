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


#define GUI_DEBUG_SCB_VIEWER_IMPORT
#include "gui_debug_scb_viewer.h"

#include "imgui.h"
#include "gearlynx.h"
#include "gui_debug_constants.h"
#include "gui_filedialogs.h"
#include "gui.h"
#include "config.h"
#include "emu.h"
#include "ogl_renderer.h"

static void draw_context_menu_sprite(int index);

static const char* k_sprite_type_names[] =
{
    "BG",
    "BG NOCOL",
    "BND SHADOW",
    "BOUND",
    "NORMAL",
    "NO COL",
    "XOR",
    "SHADOW"
};

void gui_debug_window_scb_viewer(void)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(78, 56), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(586, 506), ImGuiCond_FirstUseEver);

    ImGui::Begin("SCB Viewer", &config_debug.show_scb_viewer);

    ImGui::PushFont(gui_default_font);

    GearlynxCore* core = emu_get_core();
    Suzy::Suzy_State* suzy_state = core->GetSuzy()->GetState();

    int count = emu_debug_scb_count;

    static int selected_sprite = -1;

    ImGui::PushItemWidth(100.0f);
    if (ImGui::Combo("##scb_mode", &config_debug.scb_viewer_mode, "Real Time\0Accumulated\0\0"))
        selected_sprite = -1;
    ImGui::PopItemWidth();
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::TextColored(cyan, "Real Time:");
        ImGui::Text("Walks the SCB chain from RAM at the current\npoint in time. Best when paused mid-frame.");
        ImGui::NewLine();
        ImGui::TextColored(cyan, "Accumulated:");
        ImGui::Text("Shows all sprites rendered by Suzy during the\nlast complete frame. Captures every SPRGO call.");
        ImGui::EndTooltip();
    }
    ImGui::SameLine();

    if (config_debug.scb_viewer_mode == 0)
    {
        ImGui::Checkbox("Auto", &config_debug.scb_viewer_auto);
        ImGui::SameLine();

        if (config_debug.scb_viewer_auto)
        {
            u16 auto_addr;
            if ((suzy_state->SCBNEXT.value & 0xFF00) != 0)
                auto_addr = suzy_state->SCBNEXT.value;
            else
                auto_addr = suzy_state->SCBADR.value;
            ImGui::TextColored(orange, " START");
            ImGui::SameLine();
            ImGui::TextColored(white, "$%04X", auto_addr);
        }
        else
        {
            ImGui::AlignTextToFramePadding();
            ImGui::TextColored(orange, " START");
            ImGui::SameLine();
            u16 addr = (u16)config_debug.scb_viewer_address;
            u16 step = 1;
            u16 step_fast = 16;
            ImVec2 character_size = ImGui::CalcTextSize("X");
            ImGui::PushItemWidth((6.0f * character_size.x) + (2 * ImGui::GetFrameHeight()));
            if (ImGui::InputScalar("##scb_addr", ImGuiDataType_U16, &addr, &step, &step_fast, "%04X", ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase))
                config_debug.scb_viewer_address = addr;
            ImGui::PopItemWidth();
        }
    }

    ImGui::SameLine();
    ImGui::TextColored(orange, " SCBADR"); ImGui::SameLine();
    ImGui::TextColored(white, "$%04X", suzy_state->SCBADR.value); ImGui::SameLine();
    ImGui::TextColored(orange, " SCBNEXT"); ImGui::SameLine();
    ImGui::TextColored(white, "$%04X", suzy_state->SCBNEXT.value);
    ImGui::SameLine();
    ImGui::TextColored(orange, " COUNT"); ImGui::SameLine();
    ImGui::TextColored(white, "%d", count);

    if (count == 0)
    {
        ImGui::TextColored(gray, "No SCB chain found at this address.");
        ImGui::PopFont();
        ImGui::End();
        ImGui::PopStyleVar();
        return;
    }

    ImGui::Separator();

    ImGui::Columns(2, "scb", false);
    ImGui::SetColumnOffset(1, 200.0f);

    ImGui::BeginChild("scb_list", ImVec2(0, 0.0f), ImGuiChildFlags_Borders);

    if (selected_sprite >= count || count == 0)
        selected_sprite = -1;
    if (!emu_is_paused() && !emu_is_debug_idle())
        selected_sprite = -1;

    int hovered_sprite = -1;

    for (int s = 0; s < count && s < DEBUG_MAX_SPRITES; s++)
    {
        GLYNX_Debug_SCB_Info& entry = emu_debug_scb_info[s];

        int w = emu_debug_sprite_widths[s];
        int h = emu_debug_sprite_heights[s];

        if (entry.skipped)
        {
            ImGui::PushID(s);
            if (ImGui::Selectable("##skip", selected_sprite == s, 0, ImVec2(0, ImGui::GetTextLineHeight())))
                selected_sprite = (selected_sprite == s) ? -1 : s;
            if (ImGui::IsItemHovered())
                hovered_sprite = s;
            ImGui::SameLine();
            ImGui::TextColored(gray, "#%02d $%04X (SKIP)", s, entry.scb_address);
            ImGui::PopID();
            continue;
        }

        if (w <= 0 || h <= 0)
        {
            ImGui::PushID(s);
            if (ImGui::Selectable("##empty", selected_sprite == s, 0, ImVec2(0, ImGui::GetTextLineHeight())))
                selected_sprite = (selected_sprite == s) ? -1 : s;
            if (ImGui::IsItemHovered())
                hovered_sprite = s;
            ImGui::SameLine();
            ImGui::TextColored(gray, "#%02d $%04X (EMPTY)", s, entry.scb_address);
            ImGui::PopID();
            continue;
        }

        float scrollbar_w = ImGui::GetStyle().ScrollbarSize;
        float avail_w = MAX(ImGui::GetContentRegionAvail().x - scrollbar_w - 8.0f, 16.0f);

        int int_scale = MAX((int)(avail_w / (float)w), 1);

        float fw = (float)(w * int_scale);
        float fh = (float)(h * int_scale);
        float tex_u = (float)w / 256.0f;
        float tex_v = (float)h / 256.0f;

        ImVec2 p = ImGui::GetCursorScreenPos();

        ImGui::Image((ImTextureID)(intptr_t)ogl_renderer_emu_debug_sprites[s],
                     ImVec2(fw, fh), ImVec2(0.0f, 0.0f), ImVec2(tex_u, tex_v));

        draw_context_menu_sprite(s);

        if (ImGui::IsItemClicked())
            selected_sprite = (selected_sprite == s) ? -1 : s;
        if (ImGui::IsItemHovered())
            hovered_sprite = s;

        int highlight = (hovered_sprite == s) ? s : selected_sprite;
        if (highlight == s)
        {
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            draw_list->AddRect(ImVec2(p.x, p.y), ImVec2(p.x + fw, p.y + fh),
                             ImColor(cyan), 2.0f, ImDrawFlags_RoundCornersAll, 3.0f);
        }
    }

    ImGui::EndChild();

    ImGui::NextColumn();

    int display_sprite = (hovered_sprite >= 0) ? hovered_sprite : selected_sprite;

    if (display_sprite >= 0 && display_sprite < count && display_sprite < DEBUG_MAX_SPRITES)
    {
        GLYNX_Debug_SCB_Info& entry = emu_debug_scb_info[display_sprite];
        int w = emu_debug_sprite_widths[display_sprite];
        int h = emu_debug_sprite_heights[display_sprite];

        if (!entry.skipped && w > 0 && h > 0)
        {
            float preview_scale = 3.0f;
            float max_side = (float)MAX(w, h);
            if (max_side * preview_scale > 300.0f)
                preview_scale = 300.0f / max_side;
            preview_scale = MAX(preview_scale, 1.0f);

            float fw = (float)w * preview_scale;
            float fh = (float)h * preview_scale;
            float tex_u = (float)w / 256.0f;
            float tex_v = (float)h / 256.0f;

            ImGui::Image((ImTextureID)(intptr_t)ogl_renderer_emu_debug_sprites[display_sprite],
                         ImVec2(fw, fh), ImVec2(0.0f, 0.0f), ImVec2(tex_u, tex_v));

            ImGui::NewLine();
        }

        ImGui::TextColored(cyan, "DETAILS:");
        ImGui::Separator();

        if (entry.skipped)
        {
            ImGui::TextColored(violet, " SCB #:    "); ImGui::SameLine(); ImGui::Text("%d", display_sprite);
            ImGui::TextColored(violet, " SCB ADDR: "); ImGui::SameLine(); ImGui::Text("$%04X", entry.scb_address);
            ImGui::TextColored(violet, " SCB NEXT: "); ImGui::SameLine(); ImGui::Text("$%04X", entry.scb_next);
            ImGui::TextColored(red, " SKIPPED");
        }
        else
        {
            int w_s = emu_debug_sprite_widths[display_sprite];
            int h_s = emu_debug_sprite_heights[display_sprite];
            u8 coll_id = entry.sprcoll & 0x0F;
            bool coll_dis = IS_SET_BIT(entry.sprcoll, 5);

            if (ImGui::BeginTable("##scb_det", 2, ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_BordersInnerV))
            {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextColored(violet, " SCB #:   "); ImGui::SameLine(); ImGui::Text("%d", display_sprite);
                ImGui::TableNextColumn();
                ImGui::TextColored(violet, " SPRDLINE:"); ImGui::SameLine(); ImGui::Text("$%04X", entry.sprdline);

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextColored(violet, " SCB ADDR:"); ImGui::SameLine(); ImGui::Text("$%04X", entry.scb_address);
                ImGui::TableNextColumn();
                ImGui::TextColored(violet, " SPRHSIZ: "); ImGui::SameLine(); ImGui::Text("$%04X", entry.sprhsiz);

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextColored(violet, " SCB NEXT:"); ImGui::SameLine(); ImGui::Text("$%04X", entry.scb_next);
                ImGui::TableNextColumn();
                ImGui::TextColored(violet, " SPRVSIZ: "); ImGui::SameLine(); ImGui::Text("$%04X", entry.sprvsiz);

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextColored(violet, " SPRCTL0: "); ImGui::SameLine(); ImGui::Text("$%02X", entry.sprctl0);
                ImGui::TableNextColumn();
                ImGui::TextColored(violet, " STRETCH: "); ImGui::SameLine(); ImGui::Text("$%04X", entry.stretch);

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextColored(violet, " SPRCTL1: "); ImGui::SameLine(); ImGui::Text("$%02X", entry.sprctl1);
                ImGui::TableNextColumn();
                ImGui::TextColored(violet, " TILT:    "); ImGui::SameLine(); ImGui::Text("$%04X", entry.tilt);

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextColored(violet, " SPRCOLL: "); ImGui::SameLine(); ImGui::Text("$%02X", entry.sprcoll);
                ImGui::TableNextColumn();
                ImGui::TextColored(violet, " H FLIP:  "); ImGui::SameLine(); ImGui::TextColored(entry.h_flip ? green : gray, "%s", entry.h_flip ? "YES" : "NO");

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextColored(violet, " POS X:   "); ImGui::SameLine(); ImGui::Text("%d", entry.hpos);
                ImGui::TableNextColumn();
                ImGui::TextColored(violet, " V FLIP:  "); ImGui::SameLine(); ImGui::TextColored(entry.v_flip ? green : gray, "%s", entry.v_flip ? "YES" : "NO");

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextColored(violet, " POS Y:   "); ImGui::SameLine(); ImGui::Text("%d", entry.vpos);
                ImGui::TableNextColumn();
                ImGui::TextColored(violet, " LITERAL: "); ImGui::SameLine(); ImGui::TextColored(entry.literal_only ? green : gray, "%s", entry.literal_only ? "YES" : "NO");

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextColored(violet, " SIZE:    "); ImGui::SameLine(); ImGui::Text("%dx%d", w_s, h_s);
                ImGui::TableNextColumn();
                ImGui::TextColored(violet, " RDEPTH:  "); ImGui::SameLine(); ImGui::Text("%d", entry.reload_depth);

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextColored(violet, " BPP:     "); ImGui::SameLine(); ImGui::Text("%d", entry.bpp);
                ImGui::TableNextColumn();
                ImGui::TextColored(violet, " RPALETTE:"); ImGui::SameLine(); ImGui::TextColored(entry.reload_palette ? green : gray, "%s", entry.reload_palette ? "YES" : "NO");

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextColored(violet, " TYPE:    "); ImGui::SameLine(); ImGui::Text("%d", entry.type);
                ImGui::TableNextColumn();
                ImGui::TextColored(violet, " COLL ID: "); ImGui::SameLine(); ImGui::Text("%d", coll_id);

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextColored(violet, " TYPE:    "); ImGui::SameLine(); ImGui::TextColored(yellow, "%s", k_sprite_type_names[entry.type & 0x07]);
                ImGui::TableNextColumn();
                ImGui::TextColored(violet, " COLL DIS:"); ImGui::SameLine(); ImGui::TextColored(coll_dis ? red : gray, "%s", coll_dis ? "YES" : "NO");

                ImGui::EndTable();
            }
        }
    }
    else
    {
        ImGui::TextColored(gray, "Click a sprite to see details.");
    }

    ImGui::Columns(1);

    ImGui::PopFont();

    ImGui::End();
    ImGui::PopStyleVar();
}

static void draw_context_menu_sprite(int index)
{
    ImGui::PopFont();

    char ctx_id[16];
    snprintf(ctx_id, sizeof(ctx_id), "##spr_ctx_%02d", index);

    if (ImGui::BeginPopupContextItem(ctx_id))
    {
        if (ImGui::Selectable("Save Sprite As..."))
            gui_file_dialog_save_sprite(index);
        if (ImGui::Selectable("Save All Sprites To Folder..."))
            gui_file_dialog_save_all_sprites();

        ImGui::EndPopup();
    }

    ImGui::PushFont(gui_default_font);
}
