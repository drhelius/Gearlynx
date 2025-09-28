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

#ifndef DEFINES_H
#define DEFINES_H

#if !defined(EMULATOR_BUILD)
    #define EMULATOR_BUILD "undefined"
#endif

#define GLYNX_VERSION EMULATOR_BUILD

#define GLYNX_TITLE "Gearlynx"
#define GLYNX_TITLE_ASCII "" \
"   ____                 _                  \n" \
"  / ___| ___  __ _ _ __| |_   _ _ __ __  __\n" \
" | |  _ / _ \\/ _` | '__| | | | | '_ \\\\ \\/ /\n" \
" | |_| |  __/ (_| | |  | | |_| | | | |>  < \n" \
"  \\____|\\___|\\__,_|_|  |_|\\__, |_| |_/_/\\_\\\n" \
"                          |___/            \n"

#if defined(DEBUG)
    #define GLYNX_DEBUG 1
#endif

#define GLYNX_SCREEN_WIDTH 160
#define GLYNX_SCREEN_HEIGHT 102

#define GLYNX_MAX_GAMEPADS 1

#define GLYNX_AUDIO_SAMPLE_RATE 44100
#define GLYNX_AUDIO_CYCLES_PER_SAMPLE 363   // 16MHz / 44100Hz = ~362.81
#define GLYNX_AUDIO_BUFFER_SIZE 2048
#define GLYNX_AUDIO_BUFFER_COUNT 3

#define GLYNX_BIOS_SIZE 0x200

#define GLYNX_SAVESTATE_VERSION 1
#define GLYNX_SAVESTATE_MAGIC 0x56191212

#if !defined(NULL)
    #define NULL 0
#endif

#define SafeDelete(pointer) if(pointer != NULL) {delete pointer; pointer = NULL;}
#define SafeDeleteArray(pointer) if(pointer != NULL) {delete [] pointer; pointer = NULL;}

#define InitPointer(pointer) ((pointer) = NULL)
#define IsValidPointer(pointer) ((pointer) != NULL)

#define UNUSED(expr) (void)(expr)

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define CLAMP(value, min, max) MIN(MAX(value, min), max)

#if defined(MSB_FIRST) || defined(__BIG_ENDIAN__) || (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
    #define GLYNX_BIG_ENDIAN
#else
    #define GLYNX_LITTLE_ENDIAN
#endif

#if defined(__GNUC__) || defined(__clang__)
    #define INLINE inline __attribute__((always_inline))
    #define NO_INLINE __attribute__((noinline))
#elif defined(_MSC_VER)
    #define INLINE __forceinline
    #define NO_INLINE __declspec(noinline)
#else
    #define INLINE inline
    #define NO_INLINE
#endif

#if !defined(GLYNX_DEBUG)
    #if defined(__GNUC__) || defined(__clang__)
        #if !defined(__OPTIMIZE__) && !defined(__OPTIMIZE_SIZE__)
            #warning "Compiling without optimizations."
            #define GLYNX_NO_OPTIMIZATIONS
        #endif
    #elif defined(_MSC_VER)
        #if !defined(NDEBUG)
            #pragma message("Compiling without optimizations.")
            #define GLYNX_NO_OPTIMIZATIONS
        #endif
    #endif
#endif

#if defined(__GNUC__) || defined(__clang__)
    #define likely(x)   __builtin_expect(!!(x), 1)
    #define unlikely(x) __builtin_expect(!!(x), 0)
#else
    #define likely(x)   (x)
    #define unlikely(x) (x)
#endif

#endif /* DEFINES_H */
