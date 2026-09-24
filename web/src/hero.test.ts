import { describe, it, expect, vi, beforeEach, afterEach } from 'vitest';
import { createHeroLoop } from './hero';

const FENS = ['fen0', 'fen1', 'fen2'];

describe('createHeroLoop', () => {
  beforeEach(() => vi.useFakeTimers());
  afterEach(() => vi.useRealTimers());

  it('emits the first position immediately on start', () => {
    const onPosition = vi.fn();
    createHeroLoop({ fens: FENS, onPosition }).start();
    expect(onPosition).toHaveBeenCalledWith('fen0', 0);
    expect(onPosition).toHaveBeenCalledTimes(1);
  });

  it('advances one position per interval and wraps', () => {
    const onPosition = vi.fn();
    createHeroLoop({ fens: FENS, onPosition, intervalMs: 100 }).start();
    vi.advanceTimersByTime(300);
    expect(onPosition.mock.calls.map((c) => c[1])).toEqual([0, 1, 2, 0]);
  });

  it('stops scheduling after stop()', () => {
    const onPosition = vi.fn();
    const loop = createHeroLoop({ fens: FENS, onPosition, intervalMs: 100 });
    loop.start();
    loop.stop();
    vi.advanceTimersByTime(1000);
    expect(onPosition).toHaveBeenCalledTimes(1);
  });

  it('under reduced motion emits the final position once and schedules nothing', () => {
    const onPosition = vi.fn();
    createHeroLoop({ fens: FENS, onPosition, intervalMs: 100, reducedMotion: true }).start();
    vi.advanceTimersByTime(1000);
    expect(onPosition).toHaveBeenCalledTimes(1);
    expect(onPosition).toHaveBeenCalledWith('fen2', 2);
  });

  it('is safe to stop before start', () => {
    const loop = createHeroLoop({ fens: FENS, onPosition: vi.fn() });
    expect(() => loop.stop()).not.toThrow();
  });
});