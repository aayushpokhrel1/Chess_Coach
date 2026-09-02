#include "perft.hpp"
#include "move.hpp"
#include "movegen.hpp"

uint64_t perft(const Board& b, int depth) {
    if (depth == 0) return 1;
    uint64_t nodes = 0;
    for (const Move& m : generate_legal(b)) {
        Board nb = b;
        make_move(nb, m);
        nodes += perft(nb, depth - 1);
    }
    return nodes;
}

std::map<std::string, uint64_t> perft_divide(const Board& b, int depth) {
    std::map<std::string, uint64_t> out;
    for (const Move& m : generate_legal(b)) {
        Board nb = b;
        make_move(nb, m);
        out[to_uci(m)] = (depth <= 1) ? 1 : perft(nb, depth - 1);
    }
    return out;
}
