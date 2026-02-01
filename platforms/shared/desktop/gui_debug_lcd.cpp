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

#define GUI_DEBUG_LCD_IMPORT
#include "gui_debug_lcd.h"

#include "imgui.h"
#include "gearlynx.h"
#include "gui_debug_constants.h"
#include "gui_debug_widgets.h"
#include "gui.h"
#include "config.h"
#include "emu.h"
#include "utils.h"

void gui_debug_window_lcd(void)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(180, 120), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(220, 320), ImGuiCond_FirstUseEver);
    ImGui::Begin("LCD / Video DMA", &config_debug.show_lcd);

    ImGui::PushFont(gui_default_font);

    GearlynxCore* core = emu_get_core();
    Mikey* mikey = core->GetMikey();
    Mikey::Mikey_State* mikey_state = mikey->GetState();
    LcdScreen* lcd = mikey->GetLcdScreen();
    LcdScreen::LcdScreen_State* lcd_state = lcd->GetState();

    // Use timer2 counter/backup to compute line number
    u8 timer2_counter = mikey_state->timers[2].counter;
    u8 timer2_backup = mikey_state->timers[2].backup;

    // Current Line Status
    ImGui::TextColored(magenta, "LINE STATUS");
    ImGui::Separator();

    // Current Line = backup - counter (absolute position in frame)
    int current_line = timer2_backup - timer2_counter;
    ImGui::TextColored(violet, "CURRENT LINE   "); ImGui::SameLine();
    ImGui::Text("%03d ($%02X)", current_line, current_line);

    ImGui::TextColored(violet, "CURRENT CYCLE  "); ImGui::SameLine();
    ImGui::Text("%04d / %04d", lcd_state->current_cycle, lcd_state->line_cycles);

    // Line Type: VISIBLE N or VBLANK N
    ImGui::TextColored(violet, "LINE TYPE      "); ImGui::SameLine();
    if (current_line >= 3 && current_line <= (timer2_backup + 1))
    {
        // Visible lines 3-104 (VISIBLE 0-101)
        int visible_line = current_line - 3;
        ImGui::TextColored(green, "VISIBLE %d", visible_line);
    }
    else
    {
        // VBLANK lines 0-2
        ImGui::TextColored(red, "VBLANK %d", current_line);
    }

    ImGui::NewLine();

    // Pixel Processing
    ImGui::TextColored(magenta, "PIXEL TRANSFER");
    ImGui::Separator();

    u32 cycles_to_next_pixel = 0;
    bool pixel_active = false;
    if (!lcd_state->in_vblank && lcd_state->pixel_count < GLYNX_SCREEN_WIDTH)
    {
        pixel_active = true;
        if (lcd_state->pixel_next_at > lcd_state->current_cycle)
            cycles_to_next_pixel = lcd_state->pixel_next_at - lcd_state->current_cycle;
    }

    ImGui::TextColored(violet, "PIXEL COUNT    "); ImGui::SameLine();
    if (lcd_state->in_vblank)
        ImGui::TextColored(gray, "N/A (VBLANK)");
    else
        ImGui::Text("%03d / %03d", lcd_state->pixel_count, GLYNX_SCREEN_WIDTH);

    // Show current pen color being read
    if (!lcd_state->in_vblank && lcd_state->pixel_count < GLYNX_SCREEN_WIDTH)
    {
        u8 pen = lcd_state->dma_buffer[lcd_state->pixel_buffer_read_pos];
        u16 color = lcd_state->current_palette[pen];
        ImVec4 float_color = color_444_to_float(color);

        ImGui::TextColored(violet, "NEXT PIXEL     "); ImGui::SameLine();
        ImGui::Text("%02d -> $%03X", pen, color & 0x0FFF); ImGui::SameLine();
        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        float radius = ImGui::GetTextLineHeight() * 0.4f;
        ImVec2 center(pos.x + radius + 4.0f, pos.y + ImGui::GetTextLineHeight() * 0.5f);
        draw_list->AddCircleFilled(center, radius, ImGui::ColorConvertFloat4ToU32(float_color));
        ImGui::NewLine();
    }
    else
    {
        ImGui::TextColored(violet, "NEXT PIXEL     "); ImGui::SameLine();
        ImGui::TextColored(gray, lcd_state->in_vblank ? "N/A (VBLANK)" : "N/A");
    }

    ImGui::TextColored(violet, "NEXT PIXEL AT  "); ImGui::SameLine();
    if (lcd_state->in_vblank)
        ImGui::TextColored(gray, "N/A (VBLANK)");
    else
        ImGui::Text("%d", lcd_state->pixel_next_at);

    ImGui::TextColored(violet, "CYCLES TO PIXEL"); ImGui::SameLine();
    if (lcd_state->in_vblank)
        ImGui::TextColored(gray, "N/A (VBLANK)");
    else if (pixel_active)
        ImGui::TextColored(yellow, "%d", cycles_to_next_pixel);
    else
        ImGui::TextColored(gray, "N/A");

    ImGui::NewLine();


    ImGui::TextColored(magenta, "VIDEO DMA");
    ImGui::Separator();

    u32 cycles_to_next_dma = 0;
    bool dma_active = false;
    if (!lcd_state->in_vblank && lcd_state->dma_burst_count < k_dma_bursts_per_line)
    {
        dma_active = true;
        if (lcd_state->dma_next_at > lcd_state->current_cycle)
            cycles_to_next_dma = lcd_state->dma_next_at - lcd_state->current_cycle;
    }

    ImGui::TextColored(violet, "DMA COUNT      "); ImGui::SameLine();
    if (lcd_state->in_vblank)
        ImGui::TextColored(gray, "N/A (VBLANK)");
    else
        ImGui::Text("%02d / %02d", lcd_state->dma_burst_count, k_dma_bursts_per_line);

    ImGui::TextColored(violet, "NEXT DMA ADDR  "); ImGui::SameLine();
    if (lcd_state->in_vblank)
        ImGui::TextColored(gray, "N/A (VBLANK)");
    else
        ImGui::Text("$%04X", lcd_state->dma_current_src_addr);

    ImGui::TextColored(violet, "NEXT DMA AT    "); ImGui::SameLine();
    if (lcd_state->in_vblank)
        ImGui::TextColored(gray, "N/A (VBLANK)");
    else
        ImGui::Text("%d", lcd_state->dma_next_at);

    ImGui::TextColored(violet, "CYCLES TO DMA  "); ImGui::SameLine();
    if (lcd_state->in_vblank)
        ImGui::TextColored(gray, "N/A (VBLANK)");
    else if (dma_active)
        ImGui::TextColored(yellow, "%d", cycles_to_next_dma);
    else
        ImGui::TextColored(gray, "N/A");

    ImGui::PopFont();

    ImGui::End();
    ImGui::PopStyleVar();
}
