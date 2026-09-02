#include "eval.hpp"

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

int evaluate(const Board& b) {
    int score = material_score(b);
    return (b.side_to_move == Color::White) ? score : -score;
}
