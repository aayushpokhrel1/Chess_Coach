#include "perft.hpp"
#include "move.hpp"
#include "movegen.hpp"

uint64_t perft(Board& b, int depth) {
    if (depth == 0) return 1;
    uint64_t nodes = 0;
    for (const Move& m : generate_legal(b)) {
        Undo u = make_move(b, m);
        nodes += perft(b, depth - 1);
        unmake_move(b, m, u);
    }
    return nodes;
}

std::map<std::string, uint64_t> perft_divide(Board& b, int depth) {
    std::map<std::string, uint64_t> out;
    for (const Move& m : generate_legal(b)) {
        Undo u = make_move(b, m);
        out[to_uci(m)] = (depth <= 1) ? 1 : perft(b, depth - 1);
        unmake_move(b, m, u);
    }
    return out;
}
