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

#define STB_IMAGE_WRITE_IMPLEMENTATION
#if defined(_WIN32)
#define STBIW_WINDOWS_UTF8
#endif
#include "stb_image_write.h"

static GearlynxCore* core;
static SoundQueue* sound_queue;
static s16* audio_buffer;
static bool audio_enabled;

static void save_ram(void);
static void load_ram(void);
static void reset_buffers(void);
static const char* get_configurated_dir(int option, const char* path);
static void init_debug(void);
static void destroy_debug(void);
static void update_debug(void);
static void update_debug_framebuffers(void);
static void update_debug_sprites(void);

bool emu_init(void)
{
    emu_frame_buffer = new u8[GLYNX_SCREEN_WIDTH * GLYNX_SCREEN_HEIGHT * 4];
    audio_buffer = new s16[GLYNX_AUDIO_BUFFER_SIZE];

    init_debug();
    reset_buffers();

    core = new GearlynxCore();
    core->Init();

    sound_queue = new SoundQueue();
    if (!sound_queue->Start(GLYNX_AUDIO_SAMPLE_RATE, 2, GLYNX_AUDIO_BUFFER_SIZE, GLYNX_AUDIO_BUFFER_COUNT))
        return false;

    for (int i = 0; i < 5; i++)
        InitPointer(emu_savestates_screenshots[i].data);

    audio_enabled = true;
    emu_audio_sync = true;
    emu_debug_disable_breakpoints = false;
    emu_debug_command = Debug_Command_None;
    emu_debug_pc_changed = false;
    for (int i = 0; i < 8; i++)
        emu_debug_irq_breakpoints[i] = false;

    return true;
}

void emu_destroy(void)
{
    save_ram();
    SafeDeleteArray(audio_buffer);
    SafeDelete(sound_queue);
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

    save_ram();
    if (!core->LoadROM(file_path))
        return false;

    load_ram();

    update_savestates_data();

    return true;
}

void emu_update(void)
{
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

        debug_run.stop_on_irq = 0;
        for (int i = 0; i < 8; i++)
        {
            if (emu_debug_irq_breakpoints[i])
                debug_run.stop_on_irq = SET_BIT(debug_run.stop_on_irq, i);
        }

        if (emu_debug_command != Debug_Command_None)
            breakpoint_hit = core->RunToVBlank(emu_frame_buffer, audio_buffer, &sampleCount, &debug_run);

        if (breakpoint_hit || emu_debug_command == Debug_Command_StepFrame || emu_debug_command == Debug_Command_Step)
                emu_debug_pc_changed = true;

        if (breakpoint_hit)
            emu_debug_command = Debug_Command_None;

        if (emu_debug_command != Debug_Command_Continue)
            emu_debug_command = Debug_Command_None;

        update_debug();
    }
    else
        core->RunToVBlank(emu_frame_buffer, audio_buffer, &sampleCount);

    if ((sampleCount > 0) && !core->IsPaused())
    {
        sound_queue->Write(audio_buffer, sampleCount, emu_audio_sync);
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

    save_ram();
    core->ResetROM(false);
    load_ram();
}

void emu_force_rotation(int rotation)
{
    core->GetMedia()->ForceRotation((GLYNX_Rotation)rotation);
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
    sound_queue->Stop();
    sound_queue->Start(GLYNX_AUDIO_SAMPLE_RATE, 2, GLYNX_AUDIO_BUFFER_SIZE, GLYNX_AUDIO_BUFFER_COUNT);
}

bool emu_is_audio_enabled(void)
{
    return audio_enabled;
}

bool emu_is_audio_open(void)
{
    return sound_queue->IsOpen();
}

void emu_save_ram(const char* file_path)
{
    // TODO Implement save ram to file
    // if (!emu_is_empty())
    //     core->SaveRam(file_path, true);
    UNUSED(file_path);
}

void emu_load_ram(const char* file_path)
{
    // TODO Implement load ram from file
    // if (!emu_is_empty())
    // {
    //     save_ram();
    //     core->ResetROM(&config);
    //     core->LoadRam(file_path, true);
    // }
    UNUSED(file_path);
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
        int rom_banks = 0;// TODO: media->GetROMBankCount();

        snprintf(info, buffer_size, "File Name: %s\nCRC: %08X\nROM Size: %d bytes, %d KB\nROM Banks: %d\nScreen Resolution: %dx%d", filename, crc, rom_size, rom_size / 1024, rom_banks, runtime.screen_width, runtime.screen_height);
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

static void save_ram(void)
{
    // TOOD
    // if ((emu_savefiles_dir_option == 0) && (strcmp(emu_savefiles_path, "")))
    //     core->SaveRam(emu_savefiles_path);
    // else
    //     core->SaveRam();
}

static void load_ram(void)
{
    // TODO
    // if ((emu_savefiles_dir_option == 0) && (strcmp(emu_savefiles_path, "")))
    //     core->LoadRam(emu_savefiles_path);
    // else
    //     core->LoadRam();
}

static void reset_buffers(void)
{
    for (int i = 0; i < (GLYNX_SCREEN_WIDTH * GLYNX_SCREEN_HEIGHT * 4); i++)
        emu_frame_buffer[i] = 0;

    // emu_debug_background_buffer_width = 32;
    // emu_debug_background_buffer_height = 32;

    //  for (int i = 0; i < 1024 * 512 * 4; i++)
    //     emu_frame_buffer[i] = 0;

    // for (int i = 0; i < GLYNX_AUDIO_BUFFER_SIZE; i++)
    //     audio_buffer[i] = 0;

    // for (int i = 0; i < GLYNX_SCREEN_WIDTH * GLYNX_SCREEN_HEIGHT * 4; i++)
    //     emu_debug_background_buffer[i] = 0;

    // for (int i = 0; i < 64; i++)
    // {
    //     for (int j = 0; j < HUC6270_MAX_SPRITE_WIDTH * HUC6270_MAX_SPRITE_HEIGHT * 4; j++)
    //         emu_debug_sprite_buffers[i][j] = 0;

    //     emu_debug_sprite_widths[i] = 16;
    //     emu_debug_sprite_heights[i] = 16;
    // }
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
    for (int i = 0; i < 2; i++)
    {
        emu_debug_framebuffer[i] = new u8[256 * 128 * 4];
        memset(emu_debug_framebuffer[i], 0, 256 * 128 * 4);
    }
    // emu_debug_background_buffer = new u8[GLYNX_SCREEN_WIDTH * GLYNX_SCREEN_HEIGHT * 4];
    // for (int i = 0; i < GLYNX_SCREEN_WIDTH * GLYNX_SCREEN_HEIGHT * 4; i++)
    //     emu_debug_background_buffer[i] = 0;

    // for (int i = 0; i < 64; i++)
    // {
    //     emu_debug_sprite_buffers[i] = new u8[HUC6270_MAX_SPRITE_WIDTH * HUC6270_MAX_SPRITE_HEIGHT * 4];
    //     for (int j = 0; j < HUC6270_MAX_SPRITE_WIDTH * HUC6270_MAX_SPRITE_HEIGHT * 4; j++)
    //         emu_debug_sprite_buffers[i][j] = 0;
    // }
}

static void destroy_debug(void) 
{
    for (int i = 0; i < 2; i++)
        SafeDeleteArray(emu_debug_framebuffer[i]);
    // SafeDeleteArray(emu_debug_background_buffer);

    // for (int i = 0; i < 64; i++)
    //     SafeDeleteArray(emu_debug_sprite_buffers[i]);
}

static void update_debug(void)
{
    update_debug_framebuffers();
    update_debug_sprites();
}

static void update_debug_framebuffers(void)
{
    u16 vidbas = core->GetSuzy()->GetState()->VIDBAS.value;
    u16 dispadr = core->GetMikey()->GetState()->DISPADR.value;
    u8* ram = core->GetMemory()->GetRAM();
    u32* palette = core->GetMikey()->GetRGBA8888Palette();

    int count = GLYNX_SCREEN_WIDTH * GLYNX_SCREEN_HEIGHT;
    
    u32* frame_buffer_vidbas = (u32*)emu_debug_framebuffer[0];
    u32* frame_buffer_dispadr = (u32*)emu_debug_framebuffer[1];

    for (int i = 0; i < count; i++)
    {
        u16 src_vidbas = vidbas + (i / 2);
        u16 src_dispadr = dispadr + (i / 2);

        int color_idx_vidbas = i & 1 ? (ram[src_vidbas] & 0x0F) : (ram[src_vidbas] >> 4);
        int color_idx_dispadr = i & 1 ? (ram[src_dispadr] & 0x0F) : (ram[src_dispadr] >> 4);

        u16 green_vidbas = core->GetMikey()->GetState()->colors[color_idx_vidbas].green;
        u16 bluered_vidbas = core->GetMikey()->GetState()->colors[color_idx_vidbas].bluered;
        u16 green_dispadr = core->GetMikey()->GetState()->colors[color_idx_dispadr].green;
        u16 bluered_dispadr = core->GetMikey()->GetState()->colors[color_idx_dispadr].bluered;

        u32 final_color_vidbas = palette[green_vidbas << 8 | bluered_vidbas];
        u32 final_color_dispadr = palette[green_dispadr << 8 | bluered_dispadr];

        frame_buffer_vidbas[i] = final_color_vidbas;
        frame_buffer_dispadr[i] = final_color_dispadr;
    }
}

static void update_debug_sprites(void)
{
    
}
