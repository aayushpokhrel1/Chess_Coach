#pragma once
#include <vector>
#include "board.hpp"
#include "move.hpp"

struct SearchResult {
    Move best;               // best move found (best.from == NO_SQUARE if no legal move exists)
    int score;               // centipawns, from the side-to-move perspective
    std::vector<Move> pv;    // principal variation (main line), pv[0] == best
    int depth = 0;           // last fully completed search depth
};

SearchResult search(Board& b, int depth);  // fixed-depth negamax

// Exposed for tests. search_minimax is a full-width (un-pruned) reference
// search: it returns the same value as search(...).score but visits every node,
// so a test can prove alpha-beta prunes without changing the answer.
int  search_minimax(Board& b, int depth);
long nodes_searched();  // nodes visited by the most recent search / search_minimax call

// Exposed for tests: turn the transposition table off/on so a test can compare node
// counts with and without it. Enabled by default.
void search_use_tt(bool on);

// Exposed for tests: turn the quiet-move ordering heuristics (killer moves +
// history) off/on so a test can compare node counts with and without them.
// Enabled by default.
void search_use_order_heur(bool on);

// Exposed for tests: turn null-move pruning off/on. It is a forward-pruning
// heuristic (it changes the value, not just the node count), so an exact-equality
// test must disable it alongside the TT. Enabled by default.
void search_use_null(bool on);

// Exposed for tests: turn late move reductions off/on. Also a heuristic that can
// change the value, so an exact-equality test disables it too. Enabled by default.
void search_use_lmr(bool on);

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
