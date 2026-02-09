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

#if defined(__APPLE__)
    #define GL_SILENCE_DEPRECATION
    #include <OpenGL/gl.h>
#else
    #define GLAD_GL_IMPLEMENTATION
    #include <glad.h>
    #include <SDL_opengl.h>
#endif

#include "imgui.h"
#include "imgui_impl_opengl2.h"
#include "emu.h"
#include "config.h"
#include "gearlynx.h"
#include "gui.h"

#define OGL_RENDERER_IMPORT
#include "ogl_renderer.h"

static uint32_t system_texture;
static uint32_t history_textures[MAX_FRAME_HISTORY];
static uint32_t history_fbo[MAX_FRAME_HISTORY];
static uint32_t persistence_texture;
static uint32_t persistence_fbo;
static int frame_history_index = 0;
static uint32_t scanlines_horizontal_texture;
static uint32_t scanlines_vertical_texture;
static uint32_t scanlines_grid_texture;
static uint32_t frame_buffer_object;
static GLYNX_Runtime_Info current_runtime;
static bool first_frame;

static u32 scanlines_horizontal[16] = {
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF};
static u32 scanlines_vertical[16] = {
    0x00000000, 0x00000000, 0x00000000, 0x000000FF,
    0x00000000, 0x00000000, 0x00000000, 0x000000FF,
    0x00000000, 0x00000000, 0x00000000, 0x000000FF,
    0x00000000, 0x00000000, 0x00000000, 0x000000FF};
static u32 scanlines_grid[16] = {
    0x00000000, 0x00000000, 0x00000000, 0x000000FF,
    0x00000000, 0x00000000, 0x00000000, 0x000000FF,
    0x00000000, 0x00000000, 0x00000000, 0x000000FF,
    0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF};

static void init_ogl_gui(void);
static void init_ogl_emu(void);
static void init_ogl_debug(void);
static void init_ogl_savestates(void);
static void init_scanlines_texture(void);
static void init_frame_history(void);
static void render_gui(void);
static void render_emu_normal(void);
static void render_emu_mix(void);
static void update_emu_texture(void);
static void render_quad(void);
static void update_system_texture(void);
static void update_debug_textures(void);
static void update_savestates_texture(void);
static void render_scanlines(void);

bool ogl_renderer_init(void)
{
#if !defined(__APPLE__)
    int version = gladLoadGL((GLADloadfunc) SDL_GL_GetProcAddress);

    if (version == 0)
    {
        Error("GLAD: Failed to initialize OpenGL context");
        return false;
    }

    Log("GLAD: OpenGL %d.%d", GLAD_VERSION_MAJOR(version), GLAD_VERSION_MINOR(version));
#endif

    ogl_renderer_opengl_version = (const char*)glGetString(GL_VERSION);
    Log("Using OpenGL %s", ogl_renderer_opengl_version);

    init_ogl_gui();
    init_ogl_emu();
    init_ogl_debug();
    init_ogl_savestates();

    first_frame = true;
    return true;
}

void ogl_renderer_destroy(void)
{
    glDeleteFramebuffers(1, &frame_buffer_object); 
    glDeleteTextures(1, &ogl_renderer_emu_texture);
    glDeleteTextures(1, &system_texture);
    glDeleteTextures(1, &scanlines_horizontal_texture);
    glDeleteTextures(1, &scanlines_vertical_texture);
    glDeleteTextures(1, &scanlines_grid_texture);

    for (int i = 0; i < MAX_FRAME_HISTORY; i++)
    {
        glDeleteTextures(1, &history_textures[i]);
        glDeleteFramebuffers(1, &history_fbo[i]);
    }

    glDeleteTextures(1, &persistence_texture);
    glDeleteFramebuffers(1, &persistence_fbo);

    for (int s = 0; s < 64; s++)
        glDeleteTextures(1, &ogl_renderer_emu_debug_huc6270_sprites[s]);

    for (int s = 0; s < 4; s++)
        glDeleteTextures(1, &ogl_renderer_emu_debug_framebuffer[s]);

    glDeleteTextures(1, &ogl_renderer_emu_savestates);

    ImGui_ImplOpenGL2_Shutdown();
}

void ogl_renderer_begin_render(void)
{
    ImGui_ImplOpenGL2_NewFrame();
}

void ogl_renderer_render(void)
{
    emu_get_runtime(current_runtime);

    if (config_debug.debug)
    {
        update_debug_textures();
    }

    update_savestates_texture();

    if (config_video.ghosting)
        render_emu_mix();
    else
        render_emu_normal();

    if (config_video.scanlines_type > 0)
        render_scanlines();

    update_emu_texture();

    ImVec4 clear_color = ImVec4(config_video.background_color[0], config_video.background_color[1], config_video.background_color[2], 1.00f);

    glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);
    glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);

    render_gui();
}

void ogl_renderer_end_render(void)
{
#if defined(__APPLE__) || defined(_WIN32)
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        SDL_Window* backup_current_window = SDL_GL_GetCurrentWindow();
        SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        SDL_GL_MakeCurrent(backup_current_window, backup_current_context);
    }
#endif
}

static void init_ogl_gui(void)
{
    ImGui_ImplOpenGL2_Init();
}

static void init_ogl_emu(void)
{
    glEnable(GL_TEXTURE_2D);

    glGenFramebuffers(1, &frame_buffer_object);
    glGenTextures(1, &ogl_renderer_emu_texture);
    glGenTextures(1, &system_texture);

    glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer_object);
    glBindTexture(GL_TEXTURE_2D, ogl_renderer_emu_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ogl_renderer_emu_texture, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glBindTexture(GL_TEXTURE_2D, system_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SYSTEM_TEXTURE_WIDTH, SYSTEM_TEXTURE_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*) emu_frame_buffer);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    init_frame_history();
    init_scanlines_texture();
}

static void init_ogl_debug(void)
{
    for (int s = 0; s < 64; s++)
    {
        glGenTextures(1, &ogl_renderer_emu_debug_huc6270_sprites[s]);
        glBindTexture(GL_TEXTURE_2D, ogl_renderer_emu_debug_huc6270_sprites[s]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 32, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)emu_debug_sprite_buffers[s]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    }

    for (int s = 0; s < 4; s++)
    {
        glGenTextures(1, &ogl_renderer_emu_debug_framebuffer[s]);
        glBindTexture(GL_TEXTURE_2D, ogl_renderer_emu_debug_framebuffer[s]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 128, 0, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)emu_debug_framebuffer[s]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    }
}

static void init_ogl_savestates(void)
{
    glGenTextures(1, &ogl_renderer_emu_savestates);
    glBindTexture(GL_TEXTURE_2D, ogl_renderer_emu_savestates);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
}

static void init_scanlines_texture(void)
{
    glGenTextures(1, &scanlines_horizontal_texture);
    glBindTexture(GL_TEXTURE_2D, scanlines_horizontal_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 4, 4, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, (GLvoid*) scanlines_horizontal);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glGenTextures(1, &scanlines_vertical_texture);
    glBindTexture(GL_TEXTURE_2D, scanlines_vertical_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 4, 4, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, (GLvoid*) scanlines_vertical);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glGenTextures(1, &scanlines_grid_texture);
    glBindTexture(GL_TEXTURE_2D, scanlines_grid_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 4, 4, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, (GLvoid*) scanlines_grid);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

static void init_frame_history(void)
{
    for (int i = 0; i < MAX_FRAME_HISTORY; i++)
    {
        glGenTextures(1, &history_textures[i]);
        glBindTexture(GL_TEXTURE_2D, history_textures[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glGenFramebuffers(1, &history_fbo[i]);
        glBindFramebuffer(GL_FRAMEBUFFER, history_fbo[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, history_textures[i], 0);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    glGenTextures(1, &persistence_texture);
    glBindTexture(GL_TEXTURE_2D, persistence_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenFramebuffers(1, &persistence_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, persistence_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, persistence_texture, 0);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    frame_history_index = 0;
}

static void render_gui(void)
{
    ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
}

static void render_emu_normal(void)
{
    glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer_object);

    glDisable(GL_BLEND);

    update_system_texture();

    render_quad();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static void render_emu_mix(void)
{
    int history_count = config_video.ghosting_history;
    if (history_count < 2) history_count = 2;
    if (history_count > MAX_FRAME_HISTORY) history_count = MAX_FRAME_HISTORY;

    glBindFramebuffer(GL_FRAMEBUFFER, history_fbo[frame_history_index]);
    glDisable(GL_BLEND);
    update_system_texture();
    render_quad();
    frame_history_index = (frame_history_index + 1) % history_count;

    float intensity = config_video.ghosting_intensity;
    float response = config_video.ghosting_response;

    float weights[MAX_FRAME_HISTORY];
    float total_weight = 0.0f;

    for (int i = 0; i < history_count; i++)
    {
        float base_decay = 0.5f + (intensity * 0.5f);
        float weight = 1.0f;
        for (int j = 0; j < i; j++)
        {
            weight *= base_decay;
        }
        weights[i] = weight;
        total_weight += weight;
    }

    for (int i = 0; i < history_count; i++)
    {
        weights[i] /= total_weight;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer_object);

    if (first_frame)
    {
        first_frame = false;
        glDisable(GL_BLEND);
        glBindTexture(GL_TEXTURE_2D, history_textures[(frame_history_index + history_count - 1) % history_count]);
        render_quad();

        glBindFramebuffer(GL_FRAMEBUFFER, persistence_fbo);
        glBindTexture(GL_TEXTURE_2D, history_textures[(frame_history_index + history_count - 1) % history_count]);
        render_quad();
    }
    else
    {
        float trail_weight = response * 0.95f;
        float new_weight = 1.0f - trail_weight;

        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);

        glBindTexture(GL_TEXTURE_2D, persistence_texture);
        glColor4f(trail_weight, trail_weight, trail_weight, 1.0f);
        render_quad();

        for (int i = 0; i < history_count; i++)
        {
            int frame_idx = (frame_history_index + history_count - 1 - i) % history_count;
            float frame_weight = weights[i] * new_weight;

            glBindTexture(GL_TEXTURE_2D, history_textures[frame_idx]);
            glColor4f(frame_weight, frame_weight, frame_weight, 1.0f);
            render_quad();
        }

        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        glDisable(GL_BLEND);

        glBindFramebuffer(GL_READ_FRAMEBUFFER, frame_buffer_object);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, persistence_fbo);
        glBlitFramebuffer(0, 0, FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT,
                          0, 0, FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT,
                          GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static void update_system_texture(void)
{
    glBindTexture(GL_TEXTURE_2D, system_texture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, current_runtime.screen_width, current_runtime.screen_height,
            GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*) emu_frame_buffer);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    if (config_video.bilinear)
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }
    else
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    }
}

static void update_debug_textures(void)
{
    for (int s = 0; s < 4; s++)
    {
        glBindTexture(GL_TEXTURE_2D, ogl_renderer_emu_debug_framebuffer[s]);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, GLYNX_SCREEN_WIDTH, GLYNX_SCREEN_HEIGHT,
                GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*) emu_debug_framebuffer[s]);
    }
}

static void update_savestates_texture(void)
{
    int i = config_emulator.save_slot;

    if (IsValidPointer(emu_savestates_screenshots[i].data))
    {
        int width = emu_savestates_screenshots[i].width;
        int height = emu_savestates_screenshots[i].height;
        glBindTexture(GL_TEXTURE_2D, ogl_renderer_emu_savestates);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*) emu_savestates_screenshots[i].data);
    }
}

static void update_emu_texture(void)
{
    glBindTexture(GL_TEXTURE_2D, ogl_renderer_emu_texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    bool scanlines_filter = (config_video.scale >= 2) || (config_video.ratio != 0) ||
        ((config_video.scale <= 1) && (gui_scale_multiplier >= 3) && (gui_scale_multiplier & 1));

    if ((config_video.scanlines_type > 0) && scanlines_filter)
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }
    else
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    }
}

static void render_quad(void)
{
    int viewportWidth = current_runtime.screen_width * FRAME_BUFFER_SCALE;
    int viewportHeight = current_runtime.screen_height * FRAME_BUFFER_SCALE;

    float tex_h = (float)current_runtime.screen_width / (float)SYSTEM_TEXTURE_WIDTH;
    float tex_v = (float)current_runtime.screen_height / (float)SYSTEM_TEXTURE_HEIGHT;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glOrtho(0, viewportWidth, 0, viewportHeight, -1, 1);

    glMatrixMode(GL_MODELVIEW);
    glViewport(0, 0, viewportWidth, viewportHeight);

    glBegin(GL_QUADS);
    glTexCoord2d(0.0, 0.0);
    glVertex2d(0.0, 0.0);
    glTexCoord2d(tex_h, 0.0);
    glVertex2d(viewportWidth, 0.0);
    glTexCoord2d(tex_h, tex_v);
    glVertex2d(viewportWidth, viewportHeight);
    glTexCoord2d(0.0, tex_v);
    glVertex2d(0.0, viewportHeight);
    glEnd();
}

static void render_scanlines(void)
{
    glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer_object);
    glEnable(GL_BLEND);

    glColor4f(1.0f, 1.0f, 1.0f, config_video.scanlines_intensity);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (config_video.scanlines_type == 1)
        glBindTexture(GL_TEXTURE_2D, scanlines_horizontal_texture);
    else if (config_video.scanlines_type == 2)
        glBindTexture(GL_TEXTURE_2D, scanlines_vertical_texture);
    else if (config_video.scanlines_type == 3)
        glBindTexture(GL_TEXTURE_2D, scanlines_grid_texture);
    else
        glBindTexture(GL_TEXTURE_2D, scanlines_vertical_texture);

    int viewportWidth = current_runtime.screen_width * FRAME_BUFFER_SCALE;
    int viewportHeight = current_runtime.screen_height * FRAME_BUFFER_SCALE;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glOrtho(0, viewportWidth, 0, viewportHeight, -1, 1);

    glMatrixMode(GL_MODELVIEW);
    glViewport(0, 0, viewportWidth, viewportHeight);

    float tex_h = (float)current_runtime.screen_width;
    float tex_v = (float)current_runtime.screen_height;

    glBegin(GL_QUADS);
    glTexCoord2d(0.0, 0.0);
    glVertex2d(0.0, 0.0);
    glTexCoord2d(tex_h, 0.0);
    glVertex2d(viewportWidth, 0.0);
    glTexCoord2d(tex_h, tex_v);
    glVertex2d(viewportWidth, viewportHeight);
    glTexCoord2d(0.0, tex_v);
    glVertex2d(0.0, viewportHeight);
    glEnd();

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

    glDisable(GL_BLEND);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
