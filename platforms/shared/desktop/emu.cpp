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

#include "../../../src/gearlynx.h"
#include "sound_queue.h"
#include "config.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#if defined(_WIN32)
#define STBIW_WINDOWS_UTF8
#endif
#include "stb/stb_image_write.h"

static GearlynxCore* core;
static SoundQueue* sound_queue;
static s16* audio_buffer;
static bool audio_enabled;

static void save_ram(void);
static void load_ram(void);
static void reset_buffers(void);
static void init_debug(void);
static void destroy_debug(void);
static void update_debug(void);
static void update_debug_background(void);
static void update_debug_sprites(void);

void emu_init(void)
{
    emu_frame_buffer = new u8[1024 * 512 * 4];
    audio_buffer = new s16[GLYNX_AUDIO_BUFFER_SIZE];

    init_debug();
    reset_buffers();

    core = new GearlynxCore();
    core->Init();

    sound_queue = new SoundQueue();
    sound_queue->Start(GLYNX_AUDIO_SAMPLE_RATE, 2, GLYNX_AUDIO_BUFFER_SIZE, GLYNX_AUDIO_BUFFER_COUNT);

    for (int i = 0; i < 5; i++)
        InitPointer(emu_savestates_screenshots[i].data);

    audio_enabled = true;
    emu_audio_sync = true;
    emu_debug_disable_breakpoints = false;
    emu_debug_irq_breakpoints = false;
    emu_debug_command = Debug_Command_None;
    emu_debug_pc_changed = false;
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

void emu_load_rom(const char* file_path)
{
    emu_debug_command = Debug_Command_None;
    reset_buffers();

    save_ram();
    core->LoadROM(file_path);
    load_ram();

    update_savestates_data();
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
        debug_run.stop_on_irq = emu_debug_irq_breakpoints;

        if (emu_debug_command != Debug_Command_None)
            breakpoint_hit = core->RunToVBlank(emu_frame_buffer, audio_buffer, &sampleCount, &debug_run);

        if (breakpoint_hit || emu_debug_command == Debug_Command_StepFrame || emu_debug_command == Debug_Command_Step)
                emu_debug_pc_changed = true;

        if (breakpoint_hit)
            emu_debug_command = Debug_Command_None;

        if (emu_debug_command != Debug_Command_Continue)
            emu_debug_command = Debug_Command_None;
    }
    else
        core->RunToVBlank(emu_frame_buffer, audio_buffer, &sampleCount);

    if ((sampleCount > 0) && !core->IsPaused())
    {
        sound_queue->Write(audio_buffer, sampleCount, emu_audio_sync);
    }

    update_debug();
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
    return !core->GetCartridge()->IsReady();
}

void emu_reset(void)
{
    emu_debug_command = Debug_Command_None;
    reset_buffers();

    save_ram();
    core->ResetROM(false);
    load_ram();
}

void emu_audio_mute(bool mute)
{
    audio_enabled = !mute;
    core->GetAudio()->Mute(mute);
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
        switch ((Directory_Location)config_emulator.savestates_dir_option)
        {
            default:
            case Directory_Location_Default:
            {
                core->SaveState(config_root_path, index, true);
                break;
            }
            case Directory_Location_ROM:
            {
                core->SaveState(NULL, index, true);
                break;
            }
            case Directory_Location_Custom:
            {
                core->SaveState(config_emulator.savestates_path.c_str(), index, true);
                break;
            }
        }

        update_savestates_data();
    }
}

void emu_load_state_slot(int index)
{
    if (!emu_is_empty())
    {
        switch ((Directory_Location)config_emulator.savestates_dir_option)
        {
            default:
            case Directory_Location_Default:
            {
                core->LoadState(config_root_path, index);
                break;
            }
            case Directory_Location_ROM:
            {
                core->LoadState(NULL, index);
                break;
            }
            case Directory_Location_Custom:
            {
                core->LoadState(config_emulator.savestates_path.c_str(), index);
                break;
            }
        }
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

        const char* dir = NULL;

        switch ((Directory_Location)config_emulator.savestates_dir_option)
        {
            default:
            case Directory_Location_Default:
            {
                dir = config_root_path;
                break;
            }
            case Directory_Location_ROM:
            {
                dir = NULL;
                break;
            }
            case Directory_Location_Custom:
            {
                dir = config_emulator.savestates_path.c_str();
                break;
            }
        }

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
        Cartridge* cart = core->GetCartridge();
        GLYNX_Runtime_Info runtime;
        core->GetRuntimeInfo(runtime);

        const char* filename = cart->GetFileName();
        u32 crc = cart->GetCRC();
        int rom_size = cart->GetROMSize();
        int rom_banks = cart->GetROMBankCount();

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
    // M6502* processor = emu_get_core()->GetM6502();
    // M6502::M6502_State* proc_state = processor->GetState();
    // Memory* memory = emu_get_core()->GetMemory();
    // u16 pc = proc_state->PC->GetValue();
    // Memory::GLYNX_Disassembler_Record* record = memory->GetDisassemblerRecord(proc_state->PC->GetValue());

    // if (IsValidPointer(record) && record->subroutine)
    // {
    //     u16 return_address = pc + record->size;
    //     processor->AddRunToBreakpoint(return_address);
    //     emu_debug_command = Debug_Command_Continue;
    // }
    // else
    //     emu_debug_command = Debug_Command_Step;

    // core->Pause(false);
}

void emu_debug_step_into(void)
{
    core->Pause(false);
    emu_debug_command = Debug_Command_Step;
}

void emu_debug_step_out(void)
{
    // M6502* processor = emu_get_core()->GetM6502();
    // std::stack<M6502::GLYNX_CallStackEntry>* call_stack = processor->GetDisassemblerCallStack();

    // if (call_stack->size() > 0)
    // {
    //     M6502::GLYNX_CallStackEntry entry = call_stack->top();
    //     u16 return_address = entry.back;
    //     processor->AddRunToBreakpoint(return_address);
    //     emu_debug_command = Debug_Command_Continue;
    // }
    // else
    //     emu_debug_command = Debug_Command_Step;

    // core->Pause(false);
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
    if (!core->GetCartridge()->IsReady())
        return;

    GLYNX_Runtime_Info runtime;
    emu_get_runtime(runtime);

    stbi_write_png(file_path, runtime.screen_width, runtime.screen_height, 4, emu_frame_buffer, runtime.screen_width * 4);

    Log("Screenshot saved to %s", file_path);
}

static void save_ram(void)
{
    // if ((emu_savefiles_dir_option == 0) && (strcmp(emu_savefiles_path, "")))
    //     core->SaveRam(emu_savefiles_path);
    // else
    //     core->SaveRam();
}

static void load_ram(void)
{
    // if ((emu_savefiles_dir_option == 0) && (strcmp(emu_savefiles_path, "")))
    //     core->LoadRam(emu_savefiles_path);
    // else
    //     core->LoadRam();
}

static void reset_buffers(void)
{
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

static void init_debug(void)
{
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
    // SafeDeleteArray(emu_debug_background_buffer);

    // for (int i = 0; i < 64; i++)
    //     SafeDeleteArray(emu_debug_sprite_buffers[i]);
}

static void update_debug(void)
{
    update_debug_background();
    update_debug_sprites();
}

static void update_debug_background(void)
{
    
}

static void update_debug_sprites(void)
{
    
}
