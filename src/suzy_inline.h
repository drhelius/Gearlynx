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
        return m_TMPADR.low;
    case SUZY_TMPADRH:     // 0xFC01
        return m_TMPADR.high;
    case SUZY_TILTACUML:   // 0xFC02
        return m_TILTACUM.low;
    case SUZY_TILTACUMH:   // 0xFC03
        return m_TILTACUM.high;
    case SUZY_HOFFL:       // 0xFC04
        return m_HOFF.low;
    case SUZY_HOFFH:       // 0xFC05
        return m_HOFF.high;
    case SUZY_VOFFL:       // 0xFC06
        return m_VOFF.low;
    case SUZY_VOFFH:       // 0xFC07
        return m_VOFF.high;
    case SUZY_VIDBASL:     // 0xFC08
        return m_VIDBAS.low;   
    case SUZY_VIDBASH:     // 0xFC09
        return m_VIDBAS.high;
    case SUZY_COLLBASL:    // 0xFC0A
        return m_COLLBAS.low;
    case SUZY_COLLBASH:    // 0xFC0B
        return m_COLLBAS.high;
    case SUZY_VIDADRL:     // 0xFC0C
        return m_VIDADR.low;
    case SUZY_VIDADRH:     // 0xFC0D
        return m_VIDADR.high;
    case SUZY_COLLADRL:    // 0xFC0E
        return m_COLLADR.low;
    case SUZY_COLLADRH:    // 0xFC0F
        return m_COLLADR.high;
    case SUZY_SCBNEXTL:    // 0xFC10
        return m_SCBNEXT.low;
    case SUZY_SCBNEXTH:    // 0xFC11
        return m_SCBNEXT.high;
    case SUZY_SPRDLINEL:   // 0xFC12
        return m_SPRDLINE.low;
    case SUZY_SPRDLINEH:   // 0xFC13
        return m_SPRDLINE.high;
    case SUZY_HPOSSTRTL:   // 0xFC14
        return m_HPOSSTRT.low;
    case SUZY_HPOSSTRTH:   // 0xFC15
        return m_HPOSSTRT.high;
    case SUZY_VPOSSTRTL:   // 0xFC16
        return m_VPOSSTRT.low;
    case SUZY_VPOSSTRTH:   // 0xFC17
        return m_VPOSSTRT.high;
    case SUZY_SPRHSIZL:    // 0xFC18
        return m_SPRHSIZ.low;
    case SUZY_SPRHSIZH:    // 0xFC19
        return m_SPRHSIZ.high;
    case SUZY_SPRVSIZL:    // 0xFC1A
        return m_SPRVSIZ.low;
    case SUZY_SPRVSIZH:    // 0xFC1B
        return m_SPRVSIZ.high;
    case SUZY_STRETCHL:    // 0xFC1C
        return m_STRETCH.low;
    case SUZY_STRETCHH:    // 0xFC1D
        return m_STRETCH.high;
    case SUZY_TILTL:       // 0xFC1E
        return m_TILT.low;
    case SUZY_TILTH:       // 0xFC1F
        return m_TILT.high;
    case SUZY_SPRDOFFL:    // 0xFC20
        return m_SPRDOFF.low;
    case SUZY_SPRDOFFH:    // 0xFC21
        return m_SPRDOFF.high;
    case SUZY_SPRVPOSL:    // 0xFC22
        return m_SPRVPOS.low;
    case SUZY_SPRVPOSH:    // 0xFC23
        return m_SPRVPOS.high;
    case SUZY_COLLOFFL:    // 0xFC24
        return m_COLLOFF.low;
    case SUZY_COLLOFFH:    // 0xFC25
        return m_COLLOFF.high;
    case SUZY_VSIZACUML:   // 0xFC26
        return m_VSIZACUM.low;
    case SUZY_VSIZACUMH:   // 0xFC27
        return m_VSIZACUM.high;
    case SUZY_HSIZOFFL:    // 0xFC28
        return m_HSIZOFF.low;
    case SUZY_HSIZOFFH:    // 0xFC29
        return m_HSIZOFF.high;
    case SUZY_VSIZOFFL:    // 0xFC2A
        return m_VSIZOFF.low;
    case SUZY_VSIZOFFH:    // 0xFC2B
        return m_VSIZOFF.high;
    case SUZY_SCBADRL:     // 0xFC2C
        return m_SCBADR.low;
    case SUZY_SCBADRH:     // 0xFC2D
        return m_SCBADR.high;
    case SUZY_PROCADRL:    // 0xFC2E
        return m_PROCADR.low;
    case SUZY_PROCADRH:    // 0xFC2F
        return m_PROCADR.high;
    case SUZY_MATHD:       // 0xFC52
        return m_MATHD;
    case SUZY_MATHC:       // 0xFC53
        return m_MATHC;
    case SUZY_MATHB:       // 0xFC54
        return m_MATHB;
    case SUZY_MATHA:       // 0xFC55
        return m_MATHA;
    case SUZY_MATHP:       // 0xFC56
        return m_MATHP;
    case SUZY_MATHN:       // 0xFC57
        return m_MATHN;
    case SUZY_MATHH:       // 0xFC60
        return m_MATHH;
    case SUZY_MATHG:       // 0xFC61
        return m_MATHG;
    case SUZY_MATHF:       // 0xFC62
        return m_MATHF;
    case SUZY_MATHE:       // 0xFC63
        return m_MATHE;
    case SUZY_MATHM:       // 0xFC6C
        return m_MATHM;
    case SUZY_MATHL:       // 0xFC6D
        return m_MATHL;
    case SUZY_MATHK:       // 0xFC6E
        return m_MATHK;
    case SUZY_MATHJ:       // 0xFC6F
        return m_MATHJ;
    case SUZY_SPRCTL0:     // 0xFC80
        DebugSuzy("Reading write-only SPRCTL0: %02X", m_SPRCTL0);
        return 0xFF;
    case SUZY_SPRCTL1:     // 0xFC81
        DebugSuzy("Reading write-only SPRCTL1: %02X", m_SPRCTL1);
        return 0xFF;
    case SUZY_SPRCOLL:     // 0xFC82
        DebugSuzy("Reading write-only SPRCOLL: %02X", m_SPRCOLL);
        return 0xFF;
    case SUZY_SPRINIT:     // 0xFC83
        DebugSuzy("Reading write-only SPRINIT: %02X", m_SPRINIT);
        return 0xFF;
    case SUZY_SUZYHREV:    // 0xFC88
        return 0x01;
    case SUZY_SUZYSREV:    // 0xFC89
        return 0xFF;
    case SUZY_SUZYBUSEN:   // 0xFC90
        DebugSuzy("Reading write-only SUZYBUSEN: %02X", m_SUZYBUSEN);
        return 0xFF;
    case SUZY_SPRGO:       // 0xFC91
        DebugSuzy("Reading write-only SPRGO: %02X", m_SPRGO);
        return 0xFF;
    case SUZY_SPRSYS:      // 0xFC92
        DebugSuzy("Reading SPRSYS: %02X", m_SPRSYS);
        return 0;
    case SUZY_JOYSTICK:    // 0xFCB0
        return m_JOYSTICK;
    case SUZY_SWITCHES:    // 0xFCB1
        return m_SWITCHES;
    case SUZY_RCART0:      // 0xFCB2
        return m_cartridge->ReadBank0();
    case SUZY_RCART1:      // 0xFCB3
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
        m_TMPADR.value = value;
        break;
    case SUZY_TMPADRH:     // 0xFC01
        m_TMPADR.high = value;
        break;
    case SUZY_TILTACUML:   // 0xFC02
        m_TILTACUM.value = value;
        break;
    case SUZY_TILTACUMH:   // 0xFC03
        m_TILTACUM.high = value;
        break;
    case SUZY_HOFFL:       // 0xFC04
        m_HOFF.value = value;
        break;
    case SUZY_HOFFH:       // 0xFC05
        m_HOFF.high = value;
        break;
    case SUZY_VOFFL:       // 0xFC06
        m_VOFF.value = value;
        break;
    case SUZY_VOFFH:       // 0xFC07
        m_VOFF.high = value;
        break;
    case SUZY_VIDBASL:     // 0xFC08
        m_VIDBAS.value = value;
        break;
    case SUZY_VIDBASH:     // 0xFC09
        m_VIDBAS.high = value;
        break;
    case SUZY_COLLBASL:    // 0xFC0A
        m_COLLBAS.value = value;
        break;
    case SUZY_COLLBASH:    // 0xFC0B
        m_COLLBAS.high = value;
        break;
    case SUZY_VIDADRL:     // 0xFC0C
        m_VIDADR.value = value;
        break;
    case SUZY_VIDADRH:     // 0xFC0D
        m_VIDADR.high = value;
        break;
    case SUZY_COLLADRL:    // 0xFC0E
        m_COLLADR.value = value;
        break;
    case SUZY_COLLADRH:    // 0xFC0F
        m_COLLADR.high = value;
        break;
    case SUZY_SCBNEXTL:    // 0xFC10
        m_SCBNEXT.value = value;
        break;
    case SUZY_SCBNEXTH:    // 0xFC11
        m_SCBNEXT.high = value;
        break;
    case SUZY_SPRDLINEL:   // 0xFC12
        m_SPRDLINE.value = value;
        break;
    case SUZY_SPRDLINEH:   // 0xFC13
        m_SPRDLINE.high = value;
        break;
    case SUZY_HPOSSTRTL:   // 0xFC14
        m_HPOSSTRT.value = value;
        break;
    case SUZY_HPOSSTRTH:   // 0xFC15
        m_HPOSSTRT.high = value;
        break;
    case SUZY_VPOSSTRTL:   // 0xFC16
        m_VPOSSTRT.value = value;
        break;
    case SUZY_VPOSSTRTH:   // 0xFC17
        m_VPOSSTRT.high = value;
        break;
    case SUZY_SPRHSIZL:    // 0xFC18
        m_SPRHSIZ.value = value;
        break;
    case SUZY_SPRHSIZH:    // 0xFC19
        m_SPRHSIZ.high = value;
        break;
    case SUZY_SPRVSIZL:    // 0xFC1A
        m_SPRVSIZ.value = value;
        break;
    case SUZY_SPRVSIZH:    // 0xFC1B
        m_SPRVSIZ.high = value;
        break;
    case SUZY_STRETCHL:    // 0xFC1C
        m_STRETCH.value = value;
        break;
    case SUZY_STRETCHH:    // 0xFC1D
        m_STRETCH.high = value;
        break;
    case SUZY_TILTL:       // 0xFC1E
        m_TILT.value = value;
        break;
    case SUZY_TILTH:       // 0xFC1F
        m_TILT.high = value;
        break;
    case SUZY_SPRDOFFL:    // 0xFC20
        m_SPRDOFF.value = value;
        break;
    case SUZY_SPRDOFFH:    // 0xFC21
        m_SPRDOFF.high = value;
        break;
    case SUZY_SPRVPOSL:    // 0xFC22
        m_SPRVPOS.value = value;
        break;
    case SUZY_SPRVPOSH:    // 0xFC23
        m_SPRVPOS.high = value;
        break;
    case SUZY_COLLOFFL:    // 0xFC24
        m_COLLOFF.value = value;
        break;
    case SUZY_COLLOFFH:    // 0xFC25
        m_COLLOFF.high = value;
        break;
    case SUZY_VSIZACUML:   // 0xFC26
        m_VSIZACUM.value = value;
        break;
    case SUZY_VSIZACUMH:   // 0xFC27
        m_VSIZACUM.high = value;
        break;
    case SUZY_HSIZOFFL:    // 0xFC28
        m_HSIZOFF.value = value;
        break;
    case SUZY_HSIZOFFH:    // 0xFC29
        m_HSIZOFF.high = value;
        break;
    case SUZY_VSIZOFFL:    // 0xFC2A
        m_VSIZOFF.value = value;
        break;
    case SUZY_VSIZOFFH:    // 0xFC2B
        m_VSIZOFF.high = value;
        break;
    case SUZY_SCBADRL:     // 0xFC2C
        m_SCBADR.value = value;
        break;
    case SUZY_SCBADRH:     // 0xFC2D
        m_SCBADR.high = value;
        break;
    case SUZY_PROCADRL:    // 0xFC2E
        m_PROCADR.value = value;
        break;
    case SUZY_PROCADRH:    // 0xFC2F
        m_PROCADR.high = value;
        break;
    case SUZY_MATHD:       // 0xFC52
        m_MATHD = value;
        Write(SUZY_MATHC, 0);
        break;
    case SUZY_MATHC:       // 0xFC53
        m_MATHC = value;
        break;
    case SUZY_MATHB:       // 0xFC54
        m_MATHB = value;
        Write(SUZY_MATHA, 0);
        break;
    case SUZY_MATHA:       // 0xFC55
        m_MATHA = value;
        break;
    case SUZY_MATHP:       // 0xFC56
        m_MATHP = value;
        m_MATHN = 0;
        break;
    case SUZY_MATHN:       // 0xFC57
        m_MATHN = value;
        break;
    case SUZY_MATHH:       // 0xFC60
        m_MATHH = value;
        m_MATHG = 0;
        break;
    case SUZY_MATHG:       // 0xFC61
        m_MATHG = value;
        break;
    case SUZY_MATHF:       // 0xFC62
        m_MATHF = value;
        m_MATHE = 0;
        break;
    case SUZY_MATHE:       // 0xFC63
        m_MATHE = value;
        break;
    case SUZY_MATHM:       // 0xFC6C
        m_MATHM = value;
        m_MATHL = 0;
        break;
    case SUZY_MATHL:       // 0xFC6D
        m_MATHL = value;
        break;
    case SUZY_MATHK:       // 0xFC6E
        m_MATHK = value;
        m_MATHJ = 0;
        break;
    case SUZY_MATHJ:       // 0xFC6F
        m_MATHJ = value;
        break;
    case SUZY_SPRCTL0:     // 0xFC80
        m_SPRCTL0 = value;
        break;
    case SUZY_SPRCTL1:     // 0xFC81
        m_SPRCTL1 = value;
        break;
    case SUZY_SPRCOLL:     // 0xFC82
        m_SPRCOLL = value;
        break;
    case SUZY_SPRINIT:     // 0xFC83
        m_SPRINIT = value;
        break;
    case SUZY_SUZYHREV:    // 0xFC88
        DebugSuzy("Writing to read-only SUZYHREV: %02X", value);
        break;
    case SUZY_SUZYSREV:    // 0xFC89
        DebugSuzy("Writing to read-only SUZYSREV: %02X", value);
        break;
    case SUZY_SUZYBUSEN:   // 0xFC90
        m_SUZYBUSEN = value;
        break;
    case SUZY_SPRGO:       // 0xFC91
        m_SPRGO = value;
        break;
    case SUZY_SPRSYS:      // 0xFC92
        m_SPRSYS = value;
        break;
    case SUZY_JOYSTICK:    // 0xFCB0
        DebugSuzy("Writing to read-only JOYSTICK: %02X", value);
        break;
    case SUZY_SWITCHES:    // 0xFCB1
        DebugSuzy("Writing to read-only SWITCHES: %02X", value);
        break;
    case SUZY_RCART0:      // 0xFCB2
        m_cartridge->WriteBank0(value);
        break;
    case SUZY_RCART1:      // 0xFCB3
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

#endif /* SUZY_INLINE_H */
