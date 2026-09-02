# Chess Coach

A personal chess-coaching website plus a chess engine written from scratch in C++,
built as two parallel learning tracks: beginner-friendly coaching, and a real engine
you understand every layer of.

## About

Chess Coach is a learning project with two goals that reinforce each other: build a
genuinely useful coaching tool, and understand a chess engine by writing one line by
line rather than pulling in a library. The engine is C++ from the ground up (values,
board, move rules, search, evaluation), and the guiding rule is **correctness before
speed**: get the rules provably right with simple data structures first, optimize
later. Progress is deliberate and milestone-by-milestone, each step tested before the
next begins.

Today the engine can represent any position, round-trip it through FEN, and generate
all legal moves, with correctness proven by `perft` (leaf-node counts matching
published reference numbers). It does not yet search or evaluate, so it does not yet
play; that comes in later milestones. Until the engine can play, the coaching side
uses **Stockfish** for analysis.

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
- [ ] **M4. Evaluation v1** — material, then piece-square tables.
- [ ] **M5. Search** — minimax to alpha-beta to iterative deepening.
- [ ] **M6. UCI interface** — play a full game vs Stockfish through a GUI.
- [ ] **M7. WASM + optimization** (later) — bitboards, transposition table, move
  ordering; run in the browser.

Coach track (web) runs in parallel on Stockfish, then adopts the engine over the UCI seam.

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

## Layout

```
engine/   # C++ engine (CMake project: src/, tests/, third_party/doctest.h)
web/      # coach frontend (added when the web track starts)
docs/     # design spec and per-milestone implementation plans
```

## Docs

- Design spec: [docs/superpowers/specs/2026-08-27-chess-coach-design.md](docs/superpowers/specs/2026-08-27-chess-coach-design.md)
- Plans: [M1 board + FEN](docs/superpowers/plans/2026-08-27-engine-m1-board-representation.md),
  [M2 move generation + perft](docs/superpowers/plans/2026-08-29-engine-m2-move-generation.md),
  [M3 make / unmake](docs/superpowers/plans/2026-09-01-engine-m3-make-unmake.md)
