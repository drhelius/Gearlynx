import * as vscode from 'vscode';
import * as net from 'net';
import { DebugMonitorClient } from './debug_monitor_client';
import { SegmentInfo } from './types';

export class ScreenViewerPanel {
    public static readonly viewType = 'lynxDebug.screenViewer';
    private static instance: ScreenViewerPanel | undefined;

    private panel: vscode.WebviewPanel;
    private disposed = false;
    private socket: net.Socket | null = null;
    private connected = false;
    private recvBuf = Buffer.alloc(0);
    private monitor: DebugMonitorClient | null = null;
    private keymap: Map<string, string> = new Map();

    public static show(extensionUri: vscode.Uri, streamPort: number, monitor: DebugMonitorClient): void {
        if (ScreenViewerPanel.instance) {
            ScreenViewerPanel.instance.panel.reveal();
            return;
        }
        ScreenViewerPanel.instance = new ScreenViewerPanel(extensionUri, streamPort, monitor);
    }

    public static dispose(): void {
        ScreenViewerPanel.instance?.panel.dispose();
    }

    private constructor(_extensionUri: vscode.Uri, streamPort: number, monitor: DebugMonitorClient) {
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
            this.disconnectStream();
            ScreenViewerPanel.instance = undefined;
        });

        this.panel.webview.onDidReceiveMessage(async (msg) => {
            if (msg.command === 'keydown' || msg.command === 'keyup') {
                await this.handleKeyInput(msg.key, msg.command === 'keydown' ? 'press' : 'release');
            }
        });

        this.panel.webview.html = this.getHtml();
        this.connectStream(streamPort);
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

    private connectStream(port: number): void {
        this.disconnectStream();

        this.socket = new net.Socket();
        this.recvBuf = Buffer.alloc(0);

        this.socket.connect(port, '127.0.0.1', () => {
            this.connected = true;
            this.panel.webview.postMessage({ command: 'status', connected: true });
        });

        this.socket.on('data', (data: Buffer) => {
            this.recvBuf = Buffer.concat([this.recvBuf, data]);
            this.processFrames();
        });

        this.socket.on('close', () => {
            this.connected = false;
            if (!this.disposed) {
                this.panel.webview.postMessage({ command: 'status', connected: false });
                setTimeout(() => { if (!this.disposed) this.connectStream(port); }, 2000);
            }
        });

        this.socket.on('error', () => {});
    }

    private disconnectStream(): void {
        this.connected = false;
        if (this.socket) { this.socket.destroy(); this.socket = null; }
    }

    private processFrames(): void {
        // Raw TCP: 8-byte header (w u16 LE, h u16 LE, size u32 LE) + pixels
        while (this.recvBuf.length >= 8) {
            const w = this.recvBuf[0] | (this.recvBuf[1] << 8);
            const h = this.recvBuf[2] | (this.recvBuf[3] << 8);
            const size = this.recvBuf[4] | (this.recvBuf[5] << 8)
                       | (this.recvBuf[6] << 16) | (this.recvBuf[7] << 24);

            if (this.recvBuf.length < 8 + size) return;

            const pixels = this.recvBuf.subarray(8, 8 + size);
            this.recvBuf = this.recvBuf.subarray(8 + size);

            this.panel.webview.postMessage({
                command: 'frame',
                width: w,
                height: h,
                pixels: Array.from(pixels),
            });
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
    private streamSocket: net.Socket | null = null;
    private recvBuf = Buffer.alloc(0);
    private streamPort = 0;
    private monitor: DebugMonitorClient | null = null;
    private keymap: Map<string, string> = new Map();
    private streamGeneration = 0;

    constructor(private readonly _extensionUri: vscode.Uri) {}

    public setConnection(streamPort: number, monitor: DebugMonitorClient): void {
        this.streamPort = streamPort;
        this.monitor = monitor;
        this.loadKeymap();
        this.connectStream();
    }

    public clearConnection(): void {
        this.disconnectStream();
        this.streamPort = 0;
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
        this.disconnectStream();

        this.view = webviewView;
        webviewView.webview.options = { enableScripts: true };

        webviewView.webview.onDidReceiveMessage(async (msg) => {
            if (msg.command === 'keydown' || msg.command === 'keyup') {
                await this.handleKeyInput(msg.key, msg.command === 'keydown' ? 'press' : 'release');
            }
        });

        webviewView.onDidDispose(() => {
            this.disconnectStream();
            this.view = undefined;
        });

        webviewView.webview.html = this.getHtml();

        if (this.streamPort > 0) {
            this.connectStream();
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

    private connectStream(): void {
        if (this.streamPort <= 0 || !this.view) return;
        this.disconnectStream();

        const port = this.streamPort;
        const gen = ++this.streamGeneration;
        this.streamSocket = new net.Socket();
        this.recvBuf = Buffer.alloc(0);

        this.streamSocket.connect(port, '127.0.0.1', () => {
            if (gen !== this.streamGeneration) return;
            this.view?.webview.postMessage({ command: 'status', connected: true });
        });

        this.streamSocket.on('data', (data: Buffer) => {
            if (gen !== this.streamGeneration) return;
            this.recvBuf = Buffer.concat([this.recvBuf, data]);
            this.processFrames();
        });

        this.streamSocket.on('close', () => {
            if (gen !== this.streamGeneration) return;
            this.view?.webview.postMessage({ command: 'status', connected: false });
            if (this.streamPort > 0 && this.view) {
                setTimeout(() => {
                    if (gen === this.streamGeneration) this.connectStream();
                }, 2000);
            }
        });

        this.streamSocket.on('error', () => {});
    }

    private disconnectStream(): void {
        this.streamGeneration++;
        if (this.streamSocket) { this.streamSocket.destroy(); this.streamSocket = null; }
    }

    private processFrames(): void {
        while (this.recvBuf.length >= 8) {
            const w = this.recvBuf[0] | (this.recvBuf[1] << 8);
            const h = this.recvBuf[2] | (this.recvBuf[3] << 8);
            const size = this.recvBuf[4] | (this.recvBuf[5] << 8)
                       | (this.recvBuf[6] << 16) | (this.recvBuf[7] << 24);
            if (this.recvBuf.length < 8 + size) return;
            const pixels = this.recvBuf.subarray(8, 8 + size);
            this.recvBuf = this.recvBuf.subarray(8 + size);
            this.view?.webview.postMessage({
                command: 'frame', width: w, height: h,
                pixels: Array.from(pixels),
            });
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
