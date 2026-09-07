#include "movegen.hpp"
#include "bitboard.hpp"

namespace {
inline int idx(PieceType t) { return static_cast<int>(t); }

inline bool on_board(int f, int r) { return f >= 0 && f < 8 && r >= 0 && r < 8; }

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

// Is `sq` attacked by any piece of color `by`? Bitboard version: for each piece
// kind, take the attacks FROM sq and intersect with `by`'s pieces of that kind (a
// symmetric relation, so "who attacks sq" is "what sq's attacks land on").
bool is_square_attacked(const Board& b, Square sq, Color by) {
    int e = static_cast<int>(by);
    // A `by` pawn attacks sq from where the OPPOSITE-color pawn on sq would attack.
    Color notby = (by == Color::White) ? Color::Black : Color::White;
    if (pawn_attacks(notby, sq) & b.bb[e][idx(PieceType::Pawn)])   return true;
    if (knight_attacks(sq)      & b.bb[e][idx(PieceType::Knight)]) return true;
    if (king_attacks(sq)        & b.bb[e][idx(PieceType::King)])   return true;
    uint64_t diag  = b.bb[e][idx(PieceType::Bishop)] | b.bb[e][idx(PieceType::Queen)];
    if (bishop_attacks(sq, b.occ_all) & diag) return true;
    uint64_t orth  = b.bb[e][idx(PieceType::Rook)] | b.bb[e][idx(PieceType::Queen)];
    if (rook_attacks(sq, b.occ_all) & orth)   return true;
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
    Color us = b.side_to_move; int usi = static_cast<int>(us);
    uint64_t own = b.occ[usi];

    // Knights
    { uint64_t bbp = b.bb[usi][1]; while(bbp){ Square from=pop_lsb(bbp); uint64_t t = knight_attacks(from) & ~own; while(t){ Square to=pop_lsb(t); out.push_back(Move{from,to,PieceType::None,MoveFlag::Normal}); } } }
    // King
    { uint64_t bbp = b.bb[usi][5]; while(bbp){ Square from=pop_lsb(bbp); uint64_t t = king_attacks(from) & ~own; while(t){ Square to=pop_lsb(t); out.push_back(Move{from,to,PieceType::None,MoveFlag::Normal}); } } }
    // Bishops
    { uint64_t bbp = b.bb[usi][2]; while(bbp){ Square from=pop_lsb(bbp); uint64_t t = bishop_attacks(from, b.occ_all) & ~own; while(t){ Square to=pop_lsb(t); out.push_back(Move{from,to,PieceType::None,MoveFlag::Normal}); } } }
    // Rooks
    { uint64_t bbp = b.bb[usi][3]; while(bbp){ Square from=pop_lsb(bbp); uint64_t t = rook_attacks(from, b.occ_all) & ~own; while(t){ Square to=pop_lsb(t); out.push_back(Move{from,to,PieceType::None,MoveFlag::Normal}); } } }
    // Queens
    { uint64_t bbp = b.bb[usi][4]; while(bbp){ Square from=pop_lsb(bbp); uint64_t t = queen_attacks(from, b.occ_all) & ~own; while(t){ Square to=pop_lsb(t); out.push_back(Move{from,to,PieceType::None,MoveFlag::Normal}); } } }
    // Pawns (reuse existing gen_pawn)
    { uint64_t bbp = b.bb[usi][0]; while(bbp){ Square from=pop_lsb(bbp); gen_pawn(b, from, out); } }

    gen_castling(b, out);
    return out;
}

std::vector<Move> generate_legal(Board& b) {
    std::vector<Move> out;
    Color us = b.side_to_move;
    for (const Move& m : generate_pseudo_legal(b)) {
        Undo u = make_move(b, m);
        if (!in_check(b, us))
            out.push_back(m);
        unmake_move(b, m, u);
    }
    return out;
}
