import { describe, it, expect } from 'vitest';
import { parsePgn, splitPgnGames, parseGame } from './pgn';

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

describe('splitPgnGames', () => {
  it('splits several games on the Event tag', () => {
    const two =
      '[Event "A"]\n[White "x"]\n[Black "y"]\n\n1. e4 e5 *\n\n' +
      '[Event "B"]\n[White "p"]\n[Black "q"]\n\n1. d4 d5 *';
    expect(splitPgnGames(two).length).toBe(2);
  });
  it('treats tagless movetext as a single game', () => {
    expect(splitPgnGames('1. e4 e5 *').length).toBe(1);
  });
  it('returns nothing for empty text', () => {
    expect(splitPgnGames('   ').length).toBe(0);
  });
});

describe('parseGame', () => {
  it('reads the players and the moves', () => {
    const g = parseGame('[Event "R"]\n[White "alice"]\n[Black "bob"]\n\n1. e4 e5 *');
    expect(g.white).toBe('alice');
    expect(g.black).toBe('bob');
    expect(g.moves.length).toBe(2);
    expect(g.moves[0].san).toBe('e4');
  });
});
