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
#include <fstream>
#include "eeprom.h"
#include "bit_ops.h"
#include "state_serializer.h"

EEPROM::EEPROM()
{
    Reset(GLYNX_EEPROM_NONE);
}

EEPROM::~EEPROM()
{
}

void EEPROM::Reset(GLYNX_EEPROM type)
{
    m_state = EE_NONE;
    m_data = 0;
    m_addr = 0;
    m_read_data = 0;
    m_audin_output = true;
    m_readonly = true;
    m_dirty = false;
    m_programming = false;
    m_busy_count = 100;  // Start in ready state
    m_last_cs = false;
    m_last_clk = false;
    m_addr_bits = 0;
    m_done_mask = 0;
    m_iodir = 0;
    m_iodat = 0;
    std::memset(m_rom_data, 0xFF, sizeof(m_rom_data));
    SetType(type);
}

void EEPROM::SetType(GLYNX_EEPROM type)
{
    m_type = type;

    s32 base_type = m_type & 0x0F;

    switch (base_type)
    {
        case GLYNX_EEPROM_93C46:
            m_addr_bits = 6;
            break;
        case GLYNX_EEPROM_93C56:
        case GLYNX_EEPROM_93C66:
            m_addr_bits = 8;
            break;
        case GLYNX_EEPROM_93C76:
        case GLYNX_EEPROM_93C86:
            m_addr_bits = 10;
            break;
        default:
            m_addr_bits = 6;
            break;
    }

    // 8-bit mode has one extra address bit
    if (m_type & GLYNX_EEPROM_8BIT)
        m_addr_bits++;

    // done_mask: after receiving (1 start + 2 opcode + addr_bits) bits,
    // the MSB is at position (addr_bits + 2)
    m_done_mask = 1 << (m_addr_bits + 2);

    Debug("EEPROM type set: %d, addr_bits: %d, done_mask: 0x%04X", m_type, m_addr_bits, m_done_mask);
}

GLYNX_EEPROM EEPROM::GetType()
{
    return m_type;
}

bool EEPROM::IsAvailable()
{
    return (m_type != GLYNX_EEPROM_NONE);
}

s32 EEPROM::GetSize()
{
    if (!IsAvailable())
        return 0;

    s32 base_type = m_type & 0x0F;
    s32 size = 0;

    switch (base_type)
    {
        case GLYNX_EEPROM_93C46:
            size = 128;
            break;
        case GLYNX_EEPROM_93C56:
        case GLYNX_EEPROM_93C66:
            size = 256;
            break;
        case GLYNX_EEPROM_93C76:
        case GLYNX_EEPROM_93C86:
            size = 1024;
            break;
        default:
            size = 128;
            break;
    }

    // 8-bit mode doubles the size in bytes
    if (m_type & GLYNX_EEPROM_8BIT)
        size *= 2;

    return size;
}

void EEPROM::ProcessIO(u8 iodir, u8 iodat)
{
    // Store IODIR/IODAT for use in ProcessEepromCounter
    m_iodir = iodir;
    m_iodat = iodat;
}

void EEPROM::ProcessEepromCounter(u16 counter)
{
    if (!IsAvailable())
        return;

    // CS from counter bit 7
    bool cs = IS_SET_BIT(counter, 7);

    // CLK from counter bit 1
    bool clk = IS_SET_BIT(counter, 1);

    // DI comes from IODAT bit 4 (AUDIN) when IODIR bit 4 is set (output)
    bool di = false;
    if (IS_SET_BIT(m_iodir, 4))
        di = IS_SET_BIT(m_iodat, 4);

    // CS falling edge resets command state
    if (!cs && m_last_cs)
    {
        //Debug("EEPROM: CS LOW, state was %d data=%04X", m_state, m_data);
        m_state = EE_NONE;
        m_data = 0;
    }

    // CS rising edge - prepare for start bit detection
    if (cs && !m_last_cs)
    {
        //Debug("EEPROM: CS HIGH");
        m_state = EE_NONE;
        m_data = 0;
    }

    m_last_cs = cs;

    // Only process data on rising CLK edge when CS is high
    if (!cs || !clk || m_last_clk)
    {
        m_last_clk = clk;
        return;
    }

    m_last_clk = clk;

    // In EE_NONE state, look for start bit (DI = 1)
    if (m_state == EE_NONE)
    {
        if (di)
        {
            //Debug("EEPROM: START mask=%04X", m_done_mask);
            m_data = 0x01;  // Start bit
            m_state = EE_ADDR;
        }
        return;
    }

    // Shift in data bit
    m_data = (m_data << 1) | (di ? 1 : 0);

    switch (m_state)
    {
        case EE_NONE:
            // Should not reach here - start detection handled above
            break;

        case EE_ADDR:
            if (m_data & m_done_mask)
            {
                //Debug("EEPROM: CMD data=%04X mask=%04X", m_data, m_done_mask);
                // Extract opcode (2 bits after start bit)
                s32 opcode = (m_data >> m_addr_bits) & 0x03;
                m_addr = m_data & ((1 << m_addr_bits) - 1);

                switch (opcode)
                {
                    case 0x02:  // READ
                        // Don't pre-shift 8-bit data - output bit check uses done_mask >> 1
                        if (m_type & GLYNX_EEPROM_8BIT)
                            m_read_data = ((u8*)m_rom_data)[m_addr];
                        else
                            m_read_data = m_rom_data[m_addr];
                        m_audin_output = false;  // Dummy bit
                        m_programming = false;   // Reading, not programming
                        m_state = EE_WAIT;
                        //Debug("EEPROM READ addr: 0x%02X, data: 0x%04X", m_addr, m_read_data);
                        break;

                    case 0x01:  // WRITE
                        m_data = 0x01;
                        m_state = EE_DATA;
                        //Debug("EEPROM WRITE addr: 0x%02X", m_addr);
                        break;

                    case 0x00:  // Extended commands
                        {
                            s32 ext_cmd = (m_data >> (m_addr_bits - 2)) & 0x03;
                            switch (ext_cmd)
                            {
                                case 0x00:  // EWDS - Erase/Write Disable
                                    m_readonly = true;
                                    //Debug("EEPROM EWDS");
                                    break;
                                case 0x03:  // EWEN - Erase/Write Enable
                                    m_readonly = false;
                                    //Debug("EEPROM EWEN");
                                    break;
                                case 0x01:  // WRAL - Write All
                                    //Debug("EEPROM WRAL (not implemented)");
                                    break;
                                case 0x02:  // ERAL - Erase All
                                    //Debug("EEPROM ERAL (not implemented)");
                                    break;
                            }
                        }
                        m_state = EE_NONE;
                        break;

                    case 0x03:  // ERASE
                        if (!m_readonly)
                        {
                            if (m_type & GLYNX_EEPROM_8BIT)
                                ((u8*)m_rom_data)[m_addr] = 0xFF;
                            else
                                m_rom_data[m_addr] = 0xFFFF;
                            m_dirty = true;
                            //Debug("EEPROM ERASE addr: 0x%02X", m_addr);
                        }
                        m_busy_count = 0;
                        m_programming = true;    // Programming mode
                        m_audin_output = false;  // Busy
                        m_state = EE_WAIT;
                        break;
                }
            }
            break;

        case EE_DATA:
            {
                u32 data_done_mask = (m_type & GLYNX_EEPROM_8BIT) ? 0x0100 : 0x10000;
                if (m_data & data_done_mask)
                {
                    if (!m_readonly)
                    {
                        if (m_type & GLYNX_EEPROM_8BIT)
                        {
                            ((u8*)m_rom_data)[m_addr] = m_data & 0xFF;
                            //Debug("EEPROM WRITE data: 0x%02X", m_data & 0xFF);
                        }
                        else
                        {
                            m_rom_data[m_addr] = m_data & 0xFFFF;
                            //Debug("EEPROM WRITE data: 0x%04X", m_data & 0xFFFF);
                        }
                        m_dirty = true;
                    }
                    m_busy_count = 0;
                    m_programming = true;    // Programming mode
                    m_audin_output = false;  // Busy (ready signal)
                    m_state = EE_WAIT;
                }
            }
            break;

        case EE_WAIT:
            if (m_programming)
            {
                // Programming mode (WRITE/ERASE): just stay in busy/ready state
                // m_audin_output is managed by ProcessBusy()
            }
            else
            {
                // Read mode: shift out read data from MSB
                if (m_type & GLYNX_EEPROM_8BIT)
                    m_audin_output = (m_read_data & 0x80) != 0;
                else
                    m_audin_output = (m_read_data & 0x8000) != 0;
                m_read_data <<= 1;
            }
            break;

        case EE_BUSY:
            // EE_BUSY state is handled by ProcessBusy() when polled via IODAT read
            break;
    }
}

void EEPROM::ProcessBusy()
{
    if (!IsAvailable())
        return;

    // After write/erase, simulate busy period then go ready
    if (m_programming && m_busy_count < 100)
    {
        m_busy_count++;
        if (m_busy_count >= 100)
        {
            m_audin_output = true;  // Ready
            m_programming = false;  // Done programming
        }
    }
}

bool EEPROM::OutputBit()
{
    return m_audin_output;
}

u8* EEPROM::GetData()
{
    return (u8*)m_rom_data;
}

bool EEPROM::IsDirty()
{
    return m_dirty;
}

void EEPROM::ClearDirty()
{
    m_dirty = false;
}

void EEPROM::SetData(u8* data, s32 size)
{
    if (data != NULL && size > 0)
    {
        s32 copy_size = (size < (s32)sizeof(m_rom_data)) ? size : (s32)sizeof(m_rom_data);
        std::memcpy(m_rom_data, data, copy_size);
    }
}

void EEPROM::SaveState(std::ostream& stream)
{
    StateSerializer serializer(stream);
    Serialize(serializer);
}

void EEPROM::LoadState(std::istream& stream)
{
    StateSerializer serializer(stream);
    Serialize(serializer);
}

void EEPROM::Serialize(StateSerializer& s)
{
    s32 state = static_cast<s32>(m_state);

    G_SERIALIZE(s, state);
    G_SERIALIZE(s, m_data);
    G_SERIALIZE(s, m_addr);
    G_SERIALIZE(s, m_read_data);
    G_SERIALIZE(s, m_audin_output);
    G_SERIALIZE(s, m_readonly);
    G_SERIALIZE(s, m_dirty);
    G_SERIALIZE(s, m_programming);
    G_SERIALIZE(s, m_busy_count);
    G_SERIALIZE(s, m_last_cs);
    G_SERIALIZE(s, m_last_clk);
    G_SERIALIZE(s, m_iodir);
    G_SERIALIZE(s, m_iodat);

    s32 eeprom_size = GetSize();
    G_SERIALIZE_ARRAY(s, (u8*)m_rom_data, eeprom_size);

    m_state = static_cast<EepromState>(state);
}
