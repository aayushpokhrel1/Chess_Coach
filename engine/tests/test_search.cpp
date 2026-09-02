#include "doctest.h"
#include "board.hpp"
#include "move.hpp"
#include "search.hpp"

TEST_CASE("search grabs a free queen") {
    // White rook on d1 can capture an undefended black queen on d3.
    Board b = board_from_fen("4k3/8/8/8/8/3q4/8/3RK3 w - - 0 1");
    SearchResult r = search(b, 2);
    CHECK(r.best.from == make_square(3, 0)); // d1
    CHECK(r.best.to   == make_square(3, 2)); // d3
    CHECK(r.score > 400);                    // clearly ahead (a rook up after being down)
}

TEST_CASE("search finds mate in one") {
    // White: rook on h1, rook on g7 holds rank 7; Rh1-h8 is mate.
    Board b = board_from_fen("k7/6R1/8/8/8/8/8/K6R w - - 0 1");
    SearchResult r = search(b, 2);
    CHECK(r.best.from == make_square(7, 0)); // h1
    CHECK(r.best.to   == make_square(7, 7)); // h8
    CHECK(r.score > 29000);                  // a mate score
}

TEST_CASE("stalemate scores zero and reports no move") {
    // Black to move, king h8 has no legal move and is not in check.
    Board b = board_from_fen("7k/5Q2/6K1/8/8/8/8/8 b - - 0 1");
    SearchResult r = search(b, 3);
    CHECK(r.score == 0);
    CHECK(r.best.from == NO_SQUARE);
}

TEST_CASE("alpha-beta returns the same value as full-width minimax, with fewer nodes") {
    // A busy midgame position (after 1.e4 e5) so pruning has something to cut.
    Board b = board_from_fen("rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2");

    int full = search_minimax(b, 3);
    long full_nodes = nodes_searched();

    int pruned = search(b, 3).score;
    long pruned_nodes = nodes_searched();

    CHECK(pruned == full);              // pruning must not change the value
    CHECK(pruned_nodes < full_nodes);   // but it must visit fewer nodes
}
