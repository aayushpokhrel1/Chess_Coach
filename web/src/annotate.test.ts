import { describe, it, expect } from 'vitest';
import { buildScore } from './annotate';

const move = (san: string) => ({ san });

describe('buildScore', () => {
  it('annotates only inaccuracy, mistake and blunder', () => {
    const rows = buildScore(
      [move('e4'), move('e5'), move('Qh5'), move('Nf6')],
      [
        { quality: 'best', cpLoss: 0, explanation: 'ignored' },
        { quality: 'good', cpLoss: 12, explanation: 'ignored' },
        { quality: 'inaccuracy', cpLoss: 69, explanation: 'The queen comes out early.' },
        { quality: 'blunder', cpLoss: 10000, explanation: 'This allows Qxf7 mate.' },
      ],
    );
    expect(rows.map((r) => r.annotation)).toEqual([
      undefined,
      undefined,
      'The queen comes out early.',
      'This allows Qxf7 mate.',
    ]);
  });

  it('keeps every move as a row and preserves its ply index', () => {
    const rows = buildScore([move('e4'), move('e5')], [undefined, undefined]);
    expect(rows).toHaveLength(2);
    expect(rows.map((r) => r.index)).toEqual([0, 1]);
    expect(rows.map((r) => r.san)).toEqual(['e4', 'e5']);
  });

  it('carries cpLoss and bestLine onto annotated rows only', () => {
    const rows = buildScore(
      [move('Qh5')],
      [{ quality: 'mistake', cpLoss: 240, explanation: 'why', bestLine: 'Nf3 Nc6' }],
    );
    expect(rows[0].cpLoss).toBe(240);
    expect(rows[0].bestLine).toBe('Nf3 Nc6');
  });

  it('tolerates analyses shorter than moves', () => {
    const rows = buildScore([move('e4'), move('e5')], [
      { quality: 'blunder', cpLoss: 500, explanation: 'bad' },
    ]);
    expect(rows[1].quality).toBeUndefined();
    expect(rows[1].annotation).toBeUndefined();
  });
});