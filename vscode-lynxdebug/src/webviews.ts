import * as vscode from 'vscode';
import * as net from 'net';
import * as crypto from 'crypto';
import { DebugMonitorClient } from './debug_monitor_client';

export class ScreenViewerPanel {
    public static readonly viewType = 'lynxDebug.screenViewer';
    private static instance: ScreenViewerPanel | undefined;

    private panel: vscode.WebviewPanel;
    private disposed = false;
    private socket: net.Socket | null = null;
    private wsConnected = false;
    private recvBuf = Buffer.alloc(0);
    private wsHandshakeDone = false;
    private monitor: DebugMonitorClient | null = null;
    private keymap: Map<string, string> = new Map();

    public static show(extensionUri: vscode.Uri, wsPort: number, monitor: DebugMonitorClient): void {
        if (ScreenViewerPanel.instance) {
            ScreenViewerPanel.instance.panel.reveal();
            return;
        }
        ScreenViewerPanel.instance = new ScreenViewerPanel(extensionUri, wsPort, monitor);
    }

    public static dispose(): void {
        ScreenViewerPanel.instance?.panel.dispose();
    }

    private constructor(_extensionUri: vscode.Uri, wsPort: number, monitor: DebugMonitorClient) {
        this.monitor = monitor;
        this.loadKeymap();

        this.panel = vscode.window.createWebviewPanel(
            ScreenViewerPanel.viewType,
            'Lynx Screen',
            vscode.ViewColumn.Beside,
            { enableScripts: true, retainContextWhenHidden: true }
        );

        this.panel.onDidDispose(() => {
            this.disposed = true;
            this.disconnectWs();
            ScreenViewerPanel.instance = undefined;
        });

        this.panel.webview.onDidReceiveMessage(async (msg) => {
            if (msg.command === 'keydown' || msg.command === 'keyup') {
                await this.handleKeyInput(msg.key, msg.command === 'keydown' ? 'press' : 'release');
            }
        });

        this.panel.webview.html = this.getHtml();
        this.connectWs(wsPort);
    }

    private loadKeymap(): void {
        const cfg = vscode.workspace.getConfiguration('lynxDebug.keymap');
        const buttons = ['up', 'down', 'left', 'right', 'a', 'b', 'option1', 'option2', 'pause'];
        for (const btn of buttons) {
            const key = cfg.get<string>(btn, '');
            if (key) {
                this.keymap.set(key, btn);
            }
        }
    }

    private async handleKeyInput(key: string, action: string): Promise<void> {
        if (!this.monitor || !this.monitor.isConnected()) return;
        const button = this.keymap.get(key);
        if (!button) return;
        try {
            await this.monitor.controllerButton(button, action);
        } catch {
            // ignore send errors
        }
    }

    private connectWs(port: number): void {
        this.disconnectWs();

        this.socket = new net.Socket();
        this.wsHandshakeDone = false;
        this.recvBuf = Buffer.alloc(0);

        this.socket.connect(port, '127.0.0.1', () => {
            // Send WebSocket upgrade request
            const key = crypto.randomBytes(16).toString('base64');
            const request =
                'GET / HTTP/1.1\r\n' +
                'Host: 127.0.0.1:' + port + '\r\n' +
                'Upgrade: websocket\r\n' +
                'Connection: Upgrade\r\n' +
                'Sec-WebSocket-Key: ' + key + '\r\n' +
                'Sec-WebSocket-Version: 13\r\n' +
                '\r\n';
            this.socket!.write(request);
        });

        this.socket.on('data', (data: Buffer) => {
            if (!this.wsHandshakeDone) {
                // Look for end of HTTP response
                this.recvBuf = Buffer.concat([this.recvBuf, data]);
                const headerEnd = this.recvBuf.indexOf('\r\n\r\n');
                if (headerEnd >= 0) {
                    this.wsHandshakeDone = true;
                    this.wsConnected = true;
                    this.panel.webview.postMessage({ command: 'status', connected: true });
                    // Process remaining data as WebSocket frames
                    this.recvBuf = this.recvBuf.subarray(headerEnd + 4);
                    this.processWsFrames();
                }
            } else {
                this.recvBuf = Buffer.concat([this.recvBuf, data]);
                this.processWsFrames();
            }
        });

        this.socket.on('close', () => {
            this.wsConnected = false;
            if (!this.disposed) {
                this.panel.webview.postMessage({ command: 'status', connected: false });
                // Auto-reconnect
                setTimeout(() => {
                    if (!this.disposed) this.connectWs(port);
                }, 2000);
            }
        });

        this.socket.on('error', () => {
            // close event will handle reconnect
        });
    }

    private disconnectWs(): void {
        this.wsConnected = false;
        if (this.socket) {
            this.socket.destroy();
            this.socket = null;
        }
    }

    private processWsFrames(): void {
        while (this.recvBuf.length >= 2) {
            const byte0 = this.recvBuf[0];
            const byte1 = this.recvBuf[1];
            const masked = (byte1 & 0x80) !== 0;
            let payloadLen = byte1 & 0x7F;
            let headerLen = 2;

            if (payloadLen === 126) {
                if (this.recvBuf.length < 4) return;
                payloadLen = (this.recvBuf[2] << 8) | this.recvBuf[3];
                headerLen = 4;
            } else if (payloadLen === 127) {
                if (this.recvBuf.length < 10) return;
                // Only handle up to 32-bit lengths
                payloadLen = (this.recvBuf[6] << 24) | (this.recvBuf[7] << 16)
                           | (this.recvBuf[8] << 8) | this.recvBuf[9];
                headerLen = 10;
            }

            if (masked) headerLen += 4;

            const totalLen = headerLen + payloadLen;
            if (this.recvBuf.length < totalLen) return;

            const payload = this.recvBuf.subarray(headerLen, totalLen);
            this.recvBuf = this.recvBuf.subarray(totalLen);

            const opcode = byte0 & 0x0F;
            if (opcode === 0x02) {
                // Binary frame -- forward to webview as base64
                // Parse header: width u16 LE, height u16 LE, 4 reserved
                if (payload.length >= 8) {
                    const w = payload[0] | (payload[1] << 8);
                    const h = payload[2] | (payload[3] << 8);
                    const pixels = payload.subarray(8);
                    // Send as array of numbers for efficiency with structured clone
                    this.panel.webview.postMessage({
                        command: 'frame',
                        width: w,
                        height: h,
                        pixels: Array.from(pixels),
                    });
                }
            } else if (opcode === 0x08) {
                // Close frame
                this.disconnectWs();
                return;
            }
        }
    }

    private getHtml(): string {
        return `<!DOCTYPE html>
<html>
<head>
<style>
    body {
        margin: 0; padding: 8px;
        background: var(--vscode-editor-background);
        color: var(--vscode-foreground);
        font-family: var(--vscode-font-family);
        display: flex; flex-direction: column; align-items: center;
    }
    .controls {
        display: flex; gap: 8px; align-items: center; margin-bottom: 8px;
        font-size: 12px;
    }
    select {
        background: var(--vscode-dropdown-background);
        color: var(--vscode-dropdown-foreground);
        border: 1px solid var(--vscode-dropdown-border);
        padding: 2px 4px; font-size: 12px;
    }
    canvas {
        image-rendering: pixelated;
        border: 1px solid var(--vscode-panel-border);
    }
    .info { font-size: 11px; color: var(--vscode-descriptionForeground); margin-top: 4px; }
    .status { font-size: 11px; margin-top: 2px; }
    .status.connected { color: #4ec9b0; }
    .status.disconnected { color: #f44747; }
</style>
</head>
<body>
    <div class="controls">
        <label>Scale:</label>
        <select id="scale" onchange="updateScale()">
            <option value="2">2x</option>
            <option value="3" selected>3x</option>
            <option value="4">4x</option>
            <option value="5">5x</option>
        </select>
    </div>
    <canvas id="screen" width="160" height="102"></canvas>
    <div class="info" id="info">Waiting for frames...</div>
    <div class="status disconnected" id="status">Connecting...</div>

    <script>
        const canvas = document.getElementById('screen');
        const vscode = acquireVsCodeApi();
        const ctx = canvas.getContext('2d');
        const info = document.getElementById('info');
        const statusEl = document.getElementById('status');
        let currentScale = 3;
        let frameCount = 0;
        let lastFpsTime = Date.now();
        let displayFps = 0;

        canvas.style.width = (160 * currentScale) + 'px';
        canvas.style.height = (102 * currentScale) + 'px';

        window.addEventListener('message', (event) => {
            const msg = event.data;

            if (msg.command === 'status') {
                statusEl.textContent = msg.connected ? 'Connected' : 'Disconnected';
                statusEl.className = 'status ' + (msg.connected ? 'connected' : 'disconnected');
            }

            if (msg.command === 'frame') {
                const w = msg.width;
                const h = msg.height;
                const pixels = new Uint8ClampedArray(msg.pixels);

                if (canvas.width !== w || canvas.height !== h) {
                    canvas.width = w;
                    canvas.height = h;
                    canvas.style.width = (w * currentScale) + 'px';
                    canvas.style.height = (h * currentScale) + 'px';
                }

                const imgData = new ImageData(pixels, w, h);
                ctx.putImageData(imgData, 0, 0);

                frameCount++;
                const now = Date.now();
                if (now - lastFpsTime >= 1000) {
                    displayFps = frameCount;
                    frameCount = 0;
                    lastFpsTime = now;
                }
                info.textContent = w + 'x' + h + ' | ' + displayFps + ' fps';
            }
        });

        function updateScale() {
            currentScale = parseInt(document.getElementById('scale').value);
            canvas.style.width = (canvas.width * currentScale) + 'px';
            canvas.style.height = (canvas.height * currentScale) + 'px';
        }

        // Keyboard input -- forward to extension for controller mapping
        document.addEventListener('keydown', (e) => {
            if (e.repeat) return;
            e.preventDefault();
            vscode.postMessage({ command: 'keydown', key: e.key });
        });
        document.addEventListener('keyup', (e) => {
            e.preventDefault();
            vscode.postMessage({ command: 'keyup', key: e.key });
        });

        // Focus canvas for keyboard capture
        canvas.tabIndex = 0;
        canvas.focus();
        canvas.addEventListener('click', () => canvas.focus());
    </script>
</body>
</html>`;
    }
}

export class ScreenViewProvider implements vscode.WebviewViewProvider {
    public static readonly viewType = 'lynxDebug.screenView';

    private view: vscode.WebviewView | undefined;
    private wsSocket: net.Socket | null = null;
    private wsHandshakeDone = false;
    private recvBuf = Buffer.alloc(0);
    private wsPort = 0;
    private monitor: DebugMonitorClient | null = null;
    private keymap: Map<string, string> = new Map();
    private wsGeneration = 0;

    constructor(private readonly _extensionUri: vscode.Uri) {}

    public setConnection(wsPort: number, monitor: DebugMonitorClient): void {
        this.wsPort = wsPort;
        this.monitor = monitor;
        this.loadKeymap();
        this.connectWs();
    }

    public clearConnection(): void {
        this.disconnectWs();
        this.wsPort = 0;
        this.monitor = null;
        if (this.view) {
            this.view.webview.postMessage({ command: 'status', connected: false });
        }
    }

    public resolveWebviewView(
        webviewView: vscode.WebviewView,
        _context: vscode.WebviewViewResolveContext,
        _token: vscode.CancellationToken
    ): void {
        // Tear down old connection -- view was destroyed and re-created
        this.disconnectWs();

        this.view = webviewView;
        webviewView.webview.options = { enableScripts: true };

        webviewView.webview.onDidReceiveMessage(async (msg) => {
            if (msg.command === 'keydown' || msg.command === 'keyup') {
                await this.handleKeyInput(msg.key, msg.command === 'keydown' ? 'press' : 'release');
            }
        });

        webviewView.onDidDispose(() => {
            this.disconnectWs();
            this.view = undefined;
        });

        webviewView.webview.html = this.getHtml();

        if (this.wsPort > 0) {
            this.connectWs();
        }
    }

    private loadKeymap(): void {
        this.keymap.clear();
        const cfg = vscode.workspace.getConfiguration('lynxDebug.keymap');
        for (const btn of ['up', 'down', 'left', 'right', 'a', 'b', 'option1', 'option2', 'pause']) {
            const key = cfg.get<string>(btn, '');
            if (key) this.keymap.set(key, btn);
        }
    }

    private async handleKeyInput(key: string, action: string): Promise<void> {
        if (!this.monitor || !this.monitor.isConnected()) return;
        const button = this.keymap.get(key);
        if (!button) return;
        try { await this.monitor.controllerButton(button, action); } catch { /* ignore */ }
    }

    private connectWs(): void {
        if (this.wsPort <= 0 || !this.view) return;
        this.disconnectWs();

        const port = this.wsPort;
        const gen = ++this.wsGeneration;
        this.wsSocket = new net.Socket();
        this.wsHandshakeDone = false;
        this.recvBuf = Buffer.alloc(0);

        this.wsSocket.connect(port, '127.0.0.1', () => {
            if (gen !== this.wsGeneration) return;
            const key = crypto.randomBytes(16).toString('base64');
            this.wsSocket!.write(
                'GET / HTTP/1.1\r\nHost: 127.0.0.1:' + port +
                '\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: ' +
                key + '\r\nSec-WebSocket-Version: 13\r\n\r\n'
            );
        });

        this.wsSocket.on('data', (data: Buffer) => {
            if (gen !== this.wsGeneration) return;
            if (!this.wsHandshakeDone) {
                this.recvBuf = Buffer.concat([this.recvBuf, data]);
                const headerEnd = this.recvBuf.indexOf('\r\n\r\n');
                if (headerEnd >= 0) {
                    this.wsHandshakeDone = true;
                    this.view?.webview.postMessage({ command: 'status', connected: true });
                    this.recvBuf = this.recvBuf.subarray(headerEnd + 4);
                    this.processWsFrames();
                }
            } else {
                this.recvBuf = Buffer.concat([this.recvBuf, data]);
                this.processWsFrames();
            }
        });

        this.wsSocket.on('close', () => {
            if (gen !== this.wsGeneration) return;
            this.view?.webview.postMessage({ command: 'status', connected: false });
            if (this.wsPort > 0 && this.view) {
                setTimeout(() => {
                    if (gen === this.wsGeneration) this.connectWs();
                }, 2000);
            }
        });

        this.wsSocket.on('error', () => {});
    }

    private disconnectWs(): void {
        this.wsGeneration++;
        if (this.wsSocket) { this.wsSocket.destroy(); this.wsSocket = null; }
    }

    private processWsFrames(): void {
        while (this.recvBuf.length >= 2) {
            const byte1 = this.recvBuf[1];
            let payloadLen = byte1 & 0x7F;
            let headerLen = 2;
            if (payloadLen === 126) {
                if (this.recvBuf.length < 4) return;
                payloadLen = (this.recvBuf[2] << 8) | this.recvBuf[3]; headerLen = 4;
            } else if (payloadLen === 127) {
                if (this.recvBuf.length < 10) return;
                payloadLen = (this.recvBuf[6] << 24) | (this.recvBuf[7] << 16)
                           | (this.recvBuf[8] << 8) | this.recvBuf[9]; headerLen = 10;
            }
            if (byte1 & 0x80) headerLen += 4;
            const totalLen = headerLen + payloadLen;
            if (this.recvBuf.length < totalLen) return;
            const payload = this.recvBuf.subarray(headerLen, totalLen);
            this.recvBuf = this.recvBuf.subarray(totalLen);
            if (payload.length >= 8) {
                const w = payload[0] | (payload[1] << 8);
                const h = payload[2] | (payload[3] << 8);
                this.view?.webview.postMessage({
                    command: 'frame', width: w, height: h,
                    pixels: Array.from(payload.subarray(8)),
                });
            }
        }
    }

    private getHtml(): string {
        return `<!DOCTYPE html>
<html><head><style>
    body { margin: 0; padding: 4px; background: var(--vscode-editor-background); display: flex; flex-direction: column; align-items: center; }
    canvas { image-rendering: pixelated; border: 1px solid var(--vscode-panel-border); width: 100%; max-width: 640px; }
    .info { font-size: 10px; color: var(--vscode-descriptionForeground); font-family: var(--vscode-font-family); margin-top: 2px; }
    .status.connected { color: #4ec9b0; }
    .status.disconnected { color: #f44747; }
</style></head><body>
    <canvas id="screen" width="160" height="102"></canvas>
    <div class="info" id="info">Waiting...</div>
    <div class="info status disconnected" id="status">Disconnected</div>
    <script>
        const vscode = acquireVsCodeApi();
        const canvas = document.getElementById('screen');
        const ctx = canvas.getContext('2d');
        const info = document.getElementById('info');
        const statusEl = document.getElementById('status');
        let fc = 0, lt = Date.now(), fps = 0;
        window.addEventListener('message', (e) => {
            const m = e.data;
            if (m.command === 'status') {
                statusEl.textContent = m.connected ? 'Connected' : 'Disconnected';
                statusEl.className = 'info status ' + (m.connected ? 'connected' : 'disconnected');
            }
            if (m.command === 'frame') {
                if (canvas.width !== m.width || canvas.height !== m.height) { canvas.width = m.width; canvas.height = m.height; }
                ctx.putImageData(new ImageData(new Uint8ClampedArray(m.pixels), m.width, m.height), 0, 0);
                fc++; const now = Date.now();
                if (now - lt >= 1000) { fps = fc; fc = 0; lt = now; }
                info.textContent = m.width + 'x' + m.height + ' | ' + fps + ' fps';
            }
        });
        document.addEventListener('keydown', (e) => { if (!e.repeat) { e.preventDefault(); vscode.postMessage({ command: 'keydown', key: e.key }); } });
        document.addEventListener('keyup', (e) => { e.preventDefault(); vscode.postMessage({ command: 'keyup', key: e.key }); });
        canvas.tabIndex = 0; canvas.focus();
        canvas.addEventListener('click', () => canvas.focus());
    </script>
</body></html>`;
    }
}
