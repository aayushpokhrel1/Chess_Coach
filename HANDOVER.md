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
- **On:** Task 1 (toolchain + CMake skeleton + doctest). Not started.
- **Blocker:** waiting on the user to install the MSYS2 toolchain (only the user
  can do this). Build/test steps cannot run until `g++ && cmake && ninja` all
  report versions.

Task checklist (from the plan):
- [ ] Task 1: toolchain + CMake skeleton + doctest smoke test
- [ ] Task 2: core value types (color, piece, square)
- [ ] Task 3: Board struct + FEN parsing
- [ ] Task 4: FEN generation + round-trip tests

## Next action

1. User installs toolchain (plan Task 1, Step 1) and confirms three versions.
2. Subagent scaffolds Task 1 files (CMakeLists, download `doctest.h`, stubs,
   smoke test).
3. Build → watch smoke test fail → fix → pass → commit. Coach through CMake +
   doctest + the `.hpp`/`.cpp` split.

## Open question for next session

- Scaffold Task 1 files before the toolchain is installed (ready to build), or
  keep strictly in order? Either is fine.

## Session log

- **2026-08-27:** Brainstormed and locked the design (two tracks, UCI seam).
  Wrote and committed the spec, the Milestone 1 plan, and the README. No code
  yet. Repo is local only until pushed to GitHub.
