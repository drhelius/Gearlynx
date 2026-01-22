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

#include "m6502.h"
#include "memory.h"

void M6502::OPCode0x00()
{
    // BRK
    OPCodes_BRK();
}

void M6502::OPCode0x01()
{
    // ORA (ZP,X)
    OPCodes_ORA(MemRead8(ZeroPageIndexedIndirectAddressing()));
}

void M6502::OPCode0x02()
{
    UnofficialOPCode();
    m_s.PC.Increment();
}

void M6502::OPCode0x03()
{
    m_s.onebyte_un_nop = true;
    UnofficialOPCode();
}

void M6502::OPCode0x04()
{
    // TSB ZP
    OPCodes_TSB(ZeroPageAddressing());
}

void M6502::OPCode0x05()
{
    // ORA ZP
    OPCodes_ORA(MemRead8(ZeroPageAddressing()));
}

void M6502::OPCode0x06()
{
    // ASL ZP
    OPCodes_ASL_Memory(ZeroPageAddressing());
}

void M6502::OPCode0x07()
{
    // RMB0 ZP
    OPCodes_RMB(0, ZeroPageAddressing());
}

void M6502::OPCode0x08()
{
    // PHP
    StackPush8(m_s.P.GetValue() | FLAG_BREAK);
}

void M6502::OPCode0x09()
{
    // ORA #nn
    OPCodes_ORA(ImmediateAddressing());
}

void M6502::OPCode0x0A()
{
    // ASL A
    OPCodes_ASL_Accumulator();
}

void M6502::OPCode0x0B()
{
    m_s.onebyte_un_nop = true;
    UnofficialOPCode();
}

void M6502::OPCode0x0C()
{
    // TSB hhll
    OPCodes_TSB(AbsoluteAddressing());
}

void M6502::OPCode0x0D()
{
    // ORA hhll
    OPCodes_ORA(MemRead8(AbsoluteAddressing()));
}

void M6502::OPCode0x0E()
{
    // ASL hhll
    OPCodes_ASL_Memory(AbsoluteAddressing());
}

void M6502::OPCode0x0F()
{
    // BBR0 ZP,rr
    OPcodes_Branch(IS_NOT_SET_BIT(MemRead8(ZeroPageAddressing()), 0));
}

void M6502::OPCode0x10()
{
    // BPL rr
    OPcodes_Branch(IsNotSetFlag(FLAG_NEGATIVE));
}

void M6502::OPCode0x11()
{
    // ORA (ZP),Y
    OPCodes_ORA(MemRead8(ZeroPageIndirectIndexedAddressing()));
}

void M6502::OPCode0x12()
{
    // ORA (ZP)
    OPCodes_ORA(MemRead8(ZeroPageIndirectAddressing()));
}

void M6502::OPCode0x13()
{
    m_s.onebyte_un_nop = true;
    UnofficialOPCode();
}

void M6502::OPCode0x14()
{
    // TRB ZP
    OPCodes_TRB(ZeroPageAddressing());
}

void M6502::OPCode0x15()
{
    // ORA ZP,X
    OPCodes_ORA(MemRead8(ZeroPageAddressing(&m_s.X)));
}

void M6502::OPCode0x16()
{
    // ASL ZP,X
    OPCodes_ASL_Memory(ZeroPageAddressing(&m_s.X));
}

void M6502::OPCode0x17()
{
    // RMB1 ZP
    OPCodes_RMB(1, ZeroPageAddressing());
}

void M6502::OPCode0x18()
{
    // CLC
    ClearFlag(FLAG_CARRY);
}

void M6502::OPCode0x19()
{
    // ORA hhll,Y
    OPCodes_ORA(MemRead8(AbsoluteAddressing(&m_s.Y)));
}

void M6502::OPCode0x1A()
{
    // INC A
    OPCodes_INC_Reg(&m_s.A);
}

void M6502::OPCode0x1B()
{
    m_s.onebyte_un_nop = true;
    UnofficialOPCode();
}

void M6502::OPCode0x1C()
{
    // TRB hhll
    OPCodes_TRB(AbsoluteAddressing());
}

void M6502::OPCode0x1D()
{
    // ORA hhll,X
    OPCodes_ORA(MemRead8(AbsoluteAddressing(&m_s.X)));
}

void M6502::OPCode0x1E()
{
    // ASL hhll,X
    OPCodes_ASL_Memory(AbsoluteAddressing(&m_s.X));
}

void M6502::OPCode0x1F()
{
    // BBR1 ZP,rr
    OPcodes_Branch(IS_NOT_SET_BIT(MemRead8(ZeroPageAddressing()), 1));
}

void M6502::OPCode0x20()
{
    // JSR $nn
    u16 dest = AbsoluteAddressing();
    u16 pc = m_s.PC.GetValue();
    StackPush16(pc - 1);
    m_s.PC.SetValue(dest);

#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    PushCallStack(pc - 3, dest, pc);
#endif
}

void M6502::OPCode0x21()
{
    // AND (ZP,X)
    OPCodes_AND(MemRead8(ZeroPageIndexedIndirectAddressing()));
}

void M6502::OPCode0x22()
{
    UnofficialOPCode();
    m_s.PC.Increment();
}

void M6502::OPCode0x23()
{
    m_s.onebyte_un_nop = true;
    UnofficialOPCode();
}

void M6502::OPCode0x24()
{
    // BIT ZP
    OPCodes_BIT(ZeroPageAddressing());
}

void M6502::OPCode0x25()
{
    // AND ZP
    OPCodes_AND(MemRead8(ZeroPageAddressing()));
}

void M6502::OPCode0x26()
{
    // ROL ZP
    OPCodes_ROL_Memory(ZeroPageAddressing());
}

void M6502::OPCode0x27()
{
    // RMB2 ZP
    OPCodes_RMB(2, ZeroPageAddressing());
}

void M6502::OPCode0x28()
{
    // PLP
    m_s.P.SetValue(StackPop8());
    ClearFlag(FLAG_BREAK);
    SetFlag(FLAG_UNUSED);
}

void M6502::OPCode0x29()
{
    // AND #nn
    OPCodes_AND(ImmediateAddressing());
}

void M6502::OPCode0x2A()
{
    // ROL A
    OPCodes_ROL_Accumulator();
}

void M6502::OPCode0x2B()
{
    m_s.onebyte_un_nop = true;
    UnofficialOPCode();
}

void M6502::OPCode0x2C()
{
    // BIT hhll
    OPCodes_BIT(AbsoluteAddressing());
}

void M6502::OPCode0x2D()
{
    // AND hhll
    OPCodes_AND(MemRead8(AbsoluteAddressing()));
}

void M6502::OPCode0x2E()
{
    // ROL hhll
    OPCodes_ROL_Memory(AbsoluteAddressing());
}

void M6502::OPCode0x2F()
{
    // BBR2 ZP,rr
    OPcodes_Branch(IS_NOT_SET_BIT(MemRead8(ZeroPageAddressing()), 2));
}

void M6502::OPCode0x30()
{
    // BMI rr
    OPcodes_Branch(IsSetFlag(FLAG_NEGATIVE));
}

void M6502::OPCode0x31()
{
    // AND (ZP),Y
    OPCodes_AND(MemRead8(ZeroPageIndirectIndexedAddressing()));
}

void M6502::OPCode0x32()
{
    // AND (ZP)
    OPCodes_AND(MemRead8(ZeroPageIndirectAddressing()));
}

void M6502::OPCode0x33()
{
    m_s.onebyte_un_nop = true;
    UnofficialOPCode();
}

void M6502::OPCode0x34()
{
    // BIT ZP,X
    OPCodes_BIT(ZeroPageAddressing(&m_s.X));
}

void M6502::OPCode0x35()
{
    // AND ZP,X
    OPCodes_AND(MemRead8(ZeroPageAddressing(&m_s.X)));
}

void M6502::OPCode0x36()
{
    // ROL ZP,X
    OPCodes_ROL_Memory(ZeroPageAddressing(&m_s.X));
}

void M6502::OPCode0x37()
{
    // RMB3 ZP
    OPCodes_RMB(3, ZeroPageAddressing());
}

void M6502::OPCode0x38()
{
    // SEC
    SetFlag(FLAG_CARRY);
}

void M6502::OPCode0x39()
{
    // AND hhll,Y
    OPCodes_AND(MemRead8(AbsoluteAddressing(&m_s.Y)));
}

void M6502::OPCode0x3A()
{
    // DEC A
    OPCodes_DEC_Reg(&m_s.A);
}

void M6502::OPCode0x3B()
{
    m_s.onebyte_un_nop = true;
    UnofficialOPCode();
}

void M6502::OPCode0x3C()
{
    // BIT hhll,X
    OPCodes_BIT(AbsoluteAddressing(&m_s.X));
}

void M6502::OPCode0x3D()
{
    // AND hhll,X
    OPCodes_AND(MemRead8(AbsoluteAddressing(&m_s.X)));
}

void M6502::OPCode0x3E()
{
    // ROL hhll,X
    OPCodes_ROL_Memory(AbsoluteAddressing(&m_s.X));
}

void M6502::OPCode0x3F()
{
    // BBR3 ZP,rr
    OPcodes_Branch(IS_NOT_SET_BIT(MemRead8(ZeroPageAddressing()), 3));
}

void M6502::OPCode0x40()
{
    // RTI
    m_s.P.SetValue(StackPop8());
    m_s.PC.SetValue(StackPop16());
    ClearFlag(FLAG_BREAK);
    SetFlag(FLAG_UNUSED);
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    PopCallStack();
#endif
}

void M6502::OPCode0x41()
{
    // EOR (ZP,X)
    OPCodes_EOR(MemRead8(ZeroPageIndexedIndirectAddressing()));
}

void M6502::OPCode0x42()
{
    UnofficialOPCode();
    m_s.PC.Increment();
}

void M6502::OPCode0x43()
{
    m_s.onebyte_un_nop = true;
    UnofficialOPCode();
}

void M6502::OPCode0x44()
{
    UnofficialOPCode();
    m_s.PC.Increment();
}

void M6502::OPCode0x45()
{
    // EOR ZP
    OPCodes_EOR(MemRead8(ZeroPageAddressing()));
}

void M6502::OPCode0x46()
{
    // LSR ZP
    OPCodes_LSR_Memory(ZeroPageAddressing());
}

void M6502::OPCode0x47()
{
    // RMB4 ZP
    OPCodes_RMB(4, ZeroPageAddressing());
}

void M6502::OPCode0x48()
{
    // PHA
    StackPush8(m_s.A.GetValue());
}

void M6502::OPCode0x49()
{
    // EOR #nn
    OPCodes_EOR(ImmediateAddressing());
}

void M6502::OPCode0x4A()
{
    // LSR A
    OPCodes_LSR_Accumulator();
}

void M6502::OPCode0x4B()
{
    m_s.onebyte_un_nop = true;
    UnofficialOPCode();
}

void M6502::OPCode0x4C()
{
    // JMP hhll
    m_s.PC.SetValue(AbsoluteAddressing());
}

void M6502::OPCode0x4D()
{
    // EOR hhll
    OPCodes_EOR(MemRead8(AbsoluteAddressing()));
}

void M6502::OPCode0x4E()
{
    // LSR hhll
    OPCodes_LSR_Memory(AbsoluteAddressing());
}

void M6502::OPCode0x4F()
{
    // BBR4 ZP,rr
    OPcodes_Branch(IS_NOT_SET_BIT(MemRead8(ZeroPageAddressing()), 4));
}

void M6502::OPCode0x50()
{
    // BVC rr
    OPcodes_Branch(IsNotSetFlag(FLAG_OVERFLOW));
}

void M6502::OPCode0x51()
{
    // EOR (ZP),Y
    OPCodes_EOR(MemRead8(ZeroPageIndirectIndexedAddressing()));
}

void M6502::OPCode0x52()
{
    // EOR (ZP)
    OPCodes_EOR(MemRead8(ZeroPageIndirectAddressing()));
}

void M6502::OPCode0x53()
{
    m_s.onebyte_un_nop = true;
    UnofficialOPCode();
}

void M6502::OPCode0x54()
{
    UnofficialOPCode();
    m_s.PC.Increment();
}

void M6502::OPCode0x55()
{
    // EOR ZP,X
    OPCodes_EOR(MemRead8(ZeroPageAddressing(&m_s.X)));
}

void M6502::OPCode0x56()
{
    // LSR ZP,X
    OPCodes_LSR_Memory(ZeroPageAddressing(&m_s.X));
}

void M6502::OPCode0x57()
{
    // RMB5 ZP
    OPCodes_RMB(5, ZeroPageAddressing());
}

void M6502::OPCode0x58()
{
    // CLI
    ClearFlag(FLAG_INTERRUPT);
}

void M6502::OPCode0x59()
{
    // EOR hhll,Y
    OPCodes_EOR(MemRead8(AbsoluteAddressing(&m_s.Y)));
}

void M6502::OPCode0x5A()
{
    // PHY
    StackPush8(m_s.Y.GetValue());
}

void M6502::OPCode0x5B()
{
    m_s.onebyte_un_nop = true;
    UnofficialOPCode();
}

void M6502::OPCode0x5C()
{
    UnofficialOPCode();
    m_s.PC.Increment(2);
}

void M6502::OPCode0x5D()
{
    // EOR hhll,X
    OPCodes_EOR(MemRead8(AbsoluteAddressing(&m_s.X)));
}

void M6502::OPCode0x5E()
{
    // LSR hhll,X
    OPCodes_LSR_Memory(AbsoluteAddressing(&m_s.X));
}

void M6502::OPCode0x5F()
{
    // BBR5 ZP,r
    OPcodes_Branch(IS_NOT_SET_BIT(MemRead8(ZeroPageAddressing()), 5));
}

void M6502::OPCode0x60()
{
    // RTS
    m_s.PC.SetValue(StackPop16() + 1);
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    PopCallStack();
#endif
}

void M6502::OPCode0x61()
{
    // ADC (ZP,X)
    OPCodes_ADC(MemRead8(ZeroPageIndexedIndirectAddressing()));
}

void M6502::OPCode0x62()
{
    UnofficialOPCode();
    m_s.PC.Increment();
}

void M6502::OPCode0x63()
{
    m_s.onebyte_un_nop = true;
    UnofficialOPCode();
}

void M6502::OPCode0x64()
{
    // STZ ZP
    OPCodes_STZ(ZeroPageAddressing());
}

void M6502::OPCode0x65()
{
    // ADC ZP
    OPCodes_ADC(MemRead8(ZeroPageAddressing()));
}

void M6502::OPCode0x66()
{
    // ROR ZP
    OPCodes_ROR_Memory(ZeroPageAddressing());
}

void M6502::OPCode0x67()
{
    // RMB6 ZP
    OPCodes_RMB(6, ZeroPageAddressing());
}

void M6502::OPCode0x68()
{
    // PLA
    u8 result = StackPop8();
    m_s.A.SetValue(result);
    SetOrClearZNFlags(result);
}

void M6502::OPCode0x69()
{
    // ADC #nn
    OPCodes_ADC(ImmediateAddressing());
}

void M6502::OPCode0x6A()
{
    // ROR A
    OPCodes_ROR_Accumulator();
}

void M6502::OPCode0x6B()
{
    m_s.onebyte_un_nop = true;
    UnofficialOPCode();
}

void M6502::OPCode0x6C()
{
    // JMP (hhll)
    m_s.PC.SetValue(AbsoluteIndirectAddressing());
}

void M6502::OPCode0x6D()
{
    // ADC (hhll)
    OPCodes_ADC(MemRead8(AbsoluteAddressing()));
}

void M6502::OPCode0x6E()
{
    // ROR hhll
    OPCodes_ROR_Memory(AbsoluteAddressing());
}

void M6502::OPCode0x6F()
{
    // BBR6 ZP,rr
    OPcodes_Branch(IS_NOT_SET_BIT(MemRead8(ZeroPageAddressing()), 6));
}

void M6502::OPCode0x70()
{
    // BVS rr
    OPcodes_Branch(IsSetFlag(FLAG_OVERFLOW));
}

void M6502::OPCode0x71()
{
    // ADC (ZP),Y
    OPCodes_ADC(MemRead8(ZeroPageIndirectIndexedAddressing()));
}

void M6502::OPCode0x72()
{
    // ADC (ZP)
    OPCodes_ADC(MemRead8(ZeroPageIndirectAddressing()));
}

void M6502::OPCode0x73()
{
    m_s.onebyte_un_nop = true;
    UnofficialOPCode();
}

void M6502::OPCode0x74()
{
    // STZ ZP,X
    OPCodes_STZ(ZeroPageAddressing(&m_s.X));
}

void M6502::OPCode0x75()
{
    // ADC ZP,X
    OPCodes_ADC(MemRead8(ZeroPageAddressing(&m_s.X)));
}

void M6502::OPCode0x76()
{
    // ROR ZP,X
    OPCodes_ROR_Memory(ZeroPageAddressing(&m_s.X));
}

void M6502::OPCode0x77()
{
    // RMB7 ZP
    OPCodes_RMB(7, ZeroPageAddressing());
}

void M6502::OPCode0x78()
{
    // SEI
    SetFlag(FLAG_INTERRUPT);
}

void M6502::OPCode0x79()
{
    // ADC hhll,Y
    OPCodes_ADC(MemRead8(AbsoluteAddressing(&m_s.Y)));
}

void M6502::OPCode0x7A()
{
    // PLY
    u8 result = StackPop8();
    m_s.Y.SetValue(result);
    SetOrClearZNFlags(result);
}

void M6502::OPCode0x7B()
{
    m_s.onebyte_un_nop = true;
    UnofficialOPCode();
}

void M6502::OPCode0x7C()
{
    // JMP (hhll,X)
    m_s.PC.SetValue(AbsoluteIndexedIndirectAddressing());
}

void M6502::OPCode0x7D()
{
    // ADC hhll,X
    OPCodes_ADC(MemRead8(AbsoluteAddressing(&m_s.X)));
}

void M6502::OPCode0x7E()
{
    // ROR hhll,X
    OPCodes_ROR_Memory(AbsoluteAddressing(&m_s.X));
}

void M6502::OPCode0x7F()
{
    // BBR7 ZP,rr
    OPcodes_Branch(IS_NOT_SET_BIT(MemRead8(ZeroPageAddressing()), 7));
}

void M6502::OPCode0x80()
{
    // BRA rr
    OPcodes_Branch(true);
}

void M6502::OPCode0x81()
{
    // STA (ZP,X)
    OPCodes_Store(&m_s.A, ZeroPageIndexedIndirectAddressing());
}

void M6502::OPCode0x82()
{
    UnofficialOPCode();
    m_s.PC.Increment();
}

void M6502::OPCode0x83()
{
    m_s.onebyte_un_nop = true;
    UnofficialOPCode();
}

void M6502::OPCode0x84()
{
    // STY ZP
    OPCodes_Store(&m_s.Y, ZeroPageAddressing());
}

void M6502::OPCode0x85()
{
    // STA ZP
    OPCodes_Store(&m_s.A, ZeroPageAddressing());
}

void M6502::OPCode0x86()
{
    // STX ZP
    OPCodes_Store(&m_s.X, ZeroPageAddressing());
}

void M6502::OPCode0x87()
{
    // SMB0 ZP
    OPCodes_SMB(0, ZeroPageAddressing());
}

void M6502::OPCode0x88()
{
    // DEY
    OPCodes_DEC_Reg(&m_s.Y);
}

void M6502::OPCode0x89()
{
    // BIT #nn
    OPCodes_BIT_Immediate(m_s.PC.GetValue());
    m_s.PC.Increment();
}

void M6502::OPCode0x8A()
{
    // TXA
    OPCodes_Transfer(&m_s.X, &m_s.A);
}

void M6502::OPCode0x8B()
{
    m_s.onebyte_un_nop = true;
    UnofficialOPCode();
}

void M6502::OPCode0x8C()
{
    // STY hhll
    OPCodes_Store(&m_s.Y, AbsoluteAddressing());
}

void M6502::OPCode0x8D()
{
    // STA hhll
    OPCodes_Store(&m_s.A, AbsoluteAddressing());
}

void M6502::OPCode0x8E()
{
    // STX hhll
    OPCodes_Store(&m_s.X, AbsoluteAddressing());
}

void M6502::OPCode0x8F()
{
    // BBS0 ZP,rr
    OPcodes_Branch(IS_SET_BIT(MemRead8(ZeroPageAddressing()), 0));
}

void M6502::OPCode0x90()
{
    // BCC rr
    OPcodes_Branch(IsNotSetFlag(FLAG_CARRY));
}

void M6502::OPCode0x91()
{
    // STA (ZP),Y
    OPCodes_Store(&m_s.A, ZeroPageIndirectIndexedAddressing());
}

void M6502::OPCode0x92()
{
    // STA (ZP)
    OPCodes_Store(&m_s.A, ZeroPageIndirectAddressing());
}

void M6502::OPCode0x93()
{
    m_s.onebyte_un_nop = true;
    UnofficialOPCode();
}

void M6502::OPCode0x94()
{
    // STY ZP,X
    OPCodes_Store(&m_s.Y, ZeroPageAddressing(&m_s.X));
}

void M6502::OPCode0x95()
{
    // STA ZP,X
    OPCodes_Store(&m_s.A, ZeroPageAddressing(&m_s.X));
}

void M6502::OPCode0x96()
{
    // STX ZP,Y
    OPCodes_Store(&m_s.X, ZeroPageAddressing(&m_s.Y));
}

void M6502::OPCode0x97()
{
    // SMB1 ZP
    OPCodes_SMB(1, ZeroPageAddressing());
}

void M6502::OPCode0x98()
{
    // TYA
    OPCodes_Transfer(&m_s.Y, &m_s.A);
}

void M6502::OPCode0x99()
{
    // STA hhll,Y
    OPCodes_Store(&m_s.A, AbsoluteAddressing(&m_s.Y));
}

void M6502::OPCode0x9A()
{
    // TXS
    m_s.S.SetValue(m_s.X.GetValue());
}

void M6502::OPCode0x9B()
{
    m_s.onebyte_un_nop = true;
    UnofficialOPCode();
}

void M6502::OPCode0x9C()
{
    // STZ hhll
    OPCodes_STZ(AbsoluteAddressing());
}

void M6502::OPCode0x9D()
{
    // STA hhll,X
    OPCodes_Store(&m_s.A, AbsoluteAddressing(&m_s.X));
}

void M6502::OPCode0x9E()
{
    // STZ hhll,X
    OPCodes_STZ(AbsoluteAddressing(&m_s.X));
}

void M6502::OPCode0x9F()
{
    // BBS1 ZP,rr
    OPcodes_Branch(IS_SET_BIT(MemRead8(ZeroPageAddressing()), 1));
}

void M6502::OPCode0xA0()
{
    // LDY #nn
    OPCodes_LD(&m_s.Y, ImmediateAddressing());
}

void M6502::OPCode0xA1()
{
    // LDA $(ZP,X)
    OPCodes_LD(&m_s.A, MemRead8(ZeroPageIndexedIndirectAddressing()));
}

void M6502::OPCode0xA2()
{
    // LDX #nn
    OPCodes_LD(&m_s.X, ImmediateAddressing());
}

void M6502::OPCode0xA3()
{
    m_s.onebyte_un_nop = true;
    UnofficialOPCode();
}

void M6502::OPCode0xA4()
{
    // LDY ZP
    OPCodes_LD(&m_s.Y, MemRead8(ZeroPageAddressing()));
}

void M6502::OPCode0xA5()
{
    // LDA ZP
    OPCodes_LD(&m_s.A, MemRead8(ZeroPageAddressing()));
}

void M6502::OPCode0xA6()
{
    // LDX ZP
    OPCodes_LD(&m_s.X, MemRead8(ZeroPageAddressing()));
}

void M6502::OPCode0xA7()
{
    // SMB2 ZP
    OPCodes_SMB(2, ZeroPageAddressing());
}

void M6502::OPCode0xA8()
{
    // TAY
    OPCodes_Transfer(&m_s.A, &m_s.Y);
}

void M6502::OPCode0xA9()
{
    // LDA #nn
    OPCodes_LD(&m_s.A, ImmediateAddressing());
}

void M6502::OPCode0xAA()
{
    // TAX
    OPCodes_Transfer(&m_s.A, &m_s.X);
}

void M6502::OPCode0xAB()
{
    m_s.onebyte_un_nop = true;
    UnofficialOPCode();
}

void M6502::OPCode0xAC()
{
    // LDY hhll
    OPCodes_LD(&m_s.Y, MemRead8(AbsoluteAddressing()));
}

void M6502::OPCode0xAD()
{
    // LDA hhll
    OPCodes_LD(&m_s.A, MemRead8(AbsoluteAddressing()));
}

void M6502::OPCode0xAE()
{
    // LDX hhll
    OPCodes_LD(&m_s.X, MemRead8(AbsoluteAddressing()));
}

void M6502::OPCode0xAF()
{
    // BBS2 ZP,rr
    OPcodes_Branch(IS_SET_BIT(MemRead8(ZeroPageAddressing()), 2));
}

void M6502::OPCode0xB0()
{
    // BCS rr
    OPcodes_Branch(IsSetFlag(FLAG_CARRY));
}

void M6502::OPCode0xB1()
{
    // LDA ($n),Y
    OPCodes_LD(&m_s.A, MemRead8(ZeroPageIndirectIndexedAddressing()));
}

void M6502::OPCode0xB2()
{
    // LDA (ZP)
    OPCodes_LD(&m_s.A, MemRead8(ZeroPageIndirectAddressing()));
}

void M6502::OPCode0xB3()
{
    m_s.onebyte_un_nop = true;
    UnofficialOPCode();
}

void M6502::OPCode0xB4()
{
    // LDY ZP,X
    OPCodes_LD(&m_s.Y, MemRead8(ZeroPageAddressing(&m_s.X)));
}

void M6502::OPCode0xB5()
{
    // LDA ZP,X
    OPCodes_LD(&m_s.A, MemRead8(ZeroPageAddressing(&m_s.X)));
}

void M6502::OPCode0xB6()
{
    // LDX ZP,Y
    OPCodes_LD(&m_s.X, MemRead8(ZeroPageAddressing(&m_s.Y)));
}

void M6502::OPCode0xB7()
{
    // SMB3 ZP
    OPCodes_SMB(3, ZeroPageAddressing());
}

void M6502::OPCode0xB8()
{
    // CLV
    ClearFlag(FLAG_OVERFLOW);
}

void M6502::OPCode0xB9()
{
    // LDA hhll,Y
    OPCodes_LD(&m_s.A, MemRead8(AbsoluteAddressing(&m_s.Y)));
}

void M6502::OPCode0xBA()
{
    // TSX
    OPCodes_Transfer(&m_s.S, &m_s.X);
}

void M6502::OPCode0xBB()
{
    m_s.onebyte_un_nop = true;
    UnofficialOPCode();
}

void M6502::OPCode0xBC()
{
    // LDY hhll,X
    OPCodes_LD(&m_s.Y, MemRead8(AbsoluteAddressing(&m_s.X)));
}

void M6502::OPCode0xBD()
{
    // LDA hhll,X
    OPCodes_LD(&m_s.A, MemRead8(AbsoluteAddressing(&m_s.X)));
}

void M6502::OPCode0xBE()
{
    // LDX hhll,Y
    OPCodes_LD(&m_s.X, MemRead8(AbsoluteAddressing(&m_s.Y)));
}

void M6502::OPCode0xBF()
{
    // BBS3 ZP,rr
    OPcodes_Branch(IS_SET_BIT(MemRead8(ZeroPageAddressing()), 3));
}

void M6502::OPCode0xC0()
{
    // CPY #nn
    OPCodes_CMP(&m_s.Y, ImmediateAddressing());
}

void M6502::OPCode0xC1()
{
    // CMP (ZP,X)
    OPCodes_CMP(&m_s.A, MemRead8(ZeroPageIndexedIndirectAddressing()));
}

void M6502::OPCode0xC2()
{
    UnofficialOPCode();
    m_s.PC.Increment();
}

void M6502::OPCode0xC3()
{
    m_s.onebyte_un_nop = true;
    UnofficialOPCode();
}

void M6502::OPCode0xC4()
{
    // CPY ZP
    OPCodes_CMP(&m_s.Y, MemRead8(ZeroPageAddressing()));
}

void M6502::OPCode0xC5()
{
    // CMP ZP
    OPCodes_CMP(&m_s.A, MemRead8(ZeroPageAddressing()));
}

void M6502::OPCode0xC6()
{
    // DEC ZP
    OPCodes_DEC_Mem(ZeroPageAddressing());
}

void M6502::OPCode0xC7()
{
    // SMB4 ZP
    OPCodes_SMB(4, ZeroPageAddressing());
}

void M6502::OPCode0xC8()
{
    // INY
    OPCodes_INC_Reg(&m_s.Y);
}

void M6502::OPCode0xC9()
{
    // CMP #nn
    OPCodes_CMP(&m_s.A, ImmediateAddressing());
}

void M6502::OPCode0xCA()
{
    // DEX
    OPCodes_DEC_Reg(&m_s.X);
}

void M6502::OPCode0xCB()
{
    m_s.onebyte_un_nop = true;
    UnofficialOPCode();
}

void M6502::OPCode0xCC()
{
    // CPY hhll
    OPCodes_CMP(&m_s.Y, MemRead8(AbsoluteAddressing()));
}

void M6502::OPCode0xCD()
{
    // CMP hhll
    OPCodes_CMP(&m_s.A, MemRead8(AbsoluteAddressing()));
}

void M6502::OPCode0xCE()
{
    // DEC hhll
    OPCodes_DEC_Mem(AbsoluteAddressing());
}

void M6502::OPCode0xCF()
{
    // BBS4 ZP,rr
    OPcodes_Branch(IS_SET_BIT(MemRead8(ZeroPageAddressing()), 4));
}

void M6502::OPCode0xD0()
{
    // BNE rr
    OPcodes_Branch(IsNotSetFlag(FLAG_ZERO));
}

void M6502::OPCode0xD1()
{
    // CMP (ZP),Y
    OPCodes_CMP(&m_s.A, MemRead8(ZeroPageIndirectIndexedAddressing()));
}

void M6502::OPCode0xD2()
{
    // CMP (ZP)
    OPCodes_CMP(&m_s.A, MemRead8(ZeroPageIndirectAddressing()));
}

void M6502::OPCode0xD3()
{
    m_s.onebyte_un_nop = true;
    UnofficialOPCode();
}

void M6502::OPCode0xD4()
{
    UnofficialOPCode();
    m_s.PC.Increment();
}

void M6502::OPCode0xD5()
{
    // CMP ZP,X
    OPCodes_CMP(&m_s.A, MemRead8(ZeroPageAddressing(&m_s.X)));
}

void M6502::OPCode0xD6()
{
    // DEC ZP,X
    OPCodes_DEC_Mem(ZeroPageAddressing(&m_s.X));
}

void M6502::OPCode0xD7()
{
    // SMB5 ZP
    OPCodes_SMB(5, ZeroPageAddressing());
}

void M6502::OPCode0xD8()
{
    // CLD
    ClearFlag(FLAG_DECIMAL);
}

void M6502::OPCode0xD9()
{
    // CMP $nn,Y
    OPCodes_CMP(&m_s.A, MemRead8(AbsoluteAddressing(&m_s.Y)));
}

void M6502::OPCode0xDA()
{
    // PHX
    StackPush8(m_s.X.GetValue());
}

void M6502::OPCode0xDB()
{
    m_s.onebyte_un_nop = true;
    UnofficialOPCode();
}

void M6502::OPCode0xDC()
{
    UnofficialOPCode();
    m_s.PC.Increment(2);
}

void M6502::OPCode0xDD()
{
    // CMP hhll,X
    OPCodes_CMP(&m_s.A, MemRead8(AbsoluteAddressing(&m_s.X)));
}

void M6502::OPCode0xDE()
{
    // DEC hhll,X
    OPCodes_DEC_Mem(AbsoluteAddressing(&m_s.X));
}

void M6502::OPCode0xDF()
{
    // BBS5 ZP,rr
    OPcodes_Branch(IS_SET_BIT(MemRead8(ZeroPageAddressing()), 5));
}

void M6502::OPCode0xE0()
{
    // CPX #nn
    OPCodes_CMP(&m_s.X, ImmediateAddressing());
}

void M6502::OPCode0xE1()
{
    // SBC $(ZP,X)
    OPCodes_SBC(MemRead8(ZeroPageIndexedIndirectAddressing()));
}

void M6502::OPCode0xE2()
{
    UnofficialOPCode();
    m_s.PC.Increment();
}

void M6502::OPCode0xE3()
{
    m_s.onebyte_un_nop = true;
    UnofficialOPCode();
}

void M6502::OPCode0xE4()
{
    // CPX ZP
    OPCodes_CMP(&m_s.X, MemRead8(ZeroPageAddressing()));
}

void M6502::OPCode0xE5()
{
    // SBC ZP
    OPCodes_SBC(MemRead8(ZeroPageAddressing()));
}

void M6502::OPCode0xE6()
{
    // INC ZP
    OPCodes_INC_Mem(ZeroPageAddressing());
}

void M6502::OPCode0xE7()
{
    // SMB6 ZP
    OPCodes_SMB(6, ZeroPageAddressing());
}

void M6502::OPCode0xE8()
{
    // INX
    OPCodes_INC_Reg(&m_s.X);
}

void M6502::OPCode0xE9()
{
    // SBC #nn
    OPCodes_SBC(ImmediateAddressing());
}

void M6502::OPCode0xEA()
{
    // NOP
}

void M6502::OPCode0xEB()
{
    m_s.onebyte_un_nop = true;
    UnofficialOPCode();
}

void M6502::OPCode0xEC()
{
    // CPX hhll
    OPCodes_CMP(&m_s.X, MemRead8(AbsoluteAddressing()));
}

void M6502::OPCode0xED()
{
    // SBC hhll
    OPCodes_SBC(MemRead8(AbsoluteAddressing()));
}

void M6502::OPCode0xEE()
{
    // INC hhll
    OPCodes_INC_Mem(AbsoluteAddressing());
}

void M6502::OPCode0xEF()
{
    // BBS6 ZP,rr
    OPcodes_Branch(IS_SET_BIT(MemRead8(ZeroPageAddressing()), 6));
}

void M6502::OPCode0xF0()
{
    // BEQ rr
    OPcodes_Branch(IsSetFlag(FLAG_ZERO));
}

void M6502::OPCode0xF1()
{
    // SBC (ZP),Y
    OPCodes_SBC(MemRead8(ZeroPageIndirectIndexedAddressing()));
}

void M6502::OPCode0xF2()
{
    // SBC (ZP)
    OPCodes_SBC(MemRead8(ZeroPageIndirectAddressing()));
}

void M6502::OPCode0xF3()
{
    m_s.onebyte_un_nop = true;
    UnofficialOPCode();
}

void M6502::OPCode0xF4()
{
    UnofficialOPCode();
    m_s.PC.Increment();
}

void M6502::OPCode0xF5()
{
    // SBC ZP,X
    OPCodes_SBC(MemRead8(ZeroPageAddressing(&m_s.X)));
}

void M6502::OPCode0xF6()
{
    // INC ZP,X
    OPCodes_INC_Mem(ZeroPageAddressing(&m_s.X));
}

void M6502::OPCode0xF7()
{
    // SMB7 ZP
    OPCodes_SMB(7, ZeroPageAddressing());
}

void M6502::OPCode0xF8()
{
    // SED
    SetFlag(FLAG_DECIMAL);
}

void M6502::OPCode0xF9()
{
    // SBC hhll,Y
    OPCodes_SBC(MemRead8(AbsoluteAddressing(&m_s.Y)));
}

void M6502::OPCode0xFA()
{
    // PLX
    u8 result = StackPop8();
    m_s.X.SetValue(result);
    SetOrClearZNFlags(result);
}

void M6502::OPCode0xFB()
{
    m_s.onebyte_un_nop = true;
    UnofficialOPCode();
}

void M6502::OPCode0xFC()
{
    UnofficialOPCode();
    m_s.PC.Increment(2);
}

void M6502::OPCode0xFD()
{
    // SBC hhll,X
    OPCodes_SBC(MemRead8(AbsoluteAddressing(&m_s.X)));
}

void M6502::OPCode0xFE()
{
    // INC hhll,X
    OPCodes_INC_Mem(AbsoluteAddressing(&m_s.X));
}

void M6502::OPCode0xFF()
{
    // BBS7 ZP,rr
    OPcodes_Branch(IS_SET_BIT(MemRead8(ZeroPageAddressing()), 7));
}

void M6502::OPCodes_LynxI_NOP()
{
    m_s.onebyte_un_nop = true;
    UnofficialOPCode();
}
