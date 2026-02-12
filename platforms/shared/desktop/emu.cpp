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

        if (emu_debug_command != Debug_Command_None)
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
    sound_queue_start(GLYNX_AUDIO_SAMPLE_RATE, 2, GLYNX_AUDIO_BUFFER_SIZE, GLYNX_AUDIO_BUFFER_COUNT);
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
            case GLYNX_ROTATION_LEFT: rotation_str = "Left"; break;
            case GLYNX_ROTATION_RIGHT: rotation_str = "Right"; break;
            default: rotation_str = "None"; break;
        }

        const char* eeprom_str = "None";
        int eeprom_base = eeprom & 0x0F;
        switch (eeprom_base)
        {
            case GLYNX_EEPROM_93C46: eeprom_str = "93C46"; break;
            case GLYNX_EEPROM_93C56: eeprom_str = "93C56"; break;
            case GLYNX_EEPROM_93C66: eeprom_str = "93C66"; break;
            case GLYNX_EEPROM_93C76: eeprom_str = "93C76"; break;
            case GLYNX_EEPROM_93C86: eeprom_str = "93C86"; break;
            default: eeprom_str = "None"; break;
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
    for (int i = 0; i < 5; i++)
    {
        emu_debug_framebuffer[i] = new u8[256 * 256 * 4];
        memset(emu_debug_framebuffer[i], 0, 256 * 256 * 4);
    }
}

static void destroy_debug(void) 
{
    for (int i = 0; i < 5; i++)
        SafeDeleteArray(emu_debug_framebuffer[i]);
}

static void update_debug(void)
{
    if (config_debug.show_frame_buffers)
        update_debug_framebuffers();
    if (config_debug.show_frame_buffers)
        update_debug_sprites();
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
