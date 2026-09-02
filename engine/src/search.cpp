#include "search.hpp"
#include "movegen.hpp"
#include "eval.hpp"
#include <vector>

namespace {
const int MATE = 30000;
const int INF  = 31000;

// Value of the node for the side to move. `ply` is the distance from the root.
int negamax(Board& b, int depth, int ply) {
    std::vector<Move> moves = generate_legal(b);
    if (moves.empty())
        return in_check(b, b.side_to_move) ? -(MATE - ply) : 0;
    if (depth == 0)
        return evaluate(b);

    int best = -INF;
    for (const Move& m : moves) {
        Undo u = make_move(b, m);
        int score = -negamax(b, depth - 1, ply + 1);
        unmake_move(b, m, u);
        if (score > best) best = score;
    }
    return best;
}
} // namespace

SearchResult search(Board& b, int depth) {
    SearchResult result;
    result.best = Move{};   // from == NO_SQUARE means "no move"
    result.score = 0;

    std::vector<Move> moves = generate_legal(b);
    if (moves.empty()) {
        result.score = in_check(b, b.side_to_move) ? -MATE : 0;
        return result;
    }

    int best = -INF;
    for (const Move& m : moves) {
        Undo u = make_move(b, m);
        int score = -negamax(b, depth - 1, 1);
        unmake_move(b, m, u);
        if (score > best) {
            best = score;
            result.best = m;
        }
    }
    result.score = best;
    return result;
}
