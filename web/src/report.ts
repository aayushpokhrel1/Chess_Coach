import type { Phase } from './phase';
import type { MistakeCategory } from './explain';
import { CATEGORY_LABEL } from './explain';
import type { Quality } from './classify';

export interface MoveRecord {
  phase: Phase;
  category: MistakeCategory;
  quality: Quality;
}

export interface Report {
  games: number;
  userMoves: number;
  mistakes: number; // quality mistake or blunder
  blunders: number;
  byPhase: Record<Phase, number>;
  byCategory: Record<MistakeCategory, number>;
  headline: string;
}

const isMistake = (q: Quality) => q === 'mistake' || q === 'blunder';

export function summarize(games: number, records: MoveRecord[]): Report {
  const byPhase: Record<Phase, number> = { opening: 0, middlegame: 0, endgame: 0 };
  const byCategory: Record<MistakeCategory, number> = {
    none: 0,
    'missed-mate': 0,
    'dropped-material': 0,
    'missed-capture': 0,
    other: 0,
  };
  let mistakes = 0;
  let blunders = 0;

  for (const r of records) {
    if (!isMistake(r.quality)) continue;
    mistakes++;
    if (r.quality === 'blunder') blunders++;
    byPhase[r.phase]++;
    byCategory[r.category]++;
  }

  return {
    games,
    userMoves: records.length,
    mistakes,
    blunders,
    byPhase,
    byCategory,
    headline: headlineOf(mistakes, byPhase, byCategory),
  };
}

function topKey<K extends string>(counts: Record<K, number>): K | null {
  let bestKey: K | null = null;
  let bestVal = 0;
  (Object.entries(counts) as [K, number][]).forEach(([k, v]) => {
    if (v > bestVal) {
      bestVal = v;
      bestKey = k;
    }
  });
  return bestKey;
}

function headlineOf(
  mistakes: number,
  byPhase: Record<Phase, number>,
  byCategory: Record<MistakeCategory, number>,
): string {
  if (mistakes === 0) return 'No mistakes or blunders found. Clean games!';
  const phase = topKey(byPhase) ?? 'middlegame';
  const cats: Record<string, number> = { ...byCategory };
  delete cats['none'];
  const cat = (topKey(cats) as MistakeCategory) ?? 'other';
  return `You make the most mistakes in the ${phase}, usually by ${CATEGORY_LABEL[cat]}.`;
}
