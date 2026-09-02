#include "search.hpp"
#include "movegen.hpp"
#include "eval.hpp"
#include <vector>
#include <algorithm>

namespace {
const int MATE = 30000;
const int INF  = 31000;

long g_nodes = 0;  // reset at the top of each public search entry point

// Captures first: cheap move ordering so alpha-beta cutoffs land early.
bool is_capture(const Board& b, const Move& m) {
    return b.squares[m.to].type != PieceType::None
        || m.flag == MoveFlag::EnPassant;
}

void order_moves(const Board& b, std::vector<Move>& moves) {
    std::stable_partition(moves.begin(), moves.end(),
                          [&](const Move& m) { return is_capture(b, m); });
}

// Alpha-beta negamax. Same value as plain negamax, fewer nodes.
int negamax(Board& b, int depth, int ply, int alpha, int beta) {
    g_nodes++;
    std::vector<Move> moves = generate_legal(b);
    if (moves.empty())
        return in_check(b, b.side_to_move) ? -(MATE - ply) : 0;
    if (depth == 0)
        return evaluate(b);

    order_moves(b, moves);
    int best = -INF;
    for (const Move& m : moves) {
        Undo u = make_move(b, m);
        int score = -negamax(b, depth - 1, ply + 1, -beta, -alpha);
        unmake_move(b, m, u);
        if (score > best) best = score;
        if (best > alpha) alpha = best;
        if (alpha >= beta) break;   // beta cutoff: opponent would never allow this node
    }
    return best;
}

// Full-width reference: no cutoffs, no ordering. Test oracle only.
int negamax_full(Board& b, int depth, int ply) {
    g_nodes++;
    std::vector<Move> moves = generate_legal(b);
    if (moves.empty())
        return in_check(b, b.side_to_move) ? -(MATE - ply) : 0;
    if (depth == 0)
        return evaluate(b);

    int best = -INF;
    for (const Move& m : moves) {
        Undo u = make_move(b, m);
        int score = -negamax_full(b, depth - 1, ply + 1);
        unmake_move(b, m, u);
        if (score > best) best = score;
    }
    return best;
}
} // namespace

long nodes_searched() { return g_nodes; }

int search_minimax(Board& b, int depth) {
    g_nodes = 0;
    return negamax_full(b, depth, 0);
}

SearchResult search_to_depth(Board& b, int depth, Move first) {
    g_nodes = 0;
    SearchResult result;
    result.best = Move{};
    result.score = 0;

    std::vector<Move> moves = generate_legal(b);
    if (moves.empty()) {
        result.score = in_check(b, b.side_to_move) ? -MATE : 0;
        return result;
    }

    order_moves(b, moves);
    // Try the hint move first (from the previous, shallower iteration).
    if (first.from != NO_SQUARE) {
        auto it = std::find_if(moves.begin(), moves.end(), [&](const Move& m) {
            return m.from == first.from && m.to == first.to && m.promotion == first.promotion;
        });
        if (it != moves.end()) std::rotate(moves.begin(), it, it + 1);
    }

    int best = -INF;
    int alpha = -INF;
    for (const Move& m : moves) {
        Undo u = make_move(b, m);
        int score = -negamax(b, depth - 1, 1, -INF, -alpha);
        unmake_move(b, m, u);
        if (score > best) {
            best = score;
            result.best = m;
        }
        if (best > alpha) alpha = best;
    }
    result.score = best;
    return result;
}

SearchResult search(Board& b, int max_depth) {
    SearchResult result;
    result.best = Move{};
    result.score = 0;
    for (int d = 1; d <= max_depth; d++) {
        result = search_to_depth(b, d, result.best);  // reuse previous best for ordering
    }
    return result;
}
