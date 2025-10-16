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

#define GUI_DEBUG_UART_IMPORT
#include "gui_debug_uart.h"

#include "imgui.h"
#include "gearlynx.h"
#include "gui.h"
#include "gui_debug_constants.h"
#include "config.h"
#include "emu.h"
#include "utils.h"

void gui_debug_uart_init(void)
{
}

void gui_debug_uart_destroy(void)
{
}

void gui_debug_window_uart(void)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(200, 90), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(220, 444), ImGuiCond_FirstUseEver);
    ImGui::Begin("Mikey UART", &config_debug.show_uart);

    GearlynxCore* core = emu_get_core();
    Mikey::Mikey_State* mikey_state = core->GetMikey()->GetState();

    ImGui::PushFont(gui_default_font);


    ImGui::TextColored(cyan, "FD8C "); ImGui::SameLine();
    ImGui::TextColored(orange, "SERCTL "); ImGui::SameLine();
    ImGui::Text("$%02X (" BYTE_TO_BINARY_PATTERN_SPACED ")", mikey_state->SERCTL, BYTE_TO_BINARY(mikey_state->SERCTL));

    ImGui::TextColored(cyan, "FD8D "); ImGui::SameLine();
    ImGui::TextColored(orange, "SERDAT "); ImGui::SameLine();
    ImGui::Text("$%02X (" BYTE_TO_BINARY_PATTERN_SPACED ")", mikey_state->SERDAT, BYTE_TO_BINARY(mikey_state->SERDAT));

    ImGui::Separator();

    bool irq_enabled = (mikey_state->uart.tx_int_en || mikey_state->uart.rx_int_en);
    bool irq_asserted = IS_SET_BIT(mikey_state->irq_pending, 4);

    ImGui::TextColored(violet, "IRQ ENABLED  "); ImGui::SameLine();
    ImGui::TextColored(irq_enabled ? green : gray, irq_enabled ? "ON" : "OFF");

    ImGui::TextColored(violet, "IRQ ASSERTED "); ImGui::SameLine();
    ImGui::TextColored(irq_asserted ? green : gray, irq_asserted ? "ON" : "OFF");

    ImGui::Separator();

    ImGui::TextColored(violet, "TX IRQ       "); ImGui::SameLine();
    ImGui::TextColored(mikey_state->uart.tx_int_en ? green : gray, "%s", mikey_state->uart.tx_int_en ? "YES" : "NO");

    ImGui::TextColored(violet, "RX IRQ       "); ImGui::SameLine();
    ImGui::TextColored(mikey_state->uart.rx_int_en ? green : gray, "%s", mikey_state->uart.rx_int_en ? "YES" : "NO");

    ImGui::TextColored(violet, "PARITY       "); ImGui::SameLine();
    ImGui::TextColored(mikey_state->uart.par_en ? green : gray, "%s", mikey_state->uart.par_en ? "YES" : "NO");

    ImGui::TextColored(violet, "RESET ERR    "); ImGui::SameLine();
    ImGui::TextColored(mikey_state->uart.reset_err ? green : gray, "%s", mikey_state->uart.reset_err ? "YES" : "NO");

    ImGui::TextColored(violet, "TX OPEN      "); ImGui::SameLine();
    ImGui::TextColored(mikey_state->uart.tx_open ? green : gray, "%s", mikey_state->uart.tx_open ? "YES" : "NO");

    ImGui::TextColored(violet, "TX BREAK     "); ImGui::SameLine();
    ImGui::TextColored(mikey_state->uart.tx_brk ? green : gray, "%s", mikey_state->uart.tx_brk ? "YES" : "NO");

    ImGui::TextColored(violet, "PARITY EVEN  "); ImGui::SameLine();
    ImGui::TextColored(mikey_state->uart.par_even ? green : gray, "%s", mikey_state->uart.par_even ? "YES" : "NO");

    ImGui::Separator();

    ImGui::TextColored(violet, "TX READY     "); ImGui::SameLine();
    ImGui::TextColored(mikey_state->uart.tx_ready ? green : gray, "%s", mikey_state->uart.tx_ready ? "YES" : "NO");

    ImGui::TextColored(violet, "RX READY     "); ImGui::SameLine();
    ImGui::TextColored(mikey_state->uart.rx_ready ? green : gray, "%s", mikey_state->uart.rx_ready ? "YES" : "NO");

    ImGui::TextColored(violet, "TX EMPTY     "); ImGui::SameLine();
    ImGui::TextColored(mikey_state->uart.tx_empty ? green : gray, "%s", mikey_state->uart.tx_empty ? "YES" : "NO");

    ImGui::TextColored(violet, "PARITY ERR   "); ImGui::SameLine();
    ImGui::TextColored(mikey_state->uart.par_err ? green : gray, "%s", mikey_state->uart.par_err ? "YES" : "NO");

    ImGui::TextColored(violet, "OVERRUN ERR  "); ImGui::SameLine();
    ImGui::TextColored(mikey_state->uart.ovr_err ? green : gray, "%s", mikey_state->uart.ovr_err ? "YES" : "NO");

    ImGui::TextColored(violet, "FRAMING ERR  "); ImGui::SameLine();
    ImGui::TextColored(mikey_state->uart.fram_err ? green : gray, "%s", mikey_state->uart.fram_err ? "YES" : "NO");

    ImGui::TextColored(violet, "RX BREAK     "); ImGui::SameLine();
    ImGui::TextColored(mikey_state->uart.rx_break ? green : gray, "%s", mikey_state->uart.rx_break ? "YES" : "NO");

    ImGui::TextColored(violet, "PARITY BIT   "); ImGui::SameLine();
    ImGui::TextColored(white, "%s", mikey_state->uart.par_bit ? "1" : "0");

    ImGui::Separator();

    ImGui::TextColored(violet, "HOLDING REG  "); ImGui::SameLine();
    ImGui::Text("$%02X (" BYTE_TO_BINARY_PATTERN_SPACED ")", mikey_state->uart.tx_hold_data, BYTE_TO_BINARY(mikey_state->uart.tx_hold_data));

    ImGui::TextColored(violet, "TX DATA      "); ImGui::SameLine();
    ImGui::Text("$%02X (" BYTE_TO_BINARY_PATTERN_SPACED ")", mikey_state->uart.tx_data, BYTE_TO_BINARY(mikey_state->uart.tx_data));

    ImGui::TextColored(violet, "RX DATA      "); ImGui::SameLine();
    ImGui::Text("$%02X (" BYTE_TO_BINARY_PATTERN_SPACED ")", mikey_state->uart.rx_data, BYTE_TO_BINARY(mikey_state->uart.rx_data));

    ImGui::TextColored(violet, "TX BIT IDX   "); ImGui::SameLine();
    ImGui::TextColored(white, "%d", mikey_state->uart.tx_bit_index);

    ImGui::PopFont();

    ImGui::End();
    ImGui::PopStyleVar();
}
