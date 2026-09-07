import './style.css';
import { Chess } from 'chess.js';
import { setupBoard } from './board';
import { parsePgn, splitPgnGames, parseGame, type GameMove } from './pgn';
import { Engine } from './engine';
import { scoreToCp, classify, type Quality } from './classify';
import { explain, CATEGORY_LABEL } from './explain';
import { phaseOf } from './phase';
import { summarize, type MoveRecord, type Report } from './report';
import { legalDests, gradeAttempt, type Drill } from './drill';
import { fetchLichess, fetchChessCom } from './import';
import { barPercent } from './evalBar';
import { buildGraph, type Graph } from './evalGraph';
import { initPlay } from './play';

const boardEl = document.getElementById('board')!;
const board = setupBoard(boardEl);

interface MoveAnalysis {
  evalBeforeCp: number;
  evalAfterMoverCp: number;
  cpLoss: number;
  quality: Quality;
  bestMove: string; // best at fenBefore (uci)
  bestIsMate: boolean;
  oppBest: string; // best reply at fenAfter (uci)
  pvBefore: string[];
  explanation: string;
  whiteEvalCp: number; // eval of the position after this move, White's perspective
}

let moves: GameMove[] = [];
let analyses: MoveAnalysis[] = [];
let graph: Graph | null = null; // built after analysis; null hides the eval graph
let idx = -1; // -1 = start position, else index into moves (show fenAfter)
let drills: Drill[] = [];
let drillIdx = 0;

const engine = new Engine();
const DEPTH = 12;

const START = 'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR';
const $ = (id: string) => document.getElementById(id)!;

// Tabs: Play (default) and Analyze & drills. A chessground board created in a
// hidden panel sizes to 0, so redraw the panel's board when it becomes visible.
function showTab(name: 'play' | 'coach') {
  $('tab-play').hidden = name !== 'play';
  $('tab-coach').hidden = name !== 'coach';
  $('tabPlay').classList.toggle('active', name === 'play');
  $('tabCoach').classList.toggle('active', name === 'coach');
  if (name === 'coach') board.redrawAll();
  document.dispatchEvent(new CustomEvent('tab:' + name)); // play.ts redraws its board on 'tab:play'
}
$('tabPlay').addEventListener('click', () => showTab('play'));
$('tabCoach').addEventListener('click', () => showTab('coach'));

// Remember the inputs (not the analysis) across refreshes.
const SAVE_KEYS = ['pgn', 'username', 'fetchUser'] as const;
function saveSession() {
  try {
    const data: Record<string, string> = { source: ($('source') as HTMLSelectElement).value };
    for (const k of SAVE_KEYS) data[k] = ($(k) as HTMLInputElement | HTMLTextAreaElement).value;
    localStorage.setItem('chesscoach', JSON.stringify(data));
  } catch {
    /* ignore storage errors (private mode, quota) */
  }
}
function restoreSession() {
  try {
    const raw = localStorage.getItem('chesscoach');
    if (!raw) return;
    const data = JSON.parse(raw) as Record<string, string>;
    for (const k of SAVE_KEYS) if (data[k] != null) ($(k) as HTMLInputElement).value = data[k];
    if (data.source) ($('source') as HTMLSelectElement).value = data.source;
  } catch {
    /* ignore */
  }
}
restoreSession();
['pgn', 'username', 'fetchUser', 'source'].forEach((id) =>
  $(id).addEventListener('input', saveSession),
);

function buildMoveList() {
  const ol = $('moves');
  ol.innerHTML = '';
  moves.forEach((m, i) => {
    const li = document.createElement('li');
    const a = analyses[i];
    let badge = '';
    if (a) {
      const loss =
        a.quality === 'best' || a.quality === 'good'
          ? ''
          : a.cpLoss > 9999
            ? ' (mate)'
            : ` -${a.cpLoss}`;
      badge = ` <span class="badge q-${a.quality}">${a.quality}${loss}</span>`;
    }
    li.innerHTML = `<span class="san">${m.san}</span>${badge}`;
    li.addEventListener('click', () => {
      idx = i;
      render();
    });
    ol.appendChild(li);
  });
}

function render() {
  const fen = idx < 0 ? START : moves[idx].fenAfter;
  board.set({ fen: fen.split(' ')[0], viewOnly: true });
  $('ply').textContent = idx < 0 ? 'start' : `${idx + 1}. ${moves[idx].san}`;

  // Highlight the active move.
  Array.from($('moves').children).forEach((li, i) =>
    li.classList.toggle('active', i === idx),
  );

  // Explanation panel.
  const a = idx >= 0 ? analyses[idx] : undefined;
  $('explain').textContent = a?.explanation ?? '';

  // Eval bar (White fills from the bottom).
  const white = a ? barPercent(a.whiteEvalCp) : 50;
  ($('evalfill') as HTMLElement).style.height = `${white}%`;

  // Best line the engine wanted from this position (its "plan"), in SAN.
  $('bestline').textContent =
    a && a.pvBefore.length ? `Best line: ${pvToSan(moves[idx].fenBefore, a.pvBefore)}` : '';

  // Move the graph cursor to the current move.
  const cursor = document.getElementById('ga-cursor');
  if (cursor && graph && idx >= 0 && graph.nodes[idx]) {
    const x = graph.nodes[idx].x;
    cursor.setAttribute('x1', String(x));
    cursor.setAttribute('x2', String(x));
    cursor.removeAttribute('hidden');
  } else if (cursor) {
    cursor.setAttribute('hidden', '');
  }
}

// Convert an engine PV (uci moves) into a short SAN line from the given position.
function pvToSan(fenBefore: string, uci: string[], max = 6): string {
  try {
    const c = new Chess(fenBefore);
    const out: string[] = [];
    for (const u of uci.slice(0, max)) {
      const mv = c.move({ from: u.slice(0, 2), to: u.slice(2, 4), promotion: u.slice(4) || undefined });
      if (!mv) break;
      out.push(mv.san);
    }
    return out.join(' ');
  } catch {
    return '';
  }
}

// Draw the chess.com-style eval graph as inline SVG: White's territory under the
// eval line, a midline, colored dots at the moves that went wrong, and a movable
// cursor. Clicking anywhere jumps to the nearest move.
function renderGraph() {
  const host = $('evalgraph');
  if (!graph || graph.nodes.length === 0) {
    host.hidden = true;
    host.innerHTML = '';
    return;
  }
  const g = graph;
  const dots = g.markers
    .map(
      (m) =>
        `<circle class="ga-dot q-${m.quality}" cx="${m.x.toFixed(1)}" cy="${m.y.toFixed(1)}" r="4" data-i="${m.index}"><title>move ${m.index + 1}: ${m.quality}</title></circle>`,
    )
    .join('');
  host.innerHTML = `
    <svg viewBox="0 0 ${g.width} ${g.height}" preserveAspectRatio="none" class="ga-svg">
      <path class="ga-white" d="${g.whiteArea}" />
      <line class="ga-mid" x1="0" y1="${g.midY}" x2="${g.width}" y2="${g.midY}" />
      <polyline class="ga-line" points="${g.linePoints}" />
      ${dots}
      <line id="ga-cursor" class="ga-cursor" x1="0" y1="0" x2="0" y2="${g.height}" hidden />
    </svg>`;
  host.hidden = false;

  const svg = host.querySelector('svg')!;
  const jump = (clientX: number) => {
    const rect = svg.getBoundingClientRect();
    const frac = Math.min(1, Math.max(0, (clientX - rect.left) / rect.width));
    idx = Math.round(frac * (g.nodes.length - 1));
    render();
  };
  svg.addEventListener('click', (e) => jump(e.clientX));
}

async function analyzeGame() {
  analyses = [];
  for (let i = 0; i < moves.length; i++) {
    const before = await engine.analyze(moves[i].fenBefore, DEPTH);
    const after = await engine.analyze(moves[i].fenAfter, DEPTH);
    const evalBeforeCp = scoreToCp(before.score);
    const evalAfterMoverCp = -scoreToCp(after.score); // after: opponent to move, negate
    const { cpLoss, quality } = classify(evalBeforeCp, evalAfterMoverCp);
    const whiteEvalCp = moves[i].mover === 'w' ? evalAfterMoverCp : -evalAfterMoverCp;
    const ex = explain({
      fenBefore: moves[i].fenBefore,
      playedUci: moves[i].from + moves[i].to,
      quality,
      bestMove: before.bestMove,
      bestIsMate: before.score.mate !== undefined,
      fenAfter: moves[i].fenAfter,
      oppBest: after.bestMove,
    });
    analyses.push({
      evalBeforeCp,
      evalAfterMoverCp,
      cpLoss,
      quality,
      bestMove: before.bestMove,
      bestIsMate: before.score.mate !== undefined,
      oppBest: after.bestMove,
      pvBefore: before.pv,
      explanation: ex.text,
      whiteEvalCp,
    });
    $('status').textContent = `analyzing ${i + 1}/${moves.length}`;
  }
  $('status').textContent = 'analysis complete';
  graph = buildGraph(analyses.map((a) => ({ whiteEvalCp: a.whiteEvalCp, quality: a.quality })));
  renderGraph();
  buildMoveList();
  idx = -1;
  render();
}

$('load').addEventListener('click', () => {
  const text = ($('pgn') as HTMLTextAreaElement).value;
  try {
    moves = parsePgn(text);
    analyses = [];
    graph = null;
    renderGraph();
    idx = -1;
    $('status').textContent = `${moves.length} moves loaded`;
    buildMoveList();
    render();
  } catch {
    alert('Could not parse that PGN.');
  }
});
$('analyze').addEventListener('click', () => {
  if (moves.length) analyzeGame();
});

// --- Multi-game pattern report (C2) ---

async function analyzeAll() {
  const text = ($('pgn') as HTMLTextAreaElement).value;
  const user = ($('username') as HTMLInputElement).value.trim().toLowerCase();
  if (!user) {
    alert('Enter your username first (it must match the PGN White/Black tag).');
    return;
  }
  const chunks = splitPgnGames(text);
  const records: MoveRecord[] = [];
  drills = [];
  let counted = 0;
  let skipped = 0;

  for (let g = 0; g < chunks.length; g++) {
    let game;
    try {
      game = parseGame(chunks[g]);
    } catch {
      skipped++;
      continue;
    }
    const color: 'w' | 'b' | null =
      game.white.toLowerCase() === user
        ? 'w'
        : game.black.toLowerCase() === user
          ? 'b'
          : null;
    if (!color) {
      skipped++;
      continue;
    }
    counted++;
    for (const m of game.moves) {
      if (m.mover !== color) continue; // only the user's moves
      const before = await engine.analyze(m.fenBefore, DEPTH);
      const after = await engine.analyze(m.fenAfter, DEPTH);
      const { quality } = classify(scoreToCp(before.score), -scoreToCp(after.score));
      const { category } = explain({
        fenBefore: m.fenBefore,
        playedUci: m.from + m.to,
        quality,
        bestMove: before.bestMove,
        bestIsMate: before.score.mate !== undefined,
        fenAfter: m.fenAfter,
        oppBest: after.bestMove,
      });
      records.push({ phase: phaseOf(m.ply, m.fenBefore), category, quality });
      if (quality === 'mistake' || quality === 'blunder') {
        drills.push({
          fen: m.fenBefore,
          evalBeforeCp: scoreToCp(before.score),
          bestMove: before.bestMove,
          color,
          playedSan: m.san,
          category,
        });
      }
    }
    $('reportStatus').textContent = `analyzed ${counted} game(s)...`;
  }

  renderReport(summarize(counted, records), skipped);

  const startBtn = $('startDrills') as HTMLButtonElement;
  if (drills.length) {
    startBtn.textContent = `Start drills (${drills.length})`;
    startBtn.hidden = false;
  } else {
    startBtn.hidden = true;
  }
}

function renderReport(r: Report, skipped: number) {
  const phaseRows = (['opening', 'middlegame', 'endgame'] as const)
    .map((p) => `<tr><td>${p}</td><td>${r.byPhase[p]}</td></tr>`)
    .join('');
  const catRows = (['dropped-material', 'missed-mate', 'missed-capture', 'other'] as const)
    .map((c) => `<tr><td>${c}</td><td>${r.byCategory[c]}</td></tr>`)
    .join('');
  $('report').innerHTML =
    `<p class="headline">${r.headline}</p>` +
    `<p>${r.games} game(s), ${r.userMoves} of your moves, ` +
    `${r.mistakes} mistakes (${r.blunders} blunders)` +
    (skipped ? `, ${skipped} game(s) skipped (name not found / unparsable)` : '') +
    `</p>` +
    `<div class="tables"><table><caption>By phase</caption>${phaseRows}</table>` +
    `<table><caption>By category</caption>${catRows}</table></div>`;
  $('reportStatus').textContent = 'done';
}

$('analyzeAll').addEventListener('click', analyzeAll);

$('fetchGames').addEventListener('click', async () => {
  const src = ($('source') as HTMLSelectElement).value;
  const user = ($('fetchUser') as HTMLInputElement).value.trim();
  if (!user) return;
  $('fetchStatus').textContent = 'fetching...';
  try {
    const pgn = src === 'lichess' ? await fetchLichess(user, 10) : await fetchChessCom(user, 10);
    ($('pgn') as HTMLTextAreaElement).value = pgn;
    ($('username') as HTMLInputElement).value = user; // target this player in Analyze all
    $('fetchStatus').textContent = pgn ? 'loaded, now click Analyze all games' : 'no games found';
  } catch (e) {
    $('fetchStatus').textContent = `import failed: ${(e as Error).message}`;
  }
});
$('prev').addEventListener('click', () => {
  if (idx >= 0) {
    idx--;
    render();
  }
});
$('next').addEventListener('click', () => {
  if (idx < moves.length - 1) {
    idx++;
    render();
  }
});

// --- Drills from your own mistakes (C3) ---

function setDrillBoard(d: Drill) {
  const side = d.color === 'w' ? 'white' : 'black';
  board.set({
    fen: d.fen.split(' ')[0],
    orientation: side,
    turnColor: side,
    viewOnly: false,
    movable: {
      free: false,
      color: side,
      dests: legalDests(d.fen),
      events: { after: onUserMove },
    },
  });
}

function showDrill() {
  const d = drills[drillIdx];
  const side = d.color === 'w' ? 'white' : 'black';
  setDrillBoard(d);
  const label = CATEGORY_LABEL[d.category] || 'a mistake';
  $('drillPrompt').textContent =
    `You played ${d.playedSan} here (${label}). Find a better move for ${side}.`;
  $('drillFeedback').textContent = '';
  $('drillProgress').textContent = `${drillIdx + 1} / ${drills.length}`;
}

function bestSan(d: Drill): string {
  try {
    const c = new Chess(d.fen);
    return c.move({
      from: d.bestMove.slice(0, 2),
      to: d.bestMove.slice(2, 4),
      promotion: d.bestMove.slice(4) || undefined,
    }).san;
  } catch {
    return d.bestMove;
  }
}

function pickPromotion(): Promise<'q' | 'r' | 'b' | 'n'> {
  const box = $('promo') as HTMLElement;
  box.hidden = false;
  return new Promise((resolve) => {
    const onClick = (e: Event) => {
      const p = (e.target as HTMLElement).getAttribute('data-p');
      if (!p) return;
      box.hidden = true;
      box.querySelectorAll('button').forEach((b) => b.removeEventListener('click', onClick));
      resolve(p as 'q' | 'r' | 'b' | 'n');
    };
    box.querySelectorAll('button').forEach((b) => b.addEventListener('click', onClick));
  });
}

async function onUserMove(orig: string, dest: string) {
  const d = drills[drillIdx];
  const chess = new Chess(d.fen);
  const piece = chess.get(orig as never);
  const isPromo = piece && piece.type === 'p' && (dest[1] === '8' || dest[1] === '1');
  const promo = isPromo ? await pickPromotion() : undefined;
  let fenAfter: string;
  try {
    chess.move({ from: orig, to: dest, promotion: promo });
    fenAfter = chess.fen();
  } catch {
    setDrillBoard(d); // illegal drop: reset
    return;
  }
  $('drillFeedback').textContent = 'thinking...';
  const after = await engine.analyze(fenAfter, DEPTH);
  const grade = gradeAttempt(d.evalBeforeCp, -scoreToCp(after.score));
  if (grade === 'solved') {
    $('drillFeedback').textContent = 'Solved. That move holds up.';
  } else if (grade === 'inaccurate') {
    $('drillFeedback').textContent =
      'Better, but still a little loose. Try again or see the answer.';
  } else {
    $('drillFeedback').textContent = `That still loses material. The engine likes ${bestSan(d)}.`;
  }
  setDrillBoard(d); // reset so the puzzle position stays the question
}

function startDrills() {
  if (!drills.length) return;
  drillIdx = 0;
  ($('drill') as HTMLElement).hidden = false;
  showDrill();
}

$('startDrills').addEventListener('click', startDrills);
$('drillAnswer').addEventListener('click', () => {
  $('drillFeedback').textContent = `Engine's best: ${bestSan(drills[drillIdx])}.`;
});
$('drillNext').addEventListener('click', () => {
  if (drillIdx < drills.length - 1) {
    drillIdx++;
    showDrill();
  } else {
    $('drillFeedback').textContent = 'That was the last drill. Nice work.';
  }
});

// Play a game against our own WASM engine (separate from the analysis board).
initPlay();
