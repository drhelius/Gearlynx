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

#ifndef MCP_DEBUG_ADAPTER_H
#define MCP_DEBUG_ADAPTER_H

#include <vector>
#include <string>
#include "json.hpp"
#include "gearlynx.h"
#include "../gui_debug_memory.h"

using json = nlohmann::json;

struct MemoryAreaInfo
{
    int id;
    std::string name;
    u32 size;
    u32 cpu_offset;
    u8* data;
};

struct BreakpointInfo
{
    bool enabled;
    u16 address1;
    u16 address2;
    bool read;
    bool write;
    bool execute;
    bool range;
};

struct DisasmLine
{
    u16 address;
    bool rom;
    std::string name;
    std::string bytes;
    std::string segment;
    int size;
    bool jump;
    u16 jump_address;
    bool has_operand_address;
    u16 operand_address;
    bool operand_is_zp;
    bool subroutine;
    int irq;
};

class DebugAdapter
{
public:
    DebugAdapter(GearlynxCore* core)
    {
        m_core = core;
    }

    // Execution control
    void Pause();
    void Resume();
    void StepInto();
    void StepOver();
    void StepOut();
    void StepFrame();
    void Reset();
    json GetDebugStatus();
    json RunToAddress(u16 address);

    // Breakpoints
    void SetBreakpoint(u16 address, bool read, bool write, bool execute);
    void SetBreakpointRange(u16 start_address, u16 end_address, bool read, bool write, bool execute);
    void ClearBreakpointByAddress(u16 address, u16 end_address = 0);
    std::vector<BreakpointInfo> ListBreakpoints();
    void SetBreakpointOnIRQ(int irq);
    void ClearBreakpointOnIRQ(int irq);
    u8 GetBreakpointIRQMask();

    // Registers
    void SetRegister(const std::string& name, u32 value);

    // Memory areas (matching debugger memory editor)
    std::vector<MemoryAreaInfo> ListMemoryAreas();
    std::vector<u8> ReadMemoryArea(int area, u32 offset, size_t size);
    void WriteMemoryArea(int area, u32 offset, const std::vector<u8>& data);

    // Disassembly (using existing disassembler records)
    std::vector<DisasmLine> GetDisassembly(u16 start_address, u16 end_address, bool resolve_symbols = false);

    // Chip status info
    json Get6502Status();
    json GetMikeyRegisters(u16 address = 0xFFFF);
    json WriteMikeyRegister(u16 address, u8 value);
    json GetMikeyTimers(int timer = -1);
    json GetMikeyAudio(int channel = -1);
    json GetLcdStatus();
    json GetSuzyRegisters(u16 address = 0xFFFF);
    json WriteSuzyRegister(u16 address, u8 value);
    json GetUARTStatus();
    json GetCartStatus();
    json GetEepromStatus();
    json GetScreenshot();
    json GetFrameBuffer(const std::string& buffer_type);

    // Media and state management
    json GetMediaInfo();
    json LoadMedia(const std::string& file_path);
    json ListSaveStateSlots();
    json SelectSaveStateSlot(int slot);
    json SaveState();
    json LoadState();
    json SetFastForwardSpeed(int speed);
    json ToggleFastForward(bool enabled);

    // Controller input
    json ControllerButton(const std::string& button, const std::string& action);

    // Disassembler operations
    json AddDisassemblerBookmark(u16 address, const std::string& name);
    json RemoveDisassemblerBookmark(u16 address);
    json ListDisassemblerBookmarks();
    json AddSymbol(u16 address, const std::string& name);
    json RemoveSymbol(u16 address);
    json LoadSymbols(const std::string& file_path);
    json ListSymbols();
    json ListCallStack();

    // Memory area operations
    json SelectMemoryRange(int area, int start_address, int end_address);
    json SetMemorySelectionValue(int area, u8 value);
    json GetMemorySelection(int area);
    json AddMemoryBookmark(int area, int address, const std::string& name);
    json RemoveMemoryBookmark(int area, int address);
    json ListMemoryBookmarks(int area);
    json AddMemoryWatch(int area, int address, const std::string& notes, int size);
    json RemoveMemoryWatch(int area, int address);
    json ListMemoryWatches(int area);
    json MemorySearchCapture(int area);
    json MemorySearch(int area, const std::string& op, const std::string& compare_type, int compare_value, const std::string& data_type);

    // Core access
    GearlynxCore* GetCore() { return m_core; }

private:
    GearlynxCore* m_core;

    MemoryAreaInfo GetMemoryAreaInfo(int area);

    // Helper methods for register formatting
    void AddRegister(json& registers, std::ostringstream& ss, const char* name, u16 addr, u8 value, u16 filter_address);
    void AddRegister16(json& registers, std::ostringstream& ss, const char* name, u16 addr, u16 value, u16 filter_address);
};

#endif /* MCP_DEBUG_ADAPTER_H */
