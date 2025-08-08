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
#include "config.h"
#include "emu.h"
#include "utils.h"

static MemEditor mem_edit[6];
static float plot_x[32];
static float plot_y[32];
static bool exclusive_channel[6] = { false, false, false, false, false, false };
static float* wave_buffer_left = NULL;
static float* wave_buffer_right = NULL;

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
    for (int i = 0; i < 6; i++)
    {
        mem_edit[i].SetGuiFont(gui_roboto_font);
    }

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(180, 45), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(444, 400), ImGuiCond_FirstUseEver);
    ImGui::Begin("PSG", &config_debug.show_psg);

    GearlynxCore* core = emu_get_core();


    ImGui::End();
    ImGui::PopStyleVar();
}
