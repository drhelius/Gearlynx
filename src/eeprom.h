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

#ifndef EEPROM_H
#define EEPROM_H

#include "common.h"

class StateSerializer;

class EEPROM
{
public:
    EEPROM();
    ~EEPROM();
    void Reset(GLYNX_EEPROM type);
    GLYNX_EEPROM GetType();
    bool IsAvailable();
    s32 GetSize();
    void ProcessIO(u8 iodir, u8 iodat);
    void ProcessEepromCounter(u16 counter);
    void ProcessBusy();
    bool OutputBit();
    u8* GetData();
    bool IsDirty();
    void ClearDirty();
    void SetData(u8* data, s32 size);
    void SaveState(std::ostream& stream);
    void LoadState(std::istream& stream);

private:
    void SetType(GLYNX_EEPROM type);
    void Serialize(StateSerializer& s);

private:
    enum EepromState
    {
        EE_NONE = 0,
        EE_WAIT,
        EE_ADDR,
        EE_DATA,
        EE_BUSY
    };

    GLYNX_EEPROM m_type;
    EepromState m_state;
    u32 m_data;
    u16 m_addr;
    u16 m_read_data;
    u16 m_rom_data[1024];
    bool m_audin_output;
    bool m_readonly;
    bool m_dirty;
    bool m_programming;
    s32 m_busy_count;
    bool m_last_cs;
    bool m_last_clk;
    s32 m_addr_bits;
    u16 m_done_mask;
    u8 m_iodir;
    u8 m_iodat;
};

#endif /* EEPROM_H */
