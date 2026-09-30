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
#include <time.h>
#include "libretro.h"
#include "gearlynx.h"
#include "game_drive.h"
#include "el_cheapo_sd.h"
#include "sd_card_filesystem_libretro.h"
#include "libretro_core_options.h"
#include "libretro_link.h"
#include "random.h"

#ifdef _WIN32
static const char slash = '\\';
#else
static const char slash = '/';
#endif

#define RETRO_DEVICE_LYNX_PAD    RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 0)

#define MAX_PADS GEARLYNX_LINK_MAX_PLAYERS
#define JOYPAD_BUTTONS 9

static retro_environment_t environ_cb;
static retro_video_refresh_t video_cb;
static retro_audio_sample_batch_t audio_batch_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;

static struct retro_log_callback logging;
retro_log_printf_t log_cb;

static char retro_system_directory[4096];
static char retro_game_path[4096];

static GLYNX_Runtime_Info runtime_info;
static int current_screen_width = 0;
static int current_screen_height = 0;
static float current_aspect_ratio = 0.0f;
static float aspect_ratio = 0.0f;
static float current_fps = 60.0f;

static bool allow_up_down = false;
static bool categories_supported = false;
static bool content_info_ext_supported = false;

static bool libretro_supports_bitmasks = false;
static int joypad_current[MAX_PADS][JOYPAD_BUTTONS];
static int joypad_old[MAX_PADS][JOYPAD_BUTTONS];
static unsigned input_device[MAX_PADS] = {
    RETRO_DEVICE_LYNX_PAD, RETRO_DEVICE_LYNX_PAD, RETRO_DEVICE_LYNX_PAD, RETRO_DEVICE_LYNX_PAD
};

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

static LibretroInstance instances[GEARLYNX_LINK_MAX_PLAYERS];
static unsigned instance_count = 0;
static LibretroLink* link_cable = NULL;
static bool link_enabled = false;
static unsigned link_players = 2;
static int link_placement = LINK_GRID;
static bool link_switched = false;
static int link_screen = 0;
static int link_audio = 0;
static bool link_subsystem = false;
static bool game_loaded = false;
static const retro_vfs_interface* vfs_interface = NULL;

static void set_controller_info(void);
static void clear_input_state(void);
static void reset_controller_devices(void);
static void apply_controller_device(unsigned port, unsigned device, bool log_device);
static void update_input(void);
static void check_variables(void);
static void apply_variables(GearlynxCore* core);
static bool load_game(const struct retro_game_info* info, unsigned count, bool subsystem);

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

static bool IsJoypadDevice(unsigned device)
{
    return ((device == RETRO_DEVICE_JOYPAD) || (device == RETRO_DEVICE_LYNX_PAD));
}

static GLYNX_Bios_State load_bios_file(GearlynxCore* core, const char* path)
{
    if (!vfs_interface || !vfs_interface->open || !vfs_interface->close ||
        !vfs_interface->size || !vfs_interface->read)
        return core->LoadBios(path);

    core->UnloadBios();

    retro_vfs_file_handle* file = vfs_interface->open(path, RETRO_VFS_FILE_ACCESS_READ,
        RETRO_VFS_FILE_ACCESS_HINT_NONE);
    if (!file)
        return BIOS_LOAD_FILE_ERROR;

    s64 size = (s64)vfs_interface->size(file);
    if (size != GLYNX_BIOS_SIZE)
    {
        vfs_interface->close(file);
        return BIOS_LOAD_INVALID_SIZE;
    }

    u8 bios[GLYNX_BIOS_SIZE];
    s64 total = 0;

    while (total < size)
    {
        s64 read = (s64)vfs_interface->read(file, bios + total, size - total);
        if (read <= 0)
            break;

        total += read;
    }

    vfs_interface->close(file);

    if (total != size)
        return BIOS_LOAD_FILE_ERROR;

    return core->LoadBiosFromBuffer(bios, (int)size);
}

static void load_bootroms(GearlynxCore* core)
{
    char bios_path[4113];
    snprintf(bios_path, 4113, "%s%clynxboot.img", retro_system_directory, slash);
    GLYNX_Bios_State result = load_bios_file(core, bios_path);

    switch (result)
    {
        case BIOS_LOAD_OK:
            log_cb(RETRO_LOG_INFO, "BIOS loaded successfully from %s\n", bios_path);
            break;
        case BIOS_LOAD_FILE_ERROR:
        {
            struct retro_message msg = {};
            msg.msg = "BIOS not found: lynxboot.img";
            msg.frames = 360;
            environ_cb(RETRO_ENVIRONMENT_SET_MESSAGE, &msg);
            log_cb(RETRO_LOG_ERROR, "BIOS file error: %s\n", bios_path);
            break;
        }
        case BIOS_LOAD_INVALID_SIZE:
        {
            struct retro_message msg = {};
            msg.msg = "BIOS has invalid size: lynxboot.img";
            msg.frames = 360;
            environ_cb(RETRO_ENVIRONMENT_SET_MESSAGE, &msg);
            log_cb(RETRO_LOG_ERROR, "BIOS file has invalid size: %s\n", bios_path);
            break;
        }
        case BIOS_LOAD_INVALID_CRC:
        {
            struct retro_message msg = {};
            msg.msg = "BIOS has invalid CRC: lynxboot.img";
            msg.frames = 360;
            environ_cb(RETRO_ENVIRONMENT_SET_MESSAGE, &msg);
            log_cb(RETRO_LOG_WARN, "BIOS file has invalid CRC: %s\n", bios_path);
            break;
        }
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
    (void)cb;
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
            "lnx|lyx|o|bin",  // extensions
            false,            // need_fullpath
            false             // persistent_data
        },
        { NULL, false, false }
    };

    content_info_ext_supported = environ_cb(RETRO_ENVIRONMENT_SET_CONTENT_INFO_OVERRIDE, (void*)content_overrides);
    set_controller_info();
    static const struct retro_subsystem_memory_info memory1[] = { { "srm", GEARLYNX_LINK_RAM_1 } };
    static const struct retro_subsystem_memory_info memory2[] = { { "srm2", GEARLYNX_LINK_RAM_2 } };
    static const struct retro_subsystem_memory_info memory3[] = { { "srm3", GEARLYNX_LINK_RAM_3 } };
    static const struct retro_subsystem_memory_info memory4[] = { { "srm4", GEARLYNX_LINK_RAM_4 } };
    static const struct retro_subsystem_rom_info roms[] = {
        { "Screen 1", "lnx|lyx|o|bin", false, false, true, memory1, 1 },
        { "Screen 2", "lnx|lyx|o|bin", false, false, true, memory2, 1 },
        { "Screen 3", "lnx|lyx|o|bin", false, false, true, memory3, 1 },
        { "Screen 4", "lnx|lyx|o|bin", false, false, true, memory4, 1 }
    };
    static const struct retro_subsystem_info subsystems[] = {
        { "2 Player Lynx Link", "lynx_link_2p", roms, 2, GEARLYNX_LINK_SUBSYSTEM_2 },
        { "3 Player Lynx Link", "lynx_link_3p", roms, 3, GEARLYNX_LINK_SUBSYSTEM_3 },
        { "4 Player Lynx Link", "lynx_link_4p", roms, 4, GEARLYNX_LINK_SUBSYSTEM_4 },
        { NULL, NULL, NULL, 0, 0 }
    };
    environ_cb(RETRO_ENVIRONMENT_SET_SUBSYSTEM_INFO, (void*)subsystems);

    libretro_set_core_options(environ_cb, &categories_supported);
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

    struct retro_vfs_interface_info vfs_interface_info = {};
    vfs_interface_info.required_interface_version = 3;
    vfs_interface_info.iface = NULL;

    if (environ_cb(RETRO_ENVIRONMENT_GET_VFS_INTERFACE, &vfs_interface_info) && vfs_interface_info.iface)
    {
        vfs_interface = vfs_interface_info.iface;
        sd_card_set_vfs_interface(vfs_interface);
    }
    else
    {
        vfs_interface = NULL;
        sd_card_set_vfs_interface(NULL);
    }

    clear_input_state();

    for (int i = 0; i < MAX_PADS; i++)
        apply_controller_device(i, input_device[i], false);

    libretro_supports_bitmasks = environ_cb(RETRO_ENVIRONMENT_GET_INPUT_BITMASKS, NULL);
}

void retro_deinit(void)
{
    retro_unload_game();
    vfs_interface = NULL;
    sd_card_set_vfs_interface(NULL);

    current_screen_width = 0;
    current_screen_height = 0;
    current_aspect_ratio = 0.0f;
    aspect_ratio = 0.0f;
    current_fps = 60.0f;
    libretro_supports_bitmasks = false;

    reset_controller_devices();
    clear_input_state();
}

void retro_reset(void)
{
    if (!game_loaded)
        return;

    log_cb(RETRO_LOG_DEBUG, "Resetting...\n");
    check_variables();
    for (unsigned i = 0; i < instance_count; i++)
    {
        load_bootroms(instances[i].core);
        instances[i].core->ResetROM(true);
        memset(instances[i].frame_buffer, 0, sizeof(instances[i].frame_buffer));
        instances[i].sample_count = 0;
    }
    if (link_cable)
        link_cable->Reset();
}

void retro_set_controller_port_device(unsigned port, unsigned device)
{
    if (port >= MAX_PADS)
    {
        if (log_cb)
            log_cb(RETRO_LOG_DEBUG, "retro_set_controller_port_device invalid port number: %u\n", port);
        return;
    }

    input_device[port] = device;

    apply_controller_device(port, device, true);
}

void retro_get_system_info(struct retro_system_info *info)
{
    memset(info, 0, sizeof(*info));
    info->library_name     = GLYNX_TITLE;
    info->library_version  = GLYNX_VERSION;
    info->need_fullpath    = false;
    info->valid_extensions = "lnx|lyx|o|bin";
}

static void get_system_av_info(struct retro_system_av_info* info)
{
    runtime_info.screen_width = GLYNX_SCREEN_WIDTH;
    runtime_info.screen_height = GLYNX_SCREEN_HEIGHT;
    runtime_info.frame_time = 0.0f;
    if (instance_count > 0)
        instances[0].core->GetRuntimeInfo(runtime_info);

    unsigned width = runtime_info.screen_width;
    unsigned height = runtime_info.screen_height;
    if (link_cable)
        link_cable->Geometry(link_placement, link_screen, &width, &height);

    float ratio = aspect_ratio == 0.0f ? (float)width / height : aspect_ratio;
    if (link_cable && aspect_ratio != 0.0f && (link_screen == 0 || (unsigned)link_screen > instance_count))
        ratio *= ((float)width / height) / ((float)runtime_info.screen_width / runtime_info.screen_height);

    info->geometry.base_width   = width;
    info->geometry.base_height  = height;
    info->geometry.max_width    = GLYNX_SCREEN_WIDTH * (link_cable ? GEARLYNX_LINK_MAX_PLAYERS : 1);
    info->geometry.max_height   = GLYNX_SCREEN_WIDTH * (link_cable ? GEARLYNX_LINK_MAX_PLAYERS : 1);
    info->geometry.aspect_ratio = ratio;
    info->timing.fps            = runtime_info.frame_time > 0.0f ? 1000.0f / runtime_info.frame_time : 60.0f;
    info->timing.sample_rate    = 44100.0;
}

void retro_get_system_av_info(struct retro_system_av_info* info)
{
    get_system_av_info(info);
    if (runtime_info.frame_time > 0.0f)
        current_fps = (float)info->timing.fps;
    info->timing.fps = current_fps;
}

void retro_run(void)
{
    if (!game_loaded)
        return;

    bool core_options_updated = false;
    if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &core_options_updated) && core_options_updated)
        check_variables();

    update_input();

    for (unsigned i = 0; i < instance_count; i++)
        instances[i].sample_count = 0;

    if (link_cable)
        link_cable->RunFrame();
    else
        instances[0].core->RunToVBlank((u8*)instances[0].frame_buffer,
            instances[0].audio_buffer, &instances[0].sample_count);

    retro_system_av_info info;
    get_system_av_info(&info);
    bool fps_changed = fabsf((float)info.timing.fps - current_fps) > 0.1f;
    bool geometry_changed = ((int)info.geometry.base_width != current_screen_width) ||
                            ((int)info.geometry.base_height != current_screen_height) ||
                            (info.geometry.aspect_ratio != current_aspect_ratio);

    if (fps_changed || geometry_changed)
    {
        current_screen_width = info.geometry.base_width;
        current_screen_height = info.geometry.base_height;
        current_aspect_ratio = info.geometry.aspect_ratio;
        current_fps = (float)info.timing.fps;

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

    const u16* video = instances[0].frame_buffer;
    const s16* audio = instances[0].audio_buffer;
    int samples = instances[0].sample_count;
    if (link_cable)
    {
        video = link_cable->Video(link_placement, link_switched, link_screen);
        audio = link_cable->Audio(link_audio, &samples);
    }

    video_cb(video, info.geometry.base_width, info.geometry.base_height, info.geometry.base_width * sizeof(u16));
    if (samples > 0)
        audio_batch_cb(audio, samples / 2);
}

static bool load_rom(GearlynxCore* core, const struct retro_game_info* info, const char* path)
{
    if (!info)
        return false;

    if (IsValidPointer(info->data) && (info->size > 0))
        return info->size <= 0x7FFFFFFF && core->LoadROMFromBuffer(reinterpret_cast<const u8*>(info->data), (int)info->size, path);

    if (!path || !path[0])
        return false;

    if (!vfs_interface)
        return core->LoadROM(path);

    if (!vfs_interface->open || !vfs_interface->close || !vfs_interface->size || !vfs_interface->read)
        return false;

    retro_vfs_file_handle* file = vfs_interface->open(path, RETRO_VFS_FILE_ACCESS_READ,
        RETRO_VFS_FILE_ACCESS_HINT_NONE);
    if (!file)
        return false;

    s64 size = (s64)vfs_interface->size(file);
    if ((size <= 0) || (size > 0x7FFFFFFF))
    {
        vfs_interface->close(file);
        return false;
    }

    u8* buffer = new u8[(int)size];
    s64 total = 0;

    while (total < size)
    {
        s64 read = (s64)vfs_interface->read(file, buffer + total, size - total);
        if (read <= 0)
            break;

        total += read;
    }

    bool loaded = vfs_interface->close(file) == 0 && total == size;
    if (loaded)
        loaded = core->LoadROMFromBuffer(buffer, (int)size, path);

    SafeDeleteArray(buffer);
    return loaded;
}

bool retro_load_game(const struct retro_game_info* info)
{
    return load_game(info, 1, false);
}

static bool load_game(const struct retro_game_info* info, unsigned count, bool subsystem)
{
    retro_unload_game();
    if (!info)
        return false;

    check_variables();
    link_subsystem = subsystem;
    instance_count = subsystem ? count : (link_enabled ? link_players : 1);

    Random random;
    random.Seed((u32)time(NULL));
    for (unsigned i = 0; i < instance_count; i++)
    {
        memset(&instances[i], 0, sizeof(instances[i]));
        instances[i].core = new GearlynxCore();
        instances[i].core->Init(GLYNX_PIXEL_RGB565);
        if (instance_count > 1)
            instances[i].core->SetRandomSeed(random.Next());
    }
    check_variables();

    const struct retro_game_info_ext* game_info_ext = NULL;
    if (content_info_ext_supported)
        environ_cb(RETRO_ENVIRONMENT_GET_GAME_INFO_EXT, &game_info_ext);

    for (unsigned i = 0; i < instance_count; i++)
    {
        GearlynxCore* core = instances[i].core;
        unsigned content = subsystem ? i : 0;
        load_bootroms(core);

        const char* game_path = info[content].path ? info[content].path : "";
        char extended_game_path[4096] = {};
        if (game_info_ext)
        {
            const struct retro_game_info_ext* extended = &game_info_ext[content];
            if (extended->full_path && extended->full_path[0])
                game_path = extended->full_path;
            else if (extended->dir && extended->dir[0] && extended->name && extended->name[0])
            {
                const char* extension = (extended->ext && extended->ext[0]) ? extended->ext : "lnx";
                snprintf(extended_game_path, sizeof(extended_game_path), "%s/%s.%s", extended->dir, extended->name, extension);
                game_path = extended_game_path;
            }
        }

        if (i == 0)
            snprintf(retro_game_path, sizeof(retro_game_path), "%s", game_path);
        log_cb(RETRO_LOG_INFO, "Loading screen %u: %s\n", i + 1, game_path);

        if (!load_rom(core, &info[content], game_path))
        {
            log_cb(RETRO_LOG_ERROR, "Invalid or corrupted ROM for screen %u.\n", i + 1);
            retro_unload_game();
            return false;
        }

        GLYNX_Cartridge_Hardware cartridge_hardware = core->GetMedia()->GetCartridgeHardware();
        bool sd_unavailable =
            (cartridge_hardware == GLYNX_CARTRIDGE_HARDWARE_GAME_DRIVE && !core->GetMedia()->GetGameDriveInstance()->IsAvailable()) ||
            (cartridge_hardware == GLYNX_CARTRIDGE_HARDWARE_EL_CHEAPO_SD && !core->GetMedia()->GetElCheapoSDInstance()->IsAvailable());

        if (sd_unavailable)
        {
            struct retro_message msg = {};
            msg.msg = "SD cartridge requires frontend VFS v3 and a content directory";
            msg.frames = 360;
            environ_cb(RETRO_ENVIRONMENT_SET_MESSAGE, &msg);
            log_cb(RETRO_LOG_WARN, "%s.\n", msg.msg);
        }
    }

    enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_RGB565;
    if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt))
    {
        log_cb(RETRO_LOG_ERROR, "RGB565 is not supported.\n");
        retro_unload_game();
        return false;
    }

    if (instance_count > 1)
    {
        link_cable = new LibretroLink(instances, instance_count);
        link_cable->Reset();
    }

    bool achievements = instance_count == 1;
    environ_cb(RETRO_ENVIRONMENT_SET_SUPPORT_ACHIEVEMENTS, &achievements);
    game_loaded = true;
    clear_input_state();

    if (link_cable && !link_subsystem)
    {
        const char* directory = NULL;
        environ_cb(RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY, &directory);
        link_cable->SetSavePath(retro_game_path, directory);
        link_cable->PersistentMemory(false, vfs_interface);
    }

    return true;
}

void retro_unload_game(void)
{
    if (game_loaded && link_cable && !link_subsystem)
        link_cable->PersistentMemory(true, vfs_interface);

    SafeDelete(link_cable);
    for (unsigned i = 0; i < instance_count; i++)
    {
        SafeDelete(instances[i].core);
        instances[i].sample_count = 0;
    }

    instance_count = 0;
    game_loaded = false;
    link_subsystem = false;
    retro_game_path[0] = 0;
    current_screen_width = 0;
    current_screen_height = 0;
    current_aspect_ratio = 0.0f;
    current_fps = 60.0f;
    clear_input_state();
}

unsigned retro_get_region(void)
{
    return RETRO_REGION_NTSC;
}

bool retro_load_game_special(unsigned type, const struct retro_game_info* info, size_t num)
{
    unsigned count;
    switch (type)
    {
        case GEARLYNX_LINK_SUBSYSTEM_2: count = 2; break;
        case GEARLYNX_LINK_SUBSYSTEM_3: count = 3; break;
        case GEARLYNX_LINK_SUBSYSTEM_4: count = 4; break;
        default: return false;
    }
    if (!info || num != count)
        return false;
    return load_game(info, count, true);
}

size_t retro_serialize_size(void)
{
    if (!game_loaded || link_cable)
        return 0;

    size_t size = 0;
    if (!instances[0].core->GetMaxSaveStateSize(size))
        return 0;

    return size;
}

bool retro_serialize(void *data, size_t size)
{
    return game_loaded && !link_cable && instances[0].core->SaveState(reinterpret_cast<u8*>(data), size);
}

bool retro_unserialize(const void *data, size_t size)
{
    return game_loaded && !link_cable && instances[0].core->LoadState(reinterpret_cast<const u8*>(data), size);
}

static GearlynxCore* memory_instance(unsigned id)
{
    if (!game_loaded)
        return NULL;
    if (id < 0x100)
        return instances[0].core;

    unsigned index = (id >> 8) - 1;
    if (index >= instance_count || (id & 0xFF) != RETRO_MEMORY_SAVE_RAM)
        return NULL;
    return instances[index].core;
}

void *retro_get_memory_data(unsigned id)
{
    GearlynxCore* core = memory_instance(id);
    if (!core)
        return NULL;

    switch (id & 0xFF)
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
    GearlynxCore* core = memory_instance(id);
    if (!core)
        return 0;

    switch (id & 0xFF)
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
        { port, 3 },
        { port, 3 },
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
        { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,     "Up" },
        { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,   "Down" },
        { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,   "Left" },
        { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT,  "Right" },
        { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,      "A" },
        { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,      "B" },
        { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,      "Option 1" },
        { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,      "Option 2" },
        { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START,  "Pause" },
        { 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,     "Up" },
        { 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,   "Down" },
        { 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,   "Left" },
        { 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT,  "Right" },
        { 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,      "A" },
        { 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,      "B" },
        { 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,      "Option 1" },
        { 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,      "Option 2" },
        { 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START,  "Pause" },
        { 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,     "Up" },
        { 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,   "Down" },
        { 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,   "Left" },
        { 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT,  "Right" },
        { 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,      "A" },
        { 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,      "B" },
        { 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,      "Option 1" },
        { 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,      "Option 2" },
        { 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START,  "Pause" },
        { 0, 0, 0, 0, NULL }
    };

    environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, joypad);
}

static void clear_input_state(void)
{
    for (int i = 0; i < MAX_PADS; i++)
    {
        for (int j = 0; j < JOYPAD_BUTTONS; j++)
        {
            joypad_current[i][j] = 0;
            joypad_old[i][j] = 0;
        }
    }
}

static void reset_controller_devices(void)
{
    for (int i = 0; i < MAX_PADS; i++)
        input_device[i] = RETRO_DEVICE_LYNX_PAD;
}

static void apply_controller_device(unsigned port, unsigned device, bool log_device)
{
    if (!log_device || !log_cb)
        return;

    switch (device)
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

static void update_input(void)
{
    int16_t joypad_bits[MAX_PADS];

    input_poll_cb();

    if (libretro_supports_bitmasks)
    {
        for (int j = 0; j < (int)instance_count; j++)
        {
            if (IsJoypadDevice(input_device[j]))
                joypad_bits[j] = input_state_cb(j, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_MASK);
            else
                joypad_bits[j] = 0;
        }
    }
    else
    {
        for (int j = 0; j < (int)instance_count; j++)
        {
            joypad_bits[j] = 0;
            if (IsJoypadDevice(input_device[j]))
            {
                for (int i = 0; i < (RETRO_DEVICE_ID_JOYPAD_R3+1); i++)
                    joypad_bits[j] |= input_state_cb(j, RETRO_DEVICE_JOYPAD, 0, i) ? (1 << i) : 0;
            }
        }
    }

    // Copy previous state
    for (int j = 0; j < (int)instance_count; j++)
    {
        for (int i = 0; i < JOYPAD_BUTTONS; i++)
            joypad_old[j][i] = joypad_current[j][i];
    }

    // Get current state
    for (int j = 0; j < (int)instance_count; j++)
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
            int up = up_pressed;
            int down = down_pressed;
            int left = left_pressed;
            int right = right_pressed;

            if (up_pressed && down_pressed)
            {
                if (joypad_old[j][0])
                {
                    up = 1;
                    down = 0;
                }
                else if (joypad_old[j][1])
                {
                    up = 0;
                    down = 1;
                }
                else
                {
                    up = 1;
                    down = 0;
                }
            }

            if (left_pressed && right_pressed)
            {
                if (joypad_old[j][2])
                {
                    left = 1;
                    right = 0;
                }
                else if (joypad_old[j][3])
                {
                    left = 0;
                    right = 1;
                }
                else
                {
                    left = 1;
                    right = 0;
                }
            }

            joypad_current[j][0] = up;
            joypad_current[j][1] = down;
            joypad_current[j][2] = left;
            joypad_current[j][3] = right;
        }

        joypad_current[j][4] = IsButtonPressed(joypad_bits[j], RETRO_DEVICE_ID_JOYPAD_A);
        joypad_current[j][5] = IsButtonPressed(joypad_bits[j], RETRO_DEVICE_ID_JOYPAD_B);
        joypad_current[j][6] = IsButtonPressed(joypad_bits[j], RETRO_DEVICE_ID_JOYPAD_L);
        joypad_current[j][7] = IsButtonPressed(joypad_bits[j], RETRO_DEVICE_ID_JOYPAD_R);
        joypad_current[j][8] = IsButtonPressed(joypad_bits[j], RETRO_DEVICE_ID_JOYPAD_START);
    }

    for (int j = 0; j < (int)instance_count; j++)
    {
        for (int i = 0; i < JOYPAD_BUTTONS; i++)
        {
            if (joypad_current[j][i])
                instances[j].core->KeyPressed(keymap[i]);
            else
                instances[j].core->KeyReleased(keymap[i]);
        }
    }
}

static void check_variables(void)
{
    struct retro_variable var = {};

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

    var.key = "gearlynx_link_enable";
    var.value = NULL;
    link_enabled = environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value && strcmp(var.value, "Enabled") == 0;

    var.key = "gearlynx_link_players";
    var.value = NULL;
    link_players = 2;
    if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
    {
        if (strcmp(var.value, "3") == 0)
            link_players = 3;
        else if (strcmp(var.value, "4") == 0)
            link_players = 4;
    }

    var.key = "gearlynx_link_placement";
    var.value = NULL;
    link_placement = LINK_GRID;
    if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
    {
        if (strcmp(var.value, "Vertical") == 0)
            link_placement = LINK_VERTICAL;
        else if (strcmp(var.value, "Horizontal") == 0)
            link_placement = LINK_HORIZONTAL;
    }

    var.key = "gearlynx_link_switch";
    var.value = NULL;
    link_switched = environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value && strcmp(var.value, "Enabled") == 0;

    var.key = "gearlynx_link_screen";
    var.value = NULL;
    link_screen = 0;
    if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
    {
        for (unsigned i = 0; i < GEARLYNX_LINK_MAX_PLAYERS; i++)
        {
            char screen[16];
            snprintf(screen, sizeof(screen), "Screen %u", i + 1);
            if (strcmp(var.value, screen) == 0)
                link_screen = i + 1;
        }
    }

    var.key = "gearlynx_link_audio";
    var.value = NULL;
    link_audio = 0;
    if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
    {
        if (strcmp(var.value, "Mix") == 0)
            link_audio = GEARLYNX_LINK_MAX_PLAYERS;
        else
        {
            for (unsigned i = 0; i < GEARLYNX_LINK_MAX_PLAYERS; i++)
            {
                char screen[16];
                snprintf(screen, sizeof(screen), "Screen %u", i + 1);
                if (strcmp(var.value, screen) == 0)
                    link_audio = i;
            }
        }
    }

    for (unsigned i = 0; i < instance_count; i++)
        apply_variables(instances[i].core);
}

static void apply_variables(GearlynxCore* core)
{
    struct retro_variable var = {};

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
        else if (strcmp(var.value, "180") == 0)
            rotation = GLYNX_ROTATION_180;

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

    var.key = "gearlynx_eeprom_type";
    var.value = NULL;

    if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
    {
        GLYNX_EEPROM eeprom = GLYNX_EEPROM_NONE;
        bool force = true;

        if (strcmp(var.value, "Auto") == 0)
            force = false;
        else if (strcmp(var.value, "None") == 0)
            eeprom = GLYNX_EEPROM_NONE;
        else if (strcmp(var.value, "93C46_16bit") == 0)
            eeprom = GLYNX_EEPROM_93C46;
        else if (strcmp(var.value, "93C46_8bit") == 0)
            eeprom = (GLYNX_EEPROM)(GLYNX_EEPROM_93C46 | GLYNX_EEPROM_8BIT);
        else if (strcmp(var.value, "93C56_16bit") == 0)
            eeprom = GLYNX_EEPROM_93C56;
        else if (strcmp(var.value, "93C56_8bit") == 0)
            eeprom = (GLYNX_EEPROM)(GLYNX_EEPROM_93C56 | GLYNX_EEPROM_8BIT);
        else if (strcmp(var.value, "93C66_16bit") == 0)
            eeprom = GLYNX_EEPROM_93C66;
        else if (strcmp(var.value, "93C66_8bit") == 0)
            eeprom = (GLYNX_EEPROM)(GLYNX_EEPROM_93C66 | GLYNX_EEPROM_8BIT);
        else if (strcmp(var.value, "93C76_16bit") == 0)
            eeprom = GLYNX_EEPROM_93C76;
        else if (strcmp(var.value, "93C76_8bit") == 0)
            eeprom = (GLYNX_EEPROM)(GLYNX_EEPROM_93C76 | GLYNX_EEPROM_8BIT);
        else if (strcmp(var.value, "93C86_16bit") == 0)
            eeprom = GLYNX_EEPROM_93C86;
        else if (strcmp(var.value, "93C86_8bit") == 0)
            eeprom = (GLYNX_EEPROM)(GLYNX_EEPROM_93C86 | GLYNX_EEPROM_8BIT);
        else
            force = false;

        if (force)
            core->GetMedia()->ForceEEPROM(eeprom);
        else
            core->GetMedia()->AutoDetectEEPROM();
    }

    var.key = "gearlynx_cartridge_hardware";
    var.value = NULL;

    if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
    {
        if (strcmp(var.value, "Standard") == 0)
            core->GetMedia()->ForceCartridgeHardware(GLYNX_CARTRIDGE_HARDWARE_STANDARD);
        else if (strcmp(var.value, "GameDrive") == 0)
            core->GetMedia()->ForceCartridgeHardware(GLYNX_CARTRIDGE_HARDWARE_GAME_DRIVE);
        else if (strcmp(var.value, "ElCheapoSD") == 0)
            core->GetMedia()->ForceCartridgeHardware(GLYNX_CARTRIDGE_HARDWARE_EL_CHEAPO_SD);
        else
            core->GetMedia()->AutoDetectCartridgeHardware();
    }

    var.key = "gearlynx_legacy_sprite_renderer";
    var.value = NULL;

    bool fast_sprite_rendering = false;
    if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
        fast_sprite_rendering = strcmp(var.value, "Enabled") == 0;
    core->GetSuzy()->SetFastSpriteRendering(fast_sprite_rendering);

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
