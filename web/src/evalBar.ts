// White's share of the eval bar (0..100) from a centipawn score (White's view).
export function barPercent(whiteCp: number): number {
  const clamped = Math.max(-1000, Math.min(1000, whiteCp));
  const pct = 50 + (clamped / 1000) * 50;
  return Math.max(2, Math.min(98, pct));
}
