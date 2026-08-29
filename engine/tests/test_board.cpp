#include "doctest.h"
#include "board.hpp"

static const char* START_FEN =
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

TEST_CASE("parse start position from FEN") {
    Board b = board_from_fen(START_FEN);

    CHECK(b.squares[make_square(0, 0)].type == PieceType::Rook);   // a1
    CHECK(b.squares[make_square(0, 0)].color == Color::White);
    CHECK(b.squares[make_square(4, 7)].type == PieceType::King);   // e8
    CHECK(b.squares[make_square(4, 7)].color == Color::Black);
    CHECK(b.squares[make_square(4, 3)].type == PieceType::None);   // e4 empty

    CHECK(b.side_to_move == Color::White);
    CHECK(b.castling_rights == (CASTLE_WK | CASTLE_WQ | CASTLE_BK | CASTLE_BQ));
    CHECK(b.en_passant == NO_SQUARE);
    CHECK(b.halfmove_clock == 0);
    CHECK(b.fullmove_number == 1);
}

TEST_CASE("parse en passant and side to move") {
    Board b = board_from_fen(
        "rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR w KQkq c6 0 2");
    CHECK(b.side_to_move == Color::White);
    CHECK(b.en_passant == make_square(2, 5)); // c6
    CHECK(b.fullmove_number == 2);
}
