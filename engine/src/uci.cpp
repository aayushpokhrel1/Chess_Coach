#include "uci.hpp"
#include "movegen.hpp"
#include "types.hpp"
#include <sstream>
#include <vector>

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

namespace {
std::vector<std::string> split_ws(const std::string& s) {
    std::istringstream iss(s);
    std::vector<std::string> out;
    std::string t;
    while (iss >> t) out.push_back(t);
    return out;
}
} // namespace

std::string handle_command(UciState& state, const std::string& line) {
    std::vector<std::string> tok = split_ws(line);
    if (tok.empty()) return "";
    const std::string& cmd = tok[0];

    if (cmd == "uci")
        return "id name ChessCoach\nid author Aayush Pokhrel\nuciok";
    if (cmd == "isready")
        return "readyok";
    if (cmd == "ucinewgame") {
        state.board = start_position();
        return "";
    }
    if (cmd == "quit")
        return "";

    if (cmd == "position") {
        size_t i = 1;
        if (i < tok.size() && tok[i] == "startpos") {
            state.board = start_position();
            i++;
        } else if (i < tok.size() && tok[i] == "fen") {
            i++;
            std::string fen;
            for (int f = 0; f < 6 && i < tok.size(); f++, i++) {
                if (f) fen += " ";
                fen += tok[i];
            }
            state.board = board_from_fen(fen);
        }
        if (i < tok.size() && tok[i] == "moves") {
            i++;
            for (; i < tok.size(); i++) {
                Move m = move_from_uci(state.board, tok[i]);
                if (m.from != NO_SQUARE) make_move(state.board, m);
            }
        }
        return "";
    }

    if (cmd == "go")
        return "";   // implemented in Task 4

    return "";       // ignore unknown commands
}
