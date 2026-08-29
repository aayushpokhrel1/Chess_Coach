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
- Current plan: [docs/superpowers/plans/2026-08-29-engine-m2-move-generation.md](docs/superpowers/plans/2026-08-29-engine-m2-move-generation.md)
- Done: [Milestone 1 plan (board + FEN)](docs/superpowers/plans/2026-08-27-engine-m1-board-representation.md)

## Status

**Milestone 1 (complete):** C++ toolchain + tested board representation with FEN
(parse, generate, round-trip).

**Milestone 2 (complete):** legal move generation (all pieces, castling, en
passant, promotion) + `perft` verified against published counts for the start
position, Kiwipete, and CPW Position 3. Copy-make; make/unmake comes in M3.

Next milestone: make/unmake move (state discipline), verified by unchanged perft.

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
