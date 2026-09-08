#include "doctest.h"
#include "zobrist.hpp"
#include "board.hpp"
#include "move.hpp"
#include "movegen.hpp"
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

// Walk every line to `depth`, checking at each node that the incrementally-maintained
// b.hash equals a fresh compute_hash, both after make and after unmake. compute_hash is
// the independent oracle: any term a move forgets to XOR (a capture, a castling right, an
// en-passant file, a promotion) makes the two diverge here. Accumulates one bool and bails
// on the first mismatch so it stays cheap even over perft-sized trees.
static bool hash_in_sync = true;
static void hash_walk(Board& b, int depth) {
    if (b.hash != compute_hash(b)) { hash_in_sync = false; return; }
    if (depth == 0) return;
    for (const Move& m : generate_legal(b)) {
        Undo u = make_move(b, m);
        if (b.hash != compute_hash(b)) { hash_in_sync = false; unmake_move(b, m, u); return; }
        hash_walk(b, depth - 1);
        unmake_move(b, m, u);
        if (b.hash != compute_hash(b)) { hash_in_sync = false; return; }
        if (!hash_in_sync) return;
    }
}

TEST_CASE("zobrist: the incremental key matches a full recompute over a move tree") {
    hash_in_sync = true;
    Board start = start_position();
    hash_walk(start, 4);
    CHECK(hash_in_sync);

    // Kiwipete: castling both sides, en passant, and promotions all live, so it
    // exercises every non-piece term the incremental update has to get right.
    Board kiwi = board_from_fen(
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
    hash_walk(kiwi, 3);
    CHECK(hash_in_sync);
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
