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
#define EMU_IMPORT
#include "emu.h"

#include "gearlynx.h"
#include "sound_queue.h"
#include "config.h"
#include "mcp/mcp_manager.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#if defined(_WIN32)
#define STBIW_WINDOWS_UTF8
#endif
#include "stb_image_write.h"

static GearlynxCore* core;
static s16* audio_buffer;
static bool audio_enabled;
static McpManager* mcp_manager;

static void save_ram(void);
static void load_ram(void);
static void reset_buffers(void);
static const char* get_configurated_dir(int option, const char* path);
static void init_debug(void);
static void destroy_debug(void);
static void update_debug(void);
static void update_debug_framebuffers(void);
static void update_debug_sprites(void);
static void update_debug_sprites_accumulated(void);
static void render_debug_sprites(int count);

bool emu_init(void)
{
    emu_collision_palette[0]  = 0xFF000000; // 0: Black
    emu_collision_palette[1]  = 0xFF800000; // 1: Blue
    emu_collision_palette[2]  = 0xFF008000; // 2: Green
    emu_collision_palette[3]  = 0xFF808000; // 3: Cyan
    emu_collision_palette[4]  = 0xFF000080; // 4: Red
    emu_collision_palette[5]  = 0xFF800080; // 5: Magenta
    emu_collision_palette[6]  = 0xFF0055AA; // 6: Brown
    emu_collision_palette[7]  = 0xFFAAAAAA; // 7: Light Gray
    emu_collision_palette[8]  = 0xFF555555; // 8: Dark Gray
    emu_collision_palette[9]  = 0xFFFF5555; // 9: Light Blue
    emu_collision_palette[10] = 0xFF55FF55; // 10: Light Green
    emu_collision_palette[11] = 0xFFFFFF55; // 11: Light Cyan
    emu_collision_palette[12] = 0xFF5555FF; // 12: Light Red
    emu_collision_palette[13] = 0xFFFF55FF; // 13: Light Magenta
    emu_collision_palette[14] = 0xFF55FFFF; // 14: Yellow
    emu_collision_palette[15] = 0xFFFFFFFF; // 15: White

    emu_frame_buffer = new u8[256 * 256 * 4];
    audio_buffer = new s16[GLYNX_AUDIO_BUFFER_SIZE];

    init_debug();
    reset_buffers();

    core = new GearlynxCore();
    core->Init();

    sound_queue_init();

    for (int i = 0; i < 5; i++)
        InitPointer(emu_savestates_screenshots[i].data);

    audio_enabled = true;
    emu_audio_sync = true;
    emu_debug_disable_breakpoints = false;
    emu_debug_command = Debug_Command_None;
    emu_debug_pc_changed = false;
    emu_debug_step_frames_pending = 0;
    for (int i = 0; i < 8; i++)
        emu_debug_irq_breakpoints[i] = false;

    mcp_manager = new McpManager();
    mcp_manager->Init(core);

    return true;
}

void emu_destroy(void)
{
    save_ram();
    SafeDelete(mcp_manager);
    SafeDeleteArray(audio_buffer);
    sound_queue_destroy();
    SafeDelete(core);
    SafeDeleteArray(emu_frame_buffer);
    destroy_debug();

    for (int i = 0; i < 5; i++)
        SafeDeleteArray(emu_savestates_screenshots[i].data);
}

bool emu_load_rom(const char* file_path)
{
    emu_debug_command = Debug_Command_None;
    reset_buffers();
    emu_audio_reset();

    save_ram();

    if (!core->LoadROM(file_path))
        return false;

    load_ram();

    if (config_debug.debug && (config_debug.dis_look_ahead_count > 0))
        core->GetM6502()->DisassembleAhead(config_debug.dis_look_ahead_count);

    update_savestates_data();

    return true;
}

void emu_update(void)
{
    emu_mcp_pump_commands();

    if (emu_is_empty())
        return;

    int sampleCount = 0;

    if (config_debug.debug)
    {
        bool breakpoint_hit = false;
        GearlynxCore::GLYNX_Debug_Run debug_run;
        debug_run.step_debugger = (emu_debug_command == Debug_Command_Step);
        debug_run.stop_on_breakpoint = !emu_debug_disable_breakpoints;
        debug_run.stop_on_run_to_breakpoint = true;

        debug_run.skip_interrupts_on_step = config_debug.step_skip_interrupts && (emu_debug_command == Debug_Command_Step);

        debug_run.stop_on_irq = 0;
        for (int i = 0; i < 8; i++)
        {
            if (emu_debug_irq_breakpoints[i])
                debug_run.stop_on_irq = SET_BIT(debug_run.stop_on_irq, i);
        }

        bool executed = (emu_debug_command != Debug_Command_None);

        if (executed)
            breakpoint_hit = core->RunToVBlank(emu_frame_buffer, audio_buffer, &sampleCount, &debug_run);

        if (breakpoint_hit || emu_debug_command == Debug_Command_StepFrame || emu_debug_command == Debug_Command_Step)
        {
            emu_debug_pc_changed = true;

            if (config_debug.dis_look_ahead_count > 0)
                core->GetM6502()->DisassembleAhead(config_debug.dis_look_ahead_count);

        }

        if (breakpoint_hit)
            emu_debug_command = Debug_Command_None;

        if (emu_debug_command == Debug_Command_StepFrame && emu_debug_step_frames_pending > 0)
        {
            emu_debug_step_frames_pending--;
            if (emu_debug_step_frames_pending > 0)
                emu_debug_command = Debug_Command_StepFrame;
            else
                emu_debug_command = Debug_Command_None;
        }
        else if (emu_debug_command != Debug_Command_Continue)
            emu_debug_command = Debug_Command_None;

        if (executed)
            update_debug();
    }
    else
        core->RunToVBlank(emu_frame_buffer, audio_buffer, &sampleCount);

    if ((sampleCount > 0) && !core->IsPaused())
    {
        sound_queue_write(audio_buffer, sampleCount, emu_audio_sync);
    }
}

void emu_key_pressed(GLYNX_Keys key)
{
    core->KeyPressed(key);
}

void emu_key_released(GLYNX_Keys key)
{
    core->KeyReleased(key);
}

void emu_pause(void)
{
    core->Pause(true);
}

void emu_resume(void)
{
    core->Pause(false);
}

bool emu_is_paused(void)
{
    return core->IsPaused();
}

bool emu_is_debug_idle(void)
{
    return config_debug.debug && (emu_debug_command == Debug_Command_None);
}

bool emu_is_empty(void)
{
    return !core->GetMedia()->IsReady();
}

bool emu_is_bios_loaded(void)
{
    return core->GetMedia()->IsBiosLoaded();
}

GLYNX_Bios_State emu_load_bios(const char* file_path)
{
    return core->LoadBios(file_path);
}

void emu_reset(void)
{
    emu_debug_command = Debug_Command_None;
    reset_buffers();
    emu_audio_reset();

    save_ram();
    core->ResetROM(false);
    load_ram();
}

void emu_force_rotation(int rotation)
{
    core->GetMedia()->ForceRotation((GLYNX_Rotation)rotation);
}

void emu_force_console_type(int console_type)
{
    core->GetMedia()->ForceConsoleType((GLYNX_Console_Type)console_type);
}

void emu_audio_mute(bool mute)
{
    audio_enabled = !mute;
    core->GetAudio()->Mute(mute);
}

void emu_audio_set_volume(int channel, float volume)
{
    core->GetAudio()->SetVolume(channel, volume);
}

void emu_audio_set_lowpass_cutoff(float fc)
{
    core->GetAudio()->SetLowpassCutoff(fc);
}

void emu_audio_reset(void)
{
    sound_queue_stop();
    sound_queue_start(GLYNX_AUDIO_SAMPLE_RATE, 2, GLYNX_AUDIO_QUEUE_SIZE, config_audio.buffer_count);
}

bool emu_is_audio_enabled(void)
{
    return audio_enabled;
}

bool emu_is_audio_open(void)
{
    return sound_queue_is_open();
}

void emu_save_ram(const char* file_path)
{
    if (!emu_is_empty())
        core->SaveRam(file_path, true);
}

void emu_load_ram(const char* file_path)
{
    if (!emu_is_empty())
    {
        save_ram();
        core->ResetROM(false);
        core->LoadRam(file_path, true);
    }
}

void emu_save_state_slot(int index)
{
    if (!emu_is_empty())
    {
        const char* dir = get_configurated_dir(config_emulator.savestates_dir_option, config_emulator.savestates_path.c_str());
        core->SaveState(dir, index, true);
        update_savestates_data();
    }
}

void emu_load_state_slot(int index)
{
    if (!emu_is_empty())
    {
        const char* dir = get_configurated_dir(config_emulator.savestates_dir_option, config_emulator.savestates_path.c_str());
        core->LoadState(dir, index);
    }
}

void emu_save_state_file(const char* file_path)
{
    if (!emu_is_empty())
        core->SaveState(file_path, -1, true);
}

void emu_load_state_file(const char* file_path)
{
    if (!emu_is_empty())
        core->LoadState(file_path);
}

void update_savestates_data(void)
{
    if (emu_is_empty())
        return;

    for (int i = 0; i < 5; i++)
    {
        emu_savestates[i].rom_name[0] = 0;
        SafeDeleteArray(emu_savestates_screenshots[i].data);

        const char* dir = get_configurated_dir(config_emulator.savestates_dir_option, config_emulator.savestates_path.c_str());

        if (!core->GetSaveStateHeader(i + 1, dir, &emu_savestates[i]))
            continue;

        if (emu_savestates[i].screenshot_size > 0)
        {
            emu_savestates_screenshots[i].data = new u8[emu_savestates[i].screenshot_size];
            emu_savestates_screenshots[i].size = emu_savestates[i].screenshot_size;
            core->GetSaveStateScreenshot(i + 1, dir, &emu_savestates_screenshots[i]);
        }
    }
}


void emu_get_runtime(GLYNX_Runtime_Info& runtime)
{
    core->GetRuntimeInfo(runtime);
}

void emu_get_info(char* info, int buffer_size)
{
    if (!emu_is_empty())
    {
        Media* media = core->GetMedia();
        GLYNX_Runtime_Info runtime;
        core->GetRuntimeInfo(runtime);

        const char* filename = media->GetFileName();
        u32 crc = media->GetCRC();
        int rom_size = media->GetROMSize();
        const char* header_name = media->GetHeaderName();
        const char* header_manufacturer = media->GetHeaderManufacturer();
        u16 bank0_page_size = media->GetHeaderBank0PageSize();
        u16 bank1_page_size = media->GetHeaderBank1PageSize();
        GLYNX_Rotation rotation = media->GetRotation();
        bool audin = media->GetAudin();
        GLYNX_EEPROM eeprom = media->GetEEPROM();

        const char* rotation_str = "None";
        switch (rotation)
        {
            case GLYNX_ROTATION_LEFT:
                rotation_str = "Left";
                break;
            case GLYNX_ROTATION_RIGHT:
                rotation_str = "Right";
                break;
            default:
                rotation_str = "None";
                break;
        }

        const char* eeprom_str = "None";
        int eeprom_base = eeprom & 0x0F;
        switch (eeprom_base)
        {
            case GLYNX_EEPROM_93C46:
                eeprom_str = "93C46";
                break;
            case GLYNX_EEPROM_93C56:
                eeprom_str = "93C56";
                break;
            case GLYNX_EEPROM_93C66:
                eeprom_str = "93C66";
                break;
            case GLYNX_EEPROM_93C76:
                eeprom_str = "93C76";
                break;
            case GLYNX_EEPROM_93C86:
                eeprom_str = "93C86";
                break;
            default:
                eeprom_str = "None";
                break;
        }

        snprintf(info, buffer_size,
            "File Name: %s\n"
            "CRC: %08X\n"
            "ROM Size: %d bytes (%d KB)\n"
            "Screen: %dx%d\n"
            "Header Name: %s\n"
            "Header Manufacturer: %s\n"
            "Bank0 Page Size: %d\n"
            "Bank1 Page Size: %d\n"
            "Rotation: %s\n"
            "AUDIN: %s\n"
            "EEPROM: %s%s",
            filename, crc, rom_size, rom_size / 1024,
            runtime.screen_width, runtime.screen_height,
            header_name[0] ? header_name : "(none)",
            header_manufacturer[0] ? header_manufacturer : "(none)",
            bank0_page_size, bank1_page_size,
            rotation_str,
            audin ? "Yes" : "No",
            eeprom_str,
            (eeprom & GLYNX_EEPROM_8BIT) ? " (8-bit)" : "");
    }
    else
    {
        snprintf(info, buffer_size, "There is no ROM loaded!");
    }
}

GearlynxCore* emu_get_core(void)
{
    return core;
}

void emu_debug_step_over(void)
{
    M6502* processor = emu_get_core()->GetM6502();
    M6502::M6502_State* proc_state = processor->GetState();
    Memory* memory = emu_get_core()->GetMemory();
    u16 pc = proc_state->PC.GetValue();
    GLYNX_Disassembler_Record* record = memory->GetDisassemblerRecord(pc);

    if (IsValidPointer(record) && record->subroutine)
    {
        u16 return_address = pc + record->size;
        processor->AddRunToBreakpoint(return_address);
        emu_debug_command = Debug_Command_Continue;
    }
    else
        emu_debug_command = Debug_Command_Step;

    core->Pause(false);
}

void emu_debug_step_into(void)
{
    core->Pause(false);
    emu_debug_command = Debug_Command_Step;
}

void emu_debug_step_out(void)
{
    M6502* processor = emu_get_core()->GetM6502();
    std::stack<M6502::GLYNX_CallStackEntry>* call_stack = processor->GetDisassemblerCallStack();

    if (call_stack->size() > 0)
    {
        M6502::GLYNX_CallStackEntry entry = call_stack->top();
        u16 return_address = entry.back;
        processor->AddRunToBreakpoint(return_address);
        emu_debug_command = Debug_Command_Continue;
    }
    else
        emu_debug_command = Debug_Command_Step;

    core->Pause(false);
}

void emu_debug_step_frame(void)
{
    core->Pause(false);
    emu_debug_step_frames_pending++;
    emu_debug_command = Debug_Command_StepFrame;
}

void emu_debug_break(void)
{
    core->Pause(false);
    if (emu_debug_command == Debug_Command_Continue)
        emu_debug_command = Debug_Command_Step;
}

void emu_debug_continue(void)
{
    core->Pause(false);
    emu_debug_command = Debug_Command_Continue;
}

void emu_debug_set_callback(GearlynxCore::GLYNX_Debug_Callback callback)
{
    core->SetDebugCallback(callback);
}

void emu_save_screenshot(const char* file_path)
{
    if (!core->GetMedia()->IsReady())
        return;

    GLYNX_Runtime_Info runtime;
    emu_get_runtime(runtime);

    stbi_write_png(file_path, runtime.screen_width, runtime.screen_height, 4, emu_frame_buffer, runtime.screen_width * 4);

    Log("Screenshot saved to %s", file_path);
}

void emu_save_sprite(const char* file_path, int index)
{
    if (index < 0 || index >= DEBUG_MAX_SPRITES)
        return;

    int width = emu_debug_sprite_widths[index];
    int height = emu_debug_sprite_heights[index];
    u8* buffer = emu_debug_sprite_buffers[index];

    if (!buffer || width <= 0 || height <= 0)
        return;

    stbi_write_png(file_path, width, height, 4, buffer, 256 * 4);

    Log("Sprite saved to %s", file_path);
}

int emu_get_screenshot_png(unsigned char** out_buffer)
{
    if (!core->GetMedia()->IsReady())
        return 0;

    GLYNX_Runtime_Info runtime;
    emu_get_runtime(runtime);

    int stride = runtime.screen_width * 4;
    int len = 0;

    *out_buffer = stbi_write_png_to_mem(emu_frame_buffer, stride, 
                                         runtime.screen_width, runtime.screen_height, 
                                         4, &len);

    return len;
}

int emu_get_framebuffer_png(int buffer_index, unsigned char** out_buffer)
{
    if (!core->GetMedia()->IsReady())
        return 0;

    if (buffer_index < 0 || buffer_index > 1)
        return 0;

    int stride = GLYNX_SCREEN_WIDTH * 4;
    int len = 0;

    *out_buffer = stbi_write_png_to_mem(emu_debug_framebuffer[buffer_index], stride,
                                         GLYNX_SCREEN_WIDTH, GLYNX_SCREEN_HEIGHT,
                                         4, &len);

    return len;
}

int emu_get_sprite_png(int index, unsigned char** out_buffer)
{
    if (index < 0 || index >= DEBUG_MAX_SPRITES)
        return 0;

    int width = emu_debug_sprite_widths[index];
    int height = emu_debug_sprite_heights[index];
    u8* buffer = emu_debug_sprite_buffers[index];

    if (!buffer || width <= 0 || height <= 0)
        return 0;

    int stride = 256 * 4;
    int len = 0;

    *out_buffer = stbi_write_png_to_mem(buffer, stride,
                                         width, height,
                                         4, &len);

    return len;
}

static void save_ram(void)
{
    const char* dir = get_configurated_dir(config_emulator.savefiles_dir_option, config_emulator.savefiles_path.c_str());
    core->SaveRam(dir);
}

static void load_ram(void)
{
    const char* dir = get_configurated_dir(config_emulator.savefiles_dir_option, config_emulator.savefiles_path.c_str());
    core->LoadRam(dir);
}

static void reset_buffers(void)
{
    for (int i = 0; i < (256 * 256 * 4); i++)
        emu_frame_buffer[i] = 0;

    for (int i = 0; i < GLYNX_AUDIO_BUFFER_SIZE; i++)
        audio_buffer[i] = 0;
}

static const char* get_configurated_dir(int location, const char* path)
{
    switch ((Directory_Location)location)
    {
        default:
        case Directory_Location_Default:
            return config_root_path;
        case Directory_Location_ROM:
            return NULL;
        case Directory_Location_Custom:
            return path;
    }
}

static void init_debug(void)
{
    emu_debug_scb_count = 0;

    for (int i = 0; i < 5; i++)
    {
        emu_debug_framebuffer[i] = new u8[256 * 256 * 4];
        memset(emu_debug_framebuffer[i], 0, 256 * 256 * 4);
    }

    for (int i = 0; i < DEBUG_MAX_SPRITES; i++)
    {
        emu_debug_sprite_buffers[i] = new u8[256 * 256 * 4];
        memset(emu_debug_sprite_buffers[i], 0, 256 * 256 * 4);
        emu_debug_sprite_widths[i] = 0;
        emu_debug_sprite_heights[i] = 0;
    }
}

static void destroy_debug(void) 
{
    for (int i = 0; i < 5; i++)
        SafeDeleteArray(emu_debug_framebuffer[i]);

    for (int i = 0; i < DEBUG_MAX_SPRITES; i++)
        SafeDeleteArray(emu_debug_sprite_buffers[i]);
}

static void update_debug(void)
{
    if (config_debug.show_frame_buffers)
        update_debug_framebuffers();

    bool accumulate = config_debug.show_scb_viewer && config_debug.scb_viewer_mode == 1;

    core->GetSuzy()->SetSCBAccumulationEnabled(accumulate);

    if (config_debug.show_scb_viewer)
    {
        if (config_debug.scb_viewer_mode == 1)
            update_debug_sprites_accumulated();
        else
            update_debug_sprites();
    }
}

static void update_debug_framebuffers(void)
{
    u16 vidbas = core->GetSuzy()->GetState()->VIDBAS.value;
    u16 dispadr = core->GetMikey()->GetState()->DISPADR.value;
    u16 collbas = core->GetSuzy()->GetState()->COLLBAS.value;
    u16 custom_addr = (u16)config_debug.frame_buffer_custom_address;
    u8* ram = core->GetMemory()->GetRAM();
    u32* palette = core->GetMikey()->GetLcdScreen()->GetRGBA8888Palette();
    if (!palette)
        return;

    int count = GLYNX_SCREEN_WIDTH * GLYNX_SCREEN_HEIGHT;

    u32* frame_buffer_vidbas = (u32*)emu_debug_framebuffer[0];
    u32* frame_buffer_dispadr = (u32*)emu_debug_framebuffer[1];
    u32* frame_buffer_collbas = (u32*)emu_debug_framebuffer[2];
    u32* frame_buffer_custom = (u32*)emu_debug_framebuffer[3];
    u32* frame_buffer_coll_overlay = (u32*)emu_debug_framebuffer[4];

    for (int i = 0; i < count; i++)
    {
        u16 src_vidbas = (u16)(vidbas + (i >> 1));
        u16 src_dispadr = (u16)(dispadr + (i >> 1));
        u16 src_collbas = (u16)(collbas + (i >> 1));
        u16 src_custom = (u16)(custom_addr + (i >> 1));

        int color_idx_vidbas = i & 1 ? (ram[src_vidbas] & 0x0F) : (ram[src_vidbas] >> 4);
        int color_idx_dispadr = i & 1 ? (ram[src_dispadr] & 0x0F) : (ram[src_dispadr] >> 4);
        int color_idx_collbas = i & 1 ? (ram[src_collbas] & 0x0F) : (ram[src_collbas] >> 4);
        int color_idx_custom = i & 1 ? (ram[src_custom] & 0x0F) : (ram[src_custom] >> 4);

        u16 green_vidbas = core->GetMikey()->GetState()->colors[color_idx_vidbas].green;
        u16 bluered_vidbas = core->GetMikey()->GetState()->colors[color_idx_vidbas].bluered;
        u16 green_dispadr = core->GetMikey()->GetState()->colors[color_idx_dispadr].green;
        u16 bluered_dispadr = core->GetMikey()->GetState()->colors[color_idx_dispadr].bluered;
        u16 green_custom = core->GetMikey()->GetState()->colors[color_idx_custom].green;
        u16 bluered_custom = core->GetMikey()->GetState()->colors[color_idx_custom].bluered;

        u16 palette_idx_vidbas = ((green_vidbas & 0x0F) << 8 | (bluered_vidbas & 0xFF)) & 0x0FFF;
        u16 palette_idx_dispadr = ((green_dispadr & 0x0F) << 8 | (bluered_dispadr & 0xFF)) & 0x0FFF;
        u16 palette_idx_custom = ((green_custom & 0x0F) << 8 | (bluered_custom & 0xFF)) & 0x0FFF;

        u32 final_color_vidbas = palette[palette_idx_vidbas];
        u32 final_color_dispadr = palette[palette_idx_dispadr];
        u32 final_color_collbas = emu_collision_palette[color_idx_collbas];
        u32 final_color_custom = palette[palette_idx_custom];

        u32 final_color_coll_overlay = (color_idx_collbas == 0) ? 0x00000000 : emu_collision_palette[color_idx_collbas];

        frame_buffer_vidbas[i] = final_color_vidbas;
        frame_buffer_dispadr[i] = final_color_dispadr;
        frame_buffer_collbas[i] = final_color_collbas;
        frame_buffer_coll_overlay[i] = final_color_coll_overlay;
        frame_buffer_custom[i] = final_color_custom;
    }
}

static void update_debug_sprites(void)
{
    u8* ram = core->GetMemory()->GetRAM();
    Suzy::Suzy_State* suzy_state = core->GetSuzy()->GetState();

    u16 scb_addr;
    if (config_debug.scb_viewer_auto)
    {
        if ((suzy_state->SCBNEXT.value & 0xFF00) != 0)
            scb_addr = suzy_state->SCBNEXT.value;
        else
            scb_addr = suzy_state->SCBADR.value;
    }
    else
    {
        scb_addr = (u16)config_debug.scb_viewer_address;
    }

    u8 pen_map[16];
    memcpy(pen_map, suzy_state->pen_map, 16);
    u16 running_sprhsiz = suzy_state->SPRHSIZ.value;
    u16 running_sprvsiz = suzy_state->SPRVSIZ.value;

    int count = 0;

    while ((scb_addr & 0xFF00) != 0 && count < DEBUG_MAX_SPRITES)
    {
        u16 tmpadr = scb_addr;

        u8 sprctl0 = ram[tmpadr++];
        u8 sprctl1 = ram[tmpadr++];
        u8 sprcoll = ram[tmpadr++];
        u16 scb_next = (u16)(ram[tmpadr] | (ram[(u16)(tmpadr + 1)] << 8));
        tmpadr += 2;

        GLYNX_Debug_SCB_Info& info = emu_debug_scb_info[count];
        info.scb_address = scb_addr;
        info.scb_next = scb_next;
        info.sprctl0 = sprctl0;
        info.sprctl1 = sprctl1;
        info.sprcoll = sprcoll;

        if (IS_SET_BIT(sprctl1, 2))
        {
            info.skipped = true;
            info.bpp = 0;
            info.h_flip = false;
            info.v_flip = false;
            info.type = 0;
            info.literal_only = false;
            info.reload_depth = 0;
            info.reload_palette = false;
            info.hpos = 0;
            info.vpos = 0;
            info.sprdline = 0;
            info.sprhsiz = 0;
            info.sprvsiz = 0;
            info.stretch = 0;
            info.tilt = 0;
            memset(info.pen_map, 0, 16);

            count++;
            scb_addr = scb_next;
            continue;
        }

        int bpp = ((sprctl0 >> 6) & 0x03) + 1;
        int reload_depth = (sprctl1 >> 4) & 0x03;
        bool reload_palette = IS_NOT_SET_BIT(sprctl1, 3);

        u16 sprdline = (u16)(ram[tmpadr] | (ram[(u16)(tmpadr + 1)] << 8));
        tmpadr += 2;
        s16 hpos = (s16)(u16)(ram[tmpadr] | (ram[(u16)(tmpadr + 1)] << 8));
        tmpadr += 2;
        s16 vpos = (s16)(u16)(ram[tmpadr] | (ram[(u16)(tmpadr + 1)] << 8));
        tmpadr += 2;

        u16 stretch_val = 0;
        u16 tilt_val = 0;

        if (reload_depth >= 1)
        {
            running_sprhsiz = (u16)(ram[tmpadr] | (ram[(u16)(tmpadr + 1)] << 8));
            tmpadr += 2;
            running_sprvsiz = (u16)(ram[tmpadr] | (ram[(u16)(tmpadr + 1)] << 8));
            tmpadr += 2;
        }
        if (reload_depth >= 2)
        {
            stretch_val = (u16)(ram[tmpadr] | (ram[(u16)(tmpadr + 1)] << 8));
            tmpadr += 2;
        }
        if (reload_depth >= 3)
        {
            tilt_val = (u16)(ram[tmpadr] | (ram[(u16)(tmpadr + 1)] << 8));
            tmpadr += 2;
        }

        if (reload_palette)
        {
            int colors = 1 << bpp;
            int bytes_to_read = colors >> 1;
            for (int i = 0; i < bytes_to_read; i++)
            {
                u8 byte = ram[tmpadr++];
                pen_map[(i << 1) + 0] = (byte >> 4) & 0x0F;
                pen_map[(i << 1) + 1] = (byte & 0x0F);
            }
        }

        info.skipped = false;
        info.bpp = bpp;
        info.h_flip = IS_SET_BIT(sprctl0, 5);
        info.v_flip = IS_SET_BIT(sprctl0, 4);
        info.type = (sprctl0 & 0x07);
        info.literal_only = IS_SET_BIT(sprctl1, 7);
        info.reload_depth = reload_depth;
        info.reload_palette = reload_palette;
        info.hpos = hpos;
        info.vpos = vpos;
        info.sprdline = sprdline;
        info.sprhsiz = running_sprhsiz;
        info.sprvsiz = running_sprvsiz;
        info.stretch = stretch_val;
        info.tilt = tilt_val;
        memcpy(info.pen_map, pen_map, 16);

        count++;
        scb_addr = scb_next;
    }

    render_debug_sprites(count);
}

static void update_debug_sprites_accumulated(void)
{
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    std::vector<Suzy::GLYNX_SCB_Info>* frame_list = core->GetSuzy()->GetFrameSCBList();

    int count = MIN((int)frame_list->size(), DEBUG_MAX_SPRITES);

    for (int s = 0; s < count; s++)
    {
        Suzy::GLYNX_SCB_Info& src = (*frame_list)[s];
        GLYNX_Debug_SCB_Info& info = emu_debug_scb_info[s];

        info.scb_address = src.scb_address;
        info.scb_next = src.scb_next;
        info.sprctl0 = src.sprctl0;
        info.sprctl1 = src.sprctl1;
        info.sprcoll = src.sprcoll;
        info.skipped = src.skipped;

        if (src.skipped)
        {
            info.bpp = 0;
            info.h_flip = false;
            info.v_flip = false;
            info.type = 0;
            info.literal_only = false;
            info.reload_depth = 0;
            info.reload_palette = false;
            info.hpos = 0;
            info.vpos = 0;
            info.sprdline = 0;
            info.sprhsiz = 0;
            info.sprvsiz = 0;
            info.stretch = 0;
            info.tilt = 0;
            memset(info.pen_map, 0, 16);
            continue;
        }

        info.bpp = ((src.sprctl0 >> 6) & 0x03) + 1;
        info.h_flip = IS_SET_BIT(src.sprctl0, 5);
        info.v_flip = IS_SET_BIT(src.sprctl0, 4);
        info.type = (src.sprctl0 & 0x07);
        info.literal_only = IS_SET_BIT(src.sprctl1, 7);
        info.reload_depth = (src.sprctl1 >> 4) & 0x03;
        info.reload_palette = IS_NOT_SET_BIT(src.sprctl1, 3);
        info.hpos = src.hpos;
        info.vpos = src.vpos;
        info.sprdline = src.sprdline;
        info.sprhsiz = src.sprhsiz;
        info.sprvsiz = src.sprvsiz;
        info.stretch = src.stretch;
        info.tilt = src.tilt;
        memcpy(info.pen_map, src.pen_map, 16);
    }

    render_debug_sprites(count);
#endif
}

struct SpriteShiftReg
{
    u8* ram;
    u16 address;
    u8 current;
    s32 bit;
};

struct SpriteQuadPos
{
    bool left;
    bool up;
};

static const int k_sprite_buf_w = 256;
static const int k_sprite_buf_h = 256;
static const u32 k_sprite_sreg_eof = 0xFFFFFFFFu;
static const u8 k_sprite_max_line_size = 200;
static const int k_sprite_max_lines = 128;
static const s16 k_sprite_max_pos = 1024;

static const int k_sprite_quad_seq[4][4] = {
    { 0, 2, 3, 1 },
    { 1, 0, 2, 3 },
    { 2, 3, 1, 0 },
    { 3, 1, 0, 2 }
};

static void shift_reg_reset(SpriteShiftReg* sr, u16 addr)
{
    sr->address = addr;
    sr->current = sr->ram[addr];
    sr->bit = 7;
}

static u32 shift_reg_get_bits(SpriteShiftReg* sr, int n, u16 stop_addr)
{
    if (sr->address >= stop_addr)
        return k_sprite_sreg_eof;

    int bits_in_current = sr->bit + 1;
    u16 bytes_remaining = (u16)((stop_addr - 1) - sr->address);
    int remaining = bits_in_current + (int)bytes_remaining * 8;

    if (n >= remaining)
        return k_sprite_sreg_eof;

    u32 value = 0;
    int need = n;

    while (need > 0)
    {
        if (sr->bit < 0)
        {
            sr->address++;
            sr->current = sr->ram[sr->address];
            sr->bit = 7;
        }

        value = (value << 1) | ((sr->current >> sr->bit) & 1);
        sr->bit--;
        need--;
    }

    return value;
}

static SpriteQuadPos get_sprite_quad_pos(int qi, int sq, int fl)
{
    int fq = k_sprite_quad_seq[sq][qi] ^ fl;
    SpriteQuadPos result;
    result.left = (fq & 1) != 0;
    result.up = (fq & 2) != 0;
    return result;
}

static void render_debug_sprites(int count)
{
    u8* ram = core->GetMemory()->GetRAM();
    u32* palette = core->GetMikey()->GetLcdScreen()->GetRGBA8888Palette();
    if (!palette)
        return;

    Suzy::Suzy_State* suzy_state = core->GetSuzy()->GetState();
    Mikey::Mikey_State* mikey_state = core->GetMikey()->GetState();

    u16 hsizoff = suzy_state->HSIZOFF.value;
    u16 vsizoff = suzy_state->VSIZOFF.value;
    bool vertical_stretch = suzy_state->sprsys_vstrech;

    for (int s = 0; s < count; s++)
    {
        GLYNX_Debug_SCB_Info& info = emu_debug_scb_info[s];
        u32* buf = (u32*)emu_debug_sprite_buffers[s];
        memset(buf, 0, k_sprite_buf_w * k_sprite_buf_h * 4);
        emu_debug_sprite_widths[s] = 0;
        emu_debug_sprite_heights[s] = 0;

        if (info.skipped)
            continue;

        int bpp = info.bpp;
        bool literal_only = info.literal_only;
        u16 sprdline = info.sprdline;
        u16 cur_sprhsiz = info.sprhsiz;
        u16 cur_sprvsiz = info.sprvsiz;
        u16 stretch_val = info.stretch;
        u16 tilt_val = info.tilt;
        int flip = (info.h_flip ? 1 : 0) | (info.v_flip ? 2 : 0);
        bool start_up = IS_SET_BIT(info.sprctl1, 1);
        bool start_left = IS_SET_BIT(info.sprctl1, 0);
        int start_quad = (start_left ? 1 : 0) | (start_up ? 2 : 0);

        if (ram[sprdline] > k_sprite_max_line_size)
            continue;
        if (info.hpos > k_sprite_max_pos || info.hpos < -k_sprite_max_pos ||
            info.vpos > k_sprite_max_pos || info.vpos < -k_sprite_max_pos)
            continue;

        // First pass: compute bounding box
        s32 min_x = 0x7FFF, max_x = -0x7FFF;
        s32 min_y = 0x7FFF, max_y = -0x7FFF;
        bool bbox_valid = false;
        bool bbox_done = false;

        SpriteShiftReg sr;
        sr.ram = ram;
        sr.address = 0;
        sr.current = 0;
        sr.bit = -1;
        u16 cur_sprdline_bb = sprdline;
        u16 cur_sprhsiz_bb = cur_sprhsiz;
        u16 cur_sprvsiz_bb = cur_sprvsiz;

        int quadrant = 0;
        SpriteQuadPos pos = get_sprite_quad_pos(quadrant, start_quad, flip);
        SpriteQuadPos start_pos = pos;
        s32 dx = pos.left ? -1 : +1;
        s32 dy = pos.up ? -1 : +1;
        s32 cur_y = 0;
        u16 vsizacum = vsizoff;
        u16 tiltacum = 0;
        s32 base_hpos_bb = 0;
        int safety = 0;

        while (safety++ < k_sprite_max_lines && !bbox_done)
        {
            u8 sprdoff = ram[cur_sprdline_bb];
            if (sprdoff > k_sprite_max_line_size)
                break;

            u16 next_ptr = (u16)(cur_sprdline_bb + (u16)sprdoff);
            u16 data_begin = (u16)(cur_sprdline_bb + 1);
            u16 data_end = next_ptr;

            vsizacum = vsizacum + cur_sprvsiz_bb;
            s16 pixel_height = MIN((s16)(vsizacum >> 8), (s16)k_sprite_buf_h);
            vsizacum &= 0x00FF;

            for (int row = 0; row < pixel_height && !bbox_done; row++)
            {
                s32 start_x = base_hpos_bb;
                if (pos.left != start_pos.left)
                    start_x += dx;

                shift_reg_reset(&sr, data_begin);
                u32 h_accum = hsizoff;
                s32 x = start_x;

                if (literal_only)
                {
                    while (sr.address < data_end)
                    {
                        u32 pi = shift_reg_get_bits(&sr, bpp, data_end);
                        if (pi == k_sprite_sreg_eof)
                            break;

                        h_accum += (u32)cur_sprhsiz_bb;
                        s32 pc = (s32)(h_accum >> 8);
                        h_accum &= 0xFF;

                        for (s32 p = 0; p < pc; p++)
                        {
                            min_x = MIN(x, min_x);
                            max_x = MAX(x, max_x);
                            x += dx;
                        }
                    }
                }
                else
                {
                    while (sr.address < data_end && !bbox_done)
                    {
                        u32 header = shift_reg_get_bits(&sr, 5, data_end);
                        if (header == 0 || header == k_sprite_sreg_eof)
                            break;

                        u32 is_lit = header >> 4;
                        u32 cnt = (header & 0x0F) + 1;

                        if (is_lit)
                        {
                            while (cnt-- && !bbox_done)
                            {
                                u32 pi = shift_reg_get_bits(&sr, bpp, data_end);

                                if (pi == k_sprite_sreg_eof)
                                {
                                    bbox_done = true;
                                    break;
                                }

                                h_accum += (u32)cur_sprhsiz_bb;
                                s32 pc = (s32)(h_accum >> 8);
                                h_accum &= 0xFF;

                                for (s32 p = 0; p < pc; p++)
                                {
                                    min_x = MIN(x, min_x);
                                    max_x = MAX(x, max_x);
                                    x += dx;
                                }
                            }
                        }
                        else
                        {
                            u32 pi = shift_reg_get_bits(&sr, bpp, data_end);
                            if (pi == k_sprite_sreg_eof)
                            {
                                bbox_done = true;
                                break;
                            }

                            while (cnt--)
                            {
                                h_accum += (u32)cur_sprhsiz_bb;
                                s32 pc = (s32)(h_accum >> 8);
                                h_accum &= 0xFF;

                                for (s32 p = 0; p < pc; p++)
                                {
                                    min_x = MIN(x, min_x);
                                    max_x = MAX(x, max_x);
                                    x += dx;
                                }
                            }
                        }
                    }
                }

                if (!bbox_done)
                {
                    min_y = MIN(cur_y, min_y);
                    max_y = MAX(cur_y, max_y);
                    bbox_valid = true;

                    if ((max_x - min_x) >= k_sprite_buf_w && (max_y - min_y) >= k_sprite_buf_h)
                    {
                        bbox_done = true;
                        break;
                    }

                    cur_y += dy;
                    tiltacum = (u16)(tiltacum + tilt_val);
                    base_hpos_bb += (s16)tiltacum >> 8;
                    tiltacum &= 0x00FF;
                    cur_sprhsiz_bb += stretch_val;
                }
            }

            if (!bbox_done)
            {
                if (vertical_stretch)
                    cur_sprvsiz_bb += (s16)stretch_val * (s16)pixel_height;

                if (sprdoff == 0)
                    break;
                else if (sprdoff == 1)
                {
                    quadrant = (quadrant + 1) & 3;
                    pos = get_sprite_quad_pos(quadrant, start_quad, flip);
                    dx = pos.left ? -1 : +1;
                    dy = pos.up ? -1 : +1;
                    cur_y = 0;
                    vsizacum = vsizoff;
                    if (pos.up != start_pos.up)
                        cur_y += dy;
                }
                cur_sprdline_bb = next_ptr;
            }
        }

        if (!bbox_valid || min_x > max_x || min_y > max_y)
            continue;

        int spr_w = MIN((int)(max_x - min_x + 1), k_sprite_buf_w);
        int spr_h = MIN((int)(max_y - min_y + 1), k_sprite_buf_h);

        if (spr_w <= 0 || spr_h <= 0)
            continue;

        emu_debug_sprite_widths[s] = spr_w;
        emu_debug_sprite_heights[s] = spr_h;

        s32 ox = -min_x;
        s32 oy = -min_y;

        // Second pass: render pixels
        bool render_done = false;

        sr.ram = ram;
        sr.address = 0;
        sr.current = 0;
        sr.bit = -1;
        u16 cur_sprdline_r = sprdline;
        u16 cur_sprhsiz_r = cur_sprhsiz;
        u16 cur_sprvsiz_r = cur_sprvsiz;

        quadrant = 0;
        pos = get_sprite_quad_pos(quadrant, start_quad, flip);
        start_pos = pos;
        dx = pos.left ? -1 : +1;
        dy = pos.up ? -1 : +1;
        cur_y = 0;
        vsizacum = vsizoff;
        tiltacum = 0;
        s32 base_hpos_r = 0;
        safety = 0;

        while (safety++ < k_sprite_max_lines && !render_done)
        {
            u8 sprdoff = ram[cur_sprdline_r];
            if (sprdoff > k_sprite_max_line_size)
                break;
            u16 next_ptr = (u16)(cur_sprdline_r + (u16)sprdoff);
            u16 data_begin = (u16)(cur_sprdline_r + 1);
            u16 data_end = next_ptr;

            vsizacum = vsizacum + cur_sprvsiz_r;
            s16 pixel_height = MIN((s16)(vsizacum >> 8), (s16)k_sprite_buf_h);
            vsizacum &= 0x00FF;

            for (int row = 0; row < pixel_height && !render_done; row++)
            {
                s32 start_x = base_hpos_r;
                if (pos.left != start_pos.left)
                    start_x += dx;

                shift_reg_reset(&sr, data_begin);
                u32 h_accum = hsizoff;
                s32 x = start_x;

                if (literal_only)
                {
                    while (sr.address < data_end)
                    {
                        u32 pi = shift_reg_get_bits(&sr, bpp, data_end);
                        if (pi == k_sprite_sreg_eof)
                            break;

                        u8 pen = info.pen_map[pi & 0x0F];
                        h_accum += (u32)cur_sprhsiz_r;
                        s32 pc = (s32)(h_accum >> 8);
                        h_accum &= 0xFF;

                        for (s32 p = 0; p < pc; p++)
                        {
                            s32 px = x + ox;
                            s32 py = cur_y + oy;

                            if ((u32)px < (u32)k_sprite_buf_w && (u32)py < (u32)k_sprite_buf_h)
                            {
                                u16 gr = mikey_state->colors[pen].green;
                                u16 br = mikey_state->colors[pen].bluered;
                                u16 pidx = ((gr & 0x0F) << 8 | (br & 0xFF)) & 0x0FFF;
                                buf[py * k_sprite_buf_w + px] = palette[pidx] | 0xFF000000u;
                            }
                            x += dx;
                        }
                    }
                }
                else
                {
                    while (sr.address < data_end && !render_done)
                    {
                        u32 header = shift_reg_get_bits(&sr, 5, data_end);
                        if (header == 0 || header == k_sprite_sreg_eof)
                            break;

                        u32 is_lit = header >> 4;
                        u32 cnt = (header & 0x0F) + 1;

                        if (is_lit)
                        {
                            while (cnt-- && !render_done)
                            {
                                u32 pi = shift_reg_get_bits(&sr, bpp, data_end);
                                if (pi == k_sprite_sreg_eof)
                                {
                                    render_done = true;
                                    break;
                                }

                                u8 pen = info.pen_map[pi & 0x0F];
                                h_accum += (u32)cur_sprhsiz_r;
                                s32 pc = (s32)(h_accum >> 8);
                                h_accum &= 0xFF;

                                for (s32 p = 0; p < pc; p++)
                                {
                                    s32 px = x + ox;
                                    s32 py = cur_y + oy;

                                    if ((u32)px < (u32)k_sprite_buf_w && (u32)py < (u32)k_sprite_buf_h)
                                    {
                                        u16 gr = mikey_state->colors[pen].green;
                                        u16 br = mikey_state->colors[pen].bluered;
                                        u16 pidx = ((gr & 0x0F) << 8 | (br & 0xFF)) & 0x0FFF;
                                        buf[py * k_sprite_buf_w + px] = palette[pidx] | 0xFF000000u;
                                    }
                                    x += dx;
                                }
                            }
                        }
                        else
                        {
                            u32 pi = shift_reg_get_bits(&sr, bpp, data_end);
                            if (pi == k_sprite_sreg_eof)
                            {
                                render_done = true;
                                break;
                            }

                            u8 pen = info.pen_map[pi & 0x0F];

                            while (cnt--)
                            {
                                h_accum += (u32)cur_sprhsiz_r;
                                s32 pc = (s32)(h_accum >> 8);
                                h_accum &= 0xFF;

                                for (s32 p = 0; p < pc; p++)
                                {
                                    s32 px = x + ox;
                                    s32 py = cur_y + oy;

                                    if ((u32)px < (u32)k_sprite_buf_w && (u32)py < (u32)k_sprite_buf_h)
                                    {
                                        u16 gr = mikey_state->colors[pen].green;
                                        u16 br = mikey_state->colors[pen].bluered;
                                        u16 pidx = ((gr & 0x0F) << 8 | (br & 0xFF)) & 0x0FFF;
                                        buf[py * k_sprite_buf_w + px] = palette[pidx] | 0xFF000000u;
                                    }
                                    x += dx;
                                }
                            }
                        }
                    }
                }

                if (!render_done)
                {
                    cur_y += dy;
                    tiltacum = (u16)(tiltacum + tilt_val);
                    base_hpos_r += (s16)tiltacum >> 8;
                    tiltacum &= 0x00FF;
                    cur_sprhsiz_r += stretch_val;
                }
            }

            if (!render_done)
            {
                if (vertical_stretch)
                    cur_sprvsiz_r += (s16)stretch_val * (s16)pixel_height;

                if (sprdoff == 0)
                    break;
                else if (sprdoff == 1)
                {
                    quadrant = (quadrant + 1) & 3;
                    pos = get_sprite_quad_pos(quadrant, start_quad, flip);
                    dx = pos.left ? -1 : +1;
                    dy = pos.up ? -1 : +1;
                    cur_y = 0;
                    vsizacum = vsizoff;
                    if (pos.up != start_pos.up)
                        cur_y += dy;
                }
                cur_sprdline_r = next_ptr;
            }
        }
    }

    emu_debug_scb_count = count;

    // Clear remaining slots
    for (int idx = count; idx < DEBUG_MAX_SPRITES; idx++)
    {
        memset(emu_debug_sprite_buffers[idx], 0, k_sprite_buf_w * k_sprite_buf_h * 4);
        emu_debug_sprite_widths[idx] = 0;
        emu_debug_sprite_heights[idx] = 0;
    }
}

void emu_start_vgm_recording(const char* file_path)
{
    if (!core->GetMedia()->IsReady())
        return;

    if (core->GetAudio()->IsVgmRecording())
    {
        emu_stop_vgm_recording();
    }

    // Atari Lynx Mikey chip clock rate is 16 MHz
    const int clock_rate = 16000000;

    if (core->GetAudio()->StartVgmRecording(file_path, clock_rate))
    {
        Log("VGM recording started: %s", file_path);
    }
}

void emu_stop_vgm_recording(void)
{
    if (core->GetAudio()->IsVgmRecording())
    {
        core->GetAudio()->StopVgmRecording();
        Log("VGM recording stopped");
    }
}

bool emu_is_vgm_recording(void)
{
    return core->GetAudio()->IsVgmRecording();
}

void emu_mcp_set_transport(int mode, int tcp_port)
{
    if (mcp_manager)
        mcp_manager->SetTransportMode((McpTransportMode)mode, tcp_port);
}

void emu_mcp_start(void)
{
    if (mcp_manager)
        mcp_manager->Start();
}

void emu_mcp_stop(void)
{
    if (mcp_manager)
        mcp_manager->Stop();
}

bool emu_mcp_is_running(void)
{
    return mcp_manager && mcp_manager->IsRunning();
}

int emu_mcp_get_transport_mode(void)
{
    return mcp_manager ? mcp_manager->GetTransportMode() : -1;
}

void emu_mcp_pump_commands(void)
{
    if (mcp_manager && mcp_manager->IsRunning())
        mcp_manager->PumpCommands(core);
}
