import './style.css';
import { setupBoard } from './board';
import { parsePgn, type GameMove } from './pgn';
import { Engine } from './engine';
import { scoreToCp, classify, type Quality } from './classify';
import { explain } from './explain';

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
}

let moves: GameMove[] = [];
let analyses: MoveAnalysis[] = [];
let idx = -1; // -1 = start position, else index into moves (show fenAfter)

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
  board.set({ fen: fen.split(' ')[0] });
  $('ply').textContent = idx < 0 ? 'start' : `${idx + 1}. ${moves[idx].san}`;

  // Highlight the active move.
  Array.from($('moves').children).forEach((li, i) =>
    li.classList.toggle('active', i === idx),
  );

  // Explanation panel.
  const a = idx >= 0 ? analyses[idx] : undefined;
  $('explain').textContent = a?.explanation ?? '';
}

async function analyzeGame() {
  analyses = [];
  for (let i = 0; i < moves.length; i++) {
    const before = await engine.analyze(moves[i].fenBefore, DEPTH);
    const after = await engine.analyze(moves[i].fenAfter, DEPTH);
    const evalBeforeCp = scoreToCp(before.score);
    const evalAfterMoverCp = -scoreToCp(after.score); // after: opponent to move, negate
    const { cpLoss, quality } = classify(evalBeforeCp, evalAfterMoverCp);
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
