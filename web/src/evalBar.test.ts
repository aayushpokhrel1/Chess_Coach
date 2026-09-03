import { describe, it, expect } from 'vitest';
import { barPercent } from './evalBar';

describe('barPercent', () => {
  it('is 50 at equality', () => {
    expect(barPercent(0)).toBe(50);
  });
  it('rises for White and falls for Black, clamped', () => {
    expect(barPercent(300)).toBeCloseTo(65, 5);
    expect(barPercent(100000)).toBe(98); // mate-scale pegs near full
    expect(barPercent(-100000)).toBe(2);
  });
});
