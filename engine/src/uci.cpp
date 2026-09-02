#include "uci.hpp"
#include "movegen.hpp"
#include "types.hpp"
#include "search.hpp"
#include <sstream>
#include <vector>
#include <cstdlib>

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

long long budget_for_clock(long long remaining_ms, long long inc_ms) {
    // ponytail: crude 1/20th-of-clock split plus half the increment; tune only
    //           if the engine flags or dawdles in real games.
    long long budget = remaining_ms / 20 + inc_ms / 2;
    long long cap = remaining_ms - 30;   // keep a safety margin, never spend it all
    if (cap < 1) cap = 1;
    if (budget > cap) budget = cap;
    if (budget < 1) budget = 1;
    return budget;
}

namespace {
std::vector<std::string> split_ws(const std::string& s) {
    std::istringstream iss(s);
    std::vector<std::string> out;
    std::string t;
    while (iss >> t) out.push_back(t);
    return out;
}

SearchLimits compute_limits(const Board& b, const std::vector<std::string>& tok) {
    SearchLimits lim;
    long long movetime = 0, wtime = 0, btime = 0, winc = 0, binc = 0;
    int depth = 0;
    bool infinite = false;
    for (size_t i = 1; i < tok.size(); i++) {
        if (tok[i] == "infinite") { infinite = true; continue; }
        if (i + 1 >= tok.size()) continue;
        const std::string& v = tok[i + 1];
        if      (tok[i] == "movetime") movetime = std::atoll(v.c_str());
        else if (tok[i] == "wtime")    wtime    = std::atoll(v.c_str());
        else if (tok[i] == "btime")    btime    = std::atoll(v.c_str());
        else if (tok[i] == "winc")     winc     = std::atoll(v.c_str());
        else if (tok[i] == "binc")     binc     = std::atoll(v.c_str());
        else if (tok[i] == "depth")    depth    = std::atoi(v.c_str());
    }

    if (depth > 0) { lim.max_depth = depth; lim.budget_ms = 0; return lim; } // fixed depth
    if (infinite)  { lim.max_depth = 64;    lim.budget_ms = 0; return lim; } // depth cap only
    if (movetime > 0) {
        lim.budget_ms = movetime > 10 ? movetime - 5 : movetime;  // small safety margin
        return lim;
    }
    long long remaining = (b.side_to_move == Color::White) ? wtime : btime;
    long long inc       = (b.side_to_move == Color::White) ? winc  : binc;
    if (remaining > 0) { lim.budget_ms = budget_for_clock(remaining, inc); return lim; }

    lim.max_depth = 5;   // nothing specified: a safe default depth
    lim.budget_ms = 0;
    return lim;
}

std::string handle_go(UciState& state, const std::vector<std::string>& tok) {
    SearchLimits lim = compute_limits(state.board, tok);
    SearchResult r = search_timed(state.board, lim);
    if (r.best.from == NO_SQUARE) return "bestmove 0000";   // no legal move
    return "bestmove " + to_uci(r.best);
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
        return handle_go(state, tok);

    return "";       // ignore unknown commands
}
