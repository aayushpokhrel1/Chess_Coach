import type { Score } from './uciParse';

export type Quality = 'best' | 'good' | 'inaccuracy' | 'mistake' | 'blunder';

const MATE_BASE = 100000;

// Fold a UCI score into a single centipawn number (mover's perspective assumed by caller).
export function scoreToCp(score: Score): number {
  if (score.mate !== undefined) {
    // Faster mates score larger in magnitude; sign follows the mate sign.
    return score.mate > 0 ? MATE_BASE - score.mate : -MATE_BASE - score.mate;
  }
  return score.cp ?? 0;
}

// cpLoss = how much worse the played move was than the best available.
// evalBeforeCp: eval of the pre-move position (mover to play).
// evalAfterMoverCp: eval of the post-move position, already negated to the mover's view.
export function classify(
  evalBeforeCp: number,
  evalAfterMoverCp: number,
): { cpLoss: number; quality: Quality } {
  let cpLoss = evalBeforeCp - evalAfterMoverCp;
  if (cpLoss < 0) cpLoss = 0; // clamp engine noise / better-than-expected
  let quality: Quality;
  if (cpLoss < 20) quality = 'best';
  else if (cpLoss < 50) quality = 'good';
  else if (cpLoss < 150) quality = 'inaccuracy';
  else if (cpLoss < 300) quality = 'mistake';
  else quality = 'blunder';
  return { cpLoss, quality };
}
