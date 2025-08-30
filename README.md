# Gearlynx

[![GitHub Workflow Status](https://img.shields.io/github/actions/workflow/status/drhelius/Gearlynx/gearlynx.yml)](https://github.com/drhelius/Gearlynx/actions/workflows/gearlynx.yml)
[![GitHub Releases)](https://img.shields.io/github/v/tag/drhelius/Gearlynx?label=version)](https://github.com/drhelius/Gearlynx/releases)
[![commits)](https://img.shields.io/github/commit-activity/t/drhelius/Gearlynx)](https://github.com/drhelius/Gearlynx/commits/main)
[![GitHub contributors](https://img.shields.io/github/contributors/drhelius/Gearlynx)](https://github.com/drhelius/Gearlynx/graphs/contributors)
[![GitHub Sponsors](https://img.shields.io/github/sponsors/drhelius)](https://github.com/sponsors/drhelius)
[![License](https://img.shields.io/github/license/drhelius/Gearlynx)](https://github.com/drhelius/Gearlynx/blob/main/LICENSE)
[![Twitter Follow](https://img.shields.io/twitter/follow/drhelius)](https://x.com/drhelius)

> IN DEVELOPMENT: I develop publicly from start to finish. Not intended to be used now, it does not work!

Gearlynx is a cross-platform Atari Lynx emulator written in C++ that runs on Windows, macOS, Linux, BSD and RetroArch.

This is an open source project with its ongoing development made possible thanks to the support by these awesome [backers](backers.md). If you find it useful, please consider [sponsoring](https://github.com/sponsors/drhelius).

Don't hesitate to report bugs or ask for new features by [opening an issue](https://github.com/drhelius/Gearlynx/issues).

<img src="http://www.geardome.com/files/gearlynx/gearlynx_debug_05.png">

## Downloads

- **Dev Builds**: [GitHub Actions](https://github.com/drhelius/Gearlynx/actions/workflows/gearlynx.yml)
  
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
