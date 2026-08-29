# Engine Milestone 2: Legal Move Generation + perft — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Generate all legal moves from any position and prove the rules correct with a `perft(depth)` counter that matches published node counts.

**Architecture:** Copy-make (each move produces a fresh `Board`; nothing is undone, so there is no undo bug to chase). Generation is pseudo-legal per piece, then filtered: make each candidate move and reject it if it leaves our own king attacked. One shared `is_square_attacked` primitive powers check detection, castling legality, and the legality filter. In-place make/unmake is deliberately deferred to Milestone 3.

**Tech Stack:** C++17, CMake (+ Ninja), doctest (single-header, vendored at `engine/third_party/doctest.h`), MSYS2/MinGW-w64 UCRT64 toolchain on Windows.

**Spec:** `docs/superpowers/specs/2026-08-27-chess-coach-design.md` (implements Engine step 2 of §8; §5 rows: Milestone 2 "legal move generation", success = perft leaf counts match published numbers).

## Global Constraints

- **Language:** C++17. `CMAKE_CXX_STANDARD 17`, `CMAKE_CXX_STANDARD_REQUIRED ON` (already set).
- **Correctness before speed:** copy-make, simple 64-element array board, ray scanning for sliders and attacks. No bitboards, no incremental state. Make/unmake and speed work belong to later milestones.
- **Test framework:** doctest, vendored at `engine/third_party/doctest.h`.
- **Every code file is split into `.hpp` (declarations) + `.cpp` (definitions).** Small `static` helpers may live in the `.cpp`; small hot helpers may be `inline` in the header.
- **No dashes (— –) in any comment, message, or doc;** use commas, colons, or parentheses.
- **Square indexing (from Milestone 1, unchanged):** `Square` is an `int` 0..63. `make_square(file, rank) = rank*8 + file`, file 0..7 = a..h, rank 0..7 = ranks 1..8. `NO_SQUARE = -1`. Helpers `file_of`, `rank_of` exist in `types.hpp`.
- **Existing symbols to reuse:** `Color`, `PieceType`, `Piece`, `Square`, `make_square/file_of/rank_of`, `piece_from_char/char_from_piece` (`types.hpp`); `Board`, `CASTLE_WK/WQ/BK/BQ`, `board_from_fen`, `fen_from_board`, `start_position`, `to_ascii` (`board.hpp`).
- **Build/test loop (PowerShell; git-bash cannot see the MSYS2 toolchain):**
  ```
  $env:Path = [Environment]::GetEnvironmentVariable("Path","User") + ";" + [Environment]::GetEnvironmentVariable("Path","Machine")
  cmake --build engine/build; ctest --test-dir engine/build --output-on-failure
  ```
  Re-run `cmake -S engine -B engine/build -G Ninja` once after each CMakeLists edit that adds a file.

---

## File Structure

```
engine/
  CMakeLists.txt         # MODIFY: add the three new src + three new test files
  src/
    types.hpp/.cpp       # (Milestone 1, unchanged)
    board.hpp/.cpp       # (Milestone 1, unchanged)
    move.hpp             # NEW: Move struct, MoveFlag, to_uci, make_move decl
    move.cpp             # NEW: to_uci + make_move (copy-make)
    movegen.hpp          # NEW: is_square_attacked, in_check, generate_pseudo_legal, generate_legal
    movegen.cpp          # NEW: attack scan + per-piece pseudo-legal + legality filter
    perft.hpp            # NEW: perft, perft_divide
    perft.cpp            # NEW: perft, perft_divide
  tests/
    test_move.cpp        # NEW: make_move applies each move type (checked via FEN)
    test_movegen.cpp     # NEW: attacks, per-piece move counts, legality
    test_perft.cpp       # NEW: perft node counts vs published numbers
```

Responsibility split: `move` = "what is a move and how do I apply one"; `movegen` = "what moves exist from here"; `perft` = "count them to prove the rules." `movegen` depends on `move` (it filters by applying moves); `perft` depends on both.

---

## Task 1: Move type + make_move (copy-make)

**Files:**
- Create: `engine/src/move.hpp`
- Create: `engine/src/move.cpp`
- Create: `engine/tests/test_move.cpp`
- Modify: `engine/CMakeLists.txt` (add `src/move.cpp`, `tests/test_move.cpp`)

**Interfaces:**
- Consumes: `Board`, `Piece`, `Color`, `PieceType`, `Square`, `make_square/file_of/rank_of`, `CASTLE_*`, `board_from_fen`, `fen_from_board`, `NO_SQUARE`.
- Produces:
  - `enum class MoveFlag { Normal, DoublePawnPush, EnPassant, Castle, Promotion };`
  - `struct Move { Square from; Square to; PieceType promotion; MoveFlag flag; };`
  - `std::string to_uci(const Move& m);` (e.g. `"e2e4"`, `"e7e8q"`)
  - `Board make_move(const Board& b, const Move& m);`

**Theory:** Copy-make returns a brand-new board with the move applied; the input is never touched. `make_move` is the one place that knows all the special-move bookkeeping: en-passant capture removes a pawn that is *not* on the destination square; promotion swaps the pawn for the chosen piece; castling moves the rook too; a double pawn push sets the en-passant target (and every other move clears it); moving or capturing a king/rook strips the matching castling right; the halfmove clock resets on a pawn move or any capture; the fullmove number ticks after Black moves. A single `MoveFlag` is enough because the special cases are mutually exclusive, and ordinary captures need no flag (they are detected by "the destination is occupied").

- [ ] **Step 1: Write the failing test**

`engine/tests/test_move.cpp`:
```cpp
#include "doctest.h"
#include "board.hpp"
#include "move.hpp"

// Apply one move to a FEN and compare the resulting FEN.
static std::string after(const char* fen, const Move& m) {
    return fen_from_board(make_move(board_from_fen(fen), m));
}

TEST_CASE("to_uci formats squares and promotion") {
    CHECK(to_uci(Move{make_square(4,1), make_square(4,3),
                      PieceType::None, MoveFlag::DoublePawnPush}) == "e2e4");
    CHECK(to_uci(Move{make_square(4,6), make_square(4,7),
                      PieceType::Queen, MoveFlag::Promotion}) == "e7e8q");
}

TEST_CASE("quiet move updates side, clock, and squares") {
    // 1.Nf3 from the start position.
    Move nf3{make_square(6,0), make_square(5,2), PieceType::None, MoveFlag::Normal};
    CHECK(after("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", nf3)
          == "rnbqkbnr/pppppppp/8/8/8/5N2/PPPPPPPP/RNBQKB1R b KQkq - 1 1");
}

TEST_CASE("double pawn push sets the en passant target") {
    Move e4{make_square(4,1), make_square(4,3), PieceType::None, MoveFlag::DoublePawnPush};
    CHECK(after("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", e4)
          == "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1");
}

TEST_CASE("capture resets the halfmove clock") {
    // White pawn on e4 takes black pawn on d5.
    Move exd5{make_square(4,3), make_square(3,4), PieceType::None, MoveFlag::Normal};
    CHECK(after("rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 2", exd5)
          == "rnbqkbnr/ppp1pppp/8/3P4/8/8/PPPP1PPP/RNBQKBNR b KQkq - 0 2");
}

TEST_CASE("en passant capture removes the passed pawn") {
    // White e5xd6 e.p.; the captured black pawn sits on d5, not d6.
    Move epc{make_square(4,4), make_square(3,5), PieceType::None, MoveFlag::EnPassant};
    CHECK(after("rnbqkbnr/ppp1pppp/8/3pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 3", epc)
          == "rnbqkbnr/ppp1pppp/3P4/8/8/8/PPPP1PPP/RNBQKBNR b KQkq - 0 3");
}

TEST_CASE("promotion replaces the pawn") {
    // White pawn a7-a8 promoting to a queen.
    Move promo{make_square(0,6), make_square(0,7), PieceType::Queen, MoveFlag::Promotion};
    CHECK(after("8/P7/8/8/8/8/8/4k2K w - - 0 1", promo)
          == "Q7/8/8/8/8/8/8/4k2K b - - 0 1");
}

TEST_CASE("kingside castling moves the rook and strips rights") {
    Move ok{make_square(4,0), make_square(6,0), PieceType::None, MoveFlag::Castle};
    CHECK(after("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1", ok)
          == "r3k2r/8/8/8/8/8/8/R4RK1 b kq - 1 1");
}

TEST_CASE("moving a rook strips only its own castling right") {
    Move ra1b1{make_square(0,0), make_square(1,0), PieceType::None, MoveFlag::Normal};
    CHECK(after("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1", ra1b1)
          == "r3k2r/8/8/8/8/8/8/1R2K2R b Kkq - 1 1");
}
```

- [ ] **Step 2: Run the test, verify it FAILS to compile**

First register the files, then build. In `engine/CMakeLists.txt` add `src/move.cpp` to the `engine` library sources and `tests/test_move.cpp` to the `engine_tests` sources:
```cmake
add_library(engine
    src/types.cpp
    src/board.cpp
    src/move.cpp
)
```
```cmake
add_executable(engine_tests
    tests/test_main.cpp
    tests/test_types.cpp
    tests/test_board.cpp
    tests/test_move.cpp
)
```
Run:
```
cmake -S engine -B engine/build -G Ninja
cmake --build engine/build
```
Expected: compile error, `move.hpp: No such file or directory`.

- [ ] **Step 3: Write `move.hpp`**

`engine/src/move.hpp`:
```cpp
#pragma once
#include <string>
#include "types.hpp"
#include "board.hpp"

enum class MoveFlag { Normal, DoublePawnPush, EnPassant, Castle, Promotion };

struct Move {
    Square from = NO_SQUARE;
    Square to = NO_SQUARE;
    PieceType promotion = PieceType::None; // used only when flag == Promotion
    MoveFlag flag = MoveFlag::Normal;
};

std::string to_uci(const Move& m);
Board make_move(const Board& b, const Move& m);
```

- [ ] **Step 4: Write `move.cpp`**

`engine/src/move.cpp`:
```cpp
#include "move.hpp"
#include "types.hpp"

std::string to_uci(const Move& m) {
    std::string s;
    s += static_cast<char>('a' + file_of(m.from));
    s += static_cast<char>('1' + rank_of(m.from));
    s += static_cast<char>('a' + file_of(m.to));
    s += static_cast<char>('1' + rank_of(m.to));
    if (m.flag == MoveFlag::Promotion) {
        s += char_from_piece(Piece{Color::Black, m.promotion}); // lowercase letter
    }
    return s;
}

Board make_move(const Board& b, const Move& m) {
    Board nb = b; // copy-make: work on a fresh board
    Piece moving = nb.squares[m.from];
    Color us = moving.color;
    Color them = (us == Color::White) ? Color::Black : Color::White;

    bool capture = nb.squares[m.to].type != PieceType::None
                || m.flag == MoveFlag::EnPassant;
    bool pawn_move = moving.type == PieceType::Pawn;

    // Move the piece.
    nb.squares[m.to] = moving;
    nb.squares[m.from] = Piece{Color::None, PieceType::None};

    // En passant: remove the pawn that sits one rank behind the destination.
    if (m.flag == MoveFlag::EnPassant) {
        int behind = (us == Color::White) ? -1 : 1;
        Square cap = make_square(file_of(m.to), rank_of(m.to) + behind);
        nb.squares[cap] = Piece{Color::None, PieceType::None};
    }

    // Promotion: swap the pawn for the chosen piece.
    if (m.flag == MoveFlag::Promotion) {
        nb.squares[m.to] = Piece{us, m.promotion};
    }

    // Castling: relocate the rook to the far side of the king.
    if (m.flag == MoveFlag::Castle) {
        int r = rank_of(m.to);
        if (file_of(m.to) == 6) {            // kingside: rook h -> f
            nb.squares[make_square(5, r)] = nb.squares[make_square(7, r)];
            nb.squares[make_square(7, r)] = Piece{Color::None, PieceType::None};
        } else {                             // queenside (file 2): rook a -> d
            nb.squares[make_square(3, r)] = nb.squares[make_square(0, r)];
            nb.squares[make_square(0, r)] = Piece{Color::None, PieceType::None};
        }
    }

    // En passant target: set on a double push, cleared otherwise.
    if (m.flag == MoveFlag::DoublePawnPush) {
        int behind = (us == Color::White) ? -1 : 1;
        nb.en_passant = make_square(file_of(m.to), rank_of(m.to) + behind);
    } else {
        nb.en_passant = NO_SQUARE;
    }

    // Castling rights: strip when a king/rook leaves home, or a home rook is captured.
    auto strip = [&](Square s) {
        if (s == make_square(4, 0)) nb.castling_rights &= ~(CASTLE_WK | CASTLE_WQ);
        if (s == make_square(0, 0)) nb.castling_rights &= ~CASTLE_WQ;
        if (s == make_square(7, 0)) nb.castling_rights &= ~CASTLE_WK;
        if (s == make_square(4, 7)) nb.castling_rights &= ~(CASTLE_BK | CASTLE_BQ);
        if (s == make_square(0, 7)) nb.castling_rights &= ~CASTLE_BQ;
        if (s == make_square(7, 7)) nb.castling_rights &= ~CASTLE_BK;
    };
    strip(m.from);
    strip(m.to);

    nb.halfmove_clock = (capture || pawn_move) ? 0 : nb.halfmove_clock + 1;
    if (us == Color::Black) nb.fullmove_number += 1;
    nb.side_to_move = them;
    return nb;
}
```

- [ ] **Step 5: Run the test, verify it PASSES**

Run: `cmake --build engine/build; ctest --test-dir engine/build --output-on-failure`
Expected: PASS.

- [ ] **Step 6: Commit**

```bash
git add engine/CMakeLists.txt engine/src/move.hpp engine/src/move.cpp engine/tests/test_move.cpp
git commit -m "feat(engine): Move type and copy-make make_move"
```

---

## Task 2: is_square_attacked + in_check

**Files:**
- Create: `engine/src/movegen.hpp`
- Create: `engine/src/movegen.cpp`
- Create: `engine/tests/test_movegen.cpp`
- Modify: `engine/CMakeLists.txt` (add `src/movegen.cpp`, `tests/test_movegen.cpp`)

**Interfaces:**
- Consumes: `Board`, `Piece`, `Color`, `PieceType`, `Square`, `make_square/file_of/rank_of`.
- Produces:
  - `bool is_square_attacked(const Board& b, Square sq, Color by);`
  - `bool in_check(const Board& b, Color side);`
  - (declared now, implemented in later tasks) `std::vector<Move> generate_pseudo_legal(const Board& b);` and `std::vector<Move> generate_legal(const Board& b);`

**Theory:** "Is square X attacked by color C?" is answered by looking *outward from X* for each attacker shape: the two diagonal squares where a C pawn would stand; the eight knight jumps; the eight king neighbours; the diagonal rays (first blocker a C bishop or queen); the orthogonal rays (first blocker a C rook or queen). Because the board is a 1D array, every offset is computed in (file, rank) space with an on-board bounds check, so a step off the a-file never wraps onto the h-file. This one function is the backbone: check detection, castling legality, and the legality filter all call it.

- [ ] **Step 1: Write the failing test**

`engine/tests/test_movegen.cpp`:
```cpp
#include "doctest.h"
#include "board.hpp"
#include "movegen.hpp"

TEST_CASE("pawn attacks are diagonal and forward") {
    // White pawn on d4 attacks c5 and e5, not d5.
    Board b = board_from_fen("4k3/8/8/8/3P4/8/8/4K3 w - - 0 1");
    CHECK(is_square_attacked(b, make_square(2,4), Color::White));  // c5
    CHECK(is_square_attacked(b, make_square(4,4), Color::White));  // e5
    CHECK_FALSE(is_square_attacked(b, make_square(3,4), Color::White)); // d5
}

TEST_CASE("rook attack is blocked by an intervening piece") {
    // White rook a1; black pawn a3 blocks the file above it.
    Board b = board_from_fen("4k3/8/8/8/8/p7/8/R3K3 w - - 0 1");
    CHECK(is_square_attacked(b, make_square(0,2), Color::White));  // a3 (the pawn itself)
    CHECK_FALSE(is_square_attacked(b, make_square(0,4), Color::White)); // a5, behind blocker
}

TEST_CASE("knight attack ignores blockers") {
    Board b = board_from_fen("4k3/8/8/8/8/8/8/N3K3 w - - 0 1"); // knight a1
    CHECK(is_square_attacked(b, make_square(1,2), Color::White));  // b3
    CHECK(is_square_attacked(b, make_square(2,1), Color::White));  // c2
    CHECK_FALSE(is_square_attacked(b, make_square(2,2), Color::White)); // c3
}

TEST_CASE("in_check detects an attacked king") {
    // Black king e8, white rook e1: king is in check down the open e-file.
    Board b = board_from_fen("4k3/8/8/8/8/8/8/4R1K1 b - - 0 1");
    CHECK(in_check(b, Color::Black));
    CHECK_FALSE(in_check(b, Color::White));
}
```

- [ ] **Step 2: Register files, run, verify it FAILS to compile**

Add `src/movegen.cpp` to the `engine` sources and `tests/test_movegen.cpp` to the test sources in `engine/CMakeLists.txt` (same pattern as Task 1). Then:
```
cmake -S engine -B engine/build -G Ninja
cmake --build engine/build
```
Expected: compile error, `movegen.hpp: No such file or directory`.

- [ ] **Step 3: Write `movegen.hpp`**

`engine/src/movegen.hpp`:
```cpp
#pragma once
#include <vector>
#include "board.hpp"
#include "move.hpp"

bool is_square_attacked(const Board& b, Square sq, Color by);
bool in_check(const Board& b, Color side);

std::vector<Move> generate_pseudo_legal(const Board& b);
std::vector<Move> generate_legal(const Board& b);
```

- [ ] **Step 4: Write `movegen.cpp` with the attack scan (generators come in later tasks)**

`engine/src/movegen.cpp`:
```cpp
#include "movegen.hpp"

namespace {
inline bool on_board(int f, int r) { return f >= 0 && f < 8 && r >= 0 && r < 8; }

const int KNIGHT[8][2] = {{1,2},{2,1},{2,-1},{1,-2},{-1,-2},{-2,-1},{-2,1},{-1,2}};
const int DIAG[4][2]   = {{1,1},{1,-1},{-1,1},{-1,-1}};
const int ORTH[4][2]   = {{1,0},{-1,0},{0,1},{0,-1}};

bool ray_hits(const Board& b, int f, int r, const int dirs[4][2],
              Color by, PieceType a, PieceType c) {
    for (int i = 0; i < 4; i++) {
        int ff = f + dirs[i][0], rr = r + dirs[i][1];
        while (on_board(ff, rr)) {
            Piece p = b.squares[make_square(ff, rr)];
            if (p.type != PieceType::None) {
                if (p.color == by && (p.type == a || p.type == c)) return true;
                break; // first blocker ends this ray
            }
            ff += dirs[i][0]; rr += dirs[i][1];
        }
    }
    return false;
}
} // namespace

bool is_square_attacked(const Board& b, Square sq, Color by) {
    int f = file_of(sq), r = rank_of(sq);

    // Pawns: a `by` pawn attacking sq stands one rank toward its own side.
    int pr = (by == Color::White) ? r - 1 : r + 1;
    for (int df : {-1, 1}) {
        if (on_board(f + df, pr)) {
            Piece p = b.squares[make_square(f + df, pr)];
            if (p.color == by && p.type == PieceType::Pawn) return true;
        }
    }
    // Knights.
    for (auto& d : KNIGHT) {
        if (on_board(f + d[0], r + d[1])) {
            Piece p = b.squares[make_square(f + d[0], r + d[1])];
            if (p.color == by && p.type == PieceType::Knight) return true;
        }
    }
    // King.
    for (int df = -1; df <= 1; df++)
        for (int dr = -1; dr <= 1; dr++) {
            if (df == 0 && dr == 0) continue;
            if (on_board(f + df, r + dr)) {
                Piece p = b.squares[make_square(f + df, r + dr)];
                if (p.color == by && p.type == PieceType::King) return true;
            }
        }
    // Sliders.
    if (ray_hits(b, f, r, DIAG, by, PieceType::Bishop, PieceType::Queen)) return true;
    if (ray_hits(b, f, r, ORTH, by, PieceType::Rook,   PieceType::Queen)) return true;
    return false;
}

bool in_check(const Board& b, Color side) {
    Color them = (side == Color::White) ? Color::Black : Color::White;
    for (Square s = 0; s < 64; s++) {
        Piece p = b.squares[s];
        if (p.color == side && p.type == PieceType::King)
            return is_square_attacked(b, s, them);
    }
    return false; // no king on board (not expected in legal positions)
}
```
Note: `generate_pseudo_legal` and `generate_legal` are declared but not yet defined. `test_movegen.cpp` does not call them yet, so the test binary links (nothing references the missing bodies). They are implemented in Tasks 3-6.

- [ ] **Step 5: Run the test, verify it PASSES**

Run: `cmake --build engine/build; ctest --test-dir engine/build --output-on-failure`
Expected: PASS.

- [ ] **Step 6: Commit**

```bash
git add engine/CMakeLists.txt engine/src/movegen.hpp engine/src/movegen.cpp engine/tests/test_movegen.cpp
git commit -m "feat(engine): square-attack detection and in_check"
```

---

## Task 3: Pseudo-legal knight and king moves

**Files:**
- Modify: `engine/src/movegen.cpp` (add `generate_pseudo_legal` + knight/king helpers)
- Modify: `engine/tests/test_movegen.cpp` (add move-generation tests)

**Interfaces:**
- Consumes: `is_square_attacked` (not yet, but same file), `Board`, `Move`, `MoveFlag`.
- Produces: `std::vector<Move> generate_pseudo_legal(const Board& b);` handling **knights and kings only** for now (other piece types are skipped until later tasks). Non-castling king moves only.

**Theory:** Pseudo-legal means "follows the piece's movement rule and does not land on a friendly piece," ignoring whether it leaves our own king in check. Knights and kings are the offset pieces: a fixed set of (file, rank) deltas, each kept if on-board and not onto a friendly piece; an enemy piece there is a capture and still a legal target. Filtering out self-check happens later in `generate_legal`.

- [ ] **Step 1: Write the failing test**

Append to `engine/tests/test_movegen.cpp`:
```cpp
#include <algorithm>

// True if `list` contains a move from->to (ignoring flag/promotion).
static bool has_move(const std::vector<Move>& list, Square from, Square to) {
    return std::any_of(list.begin(), list.end(), [&](const Move& m){
        return m.from == from && m.to == to;
    });
}

TEST_CASE("knight in the corner has two moves") {
    Board b = board_from_fen("4k3/8/8/8/8/8/8/N3K3 w - - 0 1"); // Na1, Ke1
    auto moves = generate_pseudo_legal(b);
    CHECK(has_move(moves, make_square(0,0), make_square(1,2))); // Nb3
    CHECK(has_move(moves, make_square(0,0), make_square(2,1))); // Nc2
    // Knight has exactly 2; total also includes the king's moves.
    int knight_moves = std::count_if(moves.begin(), moves.end(), [](const Move& m){
        return m.from == make_square(0,0);
    });
    CHECK(knight_moves == 2);
}

TEST_CASE("king moves off its start square, not onto friendly pieces") {
    Board b = board_from_fen("4k3/8/8/8/8/8/8/4K3 w - - 0 1"); // lone Ke1
    auto moves = generate_pseudo_legal(b);
    int king_moves = std::count_if(moves.begin(), moves.end(), [](const Move& m){
        return m.from == make_square(4,0);
    });
    CHECK(king_moves == 5); // d1,f1,d2,e2,f2
}

TEST_CASE("knight does not capture its own pieces") {
    // White knight b1 with pawns on d2; only a3, c3, d2(own->blocked) ...
    Board b = board_from_fen("4k3/8/8/8/8/8/3P4/1N2K3 w - - 0 1");
    auto moves = generate_pseudo_legal(b);
    CHECK_FALSE(has_move(moves, make_square(1,0), make_square(3,1))); // Nxd2 own pawn: illegal
    CHECK(has_move(moves, make_square(1,0), make_square(0,2)));       // Na3
    CHECK(has_move(moves, make_square(1,0), make_square(2,2)));       // Nc3
}
```

- [ ] **Step 2: Run, verify it FAILS at link**

Run: `cmake --build engine/build`
Expected: link error, undefined reference to `generate_pseudo_legal` (declared in Task 2, not yet defined).

- [ ] **Step 3: Add the knight/king generators and `generate_pseudo_legal` to `movegen.cpp`**

Add inside the anonymous `namespace { ... }` in `engine/src/movegen.cpp` (after `ray_hits`):
```cpp
void gen_offsets(const Board& b, Square s, const int offs[8][2], int n,
                 std::vector<Move>& out) {
    Color us = b.squares[s].color;
    int f = file_of(s), r = rank_of(s);
    for (int i = 0; i < n; i++) {
        int nf = f + offs[i][0], nr = r + offs[i][1];
        if (!on_board(nf, nr)) continue;
        Square to = make_square(nf, nr);
        if (b.squares[to].color == us) continue; // friendly piece blocks
        out.push_back(Move{s, to, PieceType::None, MoveFlag::Normal});
    }
}

const int KING[8][2] = {{1,0},{-1,0},{0,1},{0,-1},{1,1},{1,-1},{-1,1},{-1,-1}};
```
Then add, at file scope (outside the namespace), the dispatcher:
```cpp
std::vector<Move> generate_pseudo_legal(const Board& b) {
    std::vector<Move> out;
    Color us = b.side_to_move;
    for (Square s = 0; s < 64; s++) {
        Piece p = b.squares[s];
        if (p.color != us) continue;
        switch (p.type) {
            case PieceType::Knight: gen_offsets(b, s, KNIGHT, 8, out); break;
            case PieceType::King:   gen_offsets(b, s, KING,   8, out); break;
            default: break; // pawns, sliders added in later tasks
        }
    }
    return out;
}
```

- [ ] **Step 4: Run, verify it PASSES**

Run: `cmake --build engine/build; ctest --test-dir engine/build --output-on-failure`
Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add engine/src/movegen.cpp engine/tests/test_movegen.cpp
git commit -m "feat(engine): pseudo-legal knight and king moves"
```

---

## Task 4: Pseudo-legal sliding moves (bishop, rook, queen)

**Files:**
- Modify: `engine/src/movegen.cpp` (add a slider generator, wire into the dispatcher)
- Modify: `engine/tests/test_movegen.cpp` (add slider tests)

**Interfaces:**
- Consumes: same file's helpers (`on_board`, `DIAG`, `ORTH`).
- Produces: `generate_pseudo_legal` now also handles bishops (diagonals), rooks (orthogonals), and queens (both).

**Theory:** Sliders walk a ray until they leave the board or hit a piece. An empty square is a move and we keep going; a friendly piece stops the ray with no move; an enemy piece is a capture that also stops the ray. Bishop = the four diagonal rays, rook = the four orthogonal rays, queen = all eight.

- [ ] **Step 1: Write the failing test**

Append to `engine/tests/test_movegen.cpp`:
```cpp
TEST_CASE("rook on an open board has 14 moves") {
    Board b = board_from_fen("4k3/8/8/8/3R4/8/8/4K3 w - - 0 1"); // Rd4
    auto moves = generate_pseudo_legal(b);
    int rook = std::count_if(moves.begin(), moves.end(), [](const Move& m){
        return m.from == make_square(3,3);
    });
    CHECK(rook == 14); // 7 along the file + 7 along the rank
}

TEST_CASE("bishop is stopped by and can capture a blocker") {
    // Bishop c1; white pawn e3 blocks one diagonal, black pawn a3 is capturable.
    Board b = board_from_fen("4k3/8/8/8/8/p3P3/8/2B1K3 w - - 0 1");
    auto moves = generate_pseudo_legal(b);
    CHECK(has_move(moves, make_square(2,0), make_square(1,1))); // Bb2
    CHECK(has_move(moves, make_square(2,0), make_square(0,2))); // Bxa3 (capture)
    CHECK_FALSE(has_move(moves, make_square(2,0), make_square(5,3))); // past own e3 pawn
    CHECK_FALSE(has_move(moves, make_square(2,0), make_square(4,2))); // onto own e3 pawn
}

TEST_CASE("queen combines rook and bishop rays") {
    Board b = board_from_fen("4k3/8/8/8/3Q4/8/8/4K3 w - - 0 1"); // Qd4
    auto moves = generate_pseudo_legal(b);
    int q = std::count_if(moves.begin(), moves.end(), [](const Move& m){
        return m.from == make_square(3,3);
    });
    CHECK(q == 27); // 14 rook-like + 13 bishop-like from d4
}
```

- [ ] **Step 2: Run, verify it FAILS**

Run: `cmake --build engine/build; ctest --test-dir engine/build --output-on-failure`
Expected: FAIL (rook/bishop/queen counts are 0 because sliders are not generated yet).

- [ ] **Step 3: Add the slider generator and wire it in**

Add inside the anonymous namespace in `engine/src/movegen.cpp`:
```cpp
void gen_slider(const Board& b, Square s, const int dirs[4][2],
                std::vector<Move>& out) {
    Color us = b.squares[s].color;
    int f = file_of(s), r = rank_of(s);
    for (int i = 0; i < 4; i++) {
        int nf = f + dirs[i][0], nr = r + dirs[i][1];
        while (on_board(nf, nr)) {
            Square to = make_square(nf, nr);
            Piece p = b.squares[to];
            if (p.type == PieceType::None) {
                out.push_back(Move{s, to, PieceType::None, MoveFlag::Normal});
            } else {
                if (p.color != us)
                    out.push_back(Move{s, to, PieceType::None, MoveFlag::Normal});
                break; // ray stops at the first piece either way
            }
            nf += dirs[i][0]; nr += dirs[i][1];
        }
    }
}
```
Extend the `switch` in `generate_pseudo_legal`:
```cpp
            case PieceType::Bishop: gen_slider(b, s, DIAG, out); break;
            case PieceType::Rook:   gen_slider(b, s, ORTH, out); break;
            case PieceType::Queen:  gen_slider(b, s, DIAG, out);
                                    gen_slider(b, s, ORTH, out); break;
```

- [ ] **Step 4: Run, verify it PASSES**

Run: `cmake --build engine/build; ctest --test-dir engine/build --output-on-failure`
Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add engine/src/movegen.cpp engine/tests/test_movegen.cpp
git commit -m "feat(engine): pseudo-legal sliding moves"
```

---

## Task 5: Pseudo-legal pawn moves (push, double push, capture, en passant, promotion)

**Files:**
- Modify: `engine/src/movegen.cpp` (add pawn generator, wire in)
- Modify: `engine/tests/test_movegen.cpp` (add pawn tests)

**Interfaces:**
- Consumes: `Board.en_passant`, `MoveFlag::DoublePawnPush/EnPassant/Promotion`.
- Produces: `generate_pseudo_legal` now also handles pawns, emitting the correct flags so `make_move` (Task 1) applies them right. A promotion emits four moves (Q, R, B, N).

**Theory:** Pawns are the only asymmetric, direction-dependent piece. White advances toward higher ranks, Black toward lower. A single push needs the square ahead empty; a double push additionally needs the pawn on its start rank and both squares ahead empty, and it sets the `DoublePawnPush` flag. Captures go one square diagonally forward onto an enemy piece, or onto the `en_passant` target square (flagged `EnPassant`). Any push or capture landing on the last rank is a promotion and expands into four moves. Legality (leaving the king in check, including the rare en-passant discovered check) is handled later by the filter.

- [ ] **Step 1: Write the failing test**

Append to `engine/tests/test_movegen.cpp`:
```cpp
static int moves_from(const std::vector<Move>& list, Square from) {
    return std::count_if(list.begin(), list.end(), [&](const Move& m){
        return m.from == from;
    });
}

TEST_CASE("pawn on start rank can push one or two") {
    Board b = board_from_fen("4k3/8/8/8/8/8/4P3/4K3 w - - 0 1"); // Pe2
    auto moves = generate_pseudo_legal(b);
    CHECK(has_move(moves, make_square(4,1), make_square(4,2))); // e3
    CHECK(has_move(moves, make_square(4,1), make_square(4,3))); // e4
    CHECK(moves_from(moves, make_square(4,1)) == 2);
}

TEST_CASE("pawn captures diagonally and cannot capture straight") {
    // White Pe4; black pawns d5 and f5.
    Board b = board_from_fen("4k3/8/8/3p1p2/4P3/8/8/4K3 w - - 0 1");
    auto moves = generate_pseudo_legal(b);
    CHECK(has_move(moves, make_square(4,3), make_square(3,4))); // exd5
    CHECK(has_move(moves, make_square(4,3), make_square(5,4))); // exf5
    CHECK(has_move(moves, make_square(4,3), make_square(4,4))); // e5 push
    CHECK(moves_from(moves, make_square(4,3)) == 3);
}

TEST_CASE("en passant is emitted with the right flag") {
    Board b = board_from_fen("4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 1"); // Pe5, ep target d6
    auto moves = generate_pseudo_legal(b);
    auto it = std::find_if(moves.begin(), moves.end(), [](const Move& m){
        return m.from == make_square(4,4) && m.to == make_square(3,5);
    });
    REQUIRE(it != moves.end());
    CHECK(it->flag == MoveFlag::EnPassant);
}

TEST_CASE("promotion expands into four moves") {
    Board b = board_from_fen("4k3/P7/8/8/8/8/8/4K3 w - - 0 1"); // Pa7
    auto moves = generate_pseudo_legal(b);
    CHECK(moves_from(moves, make_square(0,6)) == 4); // a8=Q,R,B,N
    CHECK(std::any_of(moves.begin(), moves.end(), [](const Move& m){
        return m.from == make_square(0,6) && m.flag == MoveFlag::Promotion
            && m.promotion == PieceType::Queen;
    }));
}
```

- [ ] **Step 2: Run, verify it FAILS**

Run: `cmake --build engine/build; ctest --test-dir engine/build --output-on-failure`
Expected: FAIL (pawn move counts are 0; pawns not generated yet).

- [ ] **Step 3: Add the pawn generator and wire it in**

Add inside the anonymous namespace in `engine/src/movegen.cpp`:
```cpp
void add_pawn(std::vector<Move>& out, Square from, Square to,
              bool promo, MoveFlag flag) {
    if (promo) {
        for (PieceType pt : {PieceType::Queen, PieceType::Rook,
                             PieceType::Bishop, PieceType::Knight})
            out.push_back(Move{from, to, pt, MoveFlag::Promotion});
    } else {
        out.push_back(Move{from, to, PieceType::None, flag});
    }
}

void gen_pawn(const Board& b, Square s, std::vector<Move>& out) {
    Color us = b.squares[s].color;
    Color them = (us == Color::White) ? Color::Black : Color::White;
    int f = file_of(s), r = rank_of(s);
    int dir       = (us == Color::White) ? 1 : -1;
    int startRank = (us == Color::White) ? 1 : 6;
    int promoRank = (us == Color::White) ? 7 : 0;

    // Single (and double) push.
    int r1 = r + dir;
    if (on_board(f, r1) && b.squares[make_square(f, r1)].type == PieceType::None) {
        add_pawn(out, s, make_square(f, r1), r1 == promoRank, MoveFlag::Normal);
        if (r == startRank) {
            int r2 = r + 2 * dir;
            if (b.squares[make_square(f, r2)].type == PieceType::None)
                out.push_back(Move{s, make_square(f, r2),
                                   PieceType::None, MoveFlag::DoublePawnPush});
        }
    }
    // Captures, including en passant.
    for (int df : {-1, 1}) {
        int nf = f + df, nr = r + dir;
        if (!on_board(nf, nr)) continue;
        Square to = make_square(nf, nr);
        if (b.squares[to].color == them) {
            add_pawn(out, s, to, nr == promoRank, MoveFlag::Normal);
        } else if (b.en_passant != NO_SQUARE && to == b.en_passant) {
            out.push_back(Move{s, to, PieceType::None, MoveFlag::EnPassant});
        }
    }
}
```
Extend the `switch` in `generate_pseudo_legal`:
```cpp
            case PieceType::Pawn:   gen_pawn(b, s, out); break;
```

- [ ] **Step 4: Run, verify it PASSES**

Run: `cmake --build engine/build; ctest --test-dir engine/build --output-on-failure`
Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add engine/src/movegen.cpp engine/tests/test_movegen.cpp
git commit -m "feat(engine): pseudo-legal pawn moves with promotion and en passant"
```

---

## Task 6: Castling move generation

**Files:**
- Modify: `engine/src/movegen.cpp` (add castling generator, call once per position)
- Modify: `engine/tests/test_movegen.cpp` (add castling tests)

**Interfaces:**
- Consumes: `is_square_attacked` (Task 2), `Board.castling_rights`, `CASTLE_*`.
- Produces: `generate_pseudo_legal` now also emits castling moves (flagged `Castle`) when legal by the castling rules.

**Theory:** Castling is legal only if: the right still exists; the squares between king and rook are empty; the king is not currently in check; and the king does not pass through or land on an attacked square. Kingside the king goes e→g (passing f), queenside e→c (passing d); queenside also needs the b-file square empty but that square is not one the king crosses, so it is checked for emptiness only, not for attack. We reuse `is_square_attacked` for the "not through/into check" tests here; the final landing square is also re-checked by the legality filter in Task 7, which is harmless overlap.

- [ ] **Step 1: Write the failing test**

Append to `engine/tests/test_movegen.cpp`:
```cpp
TEST_CASE("both castles available on an empty back rank") {
    Board b = board_from_fen("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");
    auto moves = generate_pseudo_legal(b);
    CHECK(has_move(moves, make_square(4,0), make_square(6,0))); // O-O
    CHECK(has_move(moves, make_square(4,0), make_square(2,0))); // O-O-O
}

TEST_CASE("cannot castle through an attacked square") {
    // Black rook on f8 attacks f1; kingside castling passes through f1.
    Board b = board_from_fen("r3kr2/8/8/8/8/8/8/R3K2R w KQq - 0 1");
    auto moves = generate_pseudo_legal(b);
    CHECK_FALSE(has_move(moves, make_square(4,0), make_square(6,0))); // O-O blocked
}

TEST_CASE("cannot castle without the right") {
    Board b = board_from_fen("r3k2r/8/8/8/8/8/8/R3K2R w - - 0 1"); // no rights
    auto moves = generate_pseudo_legal(b);
    CHECK_FALSE(has_move(moves, make_square(4,0), make_square(6,0)));
    CHECK_FALSE(has_move(moves, make_square(4,0), make_square(2,0)));
}

TEST_CASE("cannot castle out of check") {
    // Black rook e8 gives check down the e-file; no castling while in check.
    Board b = board_from_fen("4r3/8/8/8/8/8/8/R3K2R w KQ - 0 1");
    auto moves = generate_pseudo_legal(b);
    CHECK_FALSE(has_move(moves, make_square(4,0), make_square(6,0)));
    CHECK_FALSE(has_move(moves, make_square(4,0), make_square(2,0)));
}
```

- [ ] **Step 2: Run, verify it FAILS**

Run: `cmake --build engine/build; ctest --test-dir engine/build --output-on-failure`
Expected: FAIL (castling moves not generated; the "available" case fails).

- [ ] **Step 3: Add the castling generator and call it**

Add inside the anonymous namespace in `engine/src/movegen.cpp`:
```cpp
void gen_castling(const Board& b, std::vector<Move>& out) {
    Color us = b.side_to_move;
    Color them = (us == Color::White) ? Color::Black : Color::White;
    int r = (us == Color::White) ? 0 : 7;
    Square king = make_square(4, r);
    if (b.squares[king].type != PieceType::King || b.squares[king].color != us)
        return;
    if (is_square_attacked(b, king, them)) return; // cannot castle out of check

    int kRight = (us == Color::White) ? CASTLE_WK : CASTLE_BK;
    int qRight = (us == Color::White) ? CASTLE_WQ : CASTLE_BQ;

    // Kingside: f and g empty; f and g not attacked.
    if ((b.castling_rights & kRight)
        && b.squares[make_square(5, r)].type == PieceType::None
        && b.squares[make_square(6, r)].type == PieceType::None
        && !is_square_attacked(b, make_square(5, r), them)
        && !is_square_attacked(b, make_square(6, r), them)) {
        out.push_back(Move{king, make_square(6, r), PieceType::None, MoveFlag::Castle});
    }
    // Queenside: b, c, d empty; c and d not attacked (king crosses d, lands c).
    if ((b.castling_rights & qRight)
        && b.squares[make_square(1, r)].type == PieceType::None
        && b.squares[make_square(2, r)].type == PieceType::None
        && b.squares[make_square(3, r)].type == PieceType::None
        && !is_square_attacked(b, make_square(3, r), them)
        && !is_square_attacked(b, make_square(2, r), them)) {
        out.push_back(Move{king, make_square(2, r), PieceType::None, MoveFlag::Castle});
    }
}
```
Call it once at the end of `generate_pseudo_legal`, just before `return out;`:
```cpp
    gen_castling(b, out);
    return out;
```

- [ ] **Step 4: Run, verify it PASSES**

Run: `cmake --build engine/build; ctest --test-dir engine/build --output-on-failure`
Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add engine/src/movegen.cpp engine/tests/test_movegen.cpp
git commit -m "feat(engine): castling move generation"
```

---

## Task 7: Legality filter (generate_legal) + in_check integration

**Files:**
- Modify: `engine/src/movegen.cpp` (add `generate_legal`)
- Modify: `engine/tests/test_movegen.cpp` (add legality tests)

**Interfaces:**
- Consumes: `generate_pseudo_legal` (Tasks 3-6), `make_move` (Task 1), `in_check` (Task 2).
- Produces: `std::vector<Move> generate_legal(const Board& b);` returning only moves that do not leave the mover's own king in check.

**Theory:** The cheap, always-correct legality test: play each pseudo-legal move on a copy, then ask whether our own king is now attacked. If it is, the move was illegal (it left the king in check, or moved a pinned piece off the pin, or was an en-passant capture that opened a discovered check). This one filter subsumes pins, check evasion, and the en-passant discovered-check special case without any of them being coded explicitly. `make_move` already flips the side to move, so we pass our own color explicitly to `in_check`.

- [ ] **Step 1: Write the failing test**

Append to `engine/tests/test_movegen.cpp`:
```cpp
TEST_CASE("a pinned piece cannot move off the pin") {
    // White king e1, white knight e2, black rook e8: the knight is pinned.
    Board b = board_from_fen("4r3/8/8/8/8/8/4N3/4K3 w - - 0 1");
    auto legal = generate_legal(b);
    // Any knight move exposes the king, so none is legal.
    CHECK(moves_from(legal, make_square(4,1)) == 0);
    // The king may still step aside off the e-file.
    CHECK(has_move(legal, make_square(4,0), make_square(3,0))); // Kd1
}

TEST_CASE("in check, only moves that resolve the check are legal") {
    // Black rook e8 checks white Ke1; white also has a rook a1 that cannot help.
    Board b = board_from_fen("4r3/8/8/8/8/8/8/R3K3 w - - 0 1");
    auto legal = generate_legal(b);
    // Every legal move must leave the white king safe.
    for (const Move& m : legal) {
        CHECK_FALSE(in_check(make_move(b, m), Color::White));
    }
    // The only escapes are king steps off the e-file (Ra1 cannot reach or block e-file).
    CHECK(has_move(legal, make_square(4,0), make_square(3,0))); // Kd1
    CHECK(has_move(legal, make_square(4,0), make_square(5,0))); // Kf1
    CHECK(generate_legal(b).size() == 4); // Kd1, Kf1, Kd2, Kf2
}

TEST_CASE("start position has exactly 20 legal moves") {
    Board b = start_position();
    CHECK(generate_legal(b).size() == 20);
}
```

- [ ] **Step 2: Run, verify it FAILS at link**

Run: `cmake --build engine/build`
Expected: link error, undefined reference to `generate_legal`.

- [ ] **Step 3: Implement `generate_legal`**

Add at file scope in `engine/src/movegen.cpp` (after `generate_pseudo_legal`):
```cpp
std::vector<Move> generate_legal(const Board& b) {
    std::vector<Move> out;
    Color us = b.side_to_move;
    for (const Move& m : generate_pseudo_legal(b)) {
        if (!in_check(make_move(b, m), us))
            out.push_back(m);
    }
    return out;
}
```

- [ ] **Step 4: Run, verify it PASSES**

Run: `cmake --build engine/build; ctest --test-dir engine/build --output-on-failure`
Expected: PASS, including the 20-legal-moves start position check.

- [ ] **Step 5: Commit**

```bash
git add engine/src/movegen.cpp engine/tests/test_movegen.cpp
git commit -m "feat(engine): legal move filtering by king safety"
```

---

## Task 8: perft + verification against published counts

**Files:**
- Create: `engine/src/perft.hpp`
- Create: `engine/src/perft.cpp`
- Create: `engine/tests/test_perft.cpp`
- Modify: `engine/CMakeLists.txt` (add `src/perft.cpp`, `tests/test_perft.cpp`)

**Interfaces:**
- Consumes: `generate_legal` (Task 7), `make_move` (Task 1), `to_uci` (Task 1).
- Produces:
  - `uint64_t perft(const Board& b, int depth);`
  - `std::map<std::string, uint64_t> perft_divide(const Board& b, int depth);` (debugging aid: per-root-move node counts)

**Theory:** `perft(depth)` counts leaf nodes of the legal move tree: `perft(0) = 1`, otherwise sum `perft(child, depth-1)` over all legal moves. Because it exercises every rule at every ply, matching published counts for known positions is near-proof that move generation is correct, the single most valuable test in a chess engine. When a count is wrong, `perft_divide` (perft split by first move) localizes which move's subtree is off, then you recurse into that child to find the offending rule.

Published counts used below (Chess Programming Wiki):
- **Start position** `rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1`: d1=20, d2=400, d3=8902, d4=197281, (d5=4865609).
- **Kiwipete** `r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1`: d1=48, d2=2039, d3=97862, (d4=4085603).
- **Position 3** `8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1` (an en-passant / discovered-check trap): d1=14, d2=191, d3=2812, d4=43238.

The asserted tests stop at depths that run in well under a second on an optimized build (start d4, Kiwipete d3, Position 3 d4). The deeper counts (start d5, Kiwipete d4) are listed above for a manual confirmation run; they are correct but slow under copy-make in an unoptimized build, so they are not part of the default suite.

- [ ] **Step 1: Write the failing test**

`engine/tests/test_perft.cpp`:
```cpp
#include "doctest.h"
#include "board.hpp"
#include "perft.hpp"

TEST_CASE("perft of the start position") {
    Board b = start_position();
    CHECK(perft(b, 1) == 20);
    CHECK(perft(b, 2) == 400);
    CHECK(perft(b, 3) == 8902);
    CHECK(perft(b, 4) == 197281);
}

TEST_CASE("perft of Kiwipete exercises castling, pins, promotions") {
    Board b = board_from_fen(
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
    CHECK(perft(b, 1) == 48);
    CHECK(perft(b, 2) == 2039);
    CHECK(perft(b, 3) == 97862);
}

TEST_CASE("perft of Position 3 exercises en passant edge cases") {
    Board b = board_from_fen("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1");
    CHECK(perft(b, 1) == 14);
    CHECK(perft(b, 2) == 191);
    CHECK(perft(b, 3) == 2812);
    CHECK(perft(b, 4) == 43238);
}
```

- [ ] **Step 2: Register files, run, verify it FAILS to compile**

Add `src/perft.cpp` to the `engine` sources and `tests/test_perft.cpp` to the test sources in `engine/CMakeLists.txt`, then:
```
cmake -S engine -B engine/build -G Ninja
cmake --build engine/build
```
Expected: compile error, `perft.hpp: No such file or directory`.

- [ ] **Step 3: Write `perft.hpp`**

`engine/src/perft.hpp`:
```cpp
#pragma once
#include <cstdint>
#include <map>
#include <string>
#include "board.hpp"

uint64_t perft(const Board& b, int depth);
std::map<std::string, uint64_t> perft_divide(const Board& b, int depth);
```

- [ ] **Step 4: Write `perft.cpp`**

`engine/src/perft.cpp`:
```cpp
#include "perft.hpp"
#include "move.hpp"
#include "movegen.hpp"

uint64_t perft(const Board& b, int depth) {
    if (depth == 0) return 1;
    uint64_t nodes = 0;
    for (const Move& m : generate_legal(b))
        nodes += perft(make_move(b, m), depth - 1);
    return nodes;
}

std::map<std::string, uint64_t> perft_divide(const Board& b, int depth) {
    std::map<std::string, uint64_t> out;
    for (const Move& m : generate_legal(b))
        out[to_uci(m)] = (depth <= 1) ? 1 : perft(make_move(b, m), depth - 1);
    return out;
}
```

- [ ] **Step 5: Run, verify it PASSES**

Run: `cmake --build engine/build; ctest --test-dir engine/build --output-on-failure`
Expected: PASS. If a count is wrong, use `perft_divide` at the failing depth, compare each root move's subtree against a reference (e.g. Stockfish `go perft`), recurse into the mismatching move, and fix the rule it exposes. Do not advance with any perft failing (spec §10).

- [ ] **Step 6: Commit**

```bash
git add engine/CMakeLists.txt engine/src/perft.hpp engine/src/perft.cpp engine/tests/test_perft.cpp
git commit -m "feat(engine): perft counter verified against published counts"
```

---

## Self-Review Notes

- **Spec coverage:** Implements Engine step 2 (spec §8): "generate all legal moves from any position; write a `perft(depth)` counter; verify counts against published numbers for the start position and standard test positions (e.g. Kiwipete)." Tasks 3-6 cover generation of every piece type + castling; Task 7 covers legality (pins, check, en-passant discovered check via the make-and-test filter); Task 8 covers perft + the required positions. Make/unmake (spec §5 Milestone 3) is intentionally out of scope. Evaluation, search, UCI (Milestones 4-6) are out of scope.
- **Type consistency:** `Move`, `MoveFlag` (`Normal/DoublePawnPush/EnPassant/Castle/Promotion`), `make_move`, `to_uci`, `is_square_attacked`, `in_check`, `generate_pseudo_legal`, `generate_legal`, `perft`, `perft_divide` are named identically everywhere they appear. Board/Types symbols reused verbatim from Milestone 1.
- **Placeholders:** none; every step carries runnable code. The one deferred item, deeper perft depths (start d5, Kiwipete d4), is a documented manual-run choice, not a gap: the required positions already prove correctness.
- **Copy-make cost (deliberate ceiling):** perft under copy-make allocates a `Board` and a `std::vector<Move>` per node, and the legality filter calls `make_move` once per pseudo-legal move. That is why asserted depths are capped. The upgrade path is Milestone 3 (make/unmake) and later bitboards (spec §5 Milestone 7), with perft as the unchanged safety net. `// ponytail: copy-make, O(board copy) per node; make/unmake in M3 if perft depth matters`.

## What Milestone 3 will cover (preview, not part of this plan)

In-place `make`/`unmake` with an explicit undo record (captured piece, prior castling rights, prior en-passant square, prior halfmove clock), replacing copy-make on the perft hot path; verify identical perft counts before and after each move (the "state discipline" success test from spec §5). This is a refactor guarded entirely by the perft numbers this milestone establishes.
