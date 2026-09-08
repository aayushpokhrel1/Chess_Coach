#include "move.hpp"
#include "types.hpp"
#include "bitboard.hpp"

std::string to_uci(const Move& m) {
    std::string s;
    s += static_cast<char>('a' + file_of(m.from));
    s += static_cast<char>('1' + rank_of(m.from));
    s += static_cast<char>('a' + file_of(m.to));
    s += static_cast<char>('1' + rank_of(m.to));
    if (m.flag == MoveFlag::Promotion) {
        s += char_from_piece(Piece{Color::Black, m.promotion}); // lowercase letter
    }
    return s;
}

Undo make_move(Board& b, const Move& m) {
    Piece moving = b.squares[m.from];
    Color us = moving.color;
    Color them = (us == Color::White) ? Color::Black : Color::White;

    // Save what unmake cannot reconstruct.
    Undo u;
    u.captured = b.squares[m.to];        // empty for a quiet move
    u.castling_rights = b.castling_rights;
    u.en_passant = b.en_passant;
    u.halfmove_clock = b.halfmove_clock;
    u.fullmove_number = b.fullmove_number;
    u.hash = b.hash;   // pre-move key; unmake restores it, so unmake can never drift

    bool capture = b.squares[m.to].type != PieceType::None
                || m.flag == MoveFlag::EnPassant;
    bool pawn_move = moving.type == PieceType::Pawn;

    // Move the piece (helpers keep squares[] and the bitboards in sync). Clear a
    // normal captured piece off the destination first, so move_piece lands on empty.
    if (m.flag != MoveFlag::EnPassant && b.squares[m.to].type != PieceType::None)
        remove_piece(b, m.to);
    move_piece(b, m.from, m.to);

    // En passant: remove (and record) the pawn one rank behind the destination.
    if (m.flag == MoveFlag::EnPassant) {
        int behind = (us == Color::White) ? -1 : 1;
        Square cap = make_square(file_of(m.to), rank_of(m.to) + behind);
        u.captured = b.squares[cap];     // the pawn taken en passant
        remove_piece(b, cap);
    }

    // Promotion: swap the pawn we just moved for the chosen piece.
    if (m.flag == MoveFlag::Promotion) {
        remove_piece(b, m.to);
        add_piece(b, m.to, Piece{us, m.promotion});
    }

    // Castling: relocate the rook.
    if (m.flag == MoveFlag::Castle) {
        int r = rank_of(m.to);
        if (file_of(m.to) == 6)          // kingside: rook h -> f
            move_piece(b, make_square(7, r), make_square(5, r));
        else                             // queenside: rook a -> d
            move_piece(b, make_square(0, r), make_square(3, r));
    }

    // En passant target: set on a double push, cleared otherwise.
    if (m.flag == MoveFlag::DoublePawnPush) {
        int behind = (us == Color::White) ? -1 : 1;
        b.en_passant = make_square(file_of(m.to), rank_of(m.to) + behind);
    } else {
        b.en_passant = NO_SQUARE;
    }

    // Castling rights: strip when a king/rook leaves home or a home rook is captured.
    auto strip = [&](Square s) {
        if (s == make_square(4, 0)) b.castling_rights &= ~(CASTLE_WK | CASTLE_WQ);
        if (s == make_square(0, 0)) b.castling_rights &= ~CASTLE_WQ;
        if (s == make_square(7, 0)) b.castling_rights &= ~CASTLE_WK;
        if (s == make_square(4, 7)) b.castling_rights &= ~(CASTLE_BK | CASTLE_BQ);
        if (s == make_square(0, 7)) b.castling_rights &= ~CASTLE_BQ;
        if (s == make_square(7, 7)) b.castling_rights &= ~CASTLE_BK;
    };
    strip(m.from);
    strip(m.to);

    // Incremental hash, non-piece terms. The piece-square terms were already XORed
    // inside the move helpers (add/remove/move_piece); here we fold in what they do
    // not touch: the side flip, any castling-right bits just stripped, and the
    // en-passant file leaving/arriving. compute_hash stays the oracle the test checks.
    b.hash ^= zobrist_side();
    int cast_changed = u.castling_rights ^ b.castling_rights;
    for (int i = 0; i < 4; i++)
        if (cast_changed & (1 << i)) b.hash ^= zobrist_castle_bit(i);
    if (u.en_passant != NO_SQUARE) b.hash ^= zobrist_ep_file(file_of(u.en_passant));
    if (b.en_passant != NO_SQUARE) b.hash ^= zobrist_ep_file(file_of(b.en_passant));

    b.halfmove_clock = (capture || pawn_move) ? 0 : b.halfmove_clock + 1;
    if (us == Color::Black) b.fullmove_number += 1;
    b.side_to_move = them;
    return u;
}

void unmake_move(Board& b, const Move& m, const Undo& u) {
    // side_to_move currently points at the opponent; the mover is the other color.
    Color us = (b.side_to_move == Color::White) ? Color::Black : Color::White;

    // Restore the saved scalars.
    b.side_to_move = us;
    b.castling_rights = u.castling_rights;
    b.en_passant = u.en_passant;
    b.halfmove_clock = u.halfmove_clock;
    b.fullmove_number = u.fullmove_number;

    // Move the piece back; a promotion returns to a pawn. (to is emptied here; any
    // captured piece is restored onto it next.)
    if (m.flag == MoveFlag::Promotion) {
        remove_piece(b, m.to);                          // the promoted piece
        add_piece(b, m.from, Piece{us, PieceType::Pawn});
    } else {
        move_piece(b, m.to, m.from);
    }

    // Restore the captured piece.
    if (m.flag == MoveFlag::EnPassant) {
        int behind = (us == Color::White) ? -1 : 1;
        Square cap = make_square(file_of(m.to), rank_of(m.to) + behind);
        add_piece(b, cap, u.captured);   // the pawn taken en passant; `to` stays empty
    } else if (u.captured.type != PieceType::None) {
        add_piece(b, m.to, u.captured);  // put the captured piece back on the destination
    }

    // Undo the castle's rook move.
    if (m.flag == MoveFlag::Castle) {
        int r = rank_of(m.to);
        if (file_of(m.to) == 6)          // rook f -> h
            move_piece(b, make_square(5, r), make_square(7, r));
        else                             // rook d -> a
            move_piece(b, make_square(3, r), make_square(0, r));
    }

    // The move helpers above re-toggled the piece terms as they moved pieces back,
    // but the exact old key (piece + side + castling + ep) is already saved, so just
    // restore it. This is why unmake cannot drift out of step with compute_hash.
    b.hash = u.hash;
}
