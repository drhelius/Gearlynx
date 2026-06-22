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

static const char* cart_bank_type_name(GLYNX_Cartridge_Bank_Type type)
{
    switch (type)
    {
        case GLYNX_CART_BANK_ROM:
            return "ROM";
        case GLYNX_CART_BANK_RAM:
            return "RAM";
        case GLYNX_CART_BANK_RAM_PERSISTENT:
            return "RAM+SAVE";
        default:
            return "UNUSED";
    }
}

static void draw_cart_bank(Media* media, int bank, bool ready)
{
    if (!ready || media->GetCartBankData(bank) == NULL || media->GetCartBankSize(bank) == 0)
        return;

    ImGui::TextColored(magenta, "%s", media->GetCartBankName(bank));
    ImGui::Separator();

    ImGui::TextColored(violet, "SIZE          "); ImGui::SameLine();
    ImGui::Text("%d KB", media->GetCartBankSize(bank) / 1024);

    ImGui::TextColored(violet, "TYPE          "); ImGui::SameLine();
    ImGui::TextColored(media->IsCartBankWritable(bank) ? yellow : blue, "%s", cart_bank_type_name(media->GetCartBankType(bank)));

    ImGui::TextColored(violet, "WRITABLE      "); ImGui::SameLine();
    if (media->IsCartBankWritable(bank))
        ImGui::TextColored(green, "YES");
    else
        ImGui::TextColored(gray, "NO");

    ImGui::TextColored(violet, "BLOCKS        "); ImGui::SameLine();
    ImGui::Text("%d x %d", media->GetCartBankBlockCount(bank), media->GetCartBankBlockSize(bank));

    ImGui::TextColored(violet, "PEEK VALUE    "); ImGui::SameLine();
    ImGui::Text("$%02X", media->PeekCartBank(bank));

    ImGui::NewLine();
}

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
            ImGui::TextColored(red, "LOW  (0)");
    }
    else
        ImGui::TextColored(gray, "N/A");

    ImGui::TextColored(violet, "SHIFT BIT     "); ImGui::SameLine();
    if (ready)
    {
        if (media->GetShiftRegisterBit())
            ImGui::TextColored(green, "HIGH (1)");
        else
            ImGui::TextColored(red, "LOW  (0)");
    }
    else
        ImGui::TextColored(gray, "N/A");

    ImGui::NewLine();

    for (int bank = 0; bank < Media::CART_BANK_COUNT; bank++)
        draw_cart_bank(media, bank, ready);

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
            ImGui::TextColored(red, "LOW  (0)");
    }
    else
        ImGui::TextColored(gray, "N/A");

    ImGui::PopFont();

    ImGui::End();
    ImGui::PopStyleVar();
}
