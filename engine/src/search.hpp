#pragma once
#include "board.hpp"
#include "move.hpp"

struct SearchResult {
    Move best;   // best move found (best.from == NO_SQUARE if no legal move exists)
    int score;   // centipawns, from the side-to-move perspective
};

SearchResult search(Board& b, int depth);  // fixed-depth negamax
