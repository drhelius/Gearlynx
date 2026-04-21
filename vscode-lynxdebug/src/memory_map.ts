import * as vscode from 'vscode';
import { SegmentInfo } from './types';

export class MemoryMapPanel {
    public static readonly viewType = 'lynxDebug.memoryMap';
    private static instance: MemoryMapPanel | undefined;
    private panel: vscode.WebviewPanel;

    public static show(segments: SegmentInfo[]): void {
        if (MemoryMapPanel.instance) {
            MemoryMapPanel.instance.panel.reveal();
            MemoryMapPanel.instance.update(segments);
            return;
        }
        MemoryMapPanel.instance = new MemoryMapPanel(segments);
    }

    public static dispose(): void {
        MemoryMapPanel.instance?.panel.dispose();
    }

    private constructor(segments: SegmentInfo[]) {
        this.panel = vscode.window.createWebviewPanel(
            MemoryMapPanel.viewType,
            'Lynx Memory Map',
            vscode.ViewColumn.Beside,
            { enableScripts: true }
        );

        this.panel.onDidDispose(() => {
            MemoryMapPanel.instance = undefined;
        });

        this.update(segments);
    }

    private update(segments: SegmentInfo[]): void {
        this.panel.webview.html = this.getHtml(segments);
    }

    private getHtml(segments: SegmentInfo[]): string {
        function getColor(name: string, type: string): string {
            if (name === 'ZEROPAGE' || name === 'EXTZP') return '#dcdcaa';
            if (name.includes('CODE') || name === 'STARTUP' || name === 'LOWCODE' || name === 'ONCE') return '#4ec9b0';
            if (name.includes('RODATA')) return '#569cd6';
            if (name.includes('BSS')) return '#9cdcfe';
            if (name.includes('DATA')) return '#ce9178';
            if (type === 'ro') return '#569cd6';
            if (type === 'hw') return '#f44747';
            return '#808080';
        }

        // Filter out cart-layout-only segments that don't represent CPU address space
        const excludeNames = new Set(['EXEHDR', 'DIRECTORY', 'NULL']);

        const allRegions = [
            ...segments
                .filter(s => s.size > 0 && !excludeNames.has(s.name))
                .map(s => ({
                    name: s.name, start: s.start, size: s.size,
                    end: s.start + s.size - 1, color: getColor(s.name, s.type), hw: false,
                })),
            { name: 'Stack', start: 0x0100, size: 256, end: 0x01FF, color: '#c586c0', hw: true },
            { name: 'Suzy', start: 0xFC00, size: 256, end: 0xFCFF, color: '#f44747', hw: true },
            { name: 'Mikey', start: 0xFD00, size: 256, end: 0xFDFF, color: '#f44747', hw: true },
            { name: 'BIOS', start: 0xFE00, size: 0x1F8, end: 0xFFF7, color: '#6a9955', hw: true },
        ];

        // Usage stats
        let totalCode = 0, totalRodata = 0, totalData = 0, totalBss = 0;
        for (const s of segments) {
            if (s.name.includes('CODE') || s.name === 'STARTUP' || s.name === 'LOWCODE' || s.name === 'ONCE') totalCode += s.size;
            else if (s.name.includes('RODATA')) totalRodata += s.size;
            else if (s.name.includes('BSS')) totalBss += s.size;
            else if (s.name.includes('DATA') || s.name === 'ZEROPAGE' || s.name === 'EXTZP') totalData += s.size;
        }

        // Serialize regions as JSON for the canvas renderer
        const regionsJson = JSON.stringify(allRegions);

        return `<!DOCTYPE html>
<html>
<head>
<style>
    body {
        font-family: var(--vscode-font-family);
        color: var(--vscode-foreground);
        background: var(--vscode-editor-background);
        padding: 12px; margin: 0;
    }
    h2 { margin: 0 0 10px 0; font-size: 16px; }
    .summary {
        display: flex; gap: 16px; margin-bottom: 14px;
        padding: 10px; background: var(--vscode-editor-inactiveSelectionBackground);
        border-radius: 4px; font-size: 13px;
    }
    .stat { display: flex; flex-direction: column; align-items: center; }
    .stat-value { font-size: 18px; font-weight: bold; font-family: monospace; }
    .stat-label { font-size: 11px; color: var(--vscode-descriptionForeground); margin-top: 2px; }
    canvas { display: block; margin-top: 8px; }
    #tooltip {
        display: none; position: fixed; padding: 6px 10px;
        background: var(--vscode-editorHoverWidget-background);
        border: 1px solid var(--vscode-editorHoverWidget-border);
        font-size: 12px; font-family: monospace; z-index: 100;
        pointer-events: none; border-radius: 3px;
    }
</style>
</head>
<body>
    <h2>Memory Map</h2>
    <div class="summary">
        <div class="stat"><span class="stat-value" style="color:#4ec9b0">${(totalCode/1024).toFixed(1)}K</span><span class="stat-label">Code</span></div>
        <div class="stat"><span class="stat-value" style="color:#569cd6">${(totalRodata/1024).toFixed(1)}K</span><span class="stat-label">RODATA</span></div>
        <div class="stat"><span class="stat-value" style="color:#ce9178">${(totalData/1024).toFixed(1)}K</span><span class="stat-label">Data</span></div>
        <div class="stat"><span class="stat-value" style="color:#9cdcfe">${(totalBss/1024).toFixed(1)}K</span><span class="stat-label">BSS</span></div>
        <div class="stat"><span class="stat-value">${((totalCode+totalRodata+totalData+totalBss)/1024).toFixed(1)}K</span><span class="stat-label">Total</span></div>
    </div>
    <canvas id="map"></canvas>
    <div id="tooltip"></div>
    <script>
    (function() {
        const regions = ${regionsJson};
        const canvas = document.getElementById('map');
        const ctx = canvas.getContext('2d');
        const tooltip = document.getElementById('tooltip');
        const dpr = window.devicePixelRatio || 1;

        // Layout: assign each region to a column to avoid overlap
        // Column 0 is the main column; overlapping regions go to column 1, 2, etc.
        const placed = []; // { start, end, col }
        regions.sort((a, b) => a.start - b.start || b.size - a.size);

        for (const r of regions) {
            let col = 0;
            while (true) {
                const conflict = placed.find(p => p.col === col && r.start < p.end && r.start + r.size > p.start);
                if (!conflict) break;
                col++;
            }
            r.col = col;
            placed.push({ start: r.start, end: r.start + r.size, col: col });
        }

        const maxCol = Math.max(0, ...regions.map(r => r.col));
        const colCount = maxCol + 1;

        // Dimensions
        const addrLabelW = 55;
        const colW = Math.max(80, Math.min(150, (800 - addrLabelW) / colCount));
        const gap = 2;
        const totalW = addrLabelW + colCount * (colW + gap);
        const minBlockH = 30;

        // Give each region a unique index for position tracking
        for (let i = 0; i < regions.length; i++) regions[i]._idx = i;

        // Build sorted unique address boundaries from all regions
        const boundaries = [];
        for (const r of regions) {
            boundaries.push(r.start);
            boundaries.push(r.start + r.size);
        }
        const uniqueAddrs = [...new Set(boundaries)].sort((a, b) => a - b);

        // Assign Y positions: each address gap gets proportional space,
        // but with a minimum height so small segments are visible
        const yForAddr = new Map();
        let curY = 10;
        for (let i = 0; i < uniqueAddrs.length; i++) {
            yForAddr.set(uniqueAddrs[i], curY);
            if (i < uniqueAddrs.length - 1) {
                const addrGap = uniqueAddrs[i + 1] - uniqueAddrs[i];
                // Check if any region spans exactly this gap
                const hasRegion = regions.some(r =>
                    r.start <= uniqueAddrs[i] && r.start + r.size >= uniqueAddrs[i + 1]
                );
                if (hasRegion) {
                    const naturalH = Math.sqrt(addrGap) * 0.6;
                    curY += Math.max(minBlockH, naturalH);
                } else {
                    // Empty gap between regions -- small fixed space
                    curY += 8;
                }
            }
        }
        const totalH = curY + 20;

        canvas.width = totalW * dpr;
        canvas.height = totalH * dpr;
        canvas.style.width = totalW + 'px';
        canvas.style.height = totalH + 'px';
        ctx.scale(dpr, dpr);

        function fmtAddr(a) {
            return '$' + a.toString(16).toUpperCase().padStart(4, '0');
        }

        function fmtSize(s) {
            return s >= 1024 ? (s / 1024).toFixed(1) + 'K' : s + 'B';
        }

        // Draw address labels at each boundary that starts a col-0 region
        ctx.fillStyle = getComputedStyle(document.body).getPropertyValue('--vscode-descriptionForeground') || '#888';
        ctx.font = '9px monospace';
        ctx.textAlign = 'right';
        const labelledAddrs = new Set();
        for (const r of regions) {
            if (labelledAddrs.has(r.start)) continue;
            const y = yForAddr.get(r.start);
            if (y === undefined) continue;
            ctx.fillText(fmtAddr(r.start), addrLabelW - 4, y + 10);
            ctx.strokeStyle = 'rgba(128,128,128,0.12)';
            ctx.beginPath();
            ctx.moveTo(addrLabelW, y);
            ctx.lineTo(totalW, y);
            ctx.stroke();
            labelledAddrs.add(r.start);
        }

        // Draw regions -- each positioned by its own address
        const rects = [];
        for (const r of regions) {
            const y1 = yForAddr.get(r.start);
            const y2 = yForAddr.get(r.start + r.size);
            if (y1 === undefined || y2 === undefined) continue;
            const h = Math.max(minBlockH, y2 - y1);
            const x = addrLabelW + r.col * (colW + gap);

            ctx.fillStyle = r.color + (r.hw ? '60' : 'CC');
            ctx.fillRect(x, y1, colW, h);

            ctx.strokeStyle = r.color;
            ctx.lineWidth = 1;
            ctx.strokeRect(x + 0.5, y1 + 0.5, colW - 1, h - 1);

            // Always show label -- minBlockH guarantees enough height
            ctx.fillStyle = '#fff';
            ctx.font = 'bold 10px ' + getComputedStyle(document.body).fontFamily;
            ctx.textAlign = 'left';
            ctx.save();
            ctx.beginPath();
            ctx.rect(x, y1, colW, h);
            ctx.clip();
            ctx.fillText(r.name, x + 3, y1 + 13);
            if (h > 26) {
                ctx.font = '9px monospace';
                ctx.fillStyle = 'rgba(255,255,255,0.7)';
                ctx.fillText(fmtSize(r.size), x + 3, y1 + 25);
            }
            ctx.restore();

            rects.push({ x, y: y1, w: colW, h, region: r });
        }

        // Tooltip on hover
        canvas.addEventListener('mousemove', (e) => {
            const rect = canvas.getBoundingClientRect();
            const mx = e.clientX - rect.left;
            const my = e.clientY - rect.top;
            const hit = rects.find(r => mx >= r.x && mx <= r.x + r.w && my >= r.y && my <= r.y + r.h);
            if (hit) {
                const r = hit.region;
                tooltip.innerHTML = '<b>' + r.name + '</b><br>' +
                    fmtAddr(r.start) + ' - ' + fmtAddr(r.start + r.size - 1) + '<br>' +
                    'Size: ' + fmtSize(r.size);
                tooltip.style.display = 'block';
                tooltip.style.left = (e.clientX + 12) + 'px';
                tooltip.style.top = (e.clientY + 12) + 'px';
            } else {
                tooltip.style.display = 'none';
            }
        });
        canvas.addEventListener('mouseleave', () => { tooltip.style.display = 'none'; });
    })();
    </script>
</body>
</html>`;
    }
}
