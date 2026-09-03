// Referee: pits our engine (engine/build/chess_engine.exe) against Stockfish over UCI,
// with chess.js as the arbiter. Our engine plays White; Stockfish plays Black.
// Usage (from web/):
//   node scripts/match.mjs [--skill N] [--our-move-ms N] [--sf-move-ms N] [--max-plies N]
// --skill < 20 handicaps Stockfish (0 weakest, 20 full strength).
import { spawn } from 'child_process';
import { createRequire } from 'module';
import { Chess } from 'chess.js';

const require = createRequire(import.meta.url);
const args = process.argv.slice(2);
const arg = (name, def) => {
  const i = args.indexOf('--' + name);
  return i >= 0 ? args[i + 1] : def;
};
const skill = parseInt(arg('skill', '20'), 10);
const ourMs = parseInt(arg('our-move-ms', '800'), 10);
const sfMs = parseInt(arg('sf-move-ms', '300'), 10);
const maxPlies = parseInt(arg('max-plies', '160'), 10);

// A UCI child-process engine (our .exe).
function makeExeAdapter(path) {
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
async function makeSfAdapter() {
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

const waitFor = (a, re, cmd) =>
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

const bestMove = (a, posCmd, goCmd) =>
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

const our = makeExeAdapter('../engine/build/chess_engine.exe');
const sf = await makeSfAdapter();
await waitFor(our, /uciok/, 'uci');
await waitFor(sf, /uciok/, 'uci');
if (skill < 20) sf.send(`setoption name Skill Level value ${skill}`);
await waitFor(our, /readyok/, 'isready');
await waitFor(sf, /readyok/, 'isready');

const game = new Chess();
const moves = [];
console.log(
  `Our engine (White, ${ourMs}ms/move) vs Stockfish (Black, skill ${skill}, ${sfMs}ms/move)\n`,
);

while (!game.isGameOver() && moves.length < maxPlies) {
  const white = game.turn() === 'w';
  const a = white ? our : sf;
  const posCmd = moves.length ? `position startpos moves ${moves.join(' ')}` : 'position startpos';
  const uci = await bestMove(a, posCmd, `go movetime ${white ? ourMs : sfMs}`);
  if (!uci || uci === '0000') {
    console.log(`\n${white ? 'White' : 'Black'} returned no move (${uci}).`);
    break;
  }
  let mv;
  try {
    mv = game.move({ from: uci.slice(0, 2), to: uci.slice(2, 4), promotion: uci.slice(4) || undefined });
  } catch {
    mv = null;
  }
  if (!mv) {
    console.log(`\nILLEGAL move by ${white ? 'our engine' : 'Stockfish'}: ${uci}`);
    break;
  }
  moves.push(uci);
  process.stdout.write(white ? `${moves.length / 2 + 0.5 | 0}. ${mv.san}` : ` ${mv.san}  `);
}

let result;
if (game.isCheckmate())
  result =
    game.turn() === 'w' ? 'Black (Stockfish) wins by checkmate' : 'White (our engine) wins by checkmate';
else if (game.isStalemate()) result = 'Draw (stalemate)';
else if (game.isInsufficientMaterial()) result = 'Draw (insufficient material)';
else if (game.isDraw()) result = 'Draw (50-move or repetition)';
else result = `Unfinished (hit the ${maxPlies}-ply cap)`;

console.log(`\n\nResult: ${result}`);
console.log('\nPGN:\n' + game.pgn());
our.quit();
sf.quit();
process.exit(0);
