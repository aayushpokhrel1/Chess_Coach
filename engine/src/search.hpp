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

// One fixed-depth alpha-beta search. If `first` is a real move (from != NO_SQUARE)
// it is searched first at the root; the iterative-deepening driver passes the
// previous iteration's best move here to improve ordering.
SearchResult search_to_depth(Board& b, int depth, Move first = Move{});

// Limits for a timed search. budget_ms == 0 means "no clock, obey max_depth".
struct SearchLimits {
    int max_depth = 64;        // hard depth cap
    long long budget_ms = 0;   // per-move time budget in milliseconds
};

// Iterative deepening under an optional wall-clock budget. Returns the best move
// from the last fully completed depth (depth 1 always completes).
SearchResult search_timed(Board& b, const SearchLimits& limits);
