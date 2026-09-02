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

TEST_CASE("a knight is worth more in the center than in the corner") {
    // Same material (lone white knight + kings); only the knight's square differs.
    Board center = board_from_fen("4k3/8/8/8/3N4/8/8/4K3 w - - 0 1"); // Nd4
    Board corner = board_from_fen("4k3/8/8/8/8/8/8/N3K3 w - - 0 1");   // Na1
    CHECK(evaluate(center) > evaluate(corner));
}

TEST_CASE("piece-square tables keep the start position balanced") {
    CHECK(evaluate(start_position()) == 0);
}

TEST_CASE("piece-square tables move the score off pure material") {
    // An advanced, centralized white pawn should read higher than its raw 100.
    Board b = board_from_fen("4k3/8/8/3P4/8/8/8/4K3 w - - 0 1"); // white pawn d5
    CHECK(evaluate(b) != material_score(b));
}
