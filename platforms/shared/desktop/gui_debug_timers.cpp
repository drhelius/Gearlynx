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

#define GUI_DEBUG_TIMERS_IMPORT
#include "gui_debug_timers.h"

#include "imgui.h"
#include "gearlynx.h"
#include "gui.h"
#include "gui_debug_constants.h"
#include "config.h"
#include "emu.h"
#include "utils.h"

void gui_debug_timers_init(void)
{
}

void gui_debug_timers_destroy(void)
{
}

void gui_debug_window_timers(void)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(180, 45), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(444, 400), ImGuiCond_FirstUseEver);
    ImGui::Begin("Mikey Timers", &config_debug.show_mikey_timers);

    GearlynxCore* core = emu_get_core();
    Mikey::Mikey_State* mikey_state = core->GetMikey()->GetState();

    if (ImGui::BeginTabBar("##timer_tabs", ImGuiTabBarFlags_None))
    {
        static const int k_base_addr = 0xFD00;
        static const char* k_timer_names[8] = {
            "HBLANK TIMER", "TIMER 1", "VBLANK TIMER", "TIMER 3",
            "UART TIMER", "TIMER 5", "TIMER 6", "TIMER 7"
        };
        static const char* k_period_strs[8] = {
            "1 MHz (1us)", "512 KHz (2us)", "256 KHz (4us)", "128 KHz (8us)",
            "64 KHz (16us)", "32 KHz (32us)", "16 KHz (64us)", "N/A"
        };

        for (int t = 0; t < 8; t++)
        {
            GLYNX_Mikey_Timer* timer = &mikey_state->timers[t];
            u8 period = (timer->control_a & 0x07);
            bool is_linked = (period == 7) && (k_mikey_timer_backward_links[t] != -1);
            bool enabled = IS_SET_BIT(timer->control_a, 3);
            bool reload = IS_SET_BIT(timer->control_a, 4);
            bool interrupt = IS_SET_BIT(timer->control_a, 7);
            bool reset_timer_done = IS_SET_BIT(timer->control_a, 6);
            bool timer_done = IS_SET_BIT(timer->control_b, 3);
            bool borrow_in = IS_SET_BIT(timer->control_b, 1);
            bool borrow_out = IS_SET_BIT(timer->control_b, 0);

            char tab_name[32];
            snprintf(tab_name, 32, "%d", t);

            if (ImGui::BeginTabItem(tab_name))
            {
                ImGui::PushFont(gui_default_font);

                ImGui::TextColored(magenta, "%s", k_timer_names[t]);
                ImGui::Separator();

                ImGui::TextColored(cyan, "%04X ", k_base_addr + (t * 4) + 0); ImGui::SameLine();
                ImGui::TextColored(orange, "BACKUP    "); ImGui::SameLine();
                ImGui::Text("$%02X (" BYTE_TO_BINARY_PATTERN_SPACED ")", timer->backup, BYTE_TO_BINARY(timer->backup));

                ImGui::TextColored(cyan, "%04X ", k_base_addr + (t * 4) + 1); ImGui::SameLine();
                ImGui::TextColored(orange, "CONTROL A "); ImGui::SameLine();
                ImGui::Text("$%02X (" BYTE_TO_BINARY_PATTERN_SPACED ")", timer->control_a, BYTE_TO_BINARY(timer->control_a));

                ImGui::TextColored(cyan, "%04X ", k_base_addr + (t * 4) + 2); ImGui::SameLine();
                ImGui::TextColored(orange, "COUNTER   "); ImGui::SameLine();
                ImGui::Text("$%02X (" BYTE_TO_BINARY_PATTERN_SPACED ")", timer->counter, BYTE_TO_BINARY(timer->counter));

                ImGui::TextColored(cyan, "%04X ", k_base_addr + (t * 4) + 3); ImGui::SameLine();
                ImGui::TextColored(orange, "CONTROL B "); ImGui::SameLine();
                ImGui::Text("$%02X (" BYTE_TO_BINARY_PATTERN_SPACED ")", timer->control_b, BYTE_TO_BINARY(timer->control_b));

                ImGui::Separator();

                ImGui::TextColored(violet, "ENABLED    "); ImGui::SameLine();
                ImGui::TextColored(enabled ? green : gray, "%s", enabled ? "YES" : "NO");

                ImGui::TextColored(violet, "RELOAD     "); ImGui::SameLine();
                ImGui::TextColored(reload ? green : gray, "%s", reload ? "YES" : "NO");

                ImGui::TextColored(violet, "INTERRUPT  "); ImGui::SameLine();
                ImGui::TextColored(interrupt ? green : gray, "%s", interrupt ? "YES" : "NO");

                ImGui::TextColored(violet, "RESET DONE "); ImGui::SameLine();
                ImGui::TextColored(reset_timer_done ? green : gray, "%s", reset_timer_done ? "YES" : "NO");

                ImGui::TextColored(violet, "FREQUENCY  "); ImGui::SameLine();
                ImGui::TextColored(period != 7 ? white : gray, "%s", k_period_strs[period]);

                ImGui::TextColored(violet, "LINKED TO  "); ImGui::SameLine();
                if (is_linked)
                {
                    int link = k_mikey_timer_backward_links[t];
                    if (link < 8)
                        ImGui::TextColored(blue, "TIMER %d", link);
                    else
                        ImGui::TextColored(blue, "AUDIO CH %d", link - 8);
                }
                else
                    ImGui::TextColored(gray, "NONE");

                ImGui::TextColored(violet, "TIMER DONE "); ImGui::SameLine();
                ImGui::TextColored(timer_done ? green : gray, "%s", timer_done ? "YES" : "NO");

                ImGui::TextColored(violet, "BORROW IN  "); ImGui::SameLine();
                ImGui::TextColored(borrow_in ? green : gray, "%s", borrow_in ? "YES" : "NO");

                ImGui::TextColored(violet, "BORROW OUT "); ImGui::SameLine();
                ImGui::TextColored(borrow_out ? green : gray, "%s", borrow_out ? "YES" : "NO");

                ImGui::PopFont();
                ImGui::EndTabItem();
            }
        }

        ImGui::EndTabBar();
    }


    ImGui::End();
    ImGui::PopStyleVar();
}
