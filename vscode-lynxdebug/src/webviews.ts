import * as vscode from 'vscode';
import * as net from 'net';
import * as crypto from 'crypto';

export class ScreenViewerPanel {
    public static readonly viewType = 'lynxDebug.screenViewer';
    private static instance: ScreenViewerPanel | undefined;

    private panel: vscode.WebviewPanel;
    private disposed = false;
    private socket: net.Socket | null = null;
    private wsConnected = false;
    private recvBuf = Buffer.alloc(0);
    private wsHandshakeDone = false;

    public static show(extensionUri: vscode.Uri, wsPort: number): void {
        if (ScreenViewerPanel.instance) {
            ScreenViewerPanel.instance.panel.reveal();
            return;
        }
        ScreenViewerPanel.instance = new ScreenViewerPanel(extensionUri, wsPort);
    }

    public static dispose(): void {
        ScreenViewerPanel.instance?.panel.dispose();
    }

    private constructor(_extensionUri: vscode.Uri, wsPort: number) {
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

        this.panel.webview.html = this.getHtml();
        this.connectWs(wsPort);
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
    </script>
</body>
</html>`;
    }
}
