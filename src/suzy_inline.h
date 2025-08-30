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
#include "mikey.h"
#include "cartridge.h"
#include "m6502.h"

INLINE void Suzy::Clock(u32 cycles)
{

}

INLINE u8 Suzy::Read(u16 address)
{
    switch(address)
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
    case SUZY_MATHD:       // 0xFC52
        return m_state.MATHD;
    case SUZY_MATHC:       // 0xFC53
        return m_state.MATHC;
    case SUZY_MATHB:       // 0xFC54
        return m_state.MATHB;
    case SUZY_MATHA:       // 0xFC55
        return m_state.MATHA;
    case SUZY_MATHP:       // 0xFC56
        return m_state.MATHP;
    case SUZY_MATHN:       // 0xFC57
        return m_state.MATHN;
    case SUZY_MATHH:       // 0xFC60
        return m_state.MATHH;
    case SUZY_MATHG:       // 0xFC61
        return m_state.MATHG;
    case SUZY_MATHF:       // 0xFC62
        return m_state.MATHF;
    case SUZY_MATHE:       // 0xFC63
        return m_state.MATHE;
    case SUZY_MATHM:       // 0xFC6C
        return m_state.MATHM;
    case SUZY_MATHL:       // 0xFC6D
        return m_state.MATHL;
    case SUZY_MATHK:       // 0xFC6E
        return m_state.MATHK;
    case SUZY_MATHJ:       // 0xFC6F
        return m_state.MATHJ;
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
        DebugSuzy("Reading SPRSYS: %02X", m_state.SPRSYS);
        return 0;
    case SUZY_JOYSTICK:    // 0xFCB0
        DebugSuzy("Reading JOYSTICK: %02X", m_state.JOYSTICK);
        return m_state.JOYSTICK;
    case SUZY_SWITCHES:    // 0xFCB1
        DebugSuzy("Reading SWITCHES: %02X", m_state.SWITCHES);
        return m_state.SWITCHES;
    case SUZY_RCART0:      // 0xFCB2
        DebugSuzy("Reading RCART0");
        return m_cartridge->ReadBank0();
    case SUZY_RCART1:      // 0xFCB3
        DebugSuzy("Reading RCART1");
        return m_cartridge->ReadBank1();
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

INLINE void Suzy::Write(u16 address, u8 value)
{
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
    case SUZY_MATHD:       // 0xFC52
        m_state.MATHD = value;
        Write(SUZY_MATHC, 0);
        break;
    case SUZY_MATHC:       // 0xFC53
        m_state.MATHC = value;
        break;
    case SUZY_MATHB:       // 0xFC54
        m_state.MATHB = value;
        Write(SUZY_MATHA, 0);
        break;
    case SUZY_MATHA:       // 0xFC55
        m_state.MATHA = value;
        break;
    case SUZY_MATHP:       // 0xFC56
        m_state.MATHP = value;
        m_state.MATHN = 0;
        break;
    case SUZY_MATHN:       // 0xFC57
        m_state.MATHN = value;
        break;
    case SUZY_MATHH:       // 0xFC60
        m_state.MATHH = value;
        m_state.MATHG = 0;
        break;
    case SUZY_MATHG:       // 0xFC61
        m_state.MATHG = value;
        break;
    case SUZY_MATHF:       // 0xFC62
        m_state.MATHF = value;
        m_state.MATHE = 0;
        break;
    case SUZY_MATHE:       // 0xFC63
        m_state.MATHE = value;
        break;
    case SUZY_MATHM:       // 0xFC6C
        m_state.MATHM = value;
        m_state.MATHL = 0;
        break;
    case SUZY_MATHL:       // 0xFC6D
        m_state.MATHL = value;
        break;
    case SUZY_MATHK:       // 0xFC6E
        m_state.MATHK = value;
        m_state.MATHJ = 0;
        break;
    case SUZY_MATHJ:       // 0xFC6F
        m_state.MATHJ = value;
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
        if (value & 0x01)
            SpritesGo();
        break;
    case SUZY_SPRSYS:      // 0xFC92
        DebugSuzy("Setting SPRSYS to %02X (was %02X)", value, m_state.SPRSYS);
        m_state.SPRSYS = value;
        break;
    case SUZY_JOYSTICK:    // 0xFCB0
        DebugSuzy("Writing to read-only JOYSTICK: %02X", value);
        break;
    case SUZY_SWITCHES:    // 0xFCB1
        DebugSuzy("Writing to read-only SWITCHES: %02X", value);
        break;
    case SUZY_RCART0:      // 0xFCB2
        DebugSuzy("Writing to RCART0: %02X", value);
        m_cartridge->WriteBank0(value);
        break;
    case SUZY_RCART1:      // 0xFCB3
        DebugSuzy("Writing to RCART1: %02X", value);
        m_cartridge->WriteBank1(value);
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

INLINE void Suzy::SpritesGo()
{
    DebugSuzy("SpritesGo called: SPRCTL0=%02X, SPRCTL1=%02X, SPRCOLL=%02X, SPRINIT=%02X, SPRSYS=%02X",
              m_state.SPRCTL0, m_state.SPRCTL1, m_state.SPRCOLL, m_state.SPRINIT, m_state.SPRSYS);

    u16 scb = m_state.SCBNEXT.value;
    int index = 0;

    while ((scb & 0xFF00) != 0)
    {
        DebugSuzy("Drawing sprite %d at SCB %04X", index++, scb);
        u16 next_in_list = RamReadWord(scb + 3);
        DrawSprite(scb);
        scb = next_in_list;
    }

    m_state.SPRGO = 0x00;
}

INLINE void Suzy::DrawSprite(u16 scb_address)
{
    u16 scb = scb_address;

    u8 sprctl0 = RamRead(scb + 0);
    u8 sprctl1 = RamRead(scb + 1);
    // u8 sprcoll = RamRead(scb + 2);

    u16 data_ptr = RamReadWord(scb + 5);
    s32 hpos = (s32)RamReadWord(scb + 7);
    s32 vpos = (s32)RamReadWord(scb + 9);
    // u16 sprhsiz = RamReadWord(scb + 11);
    // u16 sprvsiz = RamReadWord(scb + 13);
    // u16 stretch = RamReadWord(scb + 15);
    // u16 tilt = RamReadWord(scb + 17);

    // Pen index palette: 8 bytes -> 16 nibbles
    u8 penmap[16];

    for (int i = 0; i < 8; ++i)
    {
        u8 b = RamRead(scb + 19 + i);
        penmap[(i << 1) + 0] = (b >> 4) & 0x0F;
        penmap[(i << 1) + 1] = b & 0x0F;
    }

    int bpp = ((sprctl0 >> 6) & 0x03) + 1;

    const bool literal_only = (sprctl1 & 0x80) != 0;

    // SE
    s32 dx = +1;
    s32 dy = +1;

    s32 cur_y = (dy < 0) ? vpos - 1 : vpos;

    int quad_rotations = 0;

    while (data_ptr != 0)
    {
        u16 line_base = data_ptr;
        u8  offset = RamRead(line_base);
        u16 next_ptr = (u16)(line_base + (u16)offset);

        // avoid infinite loops on malformed data
        if (next_ptr == line_base)
        {
            break;
        }

        s32 cur_x = (dx < 0) ? hpos - 1 : hpos;

        u16 data_begin = line_base + 1;
        u16 data_end = next_ptr;

        if (literal_only)
        {
            DrawSpriteLineLiteral(data_begin, data_end, cur_x, cur_y, dx, penmap, bpp);
        }
        else
        {
            DrawSpriteLinePacked(data_begin, data_end, cur_x, cur_y, dx, penmap, bpp);
        }

        if (offset == 0)
        {
            // End of sprite
            break;
        }
        else if (offset == 1)
        {
            // Next quadrant
            // SE -> NE -> NW -> SW
            if ((quad_rotations & 1) == 0)
                dy = -dy; // 0,2 -> flip Y
            else
                dx = -dx; // 1,3 -> flip X

            quad_rotations++;
            cur_y = (dy < 0) ? vpos - 1 : vpos;
        }
        else
        {
            // Same quadrant, next scanline
            cur_y += dy;
        }

        data_ptr = next_ptr;
    }
}

INLINE void Suzy::DrawSpriteLineLiteral(u16 data_begin, u16 data_end, s32 x0, s32 y, s32 dx, u8* penmap, int bpp)
{
    ShiftRegisterReset(data_begin);
    s32 x = x0;

    while (m_shift_register_address < data_end)
    {
        u32 pi = ShiftRegisterGetBits(bpp, data_end);
        u8 pen = penmap[pi & 0x0F];
        DrawPixel(x, y, pen);
        x += dx;
    }
}

INLINE void Suzy::DrawSpriteLinePacked(u16 data_begin, u16 data_end, s32 x0, s32 y, s32 dx, u8* penmap, int bpp)
{
    ShiftRegisterReset(data_begin);
    s32 x = x0;

    while (m_shift_register_address < data_end)
    {
        // Early EOL detector: 00000 header (type=0,len-1=0)
        if (ShiftRegisterPeek5(data_end) == 0)
        {
            // consume it and end the line
            (void)ShiftRegisterGetBits(5, data_end);
            break;
        }

        u32 is_lit = ShiftRegisterGetBits(1, data_end);
        u32 cnt    = ShiftRegisterGetBits(4, data_end) + 1;

        if (is_lit)
        {
            // literal: cnt pixels follow, each bpp bits
            while (cnt--)
            {
                u32 pi = ShiftRegisterGetBits(bpp, data_end);
                u8 pen = penmap[pi & 0x0F];
                DrawPixel(x, y, pen);
                x += dx;
            }
        }
        else
        {
            // RLE: one color index, repeated cnt times
            u32 pi = ShiftRegisterGetBits(bpp, data_end);
            u8 pen = penmap[pi & 0x0F];
            while (cnt--)
            {
                DrawPixel(x, y, pen);
                x += dx;
            }
        }
    }
}

INLINE void Suzy::DrawPixel(s32 x, s32 y, u8 pen)
{
    if ((pen & 0x0F) == 0)
        return; // color 0 is transparent

    // HOFF/VOFF define the offset from virtual origin (0,0) to the visible top-left.
    // Therefore convert world/virtual coords -> VRAM coords by subtracting offsets.
    const s32 hoff = (s32)m_state.HOFF.value;
    const s32 voff = (s32)m_state.VOFF.value;
    const s32 eff_x = x - hoff;
    const s32 eff_y = y - voff;

    if ((u32)eff_x >= (u32)GLYNX_SCREEN_WIDTH)
        return;
    if ((u32)eff_y >= (u32)GLYNX_SCREEN_HEIGHT)
        return;


    u16 base = m_state.VIDBAS.value;
    u16 addr = base + (u16)(eff_y * (GLYNX_SCREEN_WIDTH / 2)) + (u16)(eff_x >> 1);
    u8  old  = RamRead(addr);

    if ((eff_x & 1) == 0)
    {
        // left pixel -> high nibble
        old = (old & 0x0F) | (pen << 4);
    }
    else
    {
        // right pixel -> low nibble
        old = (old & 0xF0) | (pen & 0x0F);
    }

    RamWrite(addr, old);
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
    m_shift_register_address = address;
    m_shift_register_current = RamRead(address);
    m_shift_register_bit = 7;
}

INLINE u32 Suzy::ShiftRegisterGetBits(int n, u16 stop_addr)
{
    // MSB-first
    u32 value = 0;

    while (n > 0)
    {
        if (m_shift_register_bit < 0)
        {
            m_shift_register_address++;

            if (m_shift_register_address >= stop_addr)
            {
                // Clamp: further reads would overrun. Return what we have
                break;
            }

            m_shift_register_current = RamRead(m_shift_register_address);
            m_shift_register_bit = 7;
        }

        value = (value << 1) | ((m_shift_register_current >> m_shift_register_bit) & 1);
        m_shift_register_bit--;
        n--;
    }

    return value;
}

INLINE u32 Suzy::ShiftRegisterPeek5(u16 stop_addr)
{
    // Save state, read 5 bits, then restore

    u16 prev_address = m_shift_register_address;
    u8  prev_current  = m_shift_register_current;
    int prev_bit  = m_shift_register_bit;

    u32 value = ShiftRegisterGetBits(5, stop_addr);

    m_shift_register_address = prev_address;
    m_shift_register_current  = prev_current;
    m_shift_register_bit  = prev_bit;

    return value;
}

#endif /* SUZY_INLINE_H */
