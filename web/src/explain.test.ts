import { describe, it, expect } from 'vitest';
import { explain } from './explain';

describe('explain', () => {
  it('names a dropped piece from the opponent best reply', () => {
    // White plays Qd1-h5?? and Black's best reply Nf6xh5 wins the queen.
    const r = explain({
      fenBefore: 'rnbqkb1r/pppp1ppp/5n2/4p3/2B1P3/8/PPPP1PPP/RNBQK1NR w KQkq - 0 4',
      playedUci: 'd1h5',
      quality: 'blunder',
      bestMove: 'b1c3',
      bestIsMate: false,
      fenAfter: 'rnbqkb1r/pppp1ppp/5n2/4p2Q/2B1P3/8/PPPP1PPP/RNB1K1NR b KQkq - 1 4',
      oppBest: 'f6h5',
    });
    expect(r.category).toBe('dropped-material');
    expect(r.text.toLowerCase()).toContain('queen');
    expect(r.text).toContain('h5');
  });

  it('reports a missed mate when the best line was mate', () => {
    const r = explain({
      fenBefore: 'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1',
      playedUci: 'a2a3',
      quality: 'blunder',
      bestMove: 'e2e4',
      bestIsMate: true,
      fenAfter: 'rnbqkbnr/pppppppp/8/8/8/P7/1PPPPPPP/RNBQKBNR b KQkq - 0 1',
      oppBest: 'e7e5',
    });
    expect(r.category).toBe('missed-mate');
    expect(r.text.toLowerCase()).toContain('mate');
  });

  it('reports a missed free capture the engine preferred', () => {
    const r = explain({
      fenBefore: '4k3/8/8/8/3q4/8/8/3RK3 w - - 0 1',
      playedUci: 'e1e2',
      quality: 'mistake',
      bestMove: 'd1d4',
      bestIsMate: false,
      fenAfter: '4k3/8/8/8/3q4/8/4K3/3R4 b - - 1 1',
      oppBest: 'd4h4', // a quiet queen move, not a capture
    });
    expect(r.category).toBe('missed-capture');
    expect(r.text.toLowerCase()).toContain('capture');
    expect(r.text.toLowerCase()).toContain('queen');
  });

  it('says nothing for a good move', () => {
    const r = explain({
      fenBefore: 'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1',
      playedUci: 'e2e4',
      quality: 'good',
      bestMove: 'e2e4',
      bestIsMate: false,
      fenAfter: 'rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq - 0 1',
      oppBest: 'e7e5',
    });
    expect(r.category).toBe('none');
    expect(r.text).toBe('');
  });
});
