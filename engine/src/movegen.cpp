#include "movegen.hpp"

namespace {
inline bool on_board(int f, int r) { return f >= 0 && f < 8 && r >= 0 && r < 8; }

const int KNIGHT[8][2] = {{1,2},{2,1},{2,-1},{1,-2},{-1,-2},{-2,-1},{-2,1},{-1,2}};
const int DIAG[4][2]   = {{1,1},{1,-1},{-1,1},{-1,-1}};
const int ORTH[4][2]   = {{1,0},{-1,0},{0,1},{0,-1}};

bool ray_hits(const Board& b, int f, int r, const int dirs[4][2],
              Color by, PieceType a, PieceType c) {
    for (int i = 0; i < 4; i++) {
        int ff = f + dirs[i][0], rr = r + dirs[i][1];
        while (on_board(ff, rr)) {
            Piece p = b.squares[make_square(ff, rr)];
            if (p.type != PieceType::None) {
                if (p.color == by && (p.type == a || p.type == c)) return true;
                break; // first blocker ends this ray
            }
            ff += dirs[i][0]; rr += dirs[i][1];
        }
    }
    return false;
}
} // namespace

bool is_square_attacked(const Board& b, Square sq, Color by) {
    int f = file_of(sq), r = rank_of(sq);

    // Pawns: a `by` pawn attacking sq stands one rank toward its own side.
    int pr = (by == Color::White) ? r - 1 : r + 1;
    for (int df : {-1, 1}) {
        if (on_board(f + df, pr)) {
            Piece p = b.squares[make_square(f + df, pr)];
            if (p.color == by && p.type == PieceType::Pawn) return true;
        }
    }
    // Knights.
    for (auto& d : KNIGHT) {
        if (on_board(f + d[0], r + d[1])) {
            Piece p = b.squares[make_square(f + d[0], r + d[1])];
            if (p.color == by && p.type == PieceType::Knight) return true;
        }
    }
    // King.
    for (int df = -1; df <= 1; df++)
        for (int dr = -1; dr <= 1; dr++) {
            if (df == 0 && dr == 0) continue;
            if (on_board(f + df, r + dr)) {
                Piece p = b.squares[make_square(f + df, r + dr)];
                if (p.color == by && p.type == PieceType::King) return true;
            }
        }
    // Sliders.
    if (ray_hits(b, f, r, DIAG, by, PieceType::Bishop, PieceType::Queen)) return true;
    if (ray_hits(b, f, r, ORTH, by, PieceType::Rook,   PieceType::Queen)) return true;
    return false;
}

bool in_check(const Board& b, Color side) {
    Color them = (side == Color::White) ? Color::Black : Color::White;
    for (Square s = 0; s < 64; s++) {
        Piece p = b.squares[s];
        if (p.color == side && p.type == PieceType::King)
            return is_square_attacked(b, s, them);
    }
    return false; // no king on board (not expected in legal positions)
}
