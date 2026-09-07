#include "bitboard.hpp"

namespace {
// Compass directions, indexed 0..7. POSITIVE marks the rays whose squares have
// INCREASING index (bit index = rank*8 + file), so the nearest blocker along them
// is the lowest set bit (lsb); the others use the highest (msb).
enum { N, S, E, W, NE, NW, SE, SW };
const int DF[8] = { 0,  0, 1, -1,  1, -1,  1, -1 };  // file step
const int DR[8] = { 1, -1, 0,  0,  1,  1, -1, -1 };  // rank step
const bool POSITIVE[8] = { true, false, true, false, true, true, false, false };

uint64_t KNIGHT_ATT[64];
uint64_t KING_ATT[64];
uint64_t PAWN_ATT[2][64];
uint64_t RAYS[8][64];   // RAYS[dir][sq] = every square along dir from sq (origin excluded)

inline bool on_board(int f, int r) { return f >= 0 && f < 8 && r >= 0 && r < 8; }

void init_leapers() {
    const int NF[8] = {  1,  2,  2,  1, -1, -2, -2, -1 };
    const int NR[8] = {  2,  1, -1, -2, -2, -1,  1,  2 };
    const int KF[8] = {  0,  1,  1,  1,  0, -1, -1, -1 };
    const int KR[8] = {  1,  1,  0, -1, -1, -1,  0,  1 };
    for (Square s = 0; s < 64; s++) {
        int f = file_of(s), r = rank_of(s);
        KNIGHT_ATT[s] = KING_ATT[s] = PAWN_ATT[0][s] = PAWN_ATT[1][s] = 0;
        for (int i = 0; i < 8; i++) {
            if (on_board(f + NF[i], r + NR[i])) bb_set(KNIGHT_ATT[s], make_square(f + NF[i], r + NR[i]));
            if (on_board(f + KF[i], r + KR[i])) bb_set(KING_ATT[s],   make_square(f + KF[i], r + KR[i]));
        }
        // A pawn attacks the two forward diagonals (white up a rank, black down).
        if (on_board(f - 1, r + 1)) bb_set(PAWN_ATT[0][s], make_square(f - 1, r + 1));
        if (on_board(f + 1, r + 1)) bb_set(PAWN_ATT[0][s], make_square(f + 1, r + 1));
        if (on_board(f - 1, r - 1)) bb_set(PAWN_ATT[1][s], make_square(f - 1, r - 1));
        if (on_board(f + 1, r - 1)) bb_set(PAWN_ATT[1][s], make_square(f + 1, r - 1));
    }
}

void init_rays() {
    for (int d = 0; d < 8; d++)
        for (Square s = 0; s < 64; s++) {
            RAYS[d][s] = 0;
            int f = file_of(s) + DF[d], r = rank_of(s) + DR[d];
            while (on_board(f, r)) {
                bb_set(RAYS[d][s], make_square(f, r));
                f += DF[d]; r += DR[d];
            }
        }
}

struct Init { Init() { init_leapers(); init_rays(); } };
const Init init_once;

// Attacks along one ray, stopping at (and including) the first blocker. The trick:
// RAYS[d][s] minus RAYS[d][blocker] is exactly the segment (s .. blocker], since the
// tail beyond the blocker is common to both and XOR cancels it.
inline uint64_t ray_attack(int d, Square s, uint64_t occ) {
    uint64_t ray = RAYS[d][s];
    uint64_t blockers = ray & occ;
    if (blockers) {
        int b = POSITIVE[d] ? lsb(blockers) : msb(blockers);   // nearest blocker to s
        ray ^= RAYS[d][b];
    }
    return ray;
}
} // namespace

uint64_t knight_attacks(Square s) { return KNIGHT_ATT[s]; }
uint64_t king_attacks(Square s)   { return KING_ATT[s]; }
uint64_t pawn_attacks(Color c, Square s) { return PAWN_ATT[static_cast<int>(c)][s]; }

uint64_t rook_attacks(Square s, uint64_t occ) {
    return ray_attack(N, s, occ) | ray_attack(S, s, occ)
         | ray_attack(E, s, occ) | ray_attack(W, s, occ);
}
uint64_t bishop_attacks(Square s, uint64_t occ) {
    return ray_attack(NE, s, occ) | ray_attack(NW, s, occ)
         | ray_attack(SE, s, occ) | ray_attack(SW, s, occ);
}
uint64_t queen_attacks(Square s, uint64_t occ) {
    return rook_attacks(s, occ) | bishop_attacks(s, occ);
}

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
