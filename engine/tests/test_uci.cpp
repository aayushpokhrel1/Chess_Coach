#include "doctest.h"
#include "board.hpp"
#include "move.hpp"
#include "uci.hpp"

TEST_CASE("move_from_uci parses a normal move") {
    Board b = start_position();
    Move m = move_from_uci(b, "e2e4");
    CHECK(m.from == make_square(4, 1)); // e2
    CHECK(m.to   == make_square(4, 3)); // e4
}

TEST_CASE("move_from_uci parses a promotion") {
    Board b = board_from_fen("4k3/P7/8/8/8/8/8/4K3 w - - 0 1"); // white pawn a7
    Move m = move_from_uci(b, "a7a8q");
    CHECK(m.from == make_square(0, 6)); // a7
    CHECK(m.to   == make_square(0, 7)); // a8
    CHECK(m.promotion == PieceType::Queen);
}

TEST_CASE("move_from_uci rejects an illegal move") {
    Board b = start_position();
    Move m = move_from_uci(b, "e2e5"); // not a legal first move
    CHECK(m.from == NO_SQUARE);
}

TEST_CASE("uci command replies with id and uciok") {
    UciState s;
    std::string r = handle_command(s, "uci");
    CHECK(r.find("id name") != std::string::npos);
    CHECK(r.find("uciok")   != std::string::npos);
}

TEST_CASE("isready replies readyok") {
    UciState s;
    CHECK(handle_command(s, "isready") == "readyok");
}

TEST_CASE("position startpos with moves updates the board") {
    UciState s;
    handle_command(s, "position startpos moves e2e4 e7e5");
    std::string fen = fen_from_board(s.board);
    // Piece placement after 1.e4 e5 (ignore the fields after the first space).
    CHECK(fen.rfind("rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR", 0) == 0);
}

TEST_CASE("position fen sets the board") {
    UciState s;
    handle_command(s, "position fen 4k3/8/8/8/8/8/8/4K3 w - - 0 1");
    std::string fen = fen_from_board(s.board);
    CHECK(fen.rfind("4k3/8/8/8/8/8/8/4K3", 0) == 0);
}

TEST_CASE("budget_for_clock splits the clock sanely") {
    CHECK(budget_for_clock(600000, 0) == 30000); // 10 min -> ~30s
    CHECK(budget_for_clock(60000, 1000) == 3500);  // 60s + 1s inc -> 3000 + 500
    CHECK(budget_for_clock(20, 0) >= 1);           // tiny clock still yields >= 1ms
}

TEST_CASE("go emits an info line with score and pv, then bestmove") {
    UciState s;
    handle_command(s, "position startpos");
    std::string r = handle_command(s, "go depth 3");
    CHECK(r.find("info ")     != std::string::npos);
    CHECK(r.find("score cp")  != std::string::npos);
    CHECK(r.find(" pv ")      != std::string::npos);
    CHECK(r.find("bestmove ") != std::string::npos);
}

TEST_CASE("go reports a mate score as score mate") {
    UciState s;
    // White: Rh1, Rg7 seals rank 7; Rh1-h8 is mate in one.
    handle_command(s, "position fen k7/6R1/8/8/8/8/8/K6R w - - 0 1");
    std::string r = handle_command(s, "go depth 2");
    CHECK(r.find("score mate") != std::string::npos);
}

TEST_CASE("go depth returns a legal bestmove") {
    UciState s;
    handle_command(s, "position startpos");
    std::string r = handle_command(s, "go depth 2");
    size_t bm = r.find("bestmove ");
    REQUIRE(bm != std::string::npos);
    std::string mv = r.substr(bm + 9);
    mv = mv.substr(0, mv.find_first_of(" \n"));   // first token after "bestmove "
    Move m = move_from_uci(s.board, mv);
    CHECK(m.from != NO_SQUARE);   // the printed move is legal
}

TEST_CASE("go movetime returns a bestmove quickly") {
    UciState s;
    handle_command(s, "position startpos");
    std::string r = handle_command(s, "go movetime 50");
    CHECK(r.find("bestmove ") != std::string::npos);
}
