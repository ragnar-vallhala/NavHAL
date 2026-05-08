import * as vscode from 'vscode';

export class ProjectProvider implements vscode.TreeDataProvider<ProjectItem> {
  private _onDidChangeTreeData: vscode.EventEmitter<ProjectItem | undefined | null | void> =
    new vscode.EventEmitter<ProjectItem | undefined | null | void>();
  readonly onDidChangeTreeData: vscode.Event<ProjectItem | undefined | null | void> =
    this._onDidChangeTreeData.event;

  refresh(): void {
    this._onDidChangeTreeData.fire();
  }

  getTreeItem(element: ProjectItem): vscode.TreeItem {
    return element as vscode.TreeItem;
  }

  getChildren(element?: ProjectItem): Thenable<ProjectItem[]> {
    if (!element) {
      // Root items
      return Promise.resolve([
        new ProjectItem('Quick Actions', vscode.TreeItemCollapsibleState.Expanded, 'actions'),
        new ProjectItem('Project Templates', vscode.TreeItemCollapsibleState.Expanded, 'templates'),
        new ProjectItem('Tools & Settings', vscode.TreeItemCollapsibleState.Expanded, 'tools'),
      ]);
    }

    if (element.contextValue === 'actions') {
      return Promise.resolve([
        new ProjectItem('🔨 Build Project', vscode.TreeItemCollapsibleState.None, 'build', 'navhal.build'),
        new ProjectItem('⬆️ Upload/Flash', vscode.TreeItemCollapsibleState.None, 'flash', 'navhal.flash'),
        new ProjectItem('🔌 Serial Monitor', vscode.TreeItemCollapsibleState.None, 'serial', 'navhal.serial'),
        new ProjectItem('🧹 Clean Build', vscode.TreeItemCollapsibleState.None, 'clean', 'navhal.clean'),
      ]);
    }

    if (element.contextValue === 'templates') {
      return Promise.resolve([
        new ProjectItem('Blink LED', vscode.TreeItemCollapsibleState.None, 'template', 'navhal.template.blink'),
        new ProjectItem('UART Communication', vscode.TreeItemCollapsibleState.None, 'template', 'navhal.template.uart'),
        new ProjectItem('PWM Control', vscode.TreeItemCollapsibleState.None, 'template', 'navhal.template.pwm'),
        new ProjectItem('I2C Sensor', vscode.TreeItemCollapsibleState.None, 'template', 'navhal.template.i2c'),
        new ProjectItem('DMA Transfer', vscode.TreeItemCollapsibleState.None, 'template', 'navhal.template.dma'),
      ]);
    }

    if (element.contextValue === 'tools') {
      return Promise.resolve([
        new ProjectItem('📦 Install STM32 Tools', vscode.TreeItemCollapsibleState.None, 'tool', 'navhal.installStmTools'),
        new ProjectItem('📚 Open Documentation', vscode.TreeItemCollapsibleState.None, 'tool', 'navhal.docs'),
        new ProjectItem('⚙️ Project Settings', vscode.TreeItemCollapsibleState.None, 'tool', 'navhal.settings'),
      ]);
    }

    return Promise.resolve([]);
  }
}

export class ProjectItem extends vscode.TreeItem {
  public commandId?: string;

  constructor(
    public readonly label: string,
    public readonly collapsibleState: vscode.TreeItemCollapsibleState,
    public readonly contextValue: string,
    commandId?: string
  ) {
    super(label, collapsibleState);
    this.commandId = commandId;

    if (commandId) {
      this.command = {
        command: commandId,
        title: label,
        arguments: []
      };
    }

    // Set icons based on context
    switch (contextValue) {
      case 'build':
        this.iconPath = new vscode.ThemeIcon('tools');
        break;
      case 'flash':
        this.iconPath = new vscode.ThemeIcon('cloud-upload');
        break;
      case 'serial':
        this.iconPath = new vscode.ThemeIcon('terminal');
        break;
      case 'clean':
        this.iconPath = new vscode.ThemeIcon('trash');
        break;
      case 'template':
        this.iconPath = new vscode.ThemeIcon('file-code');
        break;
      case 'tool':
        this.iconPath = new vscode.ThemeIcon('wrench');
        break;
      case 'actions':
        this.iconPath = new vscode.ThemeIcon('rocket');
        break;
      case 'templates':
        this.iconPath = new vscode.ThemeIcon('symbol-folder');
        break;
      case 'tools':
        this.iconPath = new vscode.ThemeIcon('settings');
        break;
    }
  }
}
