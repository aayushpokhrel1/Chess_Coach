# Chess Coach

A personal chess-coaching website plus a chess engine written from scratch in C++,
built as two parallel learning tracks. Beginner-friendly coaching, and a real engine
you understand every layer of.

## Two tracks, one seam

- **Engine (C++):** board representation, legal move generation, search, evaluation.
  Compiles to WebAssembly later for the browser.
- **Coach (web):** import your games, flag blunders, explain them in plain language,
  drill your own mistakes.

They meet at the **UCI protocol** (standard chess-engine text interface). The coach
talks to "a UCI engine," so it uses **Stockfish** for accurate analysis now and swaps
in our own engine as it matures. Neither track blocks the other.

## Docs

- Design spec: [docs/superpowers/specs/2026-08-27-chess-coach-design.md](docs/superpowers/specs/2026-08-27-chess-coach-design.md)
- Current plan: [docs/superpowers/plans/2026-08-27-engine-m1-board-representation.md](docs/superpowers/plans/2026-08-27-engine-m1-board-representation.md)

## Status

**Milestone 1 (in progress):** C++ toolchain + tested board representation with FEN.

- [ ] Task 1: toolchain + CMake skeleton + doctest
- [ ] Task 2: core value types (color, piece, square)
- [ ] Task 3: Board struct + FEN parsing
- [ ] Task 4: FEN generation + round-trip tests

Next milestone: legal move generation + perft.

## Build (once the toolchain is installed)

Toolchain: MSYS2 UCRT64 with `gcc`, `cmake`, `ninja` (see plan Task 1, Step 1).

```bash
cmake -S engine -B engine/build -G Ninja
cmake --build engine/build
ctest --test-dir engine/build --output-on-failure
```

## Layout

```
engine/   # C++ engine (CMake project)
web/       # coach frontend (added when Track B starts)
docs/      # specs and plans
```
