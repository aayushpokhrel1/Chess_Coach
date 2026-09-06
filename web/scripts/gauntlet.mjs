// Rating gauntlet: play each of our engine's strength levels (a fixed search
// depth) against Stockfish at fixed Skill Levels, alternating colors, and turn
// the score into an Elo estimate. This is how we replace the play feature's
// NOMINAL Elo labels with measured ones.
//
// Method: our score S = (wins + 0.5*draws) / games against an opponent of known
// Elo E gives our Elo as  E + 400*log10(S/(1-S))  (the standard logistic model).
// The opponent Elo comes from SKILL_ELO below, which is a NOMINAL anchor for
// Stockfish Skill Level (the skill->Elo map is fuzzy and source-dependent), so
// the output is "measured performance through an approximate anchor", not gospel.
// More games + more skill anchors tighten the estimate.
//
// Usage (from web/):
//   node scripts/gauntlet.mjs [--depths 1,2,3,4] [--skills 2,5] [--games 6]
//                             [--sf-ms 80] [--max-plies 200]
import { Chess } from 'chess.js';
import { makeExeAdapter, makeSfAdapter, waitFor, bestMove } from './uci-harness.mjs';

const args = process.argv.slice(2);
const arg = (name, def) => {
  const i = args.indexOf('--' + name);
  return i >= 0 ? args[i + 1] : def;
};
const list = (s) => s.split(',').map((x) => parseInt(x.trim(), 10));
const depths = list(arg('depths', '1,2,3,4'));
const skills = list(arg('skills', '2,5'));
const games = parseInt(arg('games', '6'), 10);
const sfMs = parseInt(arg('sf-ms', '80'), 10);
const maxPlies = parseInt(arg('max-plies', '200'), 10);

// NOMINAL anchor: rough Elo for a Stockfish Skill Level. Low skills only (that
// is where our weak engine is competitive). Tune these if you have better data.
const SKILL_ELO = { 0: 1100, 1: 1200, 2: 1300, 3: 1400, 4: 1500, 5: 1600, 6: 1700, 8: 1900, 10: 2100 };
const anchorElo = (s) => SKILL_ELO[s] ?? 1100 + s * 100;

// Score -> Elo gap vs the opponent, clamped so 0% / 100% do not blow up to +-inf.
function eloFromScore(S, oppElo, n) {
  const eps = 0.5 / n; // half a game, so a clean sweep still yields a finite bound
  const s = Math.min(1 - eps, Math.max(eps, S));
  return Math.round(oppElo + 400 * Math.log10(s / (1 - s)));
}

// Reset an engine between games. ucinewgame and isready must be separate
// commands: Stockfish's in-process sendCommand does not split on an embedded
// newline, so a combined "ucinewgame\nisready" never answers readyok.
async function reset(a) {
  a.send('ucinewgame');
  await waitFor(a, /readyok/, 'isready');
}

// Play one game. our plays `ourColor`; returns 'our' | 'sf' | 'draw'.
async function playGame(our, sf, ourColor, ourGo, sfGo) {
  const game = new Chess();
  const moves = [];
  while (!game.isGameOver() && moves.length < maxPlies) {
    const turn = game.turn();
    const a = turn === ourColor ? our : sf;
    const go = turn === ourColor ? ourGo : sfGo;
    const pos = moves.length ? `position startpos moves ${moves.join(' ')}` : 'position startpos';
    const uci = await bestMove(a, pos, go);
    if (!uci || uci === '0000') return turn === ourColor ? 'sf' : 'our'; // no move = loss
    let mv;
    try {
      mv = game.move({ from: uci.slice(0, 2), to: uci.slice(2, 4), promotion: uci.slice(4) || undefined });
    } catch {
      mv = null;
    }
    if (!mv) return turn === ourColor ? 'sf' : 'our'; // illegal move = loss
    moves.push(uci);
  }
  if (game.isCheckmate()) return game.turn() === ourColor ? 'sf' : 'our'; // side to move is mated
  return 'draw';
}

const our = makeExeAdapter('../engine/build/chess_engine.exe');
const sf = await makeSfAdapter();
await waitFor(our, /uciok/, 'uci');
await waitFor(sf, /uciok/, 'uci');

console.log(`Gauntlet: depths [${depths}] vs Stockfish skills [${skills}], ${games} games each, sf ${sfMs}ms/move\n`);
const rows = [];
for (const depth of depths) {
  const ourGo = `go depth ${depth}`;
  const perSkill = [];
  for (const skill of skills) {
    sf.send(`setoption name Skill Level value ${skill}`);
    let w = 0, d = 0, l = 0;
    for (let i = 0; i < games; i++) {
      await reset(our);
      await reset(sf);
      const ourColor = i % 2 === 0 ? 'w' : 'b'; // alternate colors for fairness
      const r = await playGame(our, sf, ourColor, ourGo, `go movetime ${sfMs}`);
      if (r === 'our') w++;
      else if (r === 'sf') l++;
      else d++;
      process.stdout.write(r === 'our' ? '+' : r === 'sf' ? '-' : '=');
    }
    const S = (w + 0.5 * d) / games;
    const est = eloFromScore(S, anchorElo(skill), games);
    perSkill.push({ skill, w, d, l, S, est });
    process.stdout.write(` depth ${depth} vs skill ${skill}: ${w}-${d}-${l} (${Math.round(S * 100)}%) ~${est}\n`);
  }
  const overall = Math.round(perSkill.reduce((a, p) => a + p.est, 0) / perSkill.length);
  rows.push({ depth, overall, perSkill });
}

console.log('\n=== Estimated Elo per level (depth) ===');
for (const r of rows) {
  const detail = r.perSkill.map((p) => `sk${p.skill}:${p.w}-${p.d}-${p.l}~${p.est}`).join('  ');
  console.log(`depth ${r.depth}:  ~${r.overall} Elo   [${detail}]`);
}
console.log('\n(Anchor is nominal; treat +-150 as the error bar at these game counts.)');
our.quit();
sf.quit();
process.exit(0);
