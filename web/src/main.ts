import './style.css';
import { setupBoard } from './board';
import { parsePgn, type GameMove } from './pgn';

const boardEl = document.getElementById('board')!;
const board = setupBoard(boardEl);

let moves: GameMove[] = [];
let idx = -1; // -1 = start position, else index into moves (show fenAfter)

const START = 'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR';
function render() {
  const fen = idx < 0 ? START : moves[idx].fenAfter;
  board.set({ fen: fen.split(' ')[0] });
  document.getElementById('ply')!.textContent =
    idx < 0 ? 'start' : `${idx + 1}. ${moves[idx].san}`;
}

document.getElementById('load')!.addEventListener('click', () => {
  const text = (document.getElementById('pgn') as HTMLTextAreaElement).value;
  try {
    moves = parsePgn(text);
    idx = -1;
    render();
  } catch {
    alert('Could not parse that PGN.');
  }
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
