import { describe, it, expect } from 'vitest';
import { scoreToCp, classify } from './classify';

describe('scoreToCp', () => {
  it('passes centipawns through', () => {
    expect(scoreToCp({ cp: 120 })).toBe(120);
    expect(scoreToCp({ cp: -80 })).toBe(-80);
  });
  it('maps mate to a large magnitude, faster mate is larger', () => {
    expect(scoreToCp({ mate: 1 })).toBeGreaterThan(scoreToCp({ mate: 5 }));
    expect(scoreToCp({ mate: -1 })).toBeLessThan(scoreToCp({ mate: -5 }));
    expect(scoreToCp({ mate: 3 })).toBeGreaterThan(10000);
  });
});

describe('classify', () => {
  it('near-best play is best', () => {
    expect(classify(50, 45).quality).toBe('best'); // cpLoss 5
  });
  it('a small slip is good', () => {
    expect(classify(0, -30).quality).toBe('good'); // cpLoss 30
  });
  it('buckets by centipawn loss', () => {
    expect(classify(0, -80).quality).toBe('inaccuracy'); // cpLoss 80
    expect(classify(0, -200).quality).toBe('mistake'); // cpLoss 200
    expect(classify(0, -500).quality).toBe('blunder'); // cpLoss 500
  });
  it('never reports negative loss', () => {
    expect(classify(0, 40).cpLoss).toBe(0); // engine noise: clamp to 0
  });
});
