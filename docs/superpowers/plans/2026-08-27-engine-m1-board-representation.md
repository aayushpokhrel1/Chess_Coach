# Engine Milestone 1: Toolchain + Board Representation — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Stand up a native C++ build on Windows and implement a fully-tested chess board representation that can round-trip any position through FEN.

**Architecture:** A small C++17 library (`engine`) built with CMake, tested with the single-header `doctest` framework. Core value types (color, piece, square) in one unit; the `Board` struct plus FEN parsing/generation in another. Everything is native for now; WebAssembly comes much later.

**Tech Stack:** C++17, CMake (+ Ninja), doctest (single-header test framework), MSYS2/MinGW-w64 toolchain on Windows.

**Spec:** `docs/superpowers/specs/2026-08-27-chess-coach-design.md` (implements Engine steps 0–1 of §4 / §8).

## Global Constraints

- **Language:** C++17. Set `CMAKE_CXX_STANDARD 17` with `CMAKE_CXX_STANDARD_REQUIRED ON`.
- **Correctness before speed:** simple 64-element array board representation (a1=0 … h8=63). No bitboards yet (spec §9).
- **Test framework:** doctest, vendored as `engine/third_party/doctest.h`.
- **Every code file is split into `.hpp` (declarations) + `.cpp` (definitions)** — deliberate, to learn the C++ compilation model. Small inline helpers may live in headers.
- **No dashes (— –) in any comment, message, or doc** per author preference; use commas/colons/parentheses.
- **Square indexing (used by every task):** `Square` is an `int` 0..63. `make_square(file, rank) = rank*8 + file`, where file 0..7 = a..h and rank 0..7 = ranks 1..8. `NO_SQUARE = -1`.

---

## File Structure

```
engine/
  CMakeLists.txt
  third_party/
    doctest.h            # vendored single-header test framework
  src/
    types.hpp            # Color, PieceType, Piece, Square + inline helpers
    types.cpp            # piece <-> FEN char conversions
    board.hpp            # Board struct, castling flags, FEN + ascii decls
    board.cpp            # board_from_fen, fen_from_board, start_position, to_ascii
  tests/
    test_main.cpp        # doctest entry point (compiled once)
    test_types.cpp       # tests for square + piece-char helpers
    test_board.cpp       # tests for FEN parse/generate/round-trip
```

---

## Task 1: Toolchain + project skeleton + a passing smoke test

**Files:**
- Create: `engine/CMakeLists.txt`
- Create: `engine/third_party/doctest.h` (downloaded)
- Create: `engine/tests/test_main.cpp`
- Create: `engine/tests/test_smoke.cpp` (deleted at end of task, once the loop is proven)

**Interfaces:**
- Consumes: nothing.
- Produces: a working `cmake --build` + `ctest` loop that later tasks rely on.

**Theory:** CMake is a *build system generator*: `CMakeLists.txt` describes targets (libraries, executables); CMake generates the actual build files (Ninja/Make/MSVC) for your platform. doctest is a *single-header* test framework, meaning you just `#include` one file; one `.cpp` defines its `main()`.

- [ ] **Step 1: Install the toolchain (interactive, one time)**

Recommended on Windows: install **MSYS2** from https://www.msys2.org, then in the "MSYS2 UCRT64" shell:
```bash
pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja
```
Add `C:\msys64\ucrt64\bin` to your PATH. Verify:
```bash
g++ --version && cmake --version && ninja --version
```
Expected: all three print versions. (Alternative: Visual Studio Build Tools + CMake; if you use it, drop `-G Ninja` below and let CMake pick MSVC.)

- [ ] **Step 2: Vendor the doctest header**

```bash
mkdir -p engine/third_party
curl -L -o engine/third_party/doctest.h https://raw.githubusercontent.com/doctest/doctest/v2.4.11/doctest/doctest.h
```
Expected: `engine/third_party/doctest.h` exists and is large (thousands of lines).

- [ ] **Step 3: Write the CMake file**

`engine/CMakeLists.txt`:
```cmake
cmake_minimum_required(VERSION 3.16)
project(chess_engine CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# The engine library (source files are added as later tasks create them).
add_library(engine
    src/types.cpp
    src/board.cpp
)
target_include_directories(engine PUBLIC src)

# Test executable.
add_executable(engine_tests
    tests/test_main.cpp
    tests/test_smoke.cpp
    tests/test_types.cpp
    tests/test_board.cpp
)
target_link_libraries(engine_tests PRIVATE engine)
target_include_directories(engine_tests PRIVATE third_party)

enable_testing()
add_test(NAME engine_tests COMMAND engine_tests)
```

Note: this lists files created in later tasks. To build Task 1 alone, temporarily comment out the `src/*.cpp` lines and the `test_types.cpp` / `test_board.cpp` lines; uncomment each as its task creates the file. (Simplest: create empty stub files now — see Step 4.)

- [ ] **Step 4: Create the doctest entry point and a failing smoke test**

`engine/tests/test_main.cpp`:
```cpp
// This single translation unit provides doctest's main().
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
```

`engine/tests/test_smoke.cpp`:
```cpp
#include "doctest.h"

TEST_CASE("toolchain smoke test") {
    // Intentionally wrong first, to prove the test loop actually runs.
    CHECK(1 + 1 == 3);
}
```

To build Task 1 in isolation, create empty placeholder files so CMake links:
```bash
mkdir -p engine/src
: > engine/src/types.cpp
: > engine/src/board.cpp
: > engine/tests/test_types.cpp
: > engine/tests/test_board.cpp
```

- [ ] **Step 5: Configure and build, verify the test FAILS**

Run:
```bash
cmake -S engine -B engine/build -G Ninja
cmake --build engine/build
ctest --test-dir engine/build --output-on-failure
```
Expected: build succeeds; ctest reports FAILED with the `1 + 1 == 3` check failing. (This proves the loop reports failures.)

- [ ] **Step 6: Fix the smoke test, verify it PASSES**

Edit `engine/tests/test_smoke.cpp`, change `== 3` to `== 2`. Rebuild and retest:
```bash
cmake --build engine/build && ctest --test-dir engine/build --output-on-failure
```
Expected: PASS, 1 test case.

- [ ] **Step 7: Remove the smoke test and commit**

Delete `engine/tests/test_smoke.cpp` and remove its line from `CMakeLists.txt`.
```bash
rm engine/tests/test_smoke.cpp
git add engine/CMakeLists.txt engine/third_party/doctest.h engine/tests/test_main.cpp engine/src/types.cpp engine/src/board.cpp engine/tests/test_types.cpp engine/tests/test_board.cpp
git commit -m "chore(engine): C++ toolchain, CMake skeleton, doctest wired up"
```

---

## Task 2: Core value types (color, piece, square, char conversions)

**Files:**
- Create: `engine/src/types.hpp`
- Modify: `engine/src/types.cpp` (currently empty stub)
- Modify: `engine/tests/test_types.cpp` (currently empty stub)

**Interfaces:**
- Consumes: nothing.
- Produces:
  - `enum class Color { White, Black, None }`
  - `enum class PieceType { Pawn, Knight, Bishop, Rook, Queen, King, None }`
  - `struct Piece { Color color; PieceType type; }`
  - `using Square = int; constexpr Square NO_SQUARE = -1;`
  - `Square make_square(int file, int rank);` `int file_of(Square);` `int rank_of(Square);` (inline)
  - `Piece piece_from_char(char c);` `char char_from_piece(Piece p);`

**Theory:** `enum class` gives you *type-safe* named constants (you can't accidentally mix a `Color` with an `int`). Declarations go in the `.hpp` so other files know these names exist; definitions of the non-inline functions go in the `.cpp` so they're compiled exactly once.

- [ ] **Step 1: Write the failing tests**

`engine/tests/test_types.cpp`:
```cpp
#include "doctest.h"
#include "types.hpp"

TEST_CASE("square coordinate math") {
    CHECK(make_square(0, 0) == 0);   // a1
    CHECK(make_square(7, 0) == 7);   // h1
    CHECK(make_square(0, 7) == 56);  // a8
    CHECK(make_square(4, 3) == 28);  // e4
    CHECK(file_of(28) == 4);
    CHECK(rank_of(28) == 3);
}

TEST_CASE("piece <-> FEN char round trip") {
    CHECK(char_from_piece(piece_from_char('K')).color_is_white());
    CHECK(char_from_piece(Piece{Color::White, PieceType::King}) == 'K');
    CHECK(char_from_piece(Piece{Color::Black, PieceType::Pawn}) == 'p');
    CHECK(char_from_piece(Piece{Color::None,  PieceType::None}) == '.');

    Piece wn = piece_from_char('N');
    CHECK(wn.color == Color::White);
    CHECK(wn.type == PieceType::Knight);

    Piece bq = piece_from_char('q');
    CHECK(bq.color == Color::Black);
    CHECK(bq.type == PieceType::Queen);
}
```
Note: the first `CHECK` above uses a helper `color_is_white()` we are NOT building; replace that one line with a direct check. Corrected first line of the second test:
```cpp
    CHECK(piece_from_char('K').color == Color::White);
```
(Use this corrected version; the `color_is_white()` phrasing was illustrative only.)

- [ ] **Step 2: Run tests, verify they FAIL to compile**

Run:
```bash
cmake --build engine/build && ctest --test-dir engine/build --output-on-failure
```
Expected: compile error, `types.hpp` not found / functions undefined.

- [ ] **Step 3: Write `types.hpp`**

`engine/src/types.hpp`:
```cpp
#pragma once

enum class Color { White, Black, None };

enum class PieceType { Pawn, Knight, Bishop, Rook, Queen, King, None };

struct Piece {
    Color color = Color::None;
    PieceType type = PieceType::None;
};

using Square = int;
constexpr Square NO_SQUARE = -1;

// file 0..7 = a..h, rank 0..7 = ranks 1..8
inline Square make_square(int file, int rank) { return rank * 8 + file; }
inline int file_of(Square s) { return s % 8; }
inline int rank_of(Square s) { return s / 8; }

Piece piece_from_char(char c);
char char_from_piece(Piece p);
```

- [ ] **Step 4: Write `types.cpp`**

`engine/src/types.cpp`:
```cpp
#include "types.hpp"
#include <cctype>

Piece piece_from_char(char c) {
    Color color = std::isupper(static_cast<unsigned char>(c)) ? Color::White : Color::Black;
    PieceType type;
    switch (std::tolower(static_cast<unsigned char>(c))) {
        case 'p': type = PieceType::Pawn;   break;
        case 'n': type = PieceType::Knight; break;
        case 'b': type = PieceType::Bishop; break;
        case 'r': type = PieceType::Rook;   break;
        case 'q': type = PieceType::Queen;  break;
        case 'k': type = PieceType::King;   break;
        default:  return Piece{Color::None, PieceType::None};
    }
    return Piece{color, type};
}

char char_from_piece(Piece p) {
    char c;
    switch (p.type) {
        case PieceType::Pawn:   c = 'p'; break;
        case PieceType::Knight: c = 'n'; break;
        case PieceType::Bishop: c = 'b'; break;
        case PieceType::Rook:   c = 'r'; break;
        case PieceType::Queen:  c = 'q'; break;
        case PieceType::King:   c = 'k'; break;
        default: return '.';
    }
    return p.color == Color::White ? static_cast<char>(std::toupper(c)) : c;
}
```

- [ ] **Step 5: Run tests, verify they PASS**

Run:
```bash
cmake --build engine/build && ctest --test-dir engine/build --output-on-failure
```
Expected: PASS.

- [ ] **Step 6: Commit**

```bash
git add engine/src/types.hpp engine/src/types.cpp engine/tests/test_types.cpp
git commit -m "feat(engine): core value types and FEN char conversions"
```

---

## Task 3: Board struct + FEN parsing + ascii view

**Files:**
- Create: `engine/src/board.hpp`
- Modify: `engine/src/board.cpp` (empty stub)
- Modify: `engine/tests/test_board.cpp` (empty stub)

**Interfaces:**
- Consumes: everything from `types.hpp` (Task 2).
- Produces:
  - Castling flags: `constexpr int CASTLE_WK=1, CASTLE_WQ=2, CASTLE_BK=4, CASTLE_BQ=8;`
  - `struct Board { std::array<Piece,64> squares; Color side_to_move; int castling_rights; Square en_passant; int halfmove_clock; int fullmove_number; };`
  - `Board board_from_fen(const std::string& fen);`
  - `std::string to_ascii(const Board& b);`

**Theory:** FEN lists the board from rank 8 down to rank 1, left to right, with digits meaning "N empty squares." Because our `rank` index has 0 = rank 1, the first FEN rank (rank 8) maps to rank index 7, and we count *down*.

- [ ] **Step 1: Write the failing test**

`engine/tests/test_board.cpp`:
```cpp
#include "doctest.h"
#include "board.hpp"

static const char* START_FEN =
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

TEST_CASE("parse start position from FEN") {
    Board b = board_from_fen(START_FEN);

    CHECK(b.squares[make_square(0, 0)].type == PieceType::Rook);   // a1
    CHECK(b.squares[make_square(0, 0)].color == Color::White);
    CHECK(b.squares[make_square(4, 7)].type == PieceType::King);   // e8
    CHECK(b.squares[make_square(4, 7)].color == Color::Black);
    CHECK(b.squares[make_square(4, 3)].type == PieceType::None);   // e4 empty

    CHECK(b.side_to_move == Color::White);
    CHECK(b.castling_rights == (CASTLE_WK | CASTLE_WQ | CASTLE_BK | CASTLE_BQ));
    CHECK(b.en_passant == NO_SQUARE);
    CHECK(b.halfmove_clock == 0);
    CHECK(b.fullmove_number == 1);
}

TEST_CASE("parse en passant and side to move") {
    Board b = board_from_fen(
        "rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR w KQkq c6 0 2");
    CHECK(b.side_to_move == Color::White);
    CHECK(b.en_passant == make_square(2, 5)); // c6
    CHECK(b.fullmove_number == 2);
}
```

- [ ] **Step 2: Run, verify it FAILS to compile**

Run: `cmake --build engine/build && ctest --test-dir engine/build --output-on-failure`
Expected: compile error, `board.hpp` not found.

- [ ] **Step 3: Write `board.hpp`**

`engine/src/board.hpp`:
```cpp
#pragma once
#include <array>
#include <string>
#include "types.hpp"

constexpr int CASTLE_WK = 1;
constexpr int CASTLE_WQ = 2;
constexpr int CASTLE_BK = 4;
constexpr int CASTLE_BQ = 8;

struct Board {
    std::array<Piece, 64> squares;
    Color side_to_move = Color::White;
    int castling_rights = 0;
    Square en_passant = NO_SQUARE;
    int halfmove_clock = 0;
    int fullmove_number = 1;
};

Board board_from_fen(const std::string& fen);
std::string to_ascii(const Board& b);
```

- [ ] **Step 4: Write `board_from_fen` and `to_ascii` in `board.cpp`**

`engine/src/board.cpp`:
```cpp
#include "board.hpp"
#include <sstream>

Board board_from_fen(const std::string& fen) {
    Board b;
    for (auto& sq : b.squares) sq = Piece{Color::None, PieceType::None};

    std::istringstream iss(fen);
    std::string placement, side, castling, ep;
    int halfmove = 0, fullmove = 1;
    iss >> placement >> side >> castling >> ep >> halfmove >> fullmove;

    int rank = 7, file = 0;
    for (char c : placement) {
        if (c == '/') { rank--; file = 0; }
        else if (c >= '1' && c <= '8') { file += c - '0'; }
        else { b.squares[make_square(file, rank)] = piece_from_char(c); file++; }
    }

    b.side_to_move = (side == "w") ? Color::White : Color::Black;

    b.castling_rights = 0;
    if (castling != "-") {
        for (char c : castling) {
            if (c == 'K') b.castling_rights |= CASTLE_WK;
            else if (c == 'Q') b.castling_rights |= CASTLE_WQ;
            else if (c == 'k') b.castling_rights |= CASTLE_BK;
            else if (c == 'q') b.castling_rights |= CASTLE_BQ;
        }
    }

    b.en_passant = (ep == "-")
        ? NO_SQUARE
        : make_square(ep[0] - 'a', ep[1] - '1');

    b.halfmove_clock = halfmove;
    b.fullmove_number = fullmove;
    return b;
}

std::string to_ascii(const Board& b) {
    std::string out;
    for (int rank = 7; rank >= 0; rank--) {
        for (int file = 0; file < 8; file++) {
            out += char_from_piece(b.squares[make_square(file, rank)]);
        }
        out += '\n';
    }
    return out;
}
```

- [ ] **Step 5: Run, verify PASS**

Run: `cmake --build engine/build && ctest --test-dir engine/build --output-on-failure`
Expected: PASS.

- [ ] **Step 6: Commit**

```bash
git add engine/src/board.hpp engine/src/board.cpp engine/tests/test_board.cpp
git commit -m "feat(engine): Board struct and FEN parsing"
```

---

## Task 4: FEN generation + round-trip + start_position helper

**Files:**
- Modify: `engine/src/board.hpp` (add two declarations)
- Modify: `engine/src/board.cpp` (add two definitions)
- Modify: `engine/tests/test_board.cpp` (add round-trip tests)

**Interfaces:**
- Consumes: `Board`, `board_from_fen` (Task 3).
- Produces:
  - `std::string fen_from_board(const Board& b);`
  - `Board start_position();`

**Theory:** Generation is the inverse of parsing: walk ranks 8→1, and while scanning a rank, count consecutive empty squares and flush that count as a digit before the next piece. If `fen_from_board(board_from_fen(x)) == x` for known-canonical FENs, both directions are almost certainly correct: a classic *round-trip* (property) test.

- [ ] **Step 1: Write the failing round-trip tests**

Append to `engine/tests/test_board.cpp`:
```cpp
TEST_CASE("FEN round-trips for known positions") {
    const char* fens[] = {
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        // Kiwipete, a standard perft test position:
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
        "8/8/8/4k3/8/8/4K3/8 b - - 5 39",
    };
    for (const char* f : fens) {
        CHECK(fen_from_board(board_from_fen(f)) == std::string(f));
    }
}

TEST_CASE("start_position matches the canonical start FEN") {
    CHECK(fen_from_board(start_position()) ==
          "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}
```

- [ ] **Step 2: Run, verify it FAILS**

Run: `cmake --build engine/build && ctest --test-dir engine/build --output-on-failure`
Expected: compile error, `fen_from_board` / `start_position` undefined.

- [ ] **Step 3: Declare the two functions in `board.hpp`**

Add under the existing declarations in `engine/src/board.hpp`:
```cpp
std::string fen_from_board(const Board& b);
Board start_position();
```

- [ ] **Step 4: Define them in `board.cpp`**

Append to `engine/src/board.cpp`:
```cpp
std::string fen_from_board(const Board& b) {
    std::string fen;
    for (int rank = 7; rank >= 0; rank--) {
        int empty = 0;
        for (int file = 0; file < 8; file++) {
            Piece p = b.squares[make_square(file, rank)];
            if (p.type == PieceType::None) {
                empty++;
            } else {
                if (empty > 0) { fen += std::to_string(empty); empty = 0; }
                fen += char_from_piece(p);
            }
        }
        if (empty > 0) fen += std::to_string(empty);
        if (rank > 0) fen += '/';
    }

    fen += ' ';
    fen += (b.side_to_move == Color::White) ? 'w' : 'b';

    fen += ' ';
    std::string cr;
    if (b.castling_rights & CASTLE_WK) cr += 'K';
    if (b.castling_rights & CASTLE_WQ) cr += 'Q';
    if (b.castling_rights & CASTLE_BK) cr += 'k';
    if (b.castling_rights & CASTLE_BQ) cr += 'q';
    fen += cr.empty() ? "-" : cr;

    fen += ' ';
    if (b.en_passant == NO_SQUARE) {
        fen += '-';
    } else {
        fen += static_cast<char>('a' + file_of(b.en_passant));
        fen += static_cast<char>('1' + rank_of(b.en_passant));
    }

    fen += ' ';
    fen += std::to_string(b.halfmove_clock);
    fen += ' ';
    fen += std::to_string(b.fullmove_number);
    return fen;
}

Board start_position() {
    return board_from_fen(
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}
```

- [ ] **Step 5: Run, verify PASS**

Run: `cmake --build engine/build && ctest --test-dir engine/build --output-on-failure`
Expected: PASS, all test cases green.

- [ ] **Step 6: Commit**

```bash
git add engine/src/board.hpp engine/src/board.cpp engine/tests/test_board.cpp
git commit -m "feat(engine): FEN generation, round-trip tests, start_position"
```

---

## Self-Review Notes

- **Spec coverage:** Implements Engine step 0 (toolchain, Task 1) and step 1 (board representation + FEN, Tasks 2–4) from spec §8. Move generation + perft (step 2) is intentionally deferred to plan #2.
- **Type consistency:** `Color`, `PieceType`, `Piece`, `Square`, `make_square/file_of/rank_of`, `CASTLE_*`, `Board`, `board_from_fen`, `fen_from_board`, `start_position`, `to_ascii` are named identically everywhere they appear across tasks.
- **Placeholders:** none; the one illustrative `color_is_white()` line in Task 2 is explicitly corrected in the same step.

## What plan #2 will cover (preview, not part of this plan)

Legal move generation, decomposed into learnable tasks: a `Move` type and make/unmake; pseudo-legal moves per piece (pawns incl. promotion/en passant, knights, king, sliding pieces); castling; check detection and legality filtering; and a `perft(depth)` counter verified against published numbers (start position and Kiwipete). That is the milestone where the engine's rules become *provably* correct.
