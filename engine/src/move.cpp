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

Board make_move(const Board& b, const Move& m) {
    Board nb = b; // copy-make: work on a fresh board
    Piece moving = nb.squares[m.from];
    Color us = moving.color;
    Color them = (us == Color::White) ? Color::Black : Color::White;

    bool capture = nb.squares[m.to].type != PieceType::None
                || m.flag == MoveFlag::EnPassant;
    bool pawn_move = moving.type == PieceType::Pawn;

    // Move the piece.
    nb.squares[m.to] = moving;
    nb.squares[m.from] = Piece{Color::None, PieceType::None};

    // En passant: remove the pawn that sits one rank behind the destination.
    if (m.flag == MoveFlag::EnPassant) {
        int behind = (us == Color::White) ? -1 : 1;
        Square cap = make_square(file_of(m.to), rank_of(m.to) + behind);
        nb.squares[cap] = Piece{Color::None, PieceType::None};
    }

    // Promotion: swap the pawn for the chosen piece.
    if (m.flag == MoveFlag::Promotion) {
        nb.squares[m.to] = Piece{us, m.promotion};
    }

    // Castling: relocate the rook to the far side of the king.
    if (m.flag == MoveFlag::Castle) {
        int r = rank_of(m.to);
        if (file_of(m.to) == 6) {            // kingside: rook h -> f
            nb.squares[make_square(5, r)] = nb.squares[make_square(7, r)];
            nb.squares[make_square(7, r)] = Piece{Color::None, PieceType::None};
        } else {                             // queenside (file 2): rook a -> d
            nb.squares[make_square(3, r)] = nb.squares[make_square(0, r)];
            nb.squares[make_square(0, r)] = Piece{Color::None, PieceType::None};
        }
    }

    // En passant target: set on a double push, cleared otherwise.
    if (m.flag == MoveFlag::DoublePawnPush) {
        int behind = (us == Color::White) ? -1 : 1;
        nb.en_passant = make_square(file_of(m.to), rank_of(m.to) + behind);
    } else {
        nb.en_passant = NO_SQUARE;
    }

    // Castling rights: strip when a king/rook leaves home, or a home rook is captured.
    auto strip = [&](Square s) {
        if (s == make_square(4, 0)) nb.castling_rights &= ~(CASTLE_WK | CASTLE_WQ);
        if (s == make_square(0, 0)) nb.castling_rights &= ~CASTLE_WQ;
        if (s == make_square(7, 0)) nb.castling_rights &= ~CASTLE_WK;
        if (s == make_square(4, 7)) nb.castling_rights &= ~(CASTLE_BK | CASTLE_BQ);
        if (s == make_square(0, 7)) nb.castling_rights &= ~CASTLE_BQ;
        if (s == make_square(7, 7)) nb.castling_rights &= ~CASTLE_BK;
    };
    strip(m.from);
    strip(m.to);

    nb.halfmove_clock = (capture || pawn_move) ? 0 : nb.halfmove_clock + 1;
    if (us == Color::Black) nb.fullmove_number += 1;
    nb.side_to_move = them;
    return nb;
}