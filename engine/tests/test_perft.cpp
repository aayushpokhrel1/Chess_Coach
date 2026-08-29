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