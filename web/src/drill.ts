import { Chess } from 'chess.js';
import { classify } from './classify';
import type { MistakeCategory } from './explain';

export interface Drill {
  fen: string; // position to solve (user to move)
  evalBeforeCp: number; // best eval available, mover's perspective
  bestMove: string; // engine best move (uci) - the answer/hint
  color: 'w' | 'b'; // side to move (the user)
  playedSan: string; // the move the user actually played
  category: MistakeCategory;
}

export type DrillGrade = 'solved' | 'inaccurate' | 'failed';

// Legal destination squares grouped by origin, for chessground's movable.dests.
export function legalDests(fen: string): Map<string, string[]> {
  const chess = new Chess(fen);
  const dests = new Map<string, string[]>();
  for (const m of chess.moves({ verbose: true })) {
    const arr = dests.get(m.from) ?? [];
    arr.push(m.to);
    dests.set(m.from, arr);
  }
  return dests;
}

// Grade an attempt from the eval before and the eval after (mover's perspective).
export function gradeAttempt(evalBeforeCp: number, evalAfterMoverCp: number): DrillGrade {
  const { quality } = classify(evalBeforeCp, evalAfterMoverCp);
  if (quality === 'best' || quality === 'good') return 'solved';
  if (quality === 'inaccuracy') return 'inaccurate';
  return 'failed';
}
