import { describe, it, expect } from 'vitest';
import { buildGraph, squash, type GraphPoint } from './evalGraph';

describe('squash', () => {
  it('is zero at equality and monotonic, saturating without exceeding 1', () => {
    expect(squash(0)).toBe(0);
    expect(squash(300)).toBeGreaterThan(0);
    expect(squash(-300)).toBeLessThan(0);
    expect(squash(300)).toBeLessThan(squash(900)); // monotonic
    expect(Math.abs(squash(100000))).toBeLessThanOrEqual(1); // a mate score stays bounded
  });
});

describe('buildGraph', () => {
  const P = (cp: number, quality: GraphPoint['quality'] = 'good'): GraphPoint => ({
    whiteEvalCp: cp,
    quality,
  });

  it('places White advantage above the midline and Black below', () => {
    const g = buildGraph([P(0), P(500), P(-500)], 600, 100);
    expect(g.nodes[0].y).toBeCloseTo(g.midY, 5); // equal -> midline
    expect(g.nodes[1].y).toBeLessThan(g.midY); // White better -> higher (smaller y)
    expect(g.nodes[2].y).toBeGreaterThan(g.midY); // Black better -> lower
  });

  it('spreads x from left to right across the width', () => {
    const g = buildGraph([P(0), P(0), P(0)], 600, 100);
    expect(g.nodes[0].x).toBeCloseTo(0, 5);
    expect(g.nodes[2].x).toBeCloseTo(600, 5);
    expect(g.nodes[1].x).toBeGreaterThan(g.nodes[0].x);
    expect(g.nodes[1].x).toBeLessThan(g.nodes[2].x);
  });

  it('marks only the moves that went wrong', () => {
    const g = buildGraph([P(0, 'best'), P(-200, 'mistake'), P(-600, 'blunder'), P(-580, 'good')]);
    expect(g.markers.map((m) => m.index)).toEqual([1, 2]); // mistake + blunder, not best/good
    expect(g.markers[0].quality).toBe('mistake');
  });

  it('handles an empty game without throwing', () => {
    const g = buildGraph([]);
    expect(g.nodes).toEqual([]);
    expect(g.whiteArea).toBe('');
    expect(g.markers).toEqual([]);
  });
});
