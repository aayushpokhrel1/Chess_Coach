import { Chess } from 'chess.js';
import type { Quality } from './classify';

export interface ExplainInput {
  fenBefore: string;
  playedUci: string; // e.g. "d1h5" or "e7e8q"
  quality: Quality;
  bestMove: string; // best move at fenBefore (uci)
  bestIsMate: boolean; // the best line at fenBefore is a forced mate
  fenAfter: string;
  oppBest: string; // best reply at fenAfter (uci)
}

export type MistakeCategory =
  | 'none'
  | 'missed-mate'
  | 'dropped-material'
  | 'missed-capture'
  | 'other';

export const CATEGORY_LABEL: Record<MistakeCategory, string> = {
  none: '',
  'missed-mate': 'missing a forced mate',
  'dropped-material': 'hanging a piece',
  'missed-capture': 'missing a winning capture',
  other: 'a positional slip',
};

export interface Explanation {
  category: MistakeCategory;
  text: string;
}

const PIECE_NAME: Record<string, string> = {
  p: 'pawn',
  n: 'knight',
  b: 'bishop',
  r: 'rook',
  q: 'queen',
  k: 'king',
};

function uciToMove(fen: string, uci: string) {
  const chess = new Chess(fen);
  const move = {
    from: uci.slice(0, 2),
    to: uci.slice(2, 4),
    promotion: uci.slice(4) || undefined,
  };
  try {
    return chess.move(move);
  } catch {
    return null;
  }
}

function cap(q: Quality): string {
  return q.charAt(0).toUpperCase() + q.slice(1);
}

// Beginner-language note for a move, derived from the engine's own judgment.
// Returns category 'none' with empty text for best/good moves.
export function explain(input: ExplainInput): Explanation {
  if (input.quality === 'best' || input.quality === 'good') {
    return { category: 'none', text: '' };
  }

  const bestSan = uciToMove(input.fenBefore, input.bestMove)?.san ?? input.bestMove;

  // 1. Missed forced mate.
  if (input.bestIsMate) {
    return {
      category: 'missed-mate',
      text: `${cap(input.quality)}: you missed a forced mate. ${bestSan} was mate.`,
    };
  }

  // 2. Dropped material: the opponent's best reply is a capture.
  const reply = uciToMove(input.fenAfter, input.oppBest);
  if (reply && reply.captured) {
    const piece = PIECE_NAME[reply.captured] ?? 'piece';
    return {
      category: 'dropped-material',
      text: `${cap(input.quality)}: this drops material. Your opponent can play ${reply.san}, winning your ${piece} on ${reply.to}. Better was ${bestSan}.`,
    };
  }

  // 3. Missed a winning capture the engine preferred.
  const best = uciToMove(input.fenBefore, input.bestMove);
  if (best && best.captured) {
    const piece = PIECE_NAME[best.captured] ?? 'piece';
    return {
      category: 'missed-capture',
      text: `${cap(input.quality)}: you missed a stronger capture. ${best.san} wins the ${piece} on ${best.to}.`,
    };
  }

  // 4. Fallback: name the better move.
  return { category: 'other', text: `${cap(input.quality)}: a stronger move was ${bestSan}.` };
}
