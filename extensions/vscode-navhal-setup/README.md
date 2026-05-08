# NavHAL Project Setup — VS Code Extension

This extension provides two simple commands to help bootstrap a project that depends on NavHAL:

- `NavHAL: Clone/Setup Project` — prompts for a Git URL and target folder, then runs `git clone` in a terminal.
- `NavHAL: Install STM32 CLI Tools` — runs common platform package-manager commands to install `stlink` tools useful for flashing STM32 devices.

Build and install locally:

```bash
cd vscode-navhal-setup
npm install
npm run compile
# Install VSIX via `vsce package` then `code --install-extension <vsix>` or run in dev by pressing F5 in VS Code
```

Notes:
- The installer uses `apt`, `brew`, or `choco` depending on your OS; it runs commands in an integrated terminal so you can enter passwords and review output.
- The clone command will run `git clone` in a terminal; choose the target folder when prompted.
