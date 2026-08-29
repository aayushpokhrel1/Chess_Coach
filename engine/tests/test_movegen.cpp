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

#include <algorithm>

// True if `list` contains a move from->to (ignoring flag/promotion).
static bool has_move(const std::vector<Move>& list, Square from, Square to) {
    return std::any_of(list.begin(), list.end(), [&](const Move& m){
        return m.from == from && m.to == to;
    });
}

TEST_CASE("knight in the corner has two moves") {
    Board b = board_from_fen("4k3/8/8/8/8/8/8/N3K3 w - - 0 1"); // Na1, Ke1
    auto moves = generate_pseudo_legal(b);
    CHECK(has_move(moves, make_square(0,0), make_square(1,2))); // Nb3
    CHECK(has_move(moves, make_square(0,0), make_square(2,1))); // Nc2
    // Knight has exactly 2; total also includes the king's moves.
    int knight_moves = std::count_if(moves.begin(), moves.end(), [](const Move& m){
        return m.from == make_square(0,0);
    });
    CHECK(knight_moves == 2);
}

TEST_CASE("king moves off its start square, not onto friendly pieces") {
    Board b = board_from_fen("4k3/8/8/8/8/8/8/4K3 w - - 0 1"); // lone Ke1
    auto moves = generate_pseudo_legal(b);
    int king_moves = std::count_if(moves.begin(), moves.end(), [](const Move& m){
        return m.from == make_square(4,0);
    });
    CHECK(king_moves == 5); // d1,f1,d2,e2,f2
}

TEST_CASE("knight does not capture its own pieces") {
    // White knight b1 with pawns on d2; only a3, c3, d2(own->blocked) ...
    Board b = board_from_fen("4k3/8/8/8/8/8/3P4/1N2K3 w - - 0 1");
    auto moves = generate_pseudo_legal(b);
    CHECK_FALSE(has_move(moves, make_square(1,0), make_square(3,1))); // Nxd2 own pawn: illegal
    CHECK(has_move(moves, make_square(1,0), make_square(0,2)));       // Na3
    CHECK(has_move(moves, make_square(1,0), make_square(2,2)));       // Nc3
}

TEST_CASE("rook on an open board has 14 moves") {
    Board b = board_from_fen("4k3/8/8/8/3R4/8/8/4K3 w - - 0 1"); // Rd4
    auto moves = generate_pseudo_legal(b);
    int rook = std::count_if(moves.begin(), moves.end(), [](const Move& m){
        return m.from == make_square(3,3);
    });
    CHECK(rook == 14); // 7 along the file + 7 along the rank
}

TEST_CASE("bishop is stopped by and can capture a blocker") {
    // Bishop c1; white pawn e3 blocks one diagonal, black pawn a3 is capturable.
    Board b = board_from_fen("4k3/8/8/8/8/p3P3/8/2B1K3 w - - 0 1");
    auto moves = generate_pseudo_legal(b);
    CHECK(has_move(moves, make_square(2,0), make_square(1,1))); // Bb2
    CHECK(has_move(moves, make_square(2,0), make_square(0,2))); // Bxa3 (capture)
    CHECK_FALSE(has_move(moves, make_square(2,0), make_square(5,3))); // past own e3 pawn
    CHECK_FALSE(has_move(moves, make_square(2,0), make_square(4,2))); // onto own e3 pawn
}

TEST_CASE("queen combines rook and bishop rays") {
    Board b = board_from_fen("4k3/8/8/8/3Q4/8/8/4K3 w - - 0 1"); // Qd4
    auto moves = generate_pseudo_legal(b);
    int q = std::count_if(moves.begin(), moves.end(), [](const Move& m){
        return m.from == make_square(3,3);
    });
    CHECK(q == 27); // 14 rook-like + 13 bishop-like from d4
}
