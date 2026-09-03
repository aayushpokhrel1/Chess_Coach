import { describe, it, expect } from 'vitest';
import { parsePgn } from './pgn';

describe('parsePgn', () => {
  it('parses a short game into moves with FENs', () => {
    const moves = parsePgn('1. e4 e5 2. Nf3 Nc6 *');
    expect(moves.length).toBe(4);
    expect(moves[0].san).toBe('e4');
    expect(moves[0].from).toBe('e2');
    expect(moves[0].to).toBe('e4');
    expect(moves[0].mover).toBe('w');
    // The first move starts from the initial position.
    expect(
      moves[0].fenBefore.startsWith('rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w'),
    ).toBe(true);
    expect(moves[1].mover).toBe('b');
    expect(moves[3].san).toBe('Nc6');
  });
});
