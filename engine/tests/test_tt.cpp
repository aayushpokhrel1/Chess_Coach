#include "doctest.h"
#include "tt.hpp"
#include "move.hpp"

static Move mv(Square f, Square t) {
    Move m;
    m.from = f;
    m.to = t;
    return m;
}

TEST_CASE("tt: store then probe round-trips the score and move") {
    tt_clear();
    tt_store(0xABCD, 5, 0, 123, TTFlag::Exact, mv(12, 28));
    int score = 0;
    Move m;
    CHECK(tt_probe(0xABCD, 5, 0, -1000, 1000, score, m));
    CHECK(score == 123);
    CHECK(m.from == 12);
    CHECK(m.to == 28);
}

TEST_CASE("tt: a shallower stored depth does not cut a deeper probe, but still gives the move") {
    tt_clear();
    tt_store(0x1, 2, 0, 50, TTFlag::Exact, mv(1, 2));
    int score = -999;
    Move m;
    CHECK_FALSE(tt_probe(0x1, 4, 0, -1000, 1000, score, m)); // want depth 4, only stored depth 2
    CHECK(m.from == 1);     // move still returned, for ordering
    CHECK(score == -999);   // score left untouched
}

TEST_CASE("tt: a key mismatch misses (guards against collisions)") {
    tt_clear();
    tt_store(0x1, 5, 0, 50, TTFlag::Exact, mv(1, 2));
    int score = 0;
    Move m;
    CHECK_FALSE(tt_probe(0x2, 1, 0, -1000, 1000, score, m));
}

TEST_CASE("tt: a lower bound cuts only when its score >= beta") {
    tt_clear();
    tt_store(0x7, 5, 0, 200, TTFlag::Lower, mv(0, 1));
    int score = 0;
    Move m;
    CHECK_FALSE(tt_probe(0x7, 5, 0, -1000, 300, score, m)); // 200 < beta 300: not a cutoff
    CHECK(tt_probe(0x7, 5, 0, -1000, 150, score, m));       // 200 >= beta 150: cutoff
    CHECK(score == 200);
}

TEST_CASE("tt: an upper bound cuts only when its score <= alpha") {
    tt_clear();
    tt_store(0x8, 5, 0, 100, TTFlag::Upper, mv(0, 1));
    int score = 0;
    Move m;
    CHECK_FALSE(tt_probe(0x8, 5, 0, 50, 1000, score, m));   // 100 > alpha 50: not a cutoff
    CHECK(tt_probe(0x8, 5, 0, 150, 1000, score, m));        // 100 <= alpha 150: cutoff
    CHECK(score == 100);
}

TEST_CASE("tt: mate scores keep their distance-from-here across plies") {
    tt_clear();
    const int MATE = 30000;
    tt_store(0x9, 5, 4, MATE - 6, TTFlag::Exact, mv(0, 1)); // mate stored at ply 4
    int score = 0;
    Move m;
    // Same ply back out: the original score.
    CHECK(tt_probe(0x9, 5, 4, -31000, 31000, score, m));
    CHECK(score == MATE - 6);
    // A shallower occurrence (ply 2) of the same position: same distance from the
    // node (2 plies), so the score reads MATE - 4, not MATE - 6.
    CHECK(tt_probe(0x9, 5, 2, -31000, 31000, score, m));
    CHECK(score == MATE - 4);
}
