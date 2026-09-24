import type { Quality } from './classify';

export interface AnnotatableMove {
  san: string;
}
export interface AnnotatableAnalysis {
  quality: Quality;
  cpLoss: number;
  explanation: string;
  bestLine?: string;
}
export interface ScoreRow {
  index: number;
  san: string;
  quality?: Quality;
  annotation?: string;
  cpLoss?: number;
  bestLine?: string;
}

// Good moves are silent. Only the moves that cost something get marked up, the
// way a coach annotates a score.
const ANNOTATED: Quality[] = ['inaccuracy', 'mistake', 'blunder'];

export function buildScore(
  moves: AnnotatableMove[],
  analyses: (AnnotatableAnalysis | undefined)[],
): ScoreRow[] {
  return moves.map((m, index) => {
    const a = analyses[index];
    const row: ScoreRow = { index, san: m.san, quality: a?.quality };
    if (a && ANNOTATED.includes(a.quality)) {
      row.annotation = a.explanation;
      row.cpLoss = a.cpLoss;
      row.bestLine = a.bestLine;
    }
    return row;
  });
}