# Gearlynx

[![GitHub Workflow Status](https://img.shields.io/github/actions/workflow/status/drhelius/Gearlynx/gearlynx.yml)](https://github.com/drhelius/Gearlynx/actions/workflows/gearlynx.yml)
[![GitHub Releases)](https://img.shields.io/github/v/tag/drhelius/Gearlynx?label=version)](https://github.com/drhelius/Gearlynx/releases)
[![commits)](https://img.shields.io/github/commit-activity/t/drhelius/Gearlynx)](https://github.com/drhelius/Gearlynx/commits/main)
[![GitHub contributors](https://img.shields.io/github/contributors/drhelius/Gearlynx)](https://github.com/drhelius/Gearlynx/graphs/contributors)
[![GitHub Sponsors](https://img.shields.io/github/sponsors/drhelius)](https://github.com/sponsors/drhelius)
[![License](https://img.shields.io/github/license/drhelius/Gearlynx)](https://github.com/drhelius/Gearlynx/blob/main/LICENSE)
[![Twitter Follow](https://img.shields.io/twitter/follow/drhelius)](https://x.com/drhelius)

Gearlynx is a very accurate cross-platform Atari Lynx emulator written in C++ that runs on Windows, macOS, Linux, BSD and RetroArch, with an embedded MCP server for debugging and tooling.

This is an open source project with its ongoing development made possible thanks to the support by these awesome [backers](backers.md). If you find it useful, please consider [sponsoring](https://github.com/sponsors/drhelius).

Don't hesitate to report bugs or ask for new features by [opening an issue](https://github.com/drhelius/Gearlynx/issues).

<img src="http://www.geardome.com/files/gearlynx/gearlynx_debug_07.png">

## Downloads

<table>
  <thead>
    <tr>
      <th>Platform</th>
      <th>Architecture</th>
      <th>Download Link</th>
      <th>Notes</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td rowspan="4"><strong>Windows</strong></td>
      <td>Desktop x64</td>
      <td><a href="https://github.com/drhelius/Gearlynx/releases/download/1.0.0/Gearlynx-1.0.0-desktop-windows-x64.zip">Gearlynx-1.0.0-desktop-windows-x64.zip</a></td>
      <td rowspan="4">May need <a href="https://go.microsoft.com/fwlink/?LinkId=746572">Visual C++ Redistributable</a> and <a href="https://apps.microsoft.com/detail/9nqpsl29bfff">OpenGL Compatibility Pack</a></td>
    </tr>
    <tr>
      <td>Desktop ARM64</td>
      <td><a href="https://github.com/drhelius/Gearlynx/releases/download/1.0.0/Gearlynx-1.0.0-desktop-windows-arm64.zip">Gearlynx-1.0.0-desktop-windows-arm64.zip</a></td>
    </tr>
    <tr>
      <td>MCP x64</td>
      <td><a href="https://github.com/drhelius/Gearlynx/releases/download/1.0.0/Gearlynx-1.0.0-mcpb-windows-x64.mcpb">Gearlynx-1.0.0-mcpb-windows-x64.mcpb</a></td>
    </tr>
    <tr>
      <td>MCP ARM64</td>
      <td><a href="https://github.com/drhelius/Gearlynx/releases/download/1.0.0/Gearlynx-1.0.0-mcpb-windows-arm64.mcpb">Gearlynx-1.0.0-mcpb-windows-arm64.mcpb</a></td>
    </tr>
    <tr>
      <td rowspan="4"><strong>macOS</strong></td>
      <td>Desktop Apple Silicon</td>
      <td><a href="https://github.com/drhelius/Gearlynx/releases/download/1.0.0/Gearlynx-1.0.0-desktop-macos-arm64.zip">Gearlynx-1.0.0-desktop-macos-arm64.zip</a></td>
      <td rowspan="4"></td>
    </tr>
    <tr>
      <td>Desktop Intel</td>
      <td><a href="https://github.com/drhelius/Gearlynx/releases/download/1.0.0/Gearlynx-1.0.0-desktop-macos-intel.zip">Gearlynx-1.0.0-desktop-macos-intel.zip</a></td>
    </tr>
    <tr>
      <td>MCP ARM64</td>
      <td><a href="https://github.com/drhelius/Gearlynx/releases/download/1.0.0/Gearlynx-1.0.0-mcpb-macos-arm64.mcpb">Gearlynx-1.0.0-mcpb-macos-arm64.mcpb</a></td>
    </tr>
    <tr>
      <td>MCP x64</td>
      <td><a href="https://github.com/drhelius/Gearlynx/releases/download/1.0.0/Gearlynx-1.0.0-mcpb-macos-x64.mcpb">Gearlynx-1.0.0-mcpb-macos-x64.mcpb</a></td>
    </tr>
    <tr>
      <td rowspan="3"><strong>Linux</strong></td>
      <td>Desktop Ubuntu 24.04</td>
      <td><a href="https://github.com/drhelius/Gearlynx/releases/download/1.0.0/Gearlynx-1.0.0-desktop-ubuntu24.04-x64.zip">Gearlynx-1.0.0-desktop-ubuntu24.04-x64.zip</a></td>
      <td rowspan="3">May need <code>libsdl2</code></td>
    </tr>
    <tr>
      <td>Desktop Ubuntu 22.04</td>
      <td><a href="https://github.com/drhelius/Gearlynx/releases/download/1.0.0/Gearlynx-1.0.0-desktop-ubuntu22.04-x64.zip">Gearlynx-1.0.0-desktop-ubuntu22.04-x64.zip</a></td>
    </tr>
    <tr>
      <td>MCP x64</td>
      <td><a href="https://github.com/drhelius/Gearlynx/releases/download/1.0.0/Gearlynx-1.0.0-mcpb-linux-x64.mcpb">Gearlynx-1.0.0-mcpb-linux-x64.mcpb</a></td>
    </tr>
    <tr>
      <td><strong>RetroArch</strong></td>
      <td>All platforms</td>
      <td><a href="https://docs.libretro.com/library/gearlynx/">Libretro core documentation</a></td>
      <td></td>
    </tr>
    <tr>
      <td><strong>Dev Builds</strong></td>
      <td>All platforms</td>
      <td><a href="https://github.com/drhelius/Gearlynx/actions/workflows/gearlynx.yml">GitHub Actions</a></td>
      <td>Latest development builds</td>
    </tr>
  </tbody>
</table>

## Features

- Accurate emulation supporting the entire commercial Atari Lynx catalog.
- Very accurate 65C02 core.
- Compressed and hombrew rom support (lnx, lyx, o, and zip).
- Save states with preview.
- Configurable low-pass audio filter.
- VGM recorder.
- Internal database for automatic rom detection and hardware selection if `Auto` options are selected.
- Supported platforms (standalone): Windows, Linux, BSD and macOS.
- Supported platforms (libretro): Windows, Linux, macOS, Raspberry Pi, Android, iOS, tvOS, PlayStation Vita, PlayStation 3, Nintendo 3DS, Nintendo GameCube, Nintendo Wii, Nintendo WiiU, Nintendo Switch, Emscripten, Classic Mini systems (NES, SNES, C64, ...), OpenDingux, RetroFW and QNX.
- Full debugger with just-in-time disassembler that can handle self-modifying code, CPU and memory breakpoints, code navigation (goto address, JP JR and JSR double clicking), debug symbols, automatic labels, memory editor, memory search, Suzy and Mikey register viewer, audio channels and video inspector.
- MCP server for AI-assisted debugging with GitHub Copilot, Claude, ChatGPT and similar, exposing tools for execution control, memory inspection, hardware status, and more.
- Windows and Linux *Portable Mode*.
- ROM loading from the command line by adding the ROM path as an argument.
- ROM loading using drag & drop.
- Support for modern game controllers through [gamecontrollerdb.txt](https://github.com/mdqinc/SDL_GameControllerDB) file located in the same directory as the application binary.

## Tips

### Basic Usage
- **BIOS**: Gearlynx requires a BIOS to work. It is possible to load any BIOS but the original with md5 ```fcd403db69f54290b51035d82f835e7b``` is recommended.
- **Mouse Cursor**: Automatically hides when hovering over the main output window or when Main Menu is disabled.
- **Portable Mode**: Create an empty file named `portable.ini` in the same directory as the application binary to enable portable mode.

### Debugging Features
- **Docking Windows**: In debug mode, you can dock windows together by pressing SHIFT and dragging a window onto another.
- **Multi-viewport**: In Windows or macOS, you can enable "multi-viewport" in the debug menu. You must restart the emulator for the change to take effect. Once enabled, you can drag debugger windows outside the main window.
- **Debug Symbols**: The emulator automatically tries to load a symbol (.sym) file when loading a ROM. For example, for ```path_to_rom_file.rom``` it tries to load ```path_to_rom_file.sym```. You can also load symbol files using the GUI or the CLI. It supports *cc65* (VICE label file), *lyxass* (EQU) and *mads* (lab and hea) file formats.

### Command Line Usage
```
gearlynx [options] [game_file] [symbol_file]

Arguments:
  [game_file]              Game file: accepts ROMs (.pce, .sgx, .hes), CUE (.cue) or ZIP (.zip)
  [symbol_file]            Optional symbol file for debugging

Options:
  -f, --fullscreen         Start in fullscreen mode
  -w, --windowed           Start in windowed mode with menu visible
      --mcp-stdio          Auto-start MCP server with stdio transport
      --mcp-http           Auto-start MCP server with HTTP transport
      --mcp-http-port N    HTTP port for MCP server (default: 7777)
  -v, --version            Display version information
  -h, --help               Display this help message
```

### MCP Server

Gearlynx includes a [Model Context Protocol](https://modelcontextprotocol.io/introduction) (MCP) server that enables AI-assisted debugging through AI agents like GitHub Copilot, Claude, ChatGPT and similar. The server provides tools for execution control, memory inspection, breakpoints, disassembly, hardware status, sprite viewing, and more.

For complete setup instructions and tool documentation, see [MCP_README.md](MCP_README.md).

## Hardware Tests
- **Atari Lynx Hardware Test ROMs**: [https://github.com/drhelius/lynx-tests](https://github.com/drhelius/lynx-tests).

## Build Instructions

### Windows

- Install Microsoft Visual Studio Community 2022 or later.
- Open the Gearlynx Visual Studio solution `platforms/windows/Gearlynx.sln` and build.

### macOS

- Install Xcode and run `xcode-select --install` in the terminal for the compiler to be available on the command line.
- Run these commands to generate a Mac *app* bundle:

``` shell
brew install sdl2
cd platforms/macos
make dist
```

### Linux

- Ubuntu / Debian / Raspberry Pi (Raspbian):

``` shell
sudo apt install build-essential libsdl2-dev libgtk-3-dev
cd platforms/linux
make
```

- Fedora:

``` shell
sudo dnf install @development-tools gcc-c++ SDL2-devel gtk3-devel
cd platforms/linux
make
```

- Arch Linux:

``` shell
sudo pacman -S base-devel sdl2 gtk3
cd platforms/linux
make
```

### BSD

- FreeBSD:

``` shell
su root -c "pkg install -y git gmake pkgconf SDL2 lang/gcc gtk3"
cd platforms/bsd
gmake
```

- NetBSD:

``` shell
su root -c "pkgin install gmake pkgconf SDL2 lang/gcc gtk3"
cd platforms/bsd
gmake
```

### Libretro

- Ubuntu / Debian / Raspberry Pi (Raspbian):

``` shell
sudo apt install build-essential
cd platforms/libretro
make
```

- Fedora:

``` shell
sudo dnf install @development-tools gcc-c++
cd platforms/libretro
make
```

## Contributors

Thank you to all the people who have already contributed to Gearlynx!

[![Contributors](https://contrib.rocks/image?repo=drhelius/gearlynx)](https://github.com/drhelius/gearlynx/graphs/contributors)

## License

Gearlynx is licensed under the GNU General Public License v3.0 License, see [LICENSE](LICENSE) for more information.
