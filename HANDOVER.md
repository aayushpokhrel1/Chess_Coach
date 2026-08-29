# Session Handover

Living context file. At the **start** of a new session say:
> "Read HANDOVER.md and continue."

At the **end** of a session say:
> "Update HANDOVER.md and the README."

Keep this file short and current. It is the fast path back into the project.

---

## Project in one line

Chess-coaching website + a from-scratch C++ chess engine, two parallel tracks
joined at the UCI protocol. See [README.md](README.md),
[spec](docs/superpowers/specs/2026-08-27-chess-coach-design.md),
[plan](docs/superpowers/plans/2026-08-27-engine-m1-board-representation.md).

## How we work (decided, do not re-litigate)

- **Engine language:** C++ (chosen to learn systems programming).
- **Execution style:** subagent-driven, BUT coach me through each task after the
  subagent produces the code (explain the C++, then move on).
- **Toolchain:** MSYS2 UCRT64 (`gcc`, `cmake`, `ninja`).
- **Test framework:** doctest (vendored single header).
- **Board rep:** simple 64-square array for now, bitboards much later.
- **Coach analysis:** Stockfish over UCI now, our engine swaps in later.
- **Writing style:** no dashes in prose/comments/commits.

## Current status

- **Milestone 1:** toolchain + tested board representation with FEN.
- **On:** Task 2 (core value types). Not started.
- **Blocker:** none. Toolchain is installed and the build/test loop is proven
  (g++ 16.1.0, cmake 4.3.3, ninja 1.13.2 via MSYS2 UCRT64).

Task checklist (from the plan):
- [x] Task 1: toolchain + CMake skeleton + doctest smoke test (commit d821af8)
- [ ] Task 2: core value types (color, piece, square)
- [ ] Task 3: Board struct + FEN parsing
- [ ] Task 4: FEN generation + round-trip tests

## Coaching note (important, see memory)

Aayush is LEARNING C++ here and wants to be coached THROUGH each task
interactively: one concept at a time, why before code, check understanding, let
him drive the pace. Do NOT build the whole task then hand over a summary, that
reads as skipping him. (Memory: `coaching-style-interactive`.)

## Running the build (Windows quirks)

- The Bash tool (git-bash) does NOT see the MSYS2 toolchain. Run cmake/ninja/ctest
  via PowerShell, refreshing PATH first:
  `$env:Path = [Environment]::GetEnvironmentVariable("Path","User") + ";" + [Environment]::GetEnvironmentVariable("Path","Machine")`
- Full loop:
  `cmake -S engine -B engine/build -G Ninja; cmake --build engine/build; ctest --test-dir engine/build --output-on-failure`
- **Gotcha (fixed):** the test exe is `-static` linked on purpose. Without it the
  exe loads an older `libstdc++` from Git for Windows' bundled MinGW (earlier on
  PATH) and dies at startup with `0xc0000139`. Keep `-static`.

## Next action

Task 2: core value types (`Color`, `PieceType`, `Piece`, `Square`, char
conversions), test-first, in `engine/src/types.{hpp,cpp}` and
`engine/tests/test_types.cpp` (empty stubs already exist). Teach the `.hpp`/`.cpp`
split and `enum class` as we go. A mini-lesson on the compilation model (header =
promises, source = bodies, linker connects them) is mid-flight, waiting on
Aayush's answer to a check question.

## Session log

- **2026-08-28:** Toolchain installed (MSYS2 UCRT64). Scaffolded and completed
  Task 1: CMake skeleton, vendored doctest 2.4.11, proved the test loop (smoke
  test fail then pass), fixed a MinGW DLL startup crash with `-static`. Committed
  (d821af8). Course-corrected on coaching style: teach interactively, not a
  post-hoc summary. Explained Task 1 in full. Task 2 not yet started.
- **2026-08-27:** Brainstormed and locked the design (two tracks, UCI seam).
  Wrote and committed the spec, the Milestone 1 plan, and the README. No code
  yet. Repo is local only until pushed to GitHub.
