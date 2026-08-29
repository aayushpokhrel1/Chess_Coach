#include "board.hpp"
#include <sstream>

Board board_from_fen(const std::string& fen) {
    Board b;
    for (auto& sq : b.squares) sq = Piece{Color::None, PieceType::None};

    std::istringstream iss(fen);
    std::string placement, side, castling, ep;
    int halfmove = 0, fullmove = 1;
    iss >> placement >> side >> castling >> ep >> halfmove >> fullmove;

    int rank = 7, file = 0;
    for (char c : placement) {
        if (c == '/') { rank--; file = 0; }
        else if (c >= '1' && c <= '8') { file += c - '0'; }
        else { b.squares[make_square(file, rank)] = piece_from_char(c); file++; }
    }

    b.side_to_move = (side == "w") ? Color::White : Color::Black;

    b.castling_rights = 0;
    if (castling != "-") {
        for (char c : castling) {
            if (c == 'K') b.castling_rights |= CASTLE_WK;
            else if (c == 'Q') b.castling_rights |= CASTLE_WQ;
            else if (c == 'k') b.castling_rights |= CASTLE_BK;
            else if (c == 'q') b.castling_rights |= CASTLE_BQ;
        }
    }

    b.en_passant = (ep == "-")
        ? NO_SQUARE
        : make_square(ep[0] - 'a', ep[1] - '1');

    b.halfmove_clock = halfmove;
    b.fullmove_number = fullmove;
    return b;
}

std::string fen_from_board(const Board& b) {
    std::string fen;
    for (int rank = 7; rank >= 0; rank--) {
        int empty = 0;
        for (int file = 0; file < 8; file++) {
            Piece p = b.squares[make_square(file, rank)];
            if (p.type == PieceType::None) {
                empty++;
            } else {
                if (empty > 0) { fen += std::to_string(empty); empty = 0; }
                fen += char_from_piece(p);
            }
        }
        if (empty > 0) fen += std::to_string(empty);
        if (rank > 0) fen += '/';
    }

    fen += ' ';
    fen += (b.side_to_move == Color::White) ? 'w' : 'b';

    fen += ' ';
    std::string cr;
    if (b.castling_rights & CASTLE_WK) cr += 'K';
    if (b.castling_rights & CASTLE_WQ) cr += 'Q';
    if (b.castling_rights & CASTLE_BK) cr += 'k';
    if (b.castling_rights & CASTLE_BQ) cr += 'q';
    fen += cr.empty() ? "-" : cr;

    fen += ' ';
    if (b.en_passant == NO_SQUARE) {
        fen += '-';
    } else {
        fen += static_cast<char>('a' + file_of(b.en_passant));
        fen += static_cast<char>('1' + rank_of(b.en_passant));
    }

    fen += ' ';
    fen += std::to_string(b.halfmove_clock);
    fen += ' ';
    fen += std::to_string(b.fullmove_number);
    return fen;
}

Board start_position() {
    return board_from_fen(
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}

std::string to_ascii(const Board& b) {
    std::string out;
    for (int rank = 7; rank >= 0; rank--) {
        for (int file = 0; file < 8; file++) {
            out += char_from_piece(b.squares[make_square(file, rank)]);
        }
        out += '\n';
    }
    return out;
}
