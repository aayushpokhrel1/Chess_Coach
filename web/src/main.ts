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
let idx = -1; // -1 = start position, else index into moves (show fenAfter)
let drills: Drill[] = [];
let drillIdx = 0;

const engine = new Engine();
const DEPTH = 12;

const START = 'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR';
const $ = (id: string) => document.getElementById(id)!;

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
  buildMoveList();
  idx = -1;
  render();
}

$('load').addEventListener('click', () => {
  const text = ($('pgn') as HTMLTextAreaElement).value;
  try {
    moves = parsePgn(text);
    analyses = [];
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

async function onUserMove(orig: string, dest: string) {
  const d = drills[drillIdx];
  const chess = new Chess(d.fen);
  const piece = chess.get(orig as never);
  const promo =
    piece && piece.type === 'p' && (dest[1] === '8' || dest[1] === '1') ? 'q' : undefined;
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
