# Chess Coach + From-Scratch Engine — Design Spec

**Date:** 2026-08-27
**Status:** Approved architecture, pending final spec review

## 1. Purpose & Goals

Build a personal chess-coaching website that measurably improves the author's
game (currently beginner, <1000), AND build a chess engine from scratch in C++
as a first-class systems/algorithms learning journey. Both tracks are equally
important; learning the theory at every layer is an explicit goal, not just
shipping a working product.

**Success looks like:**
- A web app that imports the author's games, flags blunders, and explains them
  in beginner-friendly language.
- A hand-written C++ engine that provably generates legal moves (perft), plays
  legal chess via search, and speaks UCI.
- The author understanding every layer they built.

## 2. Author Context (drives every calibration decision)

- **Chess level:** Beginner (<1000). Coaching must target concrete beginner
  failure modes: hanging pieces, missed free captures, missed checks/mates,
  opening principles, not blundering.
- **Programming level:** Still building fundamentals; C++ chosen deliberately to
  learn systems programming and manual memory management.
- **Pace:** Intense push. Milestones can be substantial chunks; move fast.
- **Platform:** Windows 11.

## 3. Core Architectural Idea — the UCI Seam

The engine and the coach communicate through the **UCI protocol** (Universal
Chess Interface): plain text over stdin/stdout (`position ...`, `go`,
`bestmove ...`).

Consequences that make this the backbone of the whole project:
- Stockfish also speaks UCI, so the coach is written against "a UCI engine,"
  not against our specific engine.
- The coach can use **Stockfish** for accurate analysis immediately, then swap
  in our engine at the same socket the moment it can play legal moves.
- Our engine and Stockfish can analyze the same position side by side — an
  instant, motivating benchmark and the author's best feedback loop.
- Neither track blocks the other.

## 4. Two Tracks

### Track A — The Engine (C++)

A testable staircase; each rung teaches one concept and proves itself before the
next:

| Step | Deliverable | Concept learned | Correctness test |
|------|-------------|-----------------|------------------|
| 0 | Toolchain: compiler + CMake building native "hello world" on Windows | Build systems | It builds and runs |
| 1 | Board representation (squares, pieces, side to move, castling, en passant) | Modeling state | Print the start position (FEN) |
| 2 | Legal move generation | Chess rules, the core of the engine | **perft**: leaf-node counts match published/Stockfish numbers |
| 3 | Make / unmake move | State discipline, manual memory care | perft still matches after make+unmake |
| 4 | Evaluation v1 (material, then piece-square tables) | Turning judgment into math | Sanity: obvious material wins score correctly |
| 5 | Search: minimax → alpha-beta → iterative deepening | The core play algorithm | Stops hanging queen; finds mate-in-1, then -in-2 |
| 6 | UCI interface | Interop protocol | Plays a full game vs Stockfish via a GUI |
| 7 (later) | WASM build + optimization (bitboards, transposition table, move ordering) | Performance systems | Faster perft/search; deeper search in browser |

**perft is the safety net:** matching published perft numbers means the move
rules are provably correct, which prevents the most demoralizing class of
silent bugs.

### Track B — The Coach (web)

| Step | Deliverable | Notes |
|------|-------------|-------|
| 1 | Import games | Start with paste-a-PGN; add Chess.com / Lichess public-API import by username later |
| 2 | Analyze every move | Feed each position to a UCI engine (Stockfish now, ours later); record eval; classify each move by eval swing: good / inaccuracy / mistake / blunder |
| 3 | Explain blunders in beginner terms | Rule layer on top of engine numbers: hanging piece, missed free capture, missed check/mate, etc. This is the coach's soul |
| 4 | Pattern detection across games | e.g. "you hang pieces most in the opening," "you miss back-rank threats" — turns analysis into coaching |
| 5 | Drill from your own mistakes | Serve the author's real blunder positions back as puzzles |

The coach is mostly "engine numbers + human-readable beginner-chess rules." That
rule layer *is* beginner chess theory encoded, so building it teaches the author
chess. Building the engine's eval teaches what an engine values. The tracks feed
each other.

## 5. Cross-Track Build Order

1. **Engine 0–2** (toolchain → board → legal moves, perft passing). First "it
   provably works."
2. **Coach 1–2 with Stockfish** (paste PGN → every move analyzed). The website
   does something useful without waiting on our engine.
3. **Coach 3** (explain blunders). Now it genuinely coaches.
4. **Engine 3–6** (make/unmake → eval → search → UCI). Our engine starts
   replacing Stockfish in the socket.
5. Iterate: better eval, better coach rules, then the WASM/optimization deep
   dive (Engine 7).

## 6. Tech Stack

- **Engine:** C++17, CMake. Native builds for fast dev/testing; Emscripten →
  WebAssembly later (Engine step 7) for in-browser play.
- **Engine analysis for the coach (now):** Stockfish, driven over UCI. In the
  browser this can be `stockfish.js` (WASM) running in a Web Worker so there is
  no server to operate.
- **Web frontend:** Vite + TypeScript (lean, fast, good learning ergonomics).
  - Board UI: `chessground` (Lichess's board component).
  - Move legality / PGN parsing in the browser: `chess.js`.
- **Game import:** paste-a-PGN first; Lichess and Chess.com public APIs later.
- **Hosting:** static site (free) is sufficient while analysis runs client-side.

Rationale for a single simple frontend stack: the author is still building
fundamentals, so the frontend deliberately stays minimal to keep cognitive load
on the C++ engine, which is where the hard learning is.

## 7. Repository Layout (initial)

```
Chess_Coach/
  engine/        # C++ engine (its own CMake project)
  web/           # Vite + TS coach frontend (added when Track B starts)
  docs/          # specs, theory notes, learning log
```

## 8. Immediate First Milestone (what we build right after this spec)

**Engine steps 0–2**, delivered as sub-milestones:

- **0. Toolchain:** a C++ compiler + CMake building and running a native
  "hello world" on Windows. This is the real cliff; we clear it first and
  explicitly.
- **1. Board representation:** model squares, pieces, side to move, castling
  rights, en passant target, halfmove/fullmove clocks. Parse and print FEN.
  Test: round-trip the start position and a few known FENs.
- **2. Legal move generation + perft:** generate all legal moves from any
  position; write a `perft(depth)` counter; verify counts against published
  numbers for the start position and standard test positions (e.g. Kiwipete).

Theory is taught alongside each sub-milestone, with pointers to the Chess
Programming Wiki (the project's reference bible).

## 9. Non-Goals (YAGNI, for now)

- No accounts, login, or user database (single-user, local/static to start).
- No bitboards, transposition tables, or opening books in the first engine
  pass — correctness before speed (those arrive at Engine step 7).
- No mobile app; web only.
- No live-play-against-the-engine online feature until the engine speaks UCI and
  is compiled to WASM.
- No paid/hosted backend while analysis can run client-side.

## 10. Key Risks & Mitigations

- **Risk: C++ toolchain friction on Windows stalls momentum.** Mitigation: make
  step 0 an explicit, isolated milestone; get "hello world" green before any
  chess code.
- **Risk: silent move-generation bugs.** Mitigation: perft against published
  numbers from step 2 onward; never advance a rung with perft failing.
- **Risk: two tracks sprawl.** Mitigation: the UCI seam keeps them decoupled;
  follow the cross-track build order; Stockfish covers analysis until the engine
  is ready.
```
