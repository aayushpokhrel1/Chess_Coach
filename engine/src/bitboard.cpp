#include "bitboard.hpp"

void bb_rebuild(Board& b) {
    for (int c = 0; c < 2; c++)
        for (int t = 0; t < 6; t++) b.bb[c][t] = 0;
    b.occ[0] = b.occ[1] = 0;

    for (Square s = 0; s < 64; s++) {
        const Piece& p = b.squares[s];
        if (p.color == Color::None || p.type == PieceType::None) continue;
        int c = static_cast<int>(p.color);
        int t = static_cast<int>(p.type);
        bb_set(b.bb[c][t], s);
        bb_set(b.occ[c], s);
    }
    b.occ_all = b.occ[0] | b.occ[1];
}

bool bb_matches_squares(const Board& b) {
    Board tmp = b;
    bb_rebuild(tmp);   // recompute from squares[] and compare against the live bitboards
    for (int c = 0; c < 2; c++)
        for (int t = 0; t < 6; t++)
            if (tmp.bb[c][t] != b.bb[c][t]) return false;
    return tmp.occ[0] == b.occ[0] && tmp.occ[1] == b.occ[1] && tmp.occ_all == b.occ_all;
}
