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

void M6502::InitOPCodeFunctors()
{
    m_opcodes[0x00] = &M6502::OPCode0x00;
    m_opcodes[0x01] = &M6502::OPCode0x01;
    m_opcodes[0x02] = &M6502::OPCode0x02;
    m_opcodes[0x03] = &M6502::OPCode0x03;
    m_opcodes[0x04] = &M6502::OPCode0x04;
    m_opcodes[0x05] = &M6502::OPCode0x05;
    m_opcodes[0x06] = &M6502::OPCode0x06;
    m_opcodes[0x07] = &M6502::OPCode0x07;
    m_opcodes[0x08] = &M6502::OPCode0x08;
    m_opcodes[0x09] = &M6502::OPCode0x09;
    m_opcodes[0x0A] = &M6502::OPCode0x0A;
    m_opcodes[0x0B] = &M6502::OPCode0x0B;
    m_opcodes[0x0C] = &M6502::OPCode0x0C;
    m_opcodes[0x0D] = &M6502::OPCode0x0D;
    m_opcodes[0x0E] = &M6502::OPCode0x0E;
    m_opcodes[0x0F] = &M6502::OPCode0x0F;

    m_opcodes[0x10] = &M6502::OPCode0x10;
    m_opcodes[0x11] = &M6502::OPCode0x11;
    m_opcodes[0x12] = &M6502::OPCode0x12;
    m_opcodes[0x13] = &M6502::OPCode0x13;
    m_opcodes[0x14] = &M6502::OPCode0x14;
    m_opcodes[0x15] = &M6502::OPCode0x15;
    m_opcodes[0x16] = &M6502::OPCode0x16;
    m_opcodes[0x17] = &M6502::OPCode0x17;
    m_opcodes[0x18] = &M6502::OPCode0x18;
    m_opcodes[0x19] = &M6502::OPCode0x19;
    m_opcodes[0x1A] = &M6502::OPCode0x1A;
    m_opcodes[0x1B] = &M6502::OPCode0x1B;
    m_opcodes[0x1C] = &M6502::OPCode0x1C;
    m_opcodes[0x1D] = &M6502::OPCode0x1D;
    m_opcodes[0x1E] = &M6502::OPCode0x1E;
    m_opcodes[0x1F] = &M6502::OPCode0x1F;

    m_opcodes[0x20] = &M6502::OPCode0x20;
    m_opcodes[0x21] = &M6502::OPCode0x21;
    m_opcodes[0x22] = &M6502::OPCode0x22;
    m_opcodes[0x23] = &M6502::OPCode0x23;
    m_opcodes[0x24] = &M6502::OPCode0x24;
    m_opcodes[0x25] = &M6502::OPCode0x25;
    m_opcodes[0x26] = &M6502::OPCode0x26;
    m_opcodes[0x27] = &M6502::OPCode0x27;
    m_opcodes[0x28] = &M6502::OPCode0x28;
    m_opcodes[0x29] = &M6502::OPCode0x29;
    m_opcodes[0x2A] = &M6502::OPCode0x2A;
    m_opcodes[0x2B] = &M6502::OPCode0x2B;
    m_opcodes[0x2C] = &M6502::OPCode0x2C;
    m_opcodes[0x2D] = &M6502::OPCode0x2D;
    m_opcodes[0x2E] = &M6502::OPCode0x2E;
    m_opcodes[0x2F] = &M6502::OPCode0x2F;

    m_opcodes[0x30] = &M6502::OPCode0x30;
    m_opcodes[0x31] = &M6502::OPCode0x31;
    m_opcodes[0x32] = &M6502::OPCode0x32;
    m_opcodes[0x33] = &M6502::OPCode0x33;
    m_opcodes[0x34] = &M6502::OPCode0x34;
    m_opcodes[0x35] = &M6502::OPCode0x35;
    m_opcodes[0x36] = &M6502::OPCode0x36;
    m_opcodes[0x37] = &M6502::OPCode0x37;
    m_opcodes[0x38] = &M6502::OPCode0x38;
    m_opcodes[0x39] = &M6502::OPCode0x39;
    m_opcodes[0x3A] = &M6502::OPCode0x3A;
    m_opcodes[0x3B] = &M6502::OPCode0x3B;
    m_opcodes[0x3C] = &M6502::OPCode0x3C;
    m_opcodes[0x3D] = &M6502::OPCode0x3D;
    m_opcodes[0x3E] = &M6502::OPCode0x3E;
    m_opcodes[0x3F] = &M6502::OPCode0x3F;

    m_opcodes[0x40] = &M6502::OPCode0x40;
    m_opcodes[0x41] = &M6502::OPCode0x41;
    m_opcodes[0x42] = &M6502::OPCode0x42;
    m_opcodes[0x43] = &M6502::OPCode0x43;
    m_opcodes[0x44] = &M6502::OPCode0x44;
    m_opcodes[0x45] = &M6502::OPCode0x45;
    m_opcodes[0x46] = &M6502::OPCode0x46;
    m_opcodes[0x47] = &M6502::OPCode0x47;
    m_opcodes[0x48] = &M6502::OPCode0x48;
    m_opcodes[0x49] = &M6502::OPCode0x49;
    m_opcodes[0x4A] = &M6502::OPCode0x4A;
    m_opcodes[0x4B] = &M6502::OPCode0x4B;
    m_opcodes[0x4C] = &M6502::OPCode0x4C;
    m_opcodes[0x4D] = &M6502::OPCode0x4D;
    m_opcodes[0x4E] = &M6502::OPCode0x4E;
    m_opcodes[0x4F] = &M6502::OPCode0x4F;

    m_opcodes[0x50] = &M6502::OPCode0x50;
    m_opcodes[0x51] = &M6502::OPCode0x51;
    m_opcodes[0x52] = &M6502::OPCode0x52;
    m_opcodes[0x53] = &M6502::OPCode0x53;
    m_opcodes[0x54] = &M6502::OPCode0x54;
    m_opcodes[0x55] = &M6502::OPCode0x55;
    m_opcodes[0x56] = &M6502::OPCode0x56;
    m_opcodes[0x57] = &M6502::OPCode0x57;
    m_opcodes[0x58] = &M6502::OPCode0x58;
    m_opcodes[0x59] = &M6502::OPCode0x59;
    m_opcodes[0x5A] = &M6502::OPCode0x5A;
    m_opcodes[0x5B] = &M6502::OPCode0x5B;
    m_opcodes[0x5C] = &M6502::OPCode0x5C;
    m_opcodes[0x5D] = &M6502::OPCode0x5D;
    m_opcodes[0x5E] = &M6502::OPCode0x5E;
    m_opcodes[0x5F] = &M6502::OPCode0x5F;

    m_opcodes[0x60] = &M6502::OPCode0x60;
    m_opcodes[0x61] = &M6502::OPCode0x61;
    m_opcodes[0x62] = &M6502::OPCode0x62;
    m_opcodes[0x63] = &M6502::OPCode0x63;
    m_opcodes[0x64] = &M6502::OPCode0x64;
    m_opcodes[0x65] = &M6502::OPCode0x65;
    m_opcodes[0x66] = &M6502::OPCode0x66;
    m_opcodes[0x67] = &M6502::OPCode0x67;
    m_opcodes[0x68] = &M6502::OPCode0x68;
    m_opcodes[0x69] = &M6502::OPCode0x69;
    m_opcodes[0x6A] = &M6502::OPCode0x6A;
    m_opcodes[0x6B] = &M6502::OPCode0x6B;
    m_opcodes[0x6C] = &M6502::OPCode0x6C;
    m_opcodes[0x6D] = &M6502::OPCode0x6D;
    m_opcodes[0x6E] = &M6502::OPCode0x6E;
    m_opcodes[0x6F] = &M6502::OPCode0x6F;

    m_opcodes[0x70] = &M6502::OPCode0x70;
    m_opcodes[0x71] = &M6502::OPCode0x71;
    m_opcodes[0x72] = &M6502::OPCode0x72;
    m_opcodes[0x73] = &M6502::OPCode0x73;
    m_opcodes[0x74] = &M6502::OPCode0x74;
    m_opcodes[0x75] = &M6502::OPCode0x75;
    m_opcodes[0x76] = &M6502::OPCode0x76;
    m_opcodes[0x77] = &M6502::OPCode0x77;
    m_opcodes[0x78] = &M6502::OPCode0x78;
    m_opcodes[0x79] = &M6502::OPCode0x79;
    m_opcodes[0x7A] = &M6502::OPCode0x7A;
    m_opcodes[0x7B] = &M6502::OPCode0x7B;
    m_opcodes[0x7C] = &M6502::OPCode0x7C;
    m_opcodes[0x7D] = &M6502::OPCode0x7D;
    m_opcodes[0x7E] = &M6502::OPCode0x7E;
    m_opcodes[0x7F] = &M6502::OPCode0x7F;

    m_opcodes[0x80] = &M6502::OPCode0x80;
    m_opcodes[0x81] = &M6502::OPCode0x81;
    m_opcodes[0x82] = &M6502::OPCode0x82;
    m_opcodes[0x83] = &M6502::OPCode0x83;
    m_opcodes[0x84] = &M6502::OPCode0x84;
    m_opcodes[0x85] = &M6502::OPCode0x85;
    m_opcodes[0x86] = &M6502::OPCode0x86;
    m_opcodes[0x87] = &M6502::OPCode0x87;
    m_opcodes[0x88] = &M6502::OPCode0x88;
    m_opcodes[0x89] = &M6502::OPCode0x89;
    m_opcodes[0x8A] = &M6502::OPCode0x8A;
    m_opcodes[0x8B] = &M6502::OPCode0x8B;
    m_opcodes[0x8C] = &M6502::OPCode0x8C;
    m_opcodes[0x8D] = &M6502::OPCode0x8D;
    m_opcodes[0x8E] = &M6502::OPCode0x8E;
    m_opcodes[0x8F] = &M6502::OPCode0x8F;

    m_opcodes[0x90] = &M6502::OPCode0x90;
    m_opcodes[0x91] = &M6502::OPCode0x91;
    m_opcodes[0x92] = &M6502::OPCode0x92;
    m_opcodes[0x93] = &M6502::OPCode0x93;
    m_opcodes[0x94] = &M6502::OPCode0x94;
    m_opcodes[0x95] = &M6502::OPCode0x95;
    m_opcodes[0x96] = &M6502::OPCode0x96;
    m_opcodes[0x97] = &M6502::OPCode0x97;
    m_opcodes[0x98] = &M6502::OPCode0x98;
    m_opcodes[0x99] = &M6502::OPCode0x99;
    m_opcodes[0x9A] = &M6502::OPCode0x9A;
    m_opcodes[0x9B] = &M6502::OPCode0x9B;
    m_opcodes[0x9C] = &M6502::OPCode0x9C;
    m_opcodes[0x9D] = &M6502::OPCode0x9D;
    m_opcodes[0x9E] = &M6502::OPCode0x9E;
    m_opcodes[0x9F] = &M6502::OPCode0x9F;

    m_opcodes[0xA0] = &M6502::OPCode0xA0;
    m_opcodes[0xA1] = &M6502::OPCode0xA1;
    m_opcodes[0xA2] = &M6502::OPCode0xA2;
    m_opcodes[0xA3] = &M6502::OPCode0xA3;
    m_opcodes[0xA4] = &M6502::OPCode0xA4;
    m_opcodes[0xA5] = &M6502::OPCode0xA5;
    m_opcodes[0xA6] = &M6502::OPCode0xA6;
    m_opcodes[0xA7] = &M6502::OPCode0xA7;
    m_opcodes[0xA8] = &M6502::OPCode0xA8;
    m_opcodes[0xA9] = &M6502::OPCode0xA9;
    m_opcodes[0xAA] = &M6502::OPCode0xAA;
    m_opcodes[0xAB] = &M6502::OPCode0xAB;
    m_opcodes[0xAC] = &M6502::OPCode0xAC;
    m_opcodes[0xAD] = &M6502::OPCode0xAD;
    m_opcodes[0xAE] = &M6502::OPCode0xAE;
    m_opcodes[0xAF] = &M6502::OPCode0xAF;

    m_opcodes[0xB0] = &M6502::OPCode0xB0;
    m_opcodes[0xB1] = &M6502::OPCode0xB1;
    m_opcodes[0xB2] = &M6502::OPCode0xB2;
    m_opcodes[0xB3] = &M6502::OPCode0xB3;
    m_opcodes[0xB4] = &M6502::OPCode0xB4;
    m_opcodes[0xB5] = &M6502::OPCode0xB5;
    m_opcodes[0xB6] = &M6502::OPCode0xB6;
    m_opcodes[0xB7] = &M6502::OPCode0xB7;
    m_opcodes[0xB8] = &M6502::OPCode0xB8;
    m_opcodes[0xB9] = &M6502::OPCode0xB9;
    m_opcodes[0xBA] = &M6502::OPCode0xBA;
    m_opcodes[0xBB] = &M6502::OPCode0xBB;
    m_opcodes[0xBC] = &M6502::OPCode0xBC;
    m_opcodes[0xBD] = &M6502::OPCode0xBD;
    m_opcodes[0xBE] = &M6502::OPCode0xBE;
    m_opcodes[0xBF] = &M6502::OPCode0xBF;

    m_opcodes[0xC0] = &M6502::OPCode0xC0;
    m_opcodes[0xC1] = &M6502::OPCode0xC1;
    m_opcodes[0xC2] = &M6502::OPCode0xC2;
    m_opcodes[0xC3] = &M6502::OPCode0xC3;
    m_opcodes[0xC4] = &M6502::OPCode0xC4;
    m_opcodes[0xC5] = &M6502::OPCode0xC5;
    m_opcodes[0xC6] = &M6502::OPCode0xC6;
    m_opcodes[0xC7] = &M6502::OPCode0xC7;
    m_opcodes[0xC8] = &M6502::OPCode0xC8;
    m_opcodes[0xC9] = &M6502::OPCode0xC9;
    m_opcodes[0xCA] = &M6502::OPCode0xCA;
    m_opcodes[0xCB] = &M6502::OPCode0xCB;
    m_opcodes[0xCC] = &M6502::OPCode0xCC;
    m_opcodes[0xCD] = &M6502::OPCode0xCD;
    m_opcodes[0xCE] = &M6502::OPCode0xCE;
    m_opcodes[0xCF] = &M6502::OPCode0xCF;

    m_opcodes[0xD0] = &M6502::OPCode0xD0;
    m_opcodes[0xD1] = &M6502::OPCode0xD1;
    m_opcodes[0xD2] = &M6502::OPCode0xD2;
    m_opcodes[0xD3] = &M6502::OPCode0xD3;
    m_opcodes[0xD4] = &M6502::OPCode0xD4;
    m_opcodes[0xD5] = &M6502::OPCode0xD5;
    m_opcodes[0xD6] = &M6502::OPCode0xD6;
    m_opcodes[0xD7] = &M6502::OPCode0xD7;
    m_opcodes[0xD8] = &M6502::OPCode0xD8;
    m_opcodes[0xD9] = &M6502::OPCode0xD9;
    m_opcodes[0xDA] = &M6502::OPCode0xDA;
    m_opcodes[0xDB] = &M6502::OPCode0xDB;
    m_opcodes[0xDC] = &M6502::OPCode0xDC;
    m_opcodes[0xDD] = &M6502::OPCode0xDD;
    m_opcodes[0xDE] = &M6502::OPCode0xDE;
    m_opcodes[0xDF] = &M6502::OPCode0xDF;

    m_opcodes[0xE0] = &M6502::OPCode0xE0;
    m_opcodes[0xE1] = &M6502::OPCode0xE1;
    m_opcodes[0xE2] = &M6502::OPCode0xE2;
    m_opcodes[0xE3] = &M6502::OPCode0xE3;
    m_opcodes[0xE4] = &M6502::OPCode0xE4;
    m_opcodes[0xE5] = &M6502::OPCode0xE5;
    m_opcodes[0xE6] = &M6502::OPCode0xE6;
    m_opcodes[0xE7] = &M6502::OPCode0xE7;
    m_opcodes[0xE8] = &M6502::OPCode0xE8;
    m_opcodes[0xE9] = &M6502::OPCode0xE9;
    m_opcodes[0xEA] = &M6502::OPCode0xEA;
    m_opcodes[0xEB] = &M6502::OPCode0xEB;
    m_opcodes[0xEC] = &M6502::OPCode0xEC;
    m_opcodes[0xED] = &M6502::OPCode0xED;
    m_opcodes[0xEE] = &M6502::OPCode0xEE;
    m_opcodes[0xEF] = &M6502::OPCode0xEF;

    m_opcodes[0xF0] = &M6502::OPCode0xF0;
    m_opcodes[0xF1] = &M6502::OPCode0xF1;
    m_opcodes[0xF2] = &M6502::OPCode0xF2;
    m_opcodes[0xF3] = &M6502::OPCode0xF3;
    m_opcodes[0xF4] = &M6502::OPCode0xF4;
    m_opcodes[0xF5] = &M6502::OPCode0xF5;
    m_opcodes[0xF6] = &M6502::OPCode0xF6;
    m_opcodes[0xF7] = &M6502::OPCode0xF7;
    m_opcodes[0xF8] = &M6502::OPCode0xF8;
    m_opcodes[0xF9] = &M6502::OPCode0xF9;
    m_opcodes[0xFA] = &M6502::OPCode0xFA;
    m_opcodes[0xFB] = &M6502::OPCode0xFB;
    m_opcodes[0xFC] = &M6502::OPCode0xFC;
    m_opcodes[0xFD] = &M6502::OPCode0xFD;
    m_opcodes[0xFE] = &M6502::OPCode0xFE;
    m_opcodes[0xFF] = &M6502::OPCode0xFF;
}