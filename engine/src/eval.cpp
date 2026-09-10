#include "eval.hpp"
#include "bitboard.hpp"

namespace {
// Eval v2 weights. Tuning knobs, not laws: hand-set defaults a gauntlet can refine.
// Mutable (not const) so A/B self-play can retune them at runtime via eval_set_weight;
// the defaults below are the original hand-set values, so eval is unchanged until set.
// Mobility is per reachable square, lower for sliders so their squares do not swamp material.
struct EvalWeights {
    int mob_knight = 4, mob_bishop = 4, mob_rook = 2, mob_queen = 1;
    int bishop_pair = 30;
    int doubled = 15;    // penalty per extra pawn stacked on a file
    int isolated = 15;   // penalty per pawn with no friendly pawn on an adjacent file
    int shield = 12;     // penalty per file in front of the king with no pawn cover
    int passed = 12;     // passed-pawn bonus per rank advanced (0 disables the term)
};
EvalWeights W;

const uint64_t FILE_A_BB = 0x0101010101010101ULL;
inline uint64_t file_mask(int f) { return FILE_A_BB << f; }

// Reachable squares (attacks minus own pieces) per piece, weighted by kind.
int mobility_side(const Board& b, Color c) {
    int ci = static_cast<int>(c);
    uint64_t own = b.occ[ci];
    int m = 0, s;
    uint64_t bb;
    bb = b.bb[ci][1]; while (bb) { s = pop_lsb(bb); m += W.mob_knight * popcount(knight_attacks(s) & ~own); }
    bb = b.bb[ci][2]; while (bb) { s = pop_lsb(bb); m += W.mob_bishop * popcount(bishop_attacks(s, b.occ_all) & ~own); }
    bb = b.bb[ci][3]; while (bb) { s = pop_lsb(bb); m += W.mob_rook   * popcount(rook_attacks(s, b.occ_all) & ~own); }
    bb = b.bb[ci][4]; while (bb) { s = pop_lsb(bb); m += W.mob_queen  * popcount(queen_attacks(s, b.occ_all) & ~own); }
    return m;
}

// Doubled + isolated pawn penalty (a positive number; the caller subtracts it).
int pawn_penalty(const Board& b, Color c) {
    uint64_t pawns = b.bb[static_cast<int>(c)][0];
    int pen = 0;
    for (int f = 0; f < 8; f++) {
        int cnt = popcount(pawns & file_mask(f));
        if (cnt == 0) continue;
        if (cnt > 1) pen += W.doubled * (cnt - 1);
        uint64_t adj = (f > 0 ? file_mask(f - 1) : 0) | (f < 7 ? file_mask(f + 1) : 0);
        if (!(pawns & adj)) pen += W.isolated * cnt;   // no friendly pawn beside this file
    }
    return pen;
}

int bishop_pair(const Board& b, Color c) {
    return popcount(b.bb[static_cast<int>(c)][2]) >= 2 ? W.bishop_pair : 0;
}

// King safety: penalize a king whose pawn shield (the three files in front of it) has
// gaps. Only while the enemy still has a queen: in the queenless endgame the king should
// be active, so exposure there is fine and we do not penalize it. Returns a penalty (<=0).
int king_safety_side(const Board& b, Color c) {
    int ci = static_cast<int>(c), enemy = 1 - ci;
    if (b.bb[enemy][static_cast<int>(PieceType::Queen)] == 0) return 0;   // endgame: skip
    uint64_t king = b.bb[ci][static_cast<int>(PieceType::King)];
    if (king == 0) return 0;
    Square k = lsb(king);
    int kf = file_of(k), kr = rank_of(k), fwd = (c == Color::White) ? 1 : -1;
    uint64_t pawns = b.bb[ci][0];
    int covered = 0;
    for (int df = -1; df <= 1; df++) {
        int f = kf + df;
        if (f < 0 || f > 7) continue;
        // A friendly pawn on either of the two squares in front of this file is cover.
        for (int step = 1; step <= 2; step++) {
            int r = kr + step * fwd;
            if (r >= 0 && r < 8 && bb_get(pawns, make_square(f, r))) { covered++; break; }
        }
    }
    return -W.shield * (3 - covered);
}

// Passed pawns: a pawn with no enemy pawn on its own or the two adjacent files anywhere
// ahead of it (nothing can be traded off or blockaded to stop it). The bonus grows with
// how far it has advanced toward promotion, since a passer on the 7th is far more
// dangerous than one on the 3rd. Cheap per pawn: intersect a 3-file mask with an
// "everything ahead" half-board mask and check no enemy pawn sits there.
int passed_pawns(const Board& b, Color c) {
    int ci = static_cast<int>(c);
    uint64_t pawns = b.bb[ci][0];
    uint64_t enemy = b.bb[1 - ci][0];
    bool white = (c == Color::White);
    int bonus = 0;
    while (pawns) {
        int s = pop_lsb(pawns);
        int f = file_of(s), r = rank_of(s);
        uint64_t files3 = file_mask(f)
            | (f > 0 ? file_mask(f - 1) : 0)
            | (f < 7 ? file_mask(f + 1) : 0);
        // Every square on a rank strictly beyond r toward promotion (up for White,
        // down for Black), expressed as a half-board bitmask.
        uint64_t ahead = white ? (r < 7 ? (~0ULL << ((r + 1) * 8)) : 0ULL)
                               : (r > 0 ? ((1ULL << (r * 8)) - 1)   : 0ULL);
        if ((enemy & files3 & ahead) == 0)
            bonus += W.passed * (white ? r : 7 - r);   // r / 7-r = ranks advanced
    }
    return bonus;
}
// Michniewski "Simplified Evaluation Function" tables, White's perspective,
// printed rank 8 first, so index 0 = a8, index 63 = h1.
const int PAWN_PST[64] = {
     0,  0,  0,  0,  0,  0,  0,  0,
    50, 50, 50, 50, 50, 50, 50, 50,
    10, 10, 20, 30, 30, 20, 10, 10,
     5,  5, 10, 25, 25, 10,  5,  5,
     0,  0,  0, 20, 20,  0,  0,  0,
     5, -5,-10,  0,  0,-10, -5,  5,
     5, 10, 10,-20,-20, 10, 10,  5,
     0,  0,  0,  0,  0,  0,  0,  0
};
const int KNIGHT_PST[64] = {
    -50,-40,-30,-30,-30,-30,-40,-50,
    -40,-20,  0,  0,  0,  0,-20,-40,
    -30,  0, 10, 15, 15, 10,  0,-30,
    -30,  5, 15, 20, 20, 15,  5,-30,
    -30,  0, 15, 20, 20, 15,  0,-30,
    -30,  5, 10, 15, 15, 10,  5,-30,
    -40,-20,  0,  5,  5,  0,-20,-40,
    -50,-40,-30,-30,-30,-30,-40,-50
};
const int BISHOP_PST[64] = {
    -20,-10,-10,-10,-10,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5, 10, 10,  5,  0,-10,
    -10,  5,  5, 10, 10,  5,  5,-10,
    -10,  0, 10, 10, 10, 10,  0,-10,
    -10, 10, 10, 10, 10, 10, 10,-10,
    -10,  5,  0,  0,  0,  0,  5,-10,
    -20,-10,-10,-10,-10,-10,-10,-20
};
const int ROOK_PST[64] = {
     0,  0,  0,  0,  0,  0,  0,  0,
     5, 10, 10, 10, 10, 10, 10,  5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
     0,  0,  0,  5,  5,  0,  0,  0
};
const int QUEEN_PST[64] = {
    -20,-10,-10, -5, -5,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5,  5,  5,  5,  0,-10,
     -5,  0,  5,  5,  5,  5,  0, -5,
      0,  0,  5,  5,  5,  5,  0, -5,
    -10,  5,  5,  5,  5,  5,  0,-10,
    -10,  0,  5,  0,  0,  0,  0,-10,
    -20,-10,-10, -5, -5,-10,-10,-20
};
const int KING_PST[64] = {
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -20,-30,-30,-40,-40,-30,-30,-20,
    -10,-20,-20,-20,-20,-20,-20,-10,
     20, 20,  0,  0,  0,  0, 20, 20,
     20, 30, 10,  0,  0, 10, 30, 20
};

// Indexed by PieceType (Pawn=0 .. King=5).
const int* const PST[6] = {
    PAWN_PST, KNIGHT_PST, BISHOP_PST, ROOK_PST, QUEEN_PST, KING_PST
};

// Table value for one piece on one square (White's perspective magnitude).
int pst_value(Piece p, Square s) {
    const int* table = PST[static_cast<int>(p.type)];
    // White flips its square into the rank-8-first table orientation;
    // Black reads directly (its square is already the mirror image).
    Square idx = (p.color == Color::White)
        ? make_square(file_of(s), 7 - rank_of(s))
        : s;
    return table[idx];
}

int pst_score(const Board& b) {
    int score = 0;
    for (Square s = 0; s < 64; s++) {
        Piece p = b.squares[s];
        if (p.type == PieceType::None) continue;
        int v = pst_value(p, s);
        score += (p.color == Color::White) ? v : -v;
    }
    return score;
}
} // namespace

bool eval_set_weight(const std::string& name, int value) {
    if      (name == "MobKnight")  W.mob_knight = value;
    else if (name == "MobBishop")  W.mob_bishop = value;
    else if (name == "MobRook")    W.mob_rook   = value;
    else if (name == "MobQueen")   W.mob_queen  = value;
    else if (name == "BishopPair") W.bishop_pair = value;
    else if (name == "Doubled")    W.doubled    = value;
    else if (name == "Isolated")   W.isolated   = value;
    else if (name == "Shield")     W.shield     = value;
    else if (name == "Passed")     W.passed     = value;
    else return false;
    return true;
}

int piece_value(PieceType t) {
    switch (t) {
        case PieceType::Pawn:   return 100;
        case PieceType::Knight: return 320;
        case PieceType::Bishop: return 330;
        case PieceType::Rook:   return 500;
        case PieceType::Queen:  return 900;
        default:                return 0; // King and None
    }
}

int material_score(const Board& b) {
    int score = 0;
    for (Square s = 0; s < 64; s++) {
        Piece p = b.squares[s];
        if (p.type == PieceType::None) continue;
        int v = piece_value(p.type);
        score += (p.color == Color::White) ? v : -v;
    }
    return score;
}

int positional_eval(const Board& b) {
    int s = 0;
    s += mobility_side(b, Color::White) - mobility_side(b, Color::Black);
    s += bishop_pair(b, Color::White)   - bishop_pair(b, Color::Black);
    s -= pawn_penalty(b, Color::White)  - pawn_penalty(b, Color::Black); // penalties hurt their side
    s += king_safety_side(b, Color::White) - king_safety_side(b, Color::Black); // each is <=0
    s += passed_pawns(b, Color::White) - passed_pawns(b, Color::Black);         // each is >=0
    return s;
}

int evaluate(const Board& b) {
    int score = material_score(b) + pst_score(b) + positional_eval(b);
    return (b.side_to_move == Color::White) ? score : -score;
}
