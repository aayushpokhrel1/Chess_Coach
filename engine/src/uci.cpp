#include "uci.hpp"
#include "movegen.hpp"
#include "types.hpp"

Move move_from_uci(Board& b, const std::string& uci) {
    if (uci.size() < 4) return Move{};
    int ff = uci[0] - 'a', fr = uci[1] - '1';
    int tf = uci[2] - 'a', tr = uci[3] - '1';
    if (ff < 0 || ff > 7 || fr < 0 || fr > 7 ||
        tf < 0 || tf > 7 || tr < 0 || tr > 7) return Move{};
    Square from = make_square(ff, fr);
    Square to   = make_square(tf, tr);

    PieceType promo = PieceType::None;
    if (uci.size() >= 5) {
        switch (uci[4]) {
            case 'q': promo = PieceType::Queen;  break;
            case 'r': promo = PieceType::Rook;   break;
            case 'b': promo = PieceType::Bishop; break;
            case 'n': promo = PieceType::Knight; break;
            default: break;
        }
    }

    for (const Move& m : generate_legal(b)) {
        if (m.from != from || m.to != to) continue;
        if (m.flag == MoveFlag::Promotion) {
            if (m.promotion == promo) return m;   // match the promotion piece
        } else {
            return m;
        }
    }
    return Move{};   // no legal move matched
}
