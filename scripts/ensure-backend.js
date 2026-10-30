const http = require('http');
const { spawn } = require('child_process');
const path = require('path');
const fs = require('fs');

function checkBackend() {
  return new Promise((resolve) => {
    const req = http.get('http://127.0.0.1:8080/api/health', { timeout: 1000 }, (res) => {
      resolve(res.statusCode === 200);
    });
    req.on('error', () => resolve(false));
    req.on('timeout', () => {
      req.destroy();
      resolve(false);
    });
  });
}

async function main() {
  const isRunning = await checkBackend();
  if (isRunning) {
    console.log('[\x1b[32mBackend\x1b[0m] C11 backend server is already running on http://127.0.0.1:8080');
    return;
  }

  // Detect root directory whether executed from repository root or frontend subdirectory
  let rootDir = process.cwd();
  if (fs.existsSync(path.join(rootDir, '..', 'backend', 'server.exe'))) {
    rootDir = path.resolve(rootDir, '..');
  }

  const serverExe = path.join(rootDir, 'backend', 'server.exe');

  if (!fs.existsSync(serverExe)) {
    console.log('[\x1b[33mBackend\x1b[0m] server.exe not found. Compiling backend...');
    const { execSync } = require('child_process');
    execSync('powershell -ExecutionPolicy Bypass -File backend/build.ps1', {
      cwd: rootDir,
      stdio: 'inherit',
    });
  }

  console.log('[\x1b[36mBackend\x1b[0m] Starting C11 backend server on http://127.0.0.1:8080...');
  const child = spawn(serverExe, ['8080'], {
    cwd: rootDir,
    detached: true,
    stdio: 'ignore',
  });
  child.unref();

  // Poll until backend is responsive
  for (let i = 0; i < 10; i++) {
    await new Promise((r) => setTimeout(r, 500));
    if (await checkBackend()) {
      console.log('[\x1b[32mBackend\x1b[0m] C11 backend is ready and listening on http://127.0.0.1:8080');
      return;
    }
  }

  console.warn('[\x1b[33mBackend\x1b[0m] Backend started, waiting for connection...');
}

main().catch((err) => {
  console.error('[\x1b[31mBackend\x1b[0m] Error ensuring backend:', err);
});
