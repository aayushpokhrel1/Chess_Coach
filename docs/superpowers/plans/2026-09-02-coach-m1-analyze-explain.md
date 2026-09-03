# Coach Milestone 1: Import, Analyze, Explain Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** A single-page web app where you paste a PGN, step through the game on a board, and get every move analyzed by Stockfish, classified (best / good / inaccuracy / mistake / blunder), with beginner-language explanations for the mistakes and blunders.

**Architecture:** A Vite + TypeScript static site. Small pure modules do the logic and are unit-tested (`pgn` parses with chess.js; a UCI line parser; `classify` buckets the eval swing; `explain` turns the engine's own best line into beginner text). One integration module wraps **Stockfish in a Web Worker** and speaks UCI over `postMessage`. The UI (chessground board + annotated move list + explanation panel) wires them together. Analysis runs entirely client-side, so the site hosts statically with no backend. Explanations are **derived from Stockfish's output** (eval swing + best move + principal variation), not from re-implemented tactic detection.

**Tech Stack:** Vite, TypeScript, [chess.js](https://github.com/jhlywa/chess.js) (PGN parse + move interpretation), [chessground](https://github.com/lichess-org/chessground) (board UI), [stockfish](https://github.com/nmrugg/stockfish.js) (single-threaded WASM in a Web Worker), Vitest (unit tests). Node 24 / npm 11 already installed.

**Spec:** `docs/superpowers/specs/2026-08-27-chess-coach-design.md` (§4 Track B steps 1 to 3; §3 the UCI seam; §5 cross-track order; §6 tech stack). This is the first Track B sub-project; steps 4 (pattern detection) and 5 (drills) are later sub-projects.

## Global Constraints

- **New track lives under `web/`** (its own npm project), separate from `engine/`. Repo layout from spec §7.
- **Everything client-side:** no backend, no accounts, no database. Paste-PGN import only (no Chess.com / Lichess API yet).
- **Stockfish is single-threaded** (the single-threaded WASM build). The multi-threaded build needs cross-origin-isolation headers (COOP/COEP) and `SharedArrayBuffer`, which a plain static host does not provide; single-threaded needs none. (`// ponytail: single-threaded engine; go multi-threaded only if analysis speed becomes the bottleneck, and then add COOP/COEP headers`.)
- **Analysis is fixed-depth** (default 12), deterministic and reproducible. (`// ponytail: fixed depth; switch to movetime only if per-move latency needs bounding`.)
- **Score perspective:** Stockfish's `score cp` / `score mate` is always from the **side to move** at the analyzed FEN. To get a move's quality we compare the eval available before the move (mover's perspective) with the eval after it (negate, because it is now the opponent's turn).
- **Explanations are PV-driven:** derive beginner text from the engine's best move, mate flag, and the opponent's best reply. Do not hand-code tactic detectors.
- **TypeScript strict mode on** (Vite's default `tsconfig` for `vanilla-ts`).
- **No dashes (— –)** in any comment, message, or doc.
- **Coaching mode (Mix):** scaffold/config/UI glue is light-touch (delegate-friendly); the Web Worker + UCI async wrapper, the eval-swing classification, and the explanation layer are the parts to understand deeply.
- **Test framework:** Vitest. Pure modules (`pgn`, `uciParse`, `classify`, `explain`) are unit-tested; the worker and UI are verified by a manual smoke run.
- **Commands (run from `web/`):**
  ```
  npm install
  npm run dev        # Vite dev server
  npx vitest run     # unit tests
  ```

---

## File Structure

```
web/
  package.json          # Task 1: deps + scripts
  tsconfig.json         # Task 1 (Vite vanilla-ts default)
  vite.config.ts        # Task 1
  index.html            # Task 1: mounts the app
  public/
    stockfish/          # Task 3: the single-threaded Stockfish build (copied from node_modules)
  src/
    main.ts             # Task 1: entry; grows through Task 6
    board.ts            # Task 1: chessground setup helper
    pgn.ts              # Task 2: parsePgn
    pgn.test.ts         # Task 2
    uciParse.ts         # Task 3: parseInfo / parseBestMove (pure)
    uciParse.test.ts    # Task 3
    engine.ts           # Task 3: Engine class (Web Worker wrapper)
    classify.ts         # Task 4: scoreToCp / classify (pure)
    classify.test.ts    # Task 4
    explain.ts          # Task 5: explain (pure, PV-driven)
    explain.test.ts     # Task 5
    style.css           # Task 6
```

---

## Task 1: Scaffold + a board on screen

**Files:**
- Create: `web/` via Vite (`package.json`, `tsconfig.json`, `vite.config.ts`, `index.html`, `src/main.ts`)
- Create: `web/src/board.ts`
- Modify: `web/index.html`, `web/src/main.ts`

**Interfaces:**
- Produces: `setupBoard(el: HTMLElement, fen?: string): Api` (thin chessground wrapper returning its Api handle).

**Theory (light):** Vite gives a dev server and a TypeScript build with zero config. chessground is Lichess's board renderer: you hand it a container element and a FEN, it draws the position. This task just proves the toolchain and the board render; no chess logic yet.

- [ ] **Step 1: Scaffold the project**

From the repo root:
```
npm create vite@latest web -- --template vanilla-ts
cd web
npm install
npm install chess.js chessground stockfish
npm install -D vitest
```

- [ ] **Step 2: Add the test script**

In `web/package.json`, add to `"scripts"`: `"test": "vitest run"`. Keep the Vite `dev`/`build`/`preview` scripts the template created.

- [ ] **Step 3: Write `src/board.ts`**

`web/src/board.ts`:
```ts
import { Chessground } from 'chessground';
import type { Api } from 'chessground/api';
import 'chessground/assets/chessground.base.css';
import 'chessground/assets/chessground.brown.css';
import 'chessground/assets/chessground.cburnett.css';

const START_FEN = 'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR';

// Render a board into `el`. `fen` is a full FEN or just the piece-placement field.
export function setupBoard(el: HTMLElement, fen: string = START_FEN): Api {
  return Chessground(el, {
    fen: fen.split(' ')[0],
    viewOnly: true,          // analysis board: no dragging pieces for now
    coordinates: true,
  });
}
```

- [ ] **Step 4: Mount it in `main.ts` and `index.html`**

`web/index.html` body:
```html
<body>
  <main id="app">
    <h1>Chess Coach</h1>
    <div id="board" class="board"></div>
  </main>
  <script type="module" src="/src/main.ts"></script>
</body>
```
`web/src/main.ts`:
```ts
import './style.css';
import { setupBoard } from './board';

const boardEl = document.getElementById('board')!;
setupBoard(boardEl);
```
Add a fixed board size to `web/src/style.css` (chessground needs an explicitly sized square container):
```css
.board { width: 480px; height: 480px; }
```

- [ ] **Step 5: Run the dev server and verify**

Run: `npm run dev` and open the shown URL.
Expected: the start position renders on a brown board with coordinates. No console errors.

- [ ] **Step 6: Commit**

```bash
git add web
git commit -m "feat(coach): Vite + TS scaffold with a chessground board"
```

---

## Task 2: Parse a PGN and step through the game

**Files:**
- Create: `web/src/pgn.ts`, `web/src/pgn.test.ts`
- Modify: `web/index.html`, `web/src/main.ts`

**Interfaces:**
- Consumes: `chess.js`, `setupBoard` (Task 1).
- Produces:
  - `interface GameMove { ply: number; san: string; from: string; to: string; mover: 'w' | 'b'; fenBefore: string; fenAfter: string; }`
  - `function parsePgn(pgn: string): GameMove[]`

**Theory:** A PGN is the game's moves in text (`1. e4 e5 2. Nf3 ...`). chess.js parses it and, in verbose history mode, gives each move with the FEN before and after it, which is exactly what per-move analysis needs (feed each `fenBefore` to the engine). Keeping `parsePgn` a pure function (string in, array out) makes it unit-testable without any UI.

- [ ] **Step 1: Write the failing test**

`web/src/pgn.test.ts`:
```ts
import { describe, it, expect } from 'vitest';
import { parsePgn } from './pgn';

describe('parsePgn', () => {
  it('parses a short game into moves with FENs', () => {
    const moves = parsePgn('1. e4 e5 2. Nf3 Nc6 *');
    expect(moves.length).toBe(4);
    expect(moves[0].san).toBe('e4');
    expect(moves[0].from).toBe('e2');
    expect(moves[0].to).toBe('e4');
    expect(moves[0].mover).toBe('w');
    // The first move starts from the initial position.
    expect(moves[0].fenBefore.startsWith(
      'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w')).toBe(true);
    expect(moves[1].mover).toBe('b');
    expect(moves[3].san).toBe('Nc6');
  });
});
```

- [ ] **Step 2: Run, verify it FAILS**

Run: `npx vitest run src/pgn.test.ts`
Expected: FAIL, `parsePgn` is not defined / no such module.

- [ ] **Step 3: Write `src/pgn.ts`**

`web/src/pgn.ts`:
```ts
import { Chess } from 'chess.js';

export interface GameMove {
  ply: number;         // 0-based half-move index
  san: string;         // e.g. "Nf3"
  from: string;        // e.g. "g1"
  to: string;          // e.g. "f3"
  mover: 'w' | 'b';
  fenBefore: string;   // full FEN before the move
  fenAfter: string;    // full FEN after the move
}

// Parse a PGN into a flat list of moves, each carrying the FEN before and after.
// Throws if the PGN is invalid.
export function parsePgn(pgn: string): GameMove[] {
  const chess = new Chess();
  chess.loadPgn(pgn);
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
```

- [ ] **Step 4: Run, verify it PASSES**

Run: `npx vitest run src/pgn.test.ts`
Expected: PASS. (If the chess.js version exposes `load_pgn` instead of `loadPgn`, or omits `before`/`after` on verbose history, adjust to that version's API and re-run; the test asserts the behavior we need.)

- [ ] **Step 5: Wire a textarea + navigation into the UI**

Replace `web/index.html` `<main>` with:
```html
<main id="app">
  <h1>Chess Coach</h1>
  <textarea id="pgn" rows="4" placeholder="Paste PGN here"></textarea>
  <button id="load">Load</button>
  <div id="board" class="board"></div>
  <div id="nav">
    <button id="prev">&larr;</button>
    <span id="ply">-</span>
    <button id="next">&rarr;</button>
  </div>
</main>
```
Replace `web/src/main.ts`:
```ts
import './style.css';
import { setupBoard } from './board';
import { parsePgn, type GameMove } from './pgn';

const boardEl = document.getElementById('board')!;
const board = setupBoard(boardEl);

let moves: GameMove[] = [];
let idx = -1;   // -1 = start position, else index into moves (show fenAfter)

const START = 'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR';
function render() {
  const fen = idx < 0 ? START : moves[idx].fenAfter;
  board.set({ fen: fen.split(' ')[0] });
  document.getElementById('ply')!.textContent =
    idx < 0 ? 'start' : `${idx + 1}. ${moves[idx].san}`;
}

document.getElementById('load')!.addEventListener('click', () => {
  const text = (document.getElementById('pgn') as HTMLTextAreaElement).value;
  try {
    moves = parsePgn(text);
    idx = -1;
    render();
  } catch (e) {
    alert('Could not parse that PGN.');
  }
});
document.getElementById('prev')!.addEventListener('click', () => {
  if (idx >= 0) { idx--; render(); }
});
document.getElementById('next')!.addEventListener('click', () => {
  if (idx < moves.length - 1) { idx++; render(); }
});
```

- [ ] **Step 6: Verify + commit**

Run `npm run dev`, paste `1. e4 e5 2. Nf3 Nc6 *`, click Load, step with the arrows: the board follows the game.
```bash
git add web/src/pgn.ts web/src/pgn.test.ts web/index.html web/src/main.ts
git commit -m "feat(coach): parse PGN and step through the game"
```

---

## Task 3: Stockfish in a Web Worker + a UCI parser

**Files:**
- Create: `web/src/uciParse.ts`, `web/src/uciParse.test.ts`
- Create: `web/src/engine.ts`
- Create: `web/public/stockfish/` (copied engine build)
- Modify: `web/src/main.ts` (a smoke button)

**Interfaces:**
- Consumes: the Stockfish worker; `parseInfo`, `parseBestMove`.
- Produces:
  - `interface Score { cp?: number; mate?: number; }`
  - `interface Info { depth: number; score: Score; pv: string[]; }`
  - `function parseInfo(line: string): Info | null`
  - `function parseBestMove(line: string): string | null`
  - `interface Analysis { score: Score; bestMove: string; pv: string[]; }`
  - `class Engine { analyze(fen: string, depth: number): Promise<Analysis>; quit(): void; }`

**Theory:** A chess engine speaks UCI: you send `position fen ...` then `go depth N`, and it streams `info ...` lines (each with a `score` and a `pv`, the principal variation it is currently considering) ending with `bestmove <move>`. In the browser we run Stockfish in a **Web Worker** so the heavy search does not freeze the page; we talk to it with `postMessage` and listen on `onmessage`. The tricky part is that UCI is a stream of text arriving asynchronously, so the `Engine` wrapper turns "send `go`, collect `info` lines, resolve when `bestmove` arrives" into a clean `Promise`. The line parsing itself is pure text work, so we test `parseInfo` / `parseBestMove` directly.

- [ ] **Step 1: Write the failing test**

`web/src/uciParse.test.ts`:
```ts
import { describe, it, expect } from 'vitest';
import { parseInfo, parseBestMove } from './uciParse';

describe('parseInfo', () => {
  it('reads depth, centipawn score, and pv', () => {
    const info = parseInfo(
      'info depth 12 seldepth 15 multipv 1 score cp -34 nodes 1000 pv e2e4 e7e5 g1f3')!;
    expect(info.depth).toBe(12);
    expect(info.score.cp).toBe(-34);
    expect(info.score.mate).toBeUndefined();
    expect(info.pv).toEqual(['e2e4', 'e7e5', 'g1f3']);
  });

  it('reads a mate score', () => {
    const info = parseInfo('info depth 20 score mate 3 pv d1h5 g6h5 f1c4')!;
    expect(info.score.mate).toBe(3);
    expect(info.pv[0]).toBe('d1h5');
  });

  it('returns null for non-info lines', () => {
    expect(parseInfo('readyok')).toBeNull();
  });
});

describe('parseBestMove', () => {
  it('extracts the best move', () => {
    expect(parseBestMove('bestmove e2e4 ponder e7e5')).toBe('e2e4');
  });
  it('returns null for other lines', () => {
    expect(parseBestMove('info depth 1 ...')).toBeNull();
  });
});
```

- [ ] **Step 2: Run, verify it FAILS**

Run: `npx vitest run src/uciParse.test.ts`
Expected: FAIL, module not found.

- [ ] **Step 3: Write `src/uciParse.ts`**

`web/src/uciParse.ts`:
```ts
export interface Score { cp?: number; mate?: number; }
export interface Info { depth: number; score: Score; pv: string[]; }

// Parse a UCI "info ..." line. Returns null if it is not an info line or has no score.
export function parseInfo(line: string): Info | null {
  if (!line.startsWith('info')) return null;
  const t = line.split(/\s+/);
  const num = (key: string): number | undefined => {
    const i = t.indexOf(key);
    return i >= 0 ? Number(t[i + 1]) : undefined;
  };

  const score: Score = {};
  const si = t.indexOf('score');
  if (si >= 0) {
    if (t[si + 1] === 'cp') score.cp = Number(t[si + 2]);
    else if (t[si + 1] === 'mate') score.mate = Number(t[si + 2]);
  }
  if (score.cp === undefined && score.mate === undefined) return null;

  const pi = t.indexOf('pv');
  const pv = pi >= 0 ? t.slice(pi + 1) : [];
  return { depth: num('depth') ?? 0, score, pv };
}

// Parse a UCI "bestmove <move> [ponder <move>]" line. Returns null otherwise.
export function parseBestMove(line: string): string | null {
  if (!line.startsWith('bestmove')) return null;
  return line.split(/\s+/)[1] ?? null;
}
```

- [ ] **Step 4: Run, verify it PASSES**

Run: `npx vitest run src/uciParse.test.ts`
Expected: PASS.

- [ ] **Step 5: Copy the single-threaded Stockfish build into `public/`**

Inspect `node_modules/stockfish/src/` and copy the single-threaded build (its `.js` plus any `.wasm` it loads, single-threaded, NOT the ones with `part`/multi-thread naming) into `web/public/stockfish/`. Note the exact entry filename; it is loaded by URL below. Single-threaded WASM does not need COOP/COEP headers.

- [ ] **Step 6: Write `src/engine.ts`**

`web/src/engine.ts` (set `ENGINE_URL` to the file copied in Step 5):
```ts
import { parseInfo, parseBestMove, type Info, type Score } from './uciParse';

const ENGINE_URL = '/stockfish/stockfish-single.js';  // adjust to the copied filename

export interface Analysis { score: Score; bestMove: string; pv: string[]; }

export class Engine {
  private worker: Worker;
  private ready: Promise<void>;

  constructor() {
    this.worker = new Worker(ENGINE_URL);
    this.ready = new Promise<void>((resolve) => {
      const onMsg = (e: MessageEvent) => {
        if (String(e.data).includes('uciok')) { this.worker.removeEventListener('message', onMsg); resolve(); }
      };
      this.worker.addEventListener('message', onMsg);
      this.worker.postMessage('uci');
    });
  }

  // Analyze one position to `depth`, resolving with the final score, best move, and pv.
  async analyze(fen: string, depth: number): Promise<Analysis> {
    await this.ready;
    return new Promise<Analysis>((resolve) => {
      let last: Info | null = null;
      const onMsg = (e: MessageEvent) => {
        const line = String(e.data);
        const info = parseInfo(line);
        if (info) last = info;
        const best = parseBestMove(line);
        if (best) {
          this.worker.removeEventListener('message', onMsg);
          resolve({ score: last?.score ?? {}, bestMove: best, pv: last?.pv ?? [] });
        }
      };
      this.worker.addEventListener('message', onMsg);
      this.worker.postMessage(`position fen ${fen}`);
      this.worker.postMessage(`go depth ${depth}`);
    });
  }

  quit() { this.worker.postMessage('quit'); this.worker.terminate(); }
}
```

- [ ] **Step 7: Smoke-test the engine (manual)**

Temporarily add to `web/src/main.ts`:
```ts
import { Engine } from './engine';
const engine = new Engine();
engine.analyze('rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1', 12)
  .then((a) => console.log('startpos analysis', a));
```
Run `npm run dev`, open the console.
Expected: within a second or two, an analysis object logs with a small `score.cp` (the start position is near even, roughly -30..+30) and a plausible `bestMove` like `e2e4`, `d2d4`, or `g1f3`. If nothing logs, fix `ENGINE_URL` to the actual copied filename. Remove the smoke code before committing (or keep it behind a comment).

- [ ] **Step 8: Commit**

```bash
git add web/src/uciParse.ts web/src/uciParse.test.ts web/src/engine.ts web/public/stockfish web/src/main.ts
git commit -m "feat(coach): Stockfish web worker and UCI info parser"
```

---

## Task 4: Analyze the whole game and classify each move

**Files:**
- Create: `web/src/classify.ts`, `web/src/classify.test.ts`
- Modify: `web/src/main.ts` (run the analysis loop; store results)

**Interfaces:**
- Consumes: `Score` (from `uciParse`), `Engine` (Task 3), `GameMove` (Task 2).
- Produces:
  - `type Quality = 'best' | 'good' | 'inaccuracy' | 'mistake' | 'blunder';`
  - `function scoreToCp(score: Score): number` (mate mapped to a large centipawn magnitude)
  - `function classify(evalBeforeCp: number, evalAfterMoverCp: number): { cpLoss: number; quality: Quality }`

**Theory:** A move's quality is not its absolute eval, it is how much worse it is than the best move available. Stockfish reports every score from the side to move's view, so: `evalBefore` is the score of the position before the move (mover to play, so already the mover's view), and `evalAfter` must be **negated** (after the move it is the opponent's turn, so their score is the negative of the mover's). The centipawn loss is `evalBefore - evalAfter`: near zero means the move was about as good as the best; a big positive number means the mover threw away that many centipawns. Mates are folded into the same scale by mapping "mate in n" to a huge centipawn value (bigger for faster mates) so the arithmetic and thresholds still work.

- [ ] **Step 1: Write the failing test**

`web/src/classify.test.ts`:
```ts
import { describe, it, expect } from 'vitest';
import { scoreToCp, classify } from './classify';

describe('scoreToCp', () => {
  it('passes centipawns through', () => {
    expect(scoreToCp({ cp: 120 })).toBe(120);
    expect(scoreToCp({ cp: -80 })).toBe(-80);
  });
  it('maps mate to a large magnitude, faster mate is larger', () => {
    expect(scoreToCp({ mate: 1 })).toBeGreaterThan(scoreToCp({ mate: 5 }));
    expect(scoreToCp({ mate: -1 })).toBeLessThan(scoreToCp({ mate: -5 }));
    expect(scoreToCp({ mate: 3 })).toBeGreaterThan(10000);
  });
});

describe('classify', () => {
  it('near-best play is best/good', () => {
    expect(classify(50, 45).quality).toBe('best');       // cpLoss 5
    expect(classify(50, -20).quality).toBe('good');      // cpLoss 70? -> inaccuracy boundary
  });
  it('buckets by centipawn loss', () => {
    expect(classify(0, -80).quality).toBe('inaccuracy'); // cpLoss 80
    expect(classify(0, -200).quality).toBe('mistake');   // cpLoss 200
    expect(classify(0, -500).quality).toBe('blunder');   // cpLoss 500
  });
  it('never reports negative loss', () => {
    expect(classify(0, 40).cpLoss).toBe(0);              // engine noise: clamp to 0
  });
});
```
Note: the "good" boundary example above has cpLoss 70. Set thresholds so 70 is `inaccuracy` and fix that one assertion to `'inaccuracy'` if needed; the intent is best < 20, good < 50, inaccuracy < 150, mistake < 300, blunder >= 300. Adjust the two borderline expectations to match the final thresholds in Step 3.

- [ ] **Step 2: Run, verify it FAILS**

Run: `npx vitest run src/classify.test.ts`
Expected: FAIL, module not found.

- [ ] **Step 3: Write `src/classify.ts`**

`web/src/classify.ts`:
```ts
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
export function classify(evalBeforeCp: number, evalAfterMoverCp: number):
    { cpLoss: number; quality: Quality } {
  let cpLoss = evalBeforeCp - evalAfterMoverCp;
  if (cpLoss < 0) cpLoss = 0;                 // clamp engine noise / better-than-expected
  let quality: Quality;
  if (cpLoss < 20) quality = 'best';
  else if (cpLoss < 50) quality = 'good';
  else if (cpLoss < 150) quality = 'inaccuracy';
  else if (cpLoss < 300) quality = 'mistake';
  else quality = 'blunder';
  return { cpLoss, quality };
}
```
Reconcile the two borderline test expectations from Step 1 with these thresholds (cpLoss 70 is `inaccuracy`).

- [ ] **Step 4: Run, verify it PASSES**

Run: `npx vitest run src/classify.test.ts`
Expected: PASS.

- [ ] **Step 5: Run the analysis loop in `main.ts`**

Add an `analyzeGame` flow: after Load, analyze `fenBefore` of each move (that gives the best eval available before the move) and `fenAfter` (negate for the mover). Store per move: `{ evalBeforeCp, evalAfterMoverCp, cpLoss, quality, bestMove, pv }`. Add a per-move analysis holder and an "Analyze" button with a progress count:
```ts
import { Engine } from './engine';
import { scoreToCp, classify, type Quality } from './classify';

interface MoveAnalysis {
  evalBeforeCp: number;
  evalAfterMoverCp: number;
  cpLoss: number;
  quality: Quality;
  bestMove: string;   // best at fenBefore (uci)
  bestIsMate: boolean;
  oppBest: string;    // best reply at fenAfter (uci)
  pvBefore: string[];
}
let analyses: MoveAnalysis[] = [];
const engine = new Engine();
const DEPTH = 12;

async function analyzeGame() {
  analyses = [];
  for (let i = 0; i < moves.length; i++) {
    const before = await engine.analyze(moves[i].fenBefore, DEPTH);
    const after = await engine.analyze(moves[i].fenAfter, DEPTH);
    const evalBeforeCp = scoreToCp(before.score);
    const evalAfterMoverCp = -scoreToCp(after.score); // after: opponent to move, negate
    const { cpLoss, quality } = classify(evalBeforeCp, evalAfterMoverCp);
    analyses.push({
      evalBeforeCp, evalAfterMoverCp, cpLoss, quality,
      bestMove: before.bestMove, bestIsMate: before.score.mate !== undefined,
      oppBest: after.bestMove, pvBefore: before.pv,
    });
    document.getElementById('ply')!.textContent = `analyzing ${i + 1}/${moves.length}`;
  }
  render();
}
```
Wire an `#analyze` button in `index.html` to `analyzeGame`. (Full move-list rendering comes in Task 6; for now, logging or the progress text is enough to verify the loop runs and produces sane numbers.)

- [ ] **Step 6: Verify + commit**

Run `npm run dev`, load a short game with a known blunder, click Analyze: the loop completes and the stored `analyses` show a large `cpLoss` / `blunder` on the bad move (inspect via console or a temporary log).
```bash
git add web/src/classify.ts web/src/classify.test.ts web/index.html web/src/main.ts
git commit -m "feat(coach): analyze every move and classify by eval swing"
```

---

## Task 5: Explain the mistakes in beginner language

**Files:**
- Create: `web/src/explain.ts`, `web/src/explain.test.ts`
- Modify: `web/src/main.ts` (attach an explanation to each analyzed move)

**Interfaces:**
- Consumes: `chess.js` (to turn UCI moves into SAN and read captures), `Quality` (Task 4).
- Produces:
  - `interface ExplainInput { fenBefore: string; playedUci: string; quality: Quality; bestMove: string; bestIsMate: boolean; fenAfter: string; oppBest: string; }`
  - `function explain(input: ExplainInput): string`

**Theory:** This is the coach's soul, and the lazy, honest way to build it is to speak for the engine rather than re-derive chess tactics. We already have the engine's judgment (the quality), its recommended move, whether that recommendation is a forced mate, and the opponent's best reply to what was actually played. From those four facts, plus chess.js to name pieces and squares, we can say the beginner-true thing: you missed a mate, or you dropped a piece (the opponent's best reply is a capture), or you missed a free capture (the engine's best move was a winning capture you skipped). No fork-detector, no hanging-piece scanner, just narration of the engine's own line.

- [ ] **Step 1: Write the failing test**

`web/src/explain.test.ts`:
```ts
import { describe, it, expect } from 'vitest';
import { explain } from './explain';

describe('explain', () => {
  it('names a dropped piece from the opponent best reply', () => {
    // White to move plays Qd1-h5?? (a blunder); Black best reply Nc6xh5 wins the queen.
    // Position: after 1.e4 e5 2.Nf3 Nc6, white just moved queen to h5 hanging it to ...Nxh5? (illustrative)
    const msg = explain({
      fenBefore: 'r1bqkbnr/pppp1ppp/2n5/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 4 3',
      playedUci: 'd1h5',
      quality: 'blunder',
      bestMove: 'f1c4',
      bestIsMate: false,
      fenAfter: 'r1bqkbnr/pppp1ppp/2n5/4p2Q/4P3/5N2/PPPP1PPP/RNB1KB1R b KQkq - 5 3',
      oppBest: 'c6d4',   // whatever the reply; test only checks the shape below
    });
    expect(msg.toLowerCase()).toContain('blunder');
    expect(typeof msg).toBe('string');
    expect(msg.length).toBeGreaterThan(0);
  });

  it('reports a missed mate when the best line was mate', () => {
    const msg = explain({
      fenBefore: 'rnbqkbnr/pppp1ppp/8/4p3/6P1/5P2/PPPPP2P/RNBQKBNR b KQkq - 0 2',
      playedUci: 'd7d5',
      quality: 'blunder',
      bestMove: 'd8h4',
      bestIsMate: true,
      fenAfter: 'rnbqkbnr/ppp2ppp/8/3pp3/6P1/5P2/PPPPP2P/RNBQKBNR w KQkq - 0 3',
      oppBest: 'e1f2',
    });
    expect(msg.toLowerCase()).toContain('mate');
  });

  it('says little for a good move', () => {
    const msg = explain({
      fenBefore: 'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1',
      playedUci: 'e2e4',
      quality: 'good',
      bestMove: 'e2e4',
      bestIsMate: false,
      fenAfter: 'rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq - 0 1',
      oppBest: 'e7e5',
    });
    expect(msg.length).toBe(0);
  });
});
```

- [ ] **Step 2: Run, verify it FAILS**

Run: `npx vitest run src/explain.test.ts`
Expected: FAIL, module not found.

- [ ] **Step 3: Write `src/explain.ts`**

`web/src/explain.ts`:
```ts
import { Chess } from 'chess.js';
import type { Quality } from './classify';

export interface ExplainInput {
  fenBefore: string;
  playedUci: string;   // e.g. "d1h5" or "e7e8q"
  quality: Quality;
  bestMove: string;    // best move at fenBefore (uci)
  bestIsMate: boolean; // the best line at fenBefore is a forced mate
  fenAfter: string;
  oppBest: string;     // best reply at fenAfter (uci)
}

const PIECE_NAME: Record<string, string> = {
  p: 'pawn', n: 'knight', b: 'bishop', r: 'rook', q: 'queen', k: 'king',
};

function uciToMove(fen: string, uci: string) {
  const chess = new Chess(fen);
  const move = { from: uci.slice(0, 2), to: uci.slice(2, 4), promotion: uci.slice(4) || undefined };
  try { return chess.move(move); } catch { return null; }
}

// Beginner-language note for a move, derived from the engine's own judgment.
// Returns "" for best/good moves.
export function explain(input: ExplainInput): string {
  if (input.quality === 'best' || input.quality === 'good') return '';

  const bestSan = uciToMove(input.fenBefore, input.bestMove)?.san ?? input.bestMove;

  // 1. Missed forced mate.
  if (input.bestIsMate) {
    return `${cap(input.quality)}: you missed a forced mate. ${bestSan} was mate.`;
  }

  // 2. Dropped material: the opponent's best reply is a capture.
  const reply = uciToMove(input.fenAfter, input.oppBest);
  if (reply && reply.captured) {
    const piece = PIECE_NAME[reply.captured] ?? 'piece';
    return `${cap(input.quality)}: this drops material. Your opponent can play ${reply.san}, winning your ${piece} on ${reply.to}. Better was ${bestSan}.`;
  }

  // 3. Missed a winning capture the engine preferred.
  const best = uciToMove(input.fenBefore, input.bestMove);
  if (best && best.captured) {
    const piece = PIECE_NAME[best.captured] ?? 'piece';
    return `${cap(input.quality)}: you missed a stronger capture. ${best.san} wins the ${piece} on ${best.to}.`;
  }

  // 4. Fallback: name the better move.
  return `${cap(input.quality)}: a stronger move was ${bestSan}.`;
}

function cap(q: Quality): string { return q.charAt(0).toUpperCase() + q.slice(1); }
```

- [ ] **Step 4: Run, verify it PASSES**

Run: `npx vitest run src/explain.test.ts`
Expected: PASS. Adjust the illustrative FEN/uci fixtures in the test so the intended branch fires (a real dropped-piece reply that chess.js marks as a capture; a real mate best move); the assertions check the message shape, not exact wording.

- [ ] **Step 5: Attach explanations in `main.ts`**

When building each `MoveAnalysis`, also compute and store `explanation: explain({ fenBefore: moves[i].fenBefore, playedUci: moves[i].from + moves[i].to + (promo), quality, bestMove: before.bestMove, bestIsMate: before.score.mate !== undefined, fenAfter: moves[i].fenAfter, oppBest: after.bestMove })`.

- [ ] **Step 6: Commit**

```bash
git add web/src/explain.ts web/src/explain.test.ts web/src/main.ts
git commit -m "feat(coach): beginner explanations derived from the engine line"
```

---

## Task 6: The analysis UI

**Files:**
- Modify: `web/index.html`, `web/src/main.ts`, `web/src/style.css`

**Interfaces:**
- Consumes: everything from Tasks 1 to 5.
- Produces: the assembled coach UI (no new exported functions).

**Theory (light):** This task is presentation: turn the per-move analyses into something a beginner reads at a glance, a move list where each move carries a colored quality badge and its centipawn loss, an eval readout, and, when you land on a move, its explanation. Clicking a move jumps the board there. No new logic, just rendering the data Tasks 2 to 5 already produce.

- [ ] **Step 1: Build the move list + explanation panel**

In `index.html` add, beside the board, `<ol id="moves"></ol>` and `<div id="explain"></div>`. In `main.ts`, render each move as a list item showing `"{n}. {san}"`, a badge class per `quality` (e.g. `quality-blunder`), and the `cpLoss` when it is a mistake or worse; clicking item `i` sets `idx = i` and calls `render()`. In `render()`, show `analyses[idx].explanation` in `#explain` (or clear it at the start position).

- [ ] **Step 2: Style the quality badges**

In `style.css`, add layout for board + move list side by side and color the badges: `.quality-blunder { color: #b00; }`, `.quality-mistake { color: #d67; }`, `.quality-inaccuracy { color: #d9a; }`, `.quality-good`, `.quality-best { color: #393; }`. Keep it minimal.

- [ ] **Step 3: Verify end to end (manual)**

Run `npm run dev`. Paste a real game with a couple of blunders (for example, one of Aayush's own losses), click Load then Analyze. Expected: the move list fills with badges, blunders are red with a centipawn loss, clicking a move jumps the board and shows the explanation, and a blunder's explanation names the dropped piece or the missed mate. Run `npx vitest run` and confirm all unit tests pass.

- [ ] **Step 4: Commit**

```bash
git add web/index.html web/src/main.ts web/src/style.css
git commit -m "feat(coach): analysis UI with quality badges and explanations"
```

---

## Self-Review Notes

- **Spec coverage:** implements Track B steps 1 (paste-PGN import, Task 2), 2 (analyze every move + classify, Tasks 3 to 4), and 3 (explain blunders in beginner terms, Task 5), presented through the UI (Tasks 1, 6). Uses the UCI seam (§3) via Stockfish now, swappable for our engine later (our `chess_engine` speaks the same protocol). Steps 4 (patterns) and 5 (drills) are explicitly out of scope.
- **Type consistency:** `Score` is defined once in `uciParse.ts` and reused by `classify` and `engine`. `Quality` is defined in `classify.ts` and reused by `explain`. `GameMove` (pgn), `Analysis` (engine), `MoveAnalysis` (main) field names are used consistently across tasks. `parsePgn`, `parseInfo`, `parseBestMove`, `scoreToCp`, `classify`, `explain`, and `Engine.analyze` keep the exact signatures declared in their Interfaces blocks.
- **Placeholders:** none in the pure modules; every logic module ships complete code and tests. The two integration points that cannot be pinned without running are called out explicitly: the Stockfish build filename (`ENGINE_URL`, verified in Task 3 Step 7) and the two illustrative FEN fixtures in `explain.test.ts` (adjusted in Task 5 Step 4 so the intended branch fires). Both are verification steps, not vague requirements.
- **Perspective correctness (the subtle point):** `evalAfterMoverCp = -scoreToCp(after.score)` because the post-move position has the opponent to move; without the negation every move would look catastrophic. `cpLoss = evalBefore - evalAfterMover`, clamped at 0. This is the one place a sign error would silently corrupt every classification, so classify is unit-tested directly.
- **Laziness that is load-bearing (ponytail):** explanations reuse Stockfish's best move, mate flag, and the opponent's best reply instead of a bespoke tactics engine; single-threaded Stockfish avoids COOP/COEP hosting headers; fixed depth keeps analysis reproducible. Each is marked and each has a stated upgrade path.

## What the next Track B sub-project will cover (preview, not part of this plan)

Step 4: pattern detection across many games ("you hang pieces most in the opening", "you miss back-rank threats"), which aggregates the per-move classifications this milestone produces. Step 5: serve your own blunder positions back as drills. Both build directly on the `classify` + `explain` output defined here.
