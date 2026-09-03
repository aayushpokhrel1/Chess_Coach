import { Chess } from 'chess.js';

export interface GameMove {
  ply: number; // 0-based half-move index
  san: string; // e.g. "Nf3"
  from: string; // e.g. "g1"
  to: string; // e.g. "f3"
  mover: 'w' | 'b';
  fenBefore: string; // full FEN before the move
  fenAfter: string; // full FEN after the move
}

// Parse a PGN into a flat list of moves, each carrying the FEN before and after.
// Throws if the PGN is invalid.
export function parsePgn(pgn: string): GameMove[] {
  const chess = new Chess();
  chess.loadPgn(pgn);
  return chess.history({ verbose: true }).map((m, i) => ({
    ply: i,
    san: m.san,
    from: m.from,
    to: m.to,
    mover: m.color,
    fenBefore: m.before,
    fenAfter: m.after,
  }));
}
