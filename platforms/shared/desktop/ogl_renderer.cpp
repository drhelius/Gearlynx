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
    #include <OpenGL/gl3.h>
#else
    #define GLAD_GL_IMPLEMENTATION
    #include <glad.h>
#endif

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "emu.h"
#include "config.h"
#include "gearlynx.h"
#include "gui.h"

#define OGL_RENDERER_IMPORT
#include "ogl_renderer.h"

static uint32_t system_texture;
static uint32_t persistence_texture[2];
static uint32_t persistence_fbo[2];
static int persistence_index = 0;
static uint32_t history_textures[MAX_FRAME_HISTORY];
static uint32_t history_fbo[MAX_FRAME_HISTORY];
static int frame_history_index = 0;
static uint32_t scanlines_horizontal_texture;
static uint32_t scanlines_vertical_texture;
static uint32_t scanlines_grid_texture;
static uint32_t frame_buffer_object;
static GLYNX_Runtime_Info current_runtime;
static bool first_frame;

static uint32_t quad_shader_program = 0;
static uint32_t quad_vao = 0;
static uint32_t quad_vbo = 0;
static int quad_uniform_texture = -1;
static int quad_uniform_color = -1;
static int quad_uniform_tex_scale = -1;

static uint32_t mix_shader_program = 0;
static int mix_uniform_tex_new = -1;
static int mix_uniform_tex_old = -1;
static int mix_uniform_decay = -1;
static int mix_uniform_tex_scale_new = -1;
static int mix_uniform_tex_scale_old = -1;

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
static void init_persistence(void);
static void init_shaders(void);
static void render_gui(void);
static void render_emu_normal(void);
static void render_emu_mix(void);
static void update_emu_texture(void);
static void render_quad(float tex_h, float tex_v);
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
    Log("Starting OpenGL %s", ogl_renderer_opengl_version);

    init_shaders();
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

    glDeleteTextures(2, persistence_texture);
    glDeleteFramebuffers(2, persistence_fbo);

    glDeleteTextures(MAX_FRAME_HISTORY, history_textures);
    glDeleteFramebuffers(MAX_FRAME_HISTORY, history_fbo);

    for (int s = 0; s < 64; s++)
        glDeleteTextures(1, &ogl_renderer_emu_debug_huc6270_sprites[s]);

    for (int s = 0; s < 5; s++)
        glDeleteTextures(1, &ogl_renderer_emu_debug_framebuffer[s]);

    glDeleteTextures(1, &ogl_renderer_emu_savestates);

    if (quad_shader_program)
        glDeleteProgram(quad_shader_program);
    if (mix_shader_program)
        glDeleteProgram(mix_shader_program);
    if (quad_vao)
        glDeleteVertexArrays(1, &quad_vao);
    if (quad_vbo)
        glDeleteBuffers(1, &quad_vbo);

    quad_shader_program = 0;
    mix_shader_program = 0;
    quad_vao = 0;
    quad_vbo = 0;

    ImGui_ImplOpenGL3_Shutdown();
}

void ogl_renderer_begin_render(void)
{
    ImGui_ImplOpenGL3_NewFrame();
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

    ImGuiIO& io = ImGui::GetIO();
    int fb_width = (int)(io.DisplaySize.x * io.DisplayFramebufferScale.x);
    int fb_height = (int)(io.DisplaySize.y * io.DisplayFramebufferScale.y);

    glViewport(0, 0, fb_width, fb_height);
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
#if defined(__APPLE__)
    ImGui_ImplOpenGL3_Init("#version 150");
#else
    ImGui_ImplOpenGL3_Init("#version 130");
#endif
}

static void init_ogl_emu(void)
{
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

    init_persistence();
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

    for (int s = 0; s < 5; s++)
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

static void init_persistence(void)
{
    for (int i = 0; i < 2; i++)
    {
        glGenTextures(1, &persistence_texture[i]);
        glBindTexture(GL_TEXTURE_2D, persistence_texture[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glGenFramebuffers(1, &persistence_fbo[i]);
        glBindFramebuffer(GL_FRAMEBUFFER, persistence_fbo[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, persistence_texture[i], 0);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    }

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

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    persistence_index = 0;
    frame_history_index = 0;
}

static void init_shaders(void)
{
#if defined(__APPLE__)
    const char* version = "#version 150\n";
#else
    const char* version = "#version 130\n";
#endif

    const char* vs_body =
        "in vec2 aPos;\n"
        "in vec2 aTexCoord;\n"
        "out vec2 vTexCoord;\n"
        "uniform vec2 uTexScale;\n"
        "void main() {\n"
        "    gl_Position = vec4(aPos, 0.0, 1.0);\n"
        "    vTexCoord = aTexCoord * uTexScale;\n"
        "}\n";

    const char* fs_body =
        "in vec2 vTexCoord;\n"
        "out vec4 FragColor;\n"
        "uniform sampler2D uTexture;\n"
        "uniform vec4 uColor;\n"
        "void main() {\n"
        "    FragColor = texture(uTexture, vTexCoord) * uColor;\n"
        "}\n";

    const char* vs_sources[2] = { version, vs_body };
    const char* fs_sources[2] = { version, fs_body };

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 2, vs_sources, NULL);
    glCompileShader(vs);

    GLint compiled = 0;
    glGetShaderiv(vs, GL_COMPILE_STATUS, &compiled);
    if (!compiled)
    {
        GLchar info[512];
        glGetShaderInfoLog(vs, 512, NULL, info);
        Error("Vertex shader compile error: %s", info);
    }

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 2, fs_sources, NULL);
    glCompileShader(fs);

    glGetShaderiv(fs, GL_COMPILE_STATUS, &compiled);
    if (!compiled)
    {
        GLchar info[512];
        glGetShaderInfoLog(fs, 512, NULL, info);
        Error("Fragment shader compile error: %s", info);
    }

    quad_shader_program = glCreateProgram();
    glAttachShader(quad_shader_program, vs);
    glAttachShader(quad_shader_program, fs);
    glLinkProgram(quad_shader_program);

    GLint linked = 0;
    glGetProgramiv(quad_shader_program, GL_LINK_STATUS, &linked);
    if (!linked)
    {
        GLchar info[512];
        glGetProgramInfoLog(quad_shader_program, 512, NULL, info);
        Error("Shader program link error: %s", info);
    }

    glDeleteShader(vs);
    glDeleteShader(fs);

    quad_uniform_tex_scale = glGetUniformLocation(quad_shader_program, "uTexScale");
    quad_uniform_texture = glGetUniformLocation(quad_shader_program, "uTexture");
    quad_uniform_color = glGetUniformLocation(quad_shader_program, "uColor");

    glUseProgram(quad_shader_program);
    glUniform1i(quad_uniform_texture, 0);
    glUseProgram(0);

    float quad_vertices[] = {
        -1.0f, -1.0f,  0.0f,  0.0f,
         1.0f, -1.0f,  1.0f,  0.0f,
        -1.0f,  1.0f,  0.0f,  1.0f,
         1.0f,  1.0f,  1.0f,  1.0f,
    };

    glGenVertexArrays(1, &quad_vao);
    glGenBuffers(1, &quad_vbo);

    glBindVertexArray(quad_vao);
    glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);

    GLint pos_attrib = glGetAttribLocation(quad_shader_program, "aPos");
    GLint tex_attrib = glGetAttribLocation(quad_shader_program, "aTexCoord");

    glEnableVertexAttribArray(pos_attrib);
    glVertexAttribPointer(pos_attrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(tex_attrib);
    glVertexAttribPointer(tex_attrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    Debug("Quad shader initialized (program=%u, vao=%u, vbo=%u)", quad_shader_program, quad_vao, quad_vbo);

    const char* mix_vs_body =
        "in vec2 aPos;\n"
        "in vec2 aTexCoord;\n"
        "out vec2 vTexCoordNew;\n"
        "out vec2 vTexCoordOld;\n"
        "uniform vec2 uTexScaleNew;\n"
        "uniform vec2 uTexScaleOld;\n"
        "void main() {\n"
        "    gl_Position = vec4(aPos, 0.0, 1.0);\n"
        "    vTexCoordNew = aTexCoord * uTexScaleNew;\n"
        "    vTexCoordOld = aTexCoord * uTexScaleOld;\n"
        "}\n";

    const char* mix_fs_body =
        "in vec2 vTexCoordNew;\n"
        "in vec2 vTexCoordOld;\n"
        "out vec4 FragColor;\n"
        "uniform sampler2D uTexNew;\n"
        "uniform sampler2D uTexOld;\n"
        "uniform float uDecay;\n"
        "void main() {\n"
        "    vec3 new_color = texture(uTexNew, vTexCoordNew).rgb;\n"
        "    vec3 old_color = texture(uTexOld, vTexCoordOld).rgb;\n"
        "    FragColor = vec4(mix(new_color, old_color, uDecay), 1.0);\n"
        "}\n";

    const char* mix_vs_sources[2] = { version, mix_vs_body };
    const char* mix_fs_sources[2] = { version, mix_fs_body };

    GLuint mix_vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(mix_vs, 2, mix_vs_sources, NULL);
    glCompileShader(mix_vs);

    glGetShaderiv(mix_vs, GL_COMPILE_STATUS, &compiled);
    if (!compiled)
    {
        GLchar info[512];
        glGetShaderInfoLog(mix_vs, 512, NULL, info);
        Error("Mix vertex shader compile error: %s", info);
    }

    GLuint mix_fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(mix_fs, 2, mix_fs_sources, NULL);
    glCompileShader(mix_fs);

    glGetShaderiv(mix_fs, GL_COMPILE_STATUS, &compiled);
    if (!compiled)
    {
        GLchar info[512];
        glGetShaderInfoLog(mix_fs, 512, NULL, info);
        Error("Mix fragment shader compile error: %s", info);
    }

    mix_shader_program = glCreateProgram();
    glAttachShader(mix_shader_program, mix_vs);
    glAttachShader(mix_shader_program, mix_fs);
    glLinkProgram(mix_shader_program);

    glGetProgramiv(mix_shader_program, GL_LINK_STATUS, &linked);
    if (!linked)
    {
        GLchar info[512];
        glGetProgramInfoLog(mix_shader_program, 512, NULL, info);
        Error("Mix shader program link error: %s", info);
    }

    glDeleteShader(mix_vs);
    glDeleteShader(mix_fs);

    mix_uniform_tex_new = glGetUniformLocation(mix_shader_program, "uTexNew");
    mix_uniform_tex_old = glGetUniformLocation(mix_shader_program, "uTexOld");
    mix_uniform_decay = glGetUniformLocation(mix_shader_program, "uDecay");
    mix_uniform_tex_scale_new = glGetUniformLocation(mix_shader_program, "uTexScaleNew");
    mix_uniform_tex_scale_old = glGetUniformLocation(mix_shader_program, "uTexScaleOld");

    glUseProgram(mix_shader_program);
    glUniform1i(mix_uniform_tex_new, 0);
    glUniform1i(mix_uniform_tex_old, 1);
    glUseProgram(0);

    Debug("Mix shader initialized (program=%u)", mix_shader_program);
}

static void render_gui(void)
{
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

static void render_emu_normal(void)
{
    float tex_h = (float)current_runtime.screen_width / (float)SYSTEM_TEXTURE_WIDTH;
    float tex_v = (float)current_runtime.screen_height / (float)SYSTEM_TEXTURE_HEIGHT;

    glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer_object);

    glDisable(GL_BLEND);

    update_system_texture();

    render_quad(tex_h, tex_v);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static void render_emu_mix(void)
{
    float tex_h = (float)current_runtime.screen_width / (float)SYSTEM_TEXTURE_WIDTH;
    float tex_v = (float)current_runtime.screen_height / (float)SYSTEM_TEXTURE_HEIGHT;

    int viewportWidth = current_runtime.screen_width * FRAME_BUFFER_SCALE;
    int viewportHeight = current_runtime.screen_height * FRAME_BUFFER_SCALE;

    float persist_h = (float)(current_runtime.screen_width * FRAME_BUFFER_SCALE) / (float)FRAME_BUFFER_WIDTH;
    float persist_v = (float)(current_runtime.screen_height * FRAME_BUFFER_SCALE) / (float)FRAME_BUFFER_HEIGHT;

    update_system_texture();

    // Step 1: Render current frame into the history ring
    glBindFramebuffer(GL_FRAMEBUFFER, history_fbo[frame_history_index]);
    glDisable(GL_BLEND);
    render_quad(tex_h, tex_v);

    frame_history_index = (frame_history_index + 1) % config_video.ghosting_history;

    int src = persistence_index;
    int dst = 1 - persistence_index;

    if (first_frame)
    {
        first_frame = false;

        glBindFramebuffer(GL_FRAMEBUFFER, persistence_fbo[dst]);
        glDisable(GL_BLEND);
        glBindTexture(GL_TEXTURE_2D, history_textures[(frame_history_index + config_video.ghosting_history - 1) % config_video.ghosting_history]);
        render_quad(persist_h, persist_v);
    }
    else
    {
        float decay = config_video.ghosting_intensity;
        int history_count = config_video.ghosting_history;
        float frame_weight = (1.0f - decay) / (float)history_count;

        glBindFramebuffer(GL_FRAMEBUFFER, persistence_fbo[dst]);
        glViewport(0, 0, viewportWidth, viewportHeight);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);

        glUseProgram(quad_shader_program);

        glUniform2f(quad_uniform_tex_scale, persist_h, persist_v);
        glUniform4f(quad_uniform_color, decay, decay, decay, 1.0f);
        glBindTexture(GL_TEXTURE_2D, persistence_texture[src]);

        glBindVertexArray(quad_vao);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glUniform2f(quad_uniform_tex_scale, persist_h, persist_v);
        glUniform4f(quad_uniform_color, frame_weight, frame_weight, frame_weight, 1.0f);

        for (int i = 0; i < history_count; i++)
        {
            glBindTexture(GL_TEXTURE_2D, history_textures[i]);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }

        glBindVertexArray(0);
        glUseProgram(0);
        glDisable(GL_BLEND);
    }

    persistence_index = dst;

    glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer_object);
    glDisable(GL_BLEND);
    glBindTexture(GL_TEXTURE_2D, persistence_texture[dst]);
    render_quad(persist_h, persist_v);

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
    for (int s = 0; s < 5; s++)
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

static void render_quad(float tex_h, float tex_v)
{
    int viewportWidth = current_runtime.screen_width * FRAME_BUFFER_SCALE;
    int viewportHeight = current_runtime.screen_height * FRAME_BUFFER_SCALE;

    glUseProgram(quad_shader_program);
    glUniform2f(quad_uniform_tex_scale, tex_h, tex_v);
    glUniform4f(quad_uniform_color, 1.0f, 1.0f, 1.0f, 1.0f);

    glViewport(0, 0, viewportWidth, viewportHeight);

    glBindVertexArray(quad_vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
    glUseProgram(0);
}

static void render_scanlines(void)
{
    glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer_object);
    glEnable(GL_BLEND);
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

    float tex_h = (float)current_runtime.screen_width;
    float tex_v = (float)current_runtime.screen_height;

    glUseProgram(quad_shader_program);
    glUniform2f(quad_uniform_tex_scale, tex_h, tex_v);
    glUniform4f(quad_uniform_color, 1.0f, 1.0f, 1.0f, config_video.scanlines_intensity);

    glViewport(0, 0, viewportWidth, viewportHeight);

    glBindVertexArray(quad_vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
    glUseProgram(0);

    glDisable(GL_BLEND);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
