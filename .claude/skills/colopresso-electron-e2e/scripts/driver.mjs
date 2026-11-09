/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This file is part of colopresso
 *
 * Copyright (C) 2025-2026 COLOPL, Inc.
 *
 * Author: Go Kudo <g-kudo@colopl.co.jp>
 * Developed with AI (LLM) code assistance. See `NOTICE` for details.
 */

// REPL driver for the packaged colopresso Electron app, built on Playwright's _electron.
// Commands are read line by line from stdin and run one at a time, so a script can be piped in.
// playwright-core is resolved from PLAYWRIGHT_DIR (default: the current directory), so it does
// not have to be a dependency of the repository.

import { createRequire } from 'node:module';
import * as fs from 'node:fs';
import * as os from 'node:os';
import * as path from 'node:path';
import * as readline from 'node:readline';

const REPO_ROOT = path.resolve(import.meta.dirname, '../../../..');
const requireFromTools = createRequire(path.join(path.resolve(process.env.PLAYWRIGHT_DIR ?? process.cwd()), 'noop.js'));
const { _electron: electron } = requireFromTools('playwright-core');

const SHOT_DIR = path.resolve(process.env.SCREENSHOT_DIR ?? path.join(os.tmpdir(), 'colopresso-e2e-shots'));
fs.mkdirSync(SHOT_DIR, { recursive: true });

function defaultAppPath() {
  if (process.platform === 'darwin') {
    return path.join(REPO_ROOT, 'dist_build', process.arch === 'arm64' ? 'mac-arm64' : 'mac', 'colopresso.app');
  }
  if (process.platform === 'win32') {
    return path.join(REPO_ROOT, 'dist_build', process.arch === 'arm64' ? 'win-arm64-unpacked' : 'win-unpacked', 'colopresso.exe');
  }
  throw new Error('Linux Electron packaging is not supported; set COLOPRESSO_APP explicitly');
}

function resolveExecutable(appPath) {
  if (appPath.endsWith('.app')) {
    return path.join(appPath, 'Contents', 'MacOS', 'colopresso');
  }
  return appPath;
}

const DROP_ZONE = '[class*="_dropZone_"]';
const FORMAT_LABELS = { webp: 'WebP', avif: 'AVIF', png: 'PNG', pngx: 'PNG' };

let app = null;
let page = null;
let userDataDir = null;
const logs = [];

function log(line) {
  logs.push(`${new Date().toISOString().slice(11, 23)} ${line}`);
  if (logs.length > 2000) {
    logs.shift();
  }
}

function requirePage() {
  if (!page) {
    throw new Error('launch first');
  }
  return page;
}

async function processingState() {
  return requirePage().evaluate((dropZone) => {
    const zone = document.querySelector(dropZone);
    return {
      cancelVisible: Boolean(zone?.querySelector('[class*="cancelButton"]')),
      progress: document.querySelector('[class*="_progress"]')?.textContent?.trim() ?? null,
    };
  }, DROP_ZONE);
}

const COMMANDS = {
  async launch(arg) {
    if (app) {
      return console.log('already launched');
    }
    const executablePath = resolveExecutable(path.resolve(process.env.COLOPRESSO_APP ?? defaultAppPath()));
    userDataDir = arg || process.env.COLOPRESSO_E2E_USER_DATA || fs.mkdtempSync(path.join(os.tmpdir(), 'colopresso-e2e-userdata-'));
    app = await electron.launch({
      executablePath,
      args: [`--user-data-dir=${userDataDir}`],
      env: { ...process.env, ELECTRON_ENABLE_LOGGING: '1' },
      timeout: 60_000,
    });
    app.process().stdout?.on('data', (chunk) =>
      String(chunk)
        .split('\n')
        .filter(Boolean)
        .forEach((l) => log(`[main] ${l}`))
    );
    app.process().stderr?.on('data', (chunk) =>
      String(chunk)
        .split('\n')
        .filter(Boolean)
        .forEach((l) => log(`[main:err] ${l}`))
    );
    page = await app.firstWindow();
    page.on('console', (message) => log(`[renderer:${message.type()}] ${message.text()}`));
    page.on('pageerror', (error) => log(`[renderer:pageerror] ${error.message}`));
    await page.waitForSelector(DROP_ZONE, { timeout: 30_000 });
    const versions = await app.evaluate(({ app: electronApp }) => ({
      userData: electronApp.getPath('userData'),
      electron: process.versions.electron,
      chrome: process.versions.chrome,
      node: process.versions.node,
    }));
    console.log('launched', executablePath);
    console.log(JSON.stringify(versions));
  },

  async ss(name) {
    const file = path.join(SHOT_DIR, `${name || `ss-${Date.now()}`}.png`);
    await requirePage().screenshot({ path: file });
    console.log('screenshot:', file);
  },

  async format(id) {
    const label = FORMAT_LABELS[(id || '').toLowerCase()];
    if (!label) {
      throw new Error('usage: format webp|avif|png');
    }
    const result = await requirePage().evaluate((text) => {
      const button = [...document.querySelectorAll('button')].find((b) => b.textContent?.trim() === text);
      if (!button) {
        return 'NOT_FOUND';
      }
      button.click();
      return 'OK';
    }, label);
    console.log('format', label, '->', result);
  },

  async 'mock-folder'(dir) {
    const folder = path.resolve(dir);
    await app.evaluate(({ dialog }, selected) => {
      dialog.showOpenDialog = async () => ({ canceled: false, filePaths: [selected] });
    }, folder);
    console.log('folder dialog returns', folder);
  },

  async 'mock-save'(target) {
    const file = path.resolve(target);
    await app.evaluate(({ dialog }, selected) => {
      dialog.showSaveDialog = async () => ({ canceled: false, filePath: selected });
    }, file);
    console.log('save dialog returns', file);
  },

  async pick() {
    const result = await requirePage().evaluate((selector) => {
      const zone = document.querySelector(selector);
      if (!zone) {
        return 'NOT_FOUND';
      }
      zone.click();
      return 'OK';
    }, DROP_ZONE);
    console.log('pick ->', result);
  },

  async drop(args) {
    const files = args
      .split(/\s+/)
      .filter(Boolean)
      .map((file) => path.resolve(file));
    if (files.length === 0) {
      throw new Error('usage: drop <file.png> [...]');
    }
    const payload = files.map((file) => ({ name: path.basename(file), base64: fs.readFileSync(file).toString('base64') }));
    const transfer = await requirePage().evaluateHandle((items) => {
      const dataTransfer = new DataTransfer();
      for (const item of items) {
        const bytes = Uint8Array.from(atob(item.base64), (c) => c.charCodeAt(0));
        dataTransfer.items.add(new File([bytes], item.name, { type: 'image/png' }));
      }
      return dataTransfer;
    }, payload);
    for (const type of ['dragenter', 'dragover', 'drop']) {
      await requirePage().dispatchEvent(DROP_ZONE, type, { dataTransfer: transfer });
    }
    console.log('dropped', files.length, 'file(s)');
  },

  async settings() {
    const result = await requirePage().evaluate(() => {
      const button = [...document.querySelectorAll('button')].find((b) => b.textContent?.trim() === '⚙️');
      if (!button) {
        return 'NOT_FOUND';
      }
      button.click();
      return 'OK';
    });
    await requirePage().waitForSelector('#advancedModal', { timeout: 10_000 });
    console.log('settings ->', result);
  },

  async field(args) {
    const [id, ...rest] = args.split(/\s+/);
    const value = rest.join(' ');
    const result = await requirePage().evaluate(
      ({ fieldId, fieldValue }) => {
        const element = document.querySelector(`#advancedModal [data-field-id="${fieldId}"]`);
        if (!element) {
          return 'NOT_FOUND';
        }
        if (element instanceof HTMLInputElement && element.type === 'checkbox') {
          element.checked = fieldValue === 'true' || fieldValue === '1';
        } else {
          element.value = fieldValue;
        }
        element.dispatchEvent(new Event('input', { bubbles: true }));
        element.dispatchEvent(new Event('change', { bubbles: true }));
        return `OK (${element.tagName.toLowerCase()} = ${element instanceof HTMLInputElement && element.type === 'checkbox' ? element.checked : element.value})`;
      },
      { fieldId: id, fieldValue: value }
    );
    console.log('field', id, '->', result);
  },

  async apply() {
    await requirePage().click('#advancedApplyBtn');
    await requirePage().waitForTimeout(500);
    const open = await requirePage().evaluate(() => {
      const modal = document.getElementById('advancedModal');
      return Boolean(modal && getComputedStyle(modal).display !== 'none' && modal.offsetParent !== null);
    });
    console.log('apply -> modal', open ? 'still open (check field errors with `text #advancedModal`)' : 'closed');
  },

  async 'close-settings'() {
    await requirePage().evaluate(() => document.getElementById('advancedModalClose')?.click());
    console.log('settings closed');
  },

  async wait(seconds) {
    const timeoutMs = (Number(seconds) || 600) * 1000;
    const started = Date.now();
    let seenBusy = false;
    let idleSince = null;
    while (Date.now() - started < timeoutMs) {
      const state = await processingState();
      if (state.cancelVisible) {
        seenBusy = true;
        idleSince = null;
      } else if (idleSince === null) {
        idleSince = Date.now();
      }
      if (idleSince !== null && Date.now() - idleSince > (seenBusy ? 1000 : 3000)) {
        console.log(`idle after ${((Date.now() - started) / 1000).toFixed(1)}s${seenBusy ? '' : ' (never saw it busy)'}`);
        return;
      }
      await new Promise((resolve) => setTimeout(resolve, 250));
    }
    console.log('TIMEOUT waiting for idle');
  },

  async files() {
    console.log(await requirePage().evaluate(() => document.querySelector('[class*="_fileList_"]')?.innerText ?? '(no file list)'));
  },

  async status() {
    console.log(await requirePage().evaluate(() => document.querySelector('[class*="_status"]')?.innerText ?? '(no status message)'));
  },

  async text(selector) {
    console.log(await requirePage().evaluate((s) => (s ? document.querySelector(s) : document.body)?.innerText ?? '(null)', selector || null));
  },

  async eval(expression) {
    console.log(JSON.stringify(await requirePage().evaluate(expression)));
  },

  async 'main-eval'(expression) {
    console.log(JSON.stringify(await app.evaluate(new Function('electron', `return (${expression});`))));
  },

  async logs(count) {
    for (const line of logs.slice(-(Number(count) || 50))) {
      console.log(line);
    }
  },

  async windows() {
    for (const w of app.windows()) {
      console.log(' ', w.url());
    }
  },

  async sleep(seconds) {
    await new Promise((resolve) => setTimeout(resolve, (Number(seconds) || 1) * 1000));
  },

  async quit() {
    if (app) {
      await app.close().catch(() => {});
    }
    app = null;
    page = null;
  },

  help() {
    console.log('commands:', Object.keys(COMMANDS).join(', '));
  },
};

const input = process.stdin.isTTY ? fs.createReadStream(null, { fd: fs.openSync('/dev/stdin', 'r') }) : process.stdin;
const rl = readline.createInterface({ input, output: process.stdout, prompt: 'driver> ' });
const queue = [];
let running = false;
let closing = false;

async function drain() {
  if (running) {
    return;
  }
  running = true;
  while (queue.length > 0) {
    const line = queue.shift();
    const [command, ...rest] = line.split(/\s+/);
    if (!command || command.startsWith('#')) {
      continue;
    }
    console.log(`> ${line}`);
    const handler = COMMANDS[command];
    if (!handler) {
      console.log('unknown command:', command, '- try: help');
      continue;
    }
    try {
      await handler(rest.join(' '));
    } catch (error) {
      console.log('ERROR:', error.message);
    }
    if (command === 'quit') {
      process.exit(0);
    }
  }
  running = false;
  if (closing) {
    await COMMANDS.quit();
    process.exit(0);
  }
  rl.prompt();
}

rl.on('line', (line) => {
  queue.push(line.trim());
  void drain();
});
rl.on('close', () => {
  closing = true;
  void drain();
});

console.log('colopresso e2e driver - "help" for commands, "launch" to start');
rl.prompt();
