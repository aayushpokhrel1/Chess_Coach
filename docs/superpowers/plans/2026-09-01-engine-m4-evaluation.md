# Engine Milestone 4: Evaluation v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Give the engine an opinion: a static `evaluate(const Board&)` that scores a position in centipawns from the side-to-move's perspective, material first, then piece-square tables.

**Architecture:** A new `eval` unit. `material_score(b)` sums piece values from White's perspective (positive favors White); `evaluate(b)` adds a piece-square-table term and flips the sign to the side to move (positive favors whoever is about to move), matching the negamax convention Milestone 5 will use. Piece-square tables are stored in the standard published (rank-8-first) orientation; a white piece looks up its mirrored square, a black piece looks up the square directly, which makes the symmetric start position score exactly 0.

**Tech Stack:** C++17, CMake (+ Ninja), doctest (single-header, vendored at `engine/third_party/doctest.h`), MSYS2/MinGW-w64 UCRT64 toolchain on Windows.

**Spec:** `docs/superpowers/specs/2026-08-27-chess-coach-design.md` (§5 Milestone 4: "Evaluation v1 (material, then piece-square tables)", success = "obvious material wins score correctly"). Builds on Milestones 1-3.

## Global Constraints

- **Language:** C++17 (`CMAKE_CXX_STANDARD 17`, already set).
- **Perspective (decided):** `evaluate` returns centipawns from the **side to move's** perspective (positive = good for the player about to move). `material_score` returns centipawns from **White's** perspective (positive = good for White). Convert by negating when `side_to_move` is Black.
- **Material values (centipawns):** Pawn 100, Knight 320, Bishop 330, Rook 500, Queen 900, King 0.
- **Piece-square tables:** the standard Michniewski "Simplified Evaluation Function" tables, stored rank-8-first (index 0 = a8, index 63 = h1), White's perspective. White piece at square `s` reads `table[mirror(s)]` where `mirror(s) = make_square(file_of(s), 7 - rank_of(s))`; black piece at `s` reads `table[s]`. This makes the start position score exactly 0.
- **Test framework:** doctest, vendored at `engine/third_party/doctest.h`.
- **Every code file stays split** into `.hpp` (declarations) + `.cpp` (definitions).
- **No dashes (— –)** in any comment, message, or doc; use commas, colons, or parentheses.
- **Square indexing (unchanged):** `make_square(file, rank) = rank*8 + file`, file 0..7 = a..h, rank 0..7 = ranks 1..8, `NO_SQUARE = -1`. `PieceType` order is Pawn, Knight, Bishop, Rook, Queen, King, None (integer values 0..6).
- **Reused symbols:** `Color`, `PieceType`, `Piece`, `Square`, `make_square/file_of/rank_of` (`types.hpp`); `Board`, `board_from_fen`, `start_position` (`board.hpp`).
- **Build/test loop (PowerShell; git-bash cannot see the MSYS2 toolchain):**
  ```
  $env:Path = [Environment]::GetEnvironmentVariable("Path","User") + ";" + [Environment]::GetEnvironmentVariable("Path","Machine")
  cmake --build engine/build; ctest --test-dir engine/build --output-on-failure
  ```
  Re-run `cmake -S engine -B engine/build -G Ninja` once after Task 1 adds the new files to CMakeLists.

---

## File Structure

```
engine/
  CMakeLists.txt     # MODIFY (Task 1): add src/eval.cpp and tests/test_eval.cpp
  src/
    eval.hpp         # NEW: piece_value, material_score, evaluate declarations
    eval.cpp         # NEW: material (Task 1); piece-square tables added (Task 2)
  tests/
    test_eval.cpp    # NEW: material + perspective tests (Task 1); PST tests (Task 2)
```

The `eval` unit is self-contained: it reads a `Board` and returns an `int`. It does not depend on move generation, so it slots in cleanly alongside the existing units.

---

## Task 1: Material evaluation + side-to-move perspective

**Files:**
- Create: `engine/src/eval.hpp`
- Create: `engine/src/eval.cpp`
- Create: `engine/tests/test_eval.cpp`
- Modify: `engine/CMakeLists.txt` (add `src/eval.cpp`, `tests/test_eval.cpp`)

**Interfaces:**
- Consumes: `Board`, `Piece`, `PieceType`, `Color`, `Square`.
- Produces:
  - `int piece_value(PieceType t);`
  - `int material_score(const Board& b);` (White's perspective, positive favors White)
  - `int evaluate(const Board& b);` (side-to-move perspective)

**Theory:** Material counting is the floor of every chess evaluation: sum your pieces' values, subtract the opponent's. `material_score` reports that from White's fixed perspective, which is easy to test with exact numbers. `evaluate` is what search will call, so it uses the side-to-move convention: compute the White-perspective score, then negate it if Black is to move. Two facts make this milestone testable without piece-square tables muddying the numbers: the start position is materially balanced (score 0), and flipping only the side to move must negate `evaluate` exactly.

- [ ] **Step 1: Write the failing test**

`engine/tests/test_eval.cpp`:
```cpp
#include "doctest.h"
#include "board.hpp"
#include "eval.hpp"

TEST_CASE("material score is zero at the start") {
    CHECK(material_score(start_position()) == 0);
    CHECK(evaluate(start_position()) == 0);
}

TEST_CASE("material score counts the centipawn difference (White's view)") {
    // White king+queen vs black king: White is up a queen.
    Board upQ = board_from_fen("4k3/8/8/8/8/8/8/3QK3 w - - 0 1");
    CHECK(material_score(upQ) == 900);
    // Black king+rook vs white king: White is down a rook.
    Board downR = board_from_fen("r3k3/8/8/8/8/8/8/4K3 w - - 0 1");
    CHECK(material_score(downR) == -500);
}

TEST_CASE("evaluate is from the side-to-move perspective") {
    // Up a queen is good for White. Same position, different mover.
    Board whiteToMove = board_from_fen("4k3/8/8/8/8/8/8/3QK3 w - - 0 1");
    Board blackToMove = board_from_fen("4k3/8/8/8/8/8/8/3QK3 b - - 0 1");
    CHECK(evaluate(whiteToMove) > 0);                 // good for the mover (White)
    CHECK(evaluate(blackToMove) < 0);                 // bad for the mover (Black)
    CHECK(evaluate(whiteToMove) == -evaluate(blackToMove)); // exact negatives
}
```

- [ ] **Step 2: Register the files, run, verify it FAILS to compile**

In `engine/CMakeLists.txt` add `src/eval.cpp` to the `add_library(engine ...)` list and `tests/test_eval.cpp` to the `add_executable(engine_tests ...)` list:
```cmake
add_library(engine
    src/types.cpp
    src/board.cpp
    src/move.cpp
    src/movegen.cpp
    src/perft.cpp
    src/eval.cpp
)
```
```cmake
add_executable(engine_tests
    tests/test_main.cpp
    tests/test_types.cpp
    tests/test_board.cpp
    tests/test_move.cpp
    tests/test_movegen.cpp
    tests/test_perft.cpp
    tests/test_eval.cpp
)
```
Run:
```
cmake -S engine -B engine/build -G Ninja
cmake --build engine/build
```
Expected: compile error, `eval.hpp: No such file or directory`.

- [ ] **Step 3: Write `eval.hpp`**

`engine/src/eval.hpp`:
```cpp
#pragma once
#include "board.hpp"

int piece_value(PieceType t);
int material_score(const Board& b);  // centipawns, positive favors White
int evaluate(const Board& b);        // centipawns, positive favors the side to move
```

- [ ] **Step 4: Write `eval.cpp`**

`engine/src/eval.cpp`:
```cpp
#include "eval.hpp"

int piece_value(PieceType t) {
    switch (t) {
        case PieceType::Pawn:   return 100;
        case PieceType::Knight: return 320;
        case PieceType::Bishop: return 330;
        case PieceType::Rook:   return 500;
        case PieceType::Queen:  return 900;
        default:                return 0; // King and None
    }
}

int material_score(const Board& b) {
    int score = 0;
    for (Square s = 0; s < 64; s++) {
        Piece p = b.squares[s];
        if (p.type == PieceType::None) continue;
        int v = piece_value(p.type);
        score += (p.color == Color::White) ? v : -v;
    }
    return score;
}

int evaluate(const Board& b) {
    int score = material_score(b);
    return (b.side_to_move == Color::White) ? score : -score;
}
```

- [ ] **Step 5: Run, verify it PASSES**

Run: `cmake --build engine/build; ctest --test-dir engine/build --output-on-failure`
Expected: PASS.

- [ ] **Step 6: Commit**

```bash
git add engine/CMakeLists.txt engine/src/eval.hpp engine/src/eval.cpp engine/tests/test_eval.cpp
git commit -m "feat(engine): material evaluation from the side-to-move perspective"
```

---

## Task 2: Piece-square tables

**Files:**
- Modify: `engine/src/eval.cpp` (add the six tables, `pst_value`, `pst_score`; fold the PST term into `evaluate`)
- Modify: `engine/tests/test_eval.cpp` (add PST tests)

**Interfaces:**
- Consumes: `material_score`, `piece_value` (Task 1); `make_square/file_of/rank_of`.
- Produces: `evaluate` now returns material plus a positional term. `pst_value` and `pst_score` are file-local helpers in `eval.cpp` (not exported). The public signature of `evaluate` is unchanged.

**Theory:** Where a piece sits matters. A piece-square table gives each of the 64 squares a small bonus or penalty for a given piece type: knights and central pawns are rewarded near the middle, the king is pushed into the corner in the middlegame. The tables are written from White's perspective in the standard "board view" (rank 8 printed first, so array index 0 is a8). To score a white piece on square `s` (our indexing, a1 = 0), we flip to that orientation with `mirror(s)`; a black piece reads the table directly at `s`, which is the mirror-image square, so a symmetric position nets to 0. Because `material_score` is untouched, every Task 1 test still holds; the new tests cover the positional term.

- [ ] **Step 1: Write the failing test**

Append to `engine/tests/test_eval.cpp`:
```cpp
TEST_CASE("a knight is worth more in the center than in the corner") {
    // Same material (lone white knight + kings); only the knight's square differs.
    Board center = board_from_fen("4k3/8/8/8/3N4/8/8/4K3 w - - 0 1"); // Nd4
    Board corner = board_from_fen("4k3/8/8/8/8/8/8/N3K3 w - - 0 1");   // Na1
    CHECK(evaluate(center) > evaluate(corner));
}

TEST_CASE("piece-square tables keep the start position balanced") {
    CHECK(evaluate(start_position()) == 0);
}

TEST_CASE("piece-square tables move the score off pure material") {
    // An advanced, centralized white pawn should read higher than its raw 100.
    Board b = board_from_fen("4k3/8/8/3P4/8/8/8/4K3 w - - 0 1"); // white pawn d5
    CHECK(evaluate(b) != material_score(b));
}
```

- [ ] **Step 2: Run, verify it FAILS**

Run: `cmake --build engine/build; ctest --test-dir engine/build --output-on-failure`
Expected: FAIL. The center-vs-corner knight scores equal (material only), and the pawn test finds `evaluate == material_score`, so both new checks fail while Task 1's tests still pass.

- [ ] **Step 3: Add the tables, helpers, and PST term in `eval.cpp`**

Add the six tables and helpers inside a new anonymous namespace at the top of `engine/src/eval.cpp` (after the `#include`), then fold the PST term into `evaluate`. Insert this block right after `#include "eval.hpp"`:
```cpp
namespace {
// Michniewski "Simplified Evaluation Function" tables, White's perspective,
// printed rank 8 first, so index 0 = a8, index 63 = h1.
const int PAWN_PST[64] = {
     0,  0,  0,  0,  0,  0,  0,  0,
    50, 50, 50, 50, 50, 50, 50, 50,
    10, 10, 20, 30, 30, 20, 10, 10,
     5,  5, 10, 25, 25, 10,  5,  5,
     0,  0,  0, 20, 20,  0,  0,  0,
     5, -5,-10,  0,  0,-10, -5,  5,
     5, 10, 10,-20,-20, 10, 10,  5,
     0,  0,  0,  0,  0,  0,  0,  0
};
const int KNIGHT_PST[64] = {
    -50,-40,-30,-30,-30,-30,-40,-50,
    -40,-20,  0,  0,  0,  0,-20,-40,
    -30,  0, 10, 15, 15, 10,  0,-30,
    -30,  5, 15, 20, 20, 15,  5,-30,
    -30,  0, 15, 20, 20, 15,  0,-30,
    -30,  5, 10, 15, 15, 10,  5,-30,
    -40,-20,  0,  5,  5,  0,-20,-40,
    -50,-40,-30,-30,-30,-30,-40,-50
};
const int BISHOP_PST[64] = {
    -20,-10,-10,-10,-10,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5, 10, 10,  5,  0,-10,
    -10,  5,  5, 10, 10,  5,  5,-10,
    -10,  0, 10, 10, 10, 10,  0,-10,
    -10, 10, 10, 10, 10, 10, 10,-10,
    -10,  5,  0,  0,  0,  0,  5,-10,
    -20,-10,-10,-10,-10,-10,-10,-20
};
const int ROOK_PST[64] = {
     0,  0,  0,  0,  0,  0,  0,  0,
     5, 10, 10, 10, 10, 10, 10,  5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
     0,  0,  0,  5,  5,  0,  0,  0
};
const int QUEEN_PST[64] = {
    -20,-10,-10, -5, -5,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5,  5,  5,  5,  0,-10,
     -5,  0,  5,  5,  5,  5,  0, -5,
      0,  0,  5,  5,  5,  5,  0, -5,
    -10,  5,  5,  5,  5,  5,  0,-10,
    -10,  0,  5,  0,  0,  0,  0,-10,
    -20,-10,-10, -5, -5,-10,-10,-20
};
const int KING_PST[64] = {
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -20,-30,-30,-40,-40,-30,-30,-20,
    -10,-20,-20,-20,-20,-20,-20,-10,
     20, 20,  0,  0,  0,  0, 20, 20,
     20, 30, 10,  0,  0, 10, 30, 20
};

// Indexed by PieceType (Pawn=0 .. King=5).
const int* const PST[6] = {
    PAWN_PST, KNIGHT_PST, BISHOP_PST, ROOK_PST, QUEEN_PST, KING_PST
};

// Table value for one piece on one square (White's perspective magnitude).
int pst_value(Piece p, Square s) {
    const int* table = PST[static_cast<int>(p.type)];
    // White flips its square into the rank-8-first table orientation;
    // Black reads directly (its square is already the mirror image).
    Square idx = (p.color == Color::White)
        ? make_square(file_of(s), 7 - rank_of(s))
        : s;
    return table[idx];
}

int pst_score(const Board& b) {
    int score = 0;
    for (Square s = 0; s < 64; s++) {
        Piece p = b.squares[s];
        if (p.type == PieceType::None) continue;
        int v = pst_value(p, s);
        score += (p.color == Color::White) ? v : -v;
    }
    return score;
}
} // namespace
```
Then change `evaluate` to add the PST term:
```cpp
int evaluate(const Board& b) {
    int score = material_score(b) + pst_score(b);
    return (b.side_to_move == Color::White) ? score : -score;
}
```

- [ ] **Step 4: Run, verify it PASSES**

Run: `cmake --build engine/build; ctest --test-dir engine/build --output-on-failure`
Expected: PASS. The center knight now outscores the corner knight, the advanced pawn's score differs from raw material, the start position is still 0, and every Task 1 material and perspective test still passes.

- [ ] **Step 5: Commit**

```bash
git add engine/src/eval.cpp engine/tests/test_eval.cpp
git commit -m "feat(engine): piece-square tables in evaluation"
```

---

## Self-Review Notes

- **Spec coverage:** Implements spec §5 Milestone 4 ("Evaluation v1 (material, then piece-square tables)", success = "obvious material wins score correctly"). Task 1 delivers material with the exact-value sanity checks; Task 2 adds piece-square tables. Search (M5) and UCI (M6) remain out of scope; `evaluate` is deliberately side-to-move so M5's negamax can call it directly.
- **Type consistency:** `piece_value(PieceType) -> int`, `material_score(const Board&) -> int`, `evaluate(const Board&) -> int` are declared in `eval.hpp` and used identically in `eval.cpp` and the tests. `pst_value`/`pst_score` are file-local and never referenced outside `eval.cpp`. `PST` is indexed by `static_cast<int>(p.type)` with the enum order Pawn..King (0..5); the loop skips `PieceType::None` so index 6 is never used.
- **Placeholders:** none; all code is complete, including the full tables.
- **Orientation argument (the one subtle point):** tables are rank-8-first (index 0 = a8). A white piece on `s` reads `table[mirror(s)]`, converting our a1=0 square to the a8=0 table; a black piece reads `table[s]`, which is the vertically mirrored square, so a white piece on `s` and its black counterpart on `mirror(s)` contribute `+table[mirror(s)]` and `-table[mirror(s)]` and cancel. Hence the start position scores 0, which the tests assert. `// ponytail: hand-tuned Michniewski tables, replace with tuned values only if play quality needs it`.
- **Values are heuristic:** the exact centipawn constants are a well-known starting set, not sacred. Evaluation quality gets refined later (and search matters more than eval precision at this stage); the tests assert relationships (balance, center > corner, positional term is nonzero), not magic totals.

## What Milestone 5 will cover (preview, not part of this plan)

Search: minimax, then alpha-beta pruning, then iterative deepening, calling `make_move`/`unmake_move` (M3) to walk the tree and `evaluate` (this milestone) at the leaves. The side-to-move convention chosen here lets the search use negamax (one code path that negates the child score) instead of separate min and max cases. Sanity: stops hanging a queen, finds mate in one, then mate in two.
