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
