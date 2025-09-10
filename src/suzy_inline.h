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
        DebugSuzy("Reading JOYSTICK: %02X", m_input->ReadJoystick());
        return m_input->ReadJoystick();
    case SUZY_SWITCHES:    // 0xFCB1
        DebugSuzy("Reading SWITCHES: %02X", m_input->ReadSwitches());
        return m_input->ReadSwitches();
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

    while ((m_state.SCBNEXT.value & 0xFF00) != 0)
    {
        DrawSprite();
    }

    m_state.SPRGO = 0x00;
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

    if (IS_SET_BIT(m_state.SPRCTL1, 2))
    {
        DebugSuzy("Skipping sprite at SCB %04X due to SPRCTL1 bit 2 set", m_state.SCBADR.value);
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

    bool vertical_stretch = IS_SET_BIT(m_state.SPRSYS, 4);

    m_state.SPRDLINE.value = RamReadWord(m_state.TMPADR.value);
    m_state.TMPADR.value += 2;

    m_state.HPOSSTRT.value = RamReadWord(m_state.TMPADR.value);
    m_state.TMPADR.value += 2;
    m_state.VPOSSTRT.value = RamReadWord(m_state.TMPADR.value);
    m_state.TMPADR.value += 2;

    m_state.STRETCH.value = 0;
    m_state.TILT.value = 0;

    if (reload_depth == 1)
    {
        m_state.SPRHSIZ.value = RamReadWord(m_state.TMPADR.value);
        m_state.TMPADR.value += 2;
        m_state.SPRVSIZ.value = RamReadWord(m_state.TMPADR.value);
        m_state.TMPADR.value += 2;
    }
    else if (reload_depth == 2)
    {
        m_state.SPRHSIZ.value = RamReadWord(m_state.TMPADR.value);
        m_state.TMPADR.value += 2;
        m_state.SPRVSIZ.value = RamReadWord(m_state.TMPADR.value);
        m_state.TMPADR.value += 2;
        m_state.STRETCH.value = RamReadWord(m_state.TMPADR.value);
        m_state.TMPADR.value += 2;
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
    }

    if (reload_palette)
    {
        int colors = 1 << bpp;
        int bytes_to_read = colors >> 1;

        for (int i = 0; i < bytes_to_read; ++i)
        {
            u8 byte = RamRead(m_state.TMPADR.value++);
            m_state.pen_map[(i << 1) + 0] = (byte >> 4) & 0x0F;
            m_state.pen_map[(i << 1) + 1] = (byte & 0x0F);
        }
    }

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
    m_state.VSIZACUM.value = pos.up ? 0 : m_state.VSIZOFF.value; // 8.8 accumulator persistente en registro

    while (m_state.SPRDLINE.value != 0)
    {
        u8 sprdoff  = RamRead(m_state.SPRDLINE.value);
        u16 next_ptr = (u16)(m_state.SPRDLINE.value + (u16)sprdoff);

        u16 data_begin = (u16)(m_state.SPRDLINE.value + 1);
        u16 data_end   = next_ptr;

        m_state.VSIZACUM.value = m_state.VSIZACUM.value + m_state.SPRVSIZ.value;
        s16 pixel_height = (s16)(m_state.VSIZACUM.value >> 8);
        m_state.VSIZACUM.value &= 0x00FF;

        for (int row = 0; row < pixel_height; ++row)
        {
            s32 start_x = base_hpos;

            if (pos.left != start_pos.left)
                start_x += dx;

            u32 haccum_init = pos.left ? 0u : m_state.HSIZOFF.value;

            if (literal_only)
            {
                DrawSpriteLineLiteral(data_begin, data_end, start_x, cur_y, dx, bpp, type, m_state.SPRHSIZ.value, haccum_init);
            }
            else
            {
                DrawSpriteLinePacked(data_begin, data_end, start_x, cur_y, dx, bpp, type, m_state.SPRHSIZ.value, haccum_init);
            }

            cur_y += dy;

            m_state.TILTACUM.value = (u16)(m_state.TILTACUM.value + m_state.TILT.value);
            s32 tilt_carry = (s16)m_state.TILTACUM.value >> 8; // desplazamiento aritmético (tilt con signo)
            base_hpos += tilt_carry;
            m_state.TILTACUM.value &= 0x00FF;

            m_state.SPRHSIZ.value += m_state.STRETCH.value;
        }

        if (vertical_stretch)
        {
            s16 add = (s16)m_state.STRETCH.value * (s16)pixel_height;
            m_state.SPRVSIZ.value += add;
        }

        // end of sprite
        if (sprdoff == 0)
        {
            break;
        }
        // advance to next quadrant
        else if (sprdoff == 1)
        {
            quadrant = (quadrant + 1) & 3;
            pos = m_quad_lut[quadrant][start_quad][flip];

            dx = pos.left ? -1 : +1;
            dy = pos.up   ? -1 : +1;

            cur_y  = base_vpos;
            m_state.VSIZACUM.value = pos.up ? 0 : m_state.VSIZOFF.value;

            if (pos.up != start_pos.up)
                cur_y += dy;
        }

        m_state.SPRDLINE.value = next_ptr;
    }
}

INLINE void Suzy::DrawSpriteLineLiteral(u16 data_begin, u16 data_end,
                                        s32 x, s32 y, s32 dx,
                                        int bpp, int type, u16 hsiz, u32 haccum_init)
{
    ShiftRegisterReset(data_begin);

    u32 h_accum = haccum_init; // start with HSIZOFF when drawing right

    while (m_shift_register_address < data_end)
    {
        u32 pi = ShiftRegisterGetBits(bpp, data_end);
        u8 pen = m_state.pen_map[pi & 0x0F];

        h_accum += (u32)hsiz;

        while (h_accum >= 0x100)
        {
            DrawPixel(x, y, pen, type);
            x += dx;
            h_accum -= 0x100;
        }
    }
}

INLINE void Suzy::DrawSpriteLinePacked(u16 data_begin, u16 data_end,
                                       s32 x, s32 y, s32 dx,
                                       int bpp, int type, u16 hsiz, u32 haccum_init)
{
    ShiftRegisterReset(data_begin);

    u32 h_accum = haccum_init; // start with HSIZOFF when drawing right

    while (m_shift_register_address < data_end)
    {
        u32 header = ShiftRegisterGetBits(5, data_end);
        if (header == 0)
            break; // early EOL

        u32 is_literal = header >> 4;
        u32 count = (header & 0x0F) + 1;

        if (is_literal)
        {
            while (count--)
            {
                u32 pi = ShiftRegisterGetBits(bpp, data_end);
                u8 pen = m_state.pen_map[pi & 0x0F];

                h_accum += (u32)hsiz;
                while (h_accum >= 0x100)
                {
                    DrawPixel(x, y, pen, type);
                    x += dx;
                    h_accum -= 0x100;
                }
            }
        }
        else // RLE
        {
            u32 pixel_index = ShiftRegisterGetBits(bpp, data_end);
            u8 pen = m_state.pen_map[pixel_index & 0x0F];

            while (count--)
            {
                h_accum += (u32)hsiz;
                while (h_accum >= 0x100)
                {
                    DrawPixel(x, y, pen, type);
                    x += dx;
                    h_accum -= 0x100;
                }
            }
        }
    }
}

INLINE void Suzy::DrawPixel(s32 x, s32 y, u8 pen, int type)
{
    if (IsPixelTransparent(pen, type))
        return;

    // Screen-space clip (super-clip window already applied in callers via HOFF/VOFF)
    if ((u32)x >= (u32)GLYNX_SCREEN_WIDTH)
        return;
    if ((u32)y >= (u32)GLYNX_SCREEN_HEIGHT)
        return;

    u16 base = m_state.VIDBAS.value;
    u16 addr = (u16)(base + (u16)(y * (GLYNX_SCREEN_WIDTH / 2)) + (u16)(x >> 1));
    u8 byte = RamRead(addr);

    const bool is_xor = (type == 6);

    if ((x & 1) == 0)
    {
        // left pixel -> high nibble
        u8 new_nib = pen;
        if (is_xor)
            new_nib ^= (byte >> 4) & 0x0F;
        byte = (u8)((byte & 0x0F) | (new_nib << 4));
    }
    else
    {
        // right pixel -> low nibble
        u8 new_nib = pen;
        if (is_xor)
            new_nib ^= (byte & 0x0F);
        byte = (u8)((byte & 0xF0) | (new_nib & 0x0F));
    }

    RamWrite(addr, byte);
}

INLINE bool Suzy::IsPixelTransparent(u8 pen, int type)
{
    switch (type & 0x07)
    {
        case 0: // BACKGROUND
        case 1: // BACKGROUND NON-COLLIDING
            return false;
        case 2: // BOUNDARY-SHADOW
        case 3: // BOUNDARY
            return (pen == 0x00) || (pen == 0x0F);
        case 4: // NORMAL
        case 5: // NON-COLLIDABLE
        case 6: // XOR
        case 7: // SHADOW
            return (pen == 0x00);
        default:
            return false; // should not happen
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

#endif /* SUZY_INLINE_H */
