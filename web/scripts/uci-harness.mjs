// Shared UCI plumbing for the referee (match.mjs) and the rating gauntlet
// (gauntlet.mjs): a child-process engine adapter (our .exe), an in-process
// Stockfish adapter (npm module), and the two await helpers.
import { spawn } from 'child_process';
import { createRequire } from 'module';

const require = createRequire(import.meta.url);

// A UCI child-process engine (our .exe), line-buffered.
export function makeExeAdapter(path) {
  const proc = spawn(path);
  const handlers = new Set();
  let buf = '';
  proc.stdout.on('data', (d) => {
    buf += d.toString();
    let nl;
    while ((nl = buf.indexOf('\n')) >= 0) {
      const line = buf.slice(0, nl).replace(/\r$/, '');
      buf = buf.slice(nl + 1);
      handlers.forEach((h) => h(line));
    }
  });
  proc.on('error', (e) => console.error('exe error:', e.message));
  return {
    send: (cmd) => proc.stdin.write(cmd + '\n'),
    on: (h) => handlers.add(h),
    off: (h) => handlers.delete(h),
    quit: () => {
      try {
        proc.stdin.write('quit\n');
      } catch {}
      proc.kill();
    },
  };
}

// Stockfish, in-process via the npm module.
export async function makeSfAdapter() {
  const stockfish = require('stockfish');
  const engine = await stockfish('lite-single');
  const handlers = new Set();
  engine.listener = (line) => handlers.forEach((h) => h(String(line)));
  return {
    send: (cmd) => engine.sendCommand(cmd),
    on: (h) => handlers.add(h),
    off: (h) => handlers.delete(h),
    quit: () => {
      try {
        engine.sendCommand('quit');
      } catch {}
    },
  };
}

export const waitFor = (a, re, cmd) =>
  new Promise((res) => {
    const h = (line) => {
      if (re.test(line)) {
        a.off(h);
        res();
      }
    };
    a.on(h);
    if (cmd) a.send(cmd);
  });

export const bestMove = (a, posCmd, goCmd) =>
  new Promise((res) => {
    const h = (line) => {
      const m = /^bestmove\s+(\S+)/.exec(line);
      if (m) {
        a.off(h);
        res(m[1]);
      }
    };
    a.on(h);
    a.send(posCmd);
    a.send(goCmd);
  });
