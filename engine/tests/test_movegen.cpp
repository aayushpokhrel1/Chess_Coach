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
