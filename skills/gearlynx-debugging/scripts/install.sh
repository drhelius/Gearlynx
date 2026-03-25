#!/usr/bin/env bash
# Install Gearlynx emulator for use as an MCP server.
# Usage: bash scripts/install.sh
#
# This script installs Gearlynx and prints the binary path.
# It supports macOS (Homebrew) and Linux (GitHub releases).

set -euo pipefail

REPO="drhelius/Gearlynx"
INSTALL_DIR="${GEARLYNX_INSTALL_DIR:-$HOME/.local/bin}"

check_existing() {
    if command -v gearlynx &>/dev/null; then
        echo "gearlynx is already installed: $(command -v gearlynx)"
        exit 0
    fi
    if [[ -x "$INSTALL_DIR/gearlynx" ]]; then
        echo "gearlynx is already installed: $INSTALL_DIR/gearlynx"
        exit 0
    fi
}

install_macos() {
    if command -v brew &>/dev/null; then
        echo "Installing Gearlynx via Homebrew..."
        brew install --cask drhelius/geardome/gearlynx
        local app_path="/Applications/gearlynx.app/Contents/MacOS/gearlynx"
        if [[ -x "$app_path" ]]; then
            echo "$app_path"
            return
        fi
        echo "Installed via Homebrew. Run: brew info drhelius/geardome/gearlynx to find the binary path."
        return
    fi

    echo "Homebrew not found, downloading from GitHub..."
    local arch
    arch=$(uname -m)
    local suffix
    case "$arch" in
        arm64) suffix="arm64" ;;
        x86_64) suffix="intel" ;;
        *)
            echo "Unsupported architecture: $arch"
            echo "Download manually from: https://github.com/$REPO/releases/latest"
            exit 1
            ;;
    esac

    echo "Fetching latest release info..."
    local tag
    tag=$(curl -fsSL "https://api.github.com/repos/$REPO/releases/latest" | grep '"tag_name"' | head -1 | sed 's/.*"tag_name": *"\([^"]*\)".*/\1/')

    if [[ -z "$tag" ]]; then
        echo "Failed to fetch latest release. Download manually from: https://github.com/$REPO/releases/latest"
        exit 1
    fi

    local asset="Gearlynx-${tag}-desktop-macos-${suffix}.zip"
    local url="https://github.com/$REPO/releases/download/${tag}/${asset}"

    echo "Downloading $asset..."
    local tmpdir
    tmpdir=$(mktemp -d)
    trap 'rm -rf "$tmpdir"' EXIT

    curl -fsSL -o "$tmpdir/gearlynx.zip" "$url"
    unzip -q "$tmpdir/gearlynx.zip" -d "$tmpdir/gearlynx"

    local app
    app=$(find "$tmpdir/gearlynx" -name "gearlynx.app" -type d | head -1)
    if [[ -n "$app" ]]; then
        cp -R "$app" /Applications/
        local bin="/Applications/gearlynx.app/Contents/MacOS/gearlynx"
        if [[ -x "$bin" ]]; then
            echo "$bin"
            return
        fi
    fi

    echo "Binary not found in archive. Check: https://github.com/$REPO/releases/latest"
    exit 1
}

install_linux() {
    # Try PPA first (Ubuntu/Debian)
    if command -v apt-get &>/dev/null; then
        echo "Installing Gearlynx via PPA..."
        sudo mkdir -p /etc/apt/keyrings
        curl -fsSL "https://raw.githubusercontent.com/drhelius/ppa-geardome/main/KEY.gpg" | sudo gpg --dearmor -o /etc/apt/keyrings/ppa-geardome.gpg 2>/dev/null || true
        echo "deb [signed-by=/etc/apt/keyrings/ppa-geardome.gpg] https://raw.githubusercontent.com/drhelius/ppa-geardome/main ./" | sudo tee /etc/apt/sources.list.d/ppa-geardome.list > /dev/null
        sudo apt-get update -qq
        if sudo apt-get install -y gearlynx; then
            echo "$(command -v gearlynx)"
            return
        fi
        echo "PPA install failed, falling back to GitHub release..."
    fi

    local arch
    arch=$(uname -m)
    local suffix
    case "$arch" in
        x86_64)  suffix="x64" ;;
        aarch64) suffix="arm64" ;;
        *)
            echo "Unsupported architecture: $arch"
            echo "Download manually from: https://github.com/$REPO/releases/latest"
            exit 1
            ;;
    esac

    # Detect Ubuntu version or default to 24.04
    local ubuntu_ver="24.04"
    if [[ -f /etc/os-release ]]; then
        local ver
        ver=$(grep VERSION_ID /etc/os-release | cut -d'"' -f2 2>/dev/null || echo "")
        case "$ver" in
            22.04) ubuntu_ver="22.04" ;;
            24.04) ubuntu_ver="24.04" ;;
        esac
    fi

    echo "Fetching latest release info..."
    local tag
    tag=$(curl -fsSL "https://api.github.com/repos/$REPO/releases/latest" | grep '"tag_name"' | head -1 | sed 's/.*"tag_name": *"\([^"]*\)".*/\1/')

    if [[ -z "$tag" ]]; then
        echo "Failed to fetch latest release. Download manually from: https://github.com/$REPO/releases/latest"
        exit 1
    fi

    local asset="Gearlynx-${tag}-desktop-ubuntu${ubuntu_ver}-${suffix}.zip"
    local url="https://github.com/$REPO/releases/download/${tag}/${asset}"

    echo "Downloading $asset..."
    local tmpdir
    tmpdir=$(mktemp -d)
    trap 'rm -rf "$tmpdir"' EXIT

    curl -fsSL -o "$tmpdir/gearlynx.zip" "$url"
    unzip -q "$tmpdir/gearlynx.zip" -d "$tmpdir/gearlynx"

    mkdir -p "$INSTALL_DIR"
    local bin
    bin=$(find "$tmpdir/gearlynx" -name "gearlynx" -type f | head -1)
    if [[ -z "$bin" ]]; then
        echo "Binary not found in archive. Check: https://github.com/$REPO/releases/latest"
        exit 1
    fi

    cp "$bin" "$INSTALL_DIR/gearlynx"
    chmod +x "$INSTALL_DIR/gearlynx"
    echo "$INSTALL_DIR/gearlynx"
}

main() {
    check_existing

    local os
    os=$(uname -s)
    case "$os" in
        Darwin) install_macos ;;
        Linux)  install_linux ;;
        *)
            echo "Unsupported OS: $os"
            echo "Download manually from: https://github.com/$REPO/releases/latest"
            exit 1
            ;;
    esac
}

main
