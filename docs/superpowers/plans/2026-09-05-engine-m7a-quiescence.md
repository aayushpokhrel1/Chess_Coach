# Engine M7a: Quiescence + MVV-LVA Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Stop the engine banking material it is about to lose to a recapture (the horizon effect that lost both Stockfish matches) by searching captures out to a quiet position at the leaf, and order captures MVV-LVA so alpha-beta prunes deeper.

**Architecture:** One new leaf routine `quiesce()` in `search.cpp` replaces the static `evaluate(b)` call at depth 0. It stands pat on the static eval, then searches only captures (or, when in check, all legal evasions, option B) until quiet. Both search entry points (`negamax` and the full-width oracle `negamax_full`) bottom out in `quiesce`, so the existing "alpha-beta == full-width" differential test keeps proving pruning correctness with the new shared leaf. `order_moves` gains MVV-LVA within its captures-first partition, reusing eval's `piece_value`.

**Tech Stack:** C++ (MSYS2 UCRT64, gcc), CMake + Ninja, doctest. Build/test via PowerShell (git-bash does not see the toolchain).

**Spec:** inline (design approved in chat 2026-09-05; single-file change, no separate spec doc).

## Global Constraints

- Board rep is the 64-square array; no bitboards in M7a.
- Eval is side-to-move perspective; `quiesce` returns the same perspective as `evaluate`/`negamax`.
- Reuse `piece_value(PieceType)` from `eval.hpp:4`; do not add a second values table.
- No dashes in prose, comments, or commit messages.
- Mark any deliberate corner with a `ponytail:` comment naming the ceiling and upgrade path.
- Build loop (PowerShell, PATH refreshed first):
  `$env:Path = [Environment]::GetEnvironmentVariable("Path","User") + ";" + [Environment]::GetEnvironmentVariable("Path","Machine")`
  then `cmake --build engine/build; ctest --test-dir engine/build --output-on-failure`

---

### Task 1: MVV-LVA capture ordering

**Files:**
- Modify: `engine/src/search.cpp:35-38` (`order_moves`)
- Test: `engine/tests/test_search.cpp` (existing differential test is the guard)

**Interfaces:**
- Consumes: `piece_value(PieceType)` from `eval.hpp`; `is_capture(const Board&, const Move&)` (already in search.cpp anon namespace).
- Produces: `order_moves` still puts captures first, now sorted by victim value minus attacker value, descending. Value unchanged for any search (pure reordering); relied on by Task 2's `quiesce`.

- [ ] **Step 1: Add `#include "eval.hpp"` if not already present in search.cpp**

search.cpp already includes `eval.hpp` (line 3). No change needed; confirm.

- [ ] **Step 2: Rewrite `order_moves` to sort captures MVV-LVA**

```cpp
// Value of the piece captured by move m (en passant always takes a pawn).
int victim_value(const Board& b, const Move& m) {
    if (m.flag == MoveFlag::EnPassant) return piece_value(PieceType::Pawn);
    return piece_value(b.squares[m.to].type);
}

// Captures first, and within captures Most-Valuable-Victim / Least-Valuable-Attacker
// so alpha-beta tries queen-takes-queen before pawn-takes-pawn and cuts off sooner.
void order_moves(const Board& b, std::vector<Move>& moves) {
    std::stable_sort(moves.begin(), moves.end(), [&](const Move& a, const Move& c) {
        bool ca = is_capture(b, a), cc = is_capture(b, c);
        if (ca != cc) return ca;                       // captures before quiets
        if (!ca) return false;                         // keep quiet moves' order (stable)
        int sa = victim_value(b, a) - piece_value(b.squares[a.from].type);
        int sc = victim_value(b, c) - piece_value(b.squares[c.from].type);
        return sa > sc;                                // higher MVV-LVA first
    });
}
```

- [ ] **Step 3: Build and run the full search suite**

Run: `cmake --build engine/build; ctest --test-dir engine/build --output-on-failure -R search`
Expected: PASS. The differential test ("alpha-beta returns the same value as full-width minimax") proves reordering did not change any value; `pruned_nodes < full_nodes` still holds.

- [ ] **Step 4: Commit**

```bash
git add engine/src/search.cpp
git commit -m "feat(engine): order captures MVV-LVA in the search"
```

---

### Task 2: Quiescence search with check handling (option B)

**Files:**
- Modify: `engine/src/search.cpp` (add `quiesce`; change the two `depth == 0` leaves in `negamax` and `negamax_full`)
- Test: `engine/tests/test_search.cpp` (add poisoned-capture case)

**Interfaces:**
- Consumes: `evaluate`, `generate_legal`, `in_check`, `is_capture`, `make_move`/`unmake_move`, `order_moves` (Task 1), the file-scope `MATE`/`INF`, `g_nodes`, `maybe_timeout`, `g_stop`.
- Produces: `int quiesce(Board& b, int ply, int alpha, int beta, int qdepth = 0)` in the anon namespace, called at the horizon by both `negamax` and `negamax_full`. Deterministic quiet-position value in side-to-move centipawns; used only inside search.cpp.

- [ ] **Step 1: Write the failing test (poisoned capture)**

Add to `engine/tests/test_search.cpp`:

```cpp
TEST_CASE("quiescence declines a poisoned capture") {
    // White pawn on e4, Black pawn on d5 defended by the c6 pawn and the queen on d8.
    // Grabbing exd5 wins a pawn on the surface but loses the pawn straight back to cxd5,
    // so a search that stops mid-capture (no quiescence) overvalues exd5. With quiescence
    // White should NOT think it is up material after exd5.
    Board b = board_from_fen("3qkbnr/pp2pppp/2p5/3p4/4P3/8/PPPP1PPP/RNBQKBNR w KQk - 0 1");
    SearchResult r = search(b, 1);
    // The recapture is visible, so the best line is not a clear material win.
    CHECK(r.score < 90);   // less than ~one pawn: the "win" is seen to be temporary
}
```

- [ ] **Step 2: Run it to confirm it fails**

Run: `cmake --build engine/build; ctest --test-dir engine/build --output-on-failure -R "poisoned"`
Expected: FAIL (without quiescence, depth-1 search banks the pawn and scores near +100).

Note: if the FEN does not produce the intended surface-win/recapture, adjust the FEN until Step 2 fails for the right reason (a depth-1 search preferring the capture), then proceed. The behavior under test, not the exact FEN, is the deliverable.

- [ ] **Step 3: Add `quiesce` above `negamax` in the anon namespace**

```cpp
// Leaf of the main search. Instead of trusting a static eval in the middle of a
// capture fight, keep resolving captures until the position is quiet, so the score
// reflects the material that actually stays on the board.
//
// ponytail: qdepth cap (QMAX) truncates runaway check sequences (e.g. perpetual
// check) by falling back to the static eval; raise QMAX or add repetition detection
// if a real analysis position is ever cut short.
const int QMAX = 40;

int quiesce(Board& b, int ply, int alpha, int beta, int qdepth = 0) {
    maybe_timeout();
    if (g_stop) return 0;   // aborted: value discarded upstream
    g_nodes++;

    bool check = in_check(b, b.side_to_move);
    if (qdepth >= QMAX) return evaluate(b);   // depth guard, see ponytail note

    std::vector<Move> moves;
    int best;

    if (check) {
        // In check there is no "do nothing" option: search every legal escape,
        // not just captures, or we could miss the only move that survives.
        moves = generate_legal(b);
        if (moves.empty()) return -(MATE - ply);   // checkmate at the leaf
        best = -INF;
    } else {
        // Stand pat: you are never forced to capture, so the static eval is a floor.
        int stand = evaluate(b);
        if (stand >= beta) return stand;
        best = stand;
        if (stand > alpha) alpha = stand;
        moves = generate_legal(b);
        std::vector<Move> caps;
        for (const Move& m : moves)
            if (is_capture(b, m)) caps.push_back(m);
        moves.swap(caps);
    }

    order_moves(b, moves);   // MVV-LVA (Task 1)
    for (const Move& m : moves) {
        Undo u = make_move(b, m);
        int score = -quiesce(b, ply + 1, -beta, -alpha, qdepth + 1);
        unmake_move(b, m, u);
        if (g_stop) return best;
        if (score > best) best = score;
        if (best > alpha) alpha = best;
        if (alpha >= beta) break;   // beta cutoff
    }
    return best;
}
```

- [ ] **Step 4: Point both search leaves at `quiesce`**

In `negamax` (currently `if (depth == 0) return evaluate(b);`):

```cpp
    if (depth == 0)
        return quiesce(b, ply, alpha, beta);
```

In `negamax_full` (the full-width oracle, currently `if (depth == 0) return evaluate(b);`) use the full window so it returns the true value:

```cpp
    if (depth == 0)
        return quiesce(b, ply, -INF, INF);
```

Rationale: quiesce is now the shared leaf evaluator. Both searches must use it or the differential test compares two different leaf functions. Full window in the oracle keeps its value exact.

- [ ] **Step 5: Build and run the full suite**

Run: `cmake --build engine/build; ctest --test-dir engine/build --output-on-failure`
Expected: ALL PASS. Specifically:
- new "poisoned capture" test PASSES;
- "alpha-beta returns the same value as full-width minimax" still PASSES (both leaves now quiesce, so values still match; `pruned_nodes < full_nodes` still holds);
- "iterative deepening matches a single fixed-depth search" still PASSES;
- mate-in-one / mate-in-two / free-queen / stalemate / timed tests still PASS (quiesce only searches captures off the horizon; mates and stalemate are found at real depth and are unaffected).

If the differential or ID tests fail, the leaf wiring in Step 4 is the suspect: confirm both `negamax` and `negamax_full` call `quiesce` and the oracle uses the full `-INF, INF` window.

- [ ] **Step 6: Commit**

```bash
git add engine/src/search.cpp engine/tests/test_search.cpp
git commit -m "feat(engine): quiescence search with check evasions (M7a)"
```

---

## Self-Review

- **Spec coverage:** quiescence (Task 2) fixes the horizon bug; check handling = option B (Task 2 in-check branch); MVV-LVA (Task 1); differential-test integrity preserved (Task 2 Step 4/5); ponytail ceiling on perpetual check marked (Task 2 Step 3). All covered.
- **Placeholder scan:** none; the one adjustable item (poisoned-capture FEN) has an explicit "tune until it fails for the right reason" instruction, not a TODO.
- **Type consistency:** `quiesce` signature stable across Steps 3/4; `order_moves`/`victim_value`/`is_capture`/`piece_value` names match search.cpp and eval.hpp.
```
