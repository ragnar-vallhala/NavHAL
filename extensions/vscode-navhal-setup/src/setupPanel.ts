import * as vscode from 'vscode';
import * as os from 'os';

export class SetupPanel {
  public static currentPanel: SetupPanel | undefined;
  private readonly _panel: vscode.WebviewPanel;
  private _disposables: vscode.Disposable[] = [];

  private constructor(panel: vscode.WebviewPanel) {
    this._panel = panel;

    this._panel.onDidDispose(() => this.dispose(), null, this._disposables);
    this._panel.webview.onDidReceiveMessage(
      (message) => this._handleMessage(message),
      null,
      this._disposables
    );
    this._update();
  }

  public static createOrShow() {
    const column = vscode.window.activeTextEditor
      ? vscode.window.activeTextEditor.viewColumn
      : undefined;

    if (SetupPanel.currentPanel) {
      SetupPanel.currentPanel._panel.reveal(column);
      return;
    }

    const panel = vscode.window.createWebviewPanel(
      'navhalSetup',
      'NavHAL Setup',
      column || vscode.ViewColumn.One,
      { enableScripts: true }
    );

    SetupPanel.currentPanel = new SetupPanel(panel);
  }

  private _update() {
    this._panel.webview.html = this._getHtmlContent();
  }

  private _getHtmlContent(): string {
    return `
      <!DOCTYPE html>
      <html>
      <head>
        <meta charset="UTF-8" />
        <style>
          body {
            font-family: var(--vscode-font-family);
            color: var(--vscode-foreground);
            background-color: var(--vscode-editor-background);
            padding: 20px;
            margin: 0;
          }
          h1 {
            margin-top: 0;
            color: var(--vscode-terminal-ansiCyan);
          }
          .container {
            max-width: 600px;
            margin: 0 auto;
          }
          .option-grid {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 15px;
            margin-top: 30px;
          }
          button {
            padding: 15px 20px;
            font-size: 14px;
            font-weight: bold;
            border: 1px solid var(--vscode-button-border);
            background-color: var(--vscode-button-background);
            color: var(--vscode-button-foreground);
            cursor: pointer;
            border-radius: 4px;
            text-align: center;
            transition: all 0.2s;
          }
          button:hover {
            background-color: var(--vscode-button-hoverBackground);
          }
          button:active {
            transform: scale(0.98);
          }
          .button-icon {
            display: block;
            font-size: 24px;
            margin-bottom: 8px;
          }
          .button-text {
            display: block;
            font-size: 12px;
          }
          .info {
            background-color: var(--vscode-editor-selectionBackground);
            padding: 12px;
            border-radius: 4px;
            margin-top: 20px;
            font-size: 13px;
            line-height: 1.6;
          }
          .divider {
            height: 1px;
            background-color: var(--vscode-panel-border);
            margin: 25px 0;
          }
        </style>
      </head>
      <body>
        <div class="container">
          <h1>🚁 NavHAL Project Setup</h1>
          <p>Welcome! Choose an option to get started with your NavHAL project.</p>

          <div class="option-grid">
            <button onclick="sendMessage('clone')">
              <span class="button-icon">📥</span>
              <span class="button-text">Clone NavHAL Repo</span>
            </button>
            <button onclick="sendMessage('new')">
              <span class="button-icon">✨</span>
              <span class="button-text">New Project</span>
            </button>
            <button onclick="sendMessage('open')">
              <span class="button-icon">📂</span>
              <span class="button-text">Open Project</span>
            </button>
            <button onclick="sendMessage('install')">
              <span class="button-icon">🔧</span>
              <span class="button-text">Install STM32 Tools</span>
            </button>
          </div>

          <div class="divider"></div>

          <div class="info">
            <strong>ℹ️ About NavHAL</strong><br>
            NavHAL is a Hardware Abstraction Layer for ARM Cortex-M microcontrollers.
            Use this extension to quickly set up projects, clone the repository, and install
            flashing tools for STM32 development.
          </div>
        </div>

        <script>
          const vscode = acquireVsCodeApi();

          function sendMessage(action) {
            vscode.postMessage({ command: action });
          }
        </script>
      </body>
      </html>
    `;
  }

  private async _handleMessage(message: any) {
    switch (message.command) {
      case 'clone':
        await this._handleClone();
        break;
      case 'new':
        await this._handleNewProject();
        break;
      case 'open':
        await this._handleOpenProject();
        break;
      case 'install':
        await this._handleInstallTools();
        break;
    }
  }

  private async _handleClone() {
    const repo = await vscode.window.showInputBox({
      prompt: 'Repository URL for NavHAL (git clone URL)',
      value: 'https://github.com/ragnar-vallhala/NavHAL.git'
    });
    if (!repo) return;

    const folders = vscode.workspace.workspaceFolders;
    const defaultPath = folders && folders.length > 0 ? folders[0].uri.fsPath : os.homedir();
    const target = await vscode.window.showInputBox({
      prompt: 'Target folder path to clone into',
      value: defaultPath
    });
    if (!target) return;

    const term = vscode.window.createTerminal({ name: 'NavHAL Clone' });
    term.show();
    term.sendText(`git clone ${repo} "${target}"`);
    vscode.window.showInformationMessage('Cloning NavHAL repository...');
  }

  private async _handleNewProject() {
    const projectName = await vscode.window.showInputBox({
      prompt: 'Enter project name',
      placeHolder: 'my-navhal-project'
    });
    if (!projectName) return;

    const folders = vscode.workspace.workspaceFolders;
    const defaultPath = folders && folders.length > 0 ? folders[0].uri.fsPath : os.homedir();
    const targetPath = await vscode.window.showInputBox({
      prompt: 'Choose project location',
      value: defaultPath
    });
    if (!targetPath) return;

    const term = vscode.window.createTerminal({ name: 'NavHAL New Project' });
    term.show();
    term.sendText(`mkdir -p "${targetPath}/${projectName}" && cd "${targetPath}/${projectName}" && echo "# ${projectName}" > README.md && echo "Project created successfully!"`);
    vscode.window.showInformationMessage(`Created project directory: ${projectName}`);
  }

  private async _handleOpenProject() {
    const uri = await vscode.window.showOpenDialog({
      canSelectFolders: true,
      canSelectFiles: false,
      canSelectMany: false,
      title: 'Select NavHAL project folder'
    });
    if (uri && uri.length > 0) {
      vscode.commands.executeCommand('vscode.openFolder', uri[0]);
    }
  }

  private async _handleInstallTools() {
    const platform = process.platform;
    const term = vscode.window.createTerminal({ name: 'STM32 Tools' });
    term.show();

    if (platform === 'linux') {
      term.sendText('sudo apt-get update && sudo apt-get install -y stlink-tools');
    } else if (platform === 'darwin') {
      term.sendText('brew install stlink');
    } else if (platform === 'win32') {
      term.sendText('choco install -y stlink');
    } else {
      vscode.window.showInformationMessage(`Unsupported platform: ${platform}`);
    }

    vscode.window.showInformationMessage('Installing STM32 flashing tools...');
  }

  private dispose() {
    SetupPanel.currentPanel = undefined;
    this._panel.dispose();

    while (this._disposables.length) {
      const x = this._disposables.pop();
      if (x) {
        x.dispose();
      }
    }
  }
}
