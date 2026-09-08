// A/B self-play harness: play two configurations of our engine head to head
// (same exe, different eval weights set via UCI setoption) and report which is
// stronger with a confidence figure. This is how we tune the eval weights: run
// a candidate set against the current default and see whether the score moves.
//
// Method: our score S = (wA + 0.5*draws) / games gives an Elo gap of
// 400*log10(S/(1-S)) between the two configs (clamped so a clean sweep still
// yields a finite bound). LOS (likelihood A is stronger) comes from a normal
// approximation on the decisive games, using the Abramowitz-Stegun 7.1.26
// approximation for erf since JS has no Math.erf.
//
// Both engines search deterministically at a fixed depth, so from the start position
// every game would be an identical replay: a 100-game match would be just two distinct
// games (A as white, A as black) repeated, not 100 samples, and the statistics below
// would be meaningless. So each game starts from a short RANDOM opening (the same
// opening handed to both engines, and each opening is played from BOTH sides for color
// fairness). A fixed seed makes the whole run reproducible AND makes two A/B matches use
// the same openings, so comparing candidate weight sets is a paired, lower-variance test.
//
// Usage (from web/):
//   node scripts/ab.mjs [--a "Shield=8,MobKnight=6"] [--b ""] [--games 100]
//                       [--depth 6] [--max-plies 200] [--open-plies 8] [--seed 1]
import { Chess } from 'chess.js';
import { makeExeAdapter, waitFor, bestMove } from './uci-harness.mjs';

const args = process.argv.slice(2);
const arg = (name, def) => {
  const i = args.indexOf('--' + name);
  return i >= 0 ? args[i + 1] : def;
};
const aWeights = arg('a', '');
const bWeights = arg('b', '');
const games = parseInt(arg('games', '100'), 10);
const depth = parseInt(arg('depth', '6'), 10);
const maxPlies = parseInt(arg('max-plies', '200'), 10);
const openPlies = parseInt(arg('open-plies', '8'), 10);
const seed = parseInt(arg('seed', '1'), 10);

// mulberry32: a tiny seeded PRNG so opening selection is reproducible across runs.
function mulberry32(a) {
  return function () {
    a |= 0; a = (a + 0x6D2B79F5) | 0;
    let t = Math.imul(a ^ (a >>> 15), 1 | a);
    t = (t + Math.imul(t ^ (t >>> 7), 61 | t)) ^ t;
    return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
  };
}
const rng = mulberry32(seed);

// A random legal opening of up to `plies` plies from the start position, returned as a
// list of uci move strings. Both engines are handed this same opening for the game.
function randomOpening(plies) {
  const g = new Chess();
  const out = [];
  for (let i = 0; i < plies && !g.isGameOver(); i++) {
    const legal = g.moves({ verbose: true });
    const m = legal[Math.floor(rng() * legal.length)];
    g.move(m);
    out.push(m.from + m.to + (m.promotion || ''));
  }
  return out;
}

// Parse a comma-separated "Name=value" weight string into [name, intValue]
// pairs. Empty string yields no pairs. Names pass through verbatim to setoption.
function parseWeights(s) {
  if (!s) return [];
  return s.split(',').map((pair) => {
    const [name, val] = pair.split('=');
    return [name.trim(), parseInt(val.trim(), 10)];
  });
}

// Apply eval weight overrides once at startup. Our engine keeps options across
// ucinewgame, so this only needs to happen once per engine.
function applyWeights(engine, pairs) {
  for (const [name, value] of pairs) engine.send(`setoption name ${name} value ${value}`);
}

// Reset an engine between games. ucinewgame and isready must be separate
// commands: our in-process sendCommand does not split on an embedded newline,
// so a combined "ucinewgame\nisready" never answers readyok.
async function reset(a) {
  a.send('ucinewgame');
  await waitFor(a, /readyok/, 'isready');
}

// Play one game from a given opening. a plays `aColor`; returns 'a' | 'b' | 'draw'.
async function playGame(a, b, aColor, go, opening) {
  const game = new Chess();
  const moves = [];
  for (const uci of opening) {   // replay the shared opening into both the arbiter and the move list
    game.move({ from: uci.slice(0, 2), to: uci.slice(2, 4), promotion: uci.slice(4) || undefined });
    moves.push(uci);
  }
  while (!game.isGameOver() && moves.length < maxPlies) {
    const turn = game.turn();
    const eng = turn === aColor ? a : b;
    const pos = moves.length ? `position startpos moves ${moves.join(' ')}` : 'position startpos';
    const uci = await bestMove(eng, pos, go);
    if (!uci || uci === '0000') return turn === aColor ? 'b' : 'a'; // no move = loss
    let mv;
    try {
      mv = game.move({ from: uci.slice(0, 2), to: uci.slice(2, 4), promotion: uci.slice(4) || undefined });
    } catch {
      mv = null;
    }
    if (!mv) return turn === aColor ? 'b' : 'a'; // illegal move = loss
    moves.push(uci);
  }
  if (game.isCheckmate()) return game.turn() === aColor ? 'b' : 'a'; // side to move is mated
  return 'draw';
}

// Abramowitz-Stegun 7.1.26 approximation for erf(x).
function erf(x) {
  const sign = x < 0 ? -1 : 1;
  const ax = Math.abs(x);
  const t = 1 / (1 + 0.3275911 * ax);
  const y = 1 - (((((1.061405429 * t - 1.453152027) * t) + 1.421413741) * t - 0.284496736) * t + 0.254829592) * t * Math.exp(-ax * ax);
  return sign * y;
}

const engineA = makeExeAdapter('../engine/build-release/chess_engine.exe');
const engineB = makeExeAdapter('../engine/build-release/chess_engine.exe');
await waitFor(engineA, /uciok/, 'uci');
await waitFor(engineB, /uciok/, 'uci');

const aPairs = parseWeights(aWeights);
const bPairs = parseWeights(bWeights);
applyWeights(engineA, aPairs);
applyWeights(engineB, bPairs);

console.log(`A/B self-play: ${games} games, depth ${depth}, open-plies ${openPlies}, seed ${seed}`);
console.log(`  A weights: [${aWeights || 'defaults'}]`);
console.log(`  B weights: [${bWeights || 'defaults'}]`);

let wA = 0, draws = 0, wB = 0;
let opening = [];
for (let i = 0; i < games; i++) {
  await reset(engineA);
  await reset(engineB);
  if (i % 2 === 0) opening = randomOpening(openPlies); // one opening per color-pair
  const aColor = i % 2 === 0 ? 'w' : 'b';              // play the same opening from both sides
  const r = await playGame(engineA, engineB, aColor, `go depth ${depth}`, opening);
  if (r === 'a') wA++;
  else if (r === 'b') wB++;
  else draws++;
  process.stdout.write(r === 'a' ? '+' : r === 'b' ? '-' : '=');
}

const S = (wA + 0.5 * draws) / games;
const eps = 0.5 / games; // half a game, so a clean sweep still yields a finite bound
const s = Math.min(1 - eps, Math.max(eps, S));
const eloDiff = Math.round(400 * Math.log10(s / (1 - s)));
const n = wA + wB;
const los = n === 0 ? 50 : Math.round(100 * 0.5 * (1 + erf((wA - wB) / Math.sqrt(2 * n))));

console.log(`\nA vs B:  ${wA}-${draws}-${wB}  (A scored ${Math.round(S * 100)}%)  Elo diff ${eloDiff >= 0 ? '+' : ''}${eloDiff}  LOS ${los}%`);
console.log(`  A weights: [${aWeights || 'defaults'}]`);
console.log(`  B weights: [${bWeights || 'defaults'}]`);

engineA.quit();
engineB.quit();
process.exit(0);
