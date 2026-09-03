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
function render() {
  const fen = idx < 0 ? START : moves[idx].fenAfter;
  board.set({ fen: fen.split(' ')[0] });
  const label =
    idx < 0
      ? 'start'
      : `${idx + 1}. ${moves[idx].san}` +
        (analyses[idx] ? ` (${analyses[idx].quality}, -${analyses[idx].cpLoss}cp)` : '');
  document.getElementById('ply')!.textContent = label;
}

async function analyzeGame() {
  analyses = [];
  for (let i = 0; i < moves.length; i++) {
    const before = await engine.analyze(moves[i].fenBefore, DEPTH);
    const after = await engine.analyze(moves[i].fenAfter, DEPTH);
    const evalBeforeCp = scoreToCp(before.score);
    const evalAfterMoverCp = -scoreToCp(after.score); // after: opponent to move, negate
    const { cpLoss, quality } = classify(evalBeforeCp, evalAfterMoverCp);
    const explanation = explain({
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
      explanation,
    });
    document.getElementById('ply')!.textContent = `analyzing ${i + 1}/${moves.length}`;
  }
  idx = -1;
  render();
}

document.getElementById('load')!.addEventListener('click', () => {
  const text = (document.getElementById('pgn') as HTMLTextAreaElement).value;
  try {
    moves = parsePgn(text);
    analyses = [];
    idx = -1;
    render();
  } catch {
    alert('Could not parse that PGN.');
  }
});
document.getElementById('analyze')!.addEventListener('click', () => {
  if (moves.length) analyzeGame();
});
document.getElementById('prev')!.addEventListener('click', () => {
  if (idx >= 0) {
    idx--;
    render();
  }
});
document.getElementById('next')!.addEventListener('click', () => {
  if (idx < moves.length - 1) {
    idx++;
    render();
  }
});
