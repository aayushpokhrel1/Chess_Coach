import { describe, it, expect } from 'vitest';
import { summarize, type MoveRecord } from './report';

const rec = (phase: any, category: any, quality: any): MoveRecord => ({ phase, category, quality });

describe('summarize', () => {
  it('counts mistakes by phase and category', () => {
    const records: MoveRecord[] = [
      rec('opening', 'dropped-material', 'blunder'),
      rec('opening', 'dropped-material', 'mistake'),
      rec('middlegame', 'missed-capture', 'blunder'),
      rec('opening', 'none', 'best'), // a good move: not counted
    ];
    const r = summarize(2, records);
    expect(r.games).toBe(2);
    expect(r.userMoves).toBe(4);
    expect(r.mistakes).toBe(3);
    expect(r.blunders).toBe(2);
    expect(r.byPhase.opening).toBe(2);
    expect(r.byPhase.middlegame).toBe(1);
    expect(r.byCategory['dropped-material']).toBe(2);
    expect(r.headline.toLowerCase()).toContain('opening');
    expect(r.headline.toLowerCase()).toContain('hanging a piece');
  });

  it('reports clean games when there are no mistakes', () => {
    const r = summarize(1, [rec('opening', 'none', 'best')]);
    expect(r.mistakes).toBe(0);
    expect(r.headline.toLowerCase()).toContain('no mistakes');
  });
});
