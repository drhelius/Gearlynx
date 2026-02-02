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

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include "libretro.h"
#include "gearlynx.h"

#ifdef _WIN32
static const char slash = '\\';
#else
static const char slash = '/';
#endif

#define RETRO_DEVICE_LYNX_PAD    RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 0)

#define MAX_PADS 1
#define JOYPAD_BUTTONS 9

static retro_environment_t environ_cb;
static retro_video_refresh_t video_cb;
static retro_audio_sample_t audio_cb;
static retro_audio_sample_batch_t audio_batch_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;

static struct retro_log_callback logging;
retro_log_printf_t log_cb;

static char retro_system_directory[4096];
static char retro_game_path[4096];

static s16 audio_buf[GLYNX_AUDIO_BUFFER_SIZE];
static int audio_sample_count = 0;

static GLYNX_Runtime_Info runtime_info;
static int current_screen_width = 0;
static int current_screen_height = 0;
static float current_aspect_ratio = 0.0f;
static float aspect_ratio = 0.0f;
static float current_fps = 60.0f;

static bool allow_up_down = false;
static bool input_updated = false;

static bool libretro_supports_bitmasks;
static int joypad_current[MAX_PADS][JOYPAD_BUTTONS];
static int joypad_old[MAX_PADS][JOYPAD_BUTTONS];
static unsigned input_device[MAX_PADS];

static GLYNX_Keys keymap[] = {
    GLYNX_KEY_UP,
    GLYNX_KEY_DOWN,
    GLYNX_KEY_LEFT,
    GLYNX_KEY_RIGHT,
    GLYNX_KEY_A,
    GLYNX_KEY_B,
    GLYNX_KEY_OPTION1,
    GLYNX_KEY_OPTION2,
    GLYNX_KEY_PAUSE
};

static GearlynxCore* core;
static u8* frame_buffer;

static void set_controller_info(void);
static void update_input(void);
static void set_variabless(void);
static void check_variables(void);

static void fallback_log(enum retro_log_level level, const char *fmt, ...)
{
    (void)level;
    va_list va;
    va_start(va, fmt);
    vfprintf(stderr, fmt, va);
    va_end(va);
}

static int IsButtonPressed(int joypad_bits, int button)
{
    return (joypad_bits & (1 << button)) ? 1 : 0;
}

static void load_bootroms(void)
{
    char bios_path[4113];
    snprintf(bios_path, 4113, "%s%clynxboot.img", retro_system_directory, slash);
    GLYNX_Bios_State result = core->LoadBios(bios_path);

    switch (result)
    {
        case BIOS_LOAD_OK:
            log_cb(RETRO_LOG_INFO, "BIOS loaded successfully from %s\n", bios_path);
            break;
        case BIOS_LOAD_FILE_ERROR:
            log_cb(RETRO_LOG_ERROR, "BIOS file error: %s\n", bios_path);
            break;
        case BIOS_LOAD_INVALID_SIZE:
            log_cb(RETRO_LOG_ERROR, "BIOS file has invalid size: %s\n", bios_path);
            break;
        case BIOS_LOAD_INVALID_CRC:
            log_cb(RETRO_LOG_WARN, "BIOS file has invalid CRC: %s\n", bios_path);
            break;
        default:
            log_cb(RETRO_LOG_ERROR, "Unknown error loading BIOS: %s\n", bios_path);
            break;
    }
}

unsigned retro_api_version(void)
{
    return RETRO_API_VERSION;
}

void retro_set_audio_sample(retro_audio_sample_t cb)
{
    audio_cb = cb;
}

void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb)
{
    audio_batch_cb = cb;
}

void retro_set_input_poll(retro_input_poll_t cb)
{
    input_poll_cb = cb;
}

void retro_set_input_state(retro_input_state_t cb)
{
    input_state_cb = cb;
}

void retro_set_video_refresh(retro_video_refresh_t cb)
{
    video_cb = cb;
}

void retro_set_environment(retro_environment_t cb)
{
    environ_cb = cb;

    static const struct retro_system_content_info_override content_overrides[] =
    {
        {
            "lnx|lyx|o",  // extensions
            false,        // need_fullpath
            false         // persistent_data
        },
        { NULL, false, false }
    };

    environ_cb(RETRO_ENVIRONMENT_SET_CONTENT_INFO_OVERRIDE, (void*)content_overrides);
    set_controller_info();
    set_variabless();
}

void retro_init(void)
{
    if (environ_cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &logging))
        log_cb = logging.log;
    else
        log_cb = fallback_log;

    const char *dir = NULL;
    if (environ_cb(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &dir) && dir)
        snprintf(retro_system_directory, sizeof(retro_system_directory), "%s", dir);
    else
        snprintf(retro_system_directory, sizeof(retro_system_directory), "%s", ".");

    log_cb(RETRO_LOG_INFO, "%s (%s) libretro\n", GLYNX_TITLE, GLYNX_VERSION);

    core = new GearlynxCore();

    core->Init(GLYNX_PIXEL_RGB565);
    core->GetRuntimeInfo(runtime_info);

    frame_buffer = new u8[256 * 256 * 2];

    for (int i = 0; i < MAX_PADS; i++)
    {
        for (int j = 0; j < JOYPAD_BUTTONS; j++)
        {
            joypad_current[i][j] = 0;
            joypad_old[i][j] = 0;
        }
    }

    for (int i = 0; i < MAX_PADS; i++)
        input_device[i] = RETRO_DEVICE_LYNX_PAD;

    libretro_supports_bitmasks = environ_cb(RETRO_ENVIRONMENT_GET_INPUT_BITMASKS, NULL);
}

void retro_deinit(void)
{
    SafeDeleteArray(frame_buffer);
    SafeDelete(core);
}

void retro_reset(void)
{
    log_cb(RETRO_LOG_DEBUG, "Resetting...\n");

    check_variables();
    load_bootroms();
    core->ResetROM(true);
}

void retro_set_controller_port_device(unsigned port, unsigned device)
{
    if (port >= MAX_PADS)
    {
        log_cb(RETRO_LOG_DEBUG, "retro_set_controller_port_device invalid port number: %u\n", port);
        return;
    }

    input_device[port] = device;

    switch ( device )
    {
        case RETRO_DEVICE_NONE:
            log_cb(RETRO_LOG_INFO, "Controller %u: Unplugged\n", port);
            break;
        case RETRO_DEVICE_LYNX_PAD:
        case RETRO_DEVICE_JOYPAD:
            log_cb(RETRO_LOG_INFO, "Controller %u: Lynx Pad\n", port);
            break;
        default:
            log_cb(RETRO_LOG_DEBUG, "Setting descriptors for unsupported device.\n");
            break;
    }
}

void retro_get_system_info(struct retro_system_info *info)
{
    memset(info, 0, sizeof(*info));
    info->library_name     = "Gearlynx";
    info->library_version  = GLYNX_VERSION;
    info->need_fullpath    = false;
    info->valid_extensions = "lnx|lyx|o";
}

void retro_get_system_av_info(struct retro_system_av_info *info)
{
    info->geometry.base_width   = 102;
    info->geometry.base_height  = 102;
    info->geometry.max_width    = 160;
    info->geometry.max_height   = 160;
    info->geometry.aspect_ratio = aspect_ratio == 0.0f ? (float)runtime_info.screen_width / (float)runtime_info.screen_height : aspect_ratio;
    info->timing.fps            = current_fps;
    info->timing.sample_rate    = 44100.0;
}

void retro_run(void)
{
    bool core_options_updated = false;
    if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &core_options_updated) && core_options_updated)
        check_variables();

    audio_sample_count = 0;
    core->RunToVBlank(frame_buffer, audio_buf, &audio_sample_count);

    if (!input_updated)
        update_input();

    input_updated = false;

    core->GetRuntimeInfo(runtime_info);

    float new_fps = runtime_info.frame_time > 0.0f ? (1000.0f / runtime_info.frame_time) : 60.0f;
    bool fps_changed = fabsf(new_fps - current_fps) > 0.1f;
    bool geometry_changed = (runtime_info.screen_width != current_screen_width) ||
                            (runtime_info.screen_height != current_screen_height) ||
                            (aspect_ratio != current_aspect_ratio);

    if (fps_changed || geometry_changed)
    {
        current_screen_width = runtime_info.screen_width;
        current_screen_height = runtime_info.screen_height;
        current_aspect_ratio = aspect_ratio;
        current_fps = new_fps;

        retro_system_av_info info;
        info.geometry.base_width   = runtime_info.screen_width;
        info.geometry.base_height  = runtime_info.screen_height;
        info.geometry.max_width    = runtime_info.screen_width;
        info.geometry.max_height   = runtime_info.screen_height;
        info.geometry.aspect_ratio = (aspect_ratio == 0.0f ? (float)runtime_info.screen_width / (float)runtime_info.screen_height : aspect_ratio);
        info.timing.fps            = current_fps;
        info.timing.sample_rate    = 44100.0;

        if (fps_changed)
        {
            log_cb(RETRO_LOG_INFO, "Refresh rate changed to %.2f Hz\n", current_fps);
            environ_cb(RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO, &info);
        }
        else
        {
            environ_cb(RETRO_ENVIRONMENT_SET_GEOMETRY, &info.geometry);
        }
    }

    video_cb((uint8_t*)frame_buffer, runtime_info.screen_width, runtime_info.screen_height, runtime_info.screen_width * sizeof(u8) * 2);

    if (audio_sample_count > 0)
        audio_batch_cb(audio_buf, audio_sample_count / 2);
}

bool retro_load_game(const struct retro_game_info *info)
{
    check_variables();
    load_bootroms();

    snprintf(retro_game_path, sizeof(retro_game_path), "%s", info->path);
    log_cb(RETRO_LOG_INFO, "retro_load_game: %s\n", retro_game_path);

    if (!core->LoadROMFromBuffer(reinterpret_cast<const u8*>(info->data), info->size))
    {
        log_cb(RETRO_LOG_ERROR, "Invalid or corrupted ROM.\n");
        return false;
    }

    enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_RGB565;
    if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt))
    {
        log_cb(RETRO_LOG_ERROR, "RGB565 is not supported.\n");
        return false;
    }

    bool achievements = true;
    environ_cb(RETRO_ENVIRONMENT_SET_SUPPORT_ACHIEVEMENTS, &achievements);

    return true;
}

void retro_unload_game(void)
{
}

unsigned retro_get_region(void)
{
    return RETRO_REGION_NTSC;
}

bool retro_load_game_special(unsigned game_type, const struct retro_game_info *info, size_t num_info)
{
    (void)game_type;
    (void)info;
    (void)num_info;
    return false;
}

size_t retro_serialize_size(void)
{
    size_t size = 0;
    core->SaveState(NULL, size);
    return size;
}

bool retro_serialize(void *data, size_t size)
{
    return core->SaveState(reinterpret_cast<u8*>(data), size);
}

bool retro_unserialize(const void *data, size_t size)
{
    return core->LoadState(reinterpret_cast<const u8*>(data), size);
}

void *retro_get_memory_data(unsigned id)
{
    switch (id)
    {
        case RETRO_MEMORY_SAVE_RAM:
            return core->GetMedia()->GetSaveMemoryPointer();
        case RETRO_MEMORY_SYSTEM_RAM:
            return core->GetMemory()->GetRAM();
    }

    return NULL;
}

size_t retro_get_memory_size(unsigned id)
{
    switch (id)
    {
        case RETRO_MEMORY_SAVE_RAM:
            return core->GetMedia()->GetSaveMemorySize();
        case RETRO_MEMORY_SYSTEM_RAM:
            return 0x10000;
    }

    return 0;
}

void retro_cheat_reset(void)
{
}

void retro_cheat_set(unsigned index, bool enabled, const char *code)
{
    UNUSED(index);
    UNUSED(enabled);
    UNUSED(code);
}

static void set_controller_info(void)
{
    static const struct retro_controller_description port[] = {
        { "Joypad Auto", RETRO_DEVICE_JOYPAD },
        { "Joypad Port Empty", RETRO_DEVICE_NONE },
        { "Lynx Pad", RETRO_DEVICE_LYNX_PAD }
    };

    static const struct retro_controller_info ports[] = {
        { port, 3 },
        { NULL, 0 }
    };

    environ_cb(RETRO_ENVIRONMENT_SET_CONTROLLER_INFO, (void*)ports);

    struct retro_input_descriptor joypad[] = {
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,     "Up" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,   "Down" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,   "Left" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT,  "Right" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,      "A" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,      "B" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,      "Option 1" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,      "Option 2" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START,  "Pause" },

        { 0, 0, 0, 0, NULL }
    };

    environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, joypad);
}

static void update_input(void)
{
    if (input_updated)
        return;

    input_updated = true;

    int16_t joypad_bits[MAX_PADS];

    input_poll_cb();

    if (libretro_supports_bitmasks)
    {
        for (int j = 0; j < MAX_PADS; j++)
            joypad_bits[j] = input_state_cb(j, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_MASK);
    }
    else
    {
        for (int j = 0; j < MAX_PADS; j++)
        {
            joypad_bits[j] = 0;
            for (int i = 0; i < (RETRO_DEVICE_ID_JOYPAD_R3+1); i++)
                joypad_bits[j] |= input_state_cb(j, RETRO_DEVICE_JOYPAD, 0, i) ? (1 << i) : 0;
        }
    }

    // Copy previous state
    for (int j = 0; j < MAX_PADS; j++)
    {
        for (int i = 0; i < JOYPAD_BUTTONS; i++)
            joypad_old[j][i] = joypad_current[j][i];
    }

    // Get current state
    for (int j = 0; j < MAX_PADS; j++)
    {
        int up_pressed = IsButtonPressed(joypad_bits[j], RETRO_DEVICE_ID_JOYPAD_UP);
        int down_pressed = IsButtonPressed(joypad_bits[j], RETRO_DEVICE_ID_JOYPAD_DOWN);
        int left_pressed = IsButtonPressed(joypad_bits[j], RETRO_DEVICE_ID_JOYPAD_LEFT);
        int right_pressed = IsButtonPressed(joypad_bits[j], RETRO_DEVICE_ID_JOYPAD_RIGHT);

        if (allow_up_down)
        {
            joypad_current[j][0] = up_pressed;
            joypad_current[j][1] = down_pressed;
            joypad_current[j][2] = left_pressed;
            joypad_current[j][3] = right_pressed;
        }
        else
        {
            joypad_current[j][0] = (up_pressed && (!down_pressed || joypad_old[j][0])) ? 1 : 0;
            joypad_current[j][1] = (down_pressed && (!up_pressed || joypad_old[j][1])) ? 1 : 0;
            joypad_current[j][2] = (left_pressed && (!right_pressed || joypad_old[j][2])) ? 1 : 0;
            joypad_current[j][3] = (right_pressed && (!left_pressed || joypad_old[j][3])) ? 1 : 0;
        }

        joypad_current[j][4] = IsButtonPressed(joypad_bits[j], RETRO_DEVICE_ID_JOYPAD_A);
        joypad_current[j][5] = IsButtonPressed(joypad_bits[j], RETRO_DEVICE_ID_JOYPAD_B);
        joypad_current[j][6] = IsButtonPressed(joypad_bits[j], RETRO_DEVICE_ID_JOYPAD_L);
        joypad_current[j][7] = IsButtonPressed(joypad_bits[j], RETRO_DEVICE_ID_JOYPAD_R);
        joypad_current[j][8] = IsButtonPressed(joypad_bits[j], RETRO_DEVICE_ID_JOYPAD_START);
    }

    for (int j = 0; j < MAX_PADS; j++)
    {
        for (int i = 0; i < JOYPAD_BUTTONS; i++)
        {
            if (joypad_current[j][i] != joypad_old[j][i])
            {
                if (joypad_current[j][i])
                    core->KeyPressed(keymap[i]);
                else
                    core->KeyReleased(keymap[i]);
            }
        }
    }
}

static void set_variabless(void)
{
    struct retro_variable vars[] = {
        { "gearlynx_aspect_ratio", "Aspect Ratio; 1:1 PAR|4:3 DAR|16:9 DAR|16:10 DAR" },
        { "gearlynx_rotation", "Screen Rotation; Auto|Left|Right|Disabled" },
        { "gearlynx_console_type", "Console Type; Auto|Lynx I|Lynx II" },
        { "gearlynx_lowpass_filter", "Audio Low-Pass Filter (Hz); 3000|500|1000|1500|2000|2500|3000|3500|4000|4500|5000" },
        { "gearlynx_audio_ch0_volume", "Audio Channel 0 Volume; 100|0|10|20|30|40|50|60|70|80|90|100|110|120|130|140|150|160|170|180|190|200" },
        { "gearlynx_audio_ch1_volume", "Audio Channel 1 Volume; 100|0|10|20|30|40|50|60|70|80|90|100|110|120|130|140|150|160|170|180|190|200" },
        { "gearlynx_audio_ch2_volume", "Audio Channel 2 Volume; 100|0|10|20|30|40|50|60|70|80|90|100|110|120|130|140|150|160|170|180|190|200" },
        { "gearlynx_audio_ch3_volume", "Audio Channel 3 Volume; 100|0|10|20|30|40|50|60|70|80|90|100|110|120|130|140|150|160|170|180|190|200" },
        { "gearlynx_up_down_allowed", "Allow Up+Down / Left+Right; Disabled|Enabled" },
        { NULL }
    };

    environ_cb(RETRO_ENVIRONMENT_SET_VARIABLES, (void *)vars);
}

static void check_variables(void)
{
    struct retro_variable var = {0};

    var.key = "gearlynx_aspect_ratio";
    var.value = NULL;

    if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
    {
        if (strcmp(var.value, "1:1 PAR") == 0)
            aspect_ratio = 0.0f;
        else if (strcmp(var.value, "4:3 DAR") == 0)
            aspect_ratio = 4.0f / 3.0f;
        else if (strcmp(var.value, "16:9 DAR") == 0)
            aspect_ratio = 16.0f / 9.0f;
        else if (strcmp(var.value, "16:10 DAR") == 0)
            aspect_ratio = 16.0f / 10.0f;
    }

    var.key = "gearlynx_rotation";
    var.value = NULL;

    if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
    {
        GLYNX_Rotation rotation = GLYNX_ROTATION_AUTO;

        if (strcmp(var.value, "Auto") == 0)
            rotation = GLYNX_ROTATION_AUTO;
        else if (strcmp(var.value, "Left") == 0)
            rotation = GLYNX_ROTATION_LEFT;
        else if (strcmp(var.value, "Right") == 0)
            rotation = GLYNX_ROTATION_RIGHT;
        else if (strcmp(var.value, "Disabled") == 0)
            rotation = GLYNX_ROTATION_DISABLED;

        core->GetMedia()->ForceRotation(rotation);
    }

    var.key = "gearlynx_console_type";
    var.value = NULL;

    if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
    {
        GLYNX_Console_Type console_type = GLYNX_CONSOLE_AUTO;

        if (strcmp(var.value, "Auto") == 0)
            console_type = GLYNX_CONSOLE_AUTO;
        else if (strcmp(var.value, "Lynx I") == 0)
            console_type = GLYNX_CONSOLE_MODEL_I;
        else if (strcmp(var.value, "Lynx II") == 0)
            console_type = GLYNX_CONSOLE_MODEL_II;

        core->GetMedia()->ForceConsoleType(console_type);
    }

    var.key = "gearlynx_lowpass_filter";
    var.value = NULL;

    if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
    {
        float fc = (float)atoi(var.value);
        core->GetAudio()->SetLowpassCutoff(fc);
    }

    for (int i = 0; i < 4; i++)
    {
        char key[64];
        snprintf(key, sizeof(key), "gearlynx_audio_ch%d_volume", i);
        var.key = key;
        var.value = NULL;

        if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
        {
            int volume = atoi(var.value);
            core->GetAudio()->SetVolume(i, volume / 100.0f);
        }
    }

    var.key = "gearlynx_up_down_allowed";
    var.value = NULL;

    if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
    {
        if (strcmp(var.value, "Enabled") == 0)
            allow_up_down = true;
        else
            allow_up_down = false;
    }
}
