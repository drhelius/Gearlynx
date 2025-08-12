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

#ifndef M6502_OPCODES_INLINE_H
#define M6502_OPCODES_INLINE_H

#include "m6502.h"
#include "memory.h"
#include "m6502_names.h"

INLINE void M6502::OPCodes_ADC(u8 value)
{
    u8 a = m_A.GetValue();

    u16 result = 0;
    if (IsSetFlag(FLAG_DECIMAL))
    {
        m_cycles++;

        result = (u16)(a & 0x0F) + (u16)(value & 0x0F) + (IsSetFlag(FLAG_CARRY) ? 1 : 0);
        if(result > 0x09)
            result += 0x06;
        result = (u16)(a & 0xF0) + (u16)(value & 0xF0) + (result > 0x0F ? 0x10 : 0) + (result & 0x0F);

        if(~(a ^ value) & ((result & 0xFF) ^ a) & 0x80)
            SetFlag(FLAG_OVERFLOW);
        else
            ClearFlag(FLAG_OVERFLOW);

        if(result > 0x9F)
            result += 0x60;
    }
    else
    {
        result = a + value + (IsSetFlag(FLAG_CARRY) ? 1 : 0);

        if(~(a ^ value) & (a ^ result) & 0x80)
            SetFlag(FLAG_OVERFLOW);
        else
            ClearFlag(FLAG_OVERFLOW);
    }

    u8 final_result = static_cast<u8>(result & 0xFF);

    ClearFlag(FLAG_ZERO | FLAG_CARRY | FLAG_NEGATIVE);
    SetZNFlags(final_result);

    if(result > 0xFF) {
        SetFlag(FLAG_CARRY);
    }

    m_A.SetValue(final_result);
}

INLINE void M6502::OPCodes_AND(u8 value)
{
    u8 result = m_A.GetValue() & value;
    m_A.SetValue(result);
    SetOrClearZNFlags(result);
}

INLINE void M6502::OPCodes_ASL_Accumulator()
{
    u8 value = m_A.GetValue();
    u8 result = static_cast<u8>(value << 1);
    m_A.SetValue(result);
    SetOrClearZNFlags(result);
    if ((value & 0x80) != 0)
        SetFlag(FLAG_CARRY);
    else
        ClearFlag(FLAG_CARRY);
}

INLINE void M6502::OPCodes_ASL_Memory(u16 address)
{
    u8 value = m_memory->Read(address);
    u8 result = static_cast<u8>(value << 1);
    m_memory->Write(address, result);
    SetOrClearZNFlags(result);
    if ((value & 0x80) != 0)
        SetFlag(FLAG_CARRY);
    else
        ClearFlag(FLAG_CARRY);
}

INLINE void M6502::OPcodes_Branch(bool condition)
{
    if (condition)
    {
        s8 displacement = RelativeAddressing();
        u16 address = m_PC.GetValue();
        u16 result = static_cast<u16>(address + displacement);
        m_PC.SetValue(result);
        m_cycles += 2;
    }
    else
        m_PC.Increment();
}

INLINE void M6502::OPCodes_BIT(u16 address)
{
    u8 value = m_memory->Read(address);
    u8 result = m_A.GetValue() & value;
    ClearFlag(FLAG_ZERO | FLAG_OVERFLOW | FLAG_NEGATIVE);
    u8 flags = m_P.GetValue();
    flags |= (m_zn_flags_lut[result] & FLAG_ZERO);
    flags |= (value & (FLAG_OVERFLOW | FLAG_NEGATIVE));
    m_P.SetValue(flags);
}

INLINE void M6502::OPCodes_BIT_Immediate(u16 address)
{
    u8 value = m_memory->Read(address);
    u8 result = m_A.GetValue() & value;
    ClearFlag(FLAG_ZERO);
    u8 flags = m_P.GetValue();
    flags |= (m_zn_flags_lut[result] & FLAG_ZERO);
    m_P.SetValue(flags);
}

INLINE void M6502::OPCodes_BRK()
{
    u16 pc = m_PC.GetValue();
    StackPush16(pc + 1);
    StackPush8(m_P.GetValue() | FLAG_BREAK);
    ClearFlag(FLAG_DECIMAL);
    SetFlag(FLAG_INTERRUPT);

    m_PC.SetLow(m_memory->Read(0xFFFE));
    m_PC.SetHigh(m_memory->Read(0xFFFF));

#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    u16 dest = m_PC.GetValue();
    PushCallStack(pc - 1, dest, pc + 1);
#endif
}

INLINE void M6502::OPCodes_CMP(EightBitRegister* reg, u8 value)
{
    u8 reg_value = reg->GetValue();
    u8 result = reg_value - value;
    SetOrClearZNFlags(result);
    if (reg_value >= value)
        SetFlag(FLAG_CARRY);
    else
        ClearFlag(FLAG_CARRY);
}

INLINE void M6502::OPCodes_DEC_Mem(u16 address)
{
    u8 value = m_memory->Read(address);
    u8 result = value - 1;
    m_memory->Write(address, result);
    SetOrClearZNFlags(result);
}

INLINE void M6502::OPCodes_DEC_Reg(EightBitRegister* reg)
{
    reg->Decrement();
    SetOrClearZNFlags(reg->GetValue());
}

INLINE void M6502::OPCodes_EOR(u8 value)
{
    u8 result = m_A.GetValue() ^ value;
    m_A.SetValue(result);
    SetOrClearZNFlags(result);
}

INLINE void M6502::OPCodes_INC_Mem(u16 address)
{
    u8 value = m_memory->Read(address);
    u8 result = value + 1;
    m_memory->Write(address, result);
    SetOrClearZNFlags(result);
}

INLINE void M6502::OPCodes_INC_Reg(EightBitRegister* reg)
{
    reg->Increment();
    SetOrClearZNFlags(reg->GetValue());
}

INLINE void M6502::OPCodes_LD(EightBitRegister* reg, u8 value)
{
    reg->SetValue(value);
    SetOrClearZNFlags(value);
}

INLINE void M6502::OPCodes_LSR_Accumulator()
{
    u8 value = m_A.GetValue();
    u8 result = value >> 1;
    m_A.SetValue(result);
    SetOrClearZNFlags(result);
    if ((value & 0x01) != 0)
        SetFlag(FLAG_CARRY);
    else
        ClearFlag(FLAG_CARRY);
}

INLINE void M6502::OPCodes_LSR_Memory(u16 address)
{
    u8 value = m_memory->Read(address);
    u8 result = value >> 1;
    m_memory->Write(address, result);
    SetOrClearZNFlags(result);
    if ((value & 0x01) != 0)
        SetFlag(FLAG_CARRY);
    else
        ClearFlag(FLAG_CARRY);
}

INLINE void M6502::OPCodes_ORA(u8 value)
{
    u8 result = m_A.GetValue() | value;
    m_A.SetValue(result);
    SetOrClearZNFlags(result);
}

INLINE void M6502::OPCodes_RMB(u8 bit, u16 address)
{
    u8 result = UNSET_BIT(m_memory->Read(address), bit);
    m_memory->Write(address, result);
}

INLINE void M6502::OPCodes_ROL_Accumulator()
{
    u8 value = m_A.GetValue();
    u8 result = static_cast<u8>(value << 1);
    result |= IsSetFlag(FLAG_CARRY) ? 0x01 : 0x00;
    m_A.SetValue(result);
    SetOrClearZNFlags(result);
    if ((value & 0x80) != 0)
        SetFlag(FLAG_CARRY);
    else
        ClearFlag(FLAG_CARRY);
}

INLINE void M6502::OPCodes_ROL_Memory(u16 address)
{
    u8 value = m_memory->Read(address);
    u8 result = static_cast<u8>(value << 1);
    result |= IsSetFlag(FLAG_CARRY) ? 0x01 : 0x00;
    m_memory->Write(address, result);
    SetOrClearZNFlags(result);
    if ((value & 0x80) != 0)
        SetFlag(FLAG_CARRY);
    else
        ClearFlag(FLAG_CARRY);
}

INLINE void M6502::OPCodes_ROR_Accumulator()
{
    u8 value = m_A.GetValue();
    u8 result = value >> 1;
    result |= IsSetFlag(FLAG_CARRY) ? 0x80 : 0x00;
    m_A.SetValue(result);
    SetOrClearZNFlags(result);
    if ((value & 0x01) != 0)
        SetFlag(FLAG_CARRY);
    else
        ClearFlag(FLAG_CARRY);
}

INLINE void M6502::OPCodes_ROR_Memory(u16 address)
{
    u8 value = m_memory->Read(address);
    u8 result = value >> 1;
    result |= IsSetFlag(FLAG_CARRY) ? 0x80 : 0x00;
    m_memory->Write(address, result);
    SetOrClearZNFlags(result);
    if ((value & 0x01) != 0)
        SetFlag(FLAG_CARRY);
    else
        ClearFlag(FLAG_CARRY);
}

INLINE void M6502::OPCodes_SBC(u8 value)
{
    u16 result = 0;

    if (IsSetFlag(FLAG_DECIMAL))
    {
        m_cycles++;

        u16 tmp = (m_A.GetValue() & 0x0f) - (value & 0x0f) - (IsSetFlag(FLAG_CARRY) ? 0 : 1);
        result = m_A.GetValue() - value - (IsSetFlag(FLAG_CARRY) ? 0 : 1);

        if (result & 0x8000)
            result -= 0x60;

        if (tmp & 0x8000)
            result -= 0x06;

        u16 bin_result = m_A.GetValue() + ~value + (IsSetFlag(FLAG_CARRY) ? 1 : 0);
        if ((m_A.GetValue() ^ bin_result) & (~value ^ bin_result) & 0x80)
            SetFlag(FLAG_OVERFLOW);
        else
            ClearFlag(FLAG_OVERFLOW);

        if ((u16)result <= (u16)m_A.GetValue() || (result & 0xff0) == 0xff0)
            SetFlag(FLAG_CARRY);
        else
            ClearFlag(FLAG_CARRY);
    }
    else
    {
        value = ~value;
        result = (u16)m_A.GetValue() + (u16)value + (IsSetFlag(FLAG_CARRY) ? 1 : 0);

        if(~(m_A.GetValue() ^ value) & (m_A.GetValue() ^ result) & 0x80)
            SetFlag(FLAG_OVERFLOW);
        else
            ClearFlag(FLAG_OVERFLOW);

        if(result > 0xFF)
            SetFlag(FLAG_CARRY);
        else
            ClearFlag(FLAG_CARRY);
    }

    SetOrClearZNFlags((u8)result);
    m_A.SetValue((u8)result);
}

INLINE void M6502::OPCodes_SMB(u8 bit, u16 address)
{
    u8 result = SET_BIT(m_memory->Read(address), bit);
    m_memory->Write(address, result);
}

INLINE void M6502::OPCodes_Store(EightBitRegister* reg, u16 address)
{
    u8 value = reg->GetValue();
    m_memory->Write(address, value);
}

INLINE void M6502::OPCodes_STZ(u16 address)
{
    m_memory->Write(address, 0x00);
}

INLINE void M6502::OPCodes_Transfer(EightBitRegister* source, EightBitRegister* dest)
{
    u8 value = source->GetValue();
    dest->SetValue(value);
    SetOrClearZNFlags(value);
}

INLINE void M6502::OPCodes_TRB(u16 address)
{
    u8 value = m_memory->Read(address);
    u8 result = ~m_A.GetValue() & value;
    m_memory->Write(address, result);
    ClearFlag(FLAG_ZERO);
    u8 flags = m_P.GetValue();
    flags |= (m_zn_flags_lut[m_A.GetValue() & value] & FLAG_ZERO);
    m_P.SetValue(flags);
}

INLINE void M6502::OPCodes_TSB(u16 address)
{
    u8 value = m_memory->Read(address);
    u8 result = m_A.GetValue() | value;
    m_memory->Write(address, result);
    ClearFlag(FLAG_ZERO);
    u8 flags = m_P.GetValue();
    flags |= (m_zn_flags_lut[m_A.GetValue() & value] & FLAG_ZERO);
    m_P.SetValue(flags);
}

inline void M6502::UnofficialOPCode()
{
#if defined(GLYNX_DEBUG)
    u16 opcode_address = m_PC.GetValue() - 1;
    u8 opcode = m_memory->Read(opcode_address);
    Debug("** M6502 --> UNOFFICIAL OP Code (%02X) at $%.4X -- %s", opcode, opcode_address, k_m6502_opcode_names[opcode]);
#endif
}

#endif /* M6502_OPCODES_INLINE_H */
