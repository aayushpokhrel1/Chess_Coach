# Analysis Room Redesign Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the coach web UI's visual world with "The Analysis Room": a dark room with a lit board as its subject, the engine made visible as instrumentation, and the coaching explanation promoted from a sidebar box to an editorial annotation inside the game's notation.

**Architecture:** All work is in `web/`. A CSS custom property token layer in `style.css` carries the world; `index.html` is restructured into a hero plus two surfaces; new pure modules (`annotate.ts`, `hero.ts`, `controls.ts`) hold the logic so it is unit-testable, and `main.ts` / `play.ts` wire them to the DOM. The C++ engine and the WASM build are untouched.

**Tech Stack:** Vite 8, TypeScript 6, Vitest 4, chessground 9, chess.js 1.4, Stockfish 18 lite (WASM, Web Worker), our engine as WASM in a Web Worker. Fonts self-hosted via `@fontsource-variable/*`.

**Spec:** `docs/superpowers/specs/2026-09-23-analysis-room-redesign.md`

## Global Constraints

- **No em dashes or en dashes** in any prose, comment, commit message, or UI copy. Use a comma, a colon, parentheses, or two sentences.
- **Dark only.** No light mode, no `prefers-color-scheme` light variant.
- **The Lamp Never Judges Rule.** `--lamp` (`#f0d9a8`) marks structure and state only: active tab, focus ring, primary button, eval graph line, links, hero rim light. The verdict scale marks move quality only. Lamp never appears on a move, badge, or graph dot. Verdict never appears on a control, tab, or focus ring.
- **The Rationed Light Rule.** Lamp covers under 8% of any screen.
- **Colors are only ever referenced as CSS custom properties** defined in the `:root` block from Task 1. No literal hex values anywhere else in `style.css` except inside that block and inside the grain SVG.
- **Every interactive element** has a visible `:focus-visible` ring: `0 0 0 3px var(--lamp-ring)`.
- **Tabular figures** (`font-variant-numeric: tabular-nums`) on every clock, evaluation, node count, and counter.
- **`prefers-reduced-motion: reduce`** must be honored: no hero loop, no graph draw-on, no stagger, transitions collapse to 1ms.
- **Contrast floor:** body and placeholder text at least 4.5:1 against `--room` (`#121816`), large text at least 3:1. The token values below are already verified; do not substitute unverified colors.
- Run every command from `web/`. Tests: `npm test`. Typecheck: `npx tsc --noEmit`.
- Commit after each task. Do not push; the orchestrator pushes.

---

## File Structure

| File | Responsibility |
|---|---|
| `web/src/style.css` | Rewritten. The token layer and every component's styling. |
| `web/index.html` | Rewritten. Hero, sticky tab bar, play surface, analyze surface, score column, overlays. |
| `web/src/annotate.ts` | **New.** Pure: turn moves plus analyses into score rows with annotations. |
| `web/src/hero.ts` | **New.** Pure-ish: the landing loop's position scheduling and teardown. |
| `web/src/controls.ts` | **New.** The custom dropdown, segmented control, and level picker. |
| `web/src/board.ts` | Modified. Square color override for the recolored board. |
| `web/src/uciParse.ts` | Modified. `Info` gains `nodes` and `nps`. |
| `web/src/play.ts` | Modified. Level data restructured, readout wiring, control instantiation. |
| `web/src/main.ts` | Modified. Score column rendering, graph draw-on trigger, control instantiation. |
| `web/src/evalGraph.ts` | Modified. Graph height and the path data the draw-on needs. |
| `DESIGN.md`, `.impeccable/design.json` | Replaced in the final task from the shipped result. |

---

## Task 1: The room and the type foundation

Phase 1. After this task the existing layout survives unchanged in structure but renders in the dark room with the new type. Nothing is restructured yet.

**Files:**
- Modify: `web/package.json` (add `@fontsource-variable/fraunces`)
- Modify: `web/src/style.css` (token layer, globals, atmosphere; keep existing component rules working against the new tokens)
- Modify: `web/src/board.ts:1-16` (square colors)
- Modify: `web/index.html` (two overlay elements only)

**Interfaces:**
- Consumes: nothing.
- Produces: the CSS custom properties every later task references. Exact names are fixed here and must not be renamed.

- [ ] **Step 1: Install the display face**

```bash
npm install @fontsource-variable/fraunces
```

- [ ] **Step 2: Replace the top of `style.css` with the token layer**

Replace the existing `@import` line and `:root` block. Keep every rule below it for now; they will reference the new names.

```css
@import '@fontsource-variable/inter';
@import '@fontsource-variable/fraunces';

/* Analysis Room tokens. See DESIGN.md and the spec at
   docs/superpowers/specs/2026-09-23-analysis-room-redesign.md */
:root {
  /* Room */
  --void: #0b0f0e;
  --room: #121816;
  --raised: #1a2220;
  --edge-light: rgba(255, 255, 255, 0.06);
  --hairline: rgba(255, 255, 255, 0.09);

  /* Text */
  --text-bright: #f2ece1;
  --text: #cfc8bc;
  --text-muted: #8e877c;
  --text-faint: #6e675e;

  /* Signature: light, never decoration */
  --lamp: #f0d9a8;
  --lamp-dim: #d4bc89;
  --lamp-glow: rgba(240, 217, 168, 0.14);
  --lamp-ring: rgba(240, 217, 168, 0.35);

  /* Board. Not UI colors. */
  --board-dark: #4a5a52;
  --board-light: #b3bfb6;

  /* Verdict: the only saturated hue in the interface */
  --verdict-sound: #4ea87a;
  --verdict-inaccuracy: #d9a441;
  --verdict-mistake: #e07a3c;
  --verdict-blunder: #e0524a;

  /* Type */
  --font-display: 'Fraunces Variable', Fraunces, Georgia, serif;
  --font-ui: 'Inter Variable', Inter, system-ui, sans-serif;
  --font-mono: ui-monospace, SFMono-Regular, Menlo, monospace;

  /* Shape */
  --r-sm: 4px;
  --r-md: 8px;
  --r-lg: 14px;
  --r-pill: 999px;

  /* Elevation. Panels do not use shadows; only objects above the room do. */
  --shadow-board: 0 24px 60px rgba(0, 0, 0, 0.55);
  --shadow-popover: 0 12px 32px rgba(0, 0, 0, 0.5);
  --focus-ring: 0 0 0 3px var(--lamp-ring);

  /* Motion */
  --ease: cubic-bezier(0.16, 1, 0.3, 1);
  --t-state: 150ms;
  --t-piece: 240ms;
  --t-draw: 700ms;
}
```

- [ ] **Step 3: Replace the global and atmosphere rules**

Immediately after `:root`, replace the existing `body`, `#app`, `h1`, `::selection`, `:focus-visible`, and `a` rules with:

```css
html {
  background: var(--void);
}

body {
  font-family: var(--font-ui);
  font-size: 0.9375rem;
  line-height: 1.5;
  margin: 0;
  color: var(--text);
  background: var(--room);
  -webkit-text-size-adjust: 100%;
  position: relative;
}

/* The lamp above the board, and the vignette at the edges. */
body::before {
  content: '';
  position: fixed;
  inset: 0;
  pointer-events: none;
  z-index: 0;
  background:
    radial-gradient(60vw 45vh at 50% 8%, var(--lamp-glow), transparent 70%),
    radial-gradient(120vw 120vh at 50% 45%, transparent 35%, rgba(0, 0, 0, 0.55) 100%);
}

/* Film grain: removes banding from the gradients and gives the room texture. */
body::after {
  content: '';
  position: fixed;
  inset: 0;
  pointer-events: none;
  z-index: 1;
  opacity: 0.025;
  background-image: url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' width='160' height='160'%3E%3Cfilter id='n'%3E%3CfeTurbulence type='fractalNoise' baseFrequency='0.8' numOctaves='3'/%3E%3C/filter%3E%3Crect width='160' height='160' filter='url(%23n)'/%3E%3C/svg%3E");
}

#app {
  position: relative;
  z-index: 2;
  max-width: 1280px;
  margin: 0 auto;
  padding: 2rem 1.5rem 4.5rem;
}

h1 {
  font-family: var(--font-display);
  font-size: clamp(3rem, 7vw, 5rem);
  font-weight: 600;
  line-height: 0.98;
  letter-spacing: -0.03em;
  color: var(--text-bright);
  margin: 0 0 1.5rem;
  text-wrap: balance;
}

::selection {
  background: var(--lamp);
  color: var(--void);
}

:focus-visible {
  outline: none;
  box-shadow: var(--focus-ring);
}

a {
  color: var(--lamp);
  text-underline-offset: 0.2em;
  text-decoration-thickness: 1px;
}
a:hover {
  color: var(--lamp-dim);
}

@media (prefers-reduced-motion: reduce) {
  *,
  *::before,
  *::after {
    transition-duration: 1ms !important;
    animation-duration: 1ms !important;
    animation-iteration-count: 1 !important;
  }
}
```

- [ ] **Step 4: Retarget every remaining rule in `style.css` to the new tokens**

Work down the rest of the file and replace each old token reference:

| Old | New |
|---|---|
| `var(--page)` as a surface | `var(--room)` |
| `var(--surface)` (white cards) | `var(--raised)` |
| `var(--surface-sunken)` | `var(--raised)` |
| `var(--ink)` | `var(--text)` |
| `var(--ink-muted)` | `var(--text-muted)` |
| `var(--ink-faint)` | `var(--text-faint)` |
| `var(--felt)` | `var(--lamp)` |
| `var(--felt-deep)` | `var(--lamp-dim)` |
| `var(--felt-wash)` | `var(--raised)` |
| `var(--verdict-best)` | `var(--verdict-sound)` |
| `var(--shadow-rest)` on a panel | delete; add `border-top: 1px solid var(--edge-light)` |
| `--r-lg: 12px` usages | `var(--r-lg)` (now 14px) |

Two rules need more than a swap:

- `button.primary`: `background: var(--lamp); color: var(--void); border-color: var(--lamp);` and on hover `background: var(--lamp-dim); border-color: var(--lamp-dim); color: var(--void);`
- Plain `button, select`: `background: var(--raised); color: var(--text); border: 1px solid var(--hairline);` and on hover `background: var(--raised); color: var(--text-bright); border-color: var(--edge-light);`

Delete the `.evalgraph { background: #b9ae9e }` literal and set `background: var(--raised)`; delete `.ga-white { fill: var(--page) }` and set `fill: var(--board-light)`. Delete the `.moves::-webkit-scrollbar-thumb { border: 3px solid var(--surface) }` white ring and use `var(--room)`.

- [ ] **Step 5: Recolor the board**

Replace `web/src/board.ts` lines 1-5. Drop the brown theme import and set the square colors from the tokens.

```ts
import { Chessground } from 'chessground';
import type { Api } from 'chessground/api';
import 'chessground/assets/chessground.base.css';
import 'chessground/assets/chessground.cburnett.css';
```

Then add to `style.css`, after the token layer:

```css
/* Board squares belong to the room. cburnett pieces are unchanged. */
cg-board {
  background-color: var(--board-light);
}
cg-board square.light {
  background-color: var(--board-light);
}
cg-board square.dark {
  background-color: var(--board-dark);
}
.cg-wrap {
  box-shadow: var(--shadow-board);
}
```

- [ ] **Step 6: Typecheck and test**

Run: `npx tsc --noEmit && npm test`
Expected: tsc silent, 38/38 Vitest passing. No test covers CSS; the gate here is that nothing regressed.

- [ ] **Step 7: Visual check**

Start the dev server and load the page. Confirm: the page is dark, the grain is visible but subtle, the lamp gradient sits above the fold, the board squares are slate-green, the board has a cast shadow, and no element still renders on a white or cream background. Fix anything that does.

- [ ] **Step 8: Commit**

```bash
git add -A
git commit -m "feat(web): the Analysis Room token layer, dark surfaces, and recolored board"
```

---

## Task 2: Score rows (pure module, TDD)

Phase 2. A pure function so the score column's logic is testable without a DOM.

**Files:**
- Create: `web/src/annotate.ts`
- Test: `web/src/annotate.test.ts`

**Interfaces:**
- Consumes: `Quality` from `./classify`.
- Produces:

```ts
export interface ScoreRow {
  index: number;            // ply index, matching moves[] and analyses[]
  san: string;
  quality?: Quality;        // undefined when the move was not analyzed
  annotation?: string;      // present only for inaccuracy | mistake | blunder
  cpLoss?: number;          // present whenever annotation is present
  bestLine?: string;        // SAN best line, present whenever annotation is present
}
export interface AnnotatableMove { san: string }
export interface AnnotatableAnalysis {
  quality: Quality;
  cpLoss: number;
  explanation: string;
  bestLine?: string;
}
export function buildScore(
  moves: AnnotatableMove[],
  analyses: (AnnotatableAnalysis | undefined)[],
): ScoreRow[];
```

- [ ] **Step 1: Write the failing test**

Create `web/src/annotate.test.ts`:

```ts
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
```

- [ ] **Step 2: Run the test to verify it fails**

Run: `npm test -- annotate`
Expected: FAIL, cannot resolve `./annotate`.

- [ ] **Step 3: Write the minimal implementation**

Create `web/src/annotate.ts`:

```ts
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
```

- [ ] **Step 4: Run the test to verify it passes**

Run: `npm test -- annotate`
Expected: PASS, 4 tests.

- [ ] **Step 5: Commit**

```bash
git add web/src/annotate.ts web/src/annotate.test.ts
git commit -m "feat(web): score rows, annotating only the moves that cost something"
```

---

## Task 3: The score column

Phase 2. Replaces the 320px sidebar (explanation well plus move list plus best line) with a single typographic column where each mistake's explanation sits beneath the move it describes.

**Files:**
- Modify: `web/index.html` (the `.layout` block inside `#tab-coach`)
- Modify: `web/src/main.ts` (`renderMoves` and `render`)
- Modify: `web/src/style.css` (add the score column rules, delete `.side`, `.explain`, `.moves`, `.bestline`)

**Interfaces:**
- Consumes: `buildScore`, `ScoreRow` from `./annotate`.
- Produces: the `#score` element and its row markup, which Task 9's cursor sync reads.

- [ ] **Step 1: Restructure the markup**

In `web/index.html`, replace the `<div class="side">...</div>` block with:

```html
<div class="score-col">
  <ol id="score" class="score"></ol>
</div>
```

Delete the `#explain`, `#bestline`, and `#moves` elements. Keep `#nav`, `#ply`, `#prev`, `#next` where they are.

- [ ] **Step 2: Render score rows in `main.ts`**

Replace the body of `renderMoves` so it builds from `buildScore`. Each row is an `<li>` carrying `data-i`; annotated rows also render an annotation block.

```ts
import { buildScore } from './annotate';

function renderMoves() {
  const ol = $('score');
  ol.innerHTML = '';
  const rows = buildScore(moves, analyses);
  rows.forEach((r) => {
    const li = document.createElement('li');
    li.dataset.i = String(r.index);
    li.className = r.annotation ? `score-row is-${r.quality}` : 'score-row';
    const loss = r.cpLoss === undefined ? '' : r.cpLoss > 9999 ? 'mate' : `-${r.cpLoss}`;
    li.innerHTML =
      `<span class="san">${r.san}</span>` +
      (r.annotation
        ? `<div class="annotation">` +
          `<span class="verdict">${r.quality} ${loss}</span>` +
          `<p class="annotation-text">${r.annotation}</p>` +
          (r.bestLine ? `<p class="best-line">Better: ${r.bestLine}</p>` : '') +
          `</div>`
        : '');
    li.addEventListener('click', () => {
      idx = r.index;
      render();
    });
    ol.appendChild(li);
  });
}
```

In `render()`, replace the old `$('moves').children` loop and the `$('explain')` / `$('bestline')` assignments with:

```ts
Array.from($('score').children).forEach((li, i) =>
  li.classList.toggle('active', i === idx),
);
```

Delete every remaining reference to `#explain`, `#bestline`, and `#moves` in `main.ts`.

- [ ] **Step 3: Style the score column**

Delete the `.side`, `.explain`, `.moves`, `.moves li`, `.moves .san`, `.badge`, and `.bestline` rules. Add:

```css
.layout {
  display: flex;
  gap: 3rem;
  align-items: flex-start;
}

.score-col {
  flex: 1 1 420px;
  min-width: 320px;
}

.score {
  list-style: decimal;
  padding-left: 2.5rem;
  margin: 0;
  color: var(--text-muted);
  max-height: 640px;
  overflow-y: auto;
  scrollbar-width: thin;
  scrollbar-color: var(--hairline) transparent;
}

.score-row {
  padding: 0.25rem 0 0.25rem 0.5rem;
  border-left: 1px solid transparent;
  border-radius: var(--r-sm);
  cursor: pointer;
  transition: border-color var(--t-state) var(--ease);
}
.score-row:hover .san {
  color: var(--text-bright);
}
.score-row.active {
  border-left-color: var(--lamp);
}
.score-row.active .san {
  color: var(--text-bright);
}

.san {
  font-family: var(--font-mono);
  font-weight: 500;
  font-variant-numeric: tabular-nums;
  color: var(--text);
}

/* The annotation is the coach's markup inside the score, not a panel beside it. */
.annotation {
  margin: 0.5rem 0 1rem 0;
  padding-left: 1rem;
  border-left: 2px solid var(--hairline);
}
.is-inaccuracy .annotation {
  border-left-color: var(--verdict-inaccuracy);
}
.is-mistake .annotation {
  border-left-color: var(--verdict-mistake);
}
.is-blunder .annotation {
  border-left-color: var(--verdict-blunder);
}

.verdict {
  display: block;
  font-family: var(--font-ui);
  font-size: 0.75rem;
  font-weight: 600;
  letter-spacing: 0.06em;
  text-transform: uppercase;
  font-variant-numeric: tabular-nums;
  margin-bottom: 0.25rem;
}
.is-inaccuracy .verdict {
  color: var(--verdict-inaccuracy);
}
.is-mistake .verdict {
  color: var(--verdict-mistake);
}
.is-blunder .verdict {
  color: var(--verdict-blunder);
}

.annotation-text {
  font-family: var(--font-display);
  font-size: 1.15rem;
  font-weight: 400;
  line-height: 1.6;
  color: var(--text);
  margin: 0;
  max-width: 64ch;
  /* Collapsed until its move is selected. */
  display: -webkit-box;
  -webkit-line-clamp: 2;
  -webkit-box-orient: vertical;
  overflow: hidden;
}
.score-row.active .annotation-text {
  -webkit-line-clamp: unset;
  overflow: visible;
}

.best-line {
  font-family: var(--font-mono);
  font-size: 0.9rem;
  font-variant-numeric: tabular-nums;
  color: var(--text-muted);
  margin: 0.5rem 0 0;
}
.score-row:not(.active) .best-line {
  display: none;
}

@media (max-width: 1024px) {
  .layout {
    flex-wrap: wrap;
    gap: 2rem;
  }
  .score-col {
    flex-basis: 100%;
  }
}
```

- [ ] **Step 4: Typecheck and test**

Run: `npx tsc --noEmit && npm test`
Expected: tsc silent (it will fail first if any `#explain` reference remains; fix those), 42/42 passing.

- [ ] **Step 5: Visual check**

Load a PGN with at least one blunder, analyze it, and confirm: good moves are silent, mistakes carry a colored rule and a serif annotation, the selected move expands its annotation and shows its best line, and clicking a move still moves the board.

- [ ] **Step 6: Commit**

```bash
git add -A
git commit -m "feat(web): the score column, explanations annotated inside the notation"
```

---

## Task 4: The hero loop (pure module, TDD)

Phase 3. Scheduling logic only; no DOM, no chessground.

**Files:**
- Create: `web/src/hero.ts`
- Test: `web/src/hero.test.ts`

**Interfaces:**
- Produces:

```ts
export interface HeroLoopOptions {
  fens: string[];                       // one per position, index 0 is the start
  onPosition: (fen: string, index: number) => void;
  intervalMs?: number;                  // default 1100
  reducedMotion?: boolean;              // default false
}
export interface HeroLoop {
  start(): void;
  stop(): void;
}
export function createHeroLoop(options: HeroLoopOptions): HeroLoop;
```

Behavior contract: `start()` emits index 0 immediately, then advances one position every `intervalMs`, wrapping to 0 after the last. `stop()` clears the timer and makes further ticks impossible. When `reducedMotion` is true, `start()` emits the LAST position once and never schedules a timer.

- [ ] **Step 1: Write the failing test**

Create `web/src/hero.test.ts`:

```ts
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
```

- [ ] **Step 2: Run the test to verify it fails**

Run: `npm test -- hero`
Expected: FAIL, cannot resolve `./hero`.

- [ ] **Step 3: Write the minimal implementation**

Create `web/src/hero.ts`:

```ts
export interface HeroLoopOptions {
  fens: string[];
  onPosition: (fen: string, index: number) => void;
  intervalMs?: number;
  reducedMotion?: boolean;
}
export interface HeroLoop {
  start(): void;
  stop(): void;
}

// The landing board replays a miniature so the first viewport is never empty.
// Under reduced motion it shows the finished position instead of looping.
export function createHeroLoop(options: HeroLoopOptions): HeroLoop {
  const { fens, onPosition, intervalMs = 1100, reducedMotion = false } = options;
  let timer: ReturnType<typeof setInterval> | undefined;
  let i = 0;

  return {
    start() {
      if (reducedMotion) {
        onPosition(fens[fens.length - 1], fens.length - 1);
        return;
      }
      i = 0;
      onPosition(fens[0], 0);
      timer = setInterval(() => {
        i = (i + 1) % fens.length;
        onPosition(fens[i], i);
      }, intervalMs);
    },
    stop() {
      if (timer !== undefined) clearInterval(timer);
      timer = undefined;
    },
  };
}
```

- [ ] **Step 4: Run the test to verify it passes**

Run: `npm test -- hero`
Expected: PASS, 5 tests.

- [ ] **Step 5: Commit**

```bash
git add web/src/hero.ts web/src/hero.test.ts
git commit -m "feat(web): hero loop scheduling with a reduced-motion path"
```

---

## Task 5: The hero section

Phase 3. Wires the loop to a real board and makes the landing viewport a composition instead of a form.

**Files:**
- Modify: `web/index.html` (hero section above the tabs; tab bar becomes sticky)
- Modify: `web/src/main.ts` (build the FEN list, instantiate the loop, wire the entry actions, tear down on interaction)
- Modify: `web/src/style.css` (hero rules, sticky tab bar)

**Interfaces:**
- Consumes: `createHeroLoop` from `./hero`, `setupBoard` from `./board`.

- [ ] **Step 1: Add the hero markup**

In `web/index.html`, directly inside `<main id="app">` and before the existing `<h1>`, insert:

```html
<section class="hero">
  <div class="hero-copy">
    <h1>Find out what you actually did wrong.</h1>
    <p class="hero-sub">
      Paste a game you lost. Every move gets checked by an engine, the mistakes get
      explained in plain language, and then you drill the positions you got wrong.
    </p>
    <div class="hero-actions">
      <button id="goPlay" class="btn-lamp">Play the engine</button>
      <button id="goCoach" class="btn-ghost">Analyze your game</button>
    </div>
  </div>
  <div class="hero-stage">
    <div id="heroBoard" class="board hero-board"></div>
  </div>
</section>
```

Delete the old standalone `<h1>Chess Coach</h1>`.

- [ ] **Step 2: Drive the hero board**

In `web/src/main.ts`, add near the other setup code. The miniature is Morphy's Opera Game, truncated to its first 16 plies; generate the FEN list with chess.js so no FEN is hand-typed.

```ts
import { Chess } from 'chess.js';
import { createHeroLoop } from './hero';

const HERO_PGN = '1. e4 e5 2. Nf3 d6 3. d4 Bg4 4. dxe5 Bxf3 5. Qxf3 dxe5 6. Bc4 Nf6 7. Qb3 Qe7 8. Nc3 c6';

function heroFens(): string[] {
  const g = new Chess();
  const fens = [g.fen()];
  for (const san of new Chess().loadPgn(HERO_PGN) ? [] : []) void san; // placeholder removed below
  return fens;
}
```

Replace that stub with the real implementation:

```ts
function heroFens(): string[] {
  const source = new Chess();
  source.loadPgn(HERO_PGN);
  const history = source.history();
  const g = new Chess();
  const fens = [g.fen()];
  for (const san of history) {
    g.move(san);
    fens.push(g.fen());
  }
  return fens;
}

function startHero() {
  const el = document.getElementById('heroBoard');
  if (!el) return;
  const api = setupBoard(el);
  const reduced = window.matchMedia('(prefers-reduced-motion: reduce)').matches;
  const loop = createHeroLoop({
    fens: heroFens(),
    reducedMotion: reduced,
    onPosition: (fen) => api.set({ fen: fen.split(' ')[0] }),
  });
  loop.start();
  return loop;
}

const hero = startHero();
```

Wire the entry actions, stopping the loop so it does not run behind the app:

```ts
document.getElementById('goPlay')?.addEventListener('click', () => {
  hero?.stop();
  showTab('play');
  document.getElementById('tab-play')?.scrollIntoView({ behavior: 'smooth' });
});
document.getElementById('goCoach')?.addEventListener('click', () => {
  hero?.stop();
  showTab('coach');
  document.getElementById('tab-coach')?.scrollIntoView({ behavior: 'smooth' });
});
```

If `showTab` is named differently in `main.ts`, use the existing function that toggles the `active` class on `#tabPlay` / `#tabCoach`.

- [ ] **Step 3: Style the hero and the sticky tab bar**

```css
.hero {
  display: grid;
  grid-template-columns: 1fr auto;
  gap: 4.5rem;
  align-items: center;
  min-height: min(72vh, 720px);
  padding: 2rem 0 4rem;
}

.hero-copy {
  max-width: 34ch;
}

.hero-sub {
  font-size: 1.05rem;
  line-height: 1.6;
  color: var(--text-muted);
  max-width: 48ch;
  margin: 0 0 2rem;
}

.hero-actions {
  display: flex;
  gap: 0.75rem;
  flex-wrap: wrap;
}

.btn-lamp,
.btn-ghost {
  font-family: var(--font-ui);
  font-size: 1rem;
  font-weight: 600;
  padding: 0.75rem 1.5rem;
  border-radius: var(--r-md);
  cursor: pointer;
  transition:
    background var(--t-state) var(--ease),
    color var(--t-state) var(--ease),
    border-color var(--t-state) var(--ease);
}
.btn-lamp {
  background: var(--lamp);
  color: var(--void);
  border: 1px solid var(--lamp);
}
.btn-lamp:hover {
  background: var(--lamp-dim);
  border-color: var(--lamp-dim);
}
.btn-ghost {
  background: transparent;
  color: var(--text-bright);
  border: 1px solid var(--hairline);
}
.btn-ghost:hover {
  border-color: var(--edge-light);
  background: var(--raised);
}

.hero-stage {
  position: relative;
}
.hero-board {
  width: 560px;
  height: 560px;
}
/* The rim light: the lamp catching the board's top edge. */
.hero-stage::before {
  content: '';
  position: absolute;
  inset: -1px;
  border-radius: 2px;
  background: linear-gradient(to bottom, var(--lamp-glow), transparent 40%);
  pointer-events: none;
  z-index: 1;
}

.tabs {
  position: sticky;
  top: 0;
  z-index: 3;
  background: var(--room);
  display: flex;
  gap: 0.25rem;
  border-bottom: 1px solid var(--hairline);
  margin-bottom: 2rem;
}

@media (max-width: 1024px) {
  .hero {
    grid-template-columns: 1fr;
    gap: 2.5rem;
    padding-bottom: 3rem;
  }
  .hero-board {
    width: min(560px, 100vw - 3rem);
    height: auto;
    aspect-ratio: 1;
  }
}
```

- [ ] **Step 4: Typecheck and test**

Run: `npx tsc --noEmit && npm test`
Expected: tsc silent, 43/43 passing.

- [ ] **Step 5: Visual check**

Load the page. Confirm the board is replaying the miniature, the headline is at display size, both entry actions work and stop the loop, the tab bar sticks on scroll, and the hero stacks at 1024px and below. Toggle reduced motion in devtools and confirm the loop does not run.

- [ ] **Step 6: Commit**

```bash
git add -A
git commit -m "feat(web): a hero that plays chess instead of showing three dropdowns"
```

---

## Task 6: The custom dropdown (TDD)

Phase 4.

**Files:**
- Create: `web/src/controls.ts`
- Test: `web/src/controls.test.ts`

**Interfaces:**
- Produces:

```ts
export interface DropdownOption { value: string; label: string }
export interface Dropdown {
  el: HTMLElement;               // append this where the old <select> was
  get value(): string;
  set value(v: string);
  destroy(): void;
}
export function createDropdown(options: DropdownOption[], initial?: string): Dropdown;
```

Behavior contract: `el` contains a `button[role=combobox]` with `aria-expanded` and a `ul[role=listbox]` with `li[role=option][aria-selected]`. Enter, Space, ArrowDown and ArrowUp on the button open the listbox. While open, ArrowDown and ArrowUp move the active option, Home and End jump to first and last, Enter selects and closes, Escape closes without selecting and returns focus to the button. Selecting dispatches a bubbling `change` event on `el`.

- [ ] **Step 1: Write the failing test**

Create `web/src/controls.test.ts`. Vitest needs a DOM here, so add `// @vitest-environment jsdom` as the first line.

```ts
// @vitest-environment jsdom
import { describe, it, expect, vi } from 'vitest';
import { createDropdown } from './controls';

const OPTS = [
  { value: '0', label: 'Novice' },
  { value: '1', label: 'Beginner' },
  { value: '2', label: 'Advanced' },
];

const key = (el: Element, k: string) =>
  el.dispatchEvent(new KeyboardEvent('keydown', { key: k, bubbles: true }));

describe('createDropdown', () => {
  it('starts closed, on the initial value', () => {
    const d = createDropdown(OPTS, '1');
    const btn = d.el.querySelector('[role=combobox]')!;
    expect(btn.getAttribute('aria-expanded')).toBe('false');
    expect(d.value).toBe('1');
    expect(btn.textContent).toContain('Beginner');
  });

  it('opens on Enter and closes on Escape, returning focus', () => {
    const d = createDropdown(OPTS);
    document.body.appendChild(d.el);
    const btn = d.el.querySelector('[role=combobox]') as HTMLElement;
    btn.focus();
    key(btn, 'Enter');
    expect(btn.getAttribute('aria-expanded')).toBe('true');
    key(d.el.querySelector('[role=listbox]')!, 'Escape');
    expect(btn.getAttribute('aria-expanded')).toBe('false');
    expect(document.activeElement).toBe(btn);
  });

  it('ArrowDown then Enter selects the next option and emits change', () => {
    const d = createDropdown(OPTS, '0');
    document.body.appendChild(d.el);
    const onChange = vi.fn();
    d.el.addEventListener('change', onChange);
    const btn = d.el.querySelector('[role=combobox]') as HTMLElement;
    key(btn, 'Enter');
    const list = d.el.querySelector('[role=listbox]')!;
    key(list, 'ArrowDown');
    key(list, 'Enter');
    expect(d.value).toBe('1');
    expect(onChange).toHaveBeenCalledTimes(1);
  });

  it('End jumps to the last option', () => {
    const d = createDropdown(OPTS, '0');
    document.body.appendChild(d.el);
    const btn = d.el.querySelector('[role=combobox]') as HTMLElement;
    key(btn, 'Enter');
    const list = d.el.querySelector('[role=listbox]')!;
    key(list, 'End');
    key(list, 'Enter');
    expect(d.value).toBe('2');
  });

  it('marks the selected option with aria-selected', () => {
    const d = createDropdown(OPTS, '2');
    const selected = d.el.querySelectorAll('[role=option][aria-selected=true]');
    expect(selected).toHaveLength(1);
    expect(selected[0].textContent).toContain('Advanced');
  });
});
```

- [ ] **Step 2: Run the test to verify it fails**

Run: `npm test -- controls`
Expected: FAIL, cannot resolve `./controls`. If jsdom is missing, install it: `npm install -D jsdom`.

- [ ] **Step 3: Implement `createDropdown`**

Write `web/src/controls.ts` satisfying the contract above. Requirements the tests do not cover but the spec does: close on outside click and on scroll; the listbox uses `--shadow-popover`, `--raised`, and `--r-md`; the active option is marked with a class the CSS styles, not with a color set in JS.

- [ ] **Step 4: Run the test to verify it passes**

Run: `npm test -- controls`
Expected: PASS, 5 tests.

- [ ] **Step 5: Commit**

```bash
git add web/src/controls.ts web/src/controls.test.ts web/package.json
git commit -m "feat(web): keyboard-operable custom dropdown"
```

---

## Task 7: Level picker, segmented control, and empty states

Phase 4.

**Files:**
- Modify: `web/src/play.ts:17-31` (LEVELS gains `name`, `elo`, `blurb`) and its level select wiring
- Modify: `web/src/controls.ts` (add `createSegmented`, `createLevelPicker`)
- Modify: `web/index.html` (play toolbar, analyze import panel, empty states)
- Modify: `web/src/style.css`

**Interfaces:**
- Consumes: `createDropdown` from Task 6.
- Produces:

```ts
export interface Segment { value: string; label: string }
export function createSegmented(segments: Segment[], initial?: string): Dropdown;
export interface Level { name: string; elo: number | null; blurb: string; depth: number; random?: number }
export function createLevelPicker(levels: Level[], initial?: number): Dropdown;
```

`createSegmented` and `createLevelPicker` return the same `Dropdown` shape so call sites are uniform. `elo` is `null` for Novice, which is a random-move level below the engine's floor and must render as "no rating" rather than a fabricated number.

- [ ] **Step 1: Restructure the level data**

In `web/src/play.ts`, replace the `LEVELS` array. Keep the existing depths and the random probability exactly; these are measured values and must not be changed.

```ts
// Strength levels: search depth -> MEASURED Elo, from web/scripts/gauntlet.mjs.
// Do not invent ratings here. Novice is below the engine's floor (depth-1 is
// already ~1190), so it plays random moves 60% of the time and has no rating.
const LEVELS: Level[] = [
  { name: 'Novice', elo: null, blurb: 'Plays half at random. For your first games.', depth: 1, random: 0.6 },
  { name: 'Beginner', elo: 1190, blurb: 'Sees one move ahead. Punishes hanging pieces.', depth: 1 },
  { name: 'Intermediate', elo: 1520, blurb: 'Sees two moves ahead. Will trade you down.', depth: 2 },
  { name: 'Advanced', elo: 1660, blurb: 'Four moves ahead with quiescence. Plays real chess.', depth: 4 },
];
```

Replace the `levelSel.add(new Option(...))` loop with `createLevelPicker(LEVELS, 0)`, and read the chosen index from the picker's `value` where `levelSel.value` was read.

- [ ] **Step 2: Implement the two controls**

Add `createSegmented` and `createLevelPicker` to `controls.ts`. The level picker renders each level's `elo` as a large tabular numeral (or "unrated" when `null`), the `name` as its title, and the `blurb` beneath it. The segmented control renders a pill row with the selected segment filled `--raised` in `--text-bright`, and an indicator that slides using `transform` rather than animating `left` or `width`.

- [ ] **Step 3: Rebuild the toolbars and empty states in `index.html`**

Play surface: replace the `.controls` block's native `<select>` elements with mount points (`<div id="playColorMount">`, `<div id="playLevelMount">`, `<div id="playTimeMount">`). The board shows the starting position on load rather than nothing.

Analyze surface: replace the source `<select>` plus username `<input>` plus Fetch button with an import panel offering Lichess and Chess.com as two first-class buttons beside one username field, and give the empty analyze state a designed invitation rather than a bare textarea. Use the service names as text only; do not add logos.

- [ ] **Step 4: Typecheck and test**

Run: `npx tsc --noEmit && npm test`
Expected: tsc silent, all tests passing.

- [ ] **Step 5: Visual and keyboard check**

Tab through the play toolbar with the keyboard only: every control must be reachable, operable, and show the lamp focus ring. Confirm the level picker shows measured Elo and that Novice reads as unrated.

- [ ] **Step 6: Commit**

```bash
git add -A
git commit -m "feat(web): level picker with measured Elo, segmented time control, designed empty states"
```

---

## Task 8: The engine readout (TDD on the parser)

Phase 5. Makes the engine's work visible. `parseInfo` currently drops `nodes` and `nps`.

**Files:**
- Modify: `web/src/uciParse.ts:5-31`
- Modify: `web/src/uciParse.test.ts`
- Modify: `web/src/play.ts` (listen for info lines, update the readout)
- Modify: `web/index.html`, `web/src/style.css` (the readout strip)

**Interfaces:**
- Produces: `Info` gains `nodes?: number` and `nps?: number`.

- [ ] **Step 1: Write the failing test**

Add to `web/src/uciParse.test.ts`:

```ts
it('carries nodes and nps when the engine reports them', () => {
  const info = parseInfo('info depth 7 score cp 34 nodes 120450 nps 890000 pv e2e4 e7e5');
  expect(info).not.toBeNull();
  expect(info!.nodes).toBe(120450);
  expect(info!.nps).toBe(890000);
});

it('leaves nodes and nps undefined when absent', () => {
  const info = parseInfo('info depth 3 score cp 12 pv e2e4');
  expect(info!.nodes).toBeUndefined();
  expect(info!.nps).toBeUndefined();
});
```

- [ ] **Step 2: Run the test to verify it fails**

Run: `npm test -- uciParse`
Expected: FAIL, `nodes` is undefined on the first test.

- [ ] **Step 3: Extend `parseInfo`**

Add `nodes?: number` and `nps?: number` to the `Info` interface, and in the return statement add `nodes: num('nodes'), nps: num('nps')`. The existing `num` helper already returns `undefined` when the key is absent.

- [ ] **Step 4: Run the test to verify it passes**

Run: `npm test -- uciParse`
Expected: PASS.

- [ ] **Step 5: Wire the readout**

In `play.ts`'s `askEngine`, the worker message handler already receives every line. Call `parseInfo` on each and, when it returns a value, update the readout strip with depth, nodes, and nps. Hide the strip when `bestmove` arrives. Add to `index.html`, under the play board:

```html
<div id="engineReadout" class="readout" hidden></div>
```

Style it:

```css
.readout {
  font-family: var(--font-mono);
  font-size: 0.8rem;
  font-weight: 500;
  letter-spacing: 0.02em;
  font-variant-numeric: tabular-nums;
  color: var(--lamp-dim);
  text-align: center;
  min-height: 1.2rem;
  margin-top: 0.75rem;
}
```

Format it as `depth 7 · 120,450 nodes · 890k nps`, using `Intl.NumberFormat` for the grouping.

- [ ] **Step 6: Typecheck, test, and play a game**

Run: `npx tsc --noEmit && npm test`
Then start a game against Advanced and confirm the readout counts up while the engine thinks and hides when it moves.

- [ ] **Step 7: Commit**

```bash
git add -A
git commit -m "feat(web): live engine readout, depth and nodes and nps while it searches"
```

---

## Task 9: Instruments and the authored motion moment

Phase 5. The eval graph rebuild, its draw-on, and the eval bar overshoot.

**Files:**
- Modify: `web/src/evalGraph.ts` (default height 120 becomes 160; expose the line path length)
- Modify: `web/src/evalGraph.test.ts`
- Modify: `web/src/main.ts` (trigger the draw-on once per analysis)
- Modify: `web/src/style.css`

- [ ] **Step 1: Write the failing test**

Add to `web/src/evalGraph.test.ts`:

```ts
it('defaults to the Analysis Room graph height', () => {
  const g = buildGraph([{ cp: 0 }, { cp: 120 }] as never);
  expect(g.height).toBe(160);
});
```

- [ ] **Step 2: Run it, confirm it fails, then change the default**

Run: `npm test -- evalGraph`
Expected: FAIL, received 120. Change `buildGraph`'s `height` default from `120` to `160`, then rerun and confirm PASS. Fix any existing test that hard-codes 120.

- [ ] **Step 3: Restyle the graph**

Replace the `.evalgraph` and `.ga-*` rules:

```css
.evalgraph {
  width: 100%;
  height: 160px;
  margin: 1rem 0 2rem;
  background: var(--raised);
  border-radius: var(--r-lg);
  border-top: 1px solid var(--edge-light);
  overflow: hidden;
}
.ga-white {
  fill: var(--board-light);
  opacity: 0.16;
}
.ga-mid {
  stroke: var(--hairline);
  stroke-width: 1;
  stroke-dasharray: 3 3;
}
.ga-line {
  fill: none;
  stroke: var(--lamp);
  stroke-width: 1.5;
  vector-effect: non-scaling-stroke;
  filter: drop-shadow(0 0 4px var(--lamp-glow));
}
.ga-dot {
  stroke: var(--room);
  stroke-width: 1.5;
}
.ga-dot.q-inaccuracy { fill: var(--verdict-inaccuracy); }
.ga-dot.q-mistake { fill: var(--verdict-mistake); }
.ga-dot.q-blunder { fill: var(--verdict-blunder); }
.ga-cursor {
  stroke: var(--text-bright);
  stroke-width: 1.5;
  vector-effect: non-scaling-stroke;
}

/* The authored moment: the game's shape revealing itself, once per analysis. */
.evalgraph.is-revealing .ga-line {
  stroke-dasharray: var(--line-len);
  stroke-dashoffset: var(--line-len);
  animation: ga-draw var(--t-draw) var(--ease) forwards;
}
.evalgraph.is-revealing .ga-dot {
  opacity: 0;
  animation: ga-flare 320ms var(--ease) forwards;
  animation-delay: calc(var(--t-draw) + var(--flare-i, 0) * 40ms);
}
@keyframes ga-draw {
  to { stroke-dashoffset: 0; }
}
@keyframes ga-flare {
  from { opacity: 0; transform: scale(0.4); }
  to { opacity: 1; transform: scale(1); }
}
@media (prefers-reduced-motion: reduce) {
  .evalgraph.is-revealing .ga-line,
  .evalgraph.is-revealing .ga-dot {
    animation: none;
    stroke-dashoffset: 0;
    opacity: 1;
  }
}
```

- [ ] **Step 4: Trigger the reveal**

In `main.ts`, after the graph SVG is inserted and analysis has completed, measure the line and start the animation once:

```ts
const line = svg.querySelector('.ga-line') as SVGPolylineElement | null;
if (line) {
  const len = line.getTotalLength();
  host.style.setProperty('--line-len', String(len));
  svg.querySelectorAll('.ga-dot').forEach((dot, i) =>
    (dot as SVGElement).style.setProperty('--flare-i', String(i)),
  );
  host.classList.add('is-revealing');
}
```

Remove `is-revealing` when a new analysis starts so the reveal plays once per analysis, not on every re-render.

- [ ] **Step 5: Add the eval bar overshoot**

```css
.evalfill {
  transition: height var(--t-piece) var(--ease);
}
```

- [ ] **Step 6: Typecheck, test, and check the reveal**

Run: `npx tsc --noEmit && npm test`
Then analyze a game and confirm the line draws once and the flares stagger in. Re-render by clicking moves and confirm it does not replay. Enable reduced motion and confirm the graph appears complete with no animation.

- [ ] **Step 7: Commit**

```bash
git add -A
git commit -m "feat(web): the eval graph as an instrument, with the analysis reveal"
```

---

## Task 10: Record the world and verify

Phase 6.

**Files:**
- Replace: `DESIGN.md`
- Replace: `.impeccable/design.json`
- Modify: `README.md` (the DESIGN.md description line)

- [ ] **Step 1: Run the full verification**

```bash
npx tsc --noEmit && npm test
```

Then a browser pass at desktop width and at 375px covering: the hero and its loop, both surfaces, the score column with a real analyzed game, the level picker, the readout during a live search, the graph reveal, keyboard focus on every control, and the reduced-motion path. Fix everything the pass finds in one batch.

- [ ] **Step 2: Run the design detector**

```bash
sh "$HOME/.claude/skills/impeccable/scripts/impeccable" detect --json web/src/style.css web/index.html
```

Act on real findings. Record any deliberate exception with its reason rather than silently ignoring it.

- [ ] **Step 3: Rewrite `DESIGN.md` from the shipped result**

Replace the Club Room document entirely, following the same structure it already uses: YAML frontmatter with the tokens from Task 1, then Overview, Colors, Typography, Layout, Elevation & Depth, Shapes, Components, and Do's and Don'ts. North Star is "The Analysis Room". Carry The Lamp Never Judges Rule and The Rationed Light Rule verbatim from the Global Constraints above. Document what shipped, not what was planned.

- [ ] **Step 4: Regenerate `.impeccable/design.json`**

Rewrite the sidecar to schemaVersion 2 against the new tokens: `colorMeta` with tonal ramps for every color, `shadows` (board, popover, focus ring), `motion` (the four motion tokens), `breakpoints` (1024px stack, 560px phone), component snippets for the lamp button, ghost button, dropdown, score row with annotation, level picker, eval bar, and readout, and `narrative` pulled verbatim from the new DESIGN.md.

- [ ] **Step 5: Update the README line**

Change the DESIGN.md link description from "The Club Room" to "The Analysis Room".

- [ ] **Step 6: Commit**

```bash
git add -A
git commit -m "docs: record the Analysis Room design system from the shipped build"
```

---

## Self-review notes

Checked against the spec, section by section.

- Colors, typography, shape, elevation, atmosphere, motion: Task 1, with the graph's motion in Task 9.
- Hero: Tasks 4 and 5.
- Play surface, readout: Tasks 7 and 8.
- Analyze surface, score column: Tasks 2 and 3.
- Components (dropdown, level picker, segmented, readout, eval bar, eval graph, empty states): Tasks 6 through 9.
- DESIGN.md and sidecar replacement: Task 10.

Two gaps found while reviewing and closed above: `parseInfo` did not carry `nodes` or `nps`, so the readout needed a parser change (now Task 8 Steps 1 to 4); and `buildGraph` hard-coded a 120px height that the spec raises to 160px (now Task 9 Steps 1 and 2).

One naming risk to watch: `main.ts` currently calls its tab switcher inline. Task 5 assumes a named function; if none exists, extract one rather than duplicating the class toggling.

Tasks 1, 3, 5, 7, and 10 have no unit test, because their deliverable is CSS and markup. Their gate is a clean typecheck, a green suite, and the stated visual check. Do not fabricate a unit test for a stylesheet.
