---
name: gearlynx-romhacking
description: >-
  Hack, modify, and translate Atari Lynx ROMs using the Gearlynx emulator MCP
  server. Provides workflows for memory searching, value discovery, cheat
  creation, data modification, sprite/text finding, and translation patching.
  Use when the user wants to create cheats, find game values in memory, modify
  ROM data, translate a Lynx game, patch game behavior, create ROM hacks,
  discover hidden content, change sprites or graphics, find text strings, apply
  Game Genie style codes, do infinite lives or health hacks, search for score
  or item counters, or reverse engineer data structures in Atari Lynx games.
  Also use for any ROM hacking, memory poking, or game modification task
  involving Gearlynx.
compatibility: >-
  Requires the Gearlynx MCP server. Before installing or configuring, call
  debug_get_status to check if the server is already connected. If it responds,
  the server is ready — skip setup entirely.
metadata:
  author: drhelius
  version: "1.0"
---

# Atari Lynx ROM Hacking with Gearlynx

## Overview

Hack, modify, and translate Atari Lynx ROMs using the Gearlynx emulator as an MCP server. Search memory for game variables, create cheats, find text strings for translation, locate sprite data, and reverse engineer data structures — all through MCP tool calls. Use save states as checkpoints and fast forward to reach specific game states.

## MCP Server Prerequisite

**IMPORTANT — Check before installing:** Before attempting any installation or configuration, you MUST first verify if the Gearlynx MCP server is already connected in your current session. Call `debug_get_status` — if it returns a valid response, the server is active and ready.

Only if the tool is not available or the call fails, you need to help install and configure the Gearlynx MCP server:

### Installing Gearlynx

Run the bundled install script (macOS/Linux):

```bash
bash scripts/install.sh
```

This installs Gearlynx via Homebrew on macOS or downloads the latest release on Linux. It prints the binary path on completion. You can also set `GEARLYNX_INSTALL_DIR` to control where the binary goes (default: `~/.local/bin`).

Alternatively, download from [GitHub Releases](https://github.com/drhelius/Gearlynx/releases/latest) or install with `brew install --cask drhelius/geardome/gearlynx` on macOS.

### Connecting as MCP Server

Configure your AI client to run Gearlynx as an MCP server via STDIO transport. Example for Claude Desktop (`~/Library/Application Support/Claude/claude_desktop_config.json`):
```json
{
  "mcpServers": {
    "gearlynx": {
      "command": "/path/to/gearlynx",
      "args": ["--mcp-stdio"]
    }
  }
}
```
Replace `/path/to/gearlynx` with the actual binary path from the install script. Add `--headless` before `--mcp-stdio` on headless machines.

### Hardware Documentation (References)

Atari Lynx hardware documentation is available in the [references/](references/) directory. Load them into your context when you need data formats, memory layout, or hardware details.

| Reference | File | Load when... |
|---|---|---|
| General Overview | [references/lynx1_general_overview.md](references/lynx1_general_overview.md) | System specs, feature set, overall architecture |
| Hardware Overview | [references/lynx2_hardware_overview.md](references/lynx2_hardware_overview.md) | System block diagram: CPU, Mikey, Suzy, RAM, cart |
| Hardware Quirks | [references/lynx3_software_hardware_perniciousness.md](references/lynx3_software_hardware_perniciousness.md) | Unsafe register operations, hardware gotchas to avoid when patching |
| CPU/ROM | [references/lynx4_cpu_rom.md](references/lynx4_cpu_rom.md) | 65C02 cycle timing, CPU sleep, ROM mapping, opcode reference |
| Display | [references/lynx5_display.md](references/lynx5_display.md) | Frame rate, pen/palette colors, display buffer addresses |
| Sprite/Collision | [references/lynx6_sprite_collision.md](references/lynx6_sprite_collision.md) | SCB format, sprite types, collision depository, sprite data layout |
| Audio/ROM Cart | [references/lynx7_audio_tape_romcart.md](references/lynx7_audio_tape_romcart.md) | 4 audio channels, ROM cart banking (BANK0/BANK1, AUDIN) |
| Timers/Interrupts | [references/lynx8_timers_interrupts.md](references/lynx8_timers_interrupts.md) | 8 timer channels, linking, reload, IRQ sources |
| UART | [references/lynx8a_uart.md](references/lynx8a_uart.md) | ComLynx serial port, UART registers, baud rate |
| Other Hardware | [references/lynx9_other_hardware.md](references/lynx9_other_hardware.md) | Hardware multiply/divide, parallel port |
| System Reset | [references/lynx10_system_reset.md](references/lynx10_system_reset.md) | Reset/power-up sequence, savegame EEPROM, NVRAM |
| System Bus Interplay | [references/lynx11_system_bus_interplay.md](references/lynx11_system_bus_interplay.md) | Bus masters, page mode, DMA timing |
| Hardware Addresses | [references/hardware_addresses.md](references/hardware_addresses.md) | 64K memory map, Mikey/Suzy register addresses ($FC00-$FCFF, $FD00-$FDFF) |
| Interrupts & CPU Sleep | [references/irq_interrupts_cpu_sleep.md](references/irq_interrupts_cpu_sleep.md) | IRQ vector, interrupt enable/status bits, CPU sleep (JAM) |
| Sprite Engine | [references/sprite_engine.md](references/sprite_engine.md) | Display init, double-buffering, VIDBAS/COLLBAS macros |

---

## Core Technique: Memory Search

Memory search is the primary tool for ROM hacking. It uses a capture → change → compare cycle to isolate memory addresses holding game values.

### The Search Loop

```
1. memory_search_capture    → snapshot current memory state
2. (change the value in-game using controller_button, fast forward, etc.)
3. memory_search            → compare against snapshot to find changed addresses
4. Repeat 2-3 until only a few candidates remain
5. read_memory / write_memory → verify and modify the found addresses
```

### Search Operators and Types

`memory_search` supports these **operators**: `<`, `>`, `==`, `!=`, `<=`, `>=`

**Compare types**:
- `previous` — compare current value to last captured snapshot (most common)
- `value` — compare current value to a specific number
- `address` — compare current value to value at another address

**Data types**: `hex`, `signed`, `unsigned`

### Example: Finding the Lives Counter

```
1. memory_search_capture                         → snapshot with 3 lives
2. Lose a life in-game (play or use controller_button)
3. memory_search (operator: <, compare: previous) → values that decreased
4. memory_search_capture                         → snapshot with 2 lives
5. Lose another life
6. memory_search (operator: <, compare: previous) → narrow further
7. Or use: memory_search (operator: ==, compare: value, value: 1)
   → find addresses holding exactly 1
8. write_memory on the candidate address to set lives to 99
9. get_screenshot to verify the change took effect
```

### Example: Finding a Score Counter

Score values are often stored as multi-byte (16-bit little-endian on 6502):

```
1. memory_search_capture                                → snapshot at score 0
2. Score some points in-game
3. memory_search (operator: >, compare: previous)       → values that increased
4. memory_search_capture
5. Score more points
6. memory_search (operator: >, compare: previous)       → narrow down
7. read_memory on candidates — look for values matching current score
8. write_memory to set a custom score
```

For 16-bit values: the low byte is at address N, high byte at N+1 (6502 is little-endian).

---

## Fast Forward for Efficiency

Use fast forward to speed through gameplay when you need to trigger in-game changes:

```
set_fast_forward_speed (4 = unlimited)
toggle_fast_forward              → enable
(play through the game section)
toggle_fast_forward              → disable
```

This is essential when you need to reach specific game states without waiting in real-time.

---

## Save States as Checkpoints

Save states are critical for ROM hacking — they let you save your position and retry modifications:

```
select_save_state_slot (1-5)     → pick a slot
save_state                       → save current state
(try modifications)
load_state                       → revert if something breaks
```

Use different slots for different game states (e.g., slot 1 = start, slot 2 = boss fight, slot 3 = specific level).

`list_save_state_slots` shows all slots with ROM name, timestamp, and screenshot availability.

---

## Finding and Modifying Game Data

### Text and String Discovery

To find text strings for translation or modification:

1. Determine the character encoding — Lynx games often use custom character maps, not ASCII
2. `read_memory` across ROM banks (Bank 0, Bank 0A, Bank 1, Bank 1A) scanning for known byte patterns
3. Use `memory_find_bytes` to search for specific byte sequences across memory
4. Set read breakpoints on suspected text addresses with `set_breakpoint` (type: read) to confirm they're used for rendering
5. `get_screenshot` to correlate displayed text with memory contents

### Sprite and Graphics Data

1. `get_frame_buffer` to capture the current visual state (VIDBAS or DISPADR)
2. `get_suzy_registers` to find sprite control block (SCB) addresses, VIDBAS pointer
3. Set read breakpoints on sprite data addresses to find the rendering code
4. `read_memory` on the sprite data area to analyze the packed pixel format
5. `get_screenshot` before/after modifications to see visual changes

Reference the Sprite/Collision hardware docs ([references/lynx6_sprite_collision.md](references/lynx6_sprite_collision.md)) for SCB structure details.

### Data Tables and Structures

1. `debug_pause` → `get_disassembly` around code that loads data
2. Look for LDA/LDX/LDY instructions with absolute or indexed addressing — these point to data tables
3. `read_memory` at the target addresses to dump the table contents
4. `add_memory_bookmark` to mark discovered data regions
5. `add_symbol` to label data table entry points for future reference

---

## Creating Cheats

### Infinite Lives / Health

```
1. Find the address using the search loop (above)
2. Set a write breakpoint: set_breakpoint (type: write) on the address
3. debug_continue → when it hits, get_disassembly to see the decrement code
4. Note the instruction (e.g., DEC $0042 or STA $0042)
5. Option A: Periodically write_memory to reset the value (simple poke cheat)
6. Option B: Identify the decrement routine for a NOP patch
```

### Watching Values in Real-Time

Use `add_memory_watch` on discovered addresses. Watches appear in the emulator's GUI memory editor, letting you monitor values as the game runs — useful for verifying cheats work across different game situations.

### Write Breakpoint Technique

The most powerful cheat-finding technique:

1. Find the variable address via memory search
2. `set_breakpoint` (type: write) on that address
3. `debug_continue` — the emulator stops when the game writes to that address
4. `get_6502_status` + `get_disassembly` reveals the exact code modifying the value
5. `get_call_stack` shows what triggered the write
6. You now know exactly where and how the game manages that variable

---

## Translation Workflow

### 1. Identify the Font System

1. `get_screenshot` of a screen with text
2. Find text rendering code by setting read breakpoints on display buffer areas
3. Trace back to find the character mapping table
4. `read_memory` to dump the font/character table
5. `add_symbol` to label the font table and rendering routine

### 2. Find String Data

1. Look for sequential text bytes in ROM banks using `read_memory` with large ranges
2. Use `memory_find_bytes` to search for known byte patterns
3. Cross-reference with the character table to decode strings
4. `add_memory_bookmark` to mark each string location

### 3. Measure Space Constraints

ROM hacking translations must fit within existing space:

1. `read_memory` to determine how much space each string occupies
2. Check for string terminators (commonly $00, $FF, or length-prefixed)
3. If the translation is longer, look for unused ROM space or abbreviate

### 4. Apply and Test

1. `write_memory` to patch translated strings into memory
2. `get_screenshot` to verify rendering
3. `save_state` before each change so you can `load_state` if it breaks
4. Test all screens that display modified text

---

## Memory Map Quick Reference

Use `list_memory_areas` to get the full list. Common areas:

| Area | CPU Address | Use |
|---|---|---|
| Zero Page | $0000-$00FF | Fast variables, counters, game state |
| Stack | $0100-$01FF | 6502 stack |
| RAM | $0000-$FFFF | Main working memory |
| Bank 0 | Varies | ROM bank 0 |
| Bank 0A | Varies | ROM bank 0A |
| Bank 1 | Varies | ROM bank 1 |
| Bank 1A | Varies | ROM bank 1A |
| EEPROM | Varies | Save data, high scores |

Zero Page is the most common location for game variables (lives, health, score, position) because 6502 zero page addressing is faster.

---

## Bookmarks and Organization

Keep your hacking session organized:

- `add_memory_bookmark` — mark discovered data regions, variable locations, string tables
- `add_memory_watch` — track values that change during gameplay
- `add_symbol` — label addresses in disassembly for readability
- `add_disassembler_bookmark` — mark code routines you've identified

Use `list_memory_bookmarks`, `list_memory_watches`, `list_symbols`, `list_disassembler_bookmarks` to review.

---

## Persisting Changes

Changes made via `write_memory` to ROM bank areas are applied to the emulator's in-memory copy only — they are **not** persisted to the ROM file on disk. To create a permanent patch, use command-line tools (e.g., a binary patch script) to apply the discovered modifications to the actual ROM file.
