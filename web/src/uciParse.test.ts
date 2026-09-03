import { describe, it, expect } from 'vitest';
import { parseInfo, parseBestMove } from './uciParse';

describe('parseInfo', () => {
  it('reads depth, centipawn score, and pv', () => {
    const info = parseInfo(
      'info depth 12 seldepth 15 multipv 1 score cp -34 nodes 1000 pv e2e4 e7e5 g1f3',
    )!;
    expect(info.depth).toBe(12);
    expect(info.score.cp).toBe(-34);
    expect(info.score.mate).toBeUndefined();
    expect(info.pv).toEqual(['e2e4', 'e7e5', 'g1f3']);
  });

  it('reads a mate score', () => {
    const info = parseInfo('info depth 20 score mate 3 pv d1h5 g6h5 f1c4')!;
    expect(info.score.mate).toBe(3);
    expect(info.pv[0]).toBe('d1h5');
  });

  it('returns null for non-info lines', () => {
    expect(parseInfo('readyok')).toBeNull();
  });
});

describe('parseBestMove', () => {
  it('extracts the best move', () => {
    expect(parseBestMove('bestmove e2e4 ponder e7e5')).toBe('e2e4');
  });
  it('returns null for other lines', () => {
    expect(parseBestMove('info depth 1 score cp 0')).toBeNull();
  });
});
