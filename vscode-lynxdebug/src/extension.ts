import * as vscode from 'vscode';
import * as fs from 'fs';
import { LynxDebugSession } from './lynx_debug_session';

let overlayStatusBarItem: vscode.StatusBarItem | undefined;
let activeSession: LynxDebugSession | undefined;

export function activate(context: vscode.ExtensionContext): void {
    const factory = new LynxDebugAdapterFactory();
    context.subscriptions.push(
        vscode.debug.registerDebugAdapterDescriptorFactory('lynx', factory)
    );

    const provider = new LynxConfigurationProvider();
    context.subscriptions.push(
        vscode.debug.registerDebugConfigurationProvider('lynx', provider)
    );

    // Overlay selector command
    context.subscriptions.push(
        vscode.commands.registerCommand('lynxDebug.selectOverlay', async () => {
            if (!activeSession) return;
            const debugInfo = activeSession.getDebugInfo();
            if (!debugInfo || !debugInfo.hasOverlays()) {
                vscode.window.showInformationMessage('No overlays detected in debug info.');
                return;
            }

            const groups = debugInfo.getOverlayGroups();
            const items: vscode.QuickPickItem[] = [];
            const currentName = debugInfo.getActiveOverlayName();

            for (const group of groups) {
                for (const name of group.segmentNames) {
                    items.push({
                        label: name,
                        description: name === currentName ? '(active)' : '',
                    });
                }
            }

            const picked = await vscode.window.showQuickPick(items, {
                placeHolder: 'Select the active code overlay',
            });

            if (picked) {
                debugInfo.setActiveOverlay(picked.label);
                updateOverlayStatusBar(picked.label);
                // Re-emit stopped event so VSCode re-queries stack trace
                // and repositions the editor to the correct source line
                activeSession?.refreshStoppedState();
            }
        })
    );

    // Status bar item
    overlayStatusBarItem = vscode.window.createStatusBarItem(vscode.StatusBarAlignment.Left, 50);
    overlayStatusBarItem.command = 'lynxDebug.selectOverlay';
    context.subscriptions.push(overlayStatusBarItem);

    // Show/hide status bar based on debug session
    context.subscriptions.push(
        vscode.debug.onDidStartDebugSession((session) => {
            if (session.type === 'lynx') {
                updateOverlayStatusBar(null);
            }
        })
    );
    context.subscriptions.push(
        vscode.debug.onDidTerminateDebugSession((session) => {
            if (session.type === 'lynx') {
                overlayStatusBarItem?.hide();
                activeSession = undefined;
            }
        })
    );
}

function updateOverlayStatusBar(overlayName: string | null): void {
    if (!overlayStatusBarItem) return;
    if (!activeSession) {
        overlayStatusBarItem.hide();
        return;
    }
    const debugInfo = activeSession.getDebugInfo();
    if (!debugInfo || !debugInfo.hasOverlays()) {
        overlayStatusBarItem.hide();
        return;
    }
    const name = overlayName || debugInfo.getActiveOverlayName() || 'Select Overlay';
    overlayStatusBarItem.text = `$(layers) Lynx: ${name}`;
    overlayStatusBarItem.tooltip = 'Click to select active code overlay';
    overlayStatusBarItem.show();
}

export function setActiveSession(session: LynxDebugSession | undefined): void {
    activeSession = session;
    if (session) {
        const debugInfo = session.getDebugInfo();
        updateOverlayStatusBar(debugInfo?.getActiveOverlayName() || null);
    }
}

export function deactivate(): void {
    overlayStatusBarItem?.dispose();
    overlayStatusBarItem = undefined;
    activeSession = undefined;
}

class LynxDebugAdapterFactory implements vscode.DebugAdapterDescriptorFactory {
    createDebugAdapterDescriptor(
        _session: vscode.DebugSession,
        _executable: vscode.DebugAdapterExecutable | undefined
    ): vscode.ProviderResult<vscode.DebugAdapterDescriptor> {
        return new vscode.DebugAdapterInlineImplementation(new LynxDebugSession());
    }
}

class LynxConfigurationProvider implements vscode.DebugConfigurationProvider {
    resolveDebugConfiguration(
        _folder: vscode.WorkspaceFolder | undefined,
        config: vscode.DebugConfiguration,
        _token?: vscode.CancellationToken
    ): vscode.ProviderResult<vscode.DebugConfiguration> {
        if (!config.type && !config.request && !config.name) {
            // "Just press F5" with no launch.json
            const editor = vscode.window.activeTextEditor;
            if (editor) {
                config.type = 'lynx';
                config.name = 'Launch Lynx';
                config.request = 'launch';
                config.rom = '${workspaceFolder}/game.lnx';
                config.stopOnEntry = true;
            }
        }

        // Fill in default port from settings
        if (!config.port) {
            const settings = vscode.workspace.getConfiguration('lynxDebug');
            config.port = settings.get<number>('defaultPort', 6502);
        }

        // Fill in gearlynx path from settings if not in launch config
        if (config.request === 'launch' && !config.gearlynxPath) {
            const settings = vscode.workspace.getConfiguration('lynxDebug');
            const globalPath = settings.get<string>('gearlynxPath', '');
            if (globalPath) {
                config.gearlynxPath = globalPath;
            }
        }

        return config;
    }

    resolveDebugConfigurationWithSubstitutedVariables(
        _folder: vscode.WorkspaceFolder | undefined,
        config: vscode.DebugConfiguration,
        _token?: vscode.CancellationToken
    ): vscode.ProviderResult<vscode.DebugConfiguration> {
        // Auto-detect debug file after ${workspaceFolder} etc. are resolved
        if (config.rom && !config.debugFile) {
            const baseName = config.rom.replace(/\.[^.]+$/, '');
            // Try common naming patterns:
            // game.dbg, game.lnx.dbg, game.sym, game.lnx.sym
            const candidates = [
                baseName + '.dbg',
                config.rom + '.dbg',
                baseName + '.sym',
                config.rom + '.sym',
            ];
            for (const debugPath of candidates) {
                if (fs.existsSync(debugPath)) {
                    config.debugFile = debugPath;
                    break;
                }
            }
        }

        return config;
    }
}
