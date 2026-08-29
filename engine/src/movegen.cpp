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

void gen_offsets(const Board& b, Square s, const int offs[8][2], int n,
                 std::vector<Move>& out) {
    Color us = b.squares[s].color;
    int f = file_of(s), r = rank_of(s);
    for (int i = 0; i < n; i++) {
        int nf = f + offs[i][0], nr = r + offs[i][1];
        if (!on_board(nf, nr)) continue;
        Square to = make_square(nf, nr);
        if (b.squares[to].color == us) continue; // friendly piece blocks
        out.push_back(Move{s, to, PieceType::None, MoveFlag::Normal});
    }
}

const int KING[8][2] = {{1,0},{-1,0},{0,1},{0,-1},{1,1},{1,-1},{-1,1},{-1,-1}};

void gen_slider(const Board& b, Square s, const int dirs[4][2],
                std::vector<Move>& out) {
    Color us = b.squares[s].color;
    int f = file_of(s), r = rank_of(s);
    for (int i = 0; i < 4; i++) {
        int nf = f + dirs[i][0], nr = r + dirs[i][1];
        while (on_board(nf, nr)) {
            Square to = make_square(nf, nr);
            Piece p = b.squares[to];
            if (p.type == PieceType::None) {
                out.push_back(Move{s, to, PieceType::None, MoveFlag::Normal});
            } else {
                if (p.color != us)
                    out.push_back(Move{s, to, PieceType::None, MoveFlag::Normal});
                break; // ray stops at the first piece either way
            }
            nf += dirs[i][0]; nr += dirs[i][1];
        }
    }
}

void add_pawn(std::vector<Move>& out, Square from, Square to,
              bool promo, MoveFlag flag) {
    if (promo) {
        for (PieceType pt : {PieceType::Queen, PieceType::Rook,
                             PieceType::Bishop, PieceType::Knight})
            out.push_back(Move{from, to, pt, MoveFlag::Promotion});
    } else {
        out.push_back(Move{from, to, PieceType::None, flag});
    }
}

void gen_pawn(const Board& b, Square s, std::vector<Move>& out) {
    Color us = b.squares[s].color;
    Color them = (us == Color::White) ? Color::Black : Color::White;
    int f = file_of(s), r = rank_of(s);
    int dir       = (us == Color::White) ? 1 : -1;
    int startRank = (us == Color::White) ? 1 : 6;
    int promoRank = (us == Color::White) ? 7 : 0;

    // Single (and double) push.
    int r1 = r + dir;
    if (on_board(f, r1) && b.squares[make_square(f, r1)].type == PieceType::None) {
        add_pawn(out, s, make_square(f, r1), r1 == promoRank, MoveFlag::Normal);
        if (r == startRank) {
            int r2 = r + 2 * dir;
            if (b.squares[make_square(f, r2)].type == PieceType::None)
                out.push_back(Move{s, make_square(f, r2),
                                   PieceType::None, MoveFlag::DoublePawnPush});
        }
    }
    // Captures, including en passant.
    for (int df : {-1, 1}) {
        int nf = f + df, nr = r + dir;
        if (!on_board(nf, nr)) continue;
        Square to = make_square(nf, nr);
        if (b.squares[to].color == them) {
            add_pawn(out, s, to, nr == promoRank, MoveFlag::Normal);
        } else if (b.en_passant != NO_SQUARE && to == b.en_passant) {
            out.push_back(Move{s, to, PieceType::None, MoveFlag::EnPassant});
        }
    }
}

void gen_castling(const Board& b, std::vector<Move>& out) {
    Color us = b.side_to_move;
    Color them = (us == Color::White) ? Color::Black : Color::White;
    int r = (us == Color::White) ? 0 : 7;
    Square king = make_square(4, r);
    if (b.squares[king].type != PieceType::King || b.squares[king].color != us)
        return;
    if (is_square_attacked(b, king, them)) return; // cannot castle out of check

    int kRight = (us == Color::White) ? CASTLE_WK : CASTLE_BK;
    int qRight = (us == Color::White) ? CASTLE_WQ : CASTLE_BQ;

    // Kingside: f and g empty; f and g not attacked.
    if ((b.castling_rights & kRight)
        && b.squares[make_square(5, r)].type == PieceType::None
        && b.squares[make_square(6, r)].type == PieceType::None
        && !is_square_attacked(b, make_square(5, r), them)
        && !is_square_attacked(b, make_square(6, r), them)) {
        out.push_back(Move{king, make_square(6, r), PieceType::None, MoveFlag::Castle});
    }
    // Queenside: b, c, d empty; c and d not attacked (king crosses d, lands c).
    if ((b.castling_rights & qRight)
        && b.squares[make_square(1, r)].type == PieceType::None
        && b.squares[make_square(2, r)].type == PieceType::None
        && b.squares[make_square(3, r)].type == PieceType::None
        && !is_square_attacked(b, make_square(3, r), them)
        && !is_square_attacked(b, make_square(2, r), them)) {
        out.push_back(Move{king, make_square(2, r), PieceType::None, MoveFlag::Castle});
    }
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

std::vector<Move> generate_pseudo_legal(const Board& b) {
    std::vector<Move> out;
    Color us = b.side_to_move;
    for (Square s = 0; s < 64; s++) {
        Piece p = b.squares[s];
        if (p.color != us) continue;
        switch (p.type) {
            case PieceType::Knight: gen_offsets(b, s, KNIGHT, 8, out); break;
            case PieceType::King:   gen_offsets(b, s, KING,   8, out); break;
            case PieceType::Bishop: gen_slider(b, s, DIAG, out); break;
            case PieceType::Rook:   gen_slider(b, s, ORTH, out); break;
            case PieceType::Queen:  gen_slider(b, s, DIAG, out);
                                    gen_slider(b, s, ORTH, out); break;
            case PieceType::Pawn:   gen_pawn(b, s, out); break;
            default: break;
        }
    }
    gen_castling(b, out);
    return out;
}

std::vector<Move> generate_legal(const Board& b) {
    std::vector<Move> out;
    Color us = b.side_to_move;
    for (const Move& m : generate_pseudo_legal(b)) {
        if (!in_check(make_move(b, m), us))
            out.push_back(m);
    }
    return out;
}
