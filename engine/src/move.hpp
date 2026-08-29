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

std::string to_uci(const Move& m);
Board make_move(const Board& b, const Move& m);