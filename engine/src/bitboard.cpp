#include "bitboard.hpp"
#include <random>

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

// The pre-magic ray-scan sliders. Now used only to build and verify the magic tables
// (and, via the public *_ref wrappers, by the differential test).
uint64_t rook_ref(Square s, uint64_t occ) {
    return ray_attack(N, s, occ) | ray_attack(S, s, occ)
         | ray_attack(E, s, occ) | ray_attack(W, s, occ);
}
uint64_t bishop_ref(Square s, uint64_t occ) {
    return ray_attack(NE, s, occ) | ray_attack(NW, s, occ)
         | ray_attack(SE, s, occ) | ray_attack(SW, s, occ);
}

// --- Magic bitboards ---------------------------------------------------------------
// A slider's attacks depend only on which of its ray squares are occupied. That set of
// "relevant" squares is the mask; a magic multiply hashes the masked occupancy into a
// dense index, and the attack set is read straight from a per-square table.
struct Magic {
    uint64_t mask;    // relevant blocker squares for this square (edges excluded)
    uint64_t magic;   // the multiplier that spreads masked occupancy into the top bits
    int      shift;   // 64 - popcount(mask): how far to shift the product down
    uint64_t* table;  // 2^popcount(mask) attack sets, indexed by the magic hash
};
Magic ROOK_MAGIC[64];
Magic BISHOP_MAGIC[64];
// Fixed per-square capacity: rook masks have <=12 relevant bits, bishop <=9. A little
// slack is wasted per square but the code stays index-simple (no shared offset table).
uint64_t ROOK_TABLE[64][4096];
uint64_t BISHOP_TABLE[64][512];

// Relevant-blocker mask: the ray squares in each slider direction MINUS the final
// (edge) square of each ray, since a blocker on the edge gates nothing beyond it.
uint64_t slider_mask(Square s, bool rook) {
    const int rd[4] = { N, S, E, W };
    const int bd[4] = { NE, NW, SE, SW };
    uint64_t mask = 0;
    for (int i = 0; i < 4; i++) {
        int d = rook ? rd[i] : bd[i];
        int f = file_of(s) + DF[d], r = rank_of(s) + DR[d];
        while (on_board(f + DF[d], r + DR[d])) {   // stop before the edge square
            bb_set(mask, make_square(f, r));
            f += DF[d]; r += DR[d];
        }
    }
    return mask;
}

// The occupancy subset picked out by the low `bits` of `index`, scattered onto the set
// bits of `mask` (index bit i -> the i-th set bit of the mask). Enumerating index over
// 0..2^bits-1 walks every distinct blocker layout for this square.
uint64_t occupancy_for(int index, uint64_t mask) {
    uint64_t occ = 0;
    for (int i = 0; mask; i++) {
        int sq = pop_lsb(mask);
        if (index & (1 << i)) bb_set(occ, sq);
    }
    return occ;
}

// Find a magic for one square and fill its attack table. Random sparse candidates are
// tried until one maps all 2^bits layouts with no destructive collision (two layouts
// landing on one index must share the same attack set). A slider always attacks at
// least one square, so 0 is never a real attack and serves as the "empty slot" marker.
uint64_t find_magic(Square s, bool rook, uint64_t* table, std::mt19937_64& rng) {
    uint64_t mask = slider_mask(s, rook);
    int bits = popcount(mask);
    int n = 1 << bits;
    uint64_t occs[4096], atts[4096];
    for (int i = 0; i < n; i++) {
        occs[i] = occupancy_for(i, mask);
        atts[i] = rook ? rook_ref(s, occs[i]) : bishop_ref(s, occs[i]);
    }
    for (;;) {
        uint64_t magic = rng() & rng() & rng();   // sparse: few set bits hash better
        // Cheap reject: a good magic pushes plenty of mask bits into the top byte.
        if (popcount((mask * magic) & 0xFF00000000000000ULL) < 6) continue;
        for (int i = 0; i < n; i++) table[i] = 0;
        bool ok = true;
        for (int i = 0; i < n; i++) {
            int idx = static_cast<int>((occs[i] * magic) >> (64 - bits));
            if (table[idx] == 0) table[idx] = atts[i];
            else if (table[idx] != atts[i]) { ok = false; break; }   // collision, retry
        }
        if (ok) return magic;
    }
}

void init_magics() {
    std::mt19937_64 rng(0xD5C0FFEEULL);   // fixed seed: every build finds the same magics
    for (Square s = 0; s < 64; s++) {
        uint64_t rmask = slider_mask(s, true);
        ROOK_MAGIC[s] = { rmask, find_magic(s, true, ROOK_TABLE[s], rng),
                          64 - popcount(rmask), ROOK_TABLE[s] };
        uint64_t bmask = slider_mask(s, false);
        BISHOP_MAGIC[s] = { bmask, find_magic(s, false, BISHOP_TABLE[s], rng),
                            64 - popcount(bmask), BISHOP_TABLE[s] };
    }
}

struct Init { Init() { init_leapers(); init_rays(); init_magics(); } };
const Init init_once;
} // namespace

uint64_t knight_attacks(Square s) { return KNIGHT_ATT[s]; }
uint64_t king_attacks(Square s)   { return KING_ATT[s]; }
uint64_t pawn_attacks(Color c, Square s) { return PAWN_ATT[static_cast<int>(c)][s]; }

uint64_t rook_attacks(Square s, uint64_t occ) {
    const Magic& m = ROOK_MAGIC[s];
    return m.table[((occ & m.mask) * m.magic) >> m.shift];
}
uint64_t bishop_attacks(Square s, uint64_t occ) {
    const Magic& m = BISHOP_MAGIC[s];
    return m.table[((occ & m.mask) * m.magic) >> m.shift];
}
uint64_t queen_attacks(Square s, uint64_t occ) {
    return rook_attacks(s, occ) | bishop_attacks(s, occ);
}

uint64_t rook_attacks_ref(Square s, uint64_t occ)   { return rook_ref(s, occ); }
uint64_t bishop_attacks_ref(Square s, uint64_t occ) { return bishop_ref(s, occ); }

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
