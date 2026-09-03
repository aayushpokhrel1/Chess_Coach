import { describe, it, expect } from 'vitest';
import { legalDests, gradeAttempt } from './drill';

const START = 'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1';

describe('legalDests', () => {
  it('lists legal targets grouped by origin', () => {
    const d = legalDests(START);
    // 20 legal first moves spread over 10 origin squares (8 pawns + 2 knights).
    const total = [...d.values()].reduce((n, arr) => n + arr.length, 0);
    expect(total).toBe(20);
    expect(d.get('e2')).toContain('e4');
    expect(d.get('g1')).toContain('f3');
  });
});

describe('gradeAttempt', () => {
  it('a non-losing move is solved', () => {
    expect(gradeAttempt(50, 45)).toBe('solved'); // cpLoss 5
  });
  it('a small slip is close', () => {
    expect(gradeAttempt(0, -80)).toBe('inaccurate'); // cpLoss 80
  });
  it('a material-losing move fails', () => {
    expect(gradeAttempt(0, -400)).toBe('failed'); // cpLoss 400
  });
});
