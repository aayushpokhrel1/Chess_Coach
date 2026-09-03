# Coach Milestone 2: Pattern Detection Across Games Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Paste several of your games, tell the coach your username, and get a summary of where and how you go wrong: your mistakes and blunders broken down by game phase (opening / middlegame / endgame) and by category (hanging a piece / missing a mate / missing a capture / other), with a one-line headline insight.

**Architecture:** Extends Coach M1. New pure modules split multi-game PGN text and read the players (`pgn`), label a position's phase (`phase`), and aggregate per-move records into a report (`report`). The `explain` module is refactored to emit a structured **category** alongside its text, so mistakes can be counted by kind. The analysis flow filters each game to the user's moves (matched via the PGN `[White]`/`[Black]` tags), analyzes only those with the existing Stockfish worker, records phase + category + quality per move, and renders a report. Everything stays client-side.

**Tech Stack:** Same as M1: Vite, TypeScript, chess.js, chessground, single-threaded Stockfish (Web Worker), Vitest.

**Spec:** `docs/superpowers/specs/2026-08-27-chess-coach-design.md` (§4 Track B step 4: "Pattern detection across games ... turns analysis into coaching"). Builds on Coach M1 (`docs/superpowers/plans/2026-09-02-coach-m1-analyze-explain.md`). Step 5 (drills) is a later sub-project.

## Global Constraints

- **Client-side only**, no backend. Multi-game input is paste (several PGNs concatenated); games are split on the `[Event ...]` tag.
- **User identity from PGN tags:** the user enters a username; per game, match it (case-insensitive) against the `[White]`/`[Black]` header to find their color. Games where the username matches neither are skipped (report the skip count). Only the user's moves are analyzed and aggregated.
- **Analyze the user's moves only** (skip the opponent's positions), halving the Stockfish work per game. (`// ponytail: user-moves-only; analyze both sides only if the coach ever reports on opponents too`.)
- **Phase heuristic:** opening while `ply < 20` (first ~10 full moves); otherwise endgame when few major/minor pieces remain (Q/R/B/N count <= 6), else middlegame. (`// ponytail: ply + material heuristic; refine only if phase labels look wrong on real games`.)
- **Depth 12** as in M1 (bulk analysis of many games is slow; the value is the aggregate, so keep depth modest). (`// ponytail: fixed depth; lower for faster bulk passes if needed`.)
- **Mistake counting:** "mistakes" in the report means quality `mistake` or `blunder` (inaccuracies are not coaching-worthy noise for a beginner summary). Blunders are also counted separately.
- **Reuse M1:** `Engine`, `scoreToCp`, `classify`, `GameMove`, `parsePgn` stay; `explain`'s return type changes (see Task 2). The single-game M1 viewer keeps working.
- **No dashes (— –)** in any comment, message, or doc.
- **Coaching mode (Mix):** the pure aggregation/headline logic and the per-user-move analysis flow are the parts to understand; splitting/scaffolding is light-touch.
- **Test framework:** Vitest. New pure modules (`pgn` additions, `phase`, `report`) and the refactored `explain` are unit-tested; the multi-game flow + report UI are verified by a manual run.
- **Commands (from `web/`):** `npm run dev`, `npx vitest run`, `npx tsc --noEmit`.

---

## File Structure

```
web/src/
  pgn.ts          # MODIFY (Task 1): splitPgnGames, parseGame (+ Game type), shared mover mapper
  pgn.test.ts     # MODIFY (Task 1)
  explain.ts      # MODIFY (Task 2): return { category, text }; export MistakeCategory + CATEGORY_LABEL
  explain.test.ts # MODIFY (Task 2)
  phase.ts        # NEW (Task 3): Phase + phaseOf
  phase.test.ts   # NEW (Task 3)
  report.ts       # NEW (Task 4): MoveRecord, Report, summarize
  report.test.ts  # NEW (Task 4)
  main.ts         # MODIFY (Task 2 small; Task 5 the multi-game flow + report UI)
  index.html      # MODIFY (Task 5): username input, Analyze-all button, report panel
  style.css       # MODIFY (Task 5): report styling
```

---

## Task 1: Split multi-game PGNs and read the players

**Files:**
- Modify: `web/src/pgn.ts`, `web/src/pgn.test.ts`

**Interfaces:**
- Consumes: `chess.js`.
- Produces:
  - `interface Game { white: string; black: string; moves: GameMove[]; }`
  - `function splitPgnGames(text: string): string[]`
  - `function parseGame(pgn: string): Game`

**Theory:** Exported games (Lichess, Chess.com) are PGN blocks each beginning with an `[Event ...]` tag and carrying `[White]`/`[Black]` names. To handle many at once we split the pasted text at each `[Event` boundary, then parse each block. chess.js `getHeaders()` gives the players, and its verbose history gives the moves (same as M1). Splitting the mover-mapping into a shared helper keeps `parsePgn` (M1) and the new `parseGame` from duplicating it.

- [ ] **Step 1: Write the failing test**

Append to `web/src/pgn.test.ts`:
```ts
import { splitPgnGames, parseGame } from './pgn';

describe('splitPgnGames', () => {
  it('splits several games on the Event tag', () => {
    const two =
      '[Event "A"]\n[White "x"]\n[Black "y"]\n\n1. e4 e5 *\n\n' +
      '[Event "B"]\n[White "p"]\n[Black "q"]\n\n1. d4 d5 *';
    expect(splitPgnGames(two).length).toBe(2);
  });
  it('treats tagless movetext as a single game', () => {
    expect(splitPgnGames('1. e4 e5 *').length).toBe(1);
  });
  it('returns nothing for empty text', () => {
    expect(splitPgnGames('   ').length).toBe(0);
  });
});

describe('parseGame', () => {
  it('reads the players and the moves', () => {
    const g = parseGame('[Event "R"]\n[White "alice"]\n[Black "bob"]\n\n1. e4 e5 *');
    expect(g.white).toBe('alice');
    expect(g.black).toBe('bob');
    expect(g.moves.length).toBe(2);
    expect(g.moves[0].san).toBe('e4');
  });
});
```

- [ ] **Step 2: Run, verify it FAILS**

Run: `npx vitest run src/pgn.test.ts`
Expected: FAIL, `splitPgnGames` / `parseGame` not exported.

- [ ] **Step 3: Refactor `pgn.ts` and add the new functions**

Edit `web/src/pgn.ts` so the mover mapping is shared, and add the two functions. Replace the `parsePgn` body with a version that uses a shared helper, and add `Game`, `splitPgnGames`, `parseGame`:
```ts
import { Chess } from 'chess.js';

export interface GameMove {
  ply: number;
  san: string;
  from: string;
  to: string;
  mover: 'w' | 'b';
  fenBefore: string;
  fenAfter: string;
}

export interface Game {
  white: string;
  black: string;
  moves: GameMove[];
}

function movesOf(chess: Chess): GameMove[] {
  return chess.history({ verbose: true }).map((m, i) => ({
    ply: i,
    san: m.san,
    from: m.from,
    to: m.to,
    mover: m.color,
    fenBefore: m.before,
    fenAfter: m.after,
  }));
}

// Parse a single PGN into its moves (throws on invalid PGN).
export function parsePgn(pgn: string): GameMove[] {
  const chess = new Chess();
  chess.loadPgn(pgn);
  return movesOf(chess);
}

// Split text that may contain several games into per-game PGN chunks.
export function splitPgnGames(text: string): string[] {
  const trimmed = text.trim();
  if (!trimmed) return [];
  if (!trimmed.includes('[Event')) return [trimmed]; // a single tagless game
  return trimmed
    .split(/(?=\[Event )/g)
    .map((g) => g.trim())
    .filter((g) => g.length > 0);
}

// Parse one PGN game into its players and moves (throws on invalid PGN).
export function parseGame(pgn: string): Game {
  const chess = new Chess();
  chess.loadPgn(pgn);
  const h = chess.getHeaders();
  return { white: h.White ?? '', black: h.Black ?? '', moves: movesOf(chess) };
}
```
(If TypeScript flags `h.White`, the headers type is a string map; access with `h['White'] ?? ''`.)

- [ ] **Step 4: Run, verify it PASSES**

Run: `npx vitest run src/pgn.test.ts`
Expected: PASS (the M1 `parsePgn` test still passes; new tests pass).

- [ ] **Step 5: Commit**

```bash
git add web/src/pgn.ts web/src/pgn.test.ts
git commit -m "feat(coach): split multi-game PGNs and read players"
```

---

## Task 2: Give explanations a structured category

**Files:**
- Modify: `web/src/explain.ts`, `web/src/explain.test.ts`, `web/src/main.ts`

**Interfaces:**
- Produces:
  - `type MistakeCategory = 'none' | 'missed-mate' | 'dropped-material' | 'missed-capture' | 'other'`
  - `const CATEGORY_LABEL: Record<MistakeCategory, string>`
  - `interface Explanation { category: MistakeCategory; text: string; }`
  - `function explain(input: ExplainInput): Explanation` (return type change)

**Theory:** M1's `explain` returned a sentence. Pattern detection needs to *count* mistakes by kind, so the same branch logic now also names a machine-readable category. Returning `{ category, text }` keeps the categorization and the wording in one place (they are derived from the same engine facts), instead of duplicating the branch checks in a separate function. M1's UI just reads `.text`; M2 reads `.category`.

- [ ] **Step 1: Update the test**

Replace the assertions in `web/src/explain.test.ts` to check the object shape. Each existing case keeps its input; change the expectations:
```ts
  it('names a dropped piece from the opponent best reply', () => {
    const r = explain({ /* same blunder input as M1 */
      fenBefore: 'rnbqkb1r/pppp1ppp/5n2/4p3/2B1P3/8/PPPP1PPP/RNBQK1NR w KQkq - 0 4',
      playedUci: 'd1h5', quality: 'blunder', bestMove: 'b1c3', bestIsMate: false,
      fenAfter: 'rnbqkb1r/pppp1ppp/5n2/4p2Q/2B1P3/8/PPPP1PPP/RNB1K1NR b KQkq - 1 4',
      oppBest: 'f6h5',
    });
    expect(r.category).toBe('dropped-material');
    expect(r.text.toLowerCase()).toContain('queen');
  });

  it('reports a missed mate', () => {
    const r = explain({
      fenBefore: 'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1',
      playedUci: 'a2a3', quality: 'blunder', bestMove: 'e2e4', bestIsMate: true,
      fenAfter: 'rnbqkbnr/pppppppp/8/8/8/P7/1PPPPPPP/RNBQKBNR b KQkq - 0 1',
      oppBest: 'e7e5',
    });
    expect(r.category).toBe('missed-mate');
    expect(r.text.toLowerCase()).toContain('mate');
  });

  it('reports a missed capture', () => {
    const r = explain({
      fenBefore: '4k3/8/8/8/3q4/8/8/3RK3 w - - 0 1',
      playedUci: 'e1e2', quality: 'mistake', bestMove: 'd1d4', bestIsMate: false,
      fenAfter: '4k3/8/8/8/3q4/8/4K3/3R4 b - - 1 1', oppBest: 'd4h4',
    });
    expect(r.category).toBe('missed-capture');
    expect(r.text.toLowerCase()).toContain('capture');
  });

  it('says nothing for a good move', () => {
    const r = explain({
      fenBefore: 'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1',
      playedUci: 'e2e4', quality: 'good', bestMove: 'e2e4', bestIsMate: false,
      fenAfter: 'rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq - 0 1',
      oppBest: 'e7e5',
    });
    expect(r.category).toBe('none');
    expect(r.text).toBe('');
  });
```

- [ ] **Step 2: Run, verify it FAILS**

Run: `npx vitest run src/explain.test.ts`
Expected: FAIL (explain returns a string, not `{ category, text }`).

- [ ] **Step 3: Refactor `explain.ts`**

Change the return type and tag each branch. Keep `uciToMove`, `PIECE_NAME`, `cap` as they are; add the category type + label map and wrap returns:
```ts
export type MistakeCategory =
  | 'none'
  | 'missed-mate'
  | 'dropped-material'
  | 'missed-capture'
  | 'other';

export const CATEGORY_LABEL: Record<MistakeCategory, string> = {
  none: '',
  'missed-mate': 'missing a forced mate',
  'dropped-material': 'hanging a piece',
  'missed-capture': 'missing a winning capture',
  other: 'a positional slip',
};

export interface Explanation {
  category: MistakeCategory;
  text: string;
}

export function explain(input: ExplainInput): Explanation {
  if (input.quality === 'best' || input.quality === 'good') {
    return { category: 'none', text: '' };
  }
  const bestSan = uciToMove(input.fenBefore, input.bestMove)?.san ?? input.bestMove;

  if (input.bestIsMate) {
    return {
      category: 'missed-mate',
      text: `${cap(input.quality)}: you missed a forced mate. ${bestSan} was mate.`,
    };
  }

  const reply = uciToMove(input.fenAfter, input.oppBest);
  if (reply && reply.captured) {
    const piece = PIECE_NAME[reply.captured] ?? 'piece';
    return {
      category: 'dropped-material',
      text: `${cap(input.quality)}: this drops material. Your opponent can play ${reply.san}, winning your ${piece} on ${reply.to}. Better was ${bestSan}.`,
    };
  }

  const best = uciToMove(input.fenBefore, input.bestMove);
  if (best && best.captured) {
    const piece = PIECE_NAME[best.captured] ?? 'piece';
    return {
      category: 'missed-capture',
      text: `${cap(input.quality)}: you missed a stronger capture. ${best.san} wins the ${piece} on ${best.to}.`,
    };
  }

  return { category: 'other', text: `${cap(input.quality)}: a stronger move was ${bestSan}.` };
}
```

- [ ] **Step 4: Update the M1 call site in `main.ts`**

In `main.ts`, the analyze loop currently does `const explanation = explain({...})` and stores `explanation`. Change to use `.text`:
```ts
    const ex = explain({
      fenBefore: moves[i].fenBefore,
      playedUci: moves[i].from + moves[i].to,
      quality,
      bestMove: before.bestMove,
      bestIsMate: before.score.mate !== undefined,
      fenAfter: moves[i].fenAfter,
      oppBest: after.bestMove,
    });
    // ...
      explanation: ex.text,
```

- [ ] **Step 5: Run, verify it PASSES**

Run: `npx vitest run src/explain.test.ts && npx tsc --noEmit`
Expected: PASS and a clean typecheck (M1's UI still compiles against `.text`).

- [ ] **Step 6: Commit**

```bash
git add web/src/explain.ts web/src/explain.test.ts web/src/main.ts
git commit -m "feat(coach): explanations carry a structured category"
```

---

## Task 3: Label a position's phase

**Files:**
- Create: `web/src/phase.ts`, `web/src/phase.test.ts`

**Interfaces:**
- Produces:
  - `type Phase = 'opening' | 'middlegame' | 'endgame'`
  - `function phaseOf(ply: number, fen: string): Phase`

**Theory:** Coaching wants to know *when* mistakes happen. A purely move-number split mislabels a quick queen trade as "middlegame" and a long maneuvering game as "endgame", so we combine two cheap signals: it is the opening for the first several moves, and it is the endgame once few heavy pieces remain (count Q/R/B/N in the FEN), otherwise the middlegame. That is accurate enough for beginner patterns without simulating the board.

- [ ] **Step 1: Write the failing test**

`web/src/phase.test.ts`:
```ts
import { describe, it, expect } from 'vitest';
import { phaseOf } from './phase';

const START = 'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1';

describe('phaseOf', () => {
  it('early moves are the opening', () => {
    expect(phaseOf(4, START)).toBe('opening');
  });
  it('a full board later is the middlegame', () => {
    expect(phaseOf(30, START)).toBe('middlegame');
  });
  it('few pieces later is the endgame', () => {
    // kings, one rook each, some pawns: 2 major/minor pieces.
    expect(phaseOf(30, '4k3/5ppp/8/8/8/8/5PPP/R3K2r b - - 0 30')).toBe('endgame');
  });
});
```

- [ ] **Step 2: Run, verify it FAILS**

Run: `npx vitest run src/phase.test.ts`
Expected: FAIL, module not found.

- [ ] **Step 3: Write `phase.ts`**

`web/src/phase.ts`:
```ts
export type Phase = 'opening' | 'middlegame' | 'endgame';

// Count queens, rooks, bishops, and knights (both colors) in a FEN.
function majorMinorCount(fen: string): number {
  const placement = fen.split(' ')[0];
  return (placement.match(/[qrbnQRBN]/g) ?? []).length;
}

// Opening for the first ~10 full moves; endgame once few heavy pieces remain;
// otherwise the middlegame.
export function phaseOf(ply: number, fen: string): Phase {
  if (ply < 20) return 'opening';
  if (majorMinorCount(fen) <= 6) return 'endgame';
  return 'middlegame';
}
```

- [ ] **Step 4: Run, verify it PASSES**

Run: `npx vitest run src/phase.test.ts`
Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add web/src/phase.ts web/src/phase.test.ts
git commit -m "feat(coach): label a position's game phase"
```

---

## Task 4: Aggregate records into a report

**Files:**
- Create: `web/src/report.ts`, `web/src/report.test.ts`

**Interfaces:**
- Consumes: `Phase` (phase), `MistakeCategory` + `CATEGORY_LABEL` (explain), `Quality` (classify).
- Produces:
  - `interface MoveRecord { phase: Phase; category: MistakeCategory; quality: Quality; }`
  - `interface Report { games: number; userMoves: number; mistakes: number; blunders: number; byPhase: Record<Phase, number>; byCategory: Record<MistakeCategory, number>; headline: string; }`
  - `function summarize(games: number, records: MoveRecord[]): Report`

**Theory:** This is the coaching payoff: turn a pile of per-move records into one honest sentence and a couple of small tables. We count only mistakes and blunders (an inaccuracy is not worth nagging a beginner about), tally them by phase and by category, and build the headline from the most common of each. The function is pure (records in, report out), so the whole insight layer is unit-tested with no engine.

- [ ] **Step 1: Write the failing test**

`web/src/report.test.ts`:
```ts
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
```

- [ ] **Step 2: Run, verify it FAILS**

Run: `npx vitest run src/report.test.ts`
Expected: FAIL, module not found.

- [ ] **Step 3: Write `report.ts`**

`web/src/report.ts`:
```ts
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
```

- [ ] **Step 4: Run, verify it PASSES**

Run: `npx vitest run src/report.test.ts`
Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add web/src/report.ts web/src/report.test.ts
git commit -m "feat(coach): aggregate move records into a report"
```

---

## Task 5: Multi-game analysis flow + report UI

**Files:**
- Modify: `web/index.html`, `web/src/main.ts`, `web/src/style.css`

**Interfaces:**
- Consumes: `splitPgnGames`, `parseGame` (Task 1), `explain` (Task 2), `phaseOf` (Task 3), `summarize`, `type MoveRecord`, `type Report` (Task 4), plus M1's `Engine`, `scoreToCp`, `classify`.
- Produces: the report UI (no new exported functions).

**Theory:** This wires the pure pieces to the engine and the page. For each pasted game we read the players, decide the user's color from the username, and analyze just the user's moves: `fenBefore` gives the best eval available, `fenAfter` (negated) gives what they actually got, `classify` grades it, `explain` categorizes it, and `phaseOf` places it. Each becomes a `MoveRecord`; `summarize` turns the pile into the report the page shows.

- [ ] **Step 1: Add the username input, button, and report panel**

In `index.html`, add above the `layout` div (below the existing controls):
```html
<div class="controls">
  <input id="username" placeholder="Your username (as in the PGN)" />
  <button id="analyzeAll">Analyze all games</button>
  <span id="reportStatus"></span>
</div>
<div id="report" class="report"></div>
```

- [ ] **Step 2: Implement the multi-game flow in `main.ts`**

Add imports and an `analyzeAll` function; wire the button. Use the existing `engine`, `DEPTH`, `scoreToCp`, `classify`:
```ts
import { splitPgnGames, parseGame } from './pgn';
import { phaseOf } from './phase';
import { summarize, type MoveRecord } from './report';

async function analyzeAll() {
  const text = ($('pgn') as HTMLTextAreaElement).value;
  const user = ($('username') as HTMLInputElement).value.trim().toLowerCase();
  if (!user) {
    alert('Enter your username first (it must match the PGN White/Black tag).');
    return;
  }
  const chunks = splitPgnGames(text);
  const records: MoveRecord[] = [];
  let counted = 0;
  let skipped = 0;

  for (let g = 0; g < chunks.length; g++) {
    let game;
    try {
      game = parseGame(chunks[g]);
    } catch {
      skipped++;
      continue;
    }
    const color: 'w' | 'b' | null =
      game.white.toLowerCase() === user ? 'w' : game.black.toLowerCase() === user ? 'b' : null;
    if (!color) {
      skipped++;
      continue;
    }
    counted++;
    for (const m of game.moves) {
      if (m.mover !== color) continue; // only the user's moves
      const before = await engine.analyze(m.fenBefore, DEPTH);
      const after = await engine.analyze(m.fenAfter, DEPTH);
      const { cpLoss, quality } = classify(scoreToCp(before.score), -scoreToCp(after.score));
      const { category } = explain({
        fenBefore: m.fenBefore,
        playedUci: m.from + m.to,
        quality,
        bestMove: before.bestMove,
        bestIsMate: before.score.mate !== undefined,
        fenAfter: m.fenAfter,
        oppBest: after.bestMove,
      });
      void cpLoss;
      records.push({ phase: phaseOf(m.ply, m.fenBefore), category, quality });
    }
    $('reportStatus').textContent = `analyzed ${counted} game(s)...`;
  }

  renderReport(summarize(counted, records), skipped);
}

function renderReport(r: import('./report').Report, skipped: number) {
  const phaseRows = (['opening', 'middlegame', 'endgame'] as const)
    .map((p) => `<tr><td>${p}</td><td>${r.byPhase[p]}</td></tr>`)
    .join('');
  const catRows = (['dropped-material', 'missed-mate', 'missed-capture', 'other'] as const)
    .map((c) => `<tr><td>${c}</td><td>${r.byCategory[c]}</td></tr>`)
    .join('');
  $('report').innerHTML =
    `<p class="headline">${r.headline}</p>` +
    `<p>${r.games} game(s), ${r.userMoves} of your moves, ` +
    `${r.mistakes} mistakes (${r.blunders} blunders)` +
    (skipped ? `, ${skipped} game(s) skipped (name not found / unparsable)` : '') +
    `</p>` +
    `<div class="tables"><table><caption>By phase</caption>${phaseRows}</table>` +
    `<table><caption>By category</caption>${catRows}</table></div>`;
  $('reportStatus').textContent = 'done';
}

$('analyzeAll').addEventListener('click', analyzeAll);
```

- [ ] **Step 3: Style the report**

Add to `style.css`:
```css
.report { margin-top: 1rem; max-width: 640px; }
.report .headline { font-size: 1.1rem; font-weight: 600; }
.report .tables { display: flex; gap: 2rem; }
.report table { border-collapse: collapse; }
.report caption { text-align: left; font-weight: 600; margin-bottom: 0.25rem; }
.report td { padding: 2px 12px 2px 0; }
```

- [ ] **Step 4: Verify end to end (manual)**

Run `npm run dev`. Paste two or three of your own exported games into the PGN box, enter your username exactly as it appears in the tags, and click "Analyze all games". Expected: after the progress counter finishes, the report shows a headline (for example "You make the most mistakes in the opening, usually by hanging a piece."), your total moves and mistake/blunder counts, and the by-phase and by-category tables. Games where your name is not found are reported as skipped. Confirm `npx vitest run` and `npx tsc --noEmit` are both clean.

- [ ] **Step 5: Commit**

```bash
git add web/index.html web/src/main.ts web/src/style.css
git commit -m "feat(coach): multi-game analysis with a pattern report"
```

---

## Self-Review Notes

- **Spec coverage:** implements Track B step 4 (pattern detection across games): multi-game import (Task 1), user identity via PGN tags (Task 5), per-mistake category (Task 2) and phase (Task 3), aggregated into a report with a headline (Task 4, rendered in Task 5). Step 5 (drills) is out of scope but reuses this `MoveRecord` stream.
- **Type consistency:** `MistakeCategory` and `CATEGORY_LABEL` are defined once in `explain.ts` and imported by `report.ts`; `Phase` in `phase.ts`; `Quality` in `classify.ts`. `MoveRecord` and `Report` field names match across `report.ts`, its test, and the `main.ts` render. `explain` returns `Explanation { category, text }` everywhere (M1 call site updated in Task 2 Step 4).
- **Placeholders:** none in the pure modules; all ship complete code + tests. The one integration point (the multi-game flow in Task 5) is verified by the manual run in Step 4, and every piece it calls is independently unit-tested.
- **Refactor safety:** Task 2 changes `explain`'s return type, which touches M1's `main.ts`; the plan updates that call site in the same task and gates it with `npx tsc --noEmit`, so M1 cannot silently break. `parsePgn` keeps its signature (Task 1 only extracts a shared helper), so M1's single-game viewer is untouched.
- **Deliberate limits (ponytail):** user-moves-only analysis (halves engine work), the ply+material phase heuristic, fixed depth, and inaccuracies excluded from the mistake tally. Each is marked with its upgrade path. Multi-game analysis of many games is slow (many engine calls); that is acceptable for an offline coaching pass and can drop depth if needed.

## What the next Track B sub-project will cover (preview, not part of this plan)

Step 5: drills. The `MoveRecord` stream here already knows each mistake's position (via the game and ply); a drills sub-project would keep those FENs and serve the user's own blunder positions back as "find the better move" puzzles, checking their answer against the engine's best move.
