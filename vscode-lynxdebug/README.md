# LynxDebug - Atari Lynx Source-Level Debugger

A Visual Studio Code debugger extension for Atari Lynx games using the [Gearlynx](https://github.com/drhelius/Gearlynx) emulator. Supports C and 6502 assembly source-level debugging for games compiled with [cc65](https://cc65.github.io/).

## Features

- **Source-level debugging** for C and 6502 assembly via cc65 `.dbg` files
- **Symbol file fallback** via `.sym` files for assembly-only projects
- **Breakpoints** mapped from source lines to 6502 addresses
- **Step controls**: step in, step over, step out, step frame, continue, pause
- **Register inspection**: PC, A, X, Y, S, P with individual flag display
- **Local variable inspection** via cc65 scope/csym debug info
- **Global symbols** display
- **Call stack** with source locations
- **Disassembly view** for stepping through code without source
- **Memory inspection** via VSCode's hex editor
- **Hover evaluation** for registers, addresses, and symbols
- **Watch expressions** for registers (`a`, `pc`), addresses (`$FC00`), and symbol names
- **Launch mode**: starts Gearlynx and loads the ROM automatically
- **Attach mode**: connects to a running Gearlynx instance

## Requirements

- [Gearlynx](https://github.com/drhelius/Gearlynx) emulator (with `--debug-monitor` support)
- [cc65](https://cc65.github.io/) toolchain (for compiling Lynx games with debug info)

## Quick Start

1. Install the LynxDebug extension
2. Set the Gearlynx executable path in settings (`lynxDebug.gearlynxPath`)
3. Compile your cc65 Lynx game with debug info:
   ```bash
   cl65 -t lynx -g --dbgfile game.dbg -o game.lnx main.c
   ```
4. Create a `.vscode/launch.json`:
   ```json
   {
     "version": "0.2.0",
     "configurations": [
       {
         "type": "lynx",
         "request": "launch",
         "name": "Debug Lynx Game",
         "rom": "${workspaceFolder}/game.lnx",
         "stopOnEntry": true
       }
     ]
   }
   ```
5. Press F5 to start debugging

The extension auto-detects `.dbg` or `.sym` files next to the ROM.

## Launch Configuration

### Launch (starts Gearlynx)

```json
{
  "type": "lynx",
  "request": "launch",
  "name": "Launch Lynx Game",
  "rom": "${workspaceFolder}/game.lnx",
  "debugFile": "${workspaceFolder}/game.dbg",
  "stopOnEntry": true,
  "port": 6502,
  "gearlynxPath": "/path/to/gearlynx",
  "sourceRoots": ["${workspaceFolder}/src"]
}
```

| Attribute | Required | Description |
|-----------|----------|-------------|
| `rom` | Yes | Path to ROM file (.lnx, .lyx, .o) |
| `debugFile` | No | Path to .dbg or .sym file. Auto-detected if omitted. |
| `stopOnEntry` | No | Pause at entry point (default: false) |
| `port` | No | Debug monitor port (default: 6502) |
| `gearlynxPath` | No | Gearlynx executable path. Uses setting if omitted. |
| `sourceRoots` | No | Extra directories to search for source files |

### Attach (connects to running Gearlynx)

Start Gearlynx with `--debug-monitor`:
```bash
gearlynx --debug-monitor game.lnx
```

```json
{
  "type": "lynx",
  "request": "attach",
  "name": "Attach to Gearlynx",
  "hostname": "localhost",
  "port": 6502,
  "debugFile": "${workspaceFolder}/game.dbg"
}
```

## Settings

| Setting | Default | Description |
|---------|---------|-------------|
| `lynxDebug.gearlynxPath` | `""` | Path to Gearlynx executable |
| `lynxDebug.defaultPort` | `6502` | Default debug monitor TCP port |

## Debug Info Formats

### cc65 .dbg files (recommended)

Produced by `ld65 --dbgfile`. Contains full source mapping for C and assembly, plus symbols, scopes, and local variable info. Enables:
- Source-level breakpoints and stepping
- Local variable inspection
- Function-level call stack
- C and assembly interleaved debugging

### .sym files (fallback)

Simple `ADDRESS LABEL` format. Provides symbol names only (no source-line mapping). Supports multiple formats:
- cc65 `.sym` output
- VICE label format (`al CXXXX .label`)
- Generic `ADDRESS LABEL` and `LABEL = $ADDRESS`

## Watch Expressions

The debugger supports these watch expression formats:

| Expression | Example | Description |
|-----------|---------|-------------|
| Register | `a`, `pc`, `x` | CPU register value |
| Hex address | `$FC00`, `0x0200` | Byte at memory address |
| Symbol | `_main`, `score` | Symbol address lookup |

## Architecture

```
VSCode                          Gearlynx
+-----------------+   TCP/JSON  +------------------+
| LynxDebug ext   |<---------->| Debug Monitor    |
| (DAP adapter)   | port 6502  | Server           |
+-----------------+             +------------------+
                                | Emulator Core    |
                                +------------------+
```

The extension communicates with Gearlynx via a Content-Length framed JSON protocol over TCP. The debug monitor server maps commands to the emulator's existing debug primitives.

## Gearlynx Command Line

```
gearlynx [options] [game.lnx]

  --debug-monitor         Start debug monitor TCP server (default port: 6502)
  --debug-monitor-port N  Set debug monitor port
```

## Building from Source

```bash
cd vscode-lynxdebug
npm install
npm run compile
```

## License

GPL-3.0 - See [LICENSE](../LICENSE) for details.
