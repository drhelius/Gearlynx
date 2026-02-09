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

#ifndef OGL_RENDERER_H
#define OGL_RENDERER_H

#include <stdint.h>

#ifdef OGL_RENDERER_IMPORT
    #define EXTERN
#else
    #define EXTERN extern
#endif

#define FRAME_BUFFER_SCALE 4
#define SYSTEM_TEXTURE_WIDTH 256
#define SYSTEM_TEXTURE_HEIGHT 256
#define FRAME_BUFFER_WIDTH 1024
#define FRAME_BUFFER_HEIGHT 1024
#define MAX_FRAME_HISTORY 8

EXTERN uint32_t ogl_renderer_emu_texture;
EXTERN uint32_t ogl_renderer_emu_debug_framebuffer[4];
EXTERN uint32_t ogl_renderer_emu_debug_huc6270_sprites[64];
EXTERN uint32_t ogl_renderer_emu_savestates;
EXTERN const char* ogl_renderer_opengl_version;

EXTERN bool ogl_renderer_init(void);
EXTERN void ogl_renderer_destroy(void);
EXTERN void ogl_renderer_begin_render(void);
EXTERN void ogl_renderer_render(void);
EXTERN void ogl_renderer_end_render(void);

#undef OGL_RENDERER_IMPORT
#undef EXTERN
#endif /* OGL_RENDERER_H */
