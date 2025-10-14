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

#ifndef BIT_OPS_H
#define BIT_OPS_H

#include "types.h"

#define BIT(n) (1U << (n))
#define BIT_MASK(n) (BIT(n) - 1)
#define SET_BIT(value, bit) ((value) | (1U << (bit)))
#define UNSET_BIT(value, bit) ((value) & (~(1U << (bit))))
#define IS_SET_BIT(value, bit) (((value) & (1U << (bit))) != 0)
#define IS_NOT_SET_BIT(value, bit) (((value) & (1U << (bit))) == 0)
#define FLIP_BIT(value, bit) ((value) ^ (1U << (bit)))

static const u8 k_bitops_reverse_lut[16] = {
    0x0, 0x8, 0x4, 0xC, 0x2, 0xA, 0x6, 0xE,
    0x1, 0x9, 0x5, 0xD, 0x3, 0xB, 0x7, 0xF
};

INLINE u8 reverse_bits(const u8 value)
{
    return (k_bitops_reverse_lut[value & 0xF] << 4) | k_bitops_reverse_lut[value >> 4];
}

INLINE u32 l_zero16(u16 value)
{
#if defined(__GNUC__) || defined(__clang__)
    return (u32)(value ? __builtin_clz((unsigned)value) - (int)(sizeof(unsigned) * 8 - 16) : 16);
#else
    u32 n = 0;
    u16 m = 0x8000;
    while(m && !(value & m))
    {
        n++;
        m >>= 1;
    }
    return value ? n : 16;
#endif
}

INLINE u8 parity8(u8 x)
{
#if defined(__GNUC__) || defined(__clang__)
    return (u8)__builtin_parity((unsigned int)x);
#else
    x ^= x >> 4;
    x &= 0x0F;
    return (u8)((0x6996 >> x) & 1);
#endif
}

INLINE u8 parity16(u16 x)
{
#if defined(__GNUC__) || defined(__clang__)
    return (u8)__builtin_parity((unsigned int)x);
#else
    x ^= x >> 8;
    x ^= x >> 4;
    x &= 0x000F;
    return (u8)((0x6996 >> x) & 1);
#endif
}

#endif /* BIT_OPS_H */
