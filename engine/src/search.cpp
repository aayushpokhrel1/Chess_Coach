#include "search.hpp"
#include "movegen.hpp"
#include "eval.hpp"
#include "tt.hpp"
#include "zobrist.hpp"
#include <vector>
#include <algorithm>
#include <chrono>

namespace {
const int MATE = 30000;
const int INF  = 31000;
const int MATE_THRESHOLD = MATE - 1000;  // scores past this are forced mates

using Clock = std::chrono::steady_clock;

long g_nodes = 0;              // reset at the top of each public search entry point
bool g_use_tt = true;         // transposition table on? (tests toggle it off to compare)
bool g_use_order_heur = true; // killer + history quiet-move ordering on? (tests toggle it)
bool g_use_null = true;       // null-move pruning on? (a heuristic; tests toggle it off)
bool g_timed = false;         // is the current search time-limited?
bool g_can_stop = false;      // may we abort the current depth? (false during depth 1)
bool g_stop = false;          // set true once the deadline has passed
long g_check_counter = 0;     // node counter for periodic clock checks
Clock::time_point g_deadline;

// Triangular PV table: g_pv[ply] holds the best line found from that ply,
// g_pv_len[ply] its length. Filled bottom-up as negamax returns.
const int MAXPLY = 128;
Move g_pv[MAXPLY][MAXPLY];
int  g_pv_len[MAXPLY];

// Quiet-move ordering memory, both filled on beta cutoffs and cleared per search.
// Killers: up to 2 quiet moves per ply that recently caused a cutoff there; tried
// first among quiets in sibling nodes at the same ply. History: a per
// (side, from, to) tally of cutoff usefulness (weighted depth*depth), the tiebreak
// ordering for the remaining quiets.
Move g_killers[MAXPLY][2];
int  g_history[2][64][64];

void clear_order_heur() {
    for (int p = 0; p < MAXPLY; p++) g_killers[p][0] = g_killers[p][1] = Move{};
    for (int s = 0; s < 2; s++)
        for (int f = 0; f < 64; f++)
            for (int t = 0; t < 64; t++) g_history[s][f][t] = 0;
}

bool same_move(const Move& a, const Move& c) {
    return a.from == c.from && a.to == c.to && a.promotion == c.promotion;
}

// Every 2048 nodes, glance at the wall clock and set g_stop if time is up.
inline void maybe_timeout() {
    if (!g_timed || !g_can_stop) return;
    if ((++g_check_counter & 2047) == 0 && Clock::now() >= g_deadline)
        g_stop = true;
}

// Does `side` have any piece other than pawns and the king? Null-move pruning is
// unsafe in zugzwang (where passing is artificially good), and king-and-pawn
// endgames are the common zugzwang case, so we only null-move when this is true.
// ponytail: O(64) scan per attempt; a bitboard popcount makes this free later.
bool has_non_pawn_material(const Board& b, Color side) {
    for (Square s = 0; s < 64; s++) {
        const Piece& p = b.squares[s];
        if (p.color == side && p.type != PieceType::Pawn && p.type != PieceType::King)
            return true;
    }
    return false;
}

// Captures first: cheap move ordering so alpha-beta cutoffs land early.
bool is_capture(const Board& b, const Move& m) {
    return b.squares[m.to].type != PieceType::None
        || m.flag == MoveFlag::EnPassant;
}

// Value of the piece captured by move m (en passant always takes a pawn).
int victim_value(const Board& b, const Move& m) {
    if (m.flag == MoveFlag::EnPassant) return piece_value(PieceType::Pawn);
    return piece_value(b.squares[m.to].type);
}

// A single sort key per move, higher = tried first. Tiers, from the top:
//   captures  (MVV-LVA, so queen-takes-queen before pawn-takes-pawn)
//   killer 1 / killer 2  (quiet moves that cut off at this ply before)
//   other quiets  (by history score, the running cutoff tally)
// The tier bases are spaced so a capture always outranks a killer and a killer
// always outranks any history score (history is clamped below the killer base on
// store). With the heuristics off, every quiet scores 0 and stable_sort keeps them
// in generation order, i.e. the original captures-first-then-MVV behaviour.
const int CAP_BASE = 1'000'000;
const int KILLER1  =   900'000;
const int KILLER2  =   800'000;
const int HIST_MAX =   700'000;   // history is clamped here so it never reaches a killer

int move_score(const Board& b, const Move& m, int ply) {
    if (is_capture(b, m))
        return CAP_BASE + victim_value(b, m) - piece_value(b.squares[m.from].type);
    if (!g_use_order_heur || ply >= MAXPLY) return 0;
    if (same_move(m, g_killers[ply][0])) return KILLER1;
    if (same_move(m, g_killers[ply][1])) return KILLER2;
    return g_history[static_cast<int>(b.side_to_move)][m.from][m.to];
}

void order_moves(const Board& b, std::vector<Move>& moves, int ply) {
    std::stable_sort(moves.begin(), moves.end(), [&](const Move& a, const Move& c) {
        return move_score(b, a, ply) > move_score(b, c, ply);
    });
}

// Record a quiet move that just caused a beta cutoff at `ply` (searched to `depth`):
// promote it into this ply's killer slots and bump its history score.
void record_cutoff(const Board& b, const Move& m, int ply, int depth) {
    if (!g_use_order_heur || ply >= MAXPLY || is_capture(b, m)) return;
    if (!same_move(m, g_killers[ply][0])) {
        g_killers[ply][1] = g_killers[ply][0];
        g_killers[ply][0] = m;
    }
    int& h = g_history[static_cast<int>(b.side_to_move)][m.from][m.to];
    h += depth * depth;
    if (h > HIST_MAX) h = HIST_MAX;
}

// Leaf of the main search. Instead of trusting a static eval in the middle of a
// capture fight, keep resolving captures until the position is quiet, so the score
// reflects the material that actually stays on the board.
//
// ponytail: qdepth cap (QMAX) truncates runaway check sequences (e.g. perpetual
// check) by falling back to the static eval; raise QMAX or add repetition detection
// if a real analysis position is ever cut short.
const int QMAX = 40;

int quiesce(Board& b, int ply, int alpha, int beta, int qdepth = 0) {
    maybe_timeout();
    if (g_stop) return 0;   // aborted: value discarded upstream
    g_nodes++;

    if (qdepth >= QMAX) return evaluate(b);   // depth guard, see ponytail note
    bool check = in_check(b, b.side_to_move);

    std::vector<Move> moves;
    int best;

    if (check) {
        // In check there is no "do nothing" option: search every legal escape,
        // not just captures, or we could miss the only move that survives.
        moves = generate_legal(b);
        if (moves.empty()) return -(MATE - ply);   // checkmate at the leaf
        best = -INF;
    } else {
        // Stand pat: you are never forced to capture, so the static eval is a floor.
        int stand = evaluate(b);
        if (stand >= beta) return stand;
        best = stand;
        if (stand > alpha) alpha = stand;
        moves = generate_legal(b);
        std::vector<Move> caps;
        for (const Move& m : moves)
            if (is_capture(b, m)) caps.push_back(m);
        moves.swap(caps);
    }

    order_moves(b, moves, ply);   // MVV-LVA (quiescence is captures only, so no killers)
    for (const Move& m : moves) {
        Undo u = make_move(b, m);
        int score = -quiesce(b, ply + 1, -beta, -alpha, qdepth + 1);
        unmake_move(b, m, u);
        if (g_stop) return best;
        if (score > best) best = score;
        if (best > alpha) alpha = best;
        if (alpha >= beta) break;   // beta cutoff
    }
    return best;
}

// Alpha-beta negamax. Same value as plain negamax, fewer nodes.
int negamax(Board& b, int depth, int ply, int alpha, int beta, bool can_null = true) {
    maybe_timeout();
    if (g_stop) return 0;   // aborted: this value is discarded upstream
    g_nodes++;

    if (ply < MAXPLY) g_pv_len[ply] = 0;

    // Transposition table probe. If we have searched this exact position at least
    // this deep and the stored bound settles the current [alpha, beta] window, reuse
    // it. Even a too-shallow hit hands back the move that was best here, for ordering.
    const uint64_t key = compute_hash(b);
    Move tt_move{};
    if (g_use_tt) {
        int tt_score;
        if (tt_probe(key, depth, ply, alpha, beta, tt_score, tt_move))
            return tt_score;
    }

    std::vector<Move> moves = generate_legal(b);
    if (moves.empty())
        return in_check(b, b.side_to_move) ? -(MATE - ply) : 0;
    if (depth == 0)
        return quiesce(b, ply, alpha, beta);

    // Null-move pruning: hand the opponent a free move; if our position is still so
    // strong that even after passing we stay >= beta, no real move of ours would do
    // worse, so prune the node. Guards, each blocking a way the free pass would lie:
    // not in check (can't pass out of check), enough depth left for the reduced
    // search, some non-pawn material (avoid zugzwang), not two nulls in a row, and
    // not inside a mate-scoring window (do not trade a real mate for a fail-high beta).
    if (g_use_null && can_null && depth >= 3 && beta < MATE_THRESHOLD
            && !in_check(b, b.side_to_move)
            && has_non_pawn_material(b, b.side_to_move)) {
        const int R = 2;   // reduce the pass search by this many plies
        Color saved_side = b.side_to_move;
        Square saved_ep = b.en_passant;
        b.side_to_move = (saved_side == Color::White) ? Color::Black : Color::White;
        b.en_passant = NO_SQUARE;   // a pass clears any en-passant right
        int null_score = -negamax(b, depth - 1 - R, ply + 1, -beta, -beta + 1, false);
        b.side_to_move = saved_side;
        b.en_passant = saved_ep;
        if (g_stop) return 0;                   // aborted: value discarded upstream
        if (null_score >= beta) return beta;    // fail-high: prune this node
    }

    order_moves(b, moves, ply);
    // Search the TT move first: it was best here before, so it is the likeliest cutoff.
    if (tt_move.from != NO_SQUARE) {
        auto it = std::find_if(moves.begin(), moves.end(), [&](const Move& m) {
            return m.from == tt_move.from && m.to == tt_move.to && m.promotion == tt_move.promotion;
        });
        if (it != moves.end()) std::rotate(moves.begin(), it, it + 1);
    }

    const int alpha_orig = alpha;   // the window we started with, for the store flag
    int best = -INF;
    Move best_move{};
    for (const Move& m : moves) {
        Undo u = make_move(b, m);
        int score = -negamax(b, depth - 1, ply + 1, -beta, -alpha);
        unmake_move(b, m, u);
        if (g_stop) return best;    // bail out fast; result discarded upstream
        if (score > best) {
            best = score;
            best_move = m;
            if (score > alpha) {    // a PV move: record it and splice the child's line
                alpha = score;
                if (ply + 1 < MAXPLY) {
                    g_pv[ply][0] = m;
                    int n = g_pv_len[ply + 1];
                    for (int i = 0; i < n && i + 1 < MAXPLY; i++)
                        g_pv[ply][i + 1] = g_pv[ply + 1][i];
                    g_pv_len[ply] = n + 1;
                }
            }
        }
        if (alpha >= beta) {        // beta cutoff: opponent would never allow this node
            record_cutoff(b, m, ply, depth);   // remember this quiet move for sibling ordering
            break;
        }
    }

    // Store the result. The flag records how `best` sits against the original window:
    // below it (never beat alpha) is an Upper bound, at/above beta (a cutoff) is a
    // Lower bound, strictly inside is Exact.
    if (g_use_tt) {
        TTFlag flag = best <= alpha_orig ? TTFlag::Upper
                    : best >= beta       ? TTFlag::Lower
                                         : TTFlag::Exact;
        tt_store(key, depth, ply, best, flag, best_move);
    }
    return best;
}

// Full-width reference: no cutoffs, no ordering, no timing. Test oracle only.
int negamax_full(Board& b, int depth, int ply) {
    g_nodes++;
    std::vector<Move> moves = generate_legal(b);
    if (moves.empty())
        return in_check(b, b.side_to_move) ? -(MATE - ply) : 0;
    if (depth == 0)
        return quiesce(b, ply, -INF, INF);   // same leaf as negamax, full window keeps it exact

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

void search_use_tt(bool on) { g_use_tt = on; }

void search_use_order_heur(bool on) { g_use_order_heur = on; }

void search_use_null(bool on) { g_use_null = on; }

int search_minimax(Board& b, int depth) {
    g_nodes = 0;
    g_timed = false;
    g_stop = false;
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

    order_moves(b, moves, 0);   // root is ply 0
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
        if (g_stop) break;   // depth incomplete; caller discards this result
        if (score > best) {
            best = score;
            result.best = m;
            result.pv.clear();          // this root move plus the line below it
            result.pv.push_back(m);
            for (int i = 0; i < g_pv_len[1] && i < MAXPLY; i++)
                result.pv.push_back(g_pv[1][i]);
        }
        if (best > alpha) alpha = best;
    }
    result.score = best;
    result.depth = depth;
    return result;
}

SearchResult search(Board& b, int max_depth) {
    g_timed = false;
    g_stop = false;
    tt_clear();          // fresh table per search; iterative deepening reuses it across depths
    clear_order_heur();  // fresh killers/history too, likewise reused across ID depths
    SearchResult result;
    result.best = Move{};
    result.score = 0;
    for (int d = 1; d <= max_depth; d++) {
        result = search_to_depth(b, d, result.best);
    }
    return result;
}

SearchResult search_timed(Board& b, const SearchLimits& limits) {
    g_timed = (limits.budget_ms > 0);
    g_stop = false;
    tt_clear();          // fresh table per search; iterative deepening reuses it across depths
    clear_order_heur();  // fresh killers/history too, likewise reused across ID depths
    g_deadline = Clock::now() + std::chrono::milliseconds(limits.budget_ms);

    SearchResult best;
    best.best = Move{};
    best.score = 0;

    for (int d = 1; d <= limits.max_depth; d++) {
        g_can_stop = (d > 1);       // always finish depth 1 so we return a legal move
        g_check_counter = 0;
        SearchResult r = search_to_depth(b, d, best.best);
        if (g_stop) break;          // depth d aborted: keep the depth d-1 result
        best = r;
        if (best.best.from == NO_SQUARE) break;                  // no legal move at root
        if (best.score > MATE_THRESHOLD || best.score < -MATE_THRESHOLD) break;  // mate found
        if (g_timed && Clock::now() >= g_deadline) break;        // no time for another depth
    }

    g_timed = false;   // leave timing off so later untimed calls never abort
    g_stop = false;
    return best;
}
