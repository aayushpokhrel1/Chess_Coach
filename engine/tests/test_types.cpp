#include "doctest.h"
#include "types.hpp"

TEST_CASE("square coordinate math") {
    CHECK(make_square(0, 0) == 0);   // a1
    CHECK(make_square(7, 0) == 7);   // h1
    CHECK(make_square(0, 7) == 56);  // a8
    CHECK(make_square(4, 3) == 28);  // e4
    CHECK(file_of(28) == 4);
    CHECK(rank_of(28) == 3);
}

TEST_CASE("piece <-> FEN char round trip") {
    CHECK(piece_from_char('K').color == Color::White);
    CHECK(char_from_piece(Piece{Color::White, PieceType::King}) == 'K');
    CHECK(char_from_piece(Piece{Color::Black, PieceType::Pawn}) == 'p');
    CHECK(char_from_piece(Piece{Color::None,  PieceType::None}) == '.');

    Piece wn = piece_from_char('N');
    CHECK(wn.color == Color::White);
    CHECK(wn.type == PieceType::Knight);

    Piece bq = piece_from_char('q');
    CHECK(bq.color == Color::Black);
    CHECK(bq.type == PieceType::Queen);
}
