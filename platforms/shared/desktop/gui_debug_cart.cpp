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

#define GUI_DEBUG_CART_IMPORT
#include "gui_debug_cart.h"

#include "imgui.h"
#include "gearlynx.h"
#include "gui_debug_constants.h"
#include "gui.h"
#include "config.h"
#include "emu.h"

void gui_debug_window_cart(void)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(220, 160), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(280, 340), ImGuiCond_FirstUseEver);
    ImGui::Begin("Cartridge", &config_debug.show_cart);

    ImGui::PushFont(gui_default_font);

    GearlynxCore* core = emu_get_core();
    Media* media = core->GetMedia();

    bool ready = media->IsReady();

    // Shift Register / Address
    ImGui::TextColored(magenta, "ADDRESS GENERATION");
    ImGui::Separator();

    ImGui::TextColored(violet, "ADDR SHIFT    "); ImGui::SameLine();
    if (ready)
        ImGui::Text("$%02X", media->GetAddressShift());
    else
        ImGui::TextColored(gray, "N/A");

    ImGui::TextColored(violet, "PAGE OFFSET   "); ImGui::SameLine();
    if (ready)
        ImGui::Text("$%03X (%d)", media->GetCounterValue(), media->GetCounterValue());
    else
        ImGui::TextColored(gray, "N/A");

    ImGui::TextColored(violet, "STROBE        "); ImGui::SameLine();
    if (ready)
    {
        if (media->GetShiftRegisterStrobe())
            ImGui::TextColored(green, "HIGH (1)");
        else
            ImGui::TextColored(gray, "LOW  (0)");
    }
    else
        ImGui::TextColored(gray, "N/A");

    ImGui::TextColored(violet, "SHIFT BIT     "); ImGui::SameLine();
    if (ready)
    {
        if (media->GetShiftRegisterBit())
            ImGui::TextColored(green, "HIGH (1)");
        else
            ImGui::TextColored(gray, "LOW  (0)");
    }
    else
        ImGui::TextColored(gray, "N/A");

    ImGui::NewLine();

    // Bank 0
    ImGui::TextColored(magenta, "BANK 0 (ROM)");
    ImGui::Separator();

    ImGui::TextColored(violet, "SIZE          "); ImGui::SameLine();
    if (ready && media->GetBankSize(0) > 0)
        ImGui::Text("%d KB", media->GetBankSize(0) / 1024);
    else
        ImGui::TextColored(gray, "N/A");

    ImGui::TextColored(violet, "PEEK VALUE    "); ImGui::SameLine();
    if (ready && media->GetBankSize(0) > 0)
        ImGui::Text("$%02X", media->PeekBank0());
    else
        ImGui::TextColored(gray, "N/A");

    ImGui::TextColored(violet, "PEEK VALUE (A)"); ImGui::SameLine();
    if (ready && media->GetBankDataA(0) != NULL && media->GetBankSize(0) > 0)
        ImGui::Text("$%02X", media->PeekBank0A());
    else
        ImGui::TextColored(gray, "N/A");

    ImGui::NewLine();

    // Bank 1
    ImGui::TextColored(magenta, "BANK 1");
    ImGui::Separator();

    ImGui::TextColored(violet, "SIZE          "); ImGui::SameLine();
    if (ready && media->GetBankSize(1) > 0)
        ImGui::Text("%d KB", media->GetBankSize(1) / 1024);
    else
        ImGui::TextColored(gray, "N/A");

    ImGui::TextColored(violet, "TYPE          "); ImGui::SameLine();
    if (ready && media->GetBankSize(1) > 0)
    {
        if (media->IsBank1RAM())
            ImGui::TextColored(yellow, "RAM");
        else
            ImGui::Text("ROM");
    }
    else
        ImGui::TextColored(gray, "N/A");

    ImGui::TextColored(violet, "WRITE ENABLE  "); ImGui::SameLine();
    if (ready && media->GetBankSize(1) > 0 && media->IsBank1RAM())
    {
        if (media->IsBank1WriteEnabled())
            ImGui::TextColored(green, "YES");
        else
            ImGui::TextColored(red, "NO");
    }
    else
        ImGui::TextColored(gray, "N/A");

    ImGui::TextColored(violet, "PEEK VALUE    "); ImGui::SameLine();
    if (ready && media->GetBankSize(1) > 0)
        ImGui::Text("$%02X", media->PeekBank1());
    else
        ImGui::TextColored(gray, "N/A");

    ImGui::TextColored(violet, "PEEK VALUE (A)"); ImGui::SameLine();
    if (ready && media->GetBankDataA(1) != NULL && media->GetBankSize(1) > 0)
        ImGui::Text("$%02X", media->PeekBank1A());
    else
        ImGui::TextColored(gray, "N/A");

    ImGui::NewLine();

    // AUDIN
    ImGui::TextColored(magenta, "AUDIN");
    ImGui::Separator();

    ImGui::TextColored(violet, "CART AUDIN    "); ImGui::SameLine();
    if (ready)
    {
        if (media->GetAudin())
            ImGui::TextColored(green, "ACTIVE");
        else
            ImGui::TextColored(gray, "INACTIVE");
    }
    else
        ImGui::TextColored(gray, "N/A");

    ImGui::TextColored(violet, "AUDIN VALUE   "); ImGui::SameLine();
    if (ready && media->GetAudin())
    {
        if (media->GetAudinValue())
            ImGui::TextColored(green, "HIGH (1)");
        else
            ImGui::TextColored(gray, "LOW  (0)");
    }
    else
        ImGui::TextColored(gray, "N/A");

    ImGui::PopFont();

    ImGui::End();
    ImGui::PopStyleVar();
}
