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

#include <cstring>
#include <istream>
#include <ostream>
#include "mikey.h"
#include "memory.h"
#include "state_serializer.h"
#include "lcd_screen.h"

Mikey::Mikey(Suzy* suzy, Media* media, M6502* m6502, Bus* bus)
{
    m_suzy = suzy;
    m_media = media;
    m_m6502 = m6502;
    m_bus = bus;
    InitPointer(m_audio);
    InitPointer(m_lcd_screen);
}

Mikey::~Mikey()
{
    SafeDelete(m_lcd_screen);
}

void Mikey::Init(Memory* memory, GLYNX_Pixel_Format pixel_format)
{
    m_lcd_screen = new LcdScreen(this, memory, m_bus);
    m_lcd_screen->Init(pixel_format);
    Reset(true);
}

void Mikey::SetAudio(Audio* audio)
{
    m_audio = audio;
}

void Mikey::Reset(bool is_lynx2)
{
    memset(&m_state, 0, sizeof(Mikey_State));

    m_is_lynx2 = is_lynx2;

    m_lcd_screen->Reset();

    ResetPalette();
    ResetTimers();
    ResetAudio();
    ResetUART();
}

void Mikey::ResetTimers()
{
    for (int i = 0; i < 8; i++)
    {
        m_state.timers[i].backup = 0;
        m_state.timers[i].counter = 0;
        m_state.timers[i].control_a = 0;
        m_state.timers[i].control_b = 0;

        m_state.timers[i].internal_cycles = 0;
        m_state.timers[i].internal_period_cycles = k_mikey_timer_period_cycles[0];
        m_state.timers[i].internal_pending_ticks = 0;
    }

    m_lcd_screen->ConfigureLineTiming();
}

void Mikey::ResetAudio()
{
    for (int i = 0; i < 4; i++)
    {
        m_state.audio[i].volume = 0;
        m_state.audio[i].feedback = 0;
        m_state.audio[i].output = 0;
        m_state.audio[i].lfsr_low = 0;
        m_state.audio[i].backup = 0;
        m_state.audio[i].control = 0;
        m_state.audio[i].counter = 0;
        m_state.audio[i].other = 0;

        m_state.audio[i].internal_cycles = 0;
        m_state.audio[i].internal_period_cycles = k_mikey_timer_period_cycles[0];
        m_state.audio[i].internal_pending_ticks = 0;
        m_state.audio[i].internal_lfsr = 0;
        m_state.audio[i].internal_taps_mask = 0;
        m_state.audio[i].internal_mix = true;
    }

    m_state.MSTEREO = 0x00;
    m_state.MPAN = 0x00;
    m_state.ATTEN_A = 0xFF;
    m_state.ATTEN_B = 0xFF;
    m_state.ATTEN_C = 0xFF;
    m_state.ATTEN_D = 0xFF;
}

void Mikey::ResetUART()
{
    m_state.uart.tx_int_en = false;
    m_state.uart.rx_int_en = false;
    m_state.uart.par_en = false;
    m_state.uart.tx_open = false;
    m_state.uart.tx_brk = false;
    m_state.uart.par_even = false;
    m_state.uart.tx_ready = true;
    m_state.uart.rx_ready = false;
    m_state.uart.tx_empty = true;
    m_state.uart.par_err = false;
    m_state.uart.ovr_err = false;
    m_state.uart.fram_err = false;
    m_state.uart.rx_break = false;
    m_state.uart.par_bit = false;
    m_state.uart.tx_active = false;
    m_state.uart.tx_hold_valid = false;
    m_state.uart.tx_suppress_eof_loopback = false;
    m_state.uart.tx_parbit = false;
    m_state.uart.tx_hold_data = 0;
    m_state.uart.tx_data = 0;
    m_state.uart.rx_data = 0;
    m_state.uart.tx_bit_index = 0;
    m_state.uart.prescaler = 0;
    m_state.uart.tx_empty_bits = 0;
    m_state.uart.tx_ready_bits = 0;
    m_state.uart.tx_started_from_chain = false;
}

void Mikey::ResetPalette()
{
    for (int address = 0xFDA0; address < 0xFDC0; address++)
        WriteColor(address, 0xFF);
}

void Mikey::HorizontalBlank()
{
    m_state.refresh_cycle_counter = 0;

    u8 counter = m_state.timers[2].counter;
    u8 backup = m_state.timers[2].backup;

    int first_visible_counter = (backup >= 104) ? 102 : (backup - 2);

    // Start of vblank 0
    if (counter == 0)
    {
        m_lcd_screen->SetVBlank(true);
        m_state.rest = true;

        // Clear lines that won't be rendered when backup < 104
        if (backup < 104)
        {
            int visible_lines = backup - 2;
            for (int line = visible_lines; line < 102; line++)
            {
                m_lcd_screen->ClearLine(line);
            }
        }
    }
    // Start of vblank 1
    else if (counter == backup)
    {
        m_state.rest = false;
    }
    // Start of vblank 2
    else if (counter == (backup - 1))
    {
        m_state.dispadr_latch = m_state.DISPADR.value & 0xFFFC;
        m_lcd_screen->FirstDMA();
    }
    // Visible lines
    else if (counter <= first_visible_counter && counter >= 1)
    {
        int visible_line = first_visible_counter - counter;

        // Start of visible line 0 (end of vblank)
        if (visible_line == 0)
        {
            m_lcd_screen->SetVBlank(false);
            m_state.frame_ready = true;
        }
        // Start of visible line 1
        else if (visible_line == 1)
        {
            m_state.rest = true;
        }

        m_lcd_screen->ResetLine(visible_line);
    }
}

void Mikey::SaveState(std::ostream& stream)
{
    StateSerializer serializer(stream);
    Serialize(serializer);

    m_lcd_screen->SaveState(stream);
}

void Mikey::LoadState(std::istream& stream)
{
    StateSerializer serializer(stream);
    Serialize(serializer);

    m_lcd_screen->LoadState(stream);
}

void Mikey::Serialize(StateSerializer& s)
{
    for (int i = 0; i < 8; i++)
    {
        G_SERIALIZE(s, m_state.timers[i].backup);
        G_SERIALIZE(s, m_state.timers[i].control_a);
        G_SERIALIZE(s, m_state.timers[i].control_b);
        G_SERIALIZE(s, m_state.timers[i].counter);

        G_SERIALIZE(s, m_state.timers[i].internal_cycles);
        G_SERIALIZE(s, m_state.timers[i].internal_period_cycles);
        G_SERIALIZE(s, m_state.timers[i].internal_pending_ticks);
    }

    for (int i = 0; i < 16; i++)
    {
        G_SERIALIZE(s, m_state.colors[i].green);
        G_SERIALIZE(s, m_state.colors[i].bluered);
    }

    for (int i = 0; i < 4; i++)
    {
        G_SERIALIZE(s, m_state.audio[i].volume);
        G_SERIALIZE(s, m_state.audio[i].feedback);
        G_SERIALIZE(s, m_state.audio[i].output);
        G_SERIALIZE(s, m_state.audio[i].lfsr_low);
        G_SERIALIZE(s, m_state.audio[i].backup);
        G_SERIALIZE(s, m_state.audio[i].control);
        G_SERIALIZE(s, m_state.audio[i].counter);
        G_SERIALIZE(s, m_state.audio[i].other);

        G_SERIALIZE(s, m_state.audio[i].internal_cycles);
        G_SERIALIZE(s, m_state.audio[i].internal_period_cycles);
        G_SERIALIZE(s, m_state.audio[i].internal_pending_ticks);
        G_SERIALIZE(s, m_state.audio[i].internal_lfsr);
        G_SERIALIZE(s, m_state.audio[i].internal_taps_mask);
        G_SERIALIZE(s, m_state.audio[i].internal_mix);
    }

    G_SERIALIZE(s, m_state.uart.tx_int_en);
    G_SERIALIZE(s, m_state.uart.rx_int_en);
    G_SERIALIZE(s, m_state.uart.par_en);
    G_SERIALIZE(s, m_state.uart.tx_open);
    G_SERIALIZE(s, m_state.uart.tx_brk);
    G_SERIALIZE(s, m_state.uart.par_even);
    G_SERIALIZE(s, m_state.uart.tx_ready);
    G_SERIALIZE(s, m_state.uart.rx_ready);
    G_SERIALIZE(s, m_state.uart.tx_empty);
    G_SERIALIZE(s, m_state.uart.par_err);
    G_SERIALIZE(s, m_state.uart.ovr_err);
    G_SERIALIZE(s, m_state.uart.fram_err);
    G_SERIALIZE(s, m_state.uart.rx_break);
    G_SERIALIZE(s, m_state.uart.par_bit);
    G_SERIALIZE(s, m_state.uart.tx_active);
    G_SERIALIZE(s, m_state.uart.tx_hold_valid);
    G_SERIALIZE(s, m_state.uart.tx_parbit);
    G_SERIALIZE(s, m_state.uart.tx_suppress_eof_loopback);
    G_SERIALIZE(s, m_state.uart.tx_hold_data);
    G_SERIALIZE(s, m_state.uart.tx_data);
    G_SERIALIZE(s, m_state.uart.rx_data);
    G_SERIALIZE(s, m_state.uart.tx_bit_index);
    G_SERIALIZE(s, m_state.uart.prescaler);
    G_SERIALIZE(s, m_state.uart.tx_empty_bits);
    G_SERIALIZE(s, m_state.uart.tx_ready_bits);
    G_SERIALIZE(s, m_state.uart.tx_started_from_chain);
    G_SERIALIZE(s, m_state.uart.rxq_head);
    G_SERIALIZE(s, m_state.uart.rxq_count);
    G_SERIALIZE_ARRAY(s, m_state.uart.rxq_data, 2);
    G_SERIALIZE_ARRAY(s, m_state.uart.rxq_flags, 2);

    G_SERIALIZE(s, m_state.ATTEN_A);
    G_SERIALIZE(s, m_state.ATTEN_B);
    G_SERIALIZE(s, m_state.ATTEN_C);
    G_SERIALIZE(s, m_state.ATTEN_D);
    G_SERIALIZE(s, m_state.MPAN);
    G_SERIALIZE(s, m_state.MSTEREO);
    G_SERIALIZE(s, m_state.SYSCTL1);
    G_SERIALIZE(s, m_state.IODIR);
    G_SERIALIZE(s, m_state.IODAT);
    G_SERIALIZE(s, m_state.SERCTL);
    G_SERIALIZE(s, m_state.SERDAT);
    G_SERIALIZE(s, m_state.SDONEACK);
    G_SERIALIZE(s, m_state.CPUSLEEP);
    G_SERIALIZE(s, m_state.DISPCTL);
    G_SERIALIZE(s, m_state.PBKUP);
    G_SERIALIZE(s, m_state.DISPADR.value);
    G_SERIALIZE(s, m_state.irq_pending);
    G_SERIALIZE(s, m_state.irq_mask);
    G_SERIALIZE(s, m_state.frame_ready);
    G_SERIALIZE(s, m_state.dispadr_latch);
    G_SERIALIZE(s, m_state.rest);
    G_SERIALIZE(s, m_state.refresh_cycle_counter);
}