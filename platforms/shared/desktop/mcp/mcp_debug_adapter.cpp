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

#include "mcp_debug_adapter.h"
#include "log.h"
#include "../utils.h"
#include "../emu.h"
#include "../gui.h"
#include "../gui_actions.h"
#include "../gui_debug_disassembler.h"
#include "../gui_debug_memory.h"
#include "../gui_debug_memeditor.h"
#include "../gui_debug_rewind.h"
#include "../config.h"
#include "../rewind.h"
#include "mikey_defines.h"
#include <cstring>
#include <sstream>
#include <iomanip>
#include <vector>
#include <algorithm>

struct DisassemblerBookmark
{
    u16 address;
    char name[32];
};

static bool NormalizeMemoryAreaAddress(const MemoryAreaInfo& info, u32 address, u32* offset)
{
    if (!IsValidPointer(offset) || !IsValidPointer(info.data) || info.size == 0)
        return false;

    u64 display_start = info.cpu_offset;
    u64 display_end = display_start + info.size;

    if ((u64)address >= display_start && (u64)address < display_end)
    {
        *offset = address - info.cpu_offset;
        return true;
    }

    if (address < info.size)
    {
        *offset = address;
        return true;
    }

    return false;
}

static bool MemoryAreaContainsDisplayAddress(const MemoryAreaInfo& info, u32 address)
{
    if (!IsValidPointer(info.data) || info.size == 0)
        return false;

    u64 display_start = info.cpu_offset;
    u64 display_end = display_start + info.size;

    return (u64)address >= display_start && (u64)address < display_end;
}

void DebugAdapter::Pause()
{
    emu_debug_break();
}

void DebugAdapter::Resume()
{
    emu_debug_continue();
}

void DebugAdapter::StepInto()
{
    emu_debug_step_into();
}

void DebugAdapter::StepOver()
{
    emu_debug_step_over();
}

void DebugAdapter::StepOut()
{
    emu_debug_step_out();
}

void DebugAdapter::StepFrame()
{
    emu_debug_step_frame();
}

void DebugAdapter::Reset()
{
    gui_action_reset();
}

json DebugAdapter::GetDebugStatus()
{
    json result;

    if (!m_core)
    {
        result["error"] = "Core not initialized";
        return result;
    }

    bool is_paused = emu_is_debug_idle();

    result["paused"] = is_paused;

    if (is_paused)
    {
        M6502* cpu = m_core->GetM6502();
        u16 pc = cpu->GetState()->PC.GetValue();

        bool at_breakpoint = cpu->BreakpointHit();

        result["at_breakpoint"] = at_breakpoint;

        std::ostringstream pc_ss;
        pc_ss << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << pc;
        result["pc"] = pc_ss.str();
    }
    else
    {
        result["at_breakpoint"] = false;
    }

    return result;
}

void DebugAdapter::SetBreakpoint(u16 address, bool read, bool write, bool execute)
{
    M6502* cpu = m_core->GetM6502();

    if (execute && !read && !write)
    {
        cpu->AddBreakpoint(address);
    }
    else
    {
        char buffer[16];
        snprintf(buffer, sizeof(buffer), "%04X", address);
        cpu->AddBreakpoint(buffer, read, write, execute);
    }
}

void DebugAdapter::SetBreakpointRange(u16 start_address, u16 end_address, bool read, bool write, bool execute)
{
    M6502* cpu = m_core->GetM6502();

    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%04X-%04X", start_address, end_address);

    cpu->AddBreakpoint(buffer, read, write, execute);
}

void DebugAdapter::ClearBreakpointByAddress(u16 address, u16 end_address)
{
    M6502* cpu = m_core->GetM6502();

    std::vector<M6502::GLYNX_Breakpoint>* breakpoints = cpu->GetBreakpoints();

    for (int i = (int)breakpoints->size() - 1; i >= 0; i--)
    {
        M6502::GLYNX_Breakpoint& bp = (*breakpoints)[i];

        if (end_address > 0 && end_address >= address)
        {
            if (bp.range && bp.address1 == address && bp.address2 == end_address)
                breakpoints->erase(breakpoints->begin() + i);
        }
        else
        {
            if (!bp.range && bp.address1 == address)
                breakpoints->erase(breakpoints->begin() + i);
        }
    }
}

std::vector<BreakpointInfo> DebugAdapter::ListBreakpoints()
{
    std::vector<BreakpointInfo> result;
    M6502* cpu = m_core->GetM6502();
    std::vector<M6502::GLYNX_Breakpoint>* breakpoints = cpu->GetBreakpoints();

    for (const M6502::GLYNX_Breakpoint& brk : *breakpoints)
    {
        BreakpointInfo info;
        info.enabled = brk.enabled;
        info.address1 = brk.address1;
        info.address2 = brk.address2;
        info.read = brk.read;
        info.write = brk.write;
        info.execute = brk.execute;
        info.range = brk.range;
        result.push_back(info);
    }

    return result;
}

void DebugAdapter::SetBreakpointOnIRQ(int irq)
{
    if (irq >= 0 && irq < 8)
        emu_debug_irq_breakpoints[irq] = true;
}

void DebugAdapter::ClearBreakpointOnIRQ(int irq)
{
    if (irq >= 0 && irq < 8)
        emu_debug_irq_breakpoints[irq] = false;
}

u8 DebugAdapter::GetBreakpointIRQMask()
{
    u8 mask = 0;
    for (int i = 0; i < 8; i++)
    {
        if (emu_debug_irq_breakpoints[i])
            mask = SET_BIT(mask, i);
    }
    return mask;
}

void DebugAdapter::SetRegister(const std::string& name, u32 value)
{
    M6502* cpu = m_core->GetM6502();
    M6502::M6502_State* state = cpu->GetState();

    if (name == "PC")
        state->PC.SetValue((u16)value);
    else if (name == "A")
        state->A.SetValue((u8)value);
    else if (name == "X")
        state->X.SetValue((u8)value);
    else if (name == "Y")
        state->Y.SetValue((u8)value);
    else if (name == "S")
        state->S.SetValue((u8)value);
    else if (name == "P")
        state->P.SetValue((u8)value);
}

std::vector<MemoryAreaInfo> DebugAdapter::ListMemoryAreas()
{
    std::vector<MemoryAreaInfo> result;

    for (int i = 0; i < MEMORY_EDITOR_MAX; i++)
    {
        MemoryAreaInfo info = GetMemoryAreaInfo(i);
        if (IsValidPointer(info.data) && info.size > 0)
        {
            result.push_back(info);
        }
    }

    return result;
}

std::vector<u8> DebugAdapter::ReadMemoryArea(int area, u32 offset, size_t size)
{
    std::vector<u8> result;
    MemoryAreaInfo info = GetMemoryAreaInfo(area);

    if (!IsValidPointer(info.data))
        return result;

    // Adjust offset for areas with base addresses (matching GUI display)
    u32 base_address = 0;
    if (area == MEMORY_EDITOR_STACK)
        base_address = 0x100;
    else if (area == MEMORY_EDITOR_BIOS)
        base_address = 0xFE00;

    // If offset includes the base address, subtract it
    if (offset >= base_address)
        offset -= base_address;

    if (offset >= info.size)
        return result;

    u32 bytes_to_read = (u32)size;
    if (offset + bytes_to_read > info.size)
        bytes_to_read = info.size - offset;

    for (u32 i = 0; i < bytes_to_read; i++)
    {
        result.push_back(info.data[offset + i]);
    }

    return result;
}

void DebugAdapter::WriteMemoryArea(int area, u32 offset, const std::vector<u8>& data)
{
    MemoryAreaInfo info = GetMemoryAreaInfo(area);

    if (!IsValidPointer(info.data))
        return;

    // Adjust offset for areas with base addresses (matching GUI display)
    u32 base_address = 0;
    if (area == MEMORY_EDITOR_STACK)
        base_address = 0x100;
    else if (area == MEMORY_EDITOR_BIOS)
        base_address = 0xFE00;

    // If offset includes the base address, subtract it
    if (offset >= base_address)
        offset -= base_address;

    if (offset >= info.size)
        return;

    for (size_t i = 0; i < data.size() && (offset + i) < info.size; i++)
    {
        info.data[offset + i] = data[i];
    }
}

std::vector<DisasmLine> DebugAdapter::GetDisassembly(u16 start_address, u16 end_address, bool resolve_symbols)
{
    std::vector<DisasmLine> result;
    Memory* memory = m_core->GetMemory();

    // Scan backwards to find any instruction that might span into our range
    u16 scan_start = start_address;
    const int MAX_INSTRUCTION_SIZE = 3;  // 6502 instructions are 1-3 bytes

    for (int lookback = 1; lookback < MAX_INSTRUCTION_SIZE && scan_start > 0; lookback++)
    {
        u16 check_addr = start_address - lookback;

        GLYNX_Disassembler_Record* record = memory->GetDisassemblerRecord(check_addr);

        if (IsValidPointer(record) && record->name[0] != 0)
        {
            // Check if this instruction spans into our range
            u16 instr_end = check_addr + record->size - 1;
            if (instr_end >= start_address)
            {
                // This instruction overlaps with our range, start from here
                scan_start = check_addr;
                break;
            }
        }
    }

    u32 addr = scan_start;

    while (addr <= (u32)end_address)
    {
        GLYNX_Disassembler_Record* record = memory->GetDisassemblerRecord((u16)addr);

        if (IsValidPointer(record) && record->name[0] != 0)
        {
            DisasmLine line;
            line.address = (u16)addr;
            line.rom = record->rom;
            line.name = record->name;
            strip_color_tags(line.name);
            line.bytes = record->bytes;
            line.segment = record->segment;
            line.size = record->size;
            line.jump = record->jump;
            line.jump_address = record->jump_address;
            line.has_operand_address = record->has_operand_address;
            line.operand_address = record->operand_address;
            line.operand_is_zp = record->operand_is_zp;
            line.subroutine = record->subroutine;
            line.irq = record->irq;

            if (resolve_symbols)
            {
                std::string instr = line.name;
                if (!gui_debug_resolve_symbol(record, instr, "", ""))
                    gui_debug_resolve_label(record, instr, "", "");
                line.name = instr;
            }

            result.push_back(line);

            // Move to next instruction
            addr = addr + (u32)record->size;

            if (record->size == 0)
                addr++;
        }
        else
        {
            // No record at this address, try next byte
            addr++;
        }
    }

    return result;
}

MemoryAreaInfo DebugAdapter::GetMemoryAreaInfo(int area)
{
    MemoryAreaInfo info;
    info.id = area;
    info.data = NULL;
    info.size = 0;
    info.cpu_offset = 0;

    Memory* memory = m_core->GetMemory();
    Media* media = m_core->GetMedia();
    EEPROM* eeprom = media->GetEEPROMInstance();

    switch (area)
    {
        case MEMORY_EDITOR_RAM:
            info.name = "RAM";
            info.data = memory->GetRAM();
            info.size = 0x10000;
            info.cpu_offset = 0x0000;
            break;
        case MEMORY_EDITOR_ZERO_PAGE:
            info.name = "ZP";
            info.data = memory->GetRAM();
            info.size = 0x100;
            info.cpu_offset = 0x0000;
            break;
        case MEMORY_EDITOR_STACK:
            info.name = "STACK";
            info.data = memory->GetRAM() + 0x100;
            info.size = 0x100;
            info.cpu_offset = 0x0100;
            break;
        case MEMORY_EDITOR_BANK0:
            info.name = "BANK0";
            info.data = media->GetBankData(0);
            info.size = media->GetBankSize(0);
            info.cpu_offset = 0x0000;
            break;
        case MEMORY_EDITOR_BANK0A:
            info.name = "BANK0A";
            info.data = media->GetBankDataA(0);
            info.size = IsValidPointer(media->GetBankDataA(0)) ? media->GetBankSize(0) : 0;
            info.cpu_offset = 0x0000;
            break;
        case MEMORY_EDITOR_BANK1:
            info.name = "BANK1";
            info.data = media->GetBankData(1);
            info.size = media->GetBankSize(1);
            info.cpu_offset = 0x0000;
            break;
        case MEMORY_EDITOR_BANK1A:
            info.name = "BANK1A";
            info.data = media->GetBankDataA(1);
            info.size = IsValidPointer(media->GetBankDataA(1)) ? media->GetBankSize(1) : 0;
            info.cpu_offset = 0x0000;
            break;
        case MEMORY_EDITOR_BIOS:
            info.name = "BIOS";
            info.data = media->GetBIOS();
            info.size = GLYNX_BIOS_SIZE;
            info.cpu_offset = 0xFE00;
            break;
        case MEMORY_EDITOR_EEPROM:
            info.name = "EEPROM";
            if (eeprom->IsAvailable())
            {
                info.data = eeprom->GetData();
                info.size = eeprom->GetSize();
            }
            info.cpu_offset = 0x0000;
            break;
        default:
            break;
    }

    return info;
}

json DebugAdapter::GetMediaInfo()
{
    json info;
    Media* media = m_core->GetMedia();

    info["emulator"] = GLYNX_TITLE;
    info["emulator_version"] = GLYNX_VERSION;
    info["ready"] = media->IsReady();
    info["file_path"] = media->GetFilePath();
    info["file_name"] = media->GetFileName();
    info["file_directory"] = media->GetFileDirectory();
    info["file_extension"] = media->GetFileExtension();

    std::ostringstream crc_ss;
    crc_ss << std::hex << std::uppercase << std::setfill('0') << std::setw(8) << media->GetCRC();
    info["crc"] = crc_ss.str();

    info["rom_size"] = media->GetROMSize();

    // Media type
    Media::GLYNX_Media_Type type = media->GetType();
    switch (type)
    {
        case Media::MEDIA_LYNX:
            info["media_type"] = "Lynx";
            break;
        case Media::MEDIA_HOMEBREW:
            info["media_type"] = "Homebrew";
            break;
        default:
            info["media_type"] = "Unknown";
            break;
    }

    // Rotation
    GLYNX_Rotation rotation = media->GetRotation();
    switch (rotation)
    {
        case GLYNX_ROTATION_AUTO:
            info["rotation"] = "Auto";
            break;
        case GLYNX_ROTATION_LEFT:
            info["rotation"] = "90 CCW";
            break;
        case GLYNX_ROTATION_RIGHT:
            info["rotation"] = "90 CW";
            break;
        case GLYNX_ROTATION_DISABLED:
            info["rotation"] = "Disabled";
            break;
        default:
            info["rotation"] = "Unknown";
            break;
    }

    // EEPROM type
    GLYNX_EEPROM eeprom = media->GetEEPROM();
    u8 eeprom_base = eeprom & 0x3F;
    switch (eeprom_base)
    {
        case GLYNX_EEPROM_NONE:
            info["eeprom"] = "None";
            break;
        case GLYNX_EEPROM_93C46:
            info["eeprom"] = "93C46";
            break;
        case GLYNX_EEPROM_93C56:
            info["eeprom"] = "93C56";
            break;
        case GLYNX_EEPROM_93C66:
            info["eeprom"] = "93C66";
            break;
        case GLYNX_EEPROM_93C76:
            info["eeprom"] = "93C76";
            break;
        case GLYNX_EEPROM_93C86:
            info["eeprom"] = "93C86";
            break;
        default:
            info["eeprom"] = "Unknown";
            break;
    }
    info["eeprom_sd"] = (eeprom & GLYNX_EEPROM_SD) != 0;
    info["eeprom_8bit"] = (eeprom & GLYNX_EEPROM_8BIT) != 0;

    info["audin"] = media->GetAudin();
    info["bios_loaded"] = media->IsBiosLoaded();
    info["bios_valid"] = media->IsBiosValid();

    // Header data
    json header;
    header["name"] = media->GetHeaderName();
    header["manufacturer"] = media->GetHeaderManufacturer();
    header["bank0_page_size"] = media->GetHeaderBank0PageSize();
    header["bank1_page_size"] = media->GetHeaderBank1PageSize();
    info["header"] = header;

    if (type == Media::MEDIA_HOMEBREW)
    {
        std::ostringstream addr_ss;
        addr_ss << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << media->GetHomebrewBootAddress();
        info["homebrew_boot_address"] = addr_ss.str();
    }

    return info;
}

json DebugAdapter::Get6502Status()
{
    json status;
    M6502* processor = m_core->GetM6502();
    M6502::M6502_State* cpu = processor->GetState();
    Memory::Memory_State* mem = m_core->GetMemory()->GetState();
    Mikey::Mikey_State* mikey = m_core->GetMikey()->GetState();

    std::ostringstream ss;
    ss << std::hex << std::uppercase << std::setfill('0');

    // Status register (P) with flag breakdown
    ss << std::setw(2) << (int)cpu->P.GetValue();
    status["P"] = ss.str();
    ss.str("");

    // Individual flags
    u8 p = cpu->P.GetValue();
    status["flag_N"] = (p & 0x80) != 0;
    status["flag_V"] = (p & 0x40) != 0;
    status["flag_B"] = (p & 0x10) != 0;
    status["flag_D"] = (p & 0x08) != 0;
    status["flag_I"] = (p & 0x04) != 0;
    status["flag_Z"] = (p & 0x02) != 0;
    status["flag_C"] = (p & 0x01) != 0;

    // Program Counter
    ss << std::setw(4) << cpu->PC.GetValue();
    status["PC"] = ss.str();
    ss.str("");

    // Stack Pointer (full address)
    ss << std::setw(4) << (STACK_ADDR | cpu->S.GetValue());
    status["SP"] = ss.str();
    ss.str("");

    // Registers
    ss << std::setw(2) << (int)cpu->A.GetValue();
    status["A"] = ss.str();
    ss.str("");

    ss << std::setw(2) << (int)cpu->X.GetValue();
    status["X"] = ss.str();
    ss.str("");

    ss << std::setw(2) << (int)cpu->Y.GetValue();
    status["Y"] = ss.str();
    ss.str("");

    ss << std::setw(2) << (int)cpu->S.GetValue();
    status["S"] = ss.str();
    ss.str("");

    // MAPCTL register
    ss << std::setw(2) << (int)mem->MAPCTL;
    status["MAPCTL"] = ss.str();
    ss.str("");

    // Memory map visibility based on MAPCTL
    status["suzy_visible"] = !(mem->MAPCTL & 0x01);
    status["mikey_visible"] = !(mem->MAPCTL & 0x02);
    status["rom_visible"] = !(mem->MAPCTL & 0x04);
    status["vectors_visible"] = !(mem->MAPCTL & 0x08);

    // IRQ pending register
    ss << std::setw(2) << (int)mikey->irq_pending;
    status["IRQ_pending"] = ss.str();
    ss.str("");

    // IRQ mask register
    ss << std::setw(2) << (int)mikey->irq_mask;
    status["IRQ_mask"] = ss.str();
    ss.str("");

    // Individual timer/IRQ status (8 timers)
    json irqs = json::array();
    for (int i = 0; i < 8; i++)
    {
        json irq_info;
        irq_info["index"] = i;

        GLYNX_Mikey_Timer* timer = &mikey->timers[i];
        bool enabled = IS_SET_BIT(timer->control_a, 7);
        if (i == 4)
            enabled = (mikey->uart.tx_int_en || mikey->uart.rx_int_en);

        irq_info["enabled"] = enabled;
        irq_info["asserted"] = IS_SET_BIT(mikey->irq_pending, i);
        irq_info["masked"] = IS_SET_BIT(mikey->irq_mask, i);

        irqs.push_back(irq_info);
    }
    status["IRQs"] = irqs;

    // CPU IRQ line status
    status["IRQ_line_asserted"] = cpu->irq_asserted;

    // Last instruction ticks
    status["last_ticks"] = cpu->last_ticks;
    status["total_ticks"] = cpu->total_ticks;

    return status;
}

json DebugAdapter::GetMikeyRegisters(u16 address)
{
    Mikey* mikey = m_core->GetMikey();
    Mikey::Mikey_State* mikey_state = mikey->GetState();

    json registers = json::array();
    std::ostringstream ss;
    ss << std::hex << std::uppercase << std::setfill('0');

    // Timer registers ($FD00-$FD1F)
    for (int t = 0; t < 8; t++)
    {
        u16 base = 0xFD00 + (t * 4);
        GLYNX_Mikey_Timer* timer = &mikey_state->timers[t];
        
        char name[16];
        snprintf(name, sizeof(name), "TIM%dBKUP", t);
        AddRegister(registers, ss, name, base, timer->backup, address);
        snprintf(name, sizeof(name), "TIM%dCTLA", t);
        AddRegister(registers, ss, name, base + 1, timer->control_a, address);
        snprintf(name, sizeof(name), "TIM%dCNT", t);
        AddRegister(registers, ss, name, base + 2, timer->counter, address);
        snprintf(name, sizeof(name), "TIM%dCTLB", t);
        AddRegister(registers, ss, name, base + 3, timer->control_b, address);
    }

    // Audio registers ($FD20-$FD3F)
    for (int c = 0; c < 4; c++)
    {
        u16 base = 0xFD20 + (c * 8);
        GLYNX_Mikey_Audio* audio = &mikey_state->audio[c];
        
        char name[16];
        snprintf(name, sizeof(name), "AUD%dVOL", c);
        AddRegister(registers, ss, name, base, audio->volume, address);
        snprintf(name, sizeof(name), "AUD%dSHFTFB", c);
        AddRegister(registers, ss, name, base + 1, audio->feedback, address);
        snprintf(name, sizeof(name), "AUD%dOUTVAL", c);
        AddRegister(registers, ss, name, base + 2, (u8)audio->output, address);
        snprintf(name, sizeof(name), "AUD%dL8SHFT", c);
        AddRegister(registers, ss, name, base + 3, audio->lfsr_low, address);
        snprintf(name, sizeof(name), "AUD%dTBACK", c);
        AddRegister(registers, ss, name, base + 4, audio->backup, address);
        snprintf(name, sizeof(name), "AUD%dCTL", c);
        AddRegister(registers, ss, name, base + 5, audio->control, address);
        snprintf(name, sizeof(name), "AUD%dCOUNT", c);
        AddRegister(registers, ss, name, base + 6, audio->counter, address);
        snprintf(name, sizeof(name), "AUD%dMISC", c);
        AddRegister(registers, ss, name, base + 7, audio->other, address);
    }

    // Attenuation registers ($FD40-$FD43)
    AddRegister(registers, ss, "ATTEN_A", MIKEY_ATTEN_A, mikey_state->ATTEN_A, address);
    AddRegister(registers, ss, "ATTEN_B", MIKEY_ATTEN_B, mikey_state->ATTEN_B, address);
    AddRegister(registers, ss, "ATTEN_C", MIKEY_ATTEN_C, mikey_state->ATTEN_C, address);
    AddRegister(registers, ss, "ATTEN_D", MIKEY_ATTEN_D, mikey_state->ATTEN_D, address);

    // Pan register ($FD44)
    AddRegister(registers, ss, "MPAN", MIKEY_MPAN, mikey_state->MPAN, address);

    // Stereo register ($FD50)
    AddRegister(registers, ss, "MSTEREO", MIKEY_MSTEREO, mikey_state->MSTEREO, address);

    // Interrupt registers ($FD80-$FD81)
    AddRegister(registers, ss, "INTRST", MIKEY_INTRST, mikey->Read<true>(MIKEY_INTRST), address);
    AddRegister(registers, ss, "INTSET", MIKEY_INTSET, mikey->Read<true>(MIKEY_INTSET), address);

    // Misc registers ($FD84-$FD86)
    AddRegister(registers, ss, "MAGRDY0", MIKEY_MAGRDY0, mikey->Read<true>(MIKEY_MAGRDY0), address);
    AddRegister(registers, ss, "MAGRDY1", MIKEY_MAGRDY1, mikey->Read<true>(MIKEY_MAGRDY1), address);
    AddRegister(registers, ss, "AUDIN", MIKEY_AUDIN, mikey->Read<true>(MIKEY_AUDIN), address);

    // System control ($FD87)
    AddRegister(registers, ss, "SYSCTL1", MIKEY_SYSCTL1, mikey_state->SYSCTL1, address);

    // Hardware revision ($FD88-$FD89)
    AddRegister(registers, ss, "MIKEYHREV", MIKEY_MIKEYHREV, mikey->Read<true>(MIKEY_MIKEYHREV), address);
    AddRegister(registers, ss, "MIKEYSREV", MIKEY_MIKEYSREV, mikey->Read<true>(MIKEY_MIKEYSREV), address);

    // I/O registers ($FD8A-$FD8B)
    AddRegister(registers, ss, "IODIR", MIKEY_IODIR, mikey_state->IODIR, address);
    AddRegister(registers, ss, "IODAT", MIKEY_IODAT, mikey_state->IODAT, address);

    // Serial registers ($FD8C-$FD8D)
    AddRegister(registers, ss, "SERCTL", MIKEY_SERCTL, mikey_state->SERCTL, address);
    AddRegister(registers, ss, "SERDAT", MIKEY_SERDAT, mikey_state->SERDAT, address);

    // Display/sleep registers ($FD90-$FD95)
    AddRegister(registers, ss, "SDONEACK", MIKEY_SDONEACK, mikey_state->SDONEACK, address);
    AddRegister(registers, ss, "CPUSLEEP", MIKEY_CPUSLEEP, mikey_state->CPUSLEEP, address);
    AddRegister(registers, ss, "DISPCTL", MIKEY_DISPCTL, mikey_state->DISPCTL, address);
    AddRegister(registers, ss, "PBKUP", MIKEY_PBKUP, mikey_state->PBKUP, address);
    AddRegister16(registers, ss, "DISPADR", MIKEY_DISPADRL, mikey_state->DISPADR.value, address);

    // Test registers ($FD9C-$FD9E)
    AddRegister(registers, ss, "MTEST0", MIKEY_MTEST0, mikey->Read<true>(MIKEY_MTEST0), address);
    AddRegister(registers, ss, "MTEST1", MIKEY_MTEST1, mikey->Read<true>(MIKEY_MTEST1), address);
    AddRegister(registers, ss, "MTEST2", MIKEY_MTEST2, mikey->Read<true>(MIKEY_MTEST2), address);

    // Color palette registers ($FDA0-$FDBF)
    for (int i = 0; i < 16; i++)
    {
        char name_g[16], name_br[16];
        snprintf(name_g, sizeof(name_g), "GREEN%X", i);
        snprintf(name_br, sizeof(name_br), "BLUERED%X", i);
        AddRegister(registers, ss, name_g, MIKEY_GREEN0 + i, mikey->Read<true>(MIKEY_GREEN0 + i), address);
        AddRegister(registers, ss, name_br, MIKEY_BLUERED0 + i, mikey->Read<true>(MIKEY_BLUERED0 + i), address);
    }

    if (address != 0xFFFF && registers.empty())
    {
        json result;
        result["error"] = "Invalid Mikey register address";
        return result;
    }

    if (address != 0xFFFF && registers.size() == 1)
        return registers[0];

    return registers;
}

json DebugAdapter::WriteMikeyRegister(u16 address, u8 value)
{
    json result;

    if (address < 0xFD00 || address > 0xFDFF)
    {
        result["error"] = "Invalid Mikey address (must be FD00-FDFF)";
        return result;
    }

    Mikey* mikey = m_core->GetMikey();
    mikey->Write<true>(address, value);

    result["success"] = true;
    std::ostringstream ss;
    ss << std::hex << std::uppercase << std::setfill('0');
    ss << std::setw(4) << address;
    result["address"] = ss.str();
    ss.str("");
    ss << std::setw(2) << (int)value;
    result["value"] = ss.str();

    // If writing to color palette ($FDA0-$FDBF), include decoded color info
    if (address >= 0xFDA0 && address <= 0xFDBF)
    {
        int color_index = (address - 0xFDA0) / 2;
        
        u16 color_base = 0xFDA0 + (color_index * 2);
        u8 green = mikey->Read<true>(color_base) & 0x0F;
        u8 blue_red = mikey->Read<true>(color_base + 1);
        u8 blue = (blue_red >> 4) & 0x0F;
        u8 red = blue_red & 0x0F;
        
        result["color_index"] = color_index;
        ss << std::setw(1) << (int)green;
        result["green"] = ss.str();
        ss.str("");
        ss << std::setw(1) << (int)blue;
        result["blue"] = ss.str();
        ss.str("");
        ss << std::setw(1) << (int)red;
        result["red"] = ss.str();
    }

    return result;
}

json DebugAdapter::GetMikeyTimers(int timer)
{
    Mikey::Mikey_State* mikey_state = m_core->GetMikey()->GetState();
    
    if (timer < -1 || timer > 7)
    {
        json result;
        result["error"] = "Invalid timer index (must be 0-7, or -1 for all)";
        return result;
    }

    static const char* k_timer_names[8] = {
        "HBLANK TIMER", "TIMER 1", "VBLANK TIMER", "TIMER 3",
        "UART TIMER", "TIMER 5", "TIMER 6", "TIMER 7"
    };
    static const char* k_period_strs[8] = {
        "1 MHz (1us)", "500 KHz (2us)", "250 KHz (4us)", "125 KHz (8us)",
        "62.5 KHz (16us)", "31.25 KHz (32us)", "15.625 KHz (64us)", "N/A"
    };

    json timers = json::array();
    std::ostringstream ss;
    ss << std::hex << std::uppercase << std::setfill('0');

    int start = (timer == -1) ? 0 : timer;
    int end = (timer == -1) ? 8 : timer + 1;

    for (int t = start; t < end; t++)
    {
        GLYNX_Mikey_Timer* timer_data = &mikey_state->timers[t];
        u8 period = (timer_data->control_a & 0x07);
        bool is_linked = (period == 7) && (k_mikey_timer_backward_links[t] != -1);
        bool enabled = IS_SET_BIT(timer_data->control_a, 3);
        bool reload = IS_SET_BIT(timer_data->control_a, 4);
        bool interrupt = IS_SET_BIT(timer_data->control_a, 7);
        bool reset_timer_done = IS_SET_BIT(timer_data->control_a, 6);
        bool timer_done = IS_SET_BIT(timer_data->control_b, 3);
        bool borrow_in = IS_SET_BIT(timer_data->control_b, 1);
        bool borrow_out = IS_SET_BIT(timer_data->control_b, 0);

        json timer_obj;
        timer_obj["index"] = t;
        timer_obj["name"] = k_timer_names[t];

        // Register addresses and values
        u16 base_addr = 0xFD00 + (t * 4);
        ss << std::setw(4) << base_addr;
        timer_obj["backup_addr"] = ss.str();
        ss.str("");
        ss << std::setw(2) << (int)timer_data->backup;
        timer_obj["backup"] = ss.str();
        ss.str("");

        ss << std::setw(4) << (base_addr + 1);
        timer_obj["control_a_addr"] = ss.str();
        ss.str("");
        ss << std::setw(2) << (int)timer_data->control_a;
        timer_obj["control_a"] = ss.str();
        ss.str("");

        ss << std::setw(4) << (base_addr + 2);
        timer_obj["counter_addr"] = ss.str();
        ss.str("");
        ss << std::setw(2) << (int)timer_data->counter;
        timer_obj["counter"] = ss.str();
        ss.str("");

        ss << std::setw(4) << (base_addr + 3);
        timer_obj["control_b_addr"] = ss.str();
        ss.str("");
        ss << std::setw(2) << (int)timer_data->control_b;
        timer_obj["control_b"] = ss.str();
        ss.str("");

        // Status flags
        timer_obj["enabled"] = enabled;
        timer_obj["reload"] = reload;
        timer_obj["interrupt"] = (t != 4) ? interrupt : false;
        timer_obj["interrupt_available"] = (t != 4);
        timer_obj["reset_done"] = reset_timer_done;
        timer_obj["frequency"] = k_period_strs[period];
        timer_obj["period_value"] = period;
        
        if (is_linked)
        {
            int link = k_mikey_timer_backward_links[t];
            timer_obj["linked"] = true;
            if (link < 8)
            {
                timer_obj["linked_to_type"] = "timer";
                timer_obj["linked_to_index"] = link;
            }
            else
            {
                timer_obj["linked_to_type"] = "audio";
                timer_obj["linked_to_index"] = link - 8;
            }
        }
        else
        {
            timer_obj["linked"] = false;
        }

        timer_obj["timer_done"] = timer_done;
        timer_obj["borrow_in"] = borrow_in;
        timer_obj["borrow_out"] = borrow_out;

        timers.push_back(timer_obj);
    }

    json result;
    if (timer == -1)
        result["timers"] = timers;
    else
        result = timers[0];

    return result;
}

json DebugAdapter::GetMikeyAudio(int channel)
{
    Mikey::Mikey_State* mikey_state = m_core->GetMikey()->GetState();
    
    if (channel < -1 || channel > 3)
    {
        json result;
        result["error"] = "Invalid channel index (must be 0-3, or -1 for all)";
        return result;
    }

    static const char* k_period_strs[8] = {
        "1 MHz (1us)", "500 KHz (2us)", "250 KHz (4us)", "125 KHz (8us)",
        "62.5 KHz (16us)", "31.25 KHz (32us)", "15.625 KHz (64us)", "N/A"
    };

    json channels = json::array();
    std::ostringstream ss;
    ss << std::hex << std::uppercase << std::setfill('0');

    int start = (channel == -1) ? 0 : channel;
    int end = (channel == -1) ? 4 : channel + 1;

    for (int c = start; c < end; c++)
    {
        GLYNX_Mikey_Audio* channel_data = &mikey_state->audio[c];
        u8 period = (channel_data->control & 0x07);
        bool is_linked = (period == 7);
        bool enabled = IS_SET_BIT(channel_data->control, 3);
        bool reload = IS_SET_BIT(channel_data->control, 4);
        bool integrate = IS_SET_BIT(channel_data->control, 5);
        bool reset_timer_done = IS_SET_BIT(channel_data->control, 6);
        bool timer_done = IS_SET_BIT(channel_data->other, 3);
        bool borrow_in = IS_SET_BIT(channel_data->other, 1);
        bool borrow_out = IS_SET_BIT(channel_data->other, 0);
        u8 mstereo = mikey_state->MSTEREO;
        u8 mpan = mikey_state->MPAN;
        u8 ch_atten = (&mikey_state->ATTEN_A)[c];

        json channel_obj;
        channel_obj["index"] = c;

        // Register addresses and values
        u16 base_addr = 0xFD20 + (c * 8);
        ss << std::setw(4) << base_addr;
        channel_obj["volume_addr"] = ss.str();
        ss.str("");
        ss << std::setw(2) << (int)channel_data->volume;
        channel_obj["volume"] = ss.str();
        ss.str("");

        ss << std::setw(4) << (base_addr + 1);
        channel_obj["feedback_addr"] = ss.str();
        ss.str("");
        ss << std::setw(2) << (int)channel_data->feedback;
        channel_obj["feedback"] = ss.str();
        ss.str("");

        ss << std::setw(4) << (base_addr + 2);
        channel_obj["output_addr"] = ss.str();
        ss.str("");
        ss << std::setw(2) << (int)(u8)channel_data->output;
        channel_obj["output"] = ss.str();
        ss.str("");

        ss << std::setw(4) << (base_addr + 3);
        channel_obj["lfsr_low_addr"] = ss.str();
        ss.str("");
        ss << std::setw(2) << (int)channel_data->lfsr_low;
        channel_obj["lfsr_low"] = ss.str();
        ss.str("");

        ss << std::setw(4) << (base_addr + 4);
        channel_obj["backup_addr"] = ss.str();
        ss.str("");
        ss << std::setw(2) << (int)channel_data->backup;
        channel_obj["backup"] = ss.str();
        ss.str("");

        ss << std::setw(4) << (base_addr + 5);
        channel_obj["control_addr"] = ss.str();
        ss.str("");
        ss << std::setw(2) << (int)channel_data->control;
        channel_obj["control"] = ss.str();
        ss.str("");

        ss << std::setw(4) << (base_addr + 6);
        channel_obj["counter_addr"] = ss.str();
        ss.str("");
        ss << std::setw(2) << (int)channel_data->counter;
        channel_obj["counter"] = ss.str();
        ss.str("");

        ss << std::setw(4) << (base_addr + 7);
        channel_obj["other_addr"] = ss.str();
        ss.str("");
        ss << std::setw(2) << (int)channel_data->other;
        channel_obj["other"] = ss.str();
        ss.str("");

        ss << std::setw(4) << (0xFD40 + c);
        channel_obj["atten_addr"] = ss.str();
        ss.str("");
        ss << std::setw(2) << (int)ch_atten;
        channel_obj["atten"] = ss.str();
        ss.str("");

        // Status flags
        channel_obj["enabled"] = enabled;
        channel_obj["reload"] = reload;
        channel_obj["integrate"] = integrate;
        channel_obj["reset_done"] = reset_timer_done;
        channel_obj["frequency"] = k_period_strs[period];
        channel_obj["period_value"] = period;
        
        if (is_linked)
        {
            int link = k_mikey_audio_backward_links[c];
            channel_obj["linked"] = true;
            if (link < 0)
            {
                channel_obj["linked_to_type"] = "timer";
                channel_obj["linked_to_index"] = 7;
            }
            else
            {
                channel_obj["linked_to_type"] = "audio";
                channel_obj["linked_to_index"] = link;
            }
        }
        else
        {
            channel_obj["linked"] = false;
        }

        channel_obj["timer_done"] = timer_done;
        channel_obj["borrow_in"] = borrow_in;
        channel_obj["borrow_out"] = borrow_out;

        // Stereo and pan info
        bool left_enabled = IS_NOT_SET_BIT(mstereo, 4 + c);
        bool left_att_en = IS_SET_BIT(mpan, 4 + c);
        u8 left_vol = (ch_atten >> 4) & 0xF;

        channel_obj["left_enabled"] = left_enabled;
        channel_obj["left_attenuation_enabled"] = left_att_en;
        channel_obj["left_volume"] = left_vol;

        bool right_enabled = IS_NOT_SET_BIT(mstereo, c);
        bool right_att_en = IS_SET_BIT(mpan, c);
        u8 right_vol = ch_atten & 0xF;

        channel_obj["right_enabled"] = right_enabled;
        channel_obj["right_attenuation_enabled"] = right_att_en;
        channel_obj["right_volume"] = right_vol;

        channels.push_back(channel_obj);
    }

    // Add stereo control registers
    json stereo;
    ss << std::setw(2) << (int)mikey_state->MSTEREO;
    stereo["mstereo"] = ss.str();
    ss.str("");
    ss << std::setw(2) << (int)mikey_state->MPAN;
    stereo["mpan"] = ss.str();

    json result;
    if (channel == -1)
    {
        result["channels"] = channels;
        result["stereo"] = stereo;
    }
    else
    {
        result = channels[0];
        result["stereo"] = stereo;
    }

    return result;
}

json DebugAdapter::GetSuzyRegisters(u16 address)
{
    Suzy* suzy = m_core->GetSuzy();
    Suzy::Suzy_State* suzy_state = suzy->GetState();
    Input* input = m_core->GetInput();
    u8 joystick = input->ReadJoystick();
    u8 switches = input->ReadSwitches();
    u8 sprsys = suzy->Read<true>(SUZY_SPRSYS);

    json registers = json::array();
    std::ostringstream ss;
    ss << std::hex << std::uppercase << std::setfill('0');

    // 16-bit sprite registers ($FC00-$FC2F)
    AddRegister16(registers, ss, "TMPADR", SUZY_TMPADRL, suzy_state->TMPADR.value, address);
    AddRegister16(registers, ss, "TILTACUM", SUZY_TILTACUML, suzy_state->TILTACUM.value, address);
    AddRegister16(registers, ss, "HOFF", SUZY_HOFFL, suzy_state->HOFF.value, address);
    AddRegister16(registers, ss, "VOFF", SUZY_VOFFL, suzy_state->VOFF.value, address);
    AddRegister16(registers, ss, "VIDBAS", SUZY_VIDBASL, suzy_state->VIDBAS.value, address);
    AddRegister16(registers, ss, "COLLBAS", SUZY_COLLBASL, suzy_state->COLLBAS.value, address);
    AddRegister16(registers, ss, "VIDADR", SUZY_VIDADRL, suzy_state->VIDADR.value, address);
    AddRegister16(registers, ss, "COLLADR", SUZY_COLLADRL, suzy_state->COLLADR.value, address);
    AddRegister16(registers, ss, "SCBNEXT", SUZY_SCBNEXTL, suzy_state->SCBNEXT.value, address);
    AddRegister16(registers, ss, "SPRDLINE", SUZY_SPRDLINEL, suzy_state->SPRDLINE.value, address);
    AddRegister16(registers, ss, "HPOSSTRT", SUZY_HPOSSTRTL, suzy_state->HPOSSTRT.value, address);
    AddRegister16(registers, ss, "VPOSSTRT", SUZY_VPOSSTRTL, suzy_state->VPOSSTRT.value, address);
    AddRegister16(registers, ss, "SPRHSIZ", SUZY_SPRHSIZL, suzy_state->SPRHSIZ.value, address);
    AddRegister16(registers, ss, "SPRVSIZ", SUZY_SPRVSIZL, suzy_state->SPRVSIZ.value, address);
    AddRegister16(registers, ss, "STRETCH", SUZY_STRETCHL, suzy_state->STRETCH.value, address);
    AddRegister16(registers, ss, "TILT", SUZY_TILTL, suzy_state->TILT.value, address);
    AddRegister16(registers, ss, "SPRDOFF", SUZY_SPRDOFFL, suzy_state->SPRDOFF.value, address);
    AddRegister16(registers, ss, "SPRVPOS", SUZY_SPRVPOSL, suzy_state->SPRVPOS.value, address);
    AddRegister16(registers, ss, "COLLOFF", SUZY_COLLOFFL, suzy_state->COLLOFF.value, address);
    AddRegister16(registers, ss, "VSIZACUM", SUZY_VSIZACUML, suzy_state->VSIZACUM.value, address);
    AddRegister16(registers, ss, "HSIZOFF", SUZY_HSIZOFFL, suzy_state->HSIZOFF.value, address);
    AddRegister16(registers, ss, "VSIZOFF", SUZY_VSIZOFFL, suzy_state->VSIZOFF.value, address);
    AddRegister16(registers, ss, "SCBADR", SUZY_SCBADRL, suzy_state->SCBADR.value, address);
    AddRegister16(registers, ss, "PROCADR", SUZY_PROCADRL, suzy_state->PROCADR.value, address);

    // Math registers ($FC52-$FC57, $FC60-$FC63, $FC6C-$FC6F)
    AddRegister(registers, ss, "MATHD", SUZY_MATHD, suzy->Read<true>(SUZY_MATHD), address);
    AddRegister(registers, ss, "MATHC", SUZY_MATHC, suzy->Read<true>(SUZY_MATHC), address);
    AddRegister(registers, ss, "MATHB", SUZY_MATHB, suzy->Read<true>(SUZY_MATHB), address);
    AddRegister(registers, ss, "MATHA", SUZY_MATHA, suzy->Read<true>(SUZY_MATHA), address);
    AddRegister(registers, ss, "MATHP", SUZY_MATHP, suzy->Read<true>(SUZY_MATHP), address);
    AddRegister(registers, ss, "MATHN", SUZY_MATHN, suzy->Read<true>(SUZY_MATHN), address);
    AddRegister(registers, ss, "MATHH", SUZY_MATHH, suzy->Read<true>(SUZY_MATHH), address);
    AddRegister(registers, ss, "MATHG", SUZY_MATHG, suzy->Read<true>(SUZY_MATHG), address);
    AddRegister(registers, ss, "MATHF", SUZY_MATHF, suzy->Read<true>(SUZY_MATHF), address);
    AddRegister(registers, ss, "MATHE", SUZY_MATHE, suzy->Read<true>(SUZY_MATHE), address);
    AddRegister(registers, ss, "MATHM", SUZY_MATHM, suzy->Read<true>(SUZY_MATHM), address);
    AddRegister(registers, ss, "MATHL", SUZY_MATHL, suzy->Read<true>(SUZY_MATHL), address);
    AddRegister(registers, ss, "MATHK", SUZY_MATHK, suzy->Read<true>(SUZY_MATHK), address);
    AddRegister(registers, ss, "MATHJ", SUZY_MATHJ, suzy->Read<true>(SUZY_MATHJ), address);

    // Sprite control registers ($FC80-$FC83)
    AddRegister(registers, ss, "SPRCTL0", SUZY_SPRCTL0, suzy_state->SPRCTL0, address);
    AddRegister(registers, ss, "SPRCTL1", SUZY_SPRCTL1, suzy_state->SPRCTL1, address);
    AddRegister(registers, ss, "SPRCOLL", SUZY_SPRCOLL, suzy_state->SPRCOLL, address);
    AddRegister(registers, ss, "SPRINIT", SUZY_SPRINIT, suzy_state->SPRINIT, address);

    // Hardware revision ($FC88-$FC89)
    AddRegister(registers, ss, "SUZYHREV", SUZY_SUZYHREV, suzy->Read<true>(SUZY_SUZYHREV), address);
    AddRegister(registers, ss, "SUZYSREV", SUZY_SUZYSREV, suzy->Read<true>(SUZY_SUZYSREV), address);

    // Sprite engine control ($FC90-$FC92)
    AddRegister(registers, ss, "SUZYBUSEN", SUZY_SUZYBUSEN, suzy_state->SUZYBUSEN, address);
    AddRegister(registers, ss, "SPRGO", SUZY_SPRGO, suzy_state->SPRGO, address);
    AddRegister(registers, ss, "SPRSYS", SUZY_SPRSYS, sprsys, address);

    // Input registers ($FCB0-$FCB3)
    AddRegister(registers, ss, "JOYSTICK", SUZY_JOYSTICK, joystick, address);
    AddRegister(registers, ss, "SWITCHES", SUZY_SWITCHES, switches, address);
    AddRegister(registers, ss, "RCART0", SUZY_RCART0, suzy->Read<true>(SUZY_RCART0), address);
    AddRegister(registers, ss, "RCART1", SUZY_RCART1, suzy->Read<true>(SUZY_RCART1), address);

    // Misc registers ($FCC0-$FCC4)
    AddRegister(registers, ss, "LEDS", SUZY_LEDS, suzy->Read<true>(SUZY_LEDS), address);
    AddRegister(registers, ss, "PPORTSTAT", SUZY_PPORTSTAT, suzy->Read<true>(SUZY_PPORTSTAT), address);
    AddRegister(registers, ss, "PPORTDATA", SUZY_PPORTDATA, suzy->Read<true>(SUZY_PPORTDATA), address);
    AddRegister(registers, ss, "HOWIE", SUZY_HOWIE, suzy->Read<true>(SUZY_HOWIE), address);

    if (address != 0xFFFF && registers.empty())
    {
        json result;
        result["error"] = "Invalid Suzy register address";
        return result;
    }

    if (address != 0xFFFF && registers.size() == 1)
        return registers[0];

    return registers;
}

json DebugAdapter::WriteSuzyRegister(u16 address, u8 value)
{
    json result;

    if (address < 0xFC00 || address > 0xFCFF)
    {
        result["error"] = "Invalid Suzy address (must be FC00-FCFF)";
        return result;
    }

    Suzy* suzy = m_core->GetSuzy();
    suzy->Write<true>(address, value);

    result["success"] = true;
    std::ostringstream ss;
    ss << std::hex << std::uppercase << std::setfill('0');
    ss << std::setw(4) << address;
    result["address"] = ss.str();
    ss.str("");
    ss << std::setw(2) << (int)value;
    result["value"] = ss.str();

    return result;
}

json DebugAdapter::GetUARTStatus()
{
    Mikey::Mikey_State* mikey_state = m_core->GetMikey()->GetState();

    json status;
    std::ostringstream ss;
    ss << std::hex << std::uppercase << std::setfill('0');

    // Register values
    json registers;
    ss << std::setw(2) << (int)mikey_state->SERCTL;
    registers["SERCTL"] = ss.str();
    ss.str("");
    ss << std::setw(2) << (int)mikey_state->SERDAT;
    registers["SERDAT"] = ss.str();
    ss.str("");
    status["registers"] = registers;

    // IRQ status
    bool irq_enabled = (mikey_state->uart.tx_int_en || mikey_state->uart.rx_int_en);
    bool irq_asserted = IS_SET_BIT(mikey_state->irq_pending, 4);
    status["irq_enabled"] = irq_enabled;
    status["irq_asserted"] = irq_asserted;

    // Control flags
    json control;
    control["tx_irq_enabled"] = mikey_state->uart.tx_int_en;
    control["rx_irq_enabled"] = mikey_state->uart.rx_int_en;
    control["parity_enabled"] = mikey_state->uart.par_en;
    control["tx_open"] = mikey_state->uart.tx_open;
    control["tx_break"] = mikey_state->uart.tx_brk;
    control["parity_even"] = mikey_state->uart.par_even;
    status["control"] = control;

    // Status flags
    json flags;
    flags["tx_ready"] = mikey_state->uart.tx_ready;
    flags["rx_ready"] = mikey_state->uart.rx_ready;
    flags["tx_empty"] = mikey_state->uart.tx_empty;
    flags["parity_error"] = mikey_state->uart.par_err;
    flags["overrun_error"] = mikey_state->uart.ovr_err;
    flags["framing_error"] = mikey_state->uart.fram_err;
    flags["rx_break"] = mikey_state->uart.rx_break;
    flags["parity_bit"] = mikey_state->uart.par_bit;
    status["status"] = flags;

    // Data registers
    json data;
    ss << std::setw(2) << (int)mikey_state->uart.tx_hold_data;
    data["tx_holding"] = ss.str();
    ss.str("");
    ss << std::setw(2) << (int)mikey_state->uart.tx_data;
    data["tx_data"] = ss.str();
    ss.str("");
    ss << std::setw(2) << (int)mikey_state->uart.rx_data;
    data["rx_data"] = ss.str();
    ss.str("");
    data["tx_bit_index"] = mikey_state->uart.tx_bit_index;
    status["data"] = data;

    return status;
}

static const char* get_eeprom_type_name(GLYNX_EEPROM type)
{
    s32 base_type = type & 0x0F;

    switch (base_type)
    {
        case GLYNX_EEPROM_93C46:
            return "93C46";
        case GLYNX_EEPROM_93C56:
            return "93C56";
        case GLYNX_EEPROM_93C66:
            return "93C66";
        case GLYNX_EEPROM_93C76:
            return "93C76";
        case GLYNX_EEPROM_93C86:
            return "93C86";
        default:
            return "Unknown";
    }
}

json DebugAdapter::GetCartStatus()
{
    Media* media = m_core->GetMedia();
    bool ready = media->IsReady();

    json result;
    std::ostringstream ss;
    ss << std::hex << std::uppercase << std::setfill('0');

    result["ready"] = ready;

    // Address generation
    json address_gen;
    if (ready)
    {
        ss << std::setw(2) << (int)media->GetAddressShift();
        address_gen["addr_shift"] = ss.str();
        ss.str("");

        ss << std::setw(3) << media->GetCounterValue();
        address_gen["page_offset"] = ss.str();
        ss.str("");
        address_gen["page_offset_decimal"] = (int)media->GetCounterValue();

        address_gen["strobe"] = media->GetShiftRegisterStrobe();
        address_gen["shift_bit"] = media->GetShiftRegisterBit();
    }
    result["address_generation"] = address_gen;

    // Bank 0
    json bank0;
    if (ready && media->GetBankSize(0) > 0)
    {
        bank0["size_kb"] = media->GetBankSize(0) / 1024;
        bank0["type"] = "ROM";
        ss << std::setw(2) << (int)media->PeekBank0();
        bank0["peek_value"] = ss.str();
        ss.str("");
        if (IsValidPointer(media->GetBankDataA(0)))
        {
            ss << std::setw(2) << (int)media->PeekBank0A();
            bank0["peek_value_a"] = ss.str();
            ss.str("");
        }
    }
    result["bank0"] = bank0;

    // Bank 1
    json bank1;
    if (ready && media->GetBankSize(1) > 0)
    {
        bank1["size_kb"] = media->GetBankSize(1) / 1024;
        bank1["type"] = media->IsBank1RAM() ? "RAM" : "ROM";

        ss << std::setw(2) << (int)media->PeekBank1();
        bank1["peek_value"] = ss.str();
        ss.str("");
        if (IsValidPointer(media->GetBankDataA(1)))
        {
            ss << std::setw(2) << (int)media->PeekBank1A();
            bank1["peek_value_a"] = ss.str();
            ss.str("");
        }
    }
    result["bank1"] = bank1;

    // AUDIN
    json audin;
    if (ready)
    {
        audin["active"] = media->GetAudin();
        if (media->GetAudin())
        {
            audin["value"] = media->GetAudinValue();
        }
    }
    result["audin"] = audin;

    return result;
}

json DebugAdapter::GetEepromStatus()
{
    Media* media = m_core->GetMedia();
    Mikey* mikey = m_core->GetMikey();
    EEPROM* eeprom = media->GetEEPROMInstance();
    Mikey::Mikey_State* mikey_state = mikey->GetState();

    json result;
    std::ostringstream ss;
    ss << std::hex << std::uppercase << std::setfill('0');

    bool available = eeprom->IsAvailable();
    result["available"] = available;

    // Info
    json info;
    if (available)
    {
        GLYNX_EEPROM type = eeprom->GetType();
        info["type"] = get_eeprom_type_name(type);
        info["size_bytes"] = eeprom->GetSize();
        info["mode"] = (type & GLYNX_EEPROM_8BIT) ? "8-bit" : "16-bit";
    }
    result["info"] = info;

    // State
    json state;
    if (available)
    {
        state["dirty"] = eeprom->IsDirty();
        state["output_bit"] = eeprom->OutputBit();
    }
    result["state"] = state;

    // IO Pins
    json io_pins;
    u8 iodir = mikey_state->IODIR;
    u8 iodat = mikey_state->IODAT;
    u8 iodat_read = mikey->Read<true>(MIKEY_IODAT);

    ss << std::setw(2) << (int)iodir;
    io_pins["IODIR"] = ss.str();
    ss.str("");

    ss << std::setw(2) << (int)iodat_read;
    io_pins["IODAT"] = ss.str();
    ss.str("");

    // CS on bit 2, CLK on bit 1, DI on bit 0
    bool cs = IS_SET_BIT(iodat, 2) && IS_SET_BIT(iodir, 2);
    bool clk = IS_SET_BIT(iodat, 1);
    bool di = IS_SET_BIT(iodat, 0);

    io_pins["CS"] = cs;
    io_pins["CLK"] = clk;
    io_pins["DI"] = di;

    result["io_pins"] = io_pins;

    return result;
}

// Base64 encoding table
static const char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static std::string base64_encode(const unsigned char* data, int size)
{
    std::string result;
    result.reserve(((size + 2) / 3) * 4);

    int i = 0;
    while (i < size)
    {
        unsigned char byte1 = data[i++];
        unsigned char byte2 = (i < size) ? data[i++] : 0;
        unsigned char byte3 = (i < size) ? data[i++] : 0;

        result.push_back(base64_chars[byte1 >> 2]);
        result.push_back(base64_chars[((byte1 & 0x03) << 4) | (byte2 >> 4)]);
        result.push_back((i > size + 1) ? '=' : base64_chars[((byte2 & 0x0F) << 2) | (byte3 >> 6)]);
        result.push_back((i > size) ? '=' : base64_chars[byte3 & 0x3F]);
    }

    return result;
}

json DebugAdapter::GetScreenshot()
{
    json result;

    if (!m_core || !m_core->GetMedia()->IsReady())
    {
        result["error"] = "No media loaded";
        return result;
    }

    // Get runtime info for screen dimensions
    GLYNX_Runtime_Info runtime;
    m_core->GetRuntimeInfo(runtime);

    // Get PNG screenshot from emu
    unsigned char* png_buffer = NULL;
    int png_size = emu_get_screenshot_png(&png_buffer);

    if (png_size == 0 || !png_buffer)
    {
        result["error"] = "Failed to capture screenshot";
        return result;
    }

    // Encode PNG data to base64
    std::string base64_png = base64_encode(png_buffer, png_size);

    // Free the buffer allocated by stbi_write_png_to_mem (uses malloc internally)
    free(png_buffer);

    result["__mcp_image"] = true;
    result["data"] = base64_png;
    result["mimeType"] = "image/png";
    result["width"] = runtime.screen_width;
    result["height"] = runtime.screen_height;

    return result;
}

json DebugAdapter::GetFrameBuffer(const std::string& buffer_type)
{
    json result;

    if (!m_core || !m_core->GetMedia()->IsReady())
    {
        result["error"] = "No media loaded";
        return result;
    }

    int buffer_index = 0;
    std::string buffer_name;

    if (buffer_type == "vidbas")
    {
        buffer_index = 0;
        buffer_name = "VIDBAS (Suzy)";
    }
    else if (buffer_type == "dispadr")
    {
        buffer_index = 1;
        buffer_name = "DISPADR (Mikey)";
    }
    else
    {
        result["error"] = "Invalid buffer type. Use 'vidbas' or 'dispadr'";
        return result;
    }

    // Get PNG frame buffer from emu
    unsigned char* png_buffer = NULL;
    int png_size = emu_get_framebuffer_png(buffer_index, &png_buffer);

    if (png_size == 0 || !png_buffer)
    {
        result["error"] = "Failed to capture frame buffer";
        return result;
    }

    // Encode PNG data to base64
    std::string base64_png = base64_encode(png_buffer, png_size);

    // Free the buffer allocated by stbi_write_png_to_mem (uses malloc internally)
    free(png_buffer);

    result["__mcp_image"] = true;
    result["data"] = base64_png;
    result["mimeType"] = "image/png";
    result["width"] = GLYNX_SCREEN_WIDTH;
    result["height"] = GLYNX_SCREEN_HEIGHT;
    result["buffer"] = buffer_name;

    return result;
}

json DebugAdapter::GetSprite(int index, const std::string& format)
{
    json result;

    if (!m_core || !m_core->GetMedia()->IsReady())
    {
        result["error"] = "No media loaded";
        return result;
    }

    if (index < 0 || index >= DEBUG_MAX_SPRITES || index >= emu_debug_scb_count)
    {
        result["error"] = "Invalid sprite index";
        return result;
    }

    GLYNX_Debug_SCB_Info& info = emu_debug_scb_info[index];
    int width = emu_debug_sprite_widths[index];
    int height = emu_debug_sprite_heights[index];

    if (format == "info")
    {
        std::ostringstream ss;
        ss << std::hex << std::uppercase << std::setfill('0');

        result["scb_index"] = index;
        ss << std::setw(4) << info.scb_address;
        result["scb_address"] = ss.str(); ss.str("");
        ss << std::setw(4) << info.scb_next;
        result["scb_next"] = ss.str(); ss.str("");
        ss << std::setw(2) << (int)info.sprctl0;
        result["sprctl0"] = ss.str(); ss.str("");
        ss << std::setw(2) << (int)info.sprctl1;
        result["sprctl1"] = ss.str(); ss.str("");
        ss << std::setw(2) << (int)info.sprcoll;
        result["sprcoll"] = ss.str(); ss.str("");
        result["hpos"] = info.hpos;
        result["vpos"] = info.vpos;
        result["width"] = width;
        result["height"] = height;
        result["bpp"] = info.bpp;
        result["type"] = info.type;
        result["h_flip"] = info.h_flip;
        result["v_flip"] = info.v_flip;
        result["literal_only"] = info.literal_only;
        result["reload_depth"] = info.reload_depth;
        result["reload_palette"] = info.reload_palette;
        ss << std::setw(4) << info.sprdline;
        result["sprdline"] = ss.str(); ss.str("");
        ss << std::setw(4) << info.sprhsiz;
        result["sprhsiz"] = ss.str(); ss.str("");
        ss << std::setw(4) << info.sprvsiz;
        result["sprvsiz"] = ss.str(); ss.str("");
        ss << std::setw(4) << info.stretch;
        result["stretch"] = ss.str(); ss.str("");
        ss << std::setw(4) << info.tilt;
        result["tilt"] = ss.str(); ss.str("");
        result["collision_id"] = info.sprcoll & 0x0F;
        result["collision_disabled"] = IS_SET_BIT(info.sprcoll, 5);
        result["skipped"] = info.skipped;
        result["bbox_x"] = info.bbox_x;
        result["bbox_y"] = info.bbox_y;
        result["hoff"] = info.hoff;
        result["voff"] = info.voff;

        // Include pen_map
        json pen = json::array();
        for (int i = 0; i < 16; i++)
            pen.push_back(info.pen_map[i]);
        result["pen_map"] = pen;

        return result;
    }

    // format == "image"
    if (width <= 0 || height <= 0)
    {
        result["error"] = "Sprite has no rendered data";
        return result;
    }

    unsigned char* png_buffer = NULL;
    int png_size = emu_get_sprite_png(index, &png_buffer);

    if (png_size == 0 || !png_buffer)
    {
        result["error"] = "Failed to capture sprite";
        return result;
    }

    std::string base64_png = base64_encode(png_buffer, png_size);
    free(png_buffer);

    result["__mcp_image"] = true;
    result["data"] = base64_png;
    result["mimeType"] = "image/png";
    result["width"] = width;
    result["height"] = height;

    return result;
}

json DebugAdapter::LoadMedia(const std::string& file_path)
{
    json result;

    if (file_path.empty())
    {
        result["error"] = "File path is required";
        Log("[MCP] LoadMedia failed: File path is required");
        return result;
    }

    gui_load_rom(file_path.c_str());

    if (!m_core || !m_core->GetMedia()->IsReady())
    {
        result["error"] = "Failed to load media file";
        Log("[MCP] LoadMedia failed: %s", file_path.c_str());
        return result;
    }

    result["success"] = true;
    result["file_path"] = file_path;
    result["rom_name"] = m_core->GetMedia()->GetFileName();

    Media::GLYNX_Media_Type type = m_core->GetMedia()->GetType();
    result["media_type"] = (type == Media::MEDIA_HOMEBREW) ? "Homebrew" : "Lynx";

    return result;
}

json DebugAdapter::LoadBios(const std::string& file_path)
{
    json result;

    if (file_path.empty())
    {
        result["error"] = "File path is required";
        Log("[MCP] LoadBios failed: File path is required");
        return result;
    }

    GLYNX_Bios_State bios_result = emu_load_bios(file_path.c_str());

    if (bios_result == BIOS_LOAD_FILE_ERROR)
    {
        result["error"] = "Failed to load BIOS file: file not found";
        Log("[MCP] LoadBios failed: file not found: %s", file_path.c_str());
        return result;
    }
    else if (bios_result == BIOS_LOAD_INVALID_SIZE)
    {
        result["error"] = "Failed to load BIOS file: must be exactly 512 bytes";
        Log("[MCP] LoadBios failed: invalid size: %s", file_path.c_str());
        return result;
    }

    result["success"] = true;
    result["file_path"] = file_path;

    if (bios_result == BIOS_LOAD_INVALID_CRC)
    {
        result["valid_crc"] = false;
        result["warning"] = "CRC does not match original BIOS. Expected md5: fcd403db69f54290b51035d82f835e7b";
    }
    else
        result["valid_crc"] = true;

    return result;
}

json DebugAdapter::LoadSymbols(const std::string& file_path)
{
    json result;

    if (file_path.empty())
    {
        result["error"] = "File path is required";
        Log("[MCP] LoadSymbols failed: File path is required");
        return result;
    }

    gui_debug_load_symbols_file(file_path.c_str());

    result["success"] = true;
    result["file_path"] = file_path;

    return result;
}

json DebugAdapter::ListSaveStateSlots()
{
    json result;
    json slots = json::array();

    for (int i = 0; i < 5; i++)
    {
        json slot;
        slot["slot"] = i + 1;
        slot["selected"] = (config_emulator.save_slot == i);

        if (emu_savestates[i].rom_name[0] != 0)
        {
            slot["rom_name"] = emu_savestates[i].rom_name;
            slot["timestamp"] = emu_savestates[i].timestamp;
            slot["version"] = emu_savestates[i].version;
            slot["valid"] = (emu_savestates[i].version == GLYNX_SAVESTATE_VERSION);
            slot["has_screenshot"] = IsValidPointer(emu_savestates_screenshots[i].data);

            if (emu_savestates[i].emu_build[0] != 0)
                slot["emu_build"] = emu_savestates[i].emu_build;
        }
        else
        {
            slot["empty"] = true;
        }

        slots.push_back(slot);
    }

    result["slots"] = slots;
    result["current_slot"] = config_emulator.save_slot + 1;

    return result;
}

json DebugAdapter::SelectSaveStateSlot(int slot)
{
    json result;

    if (slot < 1 || slot > 5)
    {
        result["error"] = "Invalid slot number (must be 1-5)";
        Log("[MCP] SelectSaveStateSlot failed: Invalid slot %d", slot);
        return result;
    }

    config_emulator.save_slot = slot - 1;

    result["success"] = true;
    result["slot"] = slot;

    return result;
}

json DebugAdapter::SaveState()
{
    json result;

    if (!m_core || !m_core->GetMedia()->IsReady())
    {
        result["error"] = "No media loaded";
        Log("[MCP] SaveState failed: No media loaded");
        return result;
    }

    int slot = config_emulator.save_slot + 1;
    emu_save_state_slot(slot);

    result["success"] = true;
    result["slot"] = slot;
    result["rom_name"] = m_core->GetMedia()->GetFileName();

    return result;
}

json DebugAdapter::LoadState()
{
    json result;

    if (!m_core || !m_core->GetMedia()->IsReady())
    {
        result["error"] = "No media loaded";
        Log("[MCP] LoadState failed: No media loaded");
        return result;
    }

    int slot = config_emulator.save_slot + 1;

    if (emu_savestates[config_emulator.save_slot].rom_name[0] == 0)
    {
        result["error"] = "Save state slot is empty";
        Log("[MCP] LoadState failed: Slot %d is empty", slot);
        return result;
    }

    emu_load_state_slot(slot);

    result["success"] = true;
    result["slot"] = slot;

    return result;
}

json DebugAdapter::SetFastForwardSpeed(int speed)
{
    json result;

    if (speed < 0 || speed > 4)
    {
        result["error"] = "Invalid speed (must be 0-4: 0=1.5x, 1=2x, 2=2.5x, 3=3x, 4=Unlimited)";
        Log("[MCP] SetFastForwardSpeed failed: Invalid speed %d", speed);
        return result;
    }

    config_emulator.ffwd_speed = speed;

    result["success"] = true;
    result["speed"] = speed;
    
    const char* speed_names[] = {"1.5x", "2x", "2.5x", "3x", "Unlimited"};
    result["speed_name"] = speed_names[speed];

    return result;
}

json DebugAdapter::ToggleFastForward(bool enabled)
{
    json result;

    config_emulator.ffwd = enabled;
    gui_action_ffwd();

    result["success"] = true;
    result["enabled"] = enabled;
    result["speed"] = config_emulator.ffwd_speed;

    return result;
}

json DebugAdapter::ControllerButton(const std::string& button, const std::string& action)
{
    json result;

    // Validate action
    if (action != "press" && action != "release" && action != "press_and_release")
    {
        result["error"] = "Invalid action (must be: press, release, press_and_release)";
        return result;
    }

    std::string button_lower = button;
    std::transform(button_lower.begin(), button_lower.end(), button_lower.begin(), ::tolower);

    GLYNX_Keys key = (GLYNX_Keys)0;
    if (button_lower == "a") key = GLYNX_KEY_A;
    else if (button_lower == "b") key = GLYNX_KEY_B;
    else if (button_lower == "option1") key = GLYNX_KEY_OPTION1;
    else if (button_lower == "option2") key = GLYNX_KEY_OPTION2;
    else if (button_lower == "pause") key = GLYNX_KEY_PAUSE;
    else if (button_lower == "up") key = GLYNX_KEY_UP;
    else if (button_lower == "down") key = GLYNX_KEY_DOWN;
    else if (button_lower == "left") key = GLYNX_KEY_LEFT;
    else if (button_lower == "right") key = GLYNX_KEY_RIGHT;
    else
    {
        result["error"] = "Invalid button name (must be: a, b, option1, option2, pause, up, down, left, right)";
        return result;
    }

    if (action == "press")
    {
        emu_key_pressed(key);
    }
    else if (action == "release")
    {
        emu_key_released(key);
    }
    else if (action == "press_and_release")
    {
        emu_key_pressed(key);
        // Mark for delayed release - McpManager will handle releasing after some frames
        result["__delayed_release"] = true;
    }

    result["success"] = true;
    result["button"] = button;
    result["action"] = action;

    return result;
}

// Disassembler operations

json DebugAdapter::RunToAddress(u16 address)
{
    json result;

    if (!m_core || !m_core->GetMedia()->IsReady())
    {
        result["error"] = "No media loaded";
        return result;
    }

    gui_debug_runto_address(address);

    result["success"] = true;
    result["address"] = address;
    result["message"] = "Running to address";

    return result;
}

json DebugAdapter::AddDisassemblerBookmark(u16 address, const std::string& name)
{
    json result;

    if (!m_core || !m_core->GetMedia()->IsReady())
    {
        result["error"] = "No media loaded";
        return result;
    }

    gui_debug_add_disassembler_bookmark(address, name.c_str());

    result["success"] = true;
    result["address"] = address;
    result["name"] = name.empty() ? "auto-generated" : name;

    return result;
}

json DebugAdapter::RemoveDisassemblerBookmark(u16 address)
{
    json result;

    if (!m_core || !m_core->GetMedia()->IsReady())
    {
        result["error"] = "No media loaded";
        return result;
    }

    gui_debug_remove_disassembler_bookmark(address);

    result["success"] = true;
    result["address"] = address;

    return result;
}

json DebugAdapter::AddSymbol(u16 address, const std::string& name)
{
    json result;

    if (!m_core || !m_core->GetMedia()->IsReady())
    {
        result["error"] = "No media loaded";
        return result;
    }

    char symbol[128];
    snprintf(symbol, sizeof(symbol), "%04X %s", address, name.c_str());
    gui_debug_add_symbol(symbol);

    result["success"] = true;
    result["address"] = address;
    result["name"] = name;

    return result;
}

json DebugAdapter::RemoveSymbol(u16 address)
{
    json result;

    if (!m_core || !m_core->GetMedia()->IsReady())
    {
        result["error"] = "No media loaded";
        return result;
    }

    gui_debug_remove_symbol(address);

    result["success"] = true;
    result["address"] = address;

    return result;
}

// Memory editor operations

json DebugAdapter::SelectMemoryRange(int editor, int start_address, int end_address)
{
    json result;

    if (!m_core || !m_core->GetMedia()->IsReady())
    {
        result["error"] = "No media loaded";
        return result;
    }

    if (editor < 0 || editor >= MEMORY_EDITOR_MAX)
    {
        result["error"] = "Invalid editor number";
        return result;
    }

    MemoryAreaInfo info = GetMemoryAreaInfo(editor);
    if (!IsValidPointer(info.data) || info.size == 0)
    {
        result["error"] = "Memory area unavailable";
        return result;
    }

    u32 start_offset = 0;
    u32 end_offset = 0;
    if (!NormalizeMemoryAreaAddress(info, (u32)start_address, &start_offset) ||
        !NormalizeMemoryAreaAddress(info, (u32)end_address, &end_offset))
    {
        result["error"] = "Selection range outside memory area";
        return result;
    }

    if (start_offset > end_offset)
        std::swap(start_offset, end_offset);

    u32 display_start = info.cpu_offset + start_offset;
    u32 display_end = info.cpu_offset + end_offset;

    if (!gui_debug_memory_select_range(editor, (int)display_start, (int)display_end))
    {
        result["error"] = "Unable to apply memory selection";
        return result;
    }

    int actual_start = -1;
    int actual_end = -1;
    gui_debug_memory_get_selection(editor, &actual_start, &actual_end);
    if (actual_start < 0 || actual_end < actual_start || (u32)actual_end >= info.size)
    {
        result["error"] = "Unable to read applied memory selection";
        return result;
    }

    u32 actual_display_start = info.cpu_offset + (u32)actual_start;
    u32 actual_display_end = info.cpu_offset + (u32)actual_end;

    std::ostringstream start_ss, end_ss;
    start_ss << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << actual_display_start;
    end_ss << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << actual_display_end;

    result["success"] = true;
    result["area"] = editor;
    result["start_address"] = start_ss.str();
    result["end_address"] = end_ss.str();

    return result;
}

json DebugAdapter::SetMemorySelectionValue(int editor, u8 value)
{
    json result;

    if (!m_core || !m_core->GetMedia()->IsReady())
    {
        result["error"] = "No media loaded";
        return result;
    }

    if (editor < 0 || editor >= MEMORY_EDITOR_MAX)
    {
        result["error"] = "Invalid editor number";
        return result;
    }

    gui_debug_memory_set_selection_value(editor, value);

    result["success"] = true;
    result["editor"] = editor;
    result["value"] = value;

    return result;
}

json DebugAdapter::AddMemoryBookmark(int editor, int address, const std::string& name)
{
    json result;

    if (!m_core || !m_core->GetMedia()->IsReady())
    {
        result["error"] = "No media loaded";
        return result;
    }

    if (editor < 0 || editor >= MEMORY_EDITOR_MAX)
    {
        result["error"] = "Invalid editor number";
        return result;
    }

    gui_debug_memory_add_bookmark(editor, address, name.c_str());

    result["success"] = true;
    result["editor"] = editor;
    result["address"] = address;
    result["name"] = name.empty() ? "auto-generated" : name;

    return result;
}

json DebugAdapter::RemoveMemoryBookmark(int editor, int address)
{
    json result;

    if (!m_core || !m_core->GetMedia()->IsReady())
    {
        result["error"] = "No media loaded";
        return result;
    }

    if (editor < 0 || editor >= MEMORY_EDITOR_MAX)
    {
        result["error"] = "Invalid editor number";
        return result;
    }

    gui_debug_memory_remove_bookmark(editor, address);

    result["success"] = true;
    result["editor"] = editor;
    result["address"] = address;

    return result;
}

json DebugAdapter::AddMemoryWatch(int editor, int address, const std::string& notes, int size)
{
    json result;

    if (!m_core || !m_core->GetMedia()->IsReady())
    {
        result["error"] = "No media loaded";
        return result;
    }

    if (editor < 0 || editor >= MEMORY_EDITOR_MAX)
    {
        result["error"] = "Invalid editor number";
        return result;
    }

    MemoryAreaInfo info = GetMemoryAreaInfo(editor);
    if (!MemoryAreaContainsDisplayAddress(info, (u32)address))
    {
        result["error"] = "Watch address outside memory area";
        return result;
    }

    if (!gui_debug_memory_add_watch(editor, address, notes.c_str(), size))
    {
        result["error"] = "Unable to add memory watch";
        return result;
    }

    result["success"] = true;
    result["editor"] = editor;
    result["address"] = address;
    result["notes"] = notes;
    result["size"] = size;

    return result;
}

json DebugAdapter::RemoveMemoryWatch(int editor, int address)
{
    json result;

    if (!m_core || !m_core->GetMedia()->IsReady())
    {
        result["error"] = "No media loaded";
        return result;
    }

    if (editor < 0 || editor >= MEMORY_EDITOR_MAX)
    {
        result["error"] = "Invalid editor number";
        return result;
    }

    gui_debug_memory_remove_watch(editor, address);

    result["success"] = true;
    result["editor"] = editor;
    result["address"] = address;

    return result;
}

json DebugAdapter::ListDisassemblerBookmarks()
{
    json result;

    if (!m_core || !m_core->GetMedia()->IsReady())
    {
        result["error"] = "No media loaded";
        return result;
    }

    void* bookmarks_ptr = NULL;
    int count = gui_debug_get_disassembler_bookmarks(&bookmarks_ptr);

    std::vector<DisassemblerBookmark>* bookmarks = (std::vector<DisassemblerBookmark>*)bookmarks_ptr;

    json bookmarks_array = json::array();

    if (bookmarks)
    {
        for (const DisassemblerBookmark& bookmark : *bookmarks)
        {
            json bookmark_obj;

            std::ostringstream addr_ss;
            addr_ss << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << bookmark.address;
            bookmark_obj["address"] = addr_ss.str();
            bookmark_obj["name"] = bookmark.name;

            bookmarks_array.push_back(bookmark_obj);
        }
    }

    result["bookmarks"] = bookmarks_array;
    result["count"] = count;

    return result;
}

json DebugAdapter::ListSymbols()
{
    json result;

    if (!m_core || !m_core->GetMedia()->IsReady())
    {
        result["error"] = "No media loaded";
        return result;
    }

    void* symbols_ptr = NULL;
    gui_debug_get_symbols(&symbols_ptr);

    DebugSymbol** fixed_symbols = (DebugSymbol**)symbols_ptr;

    json symbols_array = json::array();

    if (fixed_symbols)
    {
        for (int address = 0; address < 0x10000; address++)
        {
            if (fixed_symbols[address])
            {
                json symbol_obj;

                std::ostringstream addr_ss;
                addr_ss << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << address;

                symbol_obj["address"] = addr_ss.str();
                symbol_obj["name"] = fixed_symbols[address]->text;

                symbols_array.push_back(symbol_obj);
            }
        }
    }

    result["symbols"] = symbols_array;
    result["count"] = symbols_array.size();

    return result;
}

json DebugAdapter::ListCallStack()
{
    json result;

    if (!m_core || !m_core->GetMedia()->IsReady())
    {
        result["error"] = "No media loaded";
        return result;
    }

    M6502* processor = m_core->GetM6502();
    std::stack<M6502::GLYNX_CallStackEntry> temp_stack = *processor->GetDisassemblerCallStack();

    void* symbols_ptr = NULL;
    gui_debug_get_symbols(&symbols_ptr);
    DebugSymbol** fixed_symbols = (DebugSymbol**)symbols_ptr;

    json stack_array = json::array();

    while (!temp_stack.empty())
    {
        M6502::GLYNX_CallStackEntry entry = temp_stack.top();
        temp_stack.pop();

        json entry_obj;

        // Format addresses as hex strings
        std::ostringstream dest_ss, src_ss, back_ss;
        dest_ss << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << entry.dest;
        src_ss << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << entry.src;
        back_ss << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << entry.back;

        entry_obj["function"] = dest_ss.str();
        entry_obj["source"] = src_ss.str();
        entry_obj["return"] = back_ss.str();

        // Try to find symbol for destination address
        if (fixed_symbols && fixed_symbols[entry.dest])
        {
            entry_obj["symbol"] = fixed_symbols[entry.dest]->text;
        }

        stack_array.push_back(entry_obj);
    }

    result["stack"] = stack_array;
    result["depth"] = stack_array.size();

    return result;
}

json DebugAdapter::ListMemoryBookmarks(int area)
{
    json result;

    if (!m_core || !m_core->GetMedia()->IsReady())
    {
        result["error"] = "No media loaded";
        return result;
    }

    if (area < 0 || area >= MEMORY_EDITOR_MAX)
    {
        result["error"] = "Invalid area number";
        return result;
    }

    void* bookmarks_ptr = NULL;
    int count = gui_debug_memory_get_bookmarks(area, &bookmarks_ptr);

    std::vector<MemEditor::Bookmark>* bookmarks = (std::vector<MemEditor::Bookmark>*)bookmarks_ptr;

    json bookmarks_array = json::array();

    if (bookmarks)
    {
        for (const MemEditor::Bookmark& bookmark : *bookmarks)
        {
            json bookmark_obj;

            std::ostringstream addr_ss;
            addr_ss << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << bookmark.address;
            bookmark_obj["address"] = addr_ss.str();
            bookmark_obj["name"] = bookmark.name;

            bookmarks_array.push_back(bookmark_obj);
        }
    }

    result["area"] = area;
    result["bookmarks"] = bookmarks_array;
    result["count"] = count;

    return result;
}

json DebugAdapter::ListMemoryWatches(int area)
{
    json result;

    if (!m_core || !m_core->GetMedia()->IsReady())
    {
        result["error"] = "No media loaded";
        return result;
    }

    if (area < 0 || area >= MEMORY_EDITOR_MAX)
    {
        result["error"] = "Invalid area number";
        return result;
    }

    void* watches_ptr = NULL;
    int count = gui_debug_memory_get_watches(area, &watches_ptr);

    std::vector<MemEditor::Watch>* watches = (std::vector<MemEditor::Watch>*)watches_ptr;

    json watches_array = json::array();

    if (watches)
    {
        const char* size_names[] = {"8", "16", "24", "32"};
        const char* format_names[] = {"hex", "binary", "decimal_unsigned", "decimal_signed"};

        for (const MemEditor::Watch& watch : *watches)
        {
            json watch_obj;

            std::ostringstream addr_ss;
            addr_ss << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << watch.address;
            watch_obj["address"] = addr_ss.str();
            watch_obj["notes"] = watch.notes;

            int size_idx = (watch.size >= 0 && watch.size <= 3) ? watch.size : 0;
            int fmt_idx = (watch.format >= 0 && watch.format <= 3) ? watch.format : 0;
            watch_obj["size"] = size_names[size_idx];
            watch_obj["format"] = format_names[fmt_idx];

            watches_array.push_back(watch_obj);
        }
    }

    result["area"] = area;
    result["watches"] = watches_array;
    result["count"] = count;

    return result;
}

json DebugAdapter::GetMemorySelection(int area)
{
    json result;

    if (!m_core || !m_core->GetMedia()->IsReady())
    {
        result["error"] = "No media loaded";
        return result;
    }

    if (area < 0 || area >= MEMORY_EDITOR_MAX)
    {
        result["error"] = "Invalid area number";
        return result;
    }

    int start = -1;
    int end = -1;
    gui_debug_memory_get_selection(area, &start, &end);

    result["area"] = area;

    if (start >= 0 && end >= 0 && start <= end)
    {
        std::ostringstream start_ss, end_ss;
        start_ss << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << start;
        end_ss << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << end;

        result["start"] = start_ss.str();
        result["end"] = end_ss.str();
        result["size"] = end - start + 1;
    }
    else
    {
        result["start"] = NULL;
        result["end"] = NULL;
        result["size"] = 0;
        result["note"] = "No selection";
    }

    return result;
}

json DebugAdapter::MemorySearchCapture(int area)
{
    json result;

    if (!m_core || !m_core->GetMedia()->IsReady())
    {
        result["error"] = "No media loaded";
        return result;
    }

    if (area < 0 || area >= MEMORY_EDITOR_MAX)
    {
        result["error"] = "Invalid area number";
        return result;
    }

    gui_debug_memory_search_capture(area);

    result["success"] = true;
    result["area"] = area;
    result["message"] = "Memory snapshot captured";

    return result;
}

json DebugAdapter::MemorySearch(int area, const std::string& op, const std::string& compare_type, int compare_value, const std::string& data_type)
{
    json result;

    if (!m_core || !m_core->GetMedia()->IsReady())
    {
        result["error"] = "No media loaded";
        return result;
    }

    if (area < 0 || area >= MEMORY_EDITOR_MAX)
    {
        result["error"] = "Invalid area number";
        return result;
    }

    int op_index = 0;
    if (op == "<") op_index = 0;
    else if (op == ">") op_index = 1;
    else if (op == "==") op_index = 2;
    else if (op == "!=") op_index = 3;
    else if (op == "<=") op_index = 4;
    else if (op == ">=") op_index = 5;
    else
    {
        result["error"] = "Invalid operator";
        return result;
    }

    int compare_type_index = 0;
    if (compare_type == "previous") compare_type_index = 0;
    else if (compare_type == "value") compare_type_index = 1;
    else if (compare_type == "address") compare_type_index = 2;
    else
    {
        result["error"] = "Invalid compare_type";
        return result;
    }

    if (compare_type_index == 2)
    {
        MemoryAreaInfo info = GetMemoryAreaInfo(area);
        u32 compare_offset = 0;
        if (!NormalizeMemoryAreaAddress(info, (u32)compare_value, &compare_offset))
        {
            result["error"] = "Compare address outside memory area";
            return result;
        }

        compare_value = (int)compare_offset;
    }

    int data_type_index = 0;
    if (data_type == "hex") data_type_index = 0;
    else if (data_type == "signed") data_type_index = 1;
    else if (data_type == "unsigned") data_type_index = 2;
    else
    {
        result["error"] = "Invalid data_type";
        return result;
    }

    void* results_ptr = NULL;
    int count = gui_debug_memory_search(area, op_index, compare_type_index, compare_value, data_type_index, &results_ptr);

    result["area"] = area;
    result["count"] = count;
    result["results"] = json::array();

    if (count > 0 && IsValidPointer(results_ptr))
    {
        std::vector<MemEditor::Search>* results = (std::vector<MemEditor::Search>*)results_ptr;

        int max_results = MIN(count, 1000);

        for (int i = 0; i < max_results; i++)
        {
            MemEditor::Search& search = (*results)[i];
            json item;

            std::ostringstream addr_ss;
            addr_ss << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << search.address;

            item["address"] = addr_ss.str();
            item["value"] = search.value;
            item["previous"] = search.prev_value;

            result["results"].push_back(item);
        }

        if (count > 1000)
        {
            result["note"] = "Results limited to first 1000 matches";
            result["total_matches"] = count;
        }
    }

    return result;
}

json DebugAdapter::MemoryFindBytes(int area, const std::string& hex_bytes)
{
    json result;

    if (!m_core || !m_core->GetMedia()->IsReady())
    {
        result["error"] = "No media loaded";
        return result;
    }

    if (area < 0 || area >= MEMORY_EDITOR_MAX)
    {
        result["error"] = "Invalid area number";
        return result;
    }

    if (hex_bytes.empty())
    {
        result["error"] = "Empty hex byte string";
        return result;
    }

    int addresses[100];
    int count = gui_debug_memory_find_bytes(area, hex_bytes.c_str(), addresses, 100);

    result["area"] = area;
    result["count"] = count;
    result["results"] = json::array();

    int max_results = MIN(count, 100);

    for (int i = 0; i < max_results; i++)
    {
        json item;
        std::ostringstream addr_ss;
        addr_ss << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << addresses[i];
        item["address"] = addr_ss.str();
        result["results"].push_back(item);
    }

    if (count > 100)
    {
        result["note"] = "Results limited to first 100 matches";
        result["total_matches"] = count;
    }

    return result;
}

json DebugAdapter::GetLcdStatus()
{
    Mikey* mikey = m_core->GetMikey();
    Mikey::Mikey_State* mikey_state = mikey->GetState();
    LcdScreen* lcd = mikey->GetLcdScreen();
    LcdScreen::LcdScreen_State* lcd_state = lcd->GetState();

    // Calculate line info
    u8 timer2_counter = mikey_state->timers[2].counter;
    u8 timer2_backup = mikey_state->timers[2].backup;
    int current_line = timer2_backup - timer2_counter;

    json result;
    std::ostringstream ss;
    ss << std::hex << std::uppercase << std::setfill('0');

    // Line Status
    json line_status;
    line_status["line_number"] = current_line;
    ss << std::setw(2) << current_line;
    line_status["line_number_hex"] = ss.str();
    ss.str("");

    // Determine line type: VISIBLE or VBLANK
    // Visualization: Lines 0-2 are VBLANK, Lines 3-104 are VISIBLE 0-101
    bool is_vblank = lcd_state->in_vblank;
    if (current_line >= 3 && current_line <= (timer2_backup + 1))
    {
        // Visible lines 3-104 (VISIBLE 0-101)
        line_status["type"] = "VISIBLE";
        line_status["visible_line"] = current_line - 3;
    }
    else
    {
        // VBLANK lines 0-2
        line_status["type"] = "VBLANK";
        line_status["vblank_line"] = current_line;
    }

    line_status["cycle"] = lcd_state->current_cycle;
    line_status["cycles_per_line"] = lcd_state->line_cycles;
    result["line"] = line_status;

    // Pixel Processing
    json pixel;
    if (!is_vblank)
    {
        bool pixel_active = lcd_state->pixel_count < GLYNX_SCREEN_WIDTH;

        pixel["count"] = lcd_state->pixel_count;
        pixel["total"] = GLYNX_SCREEN_WIDTH;
        pixel["next_at_cycle"] = lcd_state->pixel_next_at;

        if (pixel_active && lcd_state->pixel_next_at > lcd_state->current_cycle)
            pixel["cycles_until_next"] = lcd_state->pixel_next_at - lcd_state->current_cycle;
        else if (pixel_active)
            pixel["cycles_until_next"] = 0;
        else
            pixel["cycles_until_next"] = "N/A";

        // Next pixel info (pen and color)
        if (pixel_active)
        {
            u8 pen = lcd_state->dma_buffer[lcd_state->pixel_buffer_read_pos];
            u16 color = lcd_state->current_palette[pen];
            pixel["next_pen"] = pen;
            ss << std::setw(3) << (color & 0x0FFF);
            pixel["next_color_rgb444"] = ss.str();
            ss.str("");
        }
    }
    else
    {
        pixel["count"] = "N/A (VBLANK)";
        pixel["next_at_cycle"] = "N/A (VBLANK)";
        pixel["cycles_until_next"] = "N/A (VBLANK)";
    }
    result["pixel"] = pixel;

    // Video DMA
    json dma;
    if (!is_vblank)
    {
        bool dma_active = lcd_state->dma_burst_count < k_dma_bursts_per_line;

        ss << std::setw(4) << lcd_state->dma_current_src_addr;
        dma["source_address"] = ss.str();
        ss.str("");
        dma["burst_count"] = lcd_state->dma_burst_count;
        dma["bursts_per_line"] = k_dma_bursts_per_line;
        dma["next_at_cycle"] = lcd_state->dma_next_at;

        if (dma_active && lcd_state->dma_next_at > lcd_state->current_cycle)
            dma["cycles_until_next"] = lcd_state->dma_next_at - lcd_state->current_cycle;
        else if (dma_active)
            dma["cycles_until_next"] = 0;
        else
            dma["cycles_until_next"] = "N/A";
    }
    else
    {
        dma["source_address"] = "N/A (VBLANK)";
        dma["burst_count"] = "N/A (VBLANK)";
        dma["next_at_cycle"] = "N/A (VBLANK)";
        dma["cycles_until_next"] = "N/A (VBLANK)";
    }
    result["dma"] = dma;

    return result;
}

void DebugAdapter::AddRegister(json& registers, std::ostringstream& ss, const char* name, u16 addr, u8 value, u16 filter_address)
{
    if (filter_address != 0xFFFF && filter_address != addr)
        return;

    json reg;
    reg["name"] = name;
    ss << std::setw(4) << addr;
    reg["address"] = ss.str();
    ss.str("");
    ss << std::setw(2) << (int)value;
    reg["value"] = ss.str();
    ss.str("");
    registers.push_back(reg);
}

void DebugAdapter::AddRegister16(json& registers, std::ostringstream& ss, const char* name, u16 addr, u16 value, u16 filter_address)
{
    if (filter_address != 0xFFFF && filter_address != addr && filter_address != (u16)(addr + 1))
        return;

    json reg;
    reg["name"] = name;
    ss << std::setw(4) << addr;
    reg["address"] = ss.str();
    ss.str("");
    ss << std::setw(4) << value;
    reg["value"] = ss.str();
    ss.str("");
    registers.push_back(reg);
}

json DebugAdapter::GetTraceLog(int start, int count)
{
    json result;

    TraceLogger* tl = m_core->GetTraceLogger();
    if (!tl)
    {
        result["error"] = "Trace logger not available";
        return result;
    }

    u32 total = tl->GetCount();

    if (count < 1) count = 100;
    if (count > 1000) count = 1000;

    u32 actual_start;
    if (start < 0)
        actual_start = (total > (u32)count) ? (total - (u32)count) : 0;
    else
        actual_start = (u32)start;

    if (actual_start >= total)
    {
        result["total_entries"] = total;
        result["start"] = actual_start;
        result["count"] = 0;
        result["lines"] = json::array();
        return result;
    }

    u32 actual_count = (u32)count;
    if (actual_start + actual_count > total)
        actual_count = total - actual_start;

    Memory* memory = m_core->GetMemory();

    json lines = json::array();
    for (u32 i = 0; i < actual_count; i++)
    {
        const GLYNX_Trace_Entry& entry = tl->GetEntry(actual_start + i);
        char buf[256];

        switch (entry.type)
        {
            case TRACE_CPU:
            {
                GLYNX_Disassembler_Record* record = memory->GetDisassemblerRecord(entry.cpu.pc);
                char instr[64] = "???";
                char bytes[10] = "";
                if (IsValidPointer(record))
                {
                    snprintf(instr, sizeof(instr), "%s", record->name);
                    char* p = instr;
                    while (*p)
                    {
                        if (*p == '{')
                        {
                            char* end = strchr(p, '}');
                            if (end)
                                memmove(p, end + 1, strlen(end + 1) + 1);
                            else
                                break;
                        }
                        else
                            p++;
                    }
                    snprintf(bytes, sizeof(bytes), "%s", record->bytes);
                }
                u8 p = entry.cpu.p;
                snprintf(buf, sizeof(buf), "%04X  A:%02X X:%02X Y:%02X S:%02X  %c%c-%c%c%c%c%c  %-24s %s",
                         entry.cpu.pc, entry.cpu.a, entry.cpu.x, entry.cpu.y, entry.cpu.s,
                         (p & 0x80) ? 'N' : 'n', (p & 0x40) ? 'V' : 'v',
                         (p & 0x10) ? 'B' : 'b', (p & 0x08) ? 'D' : 'd',
                         (p & 0x04) ? 'I' : 'i', (p & 0x02) ? 'Z' : 'z',
                         (p & 0x01) ? 'C' : 'c', instr, bytes);
                break;
            }
            case TRACE_CPU_IRQ:
                snprintf(buf, sizeof(buf), "  [CPU]  IRQ       PC:$%04X  Vector:$%04X  Mask:%02X",
                         entry.irq.pc, entry.irq.vector, entry.irq.irq_mask);
                break;
            case TRACE_SUZY_MATH:
                if (entry.math.completed)
                    snprintf(buf, sizeof(buf), "  [SUZY] MATH      DONE");
                else if (entry.math.is_divide)
                    snprintf(buf, sizeof(buf), "  [SUZY] DIVIDE    $%04X%04X / $%04X = $%08X  R:$%04X%s",
                             entry.math.op_a, entry.math.op_b & 0xFFFF, entry.math.op_b,
                             entry.math.result, entry.math.remainder,
                             entry.math.div_by_zero ? " [DIV0]" : "");
                else
                    snprintf(buf, sizeof(buf), "  [SUZY] MULTIPLY  $%04X * $%04X = $%08X%s%s",
                             entry.math.op_a, entry.math.op_b, entry.math.result,
                             entry.math.is_signed ? " [SIGN]" : "",
                             entry.math.accumulate ? " [ACC]" : "");
                break;
            case TRACE_SUZY_SPRITE:
                if (entry.sprite.is_start)
                    snprintf(buf, sizeof(buf), "  [SUZY] SPRITES   START  SCB:$%04X", entry.sprite.scb_addr);
                else if (entry.sprite.is_end)
                    snprintf(buf, sizeof(buf), "  [SUZY] SPRITES   END  Cycles:%u", entry.sprite.total_cycles);
                else if (entry.sprite.skipped)
                    snprintf(buf, sizeof(buf), "  [SUZY]  SPRITE   SCB:$%04X  [SKIP]", entry.sprite.scb_addr);
                else
                {
                    static const char* k_types[] = {"BG","BGNC","BSHD","BNDY","NORM","NCOL","XOR","SHDW"};
                    snprintf(buf, sizeof(buf), "  [SUZY]  SPRITE   SCB:$%04X  Next:$%04X  (%d,%d)  %dBPP %s",
                             entry.sprite.scb_addr, entry.sprite.scb_next,
                             entry.sprite.hpos, entry.sprite.vpos,
                             entry.sprite.bpp, k_types[entry.sprite.type & 7]);
                }
                break;
            case TRACE_SUZY_INPUT:
                snprintf(buf, sizeof(buf), "  [SUZY]  INPUT    %s:$%02X",
                         entry.input.is_joystick ? "JOYSTICK" : "SWITCHES", entry.input.value);
                break;
            case TRACE_MIKEY_TIMER:
                snprintf(buf, sizeof(buf), "  [MIKEY] TIMER %d  IRQ  Backup:$%02X",
                         entry.timer.timer_id, entry.timer.backup);
                break;
            case TRACE_MIKEY_UART:
                snprintf(buf, sizeof(buf), "  [MIKEY] UART %s  Data:$%02X%s",
                         entry.uart.is_tx ? "TX" : "RX", entry.uart.data,
                         (!entry.uart.is_tx && entry.uart.flags) ? "  [ERR]" : "");
                break;
            case TRACE_MIKEY_AUDIO:
            {
                static const char* k_audio_regs[] = {"VOL","FDBK","OUT","LFSR","BKUP","CTL","CNT","MISC"};
                snprintf(buf, sizeof(buf), "  [MIKEY] AUDIO %d  %s=$%02X",
                         entry.audio.channel, k_audio_regs[entry.audio.reg & 7], entry.audio.value);
                break;
            }
            case TRACE_CART_SHIFT:
                snprintf(buf, sizeof(buf), "  [CART]  SHIFT    Addr:$%02X  Bit:%d",
                         entry.cart.addr_shift, entry.cart.bit);
                break;
            case TRACE_DEBUG_MESSAGE:
                snprintf(buf, sizeof(buf), "  [DEBUG] %s", entry.debug_msg.text);
                break;
            default:
                snprintf(buf, sizeof(buf), "  [???]");
                break;
        }

        lines.push_back(buf);
    }

    result["total_entries"] = total;
    result["start"] = actual_start;
    result["count"] = actual_count;
    result["lines"] = lines;
    return result;
}

json DebugAdapter::SetTraceLog(bool enabled, u32 flags, bool debug_output)
{
    json result;

    TraceLogger* tl = m_core->GetTraceLogger();
    if (!tl)
    {
        result["error"] = "Trace logger not available";
        return result;
    }

    m_core->GetMikey()->SetDebugOutputEnabled(debug_output);
    result["debug_output"] = debug_output;

    if (enabled)
    {
        if (flags == 0)
            flags = TRACE_FLAG_CPU;
        tl->SetEnabledFlags(flags);

        result["status"] = "started";
        result["enabled_flags"] = flags;

        json enabled_list = json::array();
        if (flags & TRACE_FLAG_CPU) enabled_list.push_back("cpu");
        if (flags & TRACE_FLAG_CPU_IRQ) enabled_list.push_back("cpu_irq");
        if (flags & TRACE_FLAG_SUZY_MATH) enabled_list.push_back("suzy_math");
        if (flags & TRACE_FLAG_SUZY_SPRITE) enabled_list.push_back("suzy_sprites");
        if (flags & TRACE_FLAG_SUZY_INPUT) enabled_list.push_back("suzy_input");
        if (flags & TRACE_FLAG_MIKEY_TIMER) enabled_list.push_back("mikey_timers");
        if (flags & TRACE_FLAG_MIKEY_UART) enabled_list.push_back("mikey_uart");
        if (flags & TRACE_FLAG_MIKEY_AUDIO) enabled_list.push_back("mikey_audio");
        if (flags & TRACE_FLAG_CART_SHIFT) enabled_list.push_back("cart");
        if (flags & TRACE_FLAG_DEBUG_MSG) enabled_list.push_back("debug_messages");
        result["enabled"] = enabled_list;
    }
    else
    {
        tl->SetEnabledFlags(0);
        result["status"] = "stopped";
    }

    result["total_entries"] = tl->GetCount();
    return result;
}

json DebugAdapter::GetRewindStatus()
{
    json result;
    result["enabled"] = config_rewind.enabled;
    result["snapshot_count"] = rewind_get_snapshot_count();
    result["capacity"] = rewind_get_capacity();
    result["frames_per_snapshot"] = rewind_get_frames_per_snapshot();
    result["buffer_seconds"] = config_rewind.buffer_seconds;

    int fps = rewind_get_frames_per_snapshot();
    if (fps < 1) fps = 1;
    result["buffered_seconds"] = (double)(rewind_get_snapshot_count() * fps) / 60.0;

    return result;
}

json DebugAdapter::RewindSeek(int snapshot)
{
    bool paused = emu_is_paused() || emu_is_debug_idle();

    if (!paused)
        return {{"error", "Pause the emulator before seeking the rewind buffer"}};

    int count = rewind_get_snapshot_count();

    if (count == 0)
        return {{"error", "No rewind snapshots available"}};

    if (snapshot < 1 || snapshot > count)
        return {{"error", "Snapshot out of range (1-" + std::to_string(count) + ")"}};

    int age = count - snapshot;

    if (!gui_debug_rewind_seek(age))
        return {{"error", "Failed to load snapshot"}};

    M6502::M6502_State* cpu = m_core->GetM6502()->GetState();
    std::ostringstream ss;
    ss << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << cpu->PC.GetValue();

    int fps = rewind_get_frames_per_snapshot();
    if (fps < 1) fps = 1;

    json result;
    result["success"] = true;
    result["snapshot"] = snapshot;
    result["total"] = count;
    result["age_seconds"] = (double)(age * fps) / 60.0;
    result["pc"] = ss.str();
    return result;
}
