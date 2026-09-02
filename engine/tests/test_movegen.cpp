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

static int moves_from(const std::vector<Move>& list, Square from) {
    return std::count_if(list.begin(), list.end(), [&](const Move& m){
        return m.from == from;
    });
}

TEST_CASE("pawn on start rank can push one or two") {
    Board b = board_from_fen("4k3/8/8/8/8/8/4P3/4K3 w - - 0 1"); // Pe2
    auto moves = generate_pseudo_legal(b);
    CHECK(has_move(moves, make_square(4,1), make_square(4,2))); // e3
    CHECK(has_move(moves, make_square(4,1), make_square(4,3))); // e4
    CHECK(moves_from(moves, make_square(4,1)) == 2);
}

TEST_CASE("pawn captures diagonally and cannot capture straight") {
    // White Pe4; black pawns d5 and f5.
    Board b = board_from_fen("4k3/8/8/3p1p2/4P3/8/8/4K3 w - - 0 1");
    auto moves = generate_pseudo_legal(b);
    CHECK(has_move(moves, make_square(4,3), make_square(3,4))); // exd5
    CHECK(has_move(moves, make_square(4,3), make_square(5,4))); // exf5
    CHECK(has_move(moves, make_square(4,3), make_square(4,4))); // e5 push
    CHECK(moves_from(moves, make_square(4,3)) == 3);
}

TEST_CASE("en passant is emitted with the right flag") {
    Board b = board_from_fen("4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 1"); // Pe5, ep target d6
    auto moves = generate_pseudo_legal(b);
    auto it = std::find_if(moves.begin(), moves.end(), [](const Move& m){
        return m.from == make_square(4,4) && m.to == make_square(3,5);
    });
    REQUIRE(it != moves.end());
    CHECK(it->flag == MoveFlag::EnPassant);
}

TEST_CASE("promotion expands into four moves") {
    Board b = board_from_fen("4k3/P7/8/8/8/8/8/4K3 w - - 0 1"); // Pa7
    auto moves = generate_pseudo_legal(b);
    CHECK(moves_from(moves, make_square(0,6)) == 4); // a8=Q,R,B,N
    CHECK(std::any_of(moves.begin(), moves.end(), [](const Move& m){
        return m.from == make_square(0,6) && m.flag == MoveFlag::Promotion
            && m.promotion == PieceType::Queen;
    }));
}

TEST_CASE("both castles available on an empty back rank") {
    Board b = board_from_fen("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");
    auto moves = generate_pseudo_legal(b);
    CHECK(has_move(moves, make_square(4,0), make_square(6,0))); // O-O
    CHECK(has_move(moves, make_square(4,0), make_square(2,0))); // O-O-O
}

TEST_CASE("cannot castle through an attacked square") {
    // Black rook on f8 attacks f1; kingside castling passes through f1.
    Board b = board_from_fen("r3kr2/8/8/8/8/8/8/R3K2R w KQq - 0 1");
    auto moves = generate_pseudo_legal(b);
    CHECK_FALSE(has_move(moves, make_square(4,0), make_square(6,0))); // O-O blocked
}

TEST_CASE("cannot castle without the right") {
    Board b = board_from_fen("r3k2r/8/8/8/8/8/8/R3K2R w - - 0 1"); // no rights
    auto moves = generate_pseudo_legal(b);
    CHECK_FALSE(has_move(moves, make_square(4,0), make_square(6,0)));
    CHECK_FALSE(has_move(moves, make_square(4,0), make_square(2,0)));
}

TEST_CASE("cannot castle out of check") {
    // Black rook e8 gives check down the e-file; no castling while in check.
    Board b = board_from_fen("4r3/8/8/8/8/8/8/R3K2R w KQ - 0 1");
    auto moves = generate_pseudo_legal(b);
    CHECK_FALSE(has_move(moves, make_square(4,0), make_square(6,0)));
    CHECK_FALSE(has_move(moves, make_square(4,0), make_square(2,0)));
}

TEST_CASE("a pinned piece cannot move off the pin") {
    // White king e1, white knight e2, black rook e8: the knight is pinned.
    Board b = board_from_fen("4r3/8/8/8/8/8/4N3/4K3 w - - 0 1");
    auto legal = generate_legal(b);
    // Any knight move exposes the king, so none is legal.
    CHECK(moves_from(legal, make_square(4,1)) == 0);
    // The king may still step aside off the e-file.
    CHECK(has_move(legal, make_square(4,0), make_square(3,0))); // Kd1
}

TEST_CASE("in check, only moves that resolve the check are legal") {
    // Black rook e8 checks white Ke1; white also has a rook a1 that cannot help.
    Board b = board_from_fen("4r3/8/8/8/8/8/8/R3K3 w - - 0 1");
    auto legal = generate_legal(b);
    // Every legal move must leave the white king safe.
    for (const Move& m : legal) {
        Board nb = b;
        make_move(nb, m);
        CHECK_FALSE(in_check(nb, Color::White));
    }
    // The only escapes are king steps off the e-file (Ra1 cannot reach or block e-file).
    CHECK(has_move(legal, make_square(4,0), make_square(3,0))); // Kd1
    CHECK(has_move(legal, make_square(4,0), make_square(5,0))); // Kf1
    CHECK(generate_legal(b).size() == 4); // Kd1, Kf1, Kd2, Kf2
}

TEST_CASE("start position has exactly 20 legal moves") {
    Board b = start_position();
    CHECK(generate_legal(b).size() == 20);
}
