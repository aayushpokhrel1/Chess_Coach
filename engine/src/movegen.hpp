#pragma once
#include <vector>
#include "board.hpp"
#include "move.hpp"

bool is_square_attacked(const Board& b, Square sq, Color by);
// Temporary: the old array scan, exposed only to differential-test the bitboard
// version above. Removed in the bitboard cleanup task.
bool is_square_attacked_ref(const Board& b, Square sq, Color by);
bool in_check(const Board& b, Color side);

std::vector<Move> generate_pseudo_legal(const Board& b);
std::vector<Move> generate_legal(Board& b);
