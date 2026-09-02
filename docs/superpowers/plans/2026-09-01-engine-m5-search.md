# Engine Milestone 5: Search Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Give the engine the ability to choose a move: a `search(Board&, depth)` that walks the legal move tree with negamax, prunes it with alpha-beta, and drives it with iterative deepening, scoring leaves with `evaluate` (Milestone 4) and terminal nodes as checkmate or stalemate.

**Architecture:** A new `search` unit. Negamax is one recursive function that, at every node, negates the best child score (the side-to-move convention baked into `evaluate` in M4 is what makes this single code path correct). It uses M3's `make_move`/`unmake_move` to walk one board in place, and M2's `generate_legal`/`in_check` to enumerate moves and detect mate versus stalemate. Alpha-beta adds cutoffs that never change the value, only the node count; captures-first ordering makes the cutoffs bite. Iterative deepening loops shallow to deep, returning the deepest result and reusing the previous best move to order the root. Search is fixed-depth here; time control arrives with UCI in Milestone 6.

**Tech Stack:** C++17, CMake (+ Ninja), doctest (single-header, vendored at `engine/third_party/doctest.h`), MSYS2/MinGW-w64 UCRT64 toolchain on Windows.

**Spec:** `docs/superpowers/specs/2026-08-27-chess-coach-design.md` (§5 Milestone 5: "Search (minimax, then alpha-beta, then iterative deepening)", success = "stops hanging a queen, finds mate in one, then mate in two"). Builds on Milestones 1 to 4.

## Global Constraints

- **Language:** C++17 (`CMAKE_CXX_STANDARD 17`, already set).
- **Perspective (from M4):** `evaluate(const Board&)` returns centipawns from the **side to move's** perspective. Negamax relies on this: a child score is negated to become the parent's score.
- **Mate scoring:** `MATE = 30000`, `INF = 31000`. A node with no legal moves scores `-(MATE - ply)` if the side to move is in check (checkmate, and the `- ply` makes a faster mate score higher), else `0` (stalemate). `ply` is the distance in half-moves from the root (root children are at ply 1). Any test for "this is a mate" checks `abs(score) > 29000`.
- **Terminal-before-horizon rule:** each node generates legal moves and checks for "no moves" **before** the `depth == 0` cutoff, so a mate or stalemate exactly at the horizon is scored correctly instead of by material. (`// ponytail: generates legal moves at every leaf, fine at this depth; skip only if leaf-node cost ever dominates`.)
- **Quiescence gap (deliberate):** the leaf calls `evaluate` even mid-capture, so the score can be wrong when a capture is pending at the horizon (the horizon effect). Left out of v1 on purpose. (`// ponytail: no quiescence search; add a captures-only extension at leaves if tactical eval is too noisy`.)
- **Board is mutated during search:** `search` and its helpers take `Board&` (non-const) and must leave the board exactly as they found it (every `make_move` paired with an `unmake_move`).
- **Test framework:** doctest, vendored at `engine/third_party/doctest.h`.
- **Every code file stays split** into `.hpp` (declarations) + `.cpp` (definitions).
- **No dashes (— –)** in any comment, message, or doc; use commas, colons, or parentheses.
- **Square indexing (unchanged):** `make_square(file, rank) = rank*8 + file`, file 0..7 = a..h, rank 0..7 = ranks 1..8, `NO_SQUARE = -1`.
- **Reused symbols:** `Move`, `MoveFlag`, `Undo`, `make_move`, `unmake_move` (`move.hpp`); `generate_legal(Board&) -> std::vector<Move>`, `in_check(const Board&, Color)` (`movegen.hpp`); `evaluate(const Board&)` (`eval.hpp`); `Board`, `board_from_fen`, `start_position` (`board.hpp`); `Color`, `PieceType`, `Square`, `NO_SQUARE` (`types.hpp`).
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
  CMakeLists.txt     # MODIFY (Task 1): add src/search.cpp and tests/test_search.cpp
  src/
    search.hpp       # NEW: SearchResult + search (Task 1); reference/oracle + node count (Task 2); search_to_depth (Task 3)
    search.cpp       # NEW: negamax (Task 1); alpha-beta + ordering (Task 2); iterative deepening (Task 3)
  tests/
    test_search.cpp  # NEW: tactic/mate/stalemate (Task 1); pruning parity + node drop (Task 2); ID parity + mate in two (Task 3)
```

The `search` unit depends on `eval`, `movegen`, and `move`, all of which already exist. It is the top of the engine's decision stack: nothing else depends on it yet (UCI in M6 will).

---

## Task 1: Negamax at fixed depth

**Files:**
- Create: `engine/src/search.hpp`
- Create: `engine/src/search.cpp`
- Create: `engine/tests/test_search.cpp`
- Modify: `engine/CMakeLists.txt` (add `src/search.cpp`, `tests/test_search.cpp`)

**Interfaces:**
- Consumes: `Board`, `Move`, `Undo`, `make_move`, `unmake_move`, `generate_legal`, `in_check`, `evaluate`, `board_from_fen`, `make_square`.
- Produces:
  - `struct SearchResult { Move best; int score; };`
  - `SearchResult search(Board& b, int depth);` (fixed-depth negamax; `best.from == NO_SQUARE` when there is no legal move)

**Theory:** A search picks a move by looking ahead: try each legal move, evaluate the position it leads to, and keep the best. "Best" for a position must be measured from the mover's own seat, which is exactly what `evaluate` gives us. Negamax uses that to collapse the usual max-node / min-node split into one function: the value of a node is `max over moves of ( -value(child) )`. The negation flips the child's side-to-move score into the parent's terms. At the bottom (`depth == 0`) we just call `evaluate`. At a dead end (no legal moves) it is checkmate if the mover is in check (a large negative, worse the sooner it happens) or stalemate (zero). The root does the same loop but also remembers *which* move produced the best score, because that is the move we will actually play.

- [ ] **Step 1: Write the failing test**

`engine/tests/test_search.cpp`:
```cpp
#include "doctest.h"
#include "board.hpp"
#include "move.hpp"
#include "search.hpp"

TEST_CASE("search grabs a free queen") {
    // White rook on d1 can capture an undefended black queen on d3.
    Board b = board_from_fen("4k3/8/8/8/8/3q4/8/3RK3 w - - 0 1");
    SearchResult r = search(b, 2);
    CHECK(r.best.from == make_square(3, 0)); // d1
    CHECK(r.best.to   == make_square(3, 2)); // d3
    CHECK(r.score > 400);                    // clearly ahead (a rook up after being down)
}

TEST_CASE("search finds mate in one") {
    // White: Ra... rook on h1, rook on g7 holds rank 7; Rh1-h8 is mate.
    Board b = board_from_fen("k7/6R1/8/8/8/8/8/K6R w - - 0 1");
    SearchResult r = search(b, 2);
    CHECK(r.best.from == make_square(7, 0)); // h1
    CHECK(r.best.to   == make_square(7, 7)); // h8
    CHECK(r.score > 29000);                  // a mate score
}

TEST_CASE("stalemate scores zero and reports no move") {
    // Black to move, king h8 has no legal move and is not in check.
    Board b = board_from_fen("7k/5Q2/6K1/8/8/8/8/8 b - - 0 1");
    SearchResult r = search(b, 3);
    CHECK(r.score == 0);
    CHECK(r.best.from == NO_SQUARE);
}
```

- [ ] **Step 2: Register the files, run, verify it FAILS to compile**

In `engine/CMakeLists.txt` add `src/search.cpp` to the `add_library(engine ...)` list and `tests/test_search.cpp` to the `add_executable(engine_tests ...)` list:
```cmake
add_library(engine
    src/types.cpp
    src/board.cpp
    src/move.cpp
    src/movegen.cpp
    src/perft.cpp
    src/eval.cpp
    src/search.cpp
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
    tests/test_search.cpp
)
```
Run:
```
cmake -S engine -B engine/build -G Ninja
cmake --build engine/build
```
Expected: compile error, `search.hpp: No such file or directory`.

- [ ] **Step 3: Write `search.hpp`**

`engine/src/search.hpp`:
```cpp
#pragma once
#include "board.hpp"
#include "move.hpp"

struct SearchResult {
    Move best;   // best move found (best.from == NO_SQUARE if no legal move exists)
    int score;   // centipawns, from the side-to-move perspective
};

SearchResult search(Board& b, int depth);  // fixed-depth negamax
```

- [ ] **Step 4: Write `search.cpp`**

`engine/src/search.cpp`:
```cpp
#include "search.hpp"
#include "movegen.hpp"
#include "eval.hpp"
#include <vector>

namespace {
const int MATE = 30000;
const int INF  = 31000;

// Value of the node for the side to move. `ply` is the distance from the root.
int negamax(Board& b, int depth, int ply) {
    std::vector<Move> moves = generate_legal(b);
    if (moves.empty())
        return in_check(b, b.side_to_move) ? -(MATE - ply) : 0;
    if (depth == 0)
        return evaluate(b);

    int best = -INF;
    for (const Move& m : moves) {
        Undo u = make_move(b, m);
        int score = -negamax(b, depth - 1, ply + 1);
        unmake_move(b, m, u);
        if (score > best) best = score;
    }
    return best;
}
} // namespace

SearchResult search(Board& b, int depth) {
    SearchResult result;
    result.best = Move{};   // from == NO_SQUARE means "no move"
    result.score = 0;

    std::vector<Move> moves = generate_legal(b);
    if (moves.empty()) {
        result.score = in_check(b, b.side_to_move) ? -MATE : 0;
        return result;
    }

    int best = -INF;
    for (const Move& m : moves) {
        Undo u = make_move(b, m);
        int score = -negamax(b, depth - 1, 1);
        unmake_move(b, m, u);
        if (score > best) {
            best = score;
            result.best = m;
        }
    }
    result.score = best;
    return result;
}
```

- [ ] **Step 5: Run, verify it PASSES**

Run: `cmake --build engine/build; ctest --test-dir engine/build --output-on-failure`
Expected: PASS. The free queen is taken (best move is Rd1xd3, score above 500), mate in one is found (Rh1-h8, score above 29000), and the stalemate scores 0 with no move.

- [ ] **Step 6: Commit**

```bash
git add engine/CMakeLists.txt engine/src/search.hpp engine/src/search.cpp engine/tests/test_search.cpp
git commit -m "feat(engine): fixed-depth negamax search"
```

---

## Task 2: Alpha-beta pruning + captures-first ordering

**Files:**
- Modify: `engine/src/search.hpp` (add the reference oracle and node counter used by tests)
- Modify: `engine/src/search.cpp` (add alpha/beta to negamax, add move ordering, add the reference oracle and counter)
- Modify: `engine/tests/test_search.cpp` (add the pruning parity + node-drop test)

**Interfaces:**
- Consumes: everything from Task 1, plus `MoveFlag`, `PieceType`, `std::stable_partition`.
- Produces:
  - `int search_minimax(Board& b, int depth);` (un-pruned full-width value; a test oracle, same value as `search(...).score`)
  - `long nodes_searched();` (nodes visited by the most recent `search` or `search_minimax` call)

**Theory:** Alpha-beta is a pure optimization: it returns the exact same value as plain negamax but skips branches that cannot change the answer. `alpha` is the best score the side to move is already assured of; `beta` is the best the opponent will allow. Once a move proves this node is at least as good as `beta`, the opponent would never enter it, so we stop searching the rest ("beta cutoff"). Cutoffs only help if good moves come first, so we try captures before quiet moves, a cheap heuristic that finds strong moves early. Because the *value* is unchanged, the way we prove correctness is a differential test: run a full-width reference search and the alpha-beta search on the same position and depth, assert the scores are identical, and assert alpha-beta visited strictly fewer nodes. That is what "same answer, faster" means, made testable.

- [ ] **Step 1: Write the failing test**

Append to `engine/tests/test_search.cpp`:
```cpp
TEST_CASE("alpha-beta returns the same value as full-width minimax, with fewer nodes") {
    // A busy midgame position (after 1.e4 e5) so pruning has something to cut.
    Board b = board_from_fen("rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2");

    int full = search_minimax(b, 3);
    long full_nodes = nodes_searched();

    int pruned = search(b, 3).score;
    long pruned_nodes = nodes_searched();

    CHECK(pruned == full);              // pruning must not change the value
    CHECK(pruned_nodes < full_nodes);   // but it must visit fewer nodes
}
```

- [ ] **Step 2: Run, verify it FAILS to compile**

Run: `cmake --build engine/build`
Expected: compile error, `search_minimax` and `nodes_searched` are not declared.

- [ ] **Step 3: Add the oracle + counter to `search.hpp`**

Append to `engine/src/search.hpp`:
```cpp
// Exposed for tests. search_minimax is a full-width (un-pruned) reference
// search: it returns the same value as search(...).score but visits every node,
// so a test can prove alpha-beta prunes without changing the answer.
int  search_minimax(Board& b, int depth);
long nodes_searched();  // nodes visited by the most recent search / search_minimax call
```

- [ ] **Step 4: Rewrite `search.cpp` with alpha-beta, ordering, oracle, and counter**

Replace the whole file with:
```cpp
#include "search.hpp"
#include "movegen.hpp"
#include "eval.hpp"
#include <vector>
#include <algorithm>

namespace {
const int MATE = 30000;
const int INF  = 31000;

long g_nodes = 0;  // reset at the top of each public search entry point

// Captures first: cheap move ordering so alpha-beta cutoffs land early.
bool is_capture(const Board& b, const Move& m) {
    return b.squares[m.to].type != PieceType::None
        || m.flag == MoveFlag::EnPassant;
}

void order_moves(const Board& b, std::vector<Move>& moves) {
    std::stable_partition(moves.begin(), moves.end(),
                          [&](const Move& m) { return is_capture(b, m); });
}

// Alpha-beta negamax. Same value as plain negamax, fewer nodes.
int negamax(Board& b, int depth, int ply, int alpha, int beta) {
    g_nodes++;
    std::vector<Move> moves = generate_legal(b);
    if (moves.empty())
        return in_check(b, b.side_to_move) ? -(MATE - ply) : 0;
    if (depth == 0)
        return evaluate(b);

    order_moves(b, moves);
    int best = -INF;
    for (const Move& m : moves) {
        Undo u = make_move(b, m);
        int score = -negamax(b, depth - 1, ply + 1, -beta, -alpha);
        unmake_move(b, m, u);
        if (score > best) best = score;
        if (best > alpha) alpha = best;
        if (alpha >= beta) break;   // beta cutoff: opponent would never allow this node
    }
    return best;
}

// Full-width reference: no cutoffs, no ordering. Test oracle only.
int negamax_full(Board& b, int depth, int ply) {
    g_nodes++;
    std::vector<Move> moves = generate_legal(b);
    if (moves.empty())
        return in_check(b, b.side_to_move) ? -(MATE - ply) : 0;
    if (depth == 0)
        return evaluate(b);

    int best = -INF;
    for (const Move& m : moves) {
        Undo u = make_move(b, m);
        int score = -negamax_full(b, depth - 1, ply + 1);
        unmake_move(b, m, u);
        if (score > best) best = score;
    }
    return best;
}
} // namespace

long nodes_searched() { return g_nodes; }

int search_minimax(Board& b, int depth) {
    g_nodes = 0;
    return negamax_full(b, depth, 0);
}

SearchResult search(Board& b, int depth) {
    g_nodes = 0;
    SearchResult result;
    result.best = Move{};
    result.score = 0;

    std::vector<Move> moves = generate_legal(b);
    if (moves.empty()) {
        result.score = in_check(b, b.side_to_move) ? -MATE : 0;
        return result;
    }

    order_moves(b, moves);
    int best = -INF;
    int alpha = -INF;
    for (const Move& m : moves) {
        Undo u = make_move(b, m);
        int score = -negamax(b, depth - 1, 1, -INF, -alpha);
        unmake_move(b, m, u);
        if (score > best) {
            best = score;
            result.best = m;
        }
        if (best > alpha) alpha = best;
    }
    result.score = best;
    return result;
}
```

- [ ] **Step 5: Run, verify it PASSES**

Run: `cmake --build engine/build; ctest --test-dir engine/build --output-on-failure`
Expected: PASS. The new differential test shows equal scores and fewer nodes, and every Task 1 test still passes unchanged (proof that pruning did not alter the tactical or mate results).

- [ ] **Step 6: Commit**

```bash
git add engine/src/search.hpp engine/src/search.cpp engine/tests/test_search.cpp
git commit -m "feat(engine): alpha-beta pruning with captures-first ordering"
```

---

## Task 3: Iterative deepening

**Files:**
- Modify: `engine/src/search.hpp` (expose `search_to_depth`; `search` becomes the iterative-deepening driver)
- Modify: `engine/src/search.cpp` (extract the fixed-depth root into `search_to_depth`, add the deepening loop)
- Modify: `engine/tests/test_search.cpp` (add the parity + mate-in-two test)

**Interfaces:**
- Consumes: everything from Task 2.
- Produces:
  - `SearchResult search_to_depth(Board& b, int depth, Move first = Move{});` (one fixed-depth alpha-beta search; if `first` is a real move it is tried first at the root)
  - `SearchResult search(Board& b, int max_depth);` (unchanged signature; now loops depth 1..max_depth and returns the deepest result)

**Theory:** Iterative deepening runs the search at depth 1, then 2, and so on up to the target, returning the deepest completed result. It sounds wasteful, redoing shallow work, but the shallow searches are cheap (the cost is dominated by the deepest layer) and they pay for themselves: the best move from depth `d-1` is almost always strong at depth `d`, so trying it first at the root sharpens alpha-beta's ordering. The real reason it exists, though, is time control (Milestone 6): because every depth returns a usable move, the engine can stop whenever the clock runs low and still have the best move it has found so far. The final value is identical to a single fixed-depth search, which is exactly the parity we test.

- [ ] **Step 1: Write the failing test**

Append to `engine/tests/test_search.cpp`:
```cpp
TEST_CASE("iterative deepening matches a single fixed-depth search") {
    Board b = board_from_fen("rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2");
    SearchResult id    = search(b, 3);
    SearchResult fixed = search_to_depth(b, 3);
    CHECK(id.score    == fixed.score);
    CHECK(id.best.from == fixed.best.from);
    CHECK(id.best.to   == fixed.best.to);
}

TEST_CASE("search finds mate in two") {
    // 1.Re8+ Rxe8 2.Rxe8# (the f7/g7/h7 pawns seal the back rank).
    Board b = board_from_fen("r5k1/5ppp/8/8/8/8/5PPP/3RR1K1 w - - 0 1");
    CHECK(search(b, 2).score < 29000);   // too shallow to see the mate
    CHECK(search(b, 4).score > 29000);   // deep enough: forced mate found
}
```

- [ ] **Step 2: Run, verify it FAILS to compile**

Run: `cmake --build engine/build`
Expected: compile error, `search_to_depth` is not declared.

- [ ] **Step 3: Expose `search_to_depth` in `search.hpp`**

Append to `engine/src/search.hpp`:
```cpp
// One fixed-depth alpha-beta search. If `first` is a real move (from != NO_SQUARE)
// it is searched first at the root; the iterative-deepening driver passes the
// previous iteration's best move here to improve ordering.
SearchResult search_to_depth(Board& b, int depth, Move first = Move{});
```

- [ ] **Step 4: Extract the root into `search_to_depth`, make `search` the deepening loop**

In `engine/src/search.cpp`, replace the existing `SearchResult search(Board& b, int depth) { ... }` definition with these two functions:
```cpp
SearchResult search_to_depth(Board& b, int depth, Move first) {
    g_nodes = 0;
    SearchResult result;
    result.best = Move{};
    result.score = 0;

    std::vector<Move> moves = generate_legal(b);
    if (moves.empty()) {
        result.score = in_check(b, b.side_to_move) ? -MATE : 0;
        return result;
    }

    order_moves(b, moves);
    // Try the hint move first (from the previous, shallower iteration).
    if (first.from != NO_SQUARE) {
        auto it = std::find_if(moves.begin(), moves.end(), [&](const Move& m) {
            return m.from == first.from && m.to == first.to && m.promotion == first.promotion;
        });
        if (it != moves.end()) std::rotate(moves.begin(), it, it + 1);
    }

    int best = -INF;
    int alpha = -INF;
    for (const Move& m : moves) {
        Undo u = make_move(b, m);
        int score = -negamax(b, depth - 1, 1, -INF, -alpha);
        unmake_move(b, m, u);
        if (score > best) {
            best = score;
            result.best = m;
        }
        if (best > alpha) alpha = best;
    }
    result.score = best;
    return result;
}

SearchResult search(Board& b, int max_depth) {
    SearchResult result;
    result.best = Move{};
    result.score = 0;
    for (int d = 1; d <= max_depth; d++) {
        result = search_to_depth(b, d, result.best);  // reuse previous best for ordering
    }
    return result;
}
```
Note: `search_to_depth` uses `std::find_if`, `std::rotate` (already covered by `<algorithm>`) and `NO_SQUARE` (from `types.hpp`, included transitively via `board.hpp`).

- [ ] **Step 5: Run, verify it PASSES**

Run: `cmake --build engine/build; ctest --test-dir engine/build --output-on-failure`
Expected: PASS. Iterative deepening returns the same score and best move as a single depth-3 search, the mate in two is invisible at depth 2 but found at depth 4, and every earlier test still passes.

- [ ] **Step 6: Commit**

```bash
git add engine/src/search.hpp engine/src/search.cpp engine/tests/test_search.cpp
git commit -m "feat(engine): iterative deepening driver"
```

---

## Self-Review Notes

- **Spec coverage:** Implements spec §5 Milestone 5 ("Search: minimax, then alpha-beta, then iterative deepening", success = "stops hanging a queen, finds mate in one, then mate in two"). Task 1 = negamax with the free-queen (hangs nothing), mate-in-one, and stalemate tests. Task 2 = alpha-beta with a differential parity + node-drop test. Task 3 = iterative deepening with a parity test and the mate-in-two test. UCI (M6) is out of scope; `search` returning a `SearchResult` and being fixed-depth is the seam M6 will wrap with time control.
- **Type consistency:** `search(Board&, int) -> SearchResult`, `search_minimax(Board&, int) -> int`, `nodes_searched() -> long`, `search_to_depth(Board&, int, Move) -> SearchResult` are declared in `search.hpp` and used identically in `search.cpp` and the tests. `SearchResult` fields are `best` (`Move`) and `score` (`int`) throughout. The `negamax` signature grows from `(Board&, int, int)` in Task 1 to `(Board&, int, int, int, int)` in Task 2 (alpha, beta added); the plan rewrites the whole file in Task 2 Step 4 so no stale two-argument call survives.
- **Placeholders:** none; every task carries complete, compilable code including the full function bodies.
- **Mate-distance correctness:** terminal score `-(MATE - ply)` makes a shallower mate score higher (mate at ply 1 = 29999 beats mate at ply 3 = 29997), so the engine prefers the fastest mate and the mate-in-two test's depth-2-vs-depth-4 contrast holds. The "generate moves before the depth cutoff" rule is what lets a mate at the horizon be scored as a mate rather than by material.
- **Alpha-beta correctness proof:** kept honest by a full-width reference (`negamax_full` / `search_minimax`) retained solely as a test oracle. This is a deliberate, small piece of extra code justified by the milestone's core claim ("same value, fewer nodes"); it is clearly commented as test-only. (`// ponytail: reference search kept only as a correctness oracle for pruning; delete if the differential test is ever dropped`.)
- **Board integrity:** every `make_move` is paired with `unmake_move` in all three recursive/root loops, so the single shared board is restored after each search. This mirrors the M3 make/unmake discipline that perft already proved.

## What Milestone 6 will cover (preview, not part of this plan)

UCI protocol: parse `position`/`go`, run `search` under a time or depth limit, print `bestmove`. Iterative deepening is what makes a time limit usable (stop between depths, return the best move so far). At that point the coach track can drive our own engine over UCI instead of Stockfish.
