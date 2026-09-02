#include "move.hpp"
#include "types.hpp"

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

    bool capture = b.squares[m.to].type != PieceType::None
                || m.flag == MoveFlag::EnPassant;
    bool pawn_move = moving.type == PieceType::Pawn;

    // Move the piece.
    b.squares[m.to] = moving;
    b.squares[m.from] = Piece{Color::None, PieceType::None};

    // En passant: remove (and record) the pawn one rank behind the destination.
    if (m.flag == MoveFlag::EnPassant) {
        int behind = (us == Color::White) ? -1 : 1;
        Square cap = make_square(file_of(m.to), rank_of(m.to) + behind);
        u.captured = b.squares[cap];     // the pawn taken en passant
        b.squares[cap] = Piece{Color::None, PieceType::None};
    }

    // Promotion: swap the pawn for the chosen piece.
    if (m.flag == MoveFlag::Promotion) {
        b.squares[m.to] = Piece{us, m.promotion};
    }

    // Castling: relocate the rook.
    if (m.flag == MoveFlag::Castle) {
        int r = rank_of(m.to);
        if (file_of(m.to) == 6) {        // kingside: rook h -> f
            b.squares[make_square(5, r)] = b.squares[make_square(7, r)];
            b.squares[make_square(7, r)] = Piece{Color::None, PieceType::None};
        } else {                         // queenside: rook a -> d
            b.squares[make_square(3, r)] = b.squares[make_square(0, r)];
            b.squares[make_square(0, r)] = Piece{Color::None, PieceType::None};
        }
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

    // Move the piece back; a promotion returns to a pawn.
    Piece moved = b.squares[m.to];
    if (m.flag == MoveFlag::Promotion) {
        b.squares[m.from] = Piece{us, PieceType::Pawn};
    } else {
        b.squares[m.from] = moved;
    }
    b.squares[m.to] = Piece{Color::None, PieceType::None};

    // Restore the captured piece.
    if (m.flag == MoveFlag::EnPassant) {
        int behind = (us == Color::White) ? -1 : 1;
        Square cap = make_square(file_of(m.to), rank_of(m.to) + behind);
        b.squares[cap] = u.captured;     // the pawn taken en passant; `to` stays empty
    } else {
        b.squares[m.to] = u.captured;    // empty piece if the move was not a capture
    }

    // Undo the castle's rook move.
    if (m.flag == MoveFlag::Castle) {
        int r = rank_of(m.to);
        if (file_of(m.to) == 6) {        // rook f -> h
            b.squares[make_square(7, r)] = b.squares[make_square(5, r)];
            b.squares[make_square(5, r)] = Piece{Color::None, PieceType::None};
        } else {                         // rook d -> a
            b.squares[make_square(0, r)] = b.squares[make_square(3, r)];
            b.squares[make_square(3, r)] = Piece{Color::None, PieceType::None};
        }
    }
}
