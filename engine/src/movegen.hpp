#pragma once
#include <vector>
#include "board.hpp"
#include "move.hpp"

bool is_square_attacked(const Board& b, Square sq, Color by);
bool in_check(const Board& b, Color side);

std::vector<Move> generate_pseudo_legal(const Board& b);
std::vector<Move> generate_legal(const Board& b);
