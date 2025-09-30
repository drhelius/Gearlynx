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

static ImVec4 color_444_to_float(u16 color);

void gui_debug_window_mikey_regs(void)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(93, 79), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(284, 400), ImGuiCond_FirstUseEver);

    ImGui::Begin("Mikey Registers", &config_debug.show_mikey_regs);

    ImGui::PushFont(gui_default_font);

    GearlynxCore* core = emu_get_core();
    Mikey* mikey = core->GetMikey();
    Mikey::Mikey_State* mikey_state = mikey->GetState();
    u8 iodat = mikey->Read(MIKEY_IODAT);

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
        {"IODAT   ", "FD8B", &iodat},
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
        ImGui::TextColored(orange, "%s ", regs8[i].name); ImGui::SameLine();
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
        ImGui::TextColored(orange, "%s ", regs16[i].name); ImGui::SameLine();
        ImGui::Text("$%04X (" BYTE_TO_BINARY_PATTERN_SPACED " " BYTE_TO_BINARY_PATTERN_SPACED ")", regs16[i].reg->value, BYTE_TO_BINARY(regs16[i].reg->high), BYTE_TO_BINARY(regs16[i].reg->low));
        i++;
    }

    ImGui::PopFont();

    ImGui::End();
    ImGui::PopStyleVar();
}

void gui_debug_window_mikey_timer_regs(void)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(400, 79), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(284, 400), ImGuiCond_FirstUseEver);

    ImGui::Begin("Mikey Timer Registers", &config_debug.show_mikey_timer_regs);
    ImGui::PushFont(gui_default_font);

    GearlynxCore* core = emu_get_core();
    Mikey::Mikey_State* mikey_state = core->GetMikey()->GetState();

    struct {
        const char* name;
        const char* addr;
        u8* reg;
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
        {"TIM0CNT ", "FD02", &mikey_state->timers[0].counter},
        {"TIM0CTLB", "FD03", &mikey_state->timers[0].control_b},
        {"TIM1CTLA", "FD05", &mikey_state->timers[1].control_a},
        {"TIM1CNT ", "FD06", &mikey_state->timers[1].counter},
        {"TIM1CTLB", "FD07", &mikey_state->timers[1].control_b},
        {"TIM2CTLA", "FD09", &mikey_state->timers[2].control_a},
        {"TIM2CNT ", "FD0A", &mikey_state->timers[2].counter},
        {"TIM2CTLB", "FD0B", &mikey_state->timers[2].control_b},
        {"TIM3CTLA", "FD0D", &mikey_state->timers[3].control_a},
        {"TIM3CNT ", "FD0E", &mikey_state->timers[3].counter},
        {"TIM3CTLB", "FD0F", &mikey_state->timers[3].control_b},
        {"TIM4CTLA", "FD11", &mikey_state->timers[4].control_a},
        {"TIM4CNT ", "FD12", &mikey_state->timers[4].counter},
        {"TIM4CTLB", "FD13", &mikey_state->timers[4].control_b},
        {"TIM5CTLA", "FD15", &mikey_state->timers[5].control_a},
        {"TIM5CNT ", "FD16", &mikey_state->timers[5].counter},
        {"TIM5CTLB", "FD17", &mikey_state->timers[5].control_b},
        {"TIM6CTLA", "FD19", &mikey_state->timers[6].control_a},
        {"TIM6CNT ", "FD1A", &mikey_state->timers[6].counter},
        {"TIM6CTLB", "FD1B", &mikey_state->timers[6].control_b},
        {"TIM7CTLA", "FD1D", &mikey_state->timers[7].control_a},
        {"TIM7CNT ", "FD1E", &mikey_state->timers[7].counter},
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
        ImGui::TextColored(orange, "%s ", timer_regs16[i].name); ImGui::SameLine();
        ImGui::Text("$%04X (" BYTE_TO_BINARY_PATTERN_SPACED " " BYTE_TO_BINARY_PATTERN_SPACED ")", *timer_regs16[i].reg, BYTE_TO_BINARY(*timer_regs16[i].reg >> 8), BYTE_TO_BINARY(*timer_regs16[i].reg & 0xFF));

        for (int j = 0; j < 3; j++)
        {
            int idx = (i * 3) + j;
            ImGui::TextColored(cyan, "%s ", timer_regs8[idx].addr); ImGui::SameLine();
            ImGui::TextColored(orange, "%s   ", timer_regs8[idx].name); ImGui::SameLine();
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
        {"AUD0SHFTFB", "FD21", &mikey_state->audio[0].feedback},
        {"AUD0OUTVAL", "FD22", (u8*)&mikey_state->audio[0].output},
        {"AUD0L8SHFT", "FD23", &mikey_state->audio[0].lfsr_low},
        {"AUD0TBACK ", "FD24", &mikey_state->audio[0].backup},
        {"AUD0CTL   ", "FD25", &mikey_state->audio[0].control},
        {"AUD0COUNT ", "FD26", &mikey_state->audio[0].counter},
        {"AUD0MISC  ", "FD27", &mikey_state->audio[0].other},
        {"AUD1VOL   ", "FD28", &mikey_state->audio[1].volume},
        {"AUD1SHFTFB", "FD29", &mikey_state->audio[1].feedback},
        {"AUD1OUTVAL", "FD2A", (u8*)&mikey_state->audio[1].output},
        {"AUD1L8SHFT", "FD2B", &mikey_state->audio[1].lfsr_low},
        {"AUD1TBACK ", "FD2C", &mikey_state->audio[1].backup},
        {"AUD1CTL   ", "FD2D", &mikey_state->audio[1].control},
        {"AUD1COUNT ", "FD2E", &mikey_state->audio[1].counter},
        {"AUD1MISC  ", "FD2F", &mikey_state->audio[1].other},
        {"AUD2VOL   ", "FD30", &mikey_state->audio[2].volume},
        {"AUD2SHFTFB", "FD31", &mikey_state->audio[2].feedback},
        {"AUD2OUTVAL", "FD32", (u8*)&mikey_state->audio[2].output},
        {"AUD2L8SHFT", "FD33", &mikey_state->audio[2].lfsr_low},
        {"AUD2TBACK ", "FD34", &mikey_state->audio[2].backup},
        {"AUD2CTL   ", "FD35", &mikey_state->audio[2].control},
        {"AUD2COUNT ", "FD36", &mikey_state->audio[2].counter},
        {"AUD2MISC  ", "FD37", &mikey_state->audio[2].other},
        {"AUD3VOL   ", "FD38", &mikey_state->audio[3].volume},
        {"AUD3SHFTFB", "FD39", &mikey_state->audio[3].feedback},
        {"AUD3OUTVAL", "FD3A", (u8*)&mikey_state->audio[3].output},
        {"AUD3L8SHFT", "FD3B", &mikey_state->audio[3].lfsr_low},
        {"AUD3TBACK ", "FD3C", &mikey_state->audio[3].backup},
        {"AUD3CTL   ", "FD3D", &mikey_state->audio[3].control},
        {"AUD3COUNT ", "FD3E", &mikey_state->audio[3].counter},
        {"AUD3MISC  ", "FD3F", &mikey_state->audio[3].other},
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
        ImGui::TextColored(orange, "%s ", audio_regs[i].name); ImGui::SameLine();
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

    const u16 base = 0xFDA0;

    for (int line = 0; line < 4; line++)
    {
        for (int c = 0; c < 4; c++)
        {
            int idx = (line * 4) + c;
            ImGui::TextColored(orange, "   %02d      ", idx);

            if (c < 3)
            {
                ImGui::SameLine();
                ImGui::SameLine();
            }
        }

        for (int c = 0; c < 4; c++)
        {
            u16 addr = (line * 4) + c + base;
            ImGui::TextColored(cyan, "%04X,%04X  ", addr , addr + 0x10);

            if (c < 3)
            {
                ImGui::SameLine();
                ImGui::SameLine();
            }
        }

        ImGui::Text("   "); ImGui::SameLine(0,0);

        for (int c = 0; c < 4; c++)
        {
            int idx = (line * 4) + c;
            u16 green = mikey_state->colors[idx].green;
            u16 bluered = mikey_state->colors[idx].bluered;
            u16 color = (green << 8) | bluered;
            ImVec4 float_color = color_444_to_float(color);

            if (c > 0)
            ImGui::Text("      "); ImGui::SameLine(0,0);

            char id[16];
            snprintf(id, sizeof(id), "##pal_%d_%d", line, c);

            ImGui::ColorEdit3(id, (float*)&float_color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoPicker);

            if (c < 3)
            {
                ImGui::SameLine(); ImGui::Dummy(ImVec2(8.0f, 0.0f));
                ImGui::SameLine();
            }
        }

        ImGui::Text("  "); ImGui::SameLine(0,0);

        for (int c = 0; c < 4; c++)
        {
            int idx = (line * 4) + c;

            u8 color_green = mikey_state->colors[idx].green & 0x0F;
            u8 color_blue = (mikey_state->colors[idx].bluered >> 4) & 0x0F;
            u8 color_red = mikey_state->colors[idx].bluered & 0x0F;

            ImGui::TextColored(green, " %01X", color_green); ImGui::SameLine(0,0);
            ImGui::TextColored(blue, "%01X", color_blue); ImGui::SameLine(0,0);
            ImGui::TextColored(red, "%01X       ", color_red);

            if (c < 3)
            {   
                ImGui::SameLine();
                ImGui::SameLine();
            }
        }

        if (line < 3)
        {
            ImGui::NewLine();
            ImGui::Separator();

        }

        //ImGui::Text("     "); ImGui::SameLine(0,0);

    }

    ImGui::PopFont();
    ImGui::End();
    ImGui::PopStyleVar();
}

static ImVec4 color_444_to_float(u16 color)
{
    ImVec4 ret;
    ret.w = 0;
    ret.x = (1.0f / 15.0f) * (color & 0xF);
    ret.z = (1.0f / 15.0f) * ((color >> 4) & 0xF);
    ret.y = (1.0f / 15.0f) * ((color >> 8) & 0xF);
    return ret;
}
