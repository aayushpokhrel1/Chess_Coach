#include "doctest.h"
#include "zobrist.hpp"
#include "board.hpp"
#include "move.hpp"
#include "uci.hpp"
#include <initializer_list>

// Play a list of UCI moves onto b (ignores the Undo; we never unmake here).
static void play(Board& b, std::initializer_list<const char*> ucis) {
    for (const char* u : ucis) {
        Move m = move_from_uci(b, u);
        REQUIRE(m.from != NO_SQUARE);   // the test line must be legal
        make_move(b, m);
    }
}

TEST_CASE("zobrist: a transposition hashes to the same key") {
    // Two move orders reaching the identical position. Both end on Nc6 (a knight
    // move) so the en-passant state matches (no dangling double-push square).
    Board a = start_position();
    play(a, {"e2e4", "e7e5", "g1f3", "b8c6"});
    Board c = start_position();
    play(c, {"g1f3", "e7e5", "e2e4", "b8c6"});
    CHECK(compute_hash(a) == compute_hash(c));
}

TEST_CASE("zobrist: different positions hash to different keys") {
    Board s = start_position();
    Board a = start_position();
    play(a, {"e2e4"});
    CHECK(compute_hash(s) != compute_hash(a));
}

TEST_CASE("zobrist: same board hashes the same (deterministic)") {
    CHECK(compute_hash(start_position()) == compute_hash(start_position()));
}

TEST_CASE("zobrist: en passant and side-to-move change the key") {
    // Same pieces, but one has an en-passant square live and it is a different side
    // to move: the key must reflect both.
    Board after_e4 = start_position();
    play(after_e4, {"e2e4"});                 // Black to move, ep = e3
    Board after_e4_a6 = after_e4;
    play(after_e4_a6, {"a7a6"});              // White to move, ep cleared
    CHECK(compute_hash(after_e4) != compute_hash(after_e4_a6));
}
