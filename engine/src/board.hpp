#pragma once
#include <array>
#include <cstdint>
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

    // Redundant bitboard view of squares[], kept in sync by FEN and make/unmake.
    // bb[color][PieceType] has a set bit per piece of that kind; occ is per-color
    // occupancy and occ_all is both. squares[] stays the "what is on square X"
    // source of truth; these accelerate the set-wise operations in move generation.
    uint64_t bb[2][6] = {};
    uint64_t occ[2] = {};
    uint64_t occ_all = 0;
};

Board board_from_fen(const std::string& fen);
std::string fen_from_board(const Board& b);
Board start_position();
std::string to_ascii(const Board& b);
