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

#ifndef SUZY_INLINE_H
#define SUZY_INLINE_H

#include "suzy.h"
#include "media.h"
#include "m6502.h"
#include "input.h"
#include "bus.h"
#include "trace_logger.h"

INLINE void Suzy::Clock(u32 cycles)
{
    if (m_state.fsm_phase != SUZY_PHASE_IDLE)
        StepBlitter(cycles);

    UpdateMath(cycles);
}

template<bool debug>
INLINE u8 Suzy::Read(u16 address)
{
    if (!debug)
    {
        m_bus->InjectCycles(k_bus_cycles_suzy_read);
    }

    u16 effective_addr = address;

    // Mirror math registers (FC40-FC6F) to sprite registers (FC00-FC2F)
    if (address >= 0xFC40 && address <= 0xFC6F)
    {
        effective_addr = address - 0x40;
    }

    // Open bus for unused ranges
    else if ((address >= 0xFC30 && address <= 0xFC3F) || (address >= 0xFC70 && address <= 0xFC7F))
    {
        return 0xFF;
    }

    switch(effective_addr)
    {
    case SUZY_TMPADRL:     // 0xFC00
        return m_state.TMPADR.low;
    case SUZY_TMPADRH:     // 0xFC01
        return m_state.TMPADR.high;
    case SUZY_TILTACUML:   // 0xFC02
        return m_state.TILTACUM.low;
    case SUZY_TILTACUMH:   // 0xFC03
        return m_state.TILTACUM.high;
    case SUZY_HOFFL:       // 0xFC04
        return m_state.HOFF.low;
    case SUZY_HOFFH:       // 0xFC05
        return m_state.HOFF.high;
    case SUZY_VOFFL:       // 0xFC06
        return m_state.VOFF.low;
    case SUZY_VOFFH:       // 0xFC07
        return m_state.VOFF.high;
    case SUZY_VIDBASL:     // 0xFC08
        return m_state.VIDBAS.low;   
    case SUZY_VIDBASH:     // 0xFC09
        return m_state.VIDBAS.high;
    case SUZY_COLLBASL:    // 0xFC0A
        return m_state.COLLBAS.low;
    case SUZY_COLLBASH:    // 0xFC0B
        return m_state.COLLBAS.high;
    case SUZY_VIDADRL:     // 0xFC0C
        return m_state.VIDADR.low;
    case SUZY_VIDADRH:     // 0xFC0D
        return m_state.VIDADR.high;
    case SUZY_COLLADRL:    // 0xFC0E
        return m_state.COLLADR.low;
    case SUZY_COLLADRH:    // 0xFC0F
        return m_state.COLLADR.high;
    case SUZY_SCBNEXTL:    // 0xFC10
        return m_state.SCBNEXT.low;
    case SUZY_SCBNEXTH:    // 0xFC11
        return m_state.SCBNEXT.high;
    case SUZY_SPRDLINEL:   // 0xFC12
        return m_state.SPRDLINE.low;
    case SUZY_SPRDLINEH:   // 0xFC13
        return m_state.SPRDLINE.high;
    case SUZY_HPOSSTRTL:   // 0xFC14
        return m_state.HPOSSTRT.low;
    case SUZY_HPOSSTRTH:   // 0xFC15
        return m_state.HPOSSTRT.high;
    case SUZY_VPOSSTRTL:   // 0xFC16
        return m_state.VPOSSTRT.low;
    case SUZY_VPOSSTRTH:   // 0xFC17
        return m_state.VPOSSTRT.high;
    case SUZY_SPRHSIZL:    // 0xFC18
        return m_state.SPRHSIZ.low;
    case SUZY_SPRHSIZH:    // 0xFC19
        return m_state.SPRHSIZ.high;
    case SUZY_SPRVSIZL:    // 0xFC1A
        return m_state.SPRVSIZ.low;
    case SUZY_SPRVSIZH:    // 0xFC1B
        return m_state.SPRVSIZ.high;
    case SUZY_STRETCHL:    // 0xFC1C
        return m_state.STRETCH.low;
    case SUZY_STRETCHH:    // 0xFC1D
        return m_state.STRETCH.high;
    case SUZY_TILTL:       // 0xFC1E
        return m_state.TILT.low;
    case SUZY_TILTH:       // 0xFC1F
        return m_state.TILT.high;
    case SUZY_SPRDOFFL:    // 0xFC20
        return m_state.SPRDOFF.low;
    case SUZY_SPRDOFFH:    // 0xFC21
        return m_state.SPRDOFF.high;
    case SUZY_SPRVPOSL:    // 0xFC22
        return m_state.SPRVPOS.low;
    case SUZY_SPRVPOSH:    // 0xFC23
        return m_state.SPRVPOS.high;
    case SUZY_COLLOFFL:    // 0xFC24
        return m_state.COLLOFF.low;
    case SUZY_COLLOFFH:    // 0xFC25
        return m_state.COLLOFF.high;
    case SUZY_VSIZACUML:   // 0xFC26
        return m_state.VSIZACUM.low;
    case SUZY_VSIZACUMH:   // 0xFC27
        return m_state.VSIZACUM.high;
    case SUZY_HSIZOFFL:    // 0xFC28
        return m_state.HSIZOFF.low;
    case SUZY_HSIZOFFH:    // 0xFC29
        return m_state.HSIZOFF.high;
    case SUZY_VSIZOFFL:    // 0xFC2A
        return m_state.VSIZOFF.low;
    case SUZY_VSIZOFFH:    // 0xFC2B
        return m_state.VSIZOFF.high;
    case SUZY_SCBADRL:     // 0xFC2C
        return m_state.SCBADR.low;
    case SUZY_SCBADRH:     // 0xFC2D
        return m_state.SCBADR.high;
    case SUZY_PROCADRL:    // 0xFC2E
        return m_state.PROCADR.low;
    case SUZY_PROCADRH:    // 0xFC2F
        return m_state.PROCADR.high;
    case SUZY_SPRCTL0:     // 0xFC80
        DebugSuzy("Reading write-only SPRCTL0: %02X", m_state.SPRCTL0);
        return 0xFF;
    case SUZY_SPRCTL1:     // 0xFC81
        DebugSuzy("Reading write-only SPRCTL1: %02X", m_state.SPRCTL1);
        return 0xFF;
    case SUZY_SPRCOLL:     // 0xFC82
        DebugSuzy("Reading write-only SPRCOLL: %02X", m_state.SPRCOLL);
        return 0xFF;
    case SUZY_SPRINIT:     // 0xFC83
        DebugSuzy("Reading write-only SPRINIT: %02X", m_state.SPRINIT);
        return 0xFF;
    case SUZY_SUZYHREV:    // 0xFC88
        return 0x01;
    case SUZY_SUZYSREV:    // 0xFC89
        return 0xFF;
    case SUZY_SUZYBUSEN:   // 0xFC90
        DebugSuzy("Reading write-only SUZYBUSEN: %02X", m_state.SUZYBUSEN);
        return 0xFF;
    case SUZY_SPRGO:       // 0xFC91
        DebugSuzy("Reading write-only SPRGO: %02X", m_state.SPRGO);
        return 0xFF;
    case SUZY_SPRSYS:      // 0xFC92
    {
        u8 ret = 0x00;
        ret |= (m_state.sprsys_spritesbusy ? 0x01 : 0x00);
        ret |= (m_state.sprsys_stopsprites ? 0x02 : 0x00);
        ret |= (m_state.sprsys_unsafe ? 0x04 : 0x00);
        ret |= (m_state.sprsys_lefthand ? 0x08 : 0x00);
        ret |= (m_state.sprsys_vstrech ? 0x10 : 0x00);
        ret |= (m_state.sprsys_lastcarrybit ? 0x20 : 0x00);
        ret |= (m_state.sprsys_mathbit ? 0x40 : 0x00);
        ret |= (m_state.sprsys_mathbusy ? 0x80 : 0x00);
        return ret;
    }
    case SUZY_JOYSTICK:    // 0xFCB0
    {
        u8 joy = m_input->ReadJoystick();
        DebugSuzy("Reading JOYSTICK: %02X", joy);
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
        if (m_trace_logger->IsEnabled(TRACE_SUZY_INPUT))
        {
            GLYNX_Trace_Entry e;
            e.type = TRACE_SUZY_INPUT;
            e.cycle = 0;
            e.input.value = joy;
            e.input.is_joystick = true;
            m_trace_logger->TraceLog(e);
        }
#endif
        return joy;
    }
    case SUZY_SWITCHES:    // 0xFCB1
    {
        u8 sw = m_input->ReadSwitches();
        DebugSuzy("Reading SWITCHES: %02X", sw);
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
        if (m_trace_logger->IsEnabled(TRACE_SUZY_INPUT))
        {
            GLYNX_Trace_Entry e;
            e.type = TRACE_SUZY_INPUT;
            e.cycle = 0;
            e.input.value = sw;
            e.input.is_joystick = false;
            m_trace_logger->TraceLog(e);
        }
#endif
        return sw;
    }
    case SUZY_RCART0:      // 0xFCB2
        //DebugSuzy("Reading RCART0");
        if (!debug)
        {
            m_bus->InjectCycles(k_bus_cycles_cart_read);
            if (m_media->GetAudin() && m_media->GetAudinValue())
                return m_media->ReadBank0A();
            else
                return m_media->ReadBank0();
        }
        else
        {
            if (m_media->GetAudin() && m_media->GetAudinValue())
                return m_media->PeekBank0A();
            else
                return m_media->PeekBank0();
        }
    case SUZY_RCART1:      // 0xFCB3
        DebugSuzy("Reading RCART1");
        if (!debug)
        {
            m_bus->InjectCycles(k_bus_cycles_cart_read);
            if (m_media->GetAudin() && m_media->GetAudinValue())
                return m_media->ReadBank1A();
            else
                return m_media->ReadBank1();
        }
        else
        {
            if (m_media->GetAudin() && m_media->GetAudinValue())
                return m_media->PeekBank1A();
            else
                return m_media->PeekBank1();
        }
    case SUZY_LEDS:        // 0xFCC0
        DebugSuzy("Reading LEDS (unused)");
        return 0xFF;
    case SUZY_PPORTSTAT:   // 0xFCC2
        DebugSuzy("Reading PPORTSTAT (unused)");
        return 0xFF;
    case SUZY_PPORTDATA:   // 0xFCC3
        DebugSuzy("Reading PPORTDATA (unused)");
        return 0xFF;
    case SUZY_HOWIE:       // 0xFCC4
        DebugSuzy("Reading HOWIE (unused)");
        return 0xFF;
    default:
        DebugSuzy("Register READ called with unknown address: %04X", address);
        return 0xFF;
    }

    return 0xFF;
}

template<bool debug>
INLINE void Suzy::Write(u16 address, u8 value)
{
    if (!debug)
    {
        m_bus->InjectCycles(k_bus_cycles_suzy_write);
    }

    if ((address >= 0xFC30 && address <= 0xFC3F) || (address >= 0xFC70 && address <= 0xFC7F))
    {
        return;
    }

    // Math registers (FC40-FC6F)
    if (address >= 0xFC40 && address <= 0xFC6F)
    {
        switch(address)
        {
        case SUZY_MATHD:       // 0xFC52
            REG_MATHD = value;
            REG_MATHC = 0;
            return;
        case SUZY_MATHC:       // 0xFC53
            REG_MATHC = value;
            if (m_state.sprsys_sign)
            {
                u16 cd = m_state.SPRDLINE.value;
                m_state.math_sign_C = MathIsNegative(cd);
                if (m_state.math_sign_C && cd != 0)
                    cd = (u16)(-((s16)cd));
                m_state.SPRDLINE.value = cd;
            }
            return;
        case SUZY_MATHB:       // 0xFC54
            REG_MATHB = value;
            REG_MATHA = 0;
            return;
        case SUZY_MATHA:       // 0xFC55
            REG_MATHA = value;
            if (m_state.sprsys_sign)
            {
                u16 ab = m_state.HPOSSTRT.value;
                m_state.math_sign_A = MathIsNegative(ab);
                if (m_state.math_sign_A && ab != 0)
                    ab = (u16)(-((s16)ab));
                m_state.HPOSSTRT.value = ab;
            }
            MathRunMultiply();
            return;
        case SUZY_MATHP:       // 0xFC56
            REG_MATHP = value;
            REG_MATHN = 0;
            return;
        case SUZY_MATHN:       // 0xFC57
            REG_MATHN = value;
            return;
        case SUZY_MATHH:       // 0xFC60
            REG_MATHH = value;
            REG_MATHG = 0;
            return;
        case SUZY_MATHG:       // 0xFC61
            REG_MATHG = value;
            return;
        case SUZY_MATHF:       // 0xFC62
            REG_MATHF = value;
            REG_MATHE = 0;
            return;
        case SUZY_MATHE:       // 0xFC63
            REG_MATHE = value;
            MathRunDivide();
            return;
        case SUZY_MATHM:       // 0xFC6C
            REG_MATHM = value;
            REG_MATHL = 0;
            m_state.sprsys_mathbit = false;
            return;
        case SUZY_MATHL:       // 0xFC6D
            REG_MATHL = value;
            return;
        case SUZY_MATHK:       // 0xFC6E
            REG_MATHK = value;
            REG_MATHJ = 0;
            return;
        case SUZY_MATHJ:       // 0xFC6F
            REG_MATHJ = value;
            return;
        default:
            // Other addresses that aren't math registers
            address -= 0x40;
            break;
        }
    }

    switch(address)
    {
    case SUZY_TMPADRL:     // 0xFC00
        m_state.TMPADR.value = value;
        break;
    case SUZY_TMPADRH:     // 0xFC01
        m_state.TMPADR.high = value;
        break;
    case SUZY_TILTACUML:   // 0xFC02
        m_state.TILTACUM.value = value;
        break;
    case SUZY_TILTACUMH:   // 0xFC03
        m_state.TILTACUM.high = value;
        break;
    case SUZY_HOFFL:       // 0xFC04
        m_state.HOFF.value = value;
        break;
    case SUZY_HOFFH:       // 0xFC05
        m_state.HOFF.high = value;
        break;
    case SUZY_VOFFL:       // 0xFC06
        m_state.VOFF.value = value;
        break;
    case SUZY_VOFFH:       // 0xFC07
        m_state.VOFF.high = value;
        break;
    case SUZY_VIDBASL:     // 0xFC08
        DebugSuzy("Setting VIDBAS low to %02X (was %04X)", value, m_state.VIDBAS.value);
        m_state.VIDBAS.value = value;
        DebugSuzy("VIDBAS = %04X", m_state.VIDBAS.value);
        break;
    case SUZY_VIDBASH:     // 0xFC09
        DebugSuzy("Setting VIDBAS high to %02X (was %04X)", value, m_state.VIDBAS.value);
        m_state.VIDBAS.high = value;
        DebugSuzy("VIDBAS = %04X", m_state.VIDBAS.value);
        break;
    case SUZY_COLLBASL:    // 0xFC0A
        m_state.COLLBAS.value = value;
        break;
    case SUZY_COLLBASH:    // 0xFC0B
        m_state.COLLBAS.high = value;
        break;
    case SUZY_VIDADRL:     // 0xFC0C
        DebugSuzy("Setting VIDADR low to %02X (was %04X)", value, m_state.VIDADR.value);
        m_state.VIDADR.value = value;
        DebugSuzy("VIDADR = %04X", m_state.VIDADR.value);
        break;
    case SUZY_VIDADRH:     // 0xFC0D
        DebugSuzy("Setting VIDADR high to %02X (was %04X)", value, m_state.VIDADR.value);
        m_state.VIDADR.high = value;
        DebugSuzy("VIDADR = %04X", m_state.VIDADR.value);
        break;
    case SUZY_COLLADRL:    // 0xFC0E
        m_state.COLLADR.value = value;
        break;
    case SUZY_COLLADRH:    // 0xFC0F
        m_state.COLLADR.high = value;
        break;
    case SUZY_SCBNEXTL:    // 0xFC10
        m_state.SCBNEXT.value = value;
        break;
    case SUZY_SCBNEXTH:    // 0xFC11
        m_state.SCBNEXT.high = value;
        break;
    case SUZY_SPRDLINEL:   // 0xFC12
        m_state.SPRDLINE.value = value;
        break;
    case SUZY_SPRDLINEH:   // 0xFC13
        m_state.SPRDLINE.high = value;
        break;
    case SUZY_HPOSSTRTL:   // 0xFC14
        m_state.HPOSSTRT.value = value;
        break;
    case SUZY_HPOSSTRTH:   // 0xFC15
        m_state.HPOSSTRT.high = value;
        break;
    case SUZY_VPOSSTRTL:   // 0xFC16
        m_state.VPOSSTRT.value = value;
        break;
    case SUZY_VPOSSTRTH:   // 0xFC17
        m_state.VPOSSTRT.high = value;
        break;
    case SUZY_SPRHSIZL:    // 0xFC18
        DebugSuzy("Setting SPRHSIZ low to %02X (was %04X)", value, m_state.SPRHSIZ.value);
        m_state.SPRHSIZ.value = value;
        break;
    case SUZY_SPRHSIZH:    // 0xFC19
        DebugSuzy("Setting SPRHSIZ high to %02X (was %04X)", value, m_state.SPRHSIZ.value);
        m_state.SPRHSIZ.high = value;
        break;
    case SUZY_SPRVSIZL:    // 0xFC1A
        DebugSuzy("Setting SPRVSIZ low to %02X (was %04X)", value, m_state.SPRVSIZ.value);
        m_state.SPRVSIZ.value = value;
        break;
    case SUZY_SPRVSIZH:    // 0xFC1B
        DebugSuzy("Setting SPRVSIZ high to %02X (was %04X)", value, m_state.SPRVSIZ.value);
        m_state.SPRVSIZ.high = value;
        break;
    case SUZY_STRETCHL:    // 0xFC1C
        m_state.STRETCH.value = value;
        break;
    case SUZY_STRETCHH:    // 0xFC1D
        m_state.STRETCH.high = value;
        break;
    case SUZY_TILTL:       // 0xFC1E
        m_state.TILT.value = value;
        break;
    case SUZY_TILTH:       // 0xFC1F
        m_state.TILT.high = value;
        break;
    case SUZY_SPRDOFFL:    // 0xFC20
        m_state.SPRDOFF.value = value;
        break;
    case SUZY_SPRDOFFH:    // 0xFC21
        m_state.SPRDOFF.high = value;
        break;
    case SUZY_SPRVPOSL:    // 0xFC22
        m_state.SPRVPOS.value = value;
        break;
    case SUZY_SPRVPOSH:    // 0xFC23
        m_state.SPRVPOS.high = value;
        break;
    case SUZY_COLLOFFL:    // 0xFC24
        m_state.COLLOFF.value = value;
        break;
    case SUZY_COLLOFFH:    // 0xFC25
        m_state.COLLOFF.high = value;
        break;
    case SUZY_VSIZACUML:   // 0xFC26
        m_state.VSIZACUM.value = value;
        break;
    case SUZY_VSIZACUMH:   // 0xFC27
        m_state.VSIZACUM.high = value;
        break;
    case SUZY_HSIZOFFL:    // 0xFC28
        m_state.HSIZOFF.value = value;
        break;
    case SUZY_HSIZOFFH:    // 0xFC29
        m_state.HSIZOFF.high = value;
        break;
    case SUZY_VSIZOFFL:    // 0xFC2A
        m_state.VSIZOFF.value = value;
        break;
    case SUZY_VSIZOFFH:    // 0xFC2B
        m_state.VSIZOFF.high = value;
        break;
    case SUZY_SCBADRL:     // 0xFC2C
        m_state.SCBADR.value = value;
        break;
    case SUZY_SCBADRH:     // 0xFC2D
        m_state.SCBADR.high = value;
        break;
    case SUZY_PROCADRL:    // 0xFC2E
        m_state.PROCADR.value = value;
        break;
    case SUZY_PROCADRH:    // 0xFC2F
        m_state.PROCADR.high = value;
        break;
    case SUZY_SPRCTL0:     // 0xFC80
        DebugSuzy("Setting SPRCTL0 to %02X (was %02X)", value, m_state.SPRCTL0);
        m_state.SPRCTL0 = value;
        break;
    case SUZY_SPRCTL1:     // 0xFC81
        DebugSuzy("Setting SPRCTL1 to %02X (was %02X)", value, m_state.SPRCTL1);
        m_state.SPRCTL1 = value;
        break;
    case SUZY_SPRCOLL:     // 0xFC82
        DebugSuzy("Setting SPRCOLL to %02X (was %02X)", value, m_state.SPRCOLL);
        m_state.SPRCOLL = value;
        break;
    case SUZY_SPRINIT:     // 0xFC83
        DebugSuzy("Setting SPRINIT to %02X (was %02X)", value, m_state.SPRINIT);
        m_state.SPRINIT = value;
        break;
    case SUZY_SUZYHREV:    // 0xFC88
        DebugSuzy("Writing to read-only SUZYHREV: %02X", value);
        break;
    case SUZY_SUZYSREV:    // 0xFC89
        DebugSuzy("Writing to read-only SUZYSREV: %02X", value);
        break;
    case SUZY_SUZYBUSEN:   // 0xFC90
        DebugSuzy("Setting SUZYBUSEN to %02X (was %02X)", value, m_state.SUZYBUSEN);
        m_state.SUZYBUSEN = value;
        break;
    case SUZY_SPRGO:       // 0xFC91
        DebugSuzy("Setting SPRGO to %02X (was %02X)", value, m_state.SPRGO);
        m_state.SPRGO = value;
        if ((value & 0x01) && IS_SET_BIT(m_state.SUZYBUSEN, 0))
            SpritesGo();
        break;
    case SUZY_SPRSYS:      // 0xFC92
        DebugSuzy("Setting SPRSYS to %02X", value);
        m_state.sprsys_stopsprites = IS_SET_BIT(value, 1);
        m_state.sprsys_unsafe = IS_SET_BIT(value, 2) ? false : m_state.sprsys_unsafe;
        m_state.sprsys_lefthand = IS_SET_BIT(value, 3);
        m_state.sprsys_vstrech = IS_SET_BIT(value, 4);
        m_state.sprsys_dontcollide = IS_SET_BIT(value, 5);
        m_state.sprsys_accumulate = IS_SET_BIT(value, 6);
        m_state.sprsys_sign = IS_SET_BIT(value, 7);
        break;
    case SUZY_JOYSTICK:    // 0xFCB0
        DebugSuzy("Writing to read-only JOYSTICK: %02X", value);
        break;
    case SUZY_SWITCHES:    // 0xFCB1
        DebugSuzy("Writing to read-only SWITCHES: %02X", value);
        break;
    case SUZY_RCART0:      // 0xFCB2
        DebugSuzy("Writing to RCART0: %02X", value);
        if (m_media->GetAudin() && m_media->GetAudinValue())
            m_media->WriteBank0A(value);
        else
            m_media->WriteBank0(value);
        break;
    case SUZY_RCART1:      // 0xFCB3
        DebugSuzy("Writing to RCART1: %02X", value);
        if (m_media->GetAudin() && m_media->GetAudinValue())
            m_media->WriteBank1A(value);
        else
            m_media->WriteBank1(value);
        break;
    case SUZY_LEDS:        // 0xFCC0
        DebugSuzy("Writing to LEDS (unused): %02X", value);
        break;
    case SUZY_PPORTSTAT:   // 0xFCC2
        DebugSuzy("Writing to PPORTSTAT (unused): %02X", value);
        break;
    case SUZY_PPORTDATA:   // 0xFCC3
        DebugSuzy("Writing to PPORTDATA (unused): %02X", value);
        break;
    case SUZY_HOWIE:       // 0xFCC4
        DebugSuzy("Writing to HOWIE (unused): %02X", value);
        break;
    default:
        DebugSuzy("Register WRITE called with unknown address: %04X, value: %02X", address, value);
        break;
    }
}

INLINE Suzy::Suzy_State* Suzy::GetState()
{
    return &m_state;
}

INLINE bool Suzy::IsBlitterBusy()
{
    return m_state.sprsys_spritesbusy;
};

#if !defined(GLYNX_DISABLE_DISASSEMBLER)
INLINE std::vector<Suzy::GLYNX_SCB_Info>* Suzy::GetFrameSCBList()
{
    return &m_frame_scb_list_display;
}

INLINE void Suzy::SwapFrameSCBList()
{
    if (m_scb_accumulation_enabled)
    {
        m_frame_scb_list_display.swap(m_frame_scb_list);
        m_frame_scb_list.clear();
    }
}

INLINE void Suzy::SetSCBAccumulationEnabled(bool enabled)
{
    m_scb_accumulation_enabled = enabled;
    if (!enabled)
    {
        m_frame_scb_list.clear();
        m_frame_scb_list_display.clear();
    }
}
#endif

INLINE void Suzy::SpritesGo()
{
    DebugSuzy("SpritesGo called: SPRCTL0=%02X, SPRCTL1=%02X, SPRCOLL=%02X, SPRINIT=%02X",
              m_state.SPRCTL0, m_state.SPRCTL1, m_state.SPRCOLL, m_state.SPRINIT);

    m_state.sprite_cycles = 0;
    m_state.sprsys_spritesbusy = true;
    m_state.fsm_phase = SUZY_PHASE_IDLE;

#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    if (m_trace_logger->IsEnabled(TRACE_SUZY_SPRITE))
    {
        GLYNX_Trace_Entry e = {};
        e.type = TRACE_SUZY_SPRITE;
        e.sprite.scb_addr = m_state.SCBNEXT.value;
        e.sprite.is_start = true;
        m_trace_logger->TraceLog(e);
    }
#endif

    if (m_fast_sprite_rendering)
    {
        while ((m_state.SCBNEXT.value & 0xFF00) != 0)
        {
            DrawSprite();
        }

        DebugSuzy("SpritesGo finished: total cycles = %d", m_state.sprite_cycles);

#if !defined(GLYNX_DISABLE_DISASSEMBLER)
        if (m_trace_logger->IsEnabled(TRACE_SUZY_SPRITE))
        {
            GLYNX_Trace_Entry e = {};
            e.type = TRACE_SUZY_SPRITE;
            e.sprite.is_end = true;
            e.sprite.total_cycles = m_state.sprite_cycles;
            m_trace_logger->TraceLog(e);
        }
#endif

        if (m_state.sprite_cycles == 0)
        {
            m_state.sprsys_spritesbusy = false;
            m_state.SPRGO = UNSET_BIT(m_state.SPRGO, 0);
            m_m6502->Halt(false);
        }
        else
            m_state.fsm_phase = SUZY_PHASE_LEGACY_DELAY;

        return;
    }

    m_state.fsm_phase = SUZY_PHASE_SCB_FETCH;
    m_state.spr_quadrant = 0;
    m_state.quad_row = 0;
    m_state.quad_pixel_height = 0;
    m_state.row_emit_count = 0;

    if ((m_state.SCBNEXT.value & 0xFF00) == 0)
        FinishBlitter();
}

INLINE void Suzy::FinishBlitter()
{
    DebugSuzy("SpritesGo finished: total cycles = %d", m_state.sprite_cycles);

#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    if (m_trace_logger->IsEnabled(TRACE_SUZY_SPRITE))
    {
        GLYNX_Trace_Entry e = {};
        e.type = TRACE_SUZY_SPRITE;
        e.sprite.is_end = true;
        e.sprite.total_cycles = m_state.sprite_cycles;
        m_trace_logger->TraceLog(e);
    }
#endif

    m_state.sprite_cycles = 0;
    m_state.sprsys_spritesbusy = false;
    m_state.SPRGO = UNSET_BIT(m_state.SPRGO, 0);
    m_state.fsm_phase = SUZY_PHASE_IDLE;
    m_m6502->Halt(false);
}

INLINE bool Suzy::ConsumeBlitterCycleDebt(u32* cycles)
{
    if (m_state.sprite_cycles >= *cycles)
    {
        m_state.sprite_cycles -= *cycles;
        return false;
    }

    *cycles -= m_state.sprite_cycles;
    m_state.sprite_cycles = 0;
    return true;
}

INLINE void Suzy::StepBlitter(u32 cycles)
{
    if (m_state.fsm_phase == SUZY_PHASE_LEGACY_DELAY)
    {
        if (m_state.sprite_cycles > cycles)
            m_state.sprite_cycles -= cycles;
        else
        {
            DebugSuzy("Sprite operation completed");
            m_state.sprite_cycles = 0;
            m_state.sprsys_spritesbusy = false;
            m_state.SPRGO = UNSET_BIT(m_state.SPRGO, 0);
            m_state.fsm_phase = SUZY_PHASE_IDLE;
            m_m6502->Halt(false);
        }
        return;
    }

    if (!m_m6502->IsHalted())
        return;

    if (IS_NOT_SET_BIT(m_state.SUZYBUSEN, 0))
        return;

    if (cycles == 0)
        cycles = 1;

    if (!ConsumeBlitterCycleDebt(&cycles))
        return;

    while (m_state.fsm_phase != SUZY_PHASE_IDLE)
    {
        StepBlitterPhase();

        if (m_state.fsm_phase == SUZY_PHASE_IDLE)
            return;

        if (!ConsumeBlitterCycleDebt(&cycles))
            return;
    }
}

INLINE void Suzy::StepBlitterPhase()
{
    switch (m_state.fsm_phase)
    {
        case SUZY_PHASE_SCB_FETCH:
        {
            if ((m_state.SCBNEXT.value & 0xFF00) == 0)
            {
                FinishBlitter();
                break;
            }

            DebugSuzy("Drawing sprite at SCB %04X", m_state.SCBNEXT.value);

            m_state.SCBADR.value = m_state.SCBNEXT.value;
            m_state.TMPADR.value = m_state.SCBADR.value;

            m_state.SPRCTL0 = RamRead(m_state.TMPADR.value++);
            m_state.SPRCTL1 = RamRead(m_state.TMPADR.value++);
            m_state.SPRCOLL = RamRead(m_state.TMPADR.value++);
            m_state.SCBNEXT.value = RamReadWord(m_state.TMPADR.value);
            m_state.TMPADR.value += 2;
            m_state.sprite_cycles += 5 * k_suzy_ram_read_ticks;
            m_state.fred = 0;
            m_state.everon = false;

            if (IS_SET_BIT(m_state.SPRCTL1, 2))
            {
                DebugSuzy("Skipping sprite at SCB %04X due to SPRCTL1 bit 2 set", m_state.SCBADR.value);

#if !defined(GLYNX_DISABLE_DISASSEMBLER)
                if (m_trace_logger->IsEnabled(TRACE_SUZY_SPRITE))
                {
                    GLYNX_Trace_Entry e = {};
                    e.type = TRACE_SUZY_SPRITE;
                    e.sprite.scb_addr = m_state.SCBADR.value;
                    e.sprite.skipped = true;
                    m_trace_logger->TraceLog(e);
                }

                if (m_scb_accumulation_enabled)
                {
                    GLYNX_SCB_Info si = {};
                    si.scb_address = m_state.SCBADR.value;
                    si.scb_next = m_state.SCBNEXT.value;
                    si.sprctl0 = m_state.SPRCTL0;
                    si.sprctl1 = m_state.SPRCTL1;
                    si.sprcoll = m_state.SPRCOLL;
                    si.skipped = true;
                    si.hoff = (s16)m_state.HOFF.value;
                    si.voff = (s16)m_state.VOFF.value;
                    m_frame_scb_list.push_back(si);
                }
#endif
                m_state.fsm_phase = SUZY_PHASE_SCB_NEXT;
                break;
            }

            m_state.fsm_phase = SUZY_PHASE_SCB_RELOAD;
            break;
        }

        case SUZY_PHASE_SCB_RELOAD:
        {
            int reload_depth = (m_state.SPRCTL1 >> 4) & 0x03;

#if defined(GLYNX_DEBUG_SUZY)
            int bpp = ((m_state.SPRCTL0 >> 6) & 0x03) + 1;
            bool h_flip = IS_SET_BIT(m_state.SPRCTL0, 5);
            bool v_flip = IS_SET_BIT(m_state.SPRCTL0, 4);
            int type = (m_state.SPRCTL0 & 0x07);
            bool literal_only = IS_SET_BIT(m_state.SPRCTL1, 7);
            bool reload_palette = IS_NOT_SET_BIT(m_state.SPRCTL1, 3);
            bool start_up = IS_SET_BIT(m_state.SPRCTL1, 1);
            bool start_left = IS_SET_BIT(m_state.SPRCTL1, 0);

            DebugSuzy("  SPRCTL0: BPP=%d, HFLIP=%d, VFLIP=%d, TYPE=%d", bpp, h_flip ? 1 : 0, v_flip ? 1 : 0, type);
            DebugSuzy("  SPRCTL1: LITERAL=%d, RDEPTH=%d, RPALETTE=%d, STARTUP=%d, STARTLEFT=%d",
                      literal_only ? 1 : 0, reload_depth, reload_palette ? 1 : 0, start_up ? 1 : 0, start_left ? 1 : 0);
            DebugSuzy("  SPRCOLL: COLLIDE=%s, COLLISIONID=%d",
                      (!m_state.sprsys_dontcollide && IS_NOT_SET_BIT(m_state.SPRCOLL, 5)) ? "YES" : "NO",
                      m_state.SPRCOLL & 0x0F);
#endif

            m_state.SPRDLINE.value = RamReadWord(m_state.TMPADR.value);
            m_state.TMPADR.value += 2;
            m_state.HPOSSTRT.value = RamReadWord(m_state.TMPADR.value);
            m_state.TMPADR.value += 2;
            m_state.VPOSSTRT.value = RamReadWord(m_state.TMPADR.value);
            m_state.TMPADR.value += 2;
            m_state.sprite_cycles += 6 * k_suzy_ram_read_ticks;

            m_state.STRETCH.value = 0;
            m_state.TILT.value = 0;

            if (reload_depth == 1)
            {
                m_state.SPRHSIZ.value = RamReadWord(m_state.TMPADR.value);
                m_state.TMPADR.value += 2;
                m_state.SPRVSIZ.value = RamReadWord(m_state.TMPADR.value);
                m_state.TMPADR.value += 2;
                m_state.sprite_cycles += 4 * k_suzy_ram_read_ticks;
            }
            else if (reload_depth == 2)
            {
                m_state.SPRHSIZ.value = RamReadWord(m_state.TMPADR.value);
                m_state.TMPADR.value += 2;
                m_state.SPRVSIZ.value = RamReadWord(m_state.TMPADR.value);
                m_state.TMPADR.value += 2;
                m_state.STRETCH.value = RamReadWord(m_state.TMPADR.value);
                m_state.TMPADR.value += 2;
                m_state.sprite_cycles += 6 * k_suzy_ram_read_ticks;
            }
            else if (reload_depth == 3)
            {
                m_state.SPRHSIZ.value = RamReadWord(m_state.TMPADR.value);
                m_state.TMPADR.value += 2;
                m_state.SPRVSIZ.value = RamReadWord(m_state.TMPADR.value);
                m_state.TMPADR.value += 2;
                m_state.STRETCH.value = RamReadWord(m_state.TMPADR.value);
                m_state.TMPADR.value += 2;
                m_state.TILT.value = RamReadWord(m_state.TMPADR.value);
                m_state.TMPADR.value += 2;
                m_state.sprite_cycles += 8 * k_suzy_ram_read_ticks;
            }

            m_state.fsm_phase = SUZY_PHASE_PALETTE;
            break;
        }

        case SUZY_PHASE_PALETTE:
        {
            int bpp = ((m_state.SPRCTL0 >> 6) & 0x03) + 1;
            bool reload_palette = IS_NOT_SET_BIT(m_state.SPRCTL1, 3);

            if (reload_palette)
            {
                int colors = 1 << bpp;
                int bytes_to_read = colors >> 1;
                m_state.sprite_cycles += bytes_to_read * k_suzy_ram_read_ticks;

                for (int i = 0; i < bytes_to_read; ++i)
                {
                    u8 byte = RamRead(m_state.TMPADR.value++);
                    m_state.pen_map[(i << 1) + 0] = (byte >> 4) & 0x0F;
                    m_state.pen_map[(i << 1) + 1] = (byte & 0x0F);
                }
            }

#if !defined(GLYNX_DISABLE_DISASSEMBLER)
            if (m_scb_accumulation_enabled)
            {
                GLYNX_SCB_Info si = {};
                si.scb_address = m_state.SCBADR.value;
                si.scb_next = m_state.SCBNEXT.value;
                si.sprctl0 = m_state.SPRCTL0;
                si.sprctl1 = m_state.SPRCTL1;
                si.sprcoll = m_state.SPRCOLL;
                si.hpos = (s16)m_state.HPOSSTRT.value;
                si.vpos = (s16)m_state.VPOSSTRT.value;
                si.sprdline = m_state.SPRDLINE.value;
                si.sprhsiz = m_state.SPRHSIZ.value;
                si.sprvsiz = m_state.SPRVSIZ.value;
                si.stretch = m_state.STRETCH.value;
                si.tilt = m_state.TILT.value;
                si.skipped = false;
                si.hoff = (s16)m_state.HOFF.value;
                si.voff = (s16)m_state.VOFF.value;
                memcpy(si.pen_map, m_state.pen_map, 16);
                m_frame_scb_list.push_back(si);
            }

            if (m_trace_logger->IsEnabled(TRACE_SUZY_SPRITE))
            {
                GLYNX_Trace_Entry e = {};
                e.type = TRACE_SUZY_SPRITE;
                e.sprite.scb_addr = m_state.SCBADR.value;
                e.sprite.scb_next = m_state.SCBNEXT.value;
                e.sprite.hpos = (s16)m_state.HPOSSTRT.value;
                e.sprite.vpos = (s16)m_state.VPOSSTRT.value;
                e.sprite.sprctl0 = m_state.SPRCTL0;
                e.sprite.bpp = (u8)bpp;
                e.sprite.type = (u8)(m_state.SPRCTL0 & 0x07);
                m_trace_logger->TraceLog(e);
            }
#endif

            m_state.fsm_phase = SUZY_PHASE_QUAD_INIT;
            break;
        }

        case SUZY_PHASE_QUAD_INIT:
        {
            m_state.HPOSSTRT.value = (u16)((s16)m_state.HPOSSTRT.value - (s16)m_state.HOFF.value);
            m_state.VPOSSTRT.value = (u16)((s16)m_state.VPOSSTRT.value - (s16)m_state.VOFF.value);
            m_state.spr_quadrant = 0;
            m_state.TILTACUM.value = 0;
            m_state.SPRVPOS.value = m_state.VPOSSTRT.value;
            m_state.VSIZACUM.value = m_state.VSIZOFF.value;
            m_state.fsm_phase = SUZY_PHASE_LINE_FETCH;
            break;
        }

        case SUZY_PHASE_LINE_FETCH:
        {
            u16 line_address = m_state.SPRDLINE.value;
            u8 sprdoff = RamRead(line_address);
            m_state.sprite_cycles += k_suzy_ram_read_ticks;

            m_state.SPRDOFF.value = sprdoff;
            m_state.TMPADR.value = (u16)(line_address + 1);
            m_state.SPRDLINE.value = (u16)(line_address + (u16)sprdoff);

            if (sprdoff <= 1)
            {
                m_state.sprite_cycles += k_suzy_control_line_ticks;
                m_state.quad_row = 0;
                m_state.quad_pixel_height = 0;
                m_state.fsm_phase = SUZY_PHASE_QUAD_END;
                break;
            }

            m_state.VSIZACUM.value = m_state.VSIZACUM.value + m_state.SPRVSIZ.value;
            m_state.quad_pixel_height = (s16)(m_state.VSIZACUM.value >> 8);
            m_state.VSIZACUM.value &= 0x00FF;
            m_state.quad_row = 0;

            m_state.fsm_phase = (m_state.quad_pixel_height > 0) ? SUZY_PHASE_ROW_BEGIN : SUZY_PHASE_QUAD_END;
            break;
        }

        case SUZY_PHASE_ROW_BEGIN:
        {
            bool h_flip = IS_SET_BIT(m_state.SPRCTL0, 5);
            bool v_flip = IS_SET_BIT(m_state.SPRCTL0, 4);
            bool start_up = IS_SET_BIT(m_state.SPRCTL1, 1);
            bool start_left = IS_SET_BIT(m_state.SPRCTL1, 0);
            int flip = (h_flip ? 1 : 0) | (v_flip ? 2 : 0);
            int start_quad = (start_left ? 1 : 0) | (start_up ? 2 : 0);
            QuadPos pos = m_quad_lut[m_state.spr_quadrant][start_quad][flip];
            QuadPos start_pos = m_quad_lut[0][start_quad][flip];
            s32 dx = pos.left ? -1 : +1;
            s32 start_x = (s16)m_state.HPOSSTRT.value;

            if (pos.left != start_pos.left)
                start_x += dx;

            m_state.row_x = (s16)start_x;
            m_state.row_render = ((u32)(s16)m_state.SPRVPOS.value < (u32)GLYNX_SCREEN_HEIGHT) ? 1 : 0;
            m_state.row_h_accum = m_state.HSIZOFF.value;
            m_state.row_emit_count = 0;
            m_state.row_pen = 0;
            m_state.pack_state = SUZY_PACK_HEADER;
            m_state.pack_count = 0;
            m_state.pack_pen = 0;
            m_state.pack_is_literal = 0;
            m_state.pack_pixel_pair = 0;

            ShiftRegisterReset(m_state.TMPADR.value);

            if (IS_NOT_SET_BIT(m_state.SPRCTL1, 7))
                m_state.sprite_cycles += k_suzy_packed_line_ticks;
            else if (IS_SET_BIT(m_state.SPRCTL1, 3))
                m_state.sprite_cycles += k_suzy_literal_line_ticks;

            m_state.fsm_phase = SUZY_PHASE_ROW_PAINT;
            break;
        }

        case SUZY_PHASE_ROW_PAINT:
        {
            bool h_flip = IS_SET_BIT(m_state.SPRCTL0, 5);
            bool v_flip = IS_SET_BIT(m_state.SPRCTL0, 4);
            bool start_up = IS_SET_BIT(m_state.SPRCTL1, 1);
            bool start_left = IS_SET_BIT(m_state.SPRCTL1, 0);
            int flip = (h_flip ? 1 : 0) | (v_flip ? 2 : 0);
            int start_quad = (start_left ? 1 : 0) | (start_up ? 2 : 0);
            QuadPos pos = m_quad_lut[m_state.spr_quadrant][start_quad][flip];
            s32 dx = pos.left ? -1 : +1;
            int bpp = ((m_state.SPRCTL0 >> 6) & 0x03) + 1;
            int type = (m_state.SPRCTL0 & 0x07);
            bool collide = !m_state.sprsys_dontcollide && IS_NOT_SET_BIT(m_state.SPRCOLL, 5);
            u8 collision_id = (m_state.SPRCOLL & 0x0F);
            bool done;

            if (IS_SET_BIT(m_state.SPRCTL1, 7))
                done = DrawSpriteLineLiteralStep(m_state.SPRDLINE.value, dx, bpp, type, collide, collision_id);
            else
                done = DrawSpriteLinePackedStep(m_state.SPRDLINE.value, dx, bpp, type, collide, collision_id);

            if (done)
                m_state.fsm_phase = SUZY_PHASE_ROW_END;

            break;
        }

        case SUZY_PHASE_ROW_END:
        {
            bool h_flip = IS_SET_BIT(m_state.SPRCTL0, 5);
            bool v_flip = IS_SET_BIT(m_state.SPRCTL0, 4);
            bool start_up = IS_SET_BIT(m_state.SPRCTL1, 1);
            bool start_left = IS_SET_BIT(m_state.SPRCTL1, 0);
            int flip = (h_flip ? 1 : 0) | (v_flip ? 2 : 0);
            int start_quad = (start_left ? 1 : 0) | (start_up ? 2 : 0);
            QuadPos pos = m_quad_lut[m_state.spr_quadrant][start_quad][flip];
            s32 dy = pos.up ? -1 : +1;

            m_state.SPRVPOS.value = (u16)((s16)m_state.SPRVPOS.value + dy);
            m_state.TILTACUM.value = (u16)(m_state.TILTACUM.value + m_state.TILT.value);
            s32 tilt_carry = (s16)m_state.TILTACUM.value >> 8;
            m_state.HPOSSTRT.value = (u16)((s16)m_state.HPOSSTRT.value + tilt_carry);
            m_state.TILTACUM.value &= 0x00FF;
            m_state.SPRHSIZ.value += m_state.STRETCH.value;
            m_state.quad_row++;

            bool visible_y = ((u32)(s16)m_state.SPRVPOS.value < (u32)GLYNX_SCREEN_HEIGHT);
            u32 row_bus_ticks = visible_y ? k_suzy_visible_row_ticks : 0;
            if (visible_y && IS_NOT_SET_BIT(m_state.SPRCTL1, 7))
            {
                if (((m_state.SPRCTL1 >> 4) & 0x03) != 0)
                    row_bus_ticks += k_suzy_packed_reload_row_ticks;

                if (((u16)(m_state.SPRDLINE.value - m_state.TMPADR.value) > 4))
                    row_bus_ticks -= k_suzy_packed_wide_row_discount_ticks;
            }
            else if (visible_y && (((m_state.SPRCTL1 >> 4) & 0x03) != 0) && ((u16)(m_state.SPRDLINE.value - m_state.TMPADR.value) > 16))
                row_bus_ticks += k_suzy_literal_wide_reload_row_ticks;

            m_state.sprite_cycles += row_bus_ticks;
            m_state.fsm_phase = (m_state.quad_row < m_state.quad_pixel_height) ? SUZY_PHASE_ROW_BEGIN : SUZY_PHASE_QUAD_END;
            break;
        }

        case SUZY_PHASE_QUAD_END:
        {
            if (m_state.sprsys_vstrech)
            {
                s16 add = (s16)m_state.STRETCH.value * (s16)m_state.quad_pixel_height;
                m_state.SPRVSIZ.value += add;
            }

            if (m_state.SPRDOFF.low == 0)
            {
                m_state.fsm_phase = SUZY_PHASE_SPR_END;
                break;
            }

            if (m_state.SPRDOFF.low == 1)
            {
                bool h_flip = IS_SET_BIT(m_state.SPRCTL0, 5);
                bool v_flip = IS_SET_BIT(m_state.SPRCTL0, 4);
                bool start_up = IS_SET_BIT(m_state.SPRCTL1, 1);
                bool start_left = IS_SET_BIT(m_state.SPRCTL1, 0);
                int flip = (h_flip ? 1 : 0) | (v_flip ? 2 : 0);
                int start_quad = (start_left ? 1 : 0) | (start_up ? 2 : 0);

                m_state.spr_quadrant = (m_state.spr_quadrant + 1) & 3;
                QuadPos pos = m_quad_lut[m_state.spr_quadrant][start_quad][flip];
                QuadPos start_pos = m_quad_lut[0][start_quad][flip];
                s32 dy = pos.up ? -1 : +1;

                m_state.SPRVPOS.value = m_state.VPOSSTRT.value;
                m_state.VSIZACUM.value = m_state.VSIZOFF.value;

                if (pos.up != start_pos.up)
                    m_state.SPRVPOS.value = (u16)((s16)m_state.SPRVPOS.value + dy);
            }

            m_state.fsm_phase = SUZY_PHASE_LINE_FETCH;
            break;
        }

        case SUZY_PHASE_SPR_END:
        {
            u16 colpos = m_state.SCBADR.value + m_state.COLLOFF.value;
            bool collide = !m_state.sprsys_dontcollide && IS_NOT_SET_BIT(m_state.SPRCOLL, 5);
            int type = (m_state.SPRCTL0 & 0x07);

            if (IS_NOT_SET_BIT(m_state.SPRCTL1, 7))
                m_state.sprite_cycles += k_suzy_packed_scb_ticks;

            if (collide)
            {
                switch (type)
                {
                    case 2:
                    case 3:
                    case 4:
                    case 6:
                    case 7:
                        RamWrite(colpos, m_state.fred);
                        break;
                    default:
                        break;
                }
            }

            if (IS_SET_BIT(m_state.SPRGO, 2))
            {
                u8 depository = RamRead(colpos);
                depository = m_state.everon ? UNSET_BIT(depository, 7) : SET_BIT(depository, 7);
                RamWrite(colpos, depository);
            }

            m_state.fsm_phase = SUZY_PHASE_SCB_NEXT;
            break;
        }

        case SUZY_PHASE_SCB_NEXT:
        {
            if (m_state.sprsys_stopsprites || ((m_state.SCBNEXT.value & 0xFF00) == 0))
                FinishBlitter();
            else
                m_state.fsm_phase = SUZY_PHASE_SCB_FETCH;

            break;
        }

        default:
            FinishBlitter();
            break;
    }
}

INLINE bool Suzy::DrawSpriteEmitPen(u8 pen, s32 dx, int type, bool collide, u8 collision_id)
{
    if (m_state.row_emit_count <= 0)
        return false;

    if (m_state.row_render)
    {
        DrawPixel(m_state.row_x, (s16)m_state.SPRVPOS.value, pen, type, collide, collision_id);
        m_state.row_x = (s16)(m_state.row_x + dx);

        if ((dx > 0 && m_state.row_x >= GLYNX_SCREEN_WIDTH) || (dx < 0 && m_state.row_x < 0))
            m_state.row_render = 0;
    }
    else
    {
        m_state.row_x = (s16)(m_state.row_x + (dx * m_state.row_emit_count));
        m_state.row_emit_count = 1;
    }

    m_state.row_emit_count--;
    return false;
}

INLINE bool Suzy::DrawSpriteLineLiteralStep(u16 data_end, s32 dx, int bpp, int type, bool collide, u8 collision_id)
{
    while (true)
    {
        if (m_state.row_emit_count > 0)
        {
            if (IS_SET_BIT(m_state.SPRCTL1, 3) && ((m_state.row_x & 1) == 0))
                m_state.sprite_cycles += k_suzy_literal_byte_ticks;

            return DrawSpriteEmitPen(m_state.row_pen, dx, type, collide, collision_id);
        }

        if (m_state.shift_register_address >= data_end)
            return true;

        u32 pi = ShiftRegisterGetBits(bpp, data_end);
        if (pi == SHIFTREG_EOF)
            return true;

        m_state.row_pen = m_state.pen_map[pi & 0x0F];

        u32 h_accum = (u32)m_state.row_h_accum + (u32)m_state.SPRHSIZ.value;
        m_state.row_emit_count = (s16)(h_accum >> 8);
        m_state.row_h_accum = (u16)(h_accum & 0xFF);
    }
}

INLINE void Suzy::AddPackedPixelTicks()
{
    m_state.pack_pixel_pair = (m_state.pack_pixel_pair + 1) & 3;

    if ((m_state.pack_pixel_pair & 1) == 0)
        m_state.sprite_cycles += k_suzy_packed_pair_ticks;

    if (m_state.pack_pixel_pair == 0)
        m_state.sprite_cycles += k_suzy_packed_quad_ticks;
}

INLINE bool Suzy::DrawSpriteLinePackedStep(u16 data_end, s32 dx, int bpp, int type, bool collide, u8 collision_id)
{
    while (true)
    {
        if (m_state.row_emit_count > 0)
            return DrawSpriteEmitPen(m_state.row_pen, dx, type, collide, collision_id);

        switch (m_state.pack_state)
        {
        case SUZY_PACK_HEADER:
        {
            if (m_state.shift_register_address >= data_end)
                return true;

            u32 header = ShiftRegisterGetBits(5, data_end);
            if (header == 0 || header == SHIFTREG_EOF)
                return true;

            m_state.pack_is_literal = (u8)(header >> 4);
            m_state.pack_count = (u8)((header & 0x0F) + 1);
            m_state.pack_state = m_state.pack_is_literal ? SUZY_PACK_LITERAL : SUZY_PACK_RLE_PEN;
            break;
        }

        case SUZY_PACK_LITERAL:
        {
            if (m_state.pack_count == 0)
            {
                m_state.pack_state = SUZY_PACK_HEADER;
                break;
            }

            u32 pi = ShiftRegisterGetBits(bpp, data_end);
            if (pi == SHIFTREG_EOF)
                return true;

            m_state.row_pen = m_state.pen_map[pi & 0x0F];
            m_state.pack_count--;
            AddPackedPixelTicks();

            u32 h_accum = (u32)m_state.row_h_accum + (u32)m_state.SPRHSIZ.value;
            m_state.row_emit_count = (s16)(h_accum >> 8);
            m_state.row_h_accum = (u16)(h_accum & 0xFF);
            break;
        }

        case SUZY_PACK_RLE_PEN:
        {
            u32 pixel_index = ShiftRegisterGetBits(bpp, data_end);
            if (pixel_index == SHIFTREG_EOF)
                return true;

            m_state.pack_pen = m_state.pen_map[pixel_index & 0x0F];
            m_state.pack_state = SUZY_PACK_RLE;
            break;
        }

        case SUZY_PACK_RLE:
        {
            if (m_state.pack_count == 0)
            {
                m_state.pack_state = SUZY_PACK_HEADER;
                break;
            }

            m_state.row_pen = m_state.pack_pen;
            m_state.pack_count--;
            AddPackedPixelTicks();

            u32 h_accum = (u32)m_state.row_h_accum + (u32)m_state.SPRHSIZ.value;
            m_state.row_emit_count = (s16)(h_accum >> 8);
            m_state.row_h_accum = (u16)(h_accum & 0xFF);
            break;
        }

        default:
            return true;
        }
    }
}

INLINE void Suzy::DrawSprite()
{
    DebugSuzy("Drawing sprite at SCB %04X", m_state.SCBNEXT.value);

    m_state.SCBADR.value = m_state.SCBNEXT.value;
    m_state.TMPADR.value = m_state.SCBADR.value;

    m_state.SPRCTL0 = RamRead(m_state.TMPADR.value++);
    m_state.SPRCTL1 = RamRead(m_state.TMPADR.value++);
    m_state.SPRCOLL = RamRead(m_state.TMPADR.value++);
    m_state.SCBNEXT.value = RamReadWord(m_state.TMPADR.value);
    m_state.TMPADR.value += 2;
    m_state.sprite_cycles += 5 * k_suzy_ram_read_ticks;  // 5 bytes from SCB header

    if (IS_SET_BIT(m_state.SPRCTL1, 2))
    {
        DebugSuzy("Skipping sprite at SCB %04X due to SPRCTL1 bit 2 set", m_state.SCBADR.value);

#if !defined(GLYNX_DISABLE_DISASSEMBLER)
        if (m_trace_logger->IsEnabled(TRACE_SUZY_SPRITE))
        {
            GLYNX_Trace_Entry e = {};
            e.type = TRACE_SUZY_SPRITE;
            e.sprite.scb_addr = m_state.SCBADR.value;
            e.sprite.skipped = true;
            m_trace_logger->TraceLog(e);
        }
#endif
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
        if (m_scb_accumulation_enabled)
        {
            GLYNX_SCB_Info si = {};
            si.scb_address = m_state.SCBADR.value;
            si.scb_next = m_state.SCBNEXT.value;
            si.sprctl0 = m_state.SPRCTL0;
            si.sprctl1 = m_state.SPRCTL1;
            si.sprcoll = m_state.SPRCOLL;
            si.skipped = true;
            si.hoff = (s16)m_state.HOFF.value;
            si.voff = (s16)m_state.VOFF.value;
            m_frame_scb_list.push_back(si);
        }
#endif
        return;
    }

    int bpp = ((m_state.SPRCTL0 >> 6) & 0x03) + 1;
    bool h_flip = IS_SET_BIT(m_state.SPRCTL0, 5);
    bool v_flip = IS_SET_BIT(m_state.SPRCTL0, 4);
    int flip = (h_flip ? 1 : 0) | (v_flip ? 2 : 0);
    int type = (m_state.SPRCTL0 & 0x07);

    DebugSuzy("  SPRCTL0: BPP=%d, HFLIP=%d, VFLIP=%d, TYPE=%d", bpp, h_flip ? 1 : 0, v_flip ? 1 : 0, type);

    bool literal_only = IS_SET_BIT(m_state.SPRCTL1, 7);
    int reload_depth = (m_state.SPRCTL1 >> 4) & 0x03;
    bool reload_palette = IS_NOT_SET_BIT(m_state.SPRCTL1, 3);
    bool start_up = IS_SET_BIT(m_state.SPRCTL1, 1);
    bool start_left = IS_SET_BIT(m_state.SPRCTL1, 0);
    int start_quad = (start_left ? 1 : 0) | (start_up ? 2 : 0);

    DebugSuzy("  SPRCTL1: LITERAL=%d, RDEPTH=%d, RPALETTE=%d, STARTUP=%d, STARTLEFT=%d",
              literal_only ? 1 : 0, reload_depth, reload_palette ? 1 : 0, start_up ? 1 : 0, start_left ? 1 : 0);

    m_state.fred = 0;
    bool collide = !m_state.sprsys_dontcollide && IS_NOT_SET_BIT(m_state.SPRCOLL, 5);
    u8 collision_id = (m_state.SPRCOLL & 0x0F);

    DebugSuzy("  SPRCOLL: COLLIDE=%s, COLLISIONID=%d", collide ? "YES" : "NO", collision_id);

    bool vertical_stretch = m_state.sprsys_vstrech;

    m_state.SPRDLINE.value = RamReadWord(m_state.TMPADR.value);
    m_state.TMPADR.value += 2;
    m_state.HPOSSTRT.value = RamReadWord(m_state.TMPADR.value);
    m_state.TMPADR.value += 2;
    m_state.VPOSSTRT.value = RamReadWord(m_state.TMPADR.value);
    m_state.TMPADR.value += 2;
    m_state.sprite_cycles += 6 * k_suzy_ram_read_ticks;  // 6 bytes for position data

    m_state.STRETCH.value = 0;
    m_state.TILT.value = 0;

    if (reload_depth == 1)
    {
        m_state.SPRHSIZ.value = RamReadWord(m_state.TMPADR.value);
        m_state.TMPADR.value += 2;
        m_state.SPRVSIZ.value = RamReadWord(m_state.TMPADR.value);
        m_state.TMPADR.value += 2;
        m_state.sprite_cycles += 4 * k_suzy_ram_read_ticks;  // 4 bytes for size
    }
    else if (reload_depth == 2)
    {
        m_state.SPRHSIZ.value = RamReadWord(m_state.TMPADR.value);
        m_state.TMPADR.value += 2;
        m_state.SPRVSIZ.value = RamReadWord(m_state.TMPADR.value);
        m_state.TMPADR.value += 2;
        m_state.STRETCH.value = RamReadWord(m_state.TMPADR.value);
        m_state.TMPADR.value += 2;
        m_state.sprite_cycles += 6 * k_suzy_ram_read_ticks;  // 6 bytes for size+stretch
    }
    else if (reload_depth == 3)
    {
        m_state.SPRHSIZ.value = RamReadWord(m_state.TMPADR.value);
        m_state.TMPADR.value += 2;
        m_state.SPRVSIZ.value = RamReadWord(m_state.TMPADR.value);
        m_state.TMPADR.value += 2;
        m_state.STRETCH.value = RamReadWord(m_state.TMPADR.value);
        m_state.TMPADR.value += 2;
        m_state.TILT.value = RamReadWord(m_state.TMPADR.value);
        m_state.TMPADR.value += 2;
        m_state.sprite_cycles += 8 * k_suzy_ram_read_ticks;  // 8 bytes for size+stretch+tilt
    }

    if (reload_palette)
    {
        int colors = 1 << bpp;
        int bytes_to_read = colors >> 1;
        m_state.sprite_cycles += bytes_to_read * k_suzy_ram_read_ticks;  // palette bytes

        for (int i = 0; i < bytes_to_read; ++i)
        {
            u8 byte = RamRead(m_state.TMPADR.value++);
            m_state.pen_map[(i << 1) + 0] = (byte >> 4) & 0x0F;
            m_state.pen_map[(i << 1) + 1] = (byte & 0x0F);
        }
    }

#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    if (m_scb_accumulation_enabled)
    {
        GLYNX_SCB_Info si = {};
        si.scb_address = m_state.SCBADR.value;
        si.scb_next = m_state.SCBNEXT.value;
        si.sprctl0 = m_state.SPRCTL0;
        si.sprctl1 = m_state.SPRCTL1;
        si.sprcoll = m_state.SPRCOLL;
        si.hpos = (s16)m_state.HPOSSTRT.value;
        si.vpos = (s16)m_state.VPOSSTRT.value;
        si.sprdline = m_state.SPRDLINE.value;
        si.sprhsiz = m_state.SPRHSIZ.value;
        si.sprvsiz = m_state.SPRVSIZ.value;
        si.stretch = m_state.STRETCH.value;
        si.tilt = m_state.TILT.value;
        si.skipped = false;
        si.hoff = (s16)m_state.HOFF.value;
        si.voff = (s16)m_state.VOFF.value;
        memcpy(si.pen_map, m_state.pen_map, 16);
        m_frame_scb_list.push_back(si);
    }
#endif

    m_state.everon = false;

#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    if (m_trace_logger->IsEnabled(TRACE_SUZY_SPRITE))
    {
        GLYNX_Trace_Entry e = {};
        e.type = TRACE_SUZY_SPRITE;
        e.sprite.scb_addr = m_state.SCBADR.value;
        e.sprite.scb_next = m_state.SCBNEXT.value;
        e.sprite.hpos = (s16)m_state.HPOSSTRT.value;
        e.sprite.vpos = (s16)m_state.VPOSSTRT.value;
        e.sprite.sprctl0 = m_state.SPRCTL0;
        e.sprite.bpp = (u8)bpp;
        e.sprite.type = (u8)type;
        m_trace_logger->TraceLog(e);
    }
#endif

    s32 hoff = (s16)m_state.HOFF.value;
    s32 voff = (s16)m_state.VOFF.value;

    s32 base_hpos = (s16)m_state.HPOSSTRT.value - hoff;
    s32 base_vpos = (s16)m_state.VPOSSTRT.value - voff;

    int quadrant = 0;
    QuadPos pos = m_quad_lut[quadrant][start_quad][flip];
    QuadPos start_pos = pos;

    m_state.TILTACUM.value = 0;

    s32 dx = pos.left ? -1 : +1;
    s32 dy = pos.up ? -1 : +1;

    s32 cur_y = base_vpos;
    m_state.VSIZACUM.value = m_state.VSIZOFF.value;

    while (true)
    {
        u16 line_address = m_state.SPRDLINE.value;
        u8 sprdoff = RamRead(line_address);
        m_state.sprite_cycles += k_suzy_ram_read_ticks;  // sprdoff byte
        u16 next_ptr = (u16)(line_address + (u16)sprdoff);

        m_state.SPRDLINE.value = next_ptr;

        if (sprdoff <= 1)
        {
            m_state.sprite_cycles += k_suzy_control_line_ticks;

            // end of sprite
            if (sprdoff == 0)
            {
                break;
            }

            // advance to next quadrant
            quadrant = (quadrant + 1) & 3;
            pos = m_quad_lut[quadrant][start_quad][flip];

            dx = pos.left ? -1 : +1;
            dy = pos.up   ? -1 : +1;

            cur_y = base_vpos;
            m_state.VSIZACUM.value = m_state.VSIZOFF.value;

            if (pos.up != start_pos.up)
                cur_y += dy;

            continue;
        }

        u16 data_begin = (u16)(line_address + 1);
        u16 data_end   = next_ptr;

        m_state.VSIZACUM.value = m_state.VSIZACUM.value + m_state.SPRVSIZ.value;
        s16 pixel_height = (s16)(m_state.VSIZACUM.value >> 8);
        m_state.VSIZACUM.value &= 0x00FF;

        for (int row = 0; row < pixel_height; ++row)
        {
            s32 start_x = base_hpos;

            if (pos.left != start_pos.left)
                start_x += dx;

            u32 haccum_init = m_state.HSIZOFF.value;

            if (literal_only)
            {
                DrawSpriteLineLiteral(data_begin, data_end, start_x, cur_y, dx, bpp, type, m_state.SPRHSIZ.value, haccum_init, collide, collision_id);
            }
            else
            {
                DrawSpriteLinePacked(data_begin, data_end, start_x, cur_y, dx, bpp, type, m_state.SPRHSIZ.value, haccum_init, collide, collision_id);
            }

            bool visible_y = ((u32)cur_y < (u32)GLYNX_SCREEN_HEIGHT);
            u32 row_bus_ticks = visible_y ? k_suzy_visible_row_ticks : 0;
            if (visible_y && !literal_only)
            {
                if (reload_depth != 0)
                    row_bus_ticks += k_suzy_packed_reload_row_ticks;

                if (((u16)(data_end - data_begin) > 4))
                    row_bus_ticks -= k_suzy_packed_wide_row_discount_ticks;
            }
            else if (visible_y && (reload_depth != 0) && ((u16)(data_end - data_begin) > 16))
                row_bus_ticks += k_suzy_literal_wide_reload_row_ticks;

            m_state.sprite_cycles += row_bus_ticks;
            cur_y += dy;

            m_state.TILTACUM.value = (u16)(m_state.TILTACUM.value + m_state.TILT.value);
            s32 tilt_carry = (s16)m_state.TILTACUM.value >> 8;
            base_hpos += tilt_carry;
            m_state.TILTACUM.value &= 0x00FF;

            m_state.SPRHSIZ.value += m_state.STRETCH.value;
        }

        if (vertical_stretch)
        {
            s16 add = (s16)m_state.STRETCH.value * (s16)pixel_height;
            m_state.SPRVSIZ.value += add;
        }

    }

    u16 colpos = m_state.SCBADR.value + m_state.COLLOFF.value;

    if (!literal_only)
        m_state.sprite_cycles += k_suzy_packed_scb_ticks;

    if (collide)
    {
        switch (type)
        {
            case 2: // BOUNDARY-SHADOW
            case 3: // BOUNDARY
            case 4: // NORMAL
            case 6: // XOR
            case 7: // SHADOW
                RamWrite(colpos, m_state.fred);
                break;
            default:
                // BACKGROUND, BACKGROUND NON-COLLIDING, NON-COLLIDABLE
                break;
        }
    }

    if (IS_SET_BIT(m_state.SPRGO, 2))
    {
        u8 depository = RamRead(colpos);
        depository = m_state.everon ? UNSET_BIT(depository, 7) : SET_BIT(depository, 7);
        RamWrite(colpos, depository);
    }
}

INLINE void Suzy::DrawSpriteLineLiteral(u16 data_begin, u16 data_end,
                                        s32 x, s32 y, s32 dx,
                                        int bpp, int type, u16 hsiz, u32 haccum_init, bool collide, u8 collision_id)
{
    ShiftRegisterReset(data_begin);

    if (IS_SET_BIT(m_state.SPRCTL1, 3))
        m_state.sprite_cycles += k_suzy_literal_line_ticks;

    u32 h_accum = haccum_init;
    bool render = ((u32)y < (u32)GLYNX_SCREEN_HEIGHT);

    while (m_state.shift_register_address < data_end)
    {
        u32 pi = ShiftRegisterGetBits(bpp, data_end);
        if (pi == SHIFTREG_EOF)
            break;

        u8 pen = m_state.pen_map[pi & 0x0F];

        h_accum += (u32)hsiz;
        s32 pixel_count = (s32)(h_accum >> 8);
        h_accum &= 0xFF;

        if (pixel_count > 0)
        {
            if (render)
            {
                for (s32 p = 0; p < pixel_count; ++p)
                {
                    if (IS_SET_BIT(m_state.SPRCTL1, 3) && ((x & 1) == 0))
                        m_state.sprite_cycles += k_suzy_literal_byte_ticks;

                    DrawPixel(x, y, pen, type, collide, collision_id);
                    x += dx;
                }

                if ((dx > 0 && x >= GLYNX_SCREEN_WIDTH) || (dx < 0 && x < 0))
                    render = false;
            }
            else
            {
                x += dx * pixel_count;
            }
        }
    }
}

INLINE void Suzy::DrawSpriteLinePacked(u16 data_begin, u16 data_end,
                                       s32 x, s32 y, s32 dx,
                                       int bpp, int type, u16 hsiz, u32 haccum_init, bool collide, u8 collision_id)
{
    ShiftRegisterReset(data_begin);
    m_state.sprite_cycles += k_suzy_packed_line_ticks;
    m_state.pack_pixel_pair = 0;

    u32 h_accum = haccum_init;
    bool render = ((u32)y < (u32)GLYNX_SCREEN_HEIGHT);

    while (m_state.shift_register_address < data_end)
    {
        u32 header = ShiftRegisterGetBits(5, data_end);
        if (header == 0 || header == SHIFTREG_EOF)
            break;

        u32 is_literal = header >> 4;
        u32 count = (header & 0x0F) + 1;

        if (is_literal)
        {
            while (count--)
            {
                u32 pi = ShiftRegisterGetBits(bpp, data_end);
                if (pi == SHIFTREG_EOF)
                    return;

                u8 pen = m_state.pen_map[pi & 0x0F];
                AddPackedPixelTicks();

                h_accum += (u32)hsiz;
                s32 pixel_count = (s32)(h_accum >> 8);
                h_accum &= 0xFF;

                if (pixel_count > 0)
                {
                    if (render)
                    {
                        for (s32 p = 0; p < pixel_count; ++p)
                        {
                            DrawPixel(x, y, pen, type, collide, collision_id);
                            x += dx;
                        }

                        if ((dx > 0 && x >= GLYNX_SCREEN_WIDTH) || (dx < 0 && x < 0))
                            render = false;
                    }
                    else
                    {
                        x += dx * pixel_count;
                    }
                }
            }
        }
        else // RLE
        {
            u32 pixel_index = ShiftRegisterGetBits(bpp, data_end);
            if (pixel_index == SHIFTREG_EOF)
                return;

            u8 pen = m_state.pen_map[pixel_index & 0x0F];

            while (count--)
            {
                AddPackedPixelTicks();

                h_accum += (u32)hsiz;
                s32 pixel_count = (s32)(h_accum >> 8);
                h_accum &= 0xFF;

                if (pixel_count > 0)
                {
                    if (render)
                    {
                        for (s32 p = 0; p < pixel_count; ++p)
                        {
                            DrawPixel(x, y, pen, type, collide, collision_id);
                            x += dx;
                        }

                        if ((dx > 0 && x >= GLYNX_SCREEN_WIDTH) || (dx < 0 && x < 0))
                            render = false;
                    }
                    else
                    {
                        x += dx * pixel_count;
                    }
                }
            }
        }
    }
}

INLINE void Suzy::DrawPixel(s32 x, s32 y, u8 pen, int type, bool collide, u8 collision_id)
{
    if ((u32)x >= (u32)GLYNX_SCREEN_WIDTH)
        return;
    if ((u32)y >= (u32)GLYNX_SCREEN_HEIGHT)
        return;

    m_state.everon = true;
    bool transparent = false;
    bool non_collidable = false;

    switch (type & 0x07)
    {
        case 0: // BACKGROUND
        case 1: // BACKGROUND NON-COLLIDING
            transparent = false;
            non_collidable = true;
            break;
        case 2: // BOUNDARY-SHADOW
            transparent = (pen == 0x00) || (pen == 0x0F);
            non_collidable = (pen == 0x00) || (pen == 0x0E);
            break;
        case 3: // BOUNDARY
            transparent = (pen == 0x00) || (pen == 0x0F);
            non_collidable = (pen == 0x00);
            break;
        case 4: // NORMAL
            transparent = (pen == 0x00);
            non_collidable = transparent;
            break;
        case 5: // NON-COLLIDABLE
            transparent = (pen == 0x00);
            non_collidable = true;
            break;
        case 6: // XOR
        case 7: // SHADOW
            transparent = (pen == 0x00);
            non_collidable = (pen == 0x00) || (pen == 0x0E);
            break;
        default:
            // should not happen
            transparent = true;
            non_collidable = true;
            break;
    }

    if (transparent && non_collidable)
        return;

    u16 pixel_offset = (u16)(y * (GLYNX_SCREEN_WIDTH / 2)) + (u16)(x >> 1);
    bool is_left = ((x & 1) == 0);

    if (collide)
    {
        if (type == 0) // BACKGROUND
        {
            if (pen != 0x0E)
            {
                u16 coll_addr = m_state.COLLBAS.value + pixel_offset;
                u8 back = RamRead(coll_addr);

                if (is_left)
                {
                    back = (u8)((back & 0x0F) | (collision_id << 4));
                    m_state.sprite_cycles += 2;
                }
                else
                    back = (u8)((back & 0xF0) | (collision_id & 0x0F));

                RamWrite(coll_addr, back);
            }
        }
        else if (!non_collidable)
        {
            u16 coll_addr = m_state.COLLBAS.value + pixel_offset;
            u8 back = RamRead(coll_addr);
            u8 back_nib = is_left ? (back >> 4) : (back & 0x0F);

            if (back_nib > m_state.fred)
                m_state.fred = back_nib;

            if (is_left)
            {
                back = (u8)((back & 0x0F) | (collision_id << 4));
                m_state.sprite_cycles += k_suzy_rmw_ticks + k_suzy_process_ticks;  // left pixel: R-M-W + processing
            }
            else
                back = (u8)((back & 0xF0) | (collision_id & 0x0F));

            RamWrite(coll_addr, back);
        }
    }

    if (!transparent)
    {
        u16 video_addr = m_state.VIDBAS.value + pixel_offset;
        u8 video_byte = RamRead(video_addr);
        bool is_xor = ((type & 0x07) == 6);
        u8 new_nib = pen;

        if (unlikely(is_xor))
        {
            if (is_left)
            {
                new_nib ^= (u8)((video_byte >> 4) & 0x0F);
                video_byte = (u8)((video_byte & 0x0F) | ((new_nib & 0x0F) << 4));
                m_state.sprite_cycles += k_suzy_rmw_ticks + k_suzy_process_ticks;  // XOR left pixel: R-M-W + processing
            }
            else
            {
                new_nib ^= (u8)(video_byte & 0x0F);
                video_byte = (u8)((video_byte & 0xF0) | (new_nib & 0x0F));
            }
        }
        else
        {
            if (is_left)
            {
                video_byte = (u8)((video_byte & 0x0F) | ((new_nib & 0x0F) << 4));
                m_state.sprite_cycles += k_suzy_rmw_ticks + k_suzy_process_ticks;  // normal left pixel: R-M-W + processing
            }
            else
                video_byte = (u8)((video_byte & 0xF0) | (new_nib & 0x0F));
        }

        RamWrite(video_addr, video_byte);
    }
}

INLINE u8 Suzy::RamRead(u16 address)
{
    return m_ram[address];
}

INLINE u16 Suzy::RamReadWord(u16 address)
{
    return (u16)(m_ram[address] | (m_ram[(u16)(address + 1)] << 8));
}

INLINE void Suzy::RamWrite(u16 address, u8 value)
{
    m_ram[address] = value;
}

INLINE void Suzy::ShiftRegisterReset(u16 address)
{
    m_state.shift_register_address = address;
    m_state.shift_register_current = RamRead(address);
    m_state.shift_register_bit = 7;
    m_state.sprite_cycles += k_suzy_ram_read_ticks;  // initial sprite data byte
}

INLINE u32 Suzy::ShiftRegisterGetBits(int n, u16 stop_addr)
{
    if (m_state.shift_register_address >= stop_addr)
        return SHIFTREG_EOF;

    int bits_in_current_byte = (m_state.shift_register_bit + 1);
    u16 bytes_remaining_after_current = (u16)((stop_addr - 1) - m_state.shift_register_address);
    int remaining_bits = bits_in_current_byte + (int)bytes_remaining_after_current * 8;

    // Hardware quirk: refuse when exactly equal, dropping the last bit
    if (n >= remaining_bits)
        return SHIFTREG_EOF;

    // MSB-first
    u32 value = 0;
    int need = n;

    while (need > 0)
    {
        if (m_state.shift_register_bit < 0)
        {
            m_state.shift_register_address++;
            m_state.shift_register_current = RamRead(m_state.shift_register_address);
            m_state.shift_register_bit = 7;
            m_state.sprite_cycles += k_suzy_ram_read_ticks;  // next sprite data byte
        }

        value = (value << 1) | ((m_state.shift_register_current >> m_state.shift_register_bit) & 1);
        m_state.shift_register_bit--;
        need--;
    }

    return value;
}

INLINE void Suzy::UpdateMath(u32 cycles)
{
    if (m_state.math_cycles > 0)
    {
        if (m_state.math_cycles > cycles)
        {
            m_state.math_cycles -= cycles;
        }
        else
        {
            DebugSuzy("Math operation completed");
            m_state.math_cycles = 0;
            m_state.sprsys_mathbusy = false;
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
            if (m_trace_logger->IsEnabled(TRACE_SUZY_MATH))
            {
                GLYNX_Trace_Entry e = {};
                e.type = TRACE_SUZY_MATH;
                e.math.completed = true;
                m_trace_logger->TraceLog(e);
            }
#endif
        }
    }
}

INLINE bool Suzy::MathIsNegative(u16 value)
{
    if (value == 0)
        return true;
    if (value == 0x8000)
        return false;
    return (value & 0x8000) != 0;
}

#endif /* SUZY_INLINE_H */
