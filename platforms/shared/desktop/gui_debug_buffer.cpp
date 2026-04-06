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

#define GUI_DEBUG_BUFFER_IMPORT
#include "gui_debug_buffer.h"

#include "imgui.h"
#include "gearlynx.h"
#include "gui.h"
#include "gui_debug_constants.h"
#include "config.h"
#include "emu.h"

static bool auto_scroll = true;
static bool display_mode = false; // false=hex+ASCII, true=ASCII log
static bool line_wrap = false;

void gui_debug_buffer_init(void)
{
}

void gui_debug_buffer_destroy(void)
{
}

void gui_debug_buffer_window(void)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(200, 90), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(520, 400), ImGuiCond_FirstUseEver);
    ImGui::Begin("External Debug Buffer", &config_debug.show_debug_buffer);

    GearlynxCore* core = emu_get_core();
    Mikey* mikey = core->GetMikey();
    const std::vector<u8>& buffer = mikey->GetDebugBuffer();

    ImGui::PushFont(gui_default_font);

    // Header with size and controls
    ImGui::TextColored(violet, "SIZE   "); ImGui::SameLine();
    ImGui::Text("%zu bytes", buffer.size()); ImGui::SameLine();
    ImGui::TextColored(gray, " ($%04X)", (unsigned int)buffer.size());

    ImGui::TextColored(violet, "REGS   "); ImGui::SameLine();
    ImGui::TextColored(gray, "DBGOUT=$FDC0  DBGCTL=$FDC1");

    bool enabled = mikey->IsDebugBufferEnabled();
    if (ImGui::Checkbox("Enabled", &enabled))
    {
        mikey->SetDebugBufferEnabled(enabled);
        config_debug.debug_buffer_enabled = enabled;
    }
    ImGui::SameLine();
    ImGui::Checkbox("ASCII Log", &display_mode); ImGui::SameLine();
    if (display_mode)
    {
        ImGui::Checkbox("Wrap", &line_wrap); ImGui::SameLine();
    }
    ImGui::Checkbox("Auto-Scroll", &auto_scroll); ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 62);
    if (ImGui::SmallButton("Clear"))
    {
        mikey->ClearDebugBuffer();
    }

    ImGui::Separator();

    // Scrollable content area
    ImGui::BeginChild("BufferContent", ImVec2(0, 0), ImGuiChildFlags_None,
        (!display_mode || !line_wrap) ? ImGuiWindowFlags_HorizontalScrollbar : ImGuiWindowFlags_None);

    if (buffer.empty())
    {
        ImGui::TextColored(gray, "(empty - write to $FDC0 to add data)");
    }
    else if (!display_mode)
    {
        // Hex + ASCII dump view (16 bytes per row)
        const int bytes_per_row = 16;
        size_t total = buffer.size();

        for (size_t offset = 0; offset < total; offset += bytes_per_row)
        {
            // Address column
            ImGui::TextColored(cyan, "%04X: ", (unsigned int)offset);
            ImGui::SameLine(0, 0);

            // Hex bytes
            char hex_part[bytes_per_row * 3 + 2];
            char ascii_part[bytes_per_row + 1];
            int hex_pos = 0;
            int ascii_pos = 0;

            for (int i = 0; i < bytes_per_row; i++)
            {
                if (offset + i < total)
                {
                    u8 b = buffer[offset + i];
                    hex_pos += snprintf(hex_part + hex_pos, sizeof(hex_part) - hex_pos, "%02X ", b);
                    ascii_part[ascii_pos++] = (b >= 0x20 && b < 0x7F) ? (char)b : '.';
                }
                else
                {
                    hex_pos += snprintf(hex_part + hex_pos, sizeof(hex_part) - hex_pos, "   ");
                    ascii_part[ascii_pos++] = ' ';
                }

                if (i == 7)
                    hex_pos += snprintf(hex_part + hex_pos, sizeof(hex_part) - hex_pos, " ");
            }
            hex_part[hex_pos] = '\0';
            ascii_part[ascii_pos] = '\0';

            ImGui::Text("%s", hex_part);
            ImGui::SameLine(0, 0);
            ImGui::TextColored(yellow, " |%s|", ascii_part);
        }
    }
    else
    {
        // ASCII log view — renders like a terminal/console output
        // Splits on newlines, shows line numbers, non-printable chars as escape codes
        size_t line_start = 0;
        int line_num = 1;

        for (size_t i = 0; i <= buffer.size(); i++)
        {
            bool is_end = (i == buffer.size());
            bool is_newline = !is_end && (buffer[i] == '\n');
            bool is_cr = !is_end && (buffer[i] == '\r');

            if (is_newline || is_end || is_cr)
            {
                // Skip \n after \r (treat \r\n as single newline)
                if (is_cr && (i + 1 < buffer.size()) && buffer[i + 1] == '\n')
                    continue;

                ImGui::TextColored(gray, "%4d ", line_num);
                ImGui::SameLine(0, 0);
                ImGui::TextColored(cyan, "| ");
                ImGui::SameLine(0, 0);

                if (i > line_start)
                {
                    // Build line string, replacing non-printable with visible markers
                    std::string line;
                    line.reserve(i - line_start);
                    for (size_t j = line_start; j < i; j++)
                    {
                        u8 b = buffer[j];
                        if (b == '\t')
                            line += "    ";
                        else if (b >= 0x20 && b < 0x7F)
                            line += (char)b;
                        else if (b != '\r' && b != '\n')
                        {
                            char esc[5];
                            snprintf(esc, sizeof(esc), "\\x%02X", b);
                            line += esc;
                        }
                    }
                    if (line_wrap)
                        ImGui::TextWrapped("%s", line.c_str());
                    else
                        ImGui::TextUnformatted(line.c_str(), line.c_str() + line.size());
                }
                else
                {
                    ImGui::TextUnformatted("");
                }

                line_num++;
                line_start = i + 1;
            }
        }
    }

    if (auto_scroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 10.0f)
        ImGui::SetScrollHereY(1.0f);

    ImGui::EndChild();

    ImGui::PopFont();

    ImGui::End();
    ImGui::PopStyleVar();
}
