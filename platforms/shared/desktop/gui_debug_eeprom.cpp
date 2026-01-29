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

#define GUI_DEBUG_EEPROM_IMPORT
#include "gui_debug_eeprom.h"

#include "imgui.h"
#include "gearlynx.h"
#include "gui_debug_constants.h"
#include "gui_debug_widgets.h"
#include "gui.h"
#include "config.h"
#include "emu.h"
#include "mikey_defines.h"

static const char* get_eeprom_type_name(GLYNX_EEPROM type);

static void MikeyWriteCallback8(u16 address, u8 value, void* user_data)
{
    Mikey* mikey = (Mikey*)user_data;
    mikey->Write<true>(address, value);
}

void gui_debug_window_eeprom(void)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(200, 140), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(260, 280), ImGuiCond_FirstUseEver);
    ImGui::Begin("EEPROM", &config_debug.show_eeprom);

    ImGui::PushFont(gui_default_font);

    GearlynxCore* core = emu_get_core();
    Media* media = core->GetMedia();
    Mikey* mikey = core->GetMikey();
    EEPROM* eeprom = media->GetEEPROMInstance();
    Mikey::Mikey_State* mikey_state = mikey->GetState();

    bool available = eeprom->IsAvailable();
    GLYNX_EEPROM type = available ? eeprom->GetType() : GLYNX_EEPROM_NONE;
    s32 size = available ? eeprom->GetSize() : 0;
    bool dirty = available ? eeprom->IsDirty() : false;
    bool output_bit = available ? eeprom->OutputBit() : false;

    u8 iodir = mikey_state->IODIR;
    u8 iodat = mikey_state->IODAT;

    // CS on bit 2, CLK on bit 1, DI on bit 0
    bool cs = IS_SET_BIT(iodat, 2) && IS_SET_BIT(iodir, 2);
    bool clk = IS_SET_BIT(iodat, 1);
    bool di = IS_SET_BIT(iodat, 0);

    // EEPROM Type Info
    ImGui::TextColored(magenta, "INFO");
    ImGui::Separator();

    ImGui::TextColored(violet, "TYPE          "); ImGui::SameLine();
    if (available)
        ImGui::Text("%s", get_eeprom_type_name(type));
    else
        ImGui::TextColored(gray, "N/A");

    ImGui::TextColored(violet, "SIZE          "); ImGui::SameLine();
    if (available)
        ImGui::Text("%d bytes", size);
    else
        ImGui::TextColored(gray, "N/A");

    ImGui::TextColored(violet, "MODE          "); ImGui::SameLine();
    if (available)
    {
        if (type & GLYNX_EEPROM_8BIT)
            ImGui::Text("8-bit");
        else
            ImGui::Text("16-bit");
    }
    else
        ImGui::TextColored(gray, "N/A");

    ImGui::NewLine();

    // EEPROM State
    ImGui::TextColored(magenta, "STATE");
    ImGui::Separator();

    ImGui::TextColored(violet, "DIRTY         "); ImGui::SameLine();
    if (available)
    {
        if (dirty)
            ImGui::TextColored(yellow, "YES");
        else
            ImGui::TextColored(green, "NO");
    }
    else
        ImGui::TextColored(gray, "N/A");

    ImGui::TextColored(violet, "OUTPUT BIT    "); ImGui::SameLine();
    if (available)
    {
        if (output_bit)
            ImGui::TextColored(green, "HIGH (1)");
        else
            ImGui::TextColored(red, "LOW  (0)");
    }
    else
        ImGui::TextColored(gray, "N/A");

    ImGui::NewLine();

    // Mikey IO Pins (EEPROM related)
    ImGui::TextColored(magenta, "IO PINS");
    ImGui::Separator();

    u8 iodat_read = mikey->Read<true>(MIKEY_IODAT);
    EditableRegister8("IODIR", "FD8A", MIKEY_IODIR, iodir, MikeyWriteCallback8, mikey);
    EditableRegister8("IODAT", "FD8B", MIKEY_IODAT, iodat_read, MikeyWriteCallback8, mikey);

    ImGui::TextColored(violet, "CS (CART1/IO2)"); ImGui::SameLine();
    if (cs)
        ImGui::TextColored(green, "HIGH (1)");
    else
        ImGui::TextColored(red, "LOW  (0)");

    ImGui::TextColored(violet, "CLK (IO1)     "); ImGui::SameLine();
    if (clk)
        ImGui::TextColored(green, "HIGH (1)");
    else
        ImGui::TextColored(red, "LOW  (0)");

    ImGui::TextColored(violet, "DI (IO0)      "); ImGui::SameLine();
    if (di)
        ImGui::TextColored(green, "HIGH (1)");
    else
        ImGui::TextColored(red, "LOW  (0)");

    ImGui::PopFont();

    ImGui::End();
    ImGui::PopStyleVar();
}

static const char* get_eeprom_type_name(GLYNX_EEPROM type)
{
    s32 base_type = type & 0x0F;

    switch (base_type)
    {
        case GLYNX_EEPROM_93C46:
            return "93C46";
        case GLYNX_EEPROM_93C56:
            return "93C56";
        case GLYNX_EEPROM_93C66:
            return "93C66";
        case GLYNX_EEPROM_93C76:
            return "93C76";
        case GLYNX_EEPROM_93C86:
            return "93C86";
        default:
            return "Unknown";
    }
}
