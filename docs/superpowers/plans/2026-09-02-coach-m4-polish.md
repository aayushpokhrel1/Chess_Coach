# Coach Milestone 4: Polish Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Round off the coach: import your recent games by username from Lichess or Chess.com (no manual paste), show an eval bar beside the board, remember your last session across refreshes, and let drills promote to any piece.

**Architecture:** Four independent additions to the existing coach. A new `import` module fetches concatenated PGN from the Lichess and Chess.com public APIs (both are CORS-enabled) and drops it into the existing PGN box, feeding the existing analysis. A pure `evalBar` module maps a centipawn score to a bar fill; the single-game analysis stores a White-perspective eval per move for it. Session memory is a thin localStorage layer on the PGN box, username, and source. Under-promotion adds a small picker to the drill flow in place of auto-queen.

**Tech Stack:** Same as prior coach milestones: Vite, TypeScript, chess.js, chessground, Stockfish (Web Worker), Vitest. Uses `fetch` for the game-import APIs.

**Spec:** `docs/superpowers/specs/2026-08-27-chess-coach-design.md` (§4 Track B, step 1 "add Chess.com / Lichess public-API import by username later"; general polish). Builds on Coach M1 to M3.

## Global Constraints

- **Client-side only.** Imports call the Lichess and Chess.com public APIs directly from the browser (both send CORS headers, verified). No backend, no keys.
- **Lichess import:** `GET https://lichess.org/api/games/user/{user}?max=N` with `Accept: application/x-chess-pgn` returns concatenated PGN. (`// ponytail: newest N games, no filters; add rated/time-control filters only if asked`.)
- **Chess.com import:** `GET https://api.chess.com/pub/player/{user}/games/archives` returns monthly archive URLs (oldest first); fetch from the newest backward, taking each archive's games (newest last) until N are gathered.
- **Import populates the PGN box**, then the user clicks the existing "Analyze all games". Import does not auto-analyze.
- **Eval bar** shows the position's advantage from White's perspective, clamped to +-1000cp; it reflects the currently viewed move in the single-game viewer (needs analysis first).
- **Session memory:** persist only the PGN text, username, and source select (not analysis results, which are cheap to skip and expensive to serialize). (`// ponytail: persist inputs, not results; add result caching only if re-analysis becomes a pain`.)
- **Under-promotion:** a Q/R/B/N picker appears only when a drill move is a pawn reaching the last rank; otherwise moves apply as before.
- **No dashes (— –)** in any comment, message, or doc.
- **Coaching mode (Mix):** the pure modules (`evalBar`, the Chess.com extractor) and the fetch shapes are the parts to understand; UI glue is light-touch.
- **Test framework:** Vitest. Pure pieces (`evalBar.barPercent`, `import.archivePgns`) are unit-tested; the network fetchers, eval-bar UI, localStorage, and promotion picker are verified by a manual run.
- **Commands (from `web/`):** `npm run dev`, `npx vitest run`, `npx tsc --noEmit`.

---

## File Structure

```
web/src/
  import.ts        # NEW (Task 1): fetchLichess, fetchChessCom, archivePgns
  import.test.ts   # NEW (Task 1): archivePgns
  evalBar.ts       # NEW (Task 2): barPercent
  evalBar.test.ts  # NEW (Task 2)
  main.ts          # MODIFY (Tasks 1-4): import UI, eval bar wiring, localStorage, promotion picker
  index.html       # MODIFY (Tasks 1,2,4): source select + fetch button, eval bar element, promo picker
  style.css        # MODIFY (Tasks 2,4): eval bar + promo picker styling
```

---

## Task 1: Import games from Lichess and Chess.com

**Files:**
- Create: `web/src/import.ts`, `web/src/import.test.ts`
- Modify: `web/index.html`, `web/src/main.ts`

**Interfaces:**
- Produces:
  - `function archivePgns(archive: { games?: { pgn?: string }[] }): string[]`
  - `function fetchLichess(user: string, max: number): Promise<string>`
  - `function fetchChessCom(user: string, max: number): Promise<string>`

**Theory:** Both sites expose read-only public APIs that allow cross-origin requests, so the browser can pull a user's games with no backend. Lichess hands back PGN directly. Chess.com is two hops: a list of monthly archive URLs, then each month's games as JSON with a `.pgn` per game; we walk from the newest month backward until we have enough. Both paths end as one concatenated PGN string, exactly what the existing `splitPgnGames` + analysis already consume. The only pure, testable piece is pulling the PGNs out of a Chess.com archive object; the fetch calls are thin I/O verified by a real request.

- [ ] **Step 1: Write the failing test**

`web/src/import.test.ts`:
```ts
import { describe, it, expect } from 'vitest';
import { archivePgns } from './import';

describe('archivePgns', () => {
  it('pulls the pgn out of each game', () => {
    expect(archivePgns({ games: [{ pgn: 'A' }, { pgn: 'B' }] })).toEqual(['A', 'B']);
  });
  it('handles a missing or empty games list', () => {
    expect(archivePgns({})).toEqual([]);
    expect(archivePgns({ games: [{}, { pgn: '' }] })).toEqual([]);
  });
});
```

- [ ] **Step 2: Run, verify it FAILS**

Run: `npx vitest run src/import.test.ts`
Expected: FAIL, module not found.

- [ ] **Step 3: Write `import.ts`**

`web/src/import.ts`:
```ts
// Fetch the most recent `max` games as concatenated PGN from Lichess.
export async function fetchLichess(user: string, max: number): Promise<string> {
  const url = `https://lichess.org/api/games/user/${encodeURIComponent(user)}?max=${max}`;
  const r = await fetch(url, { headers: { Accept: 'application/x-chess-pgn' } });
  if (!r.ok) throw new Error(`Lichess request failed (${r.status})`);
  return (await r.text()).trim();
}

// Extract the PGN strings from one Chess.com monthly archive JSON object.
export function archivePgns(archive: { games?: { pgn?: string }[] }): string[] {
  return (archive.games ?? []).map((g) => g.pgn ?? '').filter((p) => p.length > 0);
}

// Fetch the most recent `max` games as concatenated PGN from Chess.com.
export async function fetchChessCom(user: string, max: number): Promise<string> {
  const a = await fetch(
    `https://api.chess.com/pub/player/${encodeURIComponent(user)}/games/archives`,
  );
  if (!a.ok) throw new Error(`Chess.com request failed (${a.status})`);
  const archives: string[] = (await a.json()).archives ?? [];
  const pgns: string[] = [];
  for (let i = archives.length - 1; i >= 0 && pgns.length < max; i--) {
    const g = await fetch(archives[i]);
    if (!g.ok) continue;
    const monthly = archivePgns(await g.json()); // oldest first within a month
    for (let j = monthly.length - 1; j >= 0 && pgns.length < max; j--) {
      pgns.push(monthly[j]);
    }
  }
  return pgns.join('\n\n');
}
```

- [ ] **Step 4: Run, verify it PASSES**

Run: `npx vitest run src/import.test.ts`
Expected: PASS.

- [ ] **Step 5: Add the import UI**

In `index.html`, add a controls row above the PGN box (or beside the username row):
```html
      <div class="controls">
        <select id="source">
          <option value="lichess">Lichess</option>
          <option value="chesscom">Chess.com</option>
        </select>
        <input id="fetchUser" placeholder="Username to import" />
        <button id="fetchGames">Fetch games</button>
        <span id="fetchStatus"></span>
      </div>
```
In `main.ts`, wire it:
```ts
import { fetchLichess, fetchChessCom } from './import';

$('fetchGames').addEventListener('click', async () => {
  const src = ($('source') as HTMLSelectElement).value;
  const user = ($('fetchUser') as HTMLInputElement).value.trim();
  if (!user) return;
  $('fetchStatus').textContent = 'fetching...';
  try {
    const pgn = src === 'lichess' ? await fetchLichess(user, 10) : await fetchChessCom(user, 10);
    ($('pgn') as HTMLTextAreaElement).value = pgn;
    // pre-fill the analysis username so "Analyze all games" targets this player
    ($('username') as HTMLInputElement).value = user;
    $('fetchStatus').textContent = pgn ? 'loaded, now click Analyze all games' : 'no games found';
  } catch (e) {
    $('fetchStatus').textContent = `import failed: ${(e as Error).message}`;
  }
});
```

- [ ] **Step 6: Verify + commit**

Run `npm run dev`. Pick Lichess, enter a real username (for example a public account), click "Fetch games": the PGN box fills and the username field is set. Try Chess.com too. Then "Analyze all games" works on the imported games.
```bash
git add web/src/import.ts web/src/import.test.ts web/index.html web/src/main.ts
git commit -m "feat(coach): import games from Lichess and Chess.com by username"
```

---

## Task 2: Eval bar

**Files:**
- Create: `web/src/evalBar.ts`, `web/src/evalBar.test.ts`
- Modify: `web/index.html`, `web/src/main.ts`, `web/src/style.css`

**Interfaces:**
- Produces: `function barPercent(whiteCp: number): number` (White's share of the bar, 0..100).

**Theory:** An eval bar turns the engine's number into something a beginner reads instantly: how much White is ahead. The mapping clamps the centipawn score to a sensible range and converts it to a percentage split; huge (mate) scores peg the bar near full. The single-game analysis already computes each move's eval, so we just store it in White's perspective and drive the bar from the currently viewed move.

- [ ] **Step 1: Write the failing test**

`web/src/evalBar.test.ts`:
```ts
import { describe, it, expect } from 'vitest';
import { barPercent } from './evalBar';

describe('barPercent', () => {
  it('is 50 at equality', () => {
    expect(barPercent(0)).toBe(50);
  });
  it('rises for White and falls for Black, clamped', () => {
    expect(barPercent(300)).toBeCloseTo(65, 5);
    expect(barPercent(100000)).toBe(98); // mate-scale pegs near full
    expect(barPercent(-100000)).toBe(2);
  });
});
```

- [ ] **Step 2: Run, verify it FAILS**

Run: `npx vitest run src/evalBar.test.ts`
Expected: FAIL, module not found.

- [ ] **Step 3: Write `evalBar.ts`**

`web/src/evalBar.ts`:
```ts
// White's share of the eval bar (0..100) from a centipawn score (White's view).
export function barPercent(whiteCp: number): number {
  const clamped = Math.max(-1000, Math.min(1000, whiteCp));
  const pct = 50 + (clamped / 1000) * 50;
  return Math.max(2, Math.min(98, pct));
}
```

- [ ] **Step 4: Run, verify it PASSES**

Run: `npx vitest run src/evalBar.test.ts`
Expected: PASS.

- [ ] **Step 5: Store a White-perspective eval and render the bar**

Add `whiteEvalCp` to the `MoveAnalysis` interface in `main.ts`. In `analyzeGame`, compute it from the mover:
```ts
    const whiteEvalCp = moves[i].mover === 'w' ? evalAfterMoverCp : -evalAfterMoverCp;
```
and include `whiteEvalCp` in the pushed analysis object.

In `index.html`, place a bar beside the board:
```html
        <div class="board-row">
          <div id="evalbar" class="evalbar"><div id="evalfill" class="evalfill"></div></div>
          <div id="board" class="board"></div>
        </div>
```
(Move the existing `<div id="board" class="board"></div>` inside this `board-row`.)

In `main.ts` `render()`, drive the fill (white fills from the bottom):
```ts
  import { barPercent } from './evalBar';  // at top of file
  // inside render(), after computing `a`:
  const white = a ? barPercent(a.whiteEvalCp) : 50;
  ($('evalfill') as HTMLElement).style.height = `${white}%`;
```

- [ ] **Step 6: Style the bar**

In `style.css`:
```css
.board-row { display: flex; gap: 8px; align-items: stretch; }
.evalbar { width: 14px; height: 480px; background: #333; position: relative; border-radius: 3px; overflow: hidden; }
.evalfill { position: absolute; bottom: 0; left: 0; right: 0; background: #eee; height: 50%; transition: height 0.2s; }
```

- [ ] **Step 7: Verify + commit**

Run `npm run dev`, load and Analyze a single game (the C1 "Analyze" button), step through it: the bar swings toward White or Black as the eval changes, and pegs on a decisive move.
```bash
git add web/src/evalBar.ts web/src/evalBar.test.ts web/index.html web/src/main.ts web/src/style.css
git commit -m "feat(coach): eval bar beside the board"
```

---

## Task 3: Remember the last session

**Files:**
- Modify: `web/src/main.ts`

**Interfaces:**
- Consumes: `localStorage`. No new exports.

**Theory:** A refresh should not wipe a pasted or imported game. We persist just the inputs (PGN text, analysis username, import source) to `localStorage` and restore them on load, so the expensive part (analysis) is the only thing you repeat, and only when you choose to.

- [ ] **Step 1: Save inputs on change and restore on load**

In `main.ts`, add near the top after the element helper:
```ts
const SAVE_KEYS = ['pgn', 'username', 'fetchUser'] as const;
function saveSession() {
  try {
    const data: Record<string, string> = { source: ($('source') as HTMLSelectElement).value };
    for (const k of SAVE_KEYS) data[k] = ($(k) as HTMLInputElement | HTMLTextAreaElement).value;
    localStorage.setItem('chesscoach', JSON.stringify(data));
  } catch {
    /* ignore storage errors (private mode, quota) */
  }
}
function restoreSession() {
  try {
    const raw = localStorage.getItem('chesscoach');
    if (!raw) return;
    const data = JSON.parse(raw) as Record<string, string>;
    for (const k of SAVE_KEYS) if (data[k] != null) ($(k) as HTMLInputElement).value = data[k];
    if (data.source) ($('source') as HTMLSelectElement).value = data.source;
  } catch {
    /* ignore */
  }
}
```
Call `restoreSession()` once at startup (after the elements exist, near where the board is set up). Attach `saveSession` to the relevant inputs:
```ts
restoreSession();
['pgn', 'username', 'fetchUser', 'source'].forEach((id) =>
  $(id).addEventListener('input', saveSession),
);
```
(`select` fires `input` on change in modern browsers; if not, also listen for `change`.)

- [ ] **Step 2: Verify + commit**

Run `npm run dev`, paste or import a PGN, set the username, refresh the page: the PGN and username are still there.
```bash
git add web/src/main.ts
git commit -m "feat(coach): remember the last session in localStorage"
```

---

## Task 4: Under-promotion in drills

**Files:**
- Modify: `web/index.html`, `web/src/main.ts`, `web/src/style.css`

**Interfaces:**
- Consumes: the existing drill flow (`onUserMove`). No new exports.

**Theory:** Drills auto-queened promotions, which is wrong when the puzzle's point is a knight or rook promotion. When a dragged drill move is a pawn landing on the last rank, we pause and show a Q/R/B/N picker, then complete the move with the chosen piece. Every other move is unaffected.

- [ ] **Step 1: Add the picker element**

In `index.html`, inside the `#drill` panel, add:
```html
        <div id="promo" class="promo" hidden>
          Promote to:
          <button data-p="q">Q</button>
          <button data-p="r">R</button>
          <button data-p="b">B</button>
          <button data-p="n">N</button>
        </div>
```

- [ ] **Step 2: Route promotions through the picker in `main.ts`**

Replace the auto-queen logic in `onUserMove` so a promotion asks first. Change the promotion detection to await a choice:
```ts
async function onUserMove(orig: string, dest: string) {
  const d = drills[drillIdx];
  const chess = new Chess(d.fen);
  const piece = chess.get(orig as never);
  const isPromo = piece && piece.type === 'p' && (dest[1] === '8' || dest[1] === '1');
  const promo = isPromo ? await pickPromotion() : undefined;
  let fenAfter: string;
  try {
    chess.move({ from: orig, to: dest, promotion: promo });
    fenAfter = chess.fen();
  } catch {
    setDrillBoard(d);
    return;
  }
  // ... unchanged from here (thinking..., analyze, gradeAttempt, feedback, setDrillBoard) ...
}

function pickPromotion(): Promise<'q' | 'r' | 'b' | 'n'> {
  const box = $('promo') as HTMLElement;
  box.hidden = false;
  return new Promise((resolve) => {
    const onClick = (e: Event) => {
      const p = (e.target as HTMLElement).getAttribute('data-p');
      if (!p) return;
      box.hidden = true;
      box.querySelectorAll('button').forEach((b) => b.removeEventListener('click', onClick));
      resolve(p as 'q' | 'r' | 'b' | 'n');
    };
    box.querySelectorAll('button').forEach((b) => b.addEventListener('click', onClick));
  });
}
```

- [ ] **Step 3: Style the picker**

In `style.css`:
```css
.promo { margin: 0.5rem 0; }
.promo button { margin-right: 4px; font-weight: 600; }
```

- [ ] **Step 4: Verify + commit**

Run `npm run dev`, drive a drill whose position has a pawn on the 7th rank (or trust the logic), drag it to the last rank: the picker appears and the chosen piece is used. Run `npx vitest run` and `npx tsc --noEmit`.
```bash
git add web/index.html web/src/main.ts web/src/style.css
git commit -m "feat(coach): under-promotion picker in drills"
```

---

## Self-Review Notes

- **Spec coverage:** implements the deferred Track B step-1 API import (Lichess + Chess.com) and general polish (eval bar, session memory, under-promotion). Nothing here changes the analysis/explain/report/drill logic; it wraps and presents them.
- **Type consistency:** `archivePgns`, `fetchLichess`, `fetchChessCom`, `barPercent` keep the signatures in their Interfaces blocks. `MoveAnalysis` gains `whiteEvalCp: number`, set in `analyzeGame` and read in `render`. `pickPromotion` returns the chess.js promotion letters (`'q'|'r'|'b'|'n'`) that `Chess.move` accepts.
- **Placeholders:** the pure modules ship complete code + tests. Network fetchers, DOM wiring, localStorage, and the picker are verified by the manual runs noted in each task (the APIs' CORS and shapes were confirmed before planning).
- **Failure handling (trust boundaries):** both fetchers throw on non-OK responses and the UI catches and shows the message; localStorage reads/writes are wrapped in try/catch (private mode, quota) so the app still works with storage disabled. These are real error paths, not simplified away.
- **Independence (ponytail):** the four tasks touch mostly separate UI regions and can be built and reviewed in any order; each is small and additive, and none re-opens the analysis pipeline.

## Track B status after this milestone

Coach steps 1 to 5 plus polish: username import, per-move analysis with an eval bar, beginner explanations, cross-game patterns, drills with under-promotion, and remembered sessions. The remaining big lever is engine M7 (WASM), after which the coach could run our own engine in place of Stockfish over the same worker interface.
