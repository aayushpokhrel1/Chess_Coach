# Coach Milestone 3: Drills From Your Own Mistakes Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** After analyzing your games, replay your own mistake and blunder positions back to you as puzzles: the board shows the position where you went wrong, you make a move, and the coach tells you whether it holds up, checked live by Stockfish.

**Architecture:** Extends Coach M2. The existing "Analyze all games" pass already finds every mistake; it now also records a **drill** per mistake/blunder (the position, the best eval available, the engine's best move, your color, and what you played). A new pure module `drill` supplies the board's legal drag targets and grades an attempt by reusing `classify`. The board (view-only until now) becomes movable in drill mode: you drag a move, the coach analyzes the resulting position with the existing worker, and grades it leniently (any move that does not lose material counts, not just the single engine best).

**Tech Stack:** Same as M1/M2: Vite, TypeScript, chess.js, chessground (drag support), single-threaded Stockfish (Web Worker), Vitest.

**Spec:** `docs/superpowers/specs/2026-08-27-chess-coach-design.md` (§4 Track B step 5: "Drill from your own mistakes: serve the author's real blunder positions back as puzzles"). Builds on Coach M2 (`docs/superpowers/plans/2026-09-02-coach-m2-patterns.md`). This completes the planned Track B sub-projects.

## Global Constraints

- **Client-side only.** Drills come from the M2 analysis pass (no re-analysis to build the list); grading an attempt makes one live engine call.
- **Drill set:** every user move classified `mistake` or `blunder` becomes a drill.
- **Lenient grading (decided):** an attempt is judged by re-analyzing the position after the user's move and comparing to the best eval available (`classify`). `best`/`good` = solved, `inaccuracy` = close, `mistake`/`blunder` = failed. Any non-losing move solves it, not just the engine's move.
- **Move input:** drag on the board (chessground `movable`), legal targets from chess.js. Promotions auto-queen (`// ponytail: auto-queen in drills; add an under-promotion picker only if a drill ever needs it`).
- **Depth 12** for the grading analysis, as elsewhere.
- **Reuse M2:** the drill collection piggybacks on `analyzeAll`; `classify`, `scoreToCp`, `Engine`, `CATEGORY_LABEL`, `phaseOf` are unchanged. The board helper `setupBoard` stays view-only by default; drill mode reconfigures the same board element.
- **No dashes (— –)** in any comment, message, or doc.
- **Coaching mode (Mix):** the movable-board wiring and the live grading flow are the parts to understand; the pure `drill` helpers and glue are light-touch.
- **Test framework:** Vitest. The pure `drill` module is unit-tested; the movable board + grading flow is verified by a manual run.
- **Commands (from `web/`):** `npm run dev`, `npx vitest run`, `npx tsc --noEmit`.

---

## File Structure

```
web/src/
  drill.ts        # NEW (Task 1): Drill type, legalDests, gradeAttempt
  drill.test.ts   # NEW (Task 1)
  main.ts         # MODIFY (Task 2): collect drills in analyzeAll; (Task 3): drill mode
  index.html      # MODIFY (Task 3): Start-drills button + drill panel
  style.css       # MODIFY (Task 3): drill panel styling
```

---

## Task 1: The drill module (legal targets + grading)

**Files:**
- Create: `web/src/drill.ts`, `web/src/drill.test.ts`

**Interfaces:**
- Consumes: `chess.js`, `classify` (classify), `MistakeCategory` (explain).
- Produces:
  - `interface Drill { fen: string; evalBeforeCp: number; bestMove: string; color: 'w' | 'b'; playedSan: string; category: MistakeCategory; }`
  - `type DrillGrade = 'solved' | 'inaccurate' | 'failed'`
  - `function legalDests(fen: string): Map<string, string[]>`
  - `function gradeAttempt(evalBeforeCp: number, evalAfterMoverCp: number): DrillGrade`

**Theory:** Two small pure pieces. `legalDests` asks chess.js for every legal move in a position and groups the destination squares by their origin, which is exactly the shape chessground wants to light up drag targets (a piece can only be dropped on a legal square). `gradeAttempt` reuses the same eval-swing logic as move classification: it grades the user's chosen move by how much worse than best it is, so "solved" means "did not throw material away", the point of the drill. Keeping both pure means the whole judging rule is unit-tested without a board or a worker.

- [ ] **Step 1: Write the failing test**

`web/src/drill.test.ts`:
```ts
import { describe, it, expect } from 'vitest';
import { legalDests, gradeAttempt } from './drill';

const START = 'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1';

describe('legalDests', () => {
  it('lists legal targets grouped by origin', () => {
    const d = legalDests(START);
    // 20 legal first moves spread over 10 origin squares (8 pawns + 2 knights).
    const total = [...d.values()].reduce((n, arr) => n + arr.length, 0);
    expect(total).toBe(20);
    expect(d.get('e2')).toContain('e4');
    expect(d.get('g1')).toContain('f3');
  });
});

describe('gradeAttempt', () => {
  it('a non-losing move is solved', () => {
    expect(gradeAttempt(50, 45)).toBe('solved'); // cpLoss 5
  });
  it('a small slip is close', () => {
    expect(gradeAttempt(0, -80)).toBe('inaccurate'); // cpLoss 80
  });
  it('a material-losing move fails', () => {
    expect(gradeAttempt(0, -400)).toBe('failed'); // cpLoss 400
  });
});
```

- [ ] **Step 2: Run, verify it FAILS**

Run: `npx vitest run src/drill.test.ts`
Expected: FAIL, module not found.

- [ ] **Step 3: Write `drill.ts`**

`web/src/drill.ts`:
```ts
import { Chess } from 'chess.js';
import { classify } from './classify';
import type { MistakeCategory } from './explain';

export interface Drill {
  fen: string; // position to solve (user to move)
  evalBeforeCp: number; // best eval available, mover's perspective
  bestMove: string; // engine best move (uci) - the answer/hint
  color: 'w' | 'b'; // side to move (the user)
  playedSan: string; // the move the user actually played
  category: MistakeCategory;
}

export type DrillGrade = 'solved' | 'inaccurate' | 'failed';

// Legal destination squares grouped by origin, for chessground's movable.dests.
export function legalDests(fen: string): Map<string, string[]> {
  const chess = new Chess(fen);
  const dests = new Map<string, string[]>();
  for (const m of chess.moves({ verbose: true })) {
    const arr = dests.get(m.from) ?? [];
    arr.push(m.to);
    dests.set(m.from, arr);
  }
  return dests;
}

// Grade an attempt from the eval before and the eval after (mover's perspective).
export function gradeAttempt(evalBeforeCp: number, evalAfterMoverCp: number): DrillGrade {
  const { quality } = classify(evalBeforeCp, evalAfterMoverCp);
  if (quality === 'best' || quality === 'good') return 'solved';
  if (quality === 'inaccuracy') return 'inaccurate';
  return 'failed';
}
```

- [ ] **Step 4: Run, verify it PASSES**

Run: `npx vitest run src/drill.test.ts`
Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add web/src/drill.ts web/src/drill.test.ts
git commit -m "feat(coach): drill legal-targets and attempt grading"
```

---

## Task 2: Collect drills during the analysis pass

**Files:**
- Modify: `web/src/main.ts`

**Interfaces:**
- Consumes: `Drill` (drill), the existing `analyzeAll` loop, `scoreToCp`.
- Produces: a module-level `drills: Drill[]` populated when `analyzeAll` runs.

**Theory:** The M2 pass already computes, for each of the user's moves, the best eval, the engine's best move, and the quality. A drill is just those facts kept for the mistakes. Collecting them here means drilling costs no extra analysis; the report and the drill set fall out of one pass.

- [ ] **Step 1: Add the drill store and collection**

In `main.ts`, import the type and add state:
```ts
import { legalDests, gradeAttempt, type Drill } from './drill';
```
Add near the other state (`let moves`, `let analyses`):
```ts
let drills: Drill[] = [];
let drillIdx = 0;
```
In `analyzeAll`, reset `drills = [];` alongside `records`. Inside the per-move loop, after `quality`/`category` are known and before/after are available, push a drill for the bad moves:
```ts
      if (quality === 'mistake' || quality === 'blunder') {
        drills.push({
          fen: m.fenBefore,
          evalBeforeCp: scoreToCp(before.score),
          bestMove: before.bestMove,
          color,
          playedSan: m.san,
          category,
        });
      }
```
After the games loop (once, before/after `renderReport`), reveal the start-drills button when there are any:
```ts
  const startBtn = $('startDrills') as HTMLButtonElement;
  if (drills.length) {
    startBtn.textContent = `Start drills (${drills.length})`;
    startBtn.hidden = false;
  } else {
    startBtn.hidden = true;
  }
```

- [ ] **Step 2: Verify it compiles**

Run: `npx tsc --noEmit`
Expected: clean (the `startDrills` button element is added in Task 3 Step 1; if you run tsc before that, add the button first or expect only a runtime-null, not a type error, since `$` returns `HTMLElement`). Proceed to Task 3, which adds the button and the rest; commit at the end of Task 2 is optional. To keep commits clean, do Task 2 Step 1 and Task 3 together, then commit once. (This task has no standalone test; its effect is verified through the drill flow in Task 3.)

- [ ] **Step 3: Commit (folded with Task 3)**

Drill collection is committed together with the drill UI in Task 3, since it has no observable effect on its own.

---

## Task 3: Movable board and the drill flow

**Files:**
- Modify: `web/index.html`, `web/src/main.ts`, `web/src/style.css`

**Interfaces:**
- Consumes: `legalDests`, `gradeAttempt`, `Drill` (drill); `Engine`, `scoreToCp`, `DEPTH`; `CATEGORY_LABEL` (explain); `Chess` (chess.js).
- Produces: the drill UI and interaction (no new exported functions).

**Theory:** This is the interactive half. The board so far has been view-only; a drill needs the user to move, so we reconfigure the same chessground into `movable` mode with the legal targets and a drop handler, oriented to the user's side. When the user drops a piece we build the move (auto-queening a promoting pawn), play it on a scratch chess.js board to get the resulting FEN, and analyze that with the worker. `gradeAttempt` turns the eval swing into solved / close / failed, and we narrate it, revealing the engine's move only when they fail or ask. The board resets to the puzzle after each attempt so the position stays the question.

- [ ] **Step 1: Add the button and drill panel to `index.html`**

Add the start button to the second controls row (next to `analyzeAll`):
```html
        <button id="startDrills" hidden>Start drills</button>
```
Add a drill panel below the `report` div:
```html
      <div id="drill" class="drill" hidden>
        <p id="drillPrompt"></p>
        <p id="drillFeedback"></p>
        <div class="controls">
          <button id="drillAnswer">Show answer</button>
          <button id="drillNext">Next</button>
          <span id="drillProgress"></span>
        </div>
      </div>
```

- [ ] **Step 2: Add the drill flow to `main.ts`**

Add the chess.js import at the top if not present:
```ts
import { Chess } from 'chess.js';
import { CATEGORY_LABEL } from './explain';
```
Add these functions (and the listeners) near the other wiring:
```ts
function setDrillBoard(d: Drill) {
  const side = d.color === 'w' ? 'white' : 'black';
  board.set({
    fen: d.fen.split(' ')[0],
    orientation: side,
    turnColor: side,
    viewOnly: false,
    movable: {
      free: false,
      color: side,
      dests: legalDests(d.fen),
      events: { after: onUserMove },
    },
  });
}

function showDrill() {
  const d = drills[drillIdx];
  const side = d.color === 'w' ? 'white' : 'black';
  setDrillBoard(d);
  const label = CATEGORY_LABEL[d.category] || 'a mistake';
  $('drillPrompt').textContent =
    `You played ${d.playedSan} here (${label}). Find a better move for ${side}.`;
  $('drillFeedback').textContent = '';
  $('drillProgress').textContent = `${drillIdx + 1} / ${drills.length}`;
}

function bestSan(d: Drill): string {
  try {
    const c = new Chess(d.fen);
    return c.move({
      from: d.bestMove.slice(0, 2),
      to: d.bestMove.slice(2, 4),
      promotion: d.bestMove.slice(4) || undefined,
    }).san;
  } catch {
    return d.bestMove;
  }
}

async function onUserMove(orig: string, dest: string) {
  const d = drills[drillIdx];
  const chess = new Chess(d.fen);
  const piece = chess.get(orig as never);
  const promo =
    piece && piece.type === 'p' && (dest[1] === '8' || dest[1] === '1') ? 'q' : undefined;
  let fenAfter: string;
  try {
    chess.move({ from: orig, to: dest, promotion: promo });
    fenAfter = chess.fen();
  } catch {
    setDrillBoard(d); // illegal drop: reset
    return;
  }
  $('drillFeedback').textContent = 'thinking...';
  const after = await engine.analyze(fenAfter, DEPTH);
  const grade = gradeAttempt(d.evalBeforeCp, -scoreToCp(after.score));
  if (grade === 'solved') {
    $('drillFeedback').textContent = 'Solved. That move holds up.';
  } else if (grade === 'inaccurate') {
    $('drillFeedback').textContent = 'Better, but still a little loose. Try again or see the answer.';
  } else {
    $('drillFeedback').textContent = `That still loses material. The engine likes ${bestSan(d)}.`;
  }
  setDrillBoard(d); // reset so the puzzle position stays the question
}

function startDrills() {
  if (!drills.length) return;
  drillIdx = 0;
  ($('drill') as HTMLElement).hidden = false;
  showDrill();
}

$('startDrills').addEventListener('click', startDrills);
$('drillAnswer').addEventListener('click', () => {
  $('drillFeedback').textContent = `Engine's best: ${bestSan(drills[drillIdx])}.`;
});
$('drillNext').addEventListener('click', () => {
  if (drillIdx < drills.length - 1) {
    drillIdx++;
    showDrill();
  } else {
    $('drillFeedback').textContent = 'That was the last drill. Nice work.';
  }
});
```
Keep the C1 viewer view-only: in `render()` (the single-game viewer), change its `board.set` to also pass `viewOnly: true` so stepping through a game after drilling does not leave the board draggable:
```ts
  board.set({ fen: fen.split(' ')[0], viewOnly: true });
```

- [ ] **Step 3: Style the drill panel**

Add to `style.css`:
```css
.drill {
  margin-top: 1rem;
  max-width: 480px;
}
.drill #drillPrompt {
  font-weight: 600;
}
.drill #drillFeedback {
  min-height: 1.4rem;
}
```

- [ ] **Step 4: Verify end to end (manual)**

Run `npm run dev`. Paste a game or two where you (matched by username) blundered, enter your username, click "Analyze all games", then click "Start drills (N)". Expected: the board shows your first mistake position oriented to your side, with a prompt naming the move you played and the category. Drag a sensible move: a good move reports "Solved", a losing move reports "That still loses ... the engine likes ...". "Show answer" reveals the engine's move; "Next" advances; the last drill says it was the last. Confirm `npx vitest run` and `npx tsc --noEmit` are clean.

- [ ] **Step 5: Commit**

```bash
git add web/index.html web/src/main.ts web/src/style.css
git commit -m "feat(coach): drill your own mistakes on a movable board"
```

---

## Self-Review Notes

- **Spec coverage:** implements Track B step 5 (drill from your own mistakes): the M2 pass records each mistake as a `Drill` (Task 2), and the drill UI replays them on a movable board, grading attempts live (Tasks 1, 3). This completes the planned Track B steps (1 to 5).
- **Type consistency:** `Drill`, `DrillGrade`, `legalDests`, `gradeAttempt` are declared in `drill.ts` and used with those signatures in `drill.test.ts` and `main.ts`. `Drill.color` is `'w' | 'b'`, matching `GameMove.mover` and the `color` computed in `analyzeAll`. `gradeAttempt` reuses `classify` from M1, so its thresholds stay in one place.
- **Placeholders:** the pure module ships complete code + tests. The interactive flow is verified by the manual run in Task 3 Step 4; every pure piece it calls (`legalDests`, `gradeAttempt`, `classify`, `scoreToCp`) is unit-tested. Task 2 has no standalone test by design (it only records data consumed by Task 3), so it is committed together with Task 3.
- **Reuse over rebuild (ponytail):** drills fall out of the existing analysis pass (no second engine sweep); grading reuses `classify` rather than a new rule; the same board element is reconfigured rather than a second board. Lenient grading via re-analysis means the drill accepts any sound move, which is the correct coaching behavior and needs no new "is this move good" logic.
- **Board-mode hygiene:** `render()` (C1 viewer) now forces `viewOnly: true`, and drill mode sets `viewOnly: false` with `movable`, so the board cannot be left in the wrong interaction mode when switching between viewing a game and drilling.

## Track B status after this milestone

Steps 1 to 5 of the coach are done: import, per-move analysis, beginner explanations, cross-game pattern detection, and drills. Natural follow-ons (each its own future sub-project): richer explanations, Lichess/Chess.com username import instead of paste, saving progress, or swapping Stockfish for our own engine compiled to WASM (engine M7).
