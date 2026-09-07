#include "doctest.h"
#include "board.hpp"
#include "bitboard.hpp"

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
