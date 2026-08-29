#pragma once

enum class Color { White, Black, None };

enum class PieceType { Pawn, Knight, Bishop, Rook, Queen, King, None };

struct Piece {
    Color color = Color::None;
    PieceType type = PieceType::None;
};

using Square = int;
constexpr Square NO_SQUARE = -1;

// file 0..7 = a..h, rank 0..7 = ranks 1..8
inline Square make_square(int file, int rank) { return rank * 8 + file; }
inline int file_of(Square s) { return s % 8; }
inline int rank_of(Square s) { return s / 8; }

Piece piece_from_char(char c);
char char_from_piece(Piece p);
