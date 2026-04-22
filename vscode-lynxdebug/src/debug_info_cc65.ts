import * as fs from 'fs';
import { SourceLocation, DebugSymbol, DebugFunction, LocalVariable, OverlayGroup, SegmentInfo, DebugInfoData } from './types';

interface DbgFile {
    id: number;
    name: string;
}

interface DbgSegment {
    id: number;
    name: string;
    start: number;
    size: number;
    type?: string;
}

interface DbgSpan {
    id: number;
    seg: number;
    start: number;
    size: number;
    address?: number;
    lineInfos?: number[];
}

interface DbgLine {
    id: number;
    file: number;
    line: number;
    span?: number | number[];
    type?: number;
}

interface DbgSym {
    id: number;
    name: string;
    val?: number;
    seg?: number;
    scope?: number;
    type?: string;
}

interface DbgScope {
    id: number;
    name: string;
    mod?: number;
    parent?: number;
    span?: number | number[];
    size?: number;
}

interface DbgCSym {
    id: number;
    name: string;
    scope?: number;
    sym?: number;
    offs?: number;
    sc?: string;
}

interface DbgData {
    files: DbgFile[];
    segs: DbgSegment[];
    spans: DbgSpan[];
    lines: DbgLine[];
    syms: DbgSym[];
    scopes: DbgScope[];
    csyms: DbgCSym[];
}

export class Cc65DebugInfo {
    static parse(filePath: string, sourceRoots: string[]): DebugInfoData | null {
        let src: string;
        try {
            src = fs.readFileSync(filePath, 'utf-8');
        } catch {
            return null;
        }

        const data = Cc65DebugInfo.scan(src);
        return Cc65DebugInfo.resolve(data, sourceRoots);
    }

    private static scan(src: string): DbgData {
        const data: DbgData = {
            files: [],
            segs: [],
            spans: [],
            lines: [],
            syms: [],
            scopes: [],
            csyms: [],
        };

        const num = (attrs: Map<string, unknown>, key: string, def: number = 0): number => {
            const v = attrs.get(key);
            return typeof v === 'number' ? v : def;
        };
        const str = (attrs: Map<string, unknown>, key: string, def: string = ''): string => {
            const v = attrs.get(key);
            return typeof v === 'string' ? v : def;
        };
        const optNum = (attrs: Map<string, unknown>, key: string): number | undefined => {
            const v = attrs.get(key);
            return typeof v === 'number' ? v : undefined;
        };
        const optStr = (attrs: Map<string, unknown>, key: string): string | undefined => {
            const v = attrs.get(key);
            return typeof v === 'string' ? v : undefined;
        };
        const numOrArr = (attrs: Map<string, unknown>, key: string): number | number[] | undefined => {
            const v = attrs.get(key);
            if (typeof v === 'number') return v;
            if (Array.isArray(v)) return v as number[];
            return undefined;
        };

        const lines = src.split('\n').filter(s => s.trim().length > 0);
        for (const line of lines) {
            const parsed = Cc65DebugInfo.parseLine(line);
            if (!parsed) continue;
            const a = parsed.attrs;

            switch (parsed.key) {
                case 'file':
                    data.files.push({ id: num(a, 'id'), name: str(a, 'name') });
                    break;
                case 'seg':
                    data.segs.push({ id: num(a, 'id'), name: str(a, 'name'), start: num(a, 'start'), size: num(a, 'size'), type: optStr(a, 'type') });
                    break;
                case 'span':
                    data.spans.push({ id: num(a, 'id'), seg: num(a, 'seg'), start: num(a, 'start'), size: num(a, 'size') });
                    break;
                case 'line':
                    data.lines.push({ id: num(a, 'id'), file: num(a, 'file'), line: num(a, 'line'), span: numOrArr(a, 'span'), type: optNum(a, 'type') });
                    break;
                case 'sym':
                    data.syms.push({ id: num(a, 'id'), name: str(a, 'name'), val: optNum(a, 'val'), seg: optNum(a, 'seg'), scope: optNum(a, 'scope'), type: optStr(a, 'type') });
                    break;
                case 'scope':
                    data.scopes.push({ id: num(a, 'id'), name: str(a, 'name'), mod: optNum(a, 'mod'), parent: optNum(a, 'parent'), span: numOrArr(a, 'span'), size: optNum(a, 'size') });
                    break;
                case 'csym':
                    data.csyms.push({ id: num(a, 'id'), name: str(a, 'name'), scope: optNum(a, 'scope'), sym: optNum(a, 'sym'), offs: optNum(a, 'offs'), sc: optStr(a, 'sc') });
                    break;
            }
        }

        return data;
    }

    private static resolve(data: DbgData, _sourceRoots: string[]): DebugInfoData {
        const addressToSource = new Map<number, SourceLocation>();
        const sourceToAddresses = new Map<string, Map<number, number[]>>();
        const symbols: DebugSymbol[] = [];
        const functions: DebugFunction[] = [];
        const locals: LocalVariable[] = [];

        // Find zeropage stack pointer address (default 0x02 for cc65 on Lynx)
        let zeropageStackPointerAddr = 0x02;
        for (const seg of data.segs) {
            if (seg.name === 'ZEROPAGE') {
                zeropageStackPointerAddr = seg.start;
                break;
            }
        }

        // Detect overlay groups -- code segments sharing the same start address
        const startAddrToSegs = new Map<number, number[]>();
        for (const seg of data.segs) {
            if (seg.size === 0 || seg.type !== 'ro') continue;
            if (seg.name === 'NULL' || seg.name === 'EXEHDR' || seg.name === 'DIRECTORY') continue;
            const existing = startAddrToSegs.get(seg.start);
            if (existing) {
                existing.push(seg.id);
            } else {
                startAddrToSegs.set(seg.start, [seg.id]);
            }
        }

        const overlayGroups: OverlayGroup[] = [];
        for (const [_addr, segIds] of startAddrToSegs) {
            if (segIds.length > 1) {
                const names = segIds.map(id => {
                    const s = data.segs.find(seg => seg.id === id);
                    return s?.name || `seg${id}`;
                });
                overlayGroups.push({
                    segmentIds: segIds,
                    segmentNames: names,
                });
            }
        }

        // Build segment lookup
        const segMap = new Map<number, DbgSegment>();
        for (const seg of data.segs) {
            segMap.set(seg.id, seg);
        }

        // Build file lookup
        const fileMap = new Map<number, DbgFile>();
        for (const file of data.files) {
            fileMap.set(file.id, file);
        }

        // Resolve span addresses
        for (const span of data.spans) {
            const seg = segMap.get(span.seg);
            if (seg) {
                span.address = seg.start + span.start;
            }
        }

        // Build span lookup
        const spanMap = new Map<number, DbgSpan>();
        for (const span of data.spans) {
            spanMap.set(span.id, span);
        }

        // Process line entries -> build address<->source maps
        for (const line of data.lines) {
            const file = fileMap.get(line.file);
            if (!file) continue;

            // Skip cc65-generated intermediate assembly files
            // These have paths like CMakeFiles/xxx.dir/file.c.NNNNN.N.s
            const fileName = file.name;
            if (Cc65DebugInfo.isIntermediateFile(fileName)) continue;

            const spanIds = Cc65DebugInfo.toArray(line.span);
            for (const spanId of spanIds) {
                const span = spanMap.get(spanId);
                if (!span || span.address === undefined) continue;

                const addr = span.address;
                const addrEnd = addr + span.size - 1;
                const sourcePath = fileName.replace(/\\/g, '/');

                const loc: SourceLocation = {
                    source: sourcePath,
                    line: line.line,
                    address: addr,
                    addressEnd: addrEnd,
                    segmentId: span.seg,
                };

                // Prefer C lines (type 0 or undefined) over assembly lines (type 1)
                const existing = addressToSource.get(addr);
                const isCLine = (line.type === undefined || line.type === 0);
                if (!existing || isCLine) {
                    addressToSource.set(addr, loc);
                }

                // Source -> addresses map
                const normalizedSource = sourcePath.toLowerCase();
                let fileEntries = sourceToAddresses.get(normalizedSource);
                if (!fileEntries) {
                    fileEntries = new Map<number, number[]>();
                    sourceToAddresses.set(normalizedSource, fileEntries);
                }
                let lineAddrs = fileEntries.get(line.line);
                if (!lineAddrs) {
                    lineAddrs = [];
                    fileEntries.set(line.line, lineAddrs);
                }
                if (!lineAddrs.includes(addr)) {
                    lineAddrs.push(addr);
                }
            }
        }

        // Process symbols -- all lab symbols for lookup (evaluate/hover)
        for (const sym of data.syms) {
            if (sym.val !== undefined && sym.type === 'lab') {
                const seg = sym.seg !== undefined ? segMap.get(sym.seg) : undefined;
                const isZP = seg ? (seg.name === 'ZEROPAGE' || seg.name === 'EXTZP') : false;
                const isWritable = !seg || seg.type !== 'ro';
                // Treat writable ZP/BSS/DATA lab symbols with C-style names as C variables
                const looksLikeCVar = sym.name.startsWith('_') && !sym.name.startsWith('__') && isWritable;
                symbols.push({
                    name: sym.name,
                    address: sym.val,
                    isGlobal: sym.scope === 0 || sym.scope === undefined,
                    isZeroPage: isZP,
                    isCVariable: looksLikeCVar,
                    segment: seg?.name || '',
                });
            }
        }

        // Add csym ext variables -- these are the actual C globals
        for (const csym of data.csyms) {
            if (csym.sc === 'ext' && csym.sym !== undefined) {
                let sym = data.syms[csym.sym];
                if (sym && sym.type === 'imp' && sym.val === undefined) {
                    const exportSym = data.syms.find(
                        s => s.name === sym!.name && s.type === 'lab' && s.val !== undefined
                    );
                    if (exportSym) sym = exportSym;
                }
                if (sym && sym.val !== undefined) {
                    const seg = sym.seg !== undefined ? segMap.get(sym.seg) : undefined;
                    // Skip read-only segments (RODATA, CODE, etc.) -- these are const data
                    if (seg && seg.type === 'ro') continue;
                    const isZP = seg ? (seg.name === 'ZEROPAGE' || seg.name === 'EXTZP') : false;
                    // Mark existing asm symbol as C variable, or add new
                    const existing = symbols.find(s => s.name === sym!.name);
                    if (existing) {
                        existing.isCVariable = true;
                    } else {
                        symbols.push({
                            name: csym.name,
                            address: sym.val,
                            isGlobal: true,
                            isZeroPage: isZP,
                            isCVariable: true,
                            segment: seg?.name || '',
                        });
                    }
                }
            }
        }

        // Process csyms to find functions and local variables
        // First pass: identify function scopes
        const scopeFunctionMap = new Map<number, { address: number; endAddress: number }>();

        for (const csym of data.csyms) {
            if (csym.sym !== undefined && csym.scope !== undefined) {
                const sym = data.syms[csym.sym];
                const scope = data.scopes[csym.scope];
                if (sym && scope && sym.val !== undefined) {
                    const spanIds = Cc65DebugInfo.toArray(scope.span);
                    if (spanIds.length > 0) {
                        const span = spanMap.get(spanIds[0]);
                        if (span && span.address !== undefined) {
                            const loc = addressToSource.get(span.address);
                            functions.push({
                                name: csym.name,
                                address: span.address,
                                addressEnd: span.address + span.size - 1,
                                source: loc?.source || '',
                                line: loc?.line || 0,
                            });
                            scopeFunctionMap.set(csym.scope, {
                                address: span.address,
                                endAddress: span.address + span.size - 1,
                            });
                        }
                    }
                }
            }
        }

        // Second pass: extract local variables (csyms with sc=auto)
        for (const csym of data.csyms) {
            if (csym.sc !== 'auto' || csym.scope === undefined) {
                continue;
            }
            if (csym.sym !== undefined) {
                continue; // has a code symbol -- this is a function, not a local
            }

            // Walk up the scope tree to find the enclosing function
            let funcScope: { address: number; endAddress: number } | undefined;
            let maxStackOffset = 0;

            // Collect max offset from siblings for stack pointer correction
            if (csym.offs !== undefined) {
                for (const sibling of data.csyms) {
                    if (sibling.scope === csym.scope && sibling.sc === 'auto' && sibling.offs !== undefined) {
                        if (-sibling.offs > maxStackOffset) {
                            maxStackOffset = -sibling.offs;
                        }
                    }
                }
            }

            // Walk up to find function scope
            let currentScope = csym.scope;
            while (currentScope !== undefined) {
                funcScope = scopeFunctionMap.get(currentScope);
                if (funcScope) break;
                const s = data.scopes[currentScope];
                if (!s || s.parent === undefined) break;
                currentScope = s.parent;
            }

            if (funcScope) {
                locals.push({
                    name: csym.name,
                    scopeId: csym.scope,
                    functionAddress: funcScope.address,
                    functionEndAddress: funcScope.endAddress,
                    stackOffset: csym.offs,
                    stackPointerOffset: maxStackOffset,
                });
            }
        }

        const segments: SegmentInfo[] = data.segs
            .filter(s => s.size > 0)
            .map(s => ({ name: s.name, start: s.start, size: s.size, type: s.type || 'rw' }));

        return { addressToSource, sourceToAddresses, symbols, functions, locals, zeropageStackPointerAddr, overlayGroups, segments };
    }

    private static parseLine(line: string): { key: string; attrs: Map<string, unknown> } | null {
        const trimmed = line.trim();
        if (trimmed.length === 0 || trimmed.startsWith('#')) return null;

        let pos = trimmed.indexOf('\t');
        if (pos < 0) pos = trimmed.indexOf(' ');
        if (pos < 0) return null;

        const key = trimmed.substring(0, pos);
        const rest = trimmed.substring(pos + 1);
        const attrs = new Map<string, unknown>();

        const elements = rest.split(',');
        for (const element of elements) {
            const eqPos = element.indexOf('=');
            if (eqPos < 0) continue;

            const k = element.substring(0, eqPos).trim();
            let v = element.substring(eqPos + 1).trim();

            // Decode value
            if (v.length >= 2 && v[0] === '"' && v[v.length - 1] === '"') {
                attrs.set(k, v.substring(1, v.length - 1));
            } else if (k === 'name' || k === 'type' || k === 'addrsize' || k === 'sc' || k === 'oname') {
                attrs.set(k, v);
            } else if (v.indexOf('+') >= 0) {
                // Array of numbers (span references like "0+5")
                const nums = v.split('+').map(s => parseInt(s, 10));
                if (nums.length === 1) {
                    attrs.set(k, nums[0]);
                } else {
                    attrs.set(k, nums);
                }
            } else if (v.startsWith('0x') || v.startsWith('0X')) {
                attrs.set(k, parseInt(v.substring(2), 16));
            } else if (v.startsWith('$')) {
                attrs.set(k, parseInt(v.substring(1), 16));
            } else if (/^\d+$/.test(v)) {
                attrs.set(k, parseInt(v, 10));
            } else {
                attrs.set(k, v);
            }
        }

        return { key, attrs };
    }

    private static toArray(val: unknown): number[] {
        if (val === undefined || val === null) return [];
        if (Array.isArray(val)) return val as number[];
        if (typeof val === 'number') return [val];
        return [];
    }

    private static isIntermediateFile(name: string): boolean {
        // cc65/CMake intermediate assembly: *.c.NNNNN.N.s
        if (/\.c\.\d+\.\d+\.s$/i.test(name)) return true;
        // Build artifact directories
        if (/CMakeFiles/i.test(name)) return true;
        // cc65 macro include files (.mac, .inc)
        if (/\.mac$/i.test(name)) return true;
        if (/\.inc$/i.test(name)) return true;
        // ca65 assembly files in cc65 library paths
        if (/[\\/]asminc[\\/]/i.test(name)) return true;
        if (/[\\/]libsrc[\\/]/i.test(name)) return true;
        return false;
    }
}
