// Play a full game against OUR WASM engine (separate from the Stockfish
// analysis path in engine.ts). A chessground board for the human, chess.js as
// the arbiter, and our engine driven over the same UCI worker contract as
// Stockfish: `position startpos moves ...` then `go depth N`, read `bestmove`.
//
// Strength (Level) and time (clock) are separate knobs. Level maps to a fixed
// search depth (our engine ignores the clock when depth is set, and low depths
// are fast), so it plays at that strength regardless of clock; the clock is a
// JS-side countdown for both sides, charged to whoever is to move.
import { Chessground } from 'chessground';
import type { Api } from 'chessground/api';
import { Chess } from 'chess.js';
import { legalDests } from './drill';

const $ = (id: string) => document.getElementById(id)!;

// Strength levels: search depth -> MEASURED Elo. Calibrated by playing each
// depth vs Stockfish skills 0 and 3, alternating colors, and converting score
// to Elo (web/scripts/gauntlet.mjs). Rough (12 games/anchor, +-150, and the
// skill->Elo anchor is itself approximate), but real, not guessed. Re-measured
// after the transposition table landed: depths 2 and 3 now score the SAME
// (~1430, within the error bar), so depth 3 is dropped (same strength, slower)
// and depth 4 (~1600, which separated cleanly this time) becomes the top level.
const LEVELS = [
  // Novice is BELOW the engine's floor (depth-1 search is already ~1130), so it
  // is weakened by playing a random legal move `random` of the time. That rate is
  // an uncalibrated guess, not a gauntlet number; tune it if it plays too strong.
  { label: 'Novice (~600)', depth: 1, random: 0.6 },
  { label: 'Beginner (~1130)', depth: 1 },
  { label: 'Intermediate (~1430)', depth: 2 },
  { label: 'Advanced (~1600)', depth: 4 },
];

// Time controls: initial ms + increment ms per move. ms 0 = unlimited (no clock).
const TIMES = [
  { label: 'Unlimited', ms: 0, inc: 0 },
  { label: '1 + 0', ms: 60_000, inc: 0 },
  { label: '3 + 2', ms: 180_000, inc: 2_000 },
  { label: '5 + 0', ms: 300_000, inc: 0 },
  { label: '10 + 0', ms: 600_000, inc: 0 },
];

function fmtClock(ms: number): string {
  if (ms <= 0) return '0:00';
  const s = Math.ceil(ms / 1000);
  const m = Math.floor(s / 60);
  const sec = s % 60;
  return `${m}:${sec.toString().padStart(2, '0')}`;
}

// One request/response over the engine worker: send the move list, ask it to
// search to `depth`, resolve with the bestmove uci string ("e2e4", "e7e8q").
function askEngine(worker: Worker, moves: string[], depth: number): Promise<string> {
  return new Promise((resolve) => {
    const onMsg = (e: MessageEvent) => {
      const m = String(e.data).match(/^bestmove (\S+)/);
      if (m) {
        worker.removeEventListener('message', onMsg);
        resolve(m[1]);
      }
    };
    worker.addEventListener('message', onMsg);
    worker.postMessage(`position startpos moves ${moves.join(' ')}`);
    worker.postMessage(`go depth ${depth}`);
  });
}

// Handshake: create the worker and resolve once it answers `uci` with `uciok`.
function bootEngine(): Promise<Worker> {
  const worker = new Worker(new URL('/engine/chesscoach-worker.js', import.meta.url), {
    type: 'module',
  });
  return new Promise((resolve) => {
    const onMsg = (e: MessageEvent) => {
      if (String(e.data).includes('uciok')) {
        worker.removeEventListener('message', onMsg);
        resolve(worker);
      }
    };
    worker.addEventListener('message', onMsg);
    worker.postMessage('uci');
  });
}

export function initPlay() {
  // Fill the Level and Time selects once.
  const levelSel = $('playLevel') as HTMLSelectElement;
  const timeSel = $('playTime') as HTMLSelectElement;
  LEVELS.forEach((l, i) => levelSel.add(new Option(l.label, String(i), i === 0, i === 0)));
  TIMES.forEach((t, i) => timeSel.add(new Option(t.label, String(i), i === 0, i === 0)));

  let board: Api | null = null;
  let worker: Worker | null = null;
  let game = new Chess();
  let human: 'white' | 'black' = 'white';
  let depth = LEVELS[0].depth; // overwritten on New game from the Level select
  let randomProb = 0;          // Novice weakening: chance to play a random legal move
  const moves: string[] = []; // uci moves so far, for `position startpos moves ...`
  let viewIdx = 0;            // board history cursor; == moves.length means the live position

  const randomMove = (g: Chess): string => {
    const ms = g.moves({ verbose: true });
    const m = ms[Math.floor(Math.random() * ms.length)];
    return m.from + m.to + (m.promotion ?? '');
  };
  const replayFen = (idx: number): string => {
    const c = new Chess();
    for (let i = 0; i < idx; i++) {
      const u = moves[i];
      c.move({ from: u.slice(0, 2), to: u.slice(2, 4), promotion: u.slice(4) || undefined });
    }
    return c.fen();
  };

  // Clock state. clock[w/b] is remaining ms; unlimited when tc.ms === 0.
  let clock = { w: 0, b: 0 };
  let inc = 0;
  let unlimited = true;
  let ticker: number | null = null;
  let lastTick = 0;
  let over = false;

  const setStatus = (t: string) => ($('playStatus').textContent = t);

  function renderClocks() {
    const youMs = human === 'white' ? clock.w : clock.b;
    const engMs = human === 'white' ? clock.b : clock.w;
    $('clockYou').textContent = unlimited ? '' : fmtClock(youMs);
    $('clockEngine').textContent = unlimited ? '' : fmtClock(engMs);
  }

  function stopTicker() {
    if (ticker !== null) {
      clearInterval(ticker);
      ticker = null;
    }
  }

  // Charge elapsed real time to whoever is to move; flag on zero.
  function tick() {
    const now = Date.now();
    const dt = now - lastTick;
    lastTick = now;
    const side = game.turn() === 'w' ? 'w' : 'b';
    clock[side] -= dt;
    if (clock[side] <= 0) {
      clock[side] = 0;
      renderClocks();
      const loserIsHuman = (side === 'w') === (human === 'white');
      endGame(loserIsHuman ? 'You lost on time.' : 'Engine lost on time. You win.');
      return;
    }
    renderClocks();
  }

  function startTicker() {
    if (unlimited || over) return;
    stopTicker();
    lastTick = Date.now();
    ticker = window.setInterval(tick, 200);
  }

  function endGame(msg: string) {
    over = true;
    stopTicker();
    setStatus(msg);
    board!.set({ movable: { color: undefined, dests: new Map() } });
    ($('resign') as HTMLButtonElement).disabled = true;
    ($('analyzeGame') as HTMLButtonElement).disabled = moves.length === 0;
  }

  function gameOverText(): string | null {
    if (!game.isGameOver()) return null;
    if (game.isCheckmate()) return game.turn() === 'w' ? 'Checkmate. Black wins.' : 'Checkmate. White wins.';
    if (game.isDraw()) return 'Draw.';
    return 'Game over.';
  }

  function updateNav() {
    ($('playPrev') as HTMLButtonElement).disabled = viewIdx <= 0;
    ($('playNext') as HTMLButtonElement).disabled = viewIdx >= moves.length;
    $('playPos').textContent = moves.length === 0 ? '' : viewIdx >= moves.length ? 'live' : `${viewIdx} / ${moves.length}`;
  }

  function render() {
    const atLive = viewIdx >= moves.length;
    const fen = atLive ? game.fen() : replayFen(viewIdx); // show a past position when reviewing
    const turn = fen.split(' ')[1] === 'w' ? 'white' : 'black';
    const humansTurn = atLive && !over && turn === human && !game.isGameOver();
    board!.set({
      fen: fen.split(' ')[0],
      turnColor: turn,
      movable: {
        free: false,
        color: humansTurn ? human : undefined,
        dests: humansTurn ? legalDests(game.fen()) : new Map(),
        events: { after: onHumanMove },
      },
    });
    renderClocks();
    updateNav();
    if (!atLive) { setStatus(`Reviewing move ${viewIdx} of ${moves.length}. Use the arrow to return to the game.`); return; }
    if (over) return; // status already set (game over or flag); do not overwrite
    const done = gameOverText();
    if (done) { endGame(done); return; }
    setStatus(humansTurn ? 'Your move.' : 'Engine thinking...');
  }

  async function onHumanMove(orig: string, dest: string) {
    // ponytail: auto-queen. Under-promotion is rare in a casual game; add a
    // picker (see main.ts pickPromotion) only if it matters.
    const piece = game.get(orig as never);
    const promo = piece?.type === 'p' && (dest[1] === '8' || dest[1] === '1') ? 'q' : undefined;
    try {
      game.move({ from: orig, to: dest, promotion: promo });
    } catch {
      render(); // illegal drop somehow: snap back
      return;
    }
    moves.push(orig + dest + (promo ?? ''));
    viewIdx = moves.length; // snap the review cursor to the live position
    if (!unlimited) clock[human === 'white' ? 'w' : 'b'] += inc; // increment after moving
    render();
    if (!over && !game.isGameOver()) await engineMove();
  }

  async function engineMove() {
    const engSide = human === 'white' ? 'b' : 'w';
    // Novice weakening: play a random legal move some of the time instead of the
    // engine's best. Otherwise ask the engine to search to the level's depth.
    const uci =
      randomProb && Math.random() < randomProb ? randomMove(game) : await askEngine(worker!, moves, depth);
    if (over) return;            // flagged/resigned while the engine was thinking
    if (uci === '0000') return;  // no move (game-over is checked before we get here)
    game.move({ from: uci.slice(0, 2), to: uci.slice(2, 4), promotion: uci.slice(4) || undefined });
    moves.push(uci);
    viewIdx = moves.length;
    if (!unlimited) clock[engSide] += inc;
    render();
  }

  async function newGame() {
    stopTicker();
    over = false;
    human = ($('playColor') as HTMLSelectElement).value === 'black' ? 'black' : 'white';
    const lvl = LEVELS[Number(levelSel.value)];
    depth = lvl.depth;
    randomProb = lvl.random ?? 0;
    const tc = TIMES[Number(timeSel.value)];
    unlimited = tc.ms === 0;
    clock = { w: tc.ms, b: tc.ms };
    inc = tc.inc;
    game = new Chess();
    moves.length = 0;
    viewIdx = 0;
    ($('resign') as HTMLButtonElement).disabled = false;
    ($('analyzeGame') as HTMLButtonElement).disabled = true;
    setStatus('Loading engine...');
    if (!worker) worker = await bootEngine();
    if (!board) board = Chessground($('playBoard'), { coordinates: true });
    board.set({ orientation: human });
    startTicker();
    render();
    if (human === 'black') await engineMove(); // engine has the first move
  }

  // "Analyze this game": hand the played game to the existing Stockfish
  // analysis flow by dropping its PGN into the loader and clicking Load/Analyze.
  function analyzeGame() {
    if (!moves.length) return;
    const c = new Chess();
    c.header(
      'Event', 'You vs ChessCoach',
      'White', human === 'white' ? 'You' : 'ChessCoach',
      'Black', human === 'black' ? 'You' : 'ChessCoach',
    );
    for (const u of moves) {
      try {
        c.move({ from: u.slice(0, 2), to: u.slice(2, 4), promotion: u.slice(4) || undefined });
      } catch { break; }
    }
    ($('pgn') as HTMLTextAreaElement).value = c.pgn();
    ($('load') as HTMLButtonElement).click();
    ($('analyze') as HTMLButtonElement).click();
    document.querySelector('.layout')?.scrollIntoView({ behavior: 'smooth', block: 'start' });
  }

  $('newGame').addEventListener('click', () => void newGame());
  $('resign').addEventListener('click', () => { if (!over) endGame('You resigned. Engine wins.'); });
  $('analyzeGame').addEventListener('click', analyzeGame);
  $('playPrev').addEventListener('click', () => { if (viewIdx > 0) { viewIdx--; render(); } });
  $('playNext').addEventListener('click', () => { if (viewIdx < moves.length) { viewIdx++; render(); } });
  // Re-fix the board's size when returning to the Play tab (chessground mis-sizes
  // if the panel was hidden while it was created or resized).
  document.addEventListener('tab:play', () => board?.redrawAll());
}
