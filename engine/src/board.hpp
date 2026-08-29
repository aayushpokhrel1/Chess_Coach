#pragma once
#include <array>
#include <string>
#include "types.hpp"

constexpr int CASTLE_WK = 1;
constexpr int CASTLE_WQ = 2;
constexpr int CASTLE_BK = 4;
constexpr int CASTLE_BQ = 8;

struct Board {
    std::array<Piece, 64> squares;
    Color side_to_move = Color::White;
    int castling_rights = 0;
    Square en_passant = NO_SQUARE;
    int halfmove_clock = 0;
    int fullmove_number = 1;
};

Board board_from_fen(const std::string& fen);
std::string fen_from_board(const Board& b);
Board start_position();
std::string to_ascii(const Board& b);
