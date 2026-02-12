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

#define GUI_DEBUG_PSG_IMPORT
#include "gui_debug_psg.h"

#include "imgui.h"
#include "implot.h"
#include "fonts/IconsMaterialDesign.h"
#include "gearlynx.h"
#include "gui.h"
#include "gui_debug_constants.h"
#include "gui_debug_memeditor.h"
#include "gui_debug_widgets.h"
#include "config.h"
#include "emu.h"
#include "utils.h"

static bool exclusive_channel[4] = { false, false, false, false };
static float* wave_buffer_left = NULL;
static float* wave_buffer_right = NULL;

static void MikeyWriteCallback8(u16 address, u8 value, void* user_data)
{
    Mikey* mikey = (Mikey*)user_data;
    mikey->Write<true>(address, value);
}

void gui_debug_psg_init(void)
{
    wave_buffer_left = new float[GLYNX_AUDIO_BUFFER_SIZE];
    wave_buffer_right = new float[GLYNX_AUDIO_BUFFER_SIZE];
}

void gui_debug_psg_destroy(void)
{
    SafeDeleteArray(wave_buffer_left);
    SafeDeleteArray(wave_buffer_right);
}

void gui_debug_window_psg(void)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(180, 45), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(250, 486), ImGuiCond_FirstUseEver);
    ImGui::Begin("Mikey Audio", &config_debug.show_psg);

    GearlynxCore* core = emu_get_core();
    Mikey* mikey = core->GetMikey();
    Mikey::Mikey_State* mikey_state = mikey->GetState();

    if (ImGui::BeginTabBar("##audio_tabs", ImGuiTabBarFlags_None))
    {
        static const int k_base_addr = 0xFD20;
        static const char* k_period_strs[8] = {
            "1 MHz (1us)", "500 KHz (2us)", "250 KHz (4us)", "125 KHz (8us)",
            "62.5 KHz (16us)", "31.25 KHz (32us)", "15.625 KHz (64us)", "N/A"
        };

        for (int c = 0; c < 4; c++)
        {
            Audio* audio = core->GetAudio();
            Audio::GLYNX_Audio_Channel* psg_channels = audio->GetChannels();
            GLYNX_Mikey_Audio* channel = &mikey_state->audio[c];
            u8 period = (channel->control & 0x07);
            bool is_linked = (period == 7);
            bool enabled = IS_SET_BIT(channel->control, 3);
            bool reload = IS_SET_BIT(channel->control, 4);
            bool integrate = IS_SET_BIT(channel->control, 5);
            bool reset_timer_done = IS_SET_BIT(channel->control, 6);
            bool timer_done = IS_SET_BIT(channel->other, 3);
            bool borrow_in = IS_SET_BIT(channel->other, 1);
            bool borrow_out = IS_SET_BIT(channel->other, 0);
            u8 mstereo = mikey_state->MSTEREO;
            u8 mpan = mikey_state->MPAN;
            u8 ch_atten = (&mikey_state->ATTEN_A)[c];
            u32 frame_samples = audio->GetFrameSamples();

            char tab_name[32];
            snprintf(tab_name, 32, "%d", c);

            if (ImGui::BeginTabItem(tab_name))
            {
                ImGui::PushFont(gui_default_font);

                ImGui::BeginTable("##audio", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoPadOuterX);

                ImGui::TableNextColumn();

                ImGui::PushStyleColor(ImGuiCol_Text, psg_channels[c].mute ? mid_gray : white);
                ImGui::PushFont(gui_material_icons_font);

                char label[32];
                snprintf(label, 32, "%s##mute%d", psg_channels[c].mute ? ICON_MD_MUSIC_OFF : ICON_MD_MUSIC_NOTE, c);
                if (ImGui::Button(label))
                {
                    for (int i = 0; i < 4; i++)
                        exclusive_channel[i] = false;
                    psg_channels[c].mute = !psg_channels[c].mute;
                }
                if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                    ImGui::SetTooltip("Mute Channel");

                snprintf(label, 32, "%s##exc%d", ICON_MD_STAR, c);

                ImGui::PushStyleColor(ImGuiCol_Text, exclusive_channel[c] ? yellow : white);
                if (ImGui::Button(label))
                {
                    exclusive_channel[c] = !exclusive_channel[c];
                    psg_channels[c].mute = false;
                    for (int i = 0; i < 4; i++)
                    {
                        if (i != c)
                        {
                            exclusive_channel[i] = false;
                            psg_channels[i].mute = exclusive_channel[c] ? true : false;
                        }
                    }
                }
                ImGui::PopStyleColor();
                if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                {
                    ImGui::SetTooltip("Solo Channel");
                }
                ImGui::PopFont();
                ImGui::PopStyleColor();

                ImGui::TableNextColumn();

                ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(1, 1));

                int trigger_left = 0;
                int trigger_right = 0;
                int data_size = frame_samples / 2;

                for (int i = 0; i < data_size; i++)
                {
                    wave_buffer_left[i] = (float)(psg_channels[c].buffer[i * 2]) / 128.0f * 1.0f;
                    wave_buffer_right[i] = (float)(psg_channels[c].buffer[(i * 2) + 1]) / 128.0f * 1.0f;
                }

                for (int i = 100; i < data_size; ++i)
                {
                    if (wave_buffer_left[i - 1] < 0.0f && wave_buffer_left[i] >= 0.0f)
                    {
                        trigger_left = i;
                        break;
                    }
                }

                for (int i = 100; i < data_size; ++i)
                {
                    if (wave_buffer_right[i - 1] < 0.0f && wave_buffer_right[i] >= 0.0f)
                    {
                        trigger_right = i;
                        break;
                    }
                }

                int half_window_size = 100;
                int x_min_left = MAX(0, trigger_left - half_window_size);
                int x_max_left = MIN(data_size, trigger_left + half_window_size);

                ImPlotAxisFlags flags = ImPlotAxisFlags_NoGridLines | ImPlotAxisFlags_NoTickLabels | ImPlotAxisFlags_NoLabel | ImPlotAxisFlags_NoHighlight | ImPlotAxisFlags_Lock | ImPlotAxisFlags_NoTickMarks;

                if (ImPlot::BeginPlot("Left wave", ImVec2(90, 50), ImPlotFlags_CanvasOnly))
                {
                    ImPlot::SetupAxes("x", "y", flags, flags);
                    ImPlot::SetupAxesLimits(x_min_left, x_max_left, -1.0f, 1.0f, ImPlotCond_Always);
                    ImPlot::SetNextLineStyle(red, 1.0f);
                    ImPlot::PlotLine("Wave", wave_buffer_left, data_size);
                    ImPlot::EndPlot();
                }

                ImGui::SameLine();

                int x_min_right = MAX(0, trigger_right - half_window_size);
                int x_max_right = MIN(data_size, trigger_right + half_window_size);

                if (ImPlot::BeginPlot("Right wave", ImVec2(90, 50), ImPlotFlags_CanvasOnly))
                {
                    ImPlot::SetupAxes("x", "y", flags, flags);
                    ImPlot::SetupAxesLimits(x_min_right, x_max_right, -1.0f, 1.0f, ImPlotCond_Always);
                    ImPlot::SetNextLineStyle(red, 1.0f);
                    ImPlot::PlotLine("Wave", wave_buffer_right, data_size);
                    ImPlot::EndPlot();
                }

                ImGui::EndTable();

                ImGui::Separator();

                u16 base_addr = k_base_addr + (c * 8);
                char addr_str[8];

                snprintf(addr_str, sizeof(addr_str), "%04X", base_addr + 0);
                EditableRegister8("VOLUME    ", addr_str, base_addr + 0, channel->volume, MikeyWriteCallback8, mikey);

                snprintf(addr_str, sizeof(addr_str), "%04X", base_addr + 1);
                EditableRegister8("FEEDBACK  ", addr_str, base_addr + 1, channel->feedback, MikeyWriteCallback8, mikey);

                snprintf(addr_str, sizeof(addr_str), "%04X", base_addr + 2);
                EditableRegister8("OUTPUT    ", addr_str, base_addr + 2, (u8)channel->output, MikeyWriteCallback8, mikey);

                snprintf(addr_str, sizeof(addr_str), "%04X", base_addr + 3);
                EditableRegister8("LFSR LOW  ", addr_str, base_addr + 3, channel->lfsr_low, MikeyWriteCallback8, mikey);

                snprintf(addr_str, sizeof(addr_str), "%04X", base_addr + 4);
                EditableRegister8("BACKUP    ", addr_str, base_addr + 4, channel->backup, MikeyWriteCallback8, mikey);

                snprintf(addr_str, sizeof(addr_str), "%04X", base_addr + 5);
                EditableRegister8("CONTROL   ", addr_str, base_addr + 5, channel->control, MikeyWriteCallback8, mikey);

                snprintf(addr_str, sizeof(addr_str), "%04X", base_addr + 6);
                EditableRegister8("COUNTER   ", addr_str, base_addr + 6, channel->counter, MikeyWriteCallback8, mikey);

                snprintf(addr_str, sizeof(addr_str), "%04X", base_addr + 7);
                EditableRegister8("OTHER     ", addr_str, base_addr + 7, channel->other, MikeyWriteCallback8, mikey);

                snprintf(addr_str, sizeof(addr_str), "FD%02X", 0x40 + c);
                EditableRegister8("ATTEN     ", addr_str, MIKEY_ATTEN_A + c, ch_atten, MikeyWriteCallback8, mikey);

                ImGui::Separator();

                ImGui::TextColored(violet, "ENABLED    "); ImGui::SameLine();
                ImGui::TextColored(enabled ? green : gray, "%s", enabled ? "YES" : "NO");

                ImGui::TextColored(violet, "RELOAD     "); ImGui::SameLine();
                ImGui::TextColored(reload ? green : gray, "%s", reload ? "YES" : "NO");

                ImGui::TextColored(violet, "INTEGRATE  "); ImGui::SameLine();
                ImGui::TextColored(integrate ? green : gray, "%s", integrate ? "YES" : "NO");

                ImGui::TextColored(violet, "RESET DONE "); ImGui::SameLine();
                ImGui::TextColored(reset_timer_done ? green : gray, "%s", reset_timer_done ? "YES" : "NO");

                ImGui::TextColored(violet, "FREQUENCY  "); ImGui::SameLine();
                ImGui::TextColored(period != 7 ? white : gray, "%s", k_period_strs[period]);

                ImGui::TextColored(violet, "OUTPUT HZ  "); ImGui::SameLine();
                if (!is_linked)
                {
                    float freq_hz = 16000000.0f / (k_mikey_timer_period_cycles[period] * (channel->backup + 1));
                    if (freq_hz >= 1000.0f)
                        ImGui::TextColored(cyan, "%.2f KHz", freq_hz / 1000.0f);
                    else
                        ImGui::TextColored(cyan, "%.2f Hz", freq_hz);
                }
                else
                    ImGui::TextColored(gray, "N/A");

                ImGui::TextColored(violet, "LINKED TO  "); ImGui::SameLine();
                if (is_linked)
                {
                    int link = k_mikey_audio_backward_links[c];
                    if (link < 0)
                        ImGui::TextColored(blue, "TIMER 7");
                    else
                        ImGui::TextColored(blue, "AUDIO CH %d", link);
                }
                else
                    ImGui::TextColored(gray, "NONE");

                ImGui::TextColored(violet, "TIMER DONE "); ImGui::SameLine();
                ImGui::TextColored(timer_done ? green : gray, "%s", timer_done ? "YES" : "NO");

                ImGui::TextColored(violet, "BORROW IN  "); ImGui::SameLine();
                ImGui::TextColored(borrow_in ? green : gray, "%s", borrow_in ? "YES" : "NO");

                ImGui::TextColored(violet, "BORROW OUT "); ImGui::SameLine();
                ImGui::TextColored(borrow_out ? green : gray, "%s", borrow_out ? "YES" : "NO");

                bool left_enabled = IS_NOT_SET_BIT(mstereo, 4 + c);
                bool left_att_en = IS_SET_BIT(mpan, 4 + c);
                u8 left_vol = (ch_atten >> 4) & 0xF;

                ImGui::TextColored(violet, "LEFT EAR   "); ImGui::SameLine();
                if (left_enabled)
                {
                    if (left_att_en)
                        ImGui::TextColored(yellow, "%d/15", left_vol);
                    else
                        ImGui::TextColored(green, "FULL");
                }
                else
                    ImGui::TextColored(gray, "DISABLED");

                bool right_enabled = IS_NOT_SET_BIT(mstereo, c);
                bool right_att_en = IS_SET_BIT(mpan, c);
                u8 right_vol = ch_atten & 0xF;

                ImGui::TextColored(violet, "RIGHT EAR  "); ImGui::SameLine();
                if (right_enabled)
                {
                    if (right_att_en)
                        ImGui::TextColored(yellow, "%d/15", right_vol);
                    else
                        ImGui::TextColored(green, "FULL");
                }
                else
                    ImGui::TextColored(gray, "DISABLED");

                ImGui::PopFont();
                ImGui::EndTabItem();
            }
        }

        if (ImGui::BeginTabItem("Stereo"))
        {
            ImGui::PushFont(gui_default_font);

            EditableRegister8("MSTEREO ", "FD50", MIKEY_MSTEREO, mikey_state->MSTEREO, MikeyWriteCallback8, mikey);
            EditableRegister8("MPAN    ", "FD44", MIKEY_MPAN, mikey_state->MPAN, MikeyWriteCallback8, mikey);

            ImGui::Separator();

            for (int ch = 0; ch < 4; ch++)
            {
                bool left_enabled = IS_NOT_SET_BIT(mikey_state->MSTEREO, 4 + ch);
                bool right_enabled = IS_NOT_SET_BIT(mikey_state->MSTEREO, ch);

                ImGui::TextColored(violet, "CHANNEL %d:  ", ch); ImGui::SameLine();
                ImGui::TextColored(left_enabled ? green : gray, "L:%s", left_enabled ? "ON " : "OFF"); ImGui::SameLine();
                ImGui::TextColored(right_enabled ? green : gray, " R:%s", right_enabled ? "ON " : "OFF");
            }

            ImGui::PopFont();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
    ImGui::PopStyleVar();
}
