#pragma once
#include "board.hpp"
#include "move.hpp"

struct SearchResult {
    Move best;   // best move found (best.from == NO_SQUARE if no legal move exists)
    int score;   // centipawns, from the side-to-move perspective
};

SearchResult search(Board& b, int depth);  // fixed-depth negamax

// Exposed for tests. search_minimax is a full-width (un-pruned) reference
// search: it returns the same value as search(...).score but visits every node,
// so a test can prove alpha-beta prunes without changing the answer.
int  search_minimax(Board& b, int depth);
long nodes_searched();  // nodes visited by the most recent search / search_minimax call
