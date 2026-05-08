import * as vscode from 'vscode';
import * as os from 'os';
import { SetupPanel } from './setupPanel';
import { ProjectProvider } from './projectProvider';

export function activate(context: vscode.ExtensionContext) {
  // Register tree view provider for sidebar
  const projectProvider = new ProjectProvider();
  vscode.window.registerTreeDataProvider('navhalExplorer', projectProvider);

  // Main setup panel command
  context.subscriptions.push(
    vscode.commands.registerCommand('navhal.openSetup', () => {
      SetupPanel.createOrShow();
    })
  );

  // Build command
  context.subscriptions.push(
    vscode.commands.registerCommand('navhal.build', async () => {
      const term = vscode.window.createTerminal({ name: 'NavHAL Build' });
      term.show();
      term.sendText('cd "$(pwd)" && cmake --build build');
      vscode.window.showInformationMessage('Building NavHAL project...');
    })
  );

  // Flash/Upload command
  context.subscriptions.push(
    vscode.commands.registerCommand('navhal.flash', async () => {
      const term = vscode.window.createTerminal({ name: 'NavHAL Flash' });
      term.show();
      term.sendText('st-flash write build/firmware.bin 0x08000000 || echo "Ensure STM32 device is connected."');
      vscode.window.showInformationMessage('Flashing device...');
    })
  );

  // Serial monitor command
  context.subscriptions.push(
    vscode.commands.registerCommand('navhal.serial', async () => {
      const term = vscode.window.createTerminal({ name: 'Serial Monitor' });
      term.show();
      term.sendText('screen /dev/ttyUSB0 115200 || echo "Connect a serial device first."');
    })
  );

  // Clean build command
  context.subscriptions.push(
    vscode.commands.registerCommand('navhal.clean', async () => {
      const term = vscode.window.createTerminal({ name: 'NavHAL Clean' });
      term.show();
      term.sendText('rm -rf build && mkdir build && cmake -B build');
      vscode.window.showInformationMessage('Cleaned build directory.');
    })
  );

  // Project template commands
  const templates: { [key: string]: string } = {
    'navhal.template.blink': 'blink',
    'navhal.template.uart': 'uart',
    'navhal.template.pwm': 'pwm',
    'navhal.template.i2c': 'i2c',
    'navhal.template.dma': 'dma'
  };

  Object.entries(templates).forEach(([cmdId, template]) => {
    context.subscriptions.push(
      vscode.commands.registerCommand(cmdId, async () => {
        vscode.window.showInformationMessage(`Creating ${template.toUpperCase()} template project...`);
        const projectName = await vscode.window.showInputBox({
          prompt: `Project name for ${template.toUpperCase()} example`,
          placeHolder: `my-${template}-project`
        });
        if (!projectName) return;

        const term = vscode.window.createTerminal({ name: `NavHAL ${template}` });
        term.show();
        term.sendText(`mkdir -p "${projectName}" && cd "${projectName}" && echo "# ${template.toUpperCase()} Project" > README.md && echo "Template: ${template}" > .navhal-template`);
      })
    );
  });

  // Documentation command
  context.subscriptions.push(
    vscode.commands.registerCommand('navhal.docs', async () => {
      vscode.env.openExternal(vscode.Uri.parse('https://github.com/ragnar-vallhala/NavHAL'));
    })
  );

  // Settings command
  context.subscriptions.push(
    vscode.commands.registerCommand('navhal.settings', async () => {
      vscode.commands.executeCommand('workbench.action.openSettings', 'navhal');
    })
  );

  // Clone command
  context.subscriptions.push(
    vscode.commands.registerCommand('navhal.setupClone', async () => {
      const repo = await vscode.window.showInputBox({
        prompt: 'Repository URL for NavHAL (git clone URL)',
        value: 'https://github.com/<owner>/NavHAL.git'
      });
      if (!repo) {
        vscode.window.showInformationMessage('Clone cancelled');
        return;
      }

      const folders = vscode.workspace.workspaceFolders;
      const defaultPath = folders && folders.length > 0 ? folders[0].uri.fsPath : os.homedir();
      const target = await vscode.window.showInputBox({ prompt: 'Target folder path to clone into', value: defaultPath });
      if (!target) {
        vscode.window.showInformationMessage('Clone cancelled');
        return;
      }

      const term = vscode.window.createTerminal({ name: 'NavHAL Setup' });
      term.show();
      term.sendText(`git clone ${repo} "${target}"`);
      vscode.window.showInformationMessage('Started `git clone` in the terminal.');
    })
  );

  // Install STM32 tools command
  context.subscriptions.push(
    vscode.commands.registerCommand('navhal.installStmTools', async () => {
      const platform = process.platform;
      const term = vscode.window.createTerminal({ name: 'STM32 Tools Installer' });
      term.show();

      if (platform === 'linux') {
        term.sendText('echo "Installing stlink tools (may prompt for sudo)..."');
        term.sendText('sudo apt-get update && sudo apt-get install -y stlink-tools || echo "apt install failed; please install stlink-tools manually."');
      } else if (platform === 'darwin') {
        term.sendText('echo "Installing stlink via Homebrew..."');
        term.sendText('brew install stlink || echo "brew install failed; ensure Homebrew is installed."');
      } else if (platform === 'win32') {
        term.sendText('echo "Installing stlink via Chocolatey (requires admin PowerShell)..."');
        term.sendText('choco install -y stlink || echo "choco install failed; run from elevated PowerShell or install tools manually."');
      } else {
        vscode.window.showInformationMessage(`Unsupported platform: ${platform}`);
      }

      vscode.window.showInformationMessage('Installer commands started in terminal.');
    })
  );
}

export function deactivate() {}
