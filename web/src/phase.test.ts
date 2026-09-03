import { describe, it, expect } from 'vitest';
import { phaseOf } from './phase';

const START = 'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1';

describe('phaseOf', () => {
  it('early moves are the opening', () => {
    expect(phaseOf(4, START)).toBe('opening');
  });
  it('a full board later is the middlegame', () => {
    expect(phaseOf(30, START)).toBe('middlegame');
  });
  it('few pieces later is the endgame', () => {
    // kings, one rook each, some pawns: 2 major/minor pieces.
    expect(phaseOf(30, '4k3/5ppp/8/8/8/8/5PPP/R3K2r b - - 0 30')).toBe('endgame');
  });
});
