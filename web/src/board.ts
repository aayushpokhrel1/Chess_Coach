import { Chessground } from 'chessground';
import type { Api } from 'chessground/api';
import 'chessground/assets/chessground.base.css';
import 'chessground/assets/chessground.brown.css';
import 'chessground/assets/chessground.cburnett.css';

const START_FEN = 'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR';

// Render a board into `el`. `fen` is a full FEN or just the piece-placement field.
export function setupBoard(el: HTMLElement, fen: string = START_FEN): Api {
  return Chessground(el, {
    fen: fen.split(' ')[0],
    viewOnly: true, // analysis board: no dragging pieces for now
    coordinates: true,
  });
}
