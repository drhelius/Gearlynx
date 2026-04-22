import * as vscode from 'vscode';
import { DebugMonitorClient } from './debug_monitor_client';
import { FramebufferStreamClient, FrameData } from './framebuffer_client';

// Shared framebuffer stream -- one TCP connection, multiple subscribers
let sharedStream: FramebufferStreamClient | undefined;

export function getSharedStream(): FramebufferStreamClient {
    if (!sharedStream) {
        sharedStream = new FramebufferStreamClient();
    }
    return sharedStream;
}

export function connectSharedStream(port: number): void {
    getSharedStream().connect(port);
}

export function disconnectSharedStream(): void {
    if (sharedStream) {
        sharedStream.disconnect();
    }
}

export class ScreenViewerPanel {
    public static readonly viewType = 'lynxDebug.screenViewer';
    private static instance: ScreenViewerPanel | undefined;

    private panel: vscode.WebviewPanel;
    private disposed = false;
    private monitor: DebugMonitorClient | null = null;
    private keymap: Map<string, string> = new Map();
    private frameHandler: ((frame: FrameData) => void) | null = null;
    private statusHandler: ((connected: boolean) => void) | null = null;

    public static show(extensionUri: vscode.Uri, monitor: DebugMonitorClient): void {
        if (ScreenViewerPanel.instance) {
            ScreenViewerPanel.instance.panel.reveal();
            return;
        }
        ScreenViewerPanel.instance = new ScreenViewerPanel(extensionUri, monitor);
    }

    public static dispose(): void {
        ScreenViewerPanel.instance?.panel.dispose();
    }

    private constructor(_extensionUri: vscode.Uri, monitor: DebugMonitorClient) {
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
            this.unsubscribeStream();
            ScreenViewerPanel.instance = undefined;
        });

        this.panel.webview.onDidReceiveMessage(async (msg) => {
            if (msg.command === 'keydown' || msg.command === 'keyup') {
                await this.handleKeyInput(msg.key, msg.command === 'keydown' ? 'press' : 'release');
            }
        });

        this.panel.webview.html = this.getHtml();
        this.subscribeStream();
    }

    private subscribeStream(): void {
        const stream = getSharedStream();
        this.frameHandler = (frame: FrameData) => {
            if (!this.disposed) {
                this.panel.webview.postMessage({ command: 'frame', ...frame });
            }
        };
        this.statusHandler = (connected: boolean) => {
            if (!this.disposed) {
                this.panel.webview.postMessage({ command: 'status', connected });
            }
        };
        stream.on('frame', this.frameHandler);
        stream.on('status', this.statusHandler);
        // Send current status
        this.panel.webview.postMessage({ command: 'status', connected: stream.isConnected() });
    }

    private unsubscribeStream(): void {
        const stream = getSharedStream();
        if (this.frameHandler) stream.off('frame', this.frameHandler);
        if (this.statusHandler) stream.off('status', this.statusHandler);
        this.frameHandler = null;
        this.statusHandler = null;
    }

    private loadKeymap(): void {
        const cfg = vscode.workspace.getConfiguration('lynxDebug.keymap');
        const buttons = ['up', 'down', 'left', 'right', 'a', 'b', 'option1', 'option2', 'pause'];
        for (const btn of buttons) {
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

    private getHtml(): string {
        return `<!DOCTYPE html>
<html>
<head>
<style>
    body { margin: 0; padding: 8px; background: var(--vscode-editor-background); color: var(--vscode-foreground);
        font-family: var(--vscode-font-family); display: flex; flex-direction: column; align-items: center; }
    .controls { display: flex; gap: 8px; align-items: center; margin-bottom: 8px; font-size: 12px; }
    select { background: var(--vscode-dropdown-background); color: var(--vscode-dropdown-foreground);
        border: 1px solid var(--vscode-dropdown-border); padding: 2px 4px; font-size: 12px; }
    canvas { image-rendering: pixelated; border: 1px solid var(--vscode-panel-border); }
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
            <option value="2">2x</option><option value="3" selected>3x</option>
            <option value="4">4x</option><option value="5">5x</option>
        </select>
    </div>
    <canvas id="screen" width="160" height="102"></canvas>
    <div class="info" id="info">Waiting for frames...</div>
    <div class="status disconnected" id="status">Connecting...</div>
    <script>
        const vscode = acquireVsCodeApi();
        const canvas = document.getElementById('screen');
        const ctx = canvas.getContext('2d');
        const info = document.getElementById('info');
        const statusEl = document.getElementById('status');
        let currentScale = 3, fc = 0, lt = Date.now(), fps = 0;
        canvas.style.width = (160 * currentScale) + 'px';
        canvas.style.height = (102 * currentScale) + 'px';
        window.addEventListener('message', (e) => {
            const m = e.data;
            if (m.command === 'status') {
                statusEl.textContent = m.connected ? 'Connected' : 'Disconnected';
                statusEl.className = 'status ' + (m.connected ? 'connected' : 'disconnected');
            }
            if (m.command === 'frame') {
                if (canvas.width !== m.width || canvas.height !== m.height) {
                    canvas.width = m.width; canvas.height = m.height;
                    canvas.style.width = (m.width * currentScale) + 'px';
                    canvas.style.height = (m.height * currentScale) + 'px';
                }
                ctx.putImageData(new ImageData(new Uint8ClampedArray(m.pixels), m.width, m.height), 0, 0);
                fc++; const now = Date.now();
                if (now - lt >= 1000) { fps = fc; fc = 0; lt = now; }
                info.textContent = m.width + 'x' + m.height + ' | ' + fps + ' fps';
            }
        });
        function updateScale() {
            currentScale = parseInt(document.getElementById('scale').value);
            canvas.style.width = (canvas.width * currentScale) + 'px';
            canvas.style.height = (canvas.height * currentScale) + 'px';
        }
        document.addEventListener('keydown', (e) => { if (!e.repeat) { e.preventDefault(); vscode.postMessage({ command: 'keydown', key: e.key }); } });
        document.addEventListener('keyup', (e) => { e.preventDefault(); vscode.postMessage({ command: 'keyup', key: e.key }); });
        canvas.tabIndex = 0; canvas.focus();
        canvas.addEventListener('click', () => canvas.focus());
    </script>
</body></html>`;
    }
}

export class ScreenViewProvider implements vscode.WebviewViewProvider {
    public static readonly viewType = 'lynxDebug.screenView';

    private view: vscode.WebviewView | undefined;
    private monitor: DebugMonitorClient | null = null;
    private keymap: Map<string, string> = new Map();
    private frameHandler: ((frame: FrameData) => void) | null = null;
    private statusHandler: ((connected: boolean) => void) | null = null;

    constructor(private readonly _extensionUri: vscode.Uri) {}

    public setConnection(monitor: DebugMonitorClient): void {
        this.monitor = monitor;
        this.loadKeymap();
        this.subscribeStream();
    }

    public clearConnection(): void {
        this.unsubscribeStream();
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
        this.unsubscribeStream();
        this.view = webviewView;
        webviewView.webview.options = { enableScripts: true };

        webviewView.webview.onDidReceiveMessage(async (msg) => {
            if (msg.command === 'keydown' || msg.command === 'keyup') {
                await this.handleKeyInput(msg.key, msg.command === 'keydown' ? 'press' : 'release');
            }
        });

        webviewView.onDidDispose(() => {
            this.unsubscribeStream();
            this.view = undefined;
        });

        webviewView.webview.html = this.getHtml();

        if (this.monitor) {
            this.subscribeStream();
        }
    }

    private subscribeStream(): void {
        if (!this.view) return;
        this.unsubscribeStream();
        const stream = getSharedStream();
        this.frameHandler = (frame: FrameData) => {
            this.view?.webview.postMessage({ command: 'frame', ...frame });
        };
        this.statusHandler = (connected: boolean) => {
            this.view?.webview.postMessage({ command: 'status', connected });
        };
        stream.on('frame', this.frameHandler);
        stream.on('status', this.statusHandler);
        this.view.webview.postMessage({ command: 'status', connected: stream.isConnected() });
    }

    private unsubscribeStream(): void {
        if (sharedStream) {
            if (this.frameHandler) sharedStream.off('frame', this.frameHandler);
            if (this.statusHandler) sharedStream.off('status', this.statusHandler);
        }
        this.frameHandler = null;
        this.statusHandler = null;
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

