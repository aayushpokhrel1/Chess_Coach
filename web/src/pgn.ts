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

export interface Game {
  white: string;
  black: string;
  moves: GameMove[];
}

function movesOf(chess: Chess): GameMove[] {
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

// Parse a single PGN into its moves. Throws if the PGN is invalid.
export function parsePgn(pgn: string): GameMove[] {
  const chess = new Chess();
  chess.loadPgn(pgn);
  return movesOf(chess);
}

// Split text that may contain several games into per-game PGN chunks.
export function splitPgnGames(text: string): string[] {
  const trimmed = text.trim();
  if (!trimmed) return [];
  if (!trimmed.includes('[Event')) return [trimmed]; // a single tagless game
  return trimmed
    .split(/(?=\[Event )/g)
    .map((g) => g.trim())
    .filter((g) => g.length > 0);
}

// Parse one PGN game into its players and moves. Throws if the PGN is invalid.
export function parseGame(pgn: string): Game {
  const chess = new Chess();
  chess.loadPgn(pgn);
  const h = chess.getHeaders();
  return { white: h.White ?? '', black: h.Black ?? '', moves: movesOf(chess) };
}
