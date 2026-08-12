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
#include "gui_debug_widgets.h"
#include "config.h"
#include "emu.h"
#include "utils.h"

static void MikeyWriteCallback8(u16 address, u8 value, void* user_data)
{
    Mikey* mikey = (Mikey*)user_data;
    mikey->Write<true>(address, value);
}

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
    ImGui::SetNextWindowSize(ImVec2(228, 448), ImGuiCond_FirstUseEver);
    ImGui::Begin("Mikey UART", &config_debug.show_uart);

    GearlynxCore* core = emu_get_core();
    Mikey* mikey = core->GetMikey();
    Mikey::Mikey_State* mikey_state = mikey->GetState();

    ImGui::PushFont(gui_default_font);


    EditableRegister8("SERCTL ", "FD8C", MIKEY_SERCTL, mikey_state->SERCTL, MikeyWriteCallback8, mikey);
    EditableRegister8("SERDAT ", "FD8D", MIKEY_SERDAT, mikey_state->SERDAT, MikeyWriteCallback8, mikey);

    ImGui::Separator();

    bool irq_enabled = (mikey_state->uart.tx_int_en || mikey_state->uart.rx_int_en);
    bool irq_asserted = IS_SET_BIT(mikey_state->irq_pending, 4);

    ImGui::TextColored(violet, "IRQ ENABLED   "); ImGui::SameLine();
    ImGui::TextColored(irq_enabled ? green : gray, irq_enabled ? "ON" : "OFF");

    ImGui::TextColored(violet, "IRQ ASSERTED  "); ImGui::SameLine();
    ImGui::TextColored(irq_asserted ? green : gray, irq_asserted ? "ON" : "OFF");

    ImGui::Separator();

    ImGui::TextColored(violet, "TX IRQ        "); ImGui::SameLine();
    ImGui::TextColored(mikey_state->uart.tx_int_en ? green : gray, "%s", mikey_state->uart.tx_int_en ? "YES" : "NO");

    ImGui::TextColored(violet, "RX IRQ        "); ImGui::SameLine();
    ImGui::TextColored(mikey_state->uart.rx_int_en ? green : gray, "%s", mikey_state->uart.rx_int_en ? "YES" : "NO");

    ImGui::TextColored(violet, "PARITY        "); ImGui::SameLine();
    ImGui::TextColored(mikey_state->uart.par_en ? green : gray, "%s", mikey_state->uart.par_en ? "YES" : "NO");

    ImGui::TextColored(violet, "TX OPEN       "); ImGui::SameLine();
    ImGui::TextColored(mikey_state->uart.tx_open ? green : gray, "%s", mikey_state->uart.tx_open ? "YES" : "NO");

    ImGui::TextColored(violet, "TX BREAK      "); ImGui::SameLine();
    ImGui::TextColored(mikey_state->uart.tx_brk ? green : gray, "%s", mikey_state->uart.tx_brk ? "YES" : "NO");

    ImGui::TextColored(violet, "PARITY EVEN   "); ImGui::SameLine();
    ImGui::TextColored(mikey_state->uart.par_even ? green : gray, "%s", mikey_state->uart.par_even ? "YES" : "NO");

    ImGui::Separator();

    ImGui::TextColored(violet, "TX READY      "); ImGui::SameLine();
    ImGui::TextColored(mikey_state->uart.tx_ready ? green : gray, "%s", mikey_state->uart.tx_ready ? "YES" : "NO");

    ImGui::TextColored(violet, "RX READY      "); ImGui::SameLine();
    ImGui::TextColored(mikey_state->uart.rx_ready ? green : gray, "%s", mikey_state->uart.rx_ready ? "YES" : "NO");

    ImGui::TextColored(violet, "TX EMPTY      "); ImGui::SameLine();
    ImGui::TextColored(mikey_state->uart.tx_empty ? green : gray, "%s", mikey_state->uart.tx_empty ? "YES" : "NO");

    ImGui::TextColored(violet, "PARITY ERR    "); ImGui::SameLine();
    ImGui::TextColored(mikey_state->uart.par_err ? green : gray, "%s", mikey_state->uart.par_err ? "YES" : "NO");

    ImGui::TextColored(violet, "OVERRUN ERR   "); ImGui::SameLine();
    ImGui::TextColored(mikey_state->uart.ovr_err ? green : gray, "%s", mikey_state->uart.ovr_err ? "YES" : "NO");

    ImGui::TextColored(violet, "FRAMING ERR   "); ImGui::SameLine();
    ImGui::TextColored(mikey_state->uart.fram_err ? green : gray, "%s", mikey_state->uart.fram_err ? "YES" : "NO");

    ImGui::TextColored(violet, "RX BREAK      "); ImGui::SameLine();
    ImGui::TextColored(mikey_state->uart.rx_break ? green : gray, "%s", mikey_state->uart.rx_break ? "YES" : "NO");

    ImGui::TextColored(violet, "PARITY BIT    "); ImGui::SameLine();
    ImGui::TextColored(white, "%s", mikey_state->uart.par_bit ? "1" : "0");

    ImGui::Separator();

    ImGui::TextColored(violet, "HOLDING REG   "); ImGui::SameLine();
    ImGui::Text("$%02X ", mikey_state->uart.tx_hold_data); ImGui::SameLine(0, 0);
    ImGui::TextColored(gray, "(" BYTE_TO_BINARY_PATTERN_SPACED ")", BYTE_TO_BINARY(mikey_state->uart.tx_hold_data));

    ImGui::TextColored(violet, "TX DATA       "); ImGui::SameLine();
    ImGui::Text("$%02X ", mikey_state->uart.tx_data); ImGui::SameLine(0, 0);
    ImGui::TextColored(gray, "(" BYTE_TO_BINARY_PATTERN_SPACED ")", BYTE_TO_BINARY(mikey_state->uart.tx_data));

    ImGui::TextColored(violet, "RX DATA       "); ImGui::SameLine();
    ImGui::Text("$%02X ", mikey_state->uart.rx_data); ImGui::SameLine(0, 0);
    ImGui::TextColored(gray, "(" BYTE_TO_BINARY_PATTERN_SPACED ")", BYTE_TO_BINARY(mikey_state->uart.rx_data));

    ImGui::TextColored(violet, "TX BIT IDX    "); ImGui::SameLine();
    ImGui::TextColored(white, "%d", mikey_state->uart.tx_bit_index);

    ImGui::Separator();

    ComLynxStatus comlynx = emu_comlynx_get_status();
    ImGui::TextColored(violet, "CABLE         "); ImGui::SameLine();
    ImGui::TextColored(comlynx.cable_connected ? green : gray, "%s", comlynx.cable_connected ? "CONNECTED" : "DISCONNECTED");

    ImGui::PopFont();

    ImGui::End();
    ImGui::PopStyleVar();
}

void gui_debug_window_comlynx(void)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(450, 90), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(342, 332), ImGuiCond_FirstUseEver);
    ImGui::Begin("ComLynx", &config_debug.show_comlynx);

    ComLynxStatus comlynx = emu_comlynx_get_status();
    const ImVec4 green(0.10f, 0.90f, 0.10f, 1.0f);
    const ImVec4 gray(0.55f, 0.55f, 0.55f, 1.0f);
    const ImVec4 red(0.98f, 0.15f, 0.45f, 1.0f);
    const ImVec4 white(1.0f, 1.0f, 1.0f, 1.0f);
    const ImVec4 violet(0.75f, 0.50f, 1.0f, 1.0f);

    const char* mode = "DISABLED";
    if (comlynx.mode == ComLynxModeHosting)
        mode = "HOST";
    else if (comlynx.mode == ComLynxModeJoining)
        mode = "JOINING";
    else if (comlynx.mode == ComLynxModeConnected)
        mode = "CLIENT";
    else if (comlynx.mode == ComLynxModeFault)
        mode = "FAULT";

    ImGui::PushFont(gui_default_font);

    ImGui::TextColored(violet, "CABLE           "); ImGui::SameLine();
    ImGui::TextColored(comlynx.cable_connected ? green : gray, "%s", comlynx.cable_connected ? "CONNECTED" : "DISCONNECTED");
    ImGui::TextColored(violet, "NETWORK         "); ImGui::SameLine();
    ImGui::TextColored(comlynx.mode == ComLynxModeFault ? red : white, "%s", mode);
    ImGui::TextColored(violet, "PEER            "); ImGui::SameLine();
    ImGui::TextColored(white, "%d / %d", comlynx.local_peer_id, comlynx.peer_count);

    ImGui::Separator();
    ImGui::TextColored(violet, "FRAME GEN/TX    "); ImGui::SameLine();
    ImGui::TextColored(white, "%llu / %llu", (unsigned long long)comlynx.frames_generated,
        (unsigned long long)comlynx.frames_sent);
    ImGui::TextColored(violet, "FRAME RX NET/Q/C"); ImGui::SameLine();
    ImGui::TextColored(white, "%llu / %llu / %llu", (unsigned long long)comlynx.frames_received_network,
        (unsigned long long)comlynx.frames_queued, (unsigned long long)comlynx.frames_consumed);
    ImGui::TextColored(violet, "DROP DIS/CLEAR  "); ImGui::SameLine();
    ImGui::TextColored(white, "%llu / %llu", (unsigned long long)comlynx.frames_dropped_disabled,
        (unsigned long long)comlynx.frames_dropped_clear);
    ImGui::TextColored(violet, "DGRAM TX/RX     "); ImGui::SameLine();
    ImGui::TextColored(white, "%llu / %llu", (unsigned long long)comlynx.datagrams_sent,
        (unsigned long long)comlynx.datagrams_received);
    ImGui::TextColored(violet, "SEND EAGAIN/ERR "); ImGui::SameLine();
    ImGui::TextColored(white, "%llu / %llu", (unsigned long long)comlynx.send_eagain,
        (unsigned long long)comlynx.send_errors);
    ImGui::TextColored(violet, "QUEUE MAX TX/RX "); ImGui::SameLine();
    ImGui::TextColored(white, "%u / %u", comlynx.max_outgoing_queue_depth,
        comlynx.max_incoming_queue_depth);
    ImGui::TextColored(violet, "PENDING MAX     "); ImGui::SameLine();
    ImGui::TextColored(white, "%u", comlynx.max_pending_packet_depth);

    ImGui::Separator();
    ImGui::TextColored(violet, "GAPS/OOO/DUP    "); ImGui::SameLine();
    ImGui::TextColored(white, "%llu / %llu / %llu", (unsigned long long)comlynx.sequence_gaps,
        (unsigned long long)comlynx.out_of_order_packets,
        (unsigned long long)comlynx.duplicate_packets);
    ImGui::TextColored(violet, "QUEUE OVERFLOW  "); ImGui::SameLine();
    ImGui::TextColored(white, "%llu", (unsigned long long)comlynx.queue_overflows);

    ImGui::Separator();
    ImGui::TextColored(violet, "RX INTERVAL US  "); ImGui::SameLine();
    ImGui::TextColored(white, "%llu / %llu / %llu", (unsigned long long)comlynx.frame_rx_interval_min_us,
        (unsigned long long)comlynx.frame_rx_interval_avg_us,
        (unsigned long long)comlynx.frame_rx_interval_max_us);
    ImGui::TextColored(violet, "RX INTERVAL VAR "); ImGui::SameLine();
    ImGui::TextColored(white, "%llu", (unsigned long long)comlynx.frame_rx_interval_variation_us);
    ImGui::TextColored(violet, "RX BURST AVG/MAX"); ImGui::SameLine();
    ImGui::TextColored(white, "%llu / %u", comlynx.rx_bursts ?
        (unsigned long long)(comlynx.rx_burst_total_packets / comlynx.rx_bursts) : 0ULL,
        comlynx.rx_burst_max);
    ImGui::TextColored(violet, "BUS BURST OK/FRC"); ImGui::SameLine();
    ImGui::TextColored(white, "%llu / %llu", (unsigned long long)comlynx.bursts_delivered,
        (unsigned long long)comlynx.bursts_forced);
    ImGui::TextColored(violet, "BUS BURST MAXLEN"); ImGui::SameLine();
    ImGui::TextColored(white, "%u", comlynx.max_burst_length);

    if (comlynx.mode == ComLynxModeFault)
    {
        ImGui::Separator();
        ImGui::TextColored(red, "%s", comlynx.last_error);
    }

    ImGui::Separator();

    if (ImGui::Button("RESET METRICS"))
        emu_comlynx_reset_metrics();

    ImGui::PopFont();

    ImGui::End();
    ImGui::PopStyleVar();
}
