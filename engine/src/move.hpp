#pragma once
#include <string>
#include "types.hpp"
#include "board.hpp"

enum class MoveFlag { Normal, DoublePawnPush, EnPassant, Castle, Promotion };

struct Move {
    Square from = NO_SQUARE;
    Square to = NO_SQUARE;
    PieceType promotion = PieceType::None; // used only when flag == Promotion
    MoveFlag flag = MoveFlag::Normal;
};

struct Undo {
    Piece captured = Piece{Color::None, PieceType::None};
    int castling_rights = 0;
    Square en_passant = NO_SQUARE;
    int halfmove_clock = 0;
    int fullmove_number = 1;
};

std::string to_uci(const Move& m);
Undo make_move(Board& b, const Move& m);
void unmake_move(Board& b, const Move& m, const Undo& u);