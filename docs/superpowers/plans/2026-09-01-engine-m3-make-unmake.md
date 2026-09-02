# Engine Milestone 3: Make / Unmake Move Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace copy-make with in-place make/unmake so the engine mutates one board and reverses the change, and prove the swap faithful by keeping every perft count identical.

**Architecture:** `make_move` becomes in-place (`Undo make_move(Board&, const Move&)`), returning a tiny `Undo` record holding only what cannot be reconstructed from the move: the captured piece and the pre-move castling rights, en-passant square, halfmove clock, and fullmove number. `unmake_move` reverses the move using that record. Task 1 introduces the mechanism and migrates every call site (still copying at the call sites) so the tree stays green; Task 2 removes the copies by threading one mutable board through `generate_legal` and `perft`, with perft's published numbers as the proof.

**Tech Stack:** C++17, CMake (+ Ninja), doctest (single-header, vendored at `engine/third_party/doctest.h`), MSYS2/MinGW-w64 UCRT64 toolchain on Windows.

**Spec:** `docs/superpowers/specs/2026-08-27-chess-coach-design.md` (§5 Milestone 3: "Make / unmake move", success = "perft still matches after make+unmake"). Builds directly on the M2 plan `docs/superpowers/plans/2026-08-29-engine-m2-move-generation.md`.

## Global Constraints

- **Language:** C++17 (`CMAKE_CXX_STANDARD 17`, already set).
- **Reversibility is the invariant:** `make_move` then `unmake_move` must restore the board bit-for-bit. Tested by FEN round-trip: `fen == fen_from_board(after make then unmake)`.
- **Perft is the safety net:** the counts from M2 must be **unchanged** by this refactor. Start: d1=20, d2=400, d3=8902, d4=197281. Kiwipete (`r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1`): d1=48, d2=2039, d3=97862. Position 3 (`8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1`): d1=14, d2=191, d3=2812, d4=43238. Never advance with perft failing (spec §10).
- **Test framework:** doctest, vendored at `engine/third_party/doctest.h`.
- **Every code file stays split** into `.hpp` (declarations) + `.cpp` (definitions).
- **No dashes (— –)** in any comment, message, or doc; use commas, colons, or parentheses.
- **Square indexing (unchanged):** `make_square(file, rank) = rank*8 + file`, file 0..7 = a..h, rank 0..7 = ranks 1..8, `NO_SQUARE = -1`.
- **Existing symbols reused verbatim:** `Color`, `PieceType`, `Piece`, `Square`, `make_square/file_of/rank_of` (`types.hpp`); `Board`, `CASTLE_WK/WQ/BK/BQ`, `board_from_fen`, `fen_from_board`, `start_position` (`board.hpp`); `Move`, `MoveFlag`, `to_uci` (`move.hpp`); `is_square_attacked`, `in_check`, `generate_pseudo_legal` (`movegen.hpp`).
- **Build/test loop (PowerShell; git-bash cannot see the MSYS2 toolchain):**
  ```
  $env:Path = [Environment]::GetEnvironmentVariable("Path","User") + ";" + [Environment]::GetEnvironmentVariable("Path","Machine")
  cmake --build engine/build; ctest --test-dir engine/build --output-on-failure
  ```
  No new files are added in this milestone, so re-running `cmake -S engine -B engine/build -G Ninja` is not required.

---

## File Structure

No new files. This milestone changes the move mechanism in place:

```
engine/
  src/
    move.hpp     # MODIFY: add struct Undo; change make_move signature; declare unmake_move
    move.cpp     # MODIFY: make_move becomes in-place returning Undo; add unmake_move
    movegen.hpp  # MODIFY (Task 2): generate_legal takes Board& (non-const)
    movegen.cpp  # MODIFY: generate_legal migrates to new make_move (T1), then make/unmake in place (T2)
    perft.hpp    # MODIFY (Task 2): perft / perft_divide take Board& (non-const)
    perft.cpp    # MODIFY: migrate to new make_move (T1), then thread one board with make/unmake (T2)
  tests/
    test_move.cpp     # MODIFY (Task 1): in-place `after` helper; add make/unmake round-trip tests
    test_movegen.cpp  # MODIFY (Task 1): one make_move call site updated to in-place
```

---

## Task 1: Undo record + in-place make_move + unmake_move

**Files:**
- Modify: `engine/src/move.hpp`
- Modify: `engine/src/move.cpp`
- Modify: `engine/tests/test_move.cpp`
- Modify: `engine/src/movegen.cpp` (migrate `generate_legal` call site to the new signature, still copying)
- Modify: `engine/src/perft.cpp` (migrate `perft` / `perft_divide` call sites, still copying)
- Modify: `engine/tests/test_movegen.cpp` (one `make_move` call site in the "in check" test)

**Interfaces:**
- Consumes: `Board`, `Piece`, `Move`, `MoveFlag`, `CASTLE_*`, `make_square/file_of/rank_of`, `board_from_fen`, `fen_from_board`.
- Produces:
  - `struct Undo { Piece captured; int castling_rights; Square en_passant; int halfmove_clock; int fullmove_number; };`
  - `Undo make_move(Board& b, const Move& m);` (was `Board make_move(const Board&, const Move&)`)
  - `void unmake_move(Board& b, const Move& m, const Undo& u);`

**Theory:** `unmake_move` reverses a move using only the `Move` and the `Undo`. The piece motion reverses from the move itself: the piece is on `to`, move it back to `from` (restoring a pawn if it was a promotion); a castle's rook slides back; an en-passant capture puts the taken pawn back one rank behind `to`. Everything else was **destroyed** by `make_move` and must have been saved: the captured piece (overwritten on `to`, or removed beside it for en passant), and the four scalars `make_move` recomputes (castling rights, en-passant square, halfmove clock, fullmove number). Side-to-move flips back deterministically. Because `make_move` now changes the signature, this task also updates the three call sites so the tree still compiles and passes; they keep copying (`Board nb = b; make_move(nb, m);`) until Task 2 removes the copies.

- [ ] **Step 1: Write the failing round-trip test and update the forward helper**

Replace the `after` helper at the top of `engine/tests/test_move.cpp` and add a `roundtrip` helper plus a round-trip test. The existing forward `TEST_CASE`s stay as they are (they still assert the state after a move); only the helper changes shape.

Replace:
```cpp
// Apply one move to a FEN and compare the resulting FEN.
static std::string after(const char* fen, const Move& m) {
    return fen_from_board(make_move(board_from_fen(fen), m));
}
```
with (`move.hpp` is already included at the top of the file):
```cpp
// Apply one move in place and read back the resulting FEN.
static std::string after(const char* fen, const Move& m) {
    Board b = board_from_fen(fen);
    make_move(b, m);
    return fen_from_board(b);
}

// Make then unmake; the board must return to exactly the input FEN.
static std::string roundtrip(const char* fen, const Move& m) {
    Board b = board_from_fen(fen);
    Undo u = make_move(b, m);
    unmake_move(b, m, u);
    return fen_from_board(b);
}
```
Then append this new test case to the end of `engine/tests/test_move.cpp`:
```cpp
TEST_CASE("make then unmake restores the position exactly") {
    Move nf3{make_square(6,0), make_square(5,2), PieceType::None, MoveFlag::Normal};
    Move e4{make_square(4,1), make_square(4,3), PieceType::None, MoveFlag::DoublePawnPush};
    Move exd5{make_square(4,3), make_square(3,4), PieceType::None, MoveFlag::Normal};
    Move epc{make_square(4,4), make_square(3,5), PieceType::None, MoveFlag::EnPassant};
    Move promo{make_square(0,6), make_square(0,7), PieceType::Queen, MoveFlag::Promotion};
    Move castle{make_square(4,0), make_square(6,0), PieceType::None, MoveFlag::Castle};
    Move rookmove{make_square(0,0), make_square(1,0), PieceType::None, MoveFlag::Normal};

    const char* start = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    CHECK(roundtrip(start, nf3) == start);
    CHECK(roundtrip(start, e4)  == start);

    const char* cap = "rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 2";
    CHECK(roundtrip(cap, exd5) == cap);

    const char* ep = "rnbqkbnr/ppp1pppp/8/3pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 3";
    CHECK(roundtrip(ep, epc) == ep);

    const char* pr = "8/P7/8/8/8/8/8/4k2K w - - 0 1";
    CHECK(roundtrip(pr, promo) == pr);

    const char* cr = "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1";
    CHECK(roundtrip(cr, castle)   == cr);
    CHECK(roundtrip(cr, rookmove) == cr);
}
```

- [ ] **Step 2: Run the build, verify it FAILS to compile**

Run: `cmake --build engine/build`
Expected: compile errors, `unmake_move` not declared and `make_move` called with the wrong argument/return shape (the old `Board make_move(const Board&, ...)` no longer matches). This confirms the signature is about to change.

- [ ] **Step 3: Update `move.hpp` (add `Undo`, change signatures)**

In `engine/src/move.hpp`, add the `Undo` struct after the `Move` struct, and replace the two declarations at the bottom:
```cpp
struct Undo {
    Piece captured = Piece{Color::None, PieceType::None};
    int castling_rights = 0;
    Square en_passant = NO_SQUARE;
    int halfmove_clock = 0;
    int fullmove_number = 1;
};

std::string to_uci(const Move& m);
Undo make_move(Board& b, const Move& m);
void unmake_move(Board& b, const Move& m, const Undo& u);
```
(Remove the old `Board make_move(const Board& b, const Move& m);` line.)

- [ ] **Step 4: Rewrite `make_move` in place and add `unmake_move` in `move.cpp`**

In `engine/src/move.cpp`, replace the whole `make_move` definition with the in-place version and add `unmake_move` after it. Keep `to_uci` unchanged.
```cpp
Undo make_move(Board& b, const Move& m) {
    Piece moving = b.squares[m.from];
    Color us = moving.color;
    Color them = (us == Color::White) ? Color::Black : Color::White;

    // Save what unmake cannot reconstruct.
    Undo u;
    u.captured = b.squares[m.to];        // empty for a quiet move
    u.castling_rights = b.castling_rights;
    u.en_passant = b.en_passant;
    u.halfmove_clock = b.halfmove_clock;
    u.fullmove_number = b.fullmove_number;

    bool capture = b.squares[m.to].type != PieceType::None
                || m.flag == MoveFlag::EnPassant;
    bool pawn_move = moving.type == PieceType::Pawn;

    // Move the piece.
    b.squares[m.to] = moving;
    b.squares[m.from] = Piece{Color::None, PieceType::None};

    // En passant: remove (and record) the pawn one rank behind the destination.
    if (m.flag == MoveFlag::EnPassant) {
        int behind = (us == Color::White) ? -1 : 1;
        Square cap = make_square(file_of(m.to), rank_of(m.to) + behind);
        u.captured = b.squares[cap];     // the pawn taken en passant
        b.squares[cap] = Piece{Color::None, PieceType::None};
    }

    // Promotion: swap the pawn for the chosen piece.
    if (m.flag == MoveFlag::Promotion) {
        b.squares[m.to] = Piece{us, m.promotion};
    }

    // Castling: relocate the rook.
    if (m.flag == MoveFlag::Castle) {
        int r = rank_of(m.to);
        if (file_of(m.to) == 6) {        // kingside: rook h -> f
            b.squares[make_square(5, r)] = b.squares[make_square(7, r)];
            b.squares[make_square(7, r)] = Piece{Color::None, PieceType::None};
        } else {                         // queenside: rook a -> d
            b.squares[make_square(3, r)] = b.squares[make_square(0, r)];
            b.squares[make_square(0, r)] = Piece{Color::None, PieceType::None};
        }
    }

    // En passant target: set on a double push, cleared otherwise.
    if (m.flag == MoveFlag::DoublePawnPush) {
        int behind = (us == Color::White) ? -1 : 1;
        b.en_passant = make_square(file_of(m.to), rank_of(m.to) + behind);
    } else {
        b.en_passant = NO_SQUARE;
    }

    // Castling rights: strip when a king/rook leaves home or a home rook is captured.
    auto strip = [&](Square s) {
        if (s == make_square(4, 0)) b.castling_rights &= ~(CASTLE_WK | CASTLE_WQ);
        if (s == make_square(0, 0)) b.castling_rights &= ~CASTLE_WQ;
        if (s == make_square(7, 0)) b.castling_rights &= ~CASTLE_WK;
        if (s == make_square(4, 7)) b.castling_rights &= ~(CASTLE_BK | CASTLE_BQ);
        if (s == make_square(0, 7)) b.castling_rights &= ~CASTLE_BQ;
        if (s == make_square(7, 7)) b.castling_rights &= ~CASTLE_BK;
    };
    strip(m.from);
    strip(m.to);

    b.halfmove_clock = (capture || pawn_move) ? 0 : b.halfmove_clock + 1;
    if (us == Color::Black) b.fullmove_number += 1;
    b.side_to_move = them;
    return u;
}

void unmake_move(Board& b, const Move& m, const Undo& u) {
    // side_to_move currently points at the opponent; the mover is the other color.
    Color us = (b.side_to_move == Color::White) ? Color::Black : Color::White;

    // Restore the saved scalars.
    b.side_to_move = us;
    b.castling_rights = u.castling_rights;
    b.en_passant = u.en_passant;
    b.halfmove_clock = u.halfmove_clock;
    b.fullmove_number = u.fullmove_number;

    // Move the piece back; a promotion returns to a pawn.
    Piece moved = b.squares[m.to];
    if (m.flag == MoveFlag::Promotion) {
        b.squares[m.from] = Piece{us, PieceType::Pawn};
    } else {
        b.squares[m.from] = moved;
    }
    b.squares[m.to] = Piece{Color::None, PieceType::None};

    // Restore the captured piece.
    if (m.flag == MoveFlag::EnPassant) {
        int behind = (us == Color::White) ? -1 : 1;
        Square cap = make_square(file_of(m.to), rank_of(m.to) + behind);
        b.squares[cap] = u.captured;     // the pawn taken en passant; `to` stays empty
    } else {
        b.squares[m.to] = u.captured;    // empty piece if the move was not a capture
    }

    // Undo the castle's rook move.
    if (m.flag == MoveFlag::Castle) {
        int r = rank_of(m.to);
        if (file_of(m.to) == 6) {        // rook f -> h
            b.squares[make_square(7, r)] = b.squares[make_square(5, r)];
            b.squares[make_square(5, r)] = Piece{Color::None, PieceType::None};
        } else {                         // rook d -> a
            b.squares[make_square(0, r)] = b.squares[make_square(3, r)];
            b.squares[make_square(3, r)] = Piece{Color::None, PieceType::None};
        }
    }
}
```

- [ ] **Step 5: Migrate the three call sites to the new signature (still copying)**

In `engine/src/movegen.cpp`, replace the body of `generate_legal` so it copies then makes in place:
```cpp
std::vector<Move> generate_legal(const Board& b) {
    std::vector<Move> out;
    Color us = b.side_to_move;
    for (const Move& m : generate_pseudo_legal(b)) {
        Board nb = b;
        make_move(nb, m);
        if (!in_check(nb, us))
            out.push_back(m);
    }
    return out;
}
```

In `engine/src/perft.cpp`, replace both functions:
```cpp
uint64_t perft(const Board& b, int depth) {
    if (depth == 0) return 1;
    uint64_t nodes = 0;
    for (const Move& m : generate_legal(b)) {
        Board nb = b;
        make_move(nb, m);
        nodes += perft(nb, depth - 1);
    }
    return nodes;
}

std::map<std::string, uint64_t> perft_divide(const Board& b, int depth) {
    std::map<std::string, uint64_t> out;
    for (const Move& m : generate_legal(b)) {
        Board nb = b;
        make_move(nb, m);
        out[to_uci(m)] = (depth <= 1) ? 1 : perft(nb, depth - 1);
    }
    return out;
}
```

In `engine/tests/test_movegen.cpp`, the "in check" test calls `make_move` expecting a returned board. Replace its loop:
```cpp
    for (const Move& m : legal) {
        CHECK_FALSE(in_check(make_move(b, m), Color::White));
    }
```
with:
```cpp
    for (const Move& m : legal) {
        Board nb = b;
        make_move(nb, m);
        CHECK_FALSE(in_check(nb, Color::White));
    }
```

- [ ] **Step 6: Run the full suite, verify it PASSES**

Run: `cmake --build engine/build; ctest --test-dir engine/build --output-on-failure`
Expected: PASS, including the new round-trip test and every unchanged M2 test (movegen counts, perft numbers). If a round-trip fails, the mismatch names which field `unmake_move` restored wrong.

- [ ] **Step 7: Commit**

```bash
git add engine/src/move.hpp engine/src/move.cpp engine/src/movegen.cpp engine/src/perft.cpp engine/tests/test_move.cpp engine/tests/test_movegen.cpp
git commit -m "feat(engine): in-place make/unmake move with Undo record"
```

---

## Task 2: Thread one board through generate_legal and perft (remove the copies)

**Files:**
- Modify: `engine/src/movegen.hpp` (generate_legal takes `Board&`)
- Modify: `engine/src/movegen.cpp` (generate_legal filters with make/unmake in place)
- Modify: `engine/src/perft.hpp` (perft / perft_divide take `Board&`)
- Modify: `engine/src/perft.cpp` (thread one mutable board with make/unmake)

**Interfaces:**
- Consumes: `make_move`, `unmake_move`, `Undo` (Task 1); `generate_pseudo_legal`, `in_check`, `to_uci`.
- Produces:
  - `std::vector<Move> generate_legal(Board& b);` (was `const Board&`) — unchanged behavior, zero copies.
  - `uint64_t perft(Board& b, int depth);` and `std::map<std::string, uint64_t> perft_divide(Board& b, int depth);` (was `const Board&`).

**Theory:** The legality filter and perft both took a `const Board&` and copied per candidate move. Now they take a mutable `Board&` and, for each move, `make_move` then `unmake_move`, leaving the board exactly as found. `generate_pseudo_legal` returns a materialized `std::vector<Move>` before any mutation, so iterating it while mutating `b` is safe (the list is an independent copy of the moves, not a live view of the board). This is the state-discipline payoff: zero board copies on the hot path, and if `unmake_move` is even slightly wrong, the board drifts and perft's count changes, which is exactly what the unchanged-perft assertion catches. All existing call sites pass an lvalue `Board`, so the `const` to non-const change compiles without touching the tests.

- [ ] **Step 1: Change the declarations**

In `engine/src/movegen.hpp`, change:
```cpp
std::vector<Move> generate_legal(const Board& b);
```
to:
```cpp
std::vector<Move> generate_legal(Board& b);
```
In `engine/src/perft.hpp`, change:
```cpp
uint64_t perft(const Board& b, int depth);
std::map<std::string, uint64_t> perft_divide(const Board& b, int depth);
```
to:
```cpp
uint64_t perft(Board& b, int depth);
std::map<std::string, uint64_t> perft_divide(Board& b, int depth);
```

- [ ] **Step 2: Rewrite the two definitions to make/unmake in place**

In `engine/src/movegen.cpp`, replace `generate_legal`:
```cpp
std::vector<Move> generate_legal(Board& b) {
    std::vector<Move> out;
    Color us = b.side_to_move;
    for (const Move& m : generate_pseudo_legal(b)) {
        Undo u = make_move(b, m);
        if (!in_check(b, us))
            out.push_back(m);
        unmake_move(b, m, u);
    }
    return out;
}
```
In `engine/src/perft.cpp`, replace both functions:
```cpp
uint64_t perft(Board& b, int depth) {
    if (depth == 0) return 1;
    uint64_t nodes = 0;
    for (const Move& m : generate_legal(b)) {
        Undo u = make_move(b, m);
        nodes += perft(b, depth - 1);
        unmake_move(b, m, u);
    }
    return nodes;
}

std::map<std::string, uint64_t> perft_divide(Board& b, int depth) {
    std::map<std::string, uint64_t> out;
    for (const Move& m : generate_legal(b)) {
        Undo u = make_move(b, m);
        out[to_uci(m)] = (depth <= 1) ? 1 : perft(b, depth - 1);
        unmake_move(b, m, u);
    }
    return out;
}
```
Note: `generate_pseudo_legal(b)` and `generate_legal(b)` each return a `std::vector<Move>` by value, so the range-based `for` iterates a stable copy while the loop body mutates `b`. Do not change the loops to hold a reference to a board-derived view.

- [ ] **Step 3: Run the full suite, verify it PASSES with identical perft counts**

Run: `cmake --build engine/build; ctest --test-dir engine/build --output-on-failure`
Expected: PASS. Every perft number is byte-for-byte the same as M2 (start d4=197281, Kiwipete d3=97862, Position 3 d4=43238) and every move-generation count is unchanged. That the numbers did not move is the proof the copy-free make/unmake is faithful. If any perft number changed, `unmake_move` is not restoring some state, use `perft_divide` at the shallowest failing depth to find the move whose subtree drifted.

- [ ] **Step 4: Commit**

```bash
git add engine/src/movegen.hpp engine/src/movegen.cpp engine/src/perft.hpp engine/src/perft.cpp
git commit -m "feat(engine): thread one board through perft with make/unmake, no copies"
```

---

## Self-Review Notes

- **Spec coverage:** Implements spec §5 Milestone 3 ("Make / unmake move", success = "perft still matches after make+unmake"). Task 1 delivers the mechanism with a make/unmake round-trip proof; Task 2 removes copy-make from the hot path (`generate_legal` and `perft`) and re-verifies the exact M2 perft numbers. Evaluation, search, UCI (Milestones 4-6) remain out of scope.
- **Type consistency:** `Undo` fields (`captured`, `castling_rights`, `en_passant`, `halfmove_clock`, `fullmove_number`) are written in `make_move` and read in `unmake_move` identically. `make_move(Board&, const Move&) -> Undo` and `unmake_move(Board&, const Move&, const Undo&)` are used with matching signatures at every call site (`generate_legal`, `perft`, `perft_divide`, `test_move.cpp`, `test_movegen.cpp`). The non-const `Board&` change to `generate_legal`/`perft`/`perft_divide` in Task 2 binds at every existing call site because each passes an lvalue `Board`.
- **Placeholders:** none; every step carries runnable code.
- **Reversibility argument (the one subtle correctness point):** `unmake_move` derives the mover's color by flipping `b.side_to_move` (valid because `make_move` set it to the opponent), reverses the piece motion and any castle/promotion/en-passant special case from the `Move`, and restores the four saved scalars plus the captured piece from `Undo`. The captured square is `to` for a normal capture and one rank behind `to` for en passant, both derivable from the move, so `Undo` stores the captured piece but not its square. `// ponytail: Undo stores only destroyed state (captured piece + 4 scalars), not a board copy`.

## What Milestone 4 will cover (preview, not part of this plan)

Evaluation v1: a static `evaluate(const Board&)` returning a centipawn score from White's perspective, material first (piece values), then piece-square tables. No search yet; the sanity test is that obvious material advantages score correctly. Search (Milestone 5) will then call `make_move`/`unmake_move` from this milestone to walk the tree.
