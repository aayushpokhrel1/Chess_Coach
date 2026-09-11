# Chess Coach

A personal chess-coaching website plus a chess engine written from scratch in C++,
built as two parallel learning tracks: beginner-friendly coaching, and a real engine
you understand every layer of.

**Live demo: https://aayushpokhrel1.github.io/Chess_Coach/** (play the C++ engine, or paste
a game and get it analyzed by Stockfish with beginner explanations and drills).

## About

Chess Coach is a learning project with two goals that reinforce each other: build a
genuinely useful coaching tool, and understand a chess engine by writing one line by
line rather than pulling in a library. The engine is C++ from the ground up (values,
board, move rules, search, evaluation), and the guiding rule is **correctness before
speed**: get the rules provably right with simple data structures first, optimize
later. Progress is deliberate and milestone-by-milestone, each step tested before the
next begins.

Today the engine represents any position and round-trips it through FEN, generates all
legal moves (correctness proven by `perft`, leaf-node counts matching published reference
numbers), evaluates positions (material plus piece-square tables), and searches with
negamax, alpha-beta pruning (captures ordered most-valuable-victim first), iterative
deepening, and **quiescence search** so it stops banking material it is about to lose to a
recapture. It **plays a full game over UCI under real time controls** and reports a full
`info` line (score, nodes, principal variation). The coaching side runs on **Stockfish**
for accurate analysis; the next step compiles our engine to WebAssembly so you can play a
game against it in the browser (our engine is not fast enough to replace Stockfish for deep
analysis, so Stockfish stays the analyst and our engine becomes the sparring opponent).

## Two tracks, one seam

- **Engine (C++):** board representation, legal move generation, search, evaluation.
  Compiles to WebAssembly later for the browser.
- **Coach (web):** import your games, flag blunders, explain them in plain language,
  drill your own mistakes.

They meet at the **UCI protocol** (the standard chess-engine text interface). The coach
talks to "a UCI engine," so it uses Stockfish for accurate analysis now and swaps in our
own engine as it matures. Neither track blocks the other.

## Roadmap

Engine milestones (correctness before speed):

- [x] **M0. Toolchain** — native C++ build on Windows (MSYS2 UCRT64, CMake, Ninja, doctest).
- [x] **M1. Board + FEN** — 64-square board, parse and generate FEN, round-trip tested.
- [x] **M2. Legal move generation + perft** — all pieces, castling, en passant,
  promotion; `perft` matches published counts (start, Kiwipete, CPW Position 3).
- [x] **M3. Make / unmake** — in-place move/undo with a small undo record,
  replacing copy-make on the perft hot path; perft counts unchanged.
- [x] **M4. Evaluation v1** — material, then piece-square tables (side-to-move perspective).
- [x] **M5. Search** — negamax with alpha-beta pruning and iterative deepening; stops
  hanging a queen, finds mate in one, then mate in two.
- [x] **M6. UCI interface** — `chess_engine` executable speaks UCI (`position`, `go`,
  `bestmove`) with real time management; plays a full game in a GUI.
- [x] **M7a. Quiescence + move ordering** — MVV-LVA capture ordering and a quiescence
  search (with check evasions) that fixes the horizon-effect blunders; the engine now
  beats a handicapped Stockfish it used to lose to. The search also returns a principal
  variation and emits a full UCI `info` line.
- [x] **M7b. WebAssembly + play**: the engine compiles to WASM (Emscripten) and runs in the
  browser in a Web Worker, powering a "play a game against your engine" mode in the coach.
  Stockfish stays the analysis engine (ours is too slow for depth-12); ours is the opponent
  you play. Phase 1 (the UCI `info` line and principal variation) also landed here.
- [x] **M7c. Optimization**: a transposition table (Zobrist hashing, Exact/Lower/Upper bounds,
  stored-move-first), then killer-move and history quiet-move ordering, then null-move pruning, then
  a bitboard board representation (hybrid: a `uint64_t` per piece kind alongside the mailbox, with
  classical-ray sliding attacks and a full bitboard move generator, proven by unchanged perft). The
  start-position depth-6 search fell from about 24s to about 0.35s (roughly 68x). The play levels
  were re-calibrated against the stronger engine (Beginner ~1190 / Intermediate ~1520 / Advanced
  ~1660, measured vs Stockfish).
- [x] **More search speed**: late move reductions (search late-ordered quiet moves shallower, verify
  if they beat alpha), aspiration windows (search each iterative-deepening depth in a narrow window
  around the previous score, re-search only on a fail; about 44% fewer nodes at depth 7+), incremental
  Zobrist (update the hash by XOR in make/unmake instead of rescanning the board every node), and
  magic bitboards (slider attacks by a single perfect-hash table lookup instead of a ray scan). Each
  is toggle-guarded and proven exact (unchanged perft, or a node-count / differential test).
- [x] **Evaluation v2**: on top of material + piece-square tables, the engine now scores mobility
  (how many squares each piece reaches), the bishop pair, doubled/isolated pawns, king safety
  (a pawn shield in front of the castled king), passed pawns (a pawn nothing can stop from
  promoting, worth more the further it has advanced), and rooks on open / half-open files, so it
  plays more purposeful, developing chess rather than only counting material. Terms are validated by
  A/B self-play (`web/scripts/ab.mjs`) before they ship: passed pawns measured +31 Elo (LOS 96%) and
  open files +24 Elo (LOS 98%) over their absence, while a knight-outpost term was tried and dropped
  when it failed to show a confident gain (a strict outpost is too rare to move the needle).

Coach milestones (web, runs on Stockfish now, adopts our engine over the UCI seam):

- [x] **C1. Import, analyze, explain** — paste a PGN, step through it on a board, analyze
  every move with Stockfish (in a Web Worker), classify each by eval swing
  (best / good / inaccuracy / mistake / blunder), and explain the mistakes in beginner
  language derived from the engine's own line.
- [x] **C2. Pattern detection** — paste several games, give your username, and get your
  mistakes and blunders broken down by phase (opening/middlegame/endgame) and category
  (hanging a piece / missing a mate / missing a capture / other), with a headline insight.
- [x] **C3. Drills** — replay your own mistake and blunder positions as puzzles on a
  movable board; your move is graded live by Stockfish (any non-losing move solves it).
- [x] **C4. Polish** — import games by username from Lichess and Chess.com, an eval bar
  beside the board, remembered sessions (localStorage), and under-promotion in drills.
- [x] **C5. Eval graph** — a chess.com-style evaluation graph across the game with blunder
  markers and the engine's best line at each point.

## Build

Toolchain: MSYS2 UCRT64 with `gcc`, `cmake`, `ninja` (see the M1 plan, Task 1, Step 1,
for one-time setup).

```bash
cmake -S engine -B engine/build -G Ninja
cmake --build engine/build
ctest --test-dir engine/build --output-on-failure
```

On Windows the MSYS2 toolchain may not be on the default PATH; run the commands from a
shell where `C:\msys64\ucrt64\bin` is on PATH.

The build produces `engine/build/chess_engine.exe`, a UCI engine you can load into a chess
GUI (Arena, CuteChess) or drive by hand:

```
uci
position startpos moves e2e4
go movetime 1000
```

### Coach (web)

```bash
cd web
npm install
npm run dev      # Vite dev server; paste a PGN, Load, then Analyze
npm test         # unit tests (Vitest)
```

Analysis runs fully in the browser (Stockfish single-threaded WASM in a Web Worker), so the
site hosts as static files with no backend. The "Play the engine" tab runs OUR engine, compiled
to WASM, in its own Web Worker: pick a color, a strength level (labeled with a measured Elo, plus
a random-move Novice for beginners), and a time control (a real clock for both sides), then play.
You can step back and forth through the game, resign, and hand the finished game to the analysis
tab. Strength levels come from the rating gauntlet in `web/scripts/gauntlet.mjs`.

To rebuild the WASM engine (needs Emscripten; the built `web/public/engine/chesscoach.{js,wasm}`
is committed so this is only needed after engine changes), from the repo root with the emsdk env
sourced: `emcmake cmake -S engine -B engine/build-wasm -G Ninja && cmake --build engine/build-wasm`.

Play our engine against Stockfish headlessly (needs `engine/build/chess_engine.exe` built first):

```bash
cd web
node scripts/match.mjs --skill 2     # handicapped Stockfish (watchable)
node scripts/match.mjs --skill 20    # full-strength Stockfish
```

The referee uses chess.js as the arbiter and prints the moves, result, and PGN.

## Layout

```
engine/   # C++ engine (CMake project: src/, tests/, third_party/doctest.h)
web/      # coach frontend (Vite + TS: src/, public/stockfish/ vendored engine)
docs/     # design spec and per-milestone implementation plans
```

## Docs

- Design spec: [docs/superpowers/specs/2026-08-27-chess-coach-design.md](docs/superpowers/specs/2026-08-27-chess-coach-design.md)
- Plans: [M1 board + FEN](docs/superpowers/plans/2026-08-27-engine-m1-board-representation.md),
  [M2 move generation + perft](docs/superpowers/plans/2026-08-29-engine-m2-move-generation.md),
  [M3 make / unmake](docs/superpowers/plans/2026-09-01-engine-m3-make-unmake.md),
  [M4 evaluation v1](docs/superpowers/plans/2026-09-01-engine-m4-evaluation.md),
  [M5 search](docs/superpowers/plans/2026-09-01-engine-m5-search.md),
  [M6 UCI interface](docs/superpowers/plans/2026-09-02-engine-m6-uci.md),
  [Coach C1 import / analyze / explain](docs/superpowers/plans/2026-09-02-coach-m1-analyze-explain.md),
  [Coach C2 pattern detection](docs/superpowers/plans/2026-09-02-coach-m2-patterns.md),
  [Coach C3 drills](docs/superpowers/plans/2026-09-02-coach-m3-drills.md),
  [Coach C4 polish](docs/superpowers/plans/2026-09-02-coach-m4-polish.md),
  [M7a quiescence](docs/superpowers/plans/2026-09-05-engine-m7a-quiescence.md),
  [M7b WASM + play](docs/superpowers/plans/2026-09-05-engine-m7b-wasm.md),
  [M7c transposition table](docs/superpowers/plans/2026-09-06-engine-m7c-transposition-table.md),
  [M7c bitboards](docs/superpowers/plans/2026-09-07-engine-m7c-bitboards.md)
