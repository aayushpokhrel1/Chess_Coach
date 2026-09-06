#include "tt.hpp"
#include <vector>
#include <algorithm>

namespace {
    const int MATE = 30000;
    const int MATE_THRESHOLD = MATE - 1000;

    struct TTEntry {
        uint64_t key = 0;
        int32_t score = 0;
        Move move{};
        int16_t depth = -1;
        TTFlag flag = TTFlag::None;
    };

    const size_t TT_SIZE = 1u << 20;
    std::vector<TTEntry> g_tt(TT_SIZE);

    inline size_t slot(uint64_t key) { return key & (TT_SIZE - 1); }

    // Mate scores have magnitude > MATE_THRESHOLD and encode distance as +-(MATE - ply).
    // Make them node-relative on store and undo on probe:
    int to_tt(int score, int ply)   { if (score >  MATE_THRESHOLD) return score + ply; if (score < -MATE_THRESHOLD) return score - ply; return score; }
    int from_tt(int score, int ply) { if (score >  MATE_THRESHOLD) return score - ply; if (score < -MATE_THRESHOLD) return score + ply; return score; }
}

void tt_clear() {
    std::fill(g_tt.begin(), g_tt.end(), TTEntry{});
}

bool tt_probe(uint64_t key, int depth, int ply, int alpha, int beta, int& score, Move& move) {
    const TTEntry& e = g_tt[slot(key)];
    if (e.flag == TTFlag::None || e.key != key) return false;
    move = e.move;                 // usable for ordering even if depth is too shallow
    if (e.depth < depth) return false;
    int s = from_tt(e.score, ply);
    if (e.flag == TTFlag::Exact) { score = s; return true; }
    if (e.flag == TTFlag::Lower && s >= beta)  { score = s; return true; }
    if (e.flag == TTFlag::Upper && s <= alpha) { score = s; return true; }
    return false;
}

void tt_store(uint64_t key, int depth, int ply, int score, TTFlag flag, const Move& move) {
    TTEntry& e = g_tt[slot(key)];
    e.key = key; e.score = to_tt(score, ply); e.move = move; e.depth = (int16_t)depth; e.flag = flag;
}