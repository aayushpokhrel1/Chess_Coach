#include "types.hpp"
#include <cctype>

Piece piece_from_char(char c) {
    Color color = std::isupper(static_cast<unsigned char>(c)) ? Color::White : Color::Black;
    PieceType type;
    switch (std::tolower(static_cast<unsigned char>(c))) {
        case 'p': type = PieceType::Pawn;   break;
        case 'n': type = PieceType::Knight; break;
        case 'b': type = PieceType::Bishop; break;
        case 'r': type = PieceType::Rook;   break;
        case 'q': type = PieceType::Queen;  break;
        case 'k': type = PieceType::King;   break;
        default:  return Piece{Color::None, PieceType::None};
    }
    return Piece{color, type};
}

char char_from_piece(Piece p) {
    char c;
    switch (p.type) {
        case PieceType::Pawn:   c = 'p'; break;
        case PieceType::Knight: c = 'n'; break;
        case PieceType::Bishop: c = 'b'; break;
        case PieceType::Rook:   c = 'r'; break;
        case PieceType::Queen:  c = 'q'; break;
        case PieceType::King:   c = 'k'; break;
        default: return '.';
    }
    return p.color == Color::White ? static_cast<char>(std::toupper(c)) : c;
}
