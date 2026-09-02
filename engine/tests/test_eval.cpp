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
