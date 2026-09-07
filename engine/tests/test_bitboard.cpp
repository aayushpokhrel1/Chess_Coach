#include "doctest.h"
#include "board.hpp"
#include "bitboard.hpp"
#include "move.hpp"
#include "movegen.hpp"

static int square_count(const Board& b) {
    int n = 0;
    for (Square s = 0; s < 64; s++)
        if (b.squares[s].type != PieceType::None) n++;
    return n;
}

TEST_CASE("bitboards match squares[] after FEN parse") {
    const char* fens[] = {
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",             // start
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", // Kiwipete
        "rnbqkbnr/pp1ppppp/8/2pP4/8/8/PPP1PPPP/RNBQKBNR w KQkq c6 0 3",         // en-passant set
    };
    for (const char* f : fens) {
        Board b = board_from_fen(f);
        CHECK(bb_matches_squares(b));                 // the two views agree
        CHECK(popcount(b.occ_all) == square_count(b)); // and count the same pieces
    }
}

static Move find_move(Board& b, const std::string& uci) {
    for (const Move& m : generate_legal(b))
        if (to_uci(m) == uci) return m;
    return Move{};
}

TEST_CASE("bitboards stay in sync through make/unmake, all move types") {
    struct Case { const char* fen; const char* mv; };
    Case cases[] = {
        {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", "e2e4"},        // double push
        {"rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 2", "e4d5"},   // capture
        {"rnbqkbnr/ppp1p1pp/8/3pPp2/8/8/PPPP1PPP/RNBQKBNR w KQkq f6 0 3", "e5f6"},   // en passant
        {"r3k2r/pppppppp/8/8/8/8/PPPPPPPP/R3K2R w KQkq - 0 1", "e1g1"},              // castle O-O
        {"rnbqkbnr/ppPppppp/8/8/8/8/PP1PPPPP/RNBQKBNR w KQkq - 0 1", "c7b8q"},       // promo capture
    };
    for (auto& c : cases) {
        Board b = board_from_fen(c.fen);
        std::string before = fen_from_board(b);
        Move m = find_move(b, c.mv);
        REQUIRE(m.from != NO_SQUARE);
        Undo u = make_move(b, m);
        CHECK(bb_matches_squares(b));        // synced after make
        unmake_move(b, m, u);
        CHECK(bb_matches_squares(b));        // synced after unmake
        CHECK(fen_from_board(b) == before);  // full round-trip
    }
}
