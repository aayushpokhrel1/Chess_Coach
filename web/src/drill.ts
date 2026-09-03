import { Chess } from 'chess.js';
import type { Key } from 'chessground/types';
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
export function legalDests(fen: string): Map<Key, Key[]> {
  const chess = new Chess(fen);
  const dests = new Map<Key, Key[]>();
  for (const m of chess.moves({ verbose: true })) {
    const from = m.from as Key;
    const arr = dests.get(from) ?? [];
    arr.push(m.to as Key);
    dests.set(from, arr);
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
