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

void gui_debug_window_mikey_regs(void)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(93, 79), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(284, 400), ImGuiCond_FirstUseEver);

    ImGui::Begin("Mikey Registers", &config_debug.show_mikey_regs);

    ImGui::PushFont(gui_default_font);

    GearlynxCore* core = emu_get_core();
    Mikey::Mikey_State* mikey_state = core->GetMikey()->GetState();

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
        {"IODAT   ", "FD8B", &mikey_state->IODAT},
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
        ImGui::TextColored(violet, "%s ", regs8[i].name); ImGui::SameLine();
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
        ImGui::TextColored(violet, "%s ", regs16[i].name); ImGui::SameLine();
        ImGui::Text("$%04X (" BYTE_TO_BINARY_PATTERN_SPACED " " BYTE_TO_BINARY_PATTERN_SPACED ")", regs16[i].reg->value, BYTE_TO_BINARY(regs16[i].reg->high), BYTE_TO_BINARY(regs16[i].reg->low));
        i++;
    }

    ImGui::PopFont();

    ImGui::End();
    ImGui::PopStyleVar();
}

void gui_debug_window_mikey_timers(void)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(400, 79), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(284, 400), ImGuiCond_FirstUseEver);

    ImGui::Begin("Mikey Timer Registers", &config_debug.show_mikey_timers);
    ImGui::PushFont(gui_default_font);

    GearlynxCore* core = emu_get_core();
    Mikey::Mikey_State* mikey_state = core->GetMikey()->GetState();

    struct {
        const char* name;
        const char* addr;
        u16* reg;
    } timer_regs16[] = {
        {"TIM0BKUP", "FD00", &mikey_state->timers[0].backup},
        {"TIM1BKUP", "FD04", &mikey_state->timers[1].backup},
        {"TIM2BKUP", "FD08", &mikey_state->timers[2].backup},
        {"TIM3BKUP", "FD0C", &mikey_state->timers[3].backup},
        {"TIM4BKUP", "FD10", &mikey_state->timers[4].backup},
        {"TIM5BKUP", "FD14", &mikey_state->timers[5].backup},
        {"TIM6BKUP", "FD18", &mikey_state->timers[6].backup},
        {"TIM7BKUP", "FD1C", &mikey_state->timers[7].backup},
        {0, 0, 0}
    };

    struct {
        const char* name;
        const char* addr;
        u8* reg;
    } timer_regs8[] = {
        {"TIM0CTLA", "FD01", &mikey_state->timers[0].control_a},
        {"TIM0CNT ", "FD02", &mikey_state->timers[0].count},
        {"TIM0CTLB", "FD03", &mikey_state->timers[0].control_b},
        {"TIM1CTLA", "FD05", &mikey_state->timers[1].control_a},
        {"TIM1CNT ", "FD06", &mikey_state->timers[1].count},
        {"TIM1CTLB", "FD07", &mikey_state->timers[1].control_b},
        {"TIM2CTLA", "FD09", &mikey_state->timers[2].control_a},
        {"TIM2CNT ", "FD0A", &mikey_state->timers[2].count},
        {"TIM2CTLB", "FD0B", &mikey_state->timers[2].control_b},
        {"TIM3CTLA", "FD0D", &mikey_state->timers[3].control_a},
        {"TIM3CNT ", "FD0E", &mikey_state->timers[3].count},
        {"TIM3CTLB", "FD0F", &mikey_state->timers[3].control_b},
        {"TIM4CTLA", "FD11", &mikey_state->timers[4].control_a},
        {"TIM4CNT ", "FD12", &mikey_state->timers[4].count},
        {"TIM4CTLB", "FD13", &mikey_state->timers[4].control_b},
        {"TIM5CTLA", "FD15", &mikey_state->timers[5].control_a},
        {"TIM5CNT ", "FD16", &mikey_state->timers[5].count},
        {"TIM5CTLB", "FD17", &mikey_state->timers[5].control_b},
        {"TIM6CTLA", "FD19", &mikey_state->timers[6].control_a},
        {"TIM6CNT ", "FD1A", &mikey_state->timers[6].count},
        {"TIM6CTLB", "FD1B", &mikey_state->timers[6].control_b},
        {"TIM7CTLA", "FD1D", &mikey_state->timers[7].control_a},
        {"TIM7CNT ", "FD1E", &mikey_state->timers[7].count},
        {"TIM7CTLB", "FD1F", &mikey_state->timers[7].control_b},
        {0, 0, 0}
    };


    int i = 0;
    while (timer_regs16[i].name != 0)
    {
        if (i > 0)
            ImGui::NewLine();
        ImGui::TextColored(cyan, "TIMER %d", i); ImGui::Separator();

        ImGui::TextColored(cyan, "%s ", timer_regs16[i].addr); ImGui::SameLine();
        ImGui::TextColored(violet, "%s ", timer_regs16[i].name); ImGui::SameLine();
        ImGui::Text("$%04X (" BYTE_TO_BINARY_PATTERN_SPACED " " BYTE_TO_BINARY_PATTERN_SPACED ")", *timer_regs16[i].reg, BYTE_TO_BINARY(*timer_regs16[i].reg >> 8), BYTE_TO_BINARY(*timer_regs16[i].reg & 0xFF));

        for (int j = 0; j < 3; j++)
        {
            int idx = (i * 3) + j;
            ImGui::TextColored(cyan, "%s ", timer_regs8[idx].addr); ImGui::SameLine();
            ImGui::TextColored(violet, "%s   ", timer_regs8[idx].name); ImGui::SameLine();
            ImGui::Text("$%02X (" BYTE_TO_BINARY_PATTERN_SPACED ")", *timer_regs8[idx].reg, BYTE_TO_BINARY(*timer_regs8[idx].reg));
        }

        i++;
    }

    ImGui::PopFont();
    ImGui::End();
    ImGui::PopStyleVar();
}

void gui_debug_window_mikey_audio(void)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(700, 79), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(284, 400), ImGuiCond_FirstUseEver);

    ImGui::Begin("Mikey Audio Registers", &config_debug.show_mikey_audio);
    ImGui::PushFont(gui_default_font);

    GearlynxCore* core = emu_get_core();
    Mikey::Mikey_State* mikey_state = core->GetMikey()->GetState();

    struct {
        const char* name;
        const char* addr;
        u8* reg;
    } audio_regs[] = {
        {"AUD0VOL   ", "FD20", &mikey_state->audio[0].volume},
        {"AUD0SHFTFB", "FD21", &mikey_state->audio[0].shift_feedback},
        {"AUD0OUTVAL", "FD22", &mikey_state->audio[0].output_value},
        {"AUD0L8SHFT", "FD23", &mikey_state->audio[0].left_shift},
        {"AUD0TBACK ", "FD24", &mikey_state->audio[0].timer_backup},
        {"AUD0CTL   ", "FD25", &mikey_state->audio[0].control},
        {"AUD0COUNT ", "FD26", &mikey_state->audio[0].count},
        {"AUD0MISC  ", "FD27", &mikey_state->audio[0].misc},
        {"AUD1VOL   ", "FD28", &mikey_state->audio[1].volume},
        {"AUD1SHFTFB", "FD29", &mikey_state->audio[1].shift_feedback},
        {"AUD1OUTVAL", "FD2A", &mikey_state->audio[1].output_value},
        {"AUD1L8SHFT", "FD2B", &mikey_state->audio[1].left_shift},
        {"AUD1TBACK ", "FD2C", &mikey_state->audio[1].timer_backup},
        {"AUD1CTL   ", "FD2D", &mikey_state->audio[1].control},
        {"AUD1COUNT ", "FD2E", &mikey_state->audio[1].count},
        {"AUD1MISC  ", "FD2F", &mikey_state->audio[1].misc},
        {"AUD2VOL   ", "FD30", &mikey_state->audio[2].volume},
        {"AUD2SHFTFB", "FD31", &mikey_state->audio[2].shift_feedback},
        {"AUD2OUTVAL", "FD32", &mikey_state->audio[2].output_value},
        {"AUD2L8SHFT", "FD33", &mikey_state->audio[2].left_shift},
        {"AUD2TBACK ", "FD34", &mikey_state->audio[2].timer_backup},
        {"AUD2CTL   ", "FD35", &mikey_state->audio[2].control},
        {"AUD2COUNT ", "FD36", &mikey_state->audio[2].count},
        {"AUD2MISC  ", "FD37", &mikey_state->audio[2].misc},
        {"AUD3VOL   ", "FD38", &mikey_state->audio[3].volume},
        {"AUD3SHFTFB", "FD39", &mikey_state->audio[3].shift_feedback},
        {"AUD3OUTVAL", "FD3A", &mikey_state->audio[3].output_value},
        {"AUD3L8SHFT", "FD3B", &mikey_state->audio[3].left_shift},
        {"AUD3TBACK ", "FD3C", &mikey_state->audio[3].timer_backup},
        {"AUD3CTL   ", "FD3D", &mikey_state->audio[3].control},
        {"AUD3COUNT ", "FD3E", &mikey_state->audio[3].count},
        {"AUD3MISC  ", "FD3F", &mikey_state->audio[3].misc},
        {0, 0, 0}
    };
    int i = 0;
    while (audio_regs[i].name != 0)
    {
        if (i % 8 == 0)
        {
            if (i > 0)
                ImGui::NewLine();

            ImGui::TextColored(cyan, "CHANNEL %d", i / 8); ImGui::Separator();
        }

        ImGui::TextColored(cyan, "%s ", audio_regs[i].addr); ImGui::SameLine();
        ImGui::TextColored(violet, "%s ", audio_regs[i].name); ImGui::SameLine();
        ImGui::Text("$%02X (" BYTE_TO_BINARY_PATTERN_SPACED ")", *audio_regs[i].reg, BYTE_TO_BINARY(*audio_regs[i].reg));
        i++;
    }
    ImGui::PopFont();
    ImGui::End();
    ImGui::PopStyleVar();
}

void gui_debug_window_mikey_colors(void)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(1000, 79), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(284, 400), ImGuiCond_FirstUseEver);

    ImGui::Begin("Mikey Color Registers", &config_debug.show_mikey_colors);
    ImGui::PushFont(gui_default_font);

    GearlynxCore* core = emu_get_core();
    Mikey::Mikey_State* mikey_state = core->GetMikey()->GetState();

    struct {
        const char* name;
        const char* addr_g;
        const char* addr_br;
        u8* green;
        u8* bluered;
    } color_regs[] = {
        {"COLOR0",  "FDA0", "FDB0", &mikey_state->colors[0].green,  &mikey_state->colors[0].bluered},
        {"COLOR1",  "FDA1", "FDB1", &mikey_state->colors[1].green,  &mikey_state->colors[1].bluered},
        {"COLOR2",  "FDA2", "FDB2", &mikey_state->colors[2].green,  &mikey_state->colors[2].bluered},
        {"COLOR3",  "FDA3", "FDB3", &mikey_state->colors[3].green,  &mikey_state->colors[3].bluered},
        {"COLOR4",  "FDA4", "FDB4", &mikey_state->colors[4].green,  &mikey_state->colors[4].bluered},
        {"COLOR5",  "FDA5", "FDB5", &mikey_state->colors[5].green,  &mikey_state->colors[5].bluered},
        {"COLOR6",  "FDA6", "FDB6", &mikey_state->colors[6].green,  &mikey_state->colors[6].bluered},
        {"COLOR7",  "FDA7", "FDB7", &mikey_state->colors[7].green,  &mikey_state->colors[7].bluered},
        {"COLOR8",  "FDA8", "FDB8", &mikey_state->colors[8].green,  &mikey_state->colors[8].bluered},
        {"COLOR9",  "FDA9", "FDB9", &mikey_state->colors[9].green,  &mikey_state->colors[9].bluered},
        {"COLORA",  "FDAA", "FDBA", &mikey_state->colors[10].green, &mikey_state->colors[10].bluered},
        {"COLORB",  "FDAB", "FDBB", &mikey_state->colors[11].green, &mikey_state->colors[11].bluered},
        {"COLORC",  "FDAC", "FDBC", &mikey_state->colors[12].green, &mikey_state->colors[12].bluered},
        {"COLORD",  "FDAD", "FDBD", &mikey_state->colors[13].green, &mikey_state->colors[13].bluered},
        {"COLORE",  "FDAE", "FDBE", &mikey_state->colors[14].green, &mikey_state->colors[14].bluered},
        {"COLORF",  "FDAF", "FDBF", &mikey_state->colors[15].green, &mikey_state->colors[15].bluered},
        {0, 0, 0, 0, 0}
    };
    int i = 0;
    while (color_regs[i].name != 0)
    {
        ImGui::TextColored(cyan, "%s ", color_regs[i].name); ImGui::SameLine();
        ImGui::Text("G: %s = $%02X (" BYTE_TO_BINARY_PATTERN_SPACED ")", color_regs[i].addr_g, *color_regs[i].green, BYTE_TO_BINARY(*color_regs[i].green)); ImGui::SameLine();
        ImGui::Text("BR: %s = $%02X (" BYTE_TO_BINARY_PATTERN_SPACED ")", color_regs[i].addr_br, *color_regs[i].bluered, BYTE_TO_BINARY(*color_regs[i].bluered));
    
        i++;
    }
    ImGui::PopFont();
    ImGui::End();
    ImGui::PopStyleVar();
}
