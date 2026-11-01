const { spawn } = require('child_process');
const http = require('http');
const path = require('path');
const fs = require('fs');

// Find project root directory
let rootDir = path.resolve(__dirname, '..');
if (!fs.existsSync(path.join(rootDir, 'backend'))) {
  rootDir = process.cwd();
}

const serverExe = path.join(rootDir, 'backend', 'server.exe');

function isPortListening(port) {
  return new Promise((resolve) => {
    const req = http.get(`http://127.0.0.1:${port}/api/health`, { timeout: 1000 }, (res) => {
      resolve(true);
    });
    req.on('error', () => resolve(false));
    req.on('timeout', () => {
      req.destroy();
      resolve(false);
    });
  });
}

async function start() {
  console.log('\x1b[35m====================================================\x1b[0m');
  console.log('\x1b[35m   Smart Parking System - Full Stack Runner         \x1b[0m');
  console.log('\x1b[35m====================================================\x1b[0m');

  let backendProc = null;
  let frontendProc = null;

  // 1. Check if C11 Backend is already running
  const backendRunning = await isPortListening(8080);
  if (backendRunning) {
    console.log('\x1b[32m[Backend]\x1b[0m C11 Backend server is already running on port 8080.');
  } else {
    // Compile backend if server.exe does not exist
    if (!fs.existsSync(serverExe)) {
      console.log('\x1b[33m[Backend]\x1b[0m server.exe not found. Compiling C11 backend...');
      const { execSync } = require('child_process');
      execSync('powershell -ExecutionPolicy Bypass -File backend/build.ps1', {
        cwd: rootDir,
        stdio: 'inherit',
      });
    }

    console.log('\x1b[36m[Backend]\x1b[0m Starting C11 REST server on http://127.0.0.1:8080 ...');
    backendProc = spawn(serverExe, ['8080'], {
      cwd: rootDir,
      stdio: 'inherit',
    });

    backendProc.on('error', (err) => {
      console.error('\x1b[31m[Backend Error]\x1b[0m Failed to start server.exe:', err.message);
    });
  }

  // 2. Launch Next.js Frontend Dev Server
  console.log('\x1b[32m[Frontend]\x1b[0m Starting Next.js Dev Server on http://localhost:3000 ...');
  const frontendDir = path.join(rootDir, 'frontend');
  const frontendModules = path.join(frontendDir, 'node_modules');

  // If node_modules does not exist (fresh clone), automatically install dependencies
  const npmCmd = process.platform === 'win32' ? 'npm.cmd' : 'npm';
  if (!fs.existsSync(frontendModules)) {
    console.log('\x1b[33m[Frontend]\x1b[0m node_modules not found. Automatically running npm install in frontend folder...');
    const { execSync } = require('child_process');
    execSync(`${npmCmd} install`, {
      cwd: frontendDir,
      stdio: 'inherit',
      shell: true,
    });
  }

  frontendProc = spawn(npmCmd, ['run', 'dev:frontend'], {
    cwd: frontendDir,
    stdio: 'inherit',
    shell: true,
  });

  frontendProc.on('error', (err) => {
    console.error('\x1b[31m[Frontend Error]\x1b[0m Failed to start Next.js:', err.message);
  });

  // Handle graceful exit on Ctrl+C
  const cleanup = () => {
    console.log('\n\x1b[33m[Dev]\x1b[0m Shutting down...');
    if (backendProc) {
      try { backendProc.kill(); } catch (e) {}
    }
    if (frontendProc) {
      try { frontendProc.kill(); } catch (e) {}
    }
    process.exit(0);
  };

  process.on('SIGINT', cleanup);
  process.on('SIGTERM', cleanup);
}

start().catch(console.error);
