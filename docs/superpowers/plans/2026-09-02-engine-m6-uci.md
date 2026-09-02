# Engine Milestone 6: UCI Interface Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Let the engine play a full game in a chess GUI: speak UCI over stdin/stdout, hold a game position, search under a real time budget, and answer `go` with a legal `bestmove`.

**Architecture:** Two new pieces plus one search extension. (1) The search gains `search_timed(Board&, SearchLimits)`: iterative deepening under an optional wall-clock deadline, checked every few thousand nodes, returning the best move from the last fully completed depth (depth 1 always finishes). (2) A `uci` unit parses one command line at a time through a testable `handle_command(UciState&, line) -> string`, applies `position`/`moves`, allocates a per-move time budget from the clock, and formats `bestmove`. (3) A thin `main.cpp` is the stdin loop a GUI launches. The fixed-depth `search`/`search_to_depth` from Milestone 5 stay intact (guarded so they never time out) and remain the correctness oracles.

**Tech Stack:** C++17, CMake (+ Ninja), doctest (single-header, vendored at `engine/third_party/doctest.h`), MSYS2/MinGW-w64 UCRT64 toolchain on Windows. Uses `<chrono>` for the clock.

**Spec:** `docs/superpowers/specs/2026-08-27-chess-coach-design.md` (§4 Track A step 6: "UCI interface", success = "Plays a full game vs Stockfish via a GUI"; §3 the UCI seam). Builds on Milestones 1 to 5.

## Global Constraints

- **Language:** C++17 (`CMAKE_CXX_STANDARD 17`, already set).
- **UCI is line-based text over stdin/stdout.** Each response line must be flushed (the stdin loop uses `std::endl`). GUIs need `id`/`uciok`, `readyok`, and `bestmove <move>` exactly.
- **Move format:** long algebraic, `<from><to>[promo]`, e.g. `e2e4`, `e7e8q`. The null/no-move answer is `bestmove 0000`.
- **Time budget (decided):** `go` supports `depth N` (fixed depth, ignore clock), `movetime ms`, `wtime/btime/winc/binc` (allocate from the clock for the side to move), and `infinite`. Allocation heuristic: `remaining/20 + inc/2`, capped below `remaining - 30` and floored at 1 ms. (`// ponytail: crude clock split; tune only if the engine flags or dawdles in real games`.)
- **Search timing must not corrupt results:** an aborted depth is discarded; the move returned is always from a fully completed depth. Depth 1 is never aborted, so a legal move is always returned when one exists.
- **Milestone 5 search is preserved:** `search(Board&, int)`, `search_to_depth(Board&, int, Move)`, `search_minimax`, `nodes_searched` keep their behavior; the timing flags default off so those paths never abort.
- **Square indexing (unchanged):** `make_square(file, rank) = rank*8 + file`; file 0..7 = a..h; rank 0..7 = ranks 1..8; `NO_SQUARE = -1`.
- **Reused symbols:** `Board`, `board_from_fen`, `fen_from_board`, `start_position` (`board.hpp`); `Move`, `MoveFlag`, `make_move`, `to_uci` (`move.hpp`); `generate_legal` (`movegen.hpp`); `SearchResult`, `search_to_depth` (`search.hpp`); `Color`, `PieceType`, `Square`, `make_square`, `NO_SQUARE` (`types.hpp`).
- **Test framework:** doctest, vendored at `engine/third_party/doctest.h`.
- **Every code file stays split** into `.hpp` + `.cpp`. `main.cpp` is the only file with `int main` and it is NOT part of the `engine` library.
- **No dashes (— –)** in any comment, message, or doc.
- **Build/test loop (PowerShell; git-bash cannot see the MSYS2 toolchain):**
  ```
  $env:Path = [Environment]::GetEnvironmentVariable("Path","User") + ";" + [Environment]::GetEnvironmentVariable("Path","Machine")
  cmake --build engine/build; ctest --test-dir engine/build --output-on-failure
  ```
  Re-run `cmake -S engine -B engine/build -G Ninja` once after Task 2 adds `uci.cpp`/`test_uci.cpp` and again after Task 5 adds `main.cpp`/the executable.

---

## File Structure

```
engine/
  CMakeLists.txt     # MODIFY (Task 2): add src/uci.cpp + tests/test_uci.cpp; (Task 5): add chess_engine executable
  src/
    search.hpp       # MODIFY (Task 1): SearchLimits + search_timed
    search.cpp       # MODIFY (Task 1): timing state, deadline checks, search_timed
    uci.hpp          # NEW (Task 2): move_from_uci; (Task 3): UciState + handle_command; (Task 4): budget_for_clock
    uci.cpp          # NEW (Task 2): move_from_uci; (Task 3): handle_command; (Task 4): go + time allocation
    main.cpp         # NEW (Task 5): stdin loop
  tests/
    test_search.cpp  # MODIFY (Task 1): timed-search tests
    test_uci.cpp     # NEW (Task 2): move parsing; (Task 3): command handling; (Task 4): go
```

---

## Task 1: Time-limited iterative deepening

**Files:**
- Modify: `engine/src/search.hpp` (add `SearchLimits` + `search_timed`)
- Modify: `engine/src/search.cpp` (add timing state, deadline checks in `negamax`, `search_timed`; reset timing flags in the untimed entries)
- Modify: `engine/tests/test_search.cpp` (add `#include "movegen.hpp"` and three timed-search tests)

**Interfaces:**
- Consumes: everything from Milestone 5, plus `<chrono>`.
- Produces:
  - `struct SearchLimits { int max_depth = 64; long long budget_ms = 0; };`
  - `SearchResult search_timed(Board& b, const SearchLimits& limits);`

**Theory:** A game engine cannot search to a fixed depth: some positions are quiet and some are sharp, and the clock does not care. Iterative deepening already searches depth 1, then 2, and so on, and crucially each pass returns a complete, usable move. That is exactly what a time budget needs: search deeper and deeper until the clock says stop, then play the best move from the last depth that finished. The only new machinery is a deadline and a way to bail out: every couple of thousand nodes we glance at the wall clock, and if the deadline has passed we set a stop flag that unwinds the recursion. The depth that was interrupted is thrown away (its value is unreliable, only some of its moves were searched), so we keep the previous depth's answer. Depth 1 is never interrupted, which guarantees we always have a legal move to return.

- [ ] **Step 1: Write the failing tests**

At the top of `engine/tests/test_search.cpp`, add the movegen include (used by the first new test):
```cpp
#include "movegen.hpp"
```
Then append:
```cpp
TEST_CASE("timed search returns a legal move under a tiny budget") {
    Board b = start_position();
    SearchLimits lim;
    lim.max_depth = 64;
    lim.budget_ms = 5;
    SearchResult r = search_timed(b, lim);
    REQUIRE(r.best.from != NO_SQUARE);
    bool legal = false;
    for (const Move& m : generate_legal(b))
        if (m.from == r.best.from && m.to == r.best.to) legal = true;
    CHECK(legal);
}

TEST_CASE("timed search with a depth cap and no clock matches a fixed-depth search") {
    Board b = start_position();
    SearchLimits lim;
    lim.max_depth = 3;
    lim.budget_ms = 0;   // no time limit: pure depth
    SearchResult r = search_timed(b, lim);
    CHECK(r.score == search_to_depth(b, 3).score);
}

TEST_CASE("timed search finds mate in two with room to think") {
    Board b = board_from_fen("r5k1/5ppp/8/8/8/8/4RPPP/4R1K1 w - - 0 1");
    SearchLimits lim;
    lim.max_depth = 6;
    lim.budget_ms = 2000;
    SearchResult r = search_timed(b, lim);
    CHECK(r.score > 29000);
}
```

- [ ] **Step 2: Run, verify it FAILS to compile**

Run: `cmake --build engine/build`
Expected: compile error, `SearchLimits` / `search_timed` not declared.

- [ ] **Step 3: Add `SearchLimits` + `search_timed` to `search.hpp`**

Append to `engine/src/search.hpp`:
```cpp
// Limits for a timed search. budget_ms == 0 means "no clock, obey max_depth".
struct SearchLimits {
    int max_depth = 64;        // hard depth cap
    long long budget_ms = 0;   // per-move time budget in milliseconds
};

// Iterative deepening under an optional wall-clock budget. Returns the best move
// from the last fully completed depth (depth 1 always completes).
SearchResult search_timed(Board& b, const SearchLimits& limits);
```

- [ ] **Step 4: Rewrite `search.cpp` with timing**

Replace the whole file with:
```cpp
#include "search.hpp"
#include "movegen.hpp"
#include "eval.hpp"
#include <vector>
#include <algorithm>
#include <chrono>

namespace {
const int MATE = 30000;
const int INF  = 31000;
const int MATE_THRESHOLD = MATE - 1000;  // scores past this are forced mates

using Clock = std::chrono::steady_clock;

long g_nodes = 0;              // reset at the top of each public search entry point
bool g_timed = false;         // is the current search time-limited?
bool g_can_stop = false;      // may we abort the current depth? (false during depth 1)
bool g_stop = false;          // set true once the deadline has passed
long g_check_counter = 0;     // node counter for periodic clock checks
Clock::time_point g_deadline;

// Every 2048 nodes, glance at the wall clock and set g_stop if time is up.
inline void maybe_timeout() {
    if (!g_timed || !g_can_stop) return;
    if ((++g_check_counter & 2047) == 0 && Clock::now() >= g_deadline)
        g_stop = true;
}

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
    maybe_timeout();
    if (g_stop) return 0;   // aborted: this value is discarded upstream
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
        if (g_stop) return best;    // bail out fast; result discarded upstream
        if (score > best) best = score;
        if (best > alpha) alpha = best;
        if (alpha >= beta) break;   // beta cutoff: opponent would never allow this node
    }
    return best;
}

// Full-width reference: no cutoffs, no ordering, no timing. Test oracle only.
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
    g_timed = false;
    g_stop = false;
    return negamax_full(b, depth, 0);
}

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
        if (g_stop) break;   // depth incomplete; caller discards this result
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
    g_timed = false;
    g_stop = false;
    SearchResult result;
    result.best = Move{};
    result.score = 0;
    for (int d = 1; d <= max_depth; d++) {
        result = search_to_depth(b, d, result.best);
    }
    return result;
}

SearchResult search_timed(Board& b, const SearchLimits& limits) {
    g_timed = (limits.budget_ms > 0);
    g_stop = false;
    g_deadline = Clock::now() + std::chrono::milliseconds(limits.budget_ms);

    SearchResult best;
    best.best = Move{};
    best.score = 0;

    for (int d = 1; d <= limits.max_depth; d++) {
        g_can_stop = (d > 1);       // always finish depth 1 so we return a legal move
        g_check_counter = 0;
        SearchResult r = search_to_depth(b, d, best.best);
        if (g_stop) break;          // depth d aborted: keep the depth d-1 result
        best = r;
        if (best.best.from == NO_SQUARE) break;                  // no legal move at root
        if (best.score > MATE_THRESHOLD || best.score < -MATE_THRESHOLD) break;  // mate found
        if (g_timed && Clock::now() >= g_deadline) break;        // no time for another depth
    }

    g_timed = false;   // leave timing off so later untimed calls never abort
    g_stop = false;
    return best;
}
```

- [ ] **Step 5: Run, verify it PASSES**

Run: `cmake --build engine/build; ctest --test-dir engine/build --output-on-failure`
Expected: PASS. The tiny-budget search returns a legal move, the depth-capped timed search equals a fixed-depth search, mate in two is found with time to spare, and every Milestone 5 test still passes (the untimed paths reset the timing flags, so they never abort).

- [ ] **Step 6: Commit**

```bash
git add engine/src/search.hpp engine/src/search.cpp engine/tests/test_search.cpp
git commit -m "feat(engine): time-limited iterative deepening"
```

---

## Task 2: UCI move parsing

**Files:**
- Create: `engine/src/uci.hpp`
- Create: `engine/src/uci.cpp`
- Create: `engine/tests/test_uci.cpp`
- Modify: `engine/CMakeLists.txt` (add `src/uci.cpp`, `tests/test_uci.cpp`)

**Interfaces:**
- Consumes: `Board`, `Move`, `MoveFlag`, `PieceType`, `Square`, `make_square`, `NO_SQUARE`, `generate_legal`, `board_from_fen`, `start_position`.
- Produces: `Move move_from_uci(Board& b, const std::string& uci);`

**Theory:** UCI sends moves as text like `e2e4` (from-square, to-square) or `e7e8q` (with a promotion piece). Our `Move` carries more than the text does: a flag for castling / en passant / promotion, which the string never states. Rather than reconstruct those, we generate the legal moves for the position and find the one whose from/to (and promotion piece, when promoting) matches the string. That reuses the move generator as the single source of truth, so a parsed move is always a real legal move or nothing.

- [ ] **Step 1: Write the failing test**

`engine/tests/test_uci.cpp`:
```cpp
#include "doctest.h"
#include "board.hpp"
#include "move.hpp"
#include "uci.hpp"

TEST_CASE("move_from_uci parses a normal move") {
    Board b = start_position();
    Move m = move_from_uci(b, "e2e4");
    CHECK(m.from == make_square(4, 1)); // e2
    CHECK(m.to   == make_square(4, 3)); // e4
}

TEST_CASE("move_from_uci parses a promotion") {
    Board b = board_from_fen("4k3/P7/8/8/8/8/8/4K3 w - - 0 1"); // white pawn a7
    Move m = move_from_uci(b, "a7a8q");
    CHECK(m.from == make_square(0, 6)); // a7
    CHECK(m.to   == make_square(0, 7)); // a8
    CHECK(m.promotion == PieceType::Queen);
}

TEST_CASE("move_from_uci rejects an illegal move") {
    Board b = start_position();
    Move m = move_from_uci(b, "e2e5"); // not a legal first move
    CHECK(m.from == NO_SQUARE);
}
```

- [ ] **Step 2: Register the files, run, verify it FAILS to compile**

In `engine/CMakeLists.txt` add `src/uci.cpp` to `add_library(engine ...)` and `tests/test_uci.cpp` to `add_executable(engine_tests ...)`:
```cmake
add_library(engine
    src/types.cpp
    src/board.cpp
    src/move.cpp
    src/movegen.cpp
    src/perft.cpp
    src/eval.cpp
    src/search.cpp
    src/uci.cpp
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
    tests/test_uci.cpp
)
```
Run:
```
cmake -S engine -B engine/build -G Ninja
cmake --build engine/build
```
Expected: compile error, `uci.hpp: No such file or directory`.

- [ ] **Step 3: Write `uci.hpp`**

`engine/src/uci.hpp`:
```cpp
#pragma once
#include <string>
#include "board.hpp"
#include "move.hpp"

// Parse a UCI move string ("e2e4", "e7e8q") into a legal Move for board b.
// Returns a Move with from == NO_SQUARE if the string matches no legal move.
Move move_from_uci(Board& b, const std::string& uci);
```

- [ ] **Step 4: Write `uci.cpp`**

`engine/src/uci.cpp`:
```cpp
#include "uci.hpp"
#include "movegen.hpp"
#include "types.hpp"

Move move_from_uci(Board& b, const std::string& uci) {
    if (uci.size() < 4) return Move{};
    int ff = uci[0] - 'a', fr = uci[1] - '1';
    int tf = uci[2] - 'a', tr = uci[3] - '1';
    if (ff < 0 || ff > 7 || fr < 0 || fr > 7 ||
        tf < 0 || tf > 7 || tr < 0 || tr > 7) return Move{};
    Square from = make_square(ff, fr);
    Square to   = make_square(tf, tr);

    PieceType promo = PieceType::None;
    if (uci.size() >= 5) {
        switch (uci[4]) {
            case 'q': promo = PieceType::Queen;  break;
            case 'r': promo = PieceType::Rook;   break;
            case 'b': promo = PieceType::Bishop; break;
            case 'n': promo = PieceType::Knight; break;
            default: break;
        }
    }

    for (const Move& m : generate_legal(b)) {
        if (m.from != from || m.to != to) continue;
        if (m.flag == MoveFlag::Promotion) {
            if (m.promotion == promo) return m;   // match the promotion piece
        } else {
            return m;
        }
    }
    return Move{};   // no legal move matched
}
```

- [ ] **Step 5: Run, verify it PASSES**

Run: `cmake --build engine/build; ctest --test-dir engine/build --output-on-failure`
Expected: PASS. `e2e4` and `a7a8q` parse to the right legal moves; `e2e5` returns a null move.

- [ ] **Step 6: Commit**

```bash
git add engine/CMakeLists.txt engine/src/uci.hpp engine/src/uci.cpp engine/tests/test_uci.cpp
git commit -m "feat(engine): parse UCI move strings against legal moves"
```

---

## Task 3: UCI command handling (state, position, moves)

**Files:**
- Modify: `engine/src/uci.hpp` (add `UciState` + `handle_command`)
- Modify: `engine/src/uci.cpp` (add `handle_command` and a line splitter)
- Modify: `engine/tests/test_uci.cpp` (add command-handling tests)

**Interfaces:**
- Consumes: `move_from_uci`, `make_move`, `board_from_fen`, `fen_from_board`, `start_position`.
- Produces:
  - `struct UciState { Board board = start_position(); };`
  - `std::string handle_command(UciState& state, const std::string& line);`

**Theory:** UCI is a line protocol: the GUI sends one command per line, the engine prints its reply. Keeping the logic in a pure `handle_command(state, line) -> string` (rather than reading stdin directly) makes the whole protocol unit-testable, feed a line, check the string. `uci` announces the engine and ends with `uciok`; `isready` answers `readyok` (a sync point); `ucinewgame` resets the position; `position` sets the board from `startpos` or a FEN and then replays the listed moves with `make_move`, leaving `state.board` at the current game position. `go` is stubbed here and filled in next task.

- [ ] **Step 1: Write the failing test**

Append to `engine/tests/test_uci.cpp`:
```cpp
TEST_CASE("uci command replies with id and uciok") {
    UciState s;
    std::string r = handle_command(s, "uci");
    CHECK(r.find("id name") != std::string::npos);
    CHECK(r.find("uciok")   != std::string::npos);
}

TEST_CASE("isready replies readyok") {
    UciState s;
    CHECK(handle_command(s, "isready") == "readyok");
}

TEST_CASE("position startpos with moves updates the board") {
    UciState s;
    handle_command(s, "position startpos moves e2e4 e7e5");
    std::string fen = fen_from_board(s.board);
    // Piece placement after 1.e4 e5 (ignore the fields after the first space).
    CHECK(fen.rfind("rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR", 0) == 0);
}

TEST_CASE("position fen sets the board") {
    UciState s;
    handle_command(s, "position fen 4k3/8/8/8/8/8/8/4K3 w - - 0 1");
    std::string fen = fen_from_board(s.board);
    CHECK(fen.rfind("4k3/8/8/8/8/8/8/4K3", 0) == 0);
}
```
Note: `test_uci.cpp` already includes `board.hpp` (for `fen_from_board`) from Task 2.

- [ ] **Step 2: Run, verify it FAILS to compile**

Run: `cmake --build engine/build`
Expected: compile error, `UciState` / `handle_command` not declared.

- [ ] **Step 3: Add `UciState` + `handle_command` to `uci.hpp`**

Append to `engine/src/uci.hpp`:
```cpp
// Engine state carried across UCI commands (the current game position).
struct UciState {
    Board board = start_position();
};

// Handle one UCI input line; returns the text to print (possibly empty or
// multiple lines, with no trailing newline). Updates state for position /
// ucinewgame. Unknown commands return "".
std::string handle_command(UciState& state, const std::string& line);
```

- [ ] **Step 4: Add `handle_command` to `uci.cpp`**

Add these includes near the top of `engine/src/uci.cpp` (after the existing includes):
```cpp
#include <sstream>
#include <vector>
```
Append to `engine/src/uci.cpp`:
```cpp
namespace {
std::vector<std::string> split_ws(const std::string& s) {
    std::istringstream iss(s);
    std::vector<std::string> out;
    std::string t;
    while (iss >> t) out.push_back(t);
    return out;
}
} // namespace

std::string handle_command(UciState& state, const std::string& line) {
    std::vector<std::string> tok = split_ws(line);
    if (tok.empty()) return "";
    const std::string& cmd = tok[0];

    if (cmd == "uci")
        return "id name ChessCoach\nid author Aayush Pokhrel\nuciok";
    if (cmd == "isready")
        return "readyok";
    if (cmd == "ucinewgame") {
        state.board = start_position();
        return "";
    }
    if (cmd == "quit")
        return "";

    if (cmd == "position") {
        size_t i = 1;
        if (i < tok.size() && tok[i] == "startpos") {
            state.board = start_position();
            i++;
        } else if (i < tok.size() && tok[i] == "fen") {
            i++;
            std::string fen;
            for (int f = 0; f < 6 && i < tok.size(); f++, i++) {
                if (f) fen += " ";
                fen += tok[i];
            }
            state.board = board_from_fen(fen);
        }
        if (i < tok.size() && tok[i] == "moves") {
            i++;
            for (; i < tok.size(); i++) {
                Move m = move_from_uci(state.board, tok[i]);
                if (m.from != NO_SQUARE) make_move(state.board, m);
            }
        }
        return "";
    }

    if (cmd == "go")
        return "";   // implemented in Task 4

    return "";       // ignore unknown commands
}
```

- [ ] **Step 5: Run, verify it PASSES**

Run: `cmake --build engine/build; ctest --test-dir engine/build --output-on-failure`
Expected: PASS. `uci` returns id + uciok, `isready` returns readyok, and both `position` forms leave the board where expected.

- [ ] **Step 6: Commit**

```bash
git add engine/src/uci.hpp engine/src/uci.cpp engine/tests/test_uci.cpp
git commit -m "feat(engine): UCI command handling for position and moves"
```

---

## Task 4: `go` with time allocation

**Files:**
- Modify: `engine/src/uci.hpp` (expose `budget_for_clock`)
- Modify: `engine/src/uci.cpp` (add `budget_for_clock`, `compute_limits`, `handle_go`; wire the `go` branch)
- Modify: `engine/tests/test_uci.cpp` (add go + allocation tests)

**Interfaces:**
- Consumes: `SearchLimits`, `search_timed`, `SearchResult`, `to_uci`, `Color`.
- Produces:
  - `long long budget_for_clock(long long remaining_ms, long long inc_ms);` (per-move budget from the clock)
  - `go` now returns `bestmove <uci>` (or `bestmove 0000` when there is no legal move).

**Theory:** `go` carries the time context. `movetime` fixes a budget directly; `depth` fixes a depth and ignores the clock; `wtime/btime/winc/binc` give the remaining clock and increment, from which we carve a per-move slice. The slice is a heuristic: a fraction of the remaining time plus part of the increment, never more than "almost all of the clock." Whatever the source, it becomes a `SearchLimits`, `search_timed` runs, and we print `bestmove` in UCI text. Pulling the pure clock-to-budget arithmetic into `budget_for_clock` lets us test the allocation directly, without a clock.

- [ ] **Step 1: Write the failing test**

Append to `engine/tests/test_uci.cpp`:
```cpp
TEST_CASE("budget_for_clock splits the clock sanely") {
    CHECK(budget_for_clock(600000, 0) == 30000); // 10 min -> ~30s
    CHECK(budget_for_clock(60000, 1000) == 3500);  // 60s + 1s inc -> 3000 + 500
    CHECK(budget_for_clock(20, 0) >= 1);           // tiny clock still yields >= 1ms
}

TEST_CASE("go depth returns a legal bestmove") {
    UciState s;
    handle_command(s, "position startpos");
    std::string r = handle_command(s, "go depth 2");
    REQUIRE(r.rfind("bestmove ", 0) == 0);
    std::string mv = r.substr(9);
    Move m = move_from_uci(s.board, mv);
    CHECK(m.from != NO_SQUARE);   // the printed move is legal
}

TEST_CASE("go movetime returns a bestmove quickly") {
    UciState s;
    handle_command(s, "position startpos");
    std::string r = handle_command(s, "go movetime 50");
    CHECK(r.rfind("bestmove ", 0) == 0);
}
```

- [ ] **Step 2: Run, verify it FAILS to compile**

Run: `cmake --build engine/build`
Expected: compile error, `budget_for_clock` not declared.

- [ ] **Step 3: Expose `budget_for_clock` in `uci.hpp`**

Append to `engine/src/uci.hpp`:
```cpp
// Per-move time budget (ms) from the remaining clock and increment for the side
// to move. Exposed for testing.
long long budget_for_clock(long long remaining_ms, long long inc_ms);
```

- [ ] **Step 4: Implement allocation + `go` in `uci.cpp`**

Add these includes near the top of `engine/src/uci.cpp` (after the existing includes):
```cpp
#include "search.hpp"
#include <cstdlib>
```
Add the public allocator (outside any namespace) anywhere above `handle_command`:
```cpp
long long budget_for_clock(long long remaining_ms, long long inc_ms) {
    // ponytail: crude 1/20th-of-clock split plus half the increment; tune only
    //           if the engine flags or dawdles in real games.
    long long budget = remaining_ms / 20 + inc_ms / 2;
    long long cap = remaining_ms - 30;   // keep a safety margin, never spend it all
    if (cap < 1) cap = 1;
    if (budget > cap) budget = cap;
    if (budget < 1) budget = 1;
    return budget;
}
```
Add these file-local helpers inside the existing anonymous namespace in `uci.cpp` (next to `split_ws`):
```cpp
SearchLimits compute_limits(const Board& b, const std::vector<std::string>& tok) {
    SearchLimits lim;
    long long movetime = 0, wtime = 0, btime = 0, winc = 0, binc = 0;
    int depth = 0;
    bool infinite = false;
    for (size_t i = 1; i < tok.size(); i++) {
        if (tok[i] == "infinite") { infinite = true; continue; }
        if (i + 1 >= tok.size()) continue;
        const std::string& v = tok[i + 1];
        if      (tok[i] == "movetime") movetime = std::atoll(v.c_str());
        else if (tok[i] == "wtime")    wtime    = std::atoll(v.c_str());
        else if (tok[i] == "btime")    btime    = std::atoll(v.c_str());
        else if (tok[i] == "winc")     winc     = std::atoll(v.c_str());
        else if (tok[i] == "binc")     binc     = std::atoll(v.c_str());
        else if (tok[i] == "depth")    depth    = std::atoi(v.c_str());
    }

    if (depth > 0) { lim.max_depth = depth; lim.budget_ms = 0; return lim; } // fixed depth
    if (infinite)  { lim.max_depth = 64;    lim.budget_ms = 0; return lim; } // depth cap only
    if (movetime > 0) {
        lim.budget_ms = movetime > 10 ? movetime - 5 : movetime;  // small safety margin
        return lim;
    }
    long long remaining = (b.side_to_move == Color::White) ? wtime : btime;
    long long inc       = (b.side_to_move == Color::White) ? winc  : binc;
    if (remaining > 0) { lim.budget_ms = budget_for_clock(remaining, inc); return lim; }

    lim.max_depth = 5;   // nothing specified: a safe default depth
    lim.budget_ms = 0;
    return lim;
}

std::string handle_go(UciState& state, const std::vector<std::string>& tok) {
    SearchLimits lim = compute_limits(state.board, tok);
    SearchResult r = search_timed(state.board, lim);
    if (r.best.from == NO_SQUARE) return "bestmove 0000";   // no legal move
    return "bestmove " + to_uci(r.best);
}
```
Then change the `go` branch in `handle_command` from:
```cpp
    if (cmd == "go")
        return "";   // implemented in Task 4
```
to:
```cpp
    if (cmd == "go")
        return handle_go(state, tok);
```

- [ ] **Step 5: Run, verify it PASSES**

Run: `cmake --build engine/build; ctest --test-dir engine/build --output-on-failure`
Expected: PASS. The allocator splits clocks as asserted, `go depth 2` prints a legal `bestmove`, and `go movetime 50` returns a `bestmove` line.

- [ ] **Step 6: Commit**

```bash
git add engine/src/uci.hpp engine/src/uci.cpp engine/tests/test_uci.cpp
git commit -m "feat(engine): go with time allocation and bestmove output"
```

---

## Task 5: `main.cpp` stdin loop + executable

**Files:**
- Create: `engine/src/main.cpp`
- Modify: `engine/CMakeLists.txt` (add the `chess_engine` executable)

**Interfaces:**
- Consumes: `UciState`, `handle_command`.
- Produces: a `chess_engine` executable that a GUI can launch.

**Theory:** Everything the protocol does is already tested through `handle_command`. `main` is only the loop that connects it to real stdin/stdout: read a line, hand it to `handle_command`, print any non-empty reply, and flush (`std::endl`) so the GUI sees it immediately. `quit` ends the loop. Because the loop is trivial glue over fully tested functions, its check is a manual smoke run rather than a unit test.

- [ ] **Step 1: Write `main.cpp`**

`engine/src/main.cpp`:
```cpp
#include <iostream>
#include <string>
#include "uci.hpp"

int main() {
    UciState state;
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line == "quit") break;
        std::string out = handle_command(state, line);
        if (!out.empty())
            std::cout << out << std::endl;   // endl flushes so the GUI sees it
    }
    return 0;
}
```

- [ ] **Step 2: Add the executable to `engine/CMakeLists.txt`**

After the `engine_tests` target block, add:
```cmake
# The UCI engine executable a chess GUI launches.
add_executable(chess_engine src/main.cpp)
target_link_libraries(chess_engine PRIVATE engine)
target_include_directories(chess_engine PRIVATE src)
# Same static-link fix as the test exe (avoid loading an older libstdc++ on PATH).
target_link_options(chess_engine PRIVATE -static)
```

- [ ] **Step 3: Reconfigure + build**

Run:
```
cmake -S engine -B engine/build -G Ninja
cmake --build engine/build
```
Expected: builds `chess_engine.exe` with no errors.

- [ ] **Step 4: Smoke-test the protocol end to end (manual)**

Run (PowerShell):
```
"uci`nisready`nposition startpos moves e2e4`ngo depth 3`nquit" | engine/build/chess_engine.exe
```
Expected output contains `id name ChessCoach`, `uciok`, `readyok`, and a `bestmove ` line (a legal Black reply to 1.e4). Confirm the full test suite is still green:
```
ctest --test-dir engine/build --output-on-failure
```

- [ ] **Step 5: Commit**

```bash
git add engine/CMakeLists.txt engine/src/main.cpp
git commit -m "feat(engine): UCI stdin loop and chess_engine executable"
```

---

## Self-Review Notes

- **Spec coverage:** Implements spec §4 Track A step 6 (UCI) and §3 (the UCI seam). Success criterion "plays a full game vs Stockfish via a GUI" is met by: a launchable `chess_engine` executable (Task 5), the required handshake and `position`/`go`/`bestmove` handling (Tasks 3 to 4), legal move application and output (Tasks 2, 4), and time management so the engine does not flag in real games (Task 1, 4). The manual smoke run in Task 5 plus loading the exe in a GUI (Arena / CuteChess) is the end-to-end confirmation.
- **Type consistency:** `search_timed(Board&, const SearchLimits&) -> SearchResult`, `move_from_uci(Board&, const std::string&) -> Move`, `handle_command(UciState&, const std::string&) -> std::string`, `budget_for_clock(long long, long long) -> long long` are declared in headers and used identically in `.cpp` and tests. `SearchLimits { int max_depth; long long budget_ms; }` and `UciState { Board board; }` field names match every use.
- **Placeholders:** the Task 3 `go` branch returns `""` as a deliberate, labeled stub that Task 4 replaces; no other placeholders. All code is complete and compilable.
- **Timing safety:** timing flags (`g_timed`, `g_stop`) are reset off by `search`, `search_minimax`, and at the end of `search_timed`, so no untimed call (including the Milestone 5 tests that call `search`/`search_to_depth` directly) can inherit a stale deadline and abort. Depth 1 is never aborted (`g_can_stop = (d > 1)`), guaranteeing a legal move whenever one exists.
- **Deliberate limits (ponytail):** the clock split is a crude heuristic (marked in code); no pondering, no `setoption`, no `stop`/`ponderhit` handling, no move-overhead tuning. All are real UCI features but not needed to play a full timed game, and they slot on top of this structure later.

## What comes next (preview, not part of this plan)

With the engine speaking UCI, Track B (the coach) can drive it at the same socket it uses for Stockfish, and the two can analyze the same position side by side. Engine step 7 (bitboards, transposition table, WASM build) is the performance deep-dive after correctness is proven end to end.
