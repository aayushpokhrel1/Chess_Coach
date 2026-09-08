#include "doctest.h"
#include "board.hpp"
#include "bitboard.hpp"
#include "move.hpp"
#include "movegen.hpp"
#include <random>

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

TEST_CASE("magic slider lookup matches the ray-scan reference") {
    // The whole point of magics: the table lookup must return exactly what the old
    // ray scan did, for every square and every blocker layout. Check all 64 squares
    // against a spread of occupancies (empty, full, and pseudo-random), so any bad
    // magic, mask, or index shows up here rather than as a subtle perft miscount.
    std::mt19937_64 rng(0x1234ABCDULL);
    for (Square s = 0; s < 64; s++) {
        CHECK(rook_attacks(s, 0) == rook_attacks_ref(s, 0));
        CHECK(bishop_attacks(s, 0) == bishop_attacks_ref(s, 0));
        uint64_t full = ~0ULL;
        CHECK(rook_attacks(s, full) == rook_attacks_ref(s, full));
        CHECK(bishop_attacks(s, full) == bishop_attacks_ref(s, full));
        for (int t = 0; t < 200; t++) {
            uint64_t occ = rng() & rng();   // sparse-ish random boards
            CHECK(rook_attacks(s, occ) == rook_attacks_ref(s, occ));
            CHECK(bishop_attacks(s, occ) == bishop_attacks_ref(s, occ));
        }
    }
}

TEST_CASE("slider and leaper attacks") {
    // Rook on a1, empty board: whole a-file + rank 1 = 14 squares.
    CHECK(popcount(rook_attacks(make_square(0, 0), 0)) == 14);

    // Blocker on a4: the a-file attack stops at a4 (inclusive), a5..a8 drop off;
    // the rank is still clear out to h1.
    uint64_t occ = 1ULL << make_square(0, 3);            // a4
    uint64_t rat = rook_attacks(make_square(0, 0), occ);
    CHECK(bb_get(rat, make_square(0, 3)));               // a4 attacked (capturable)
    CHECK(!bb_get(rat, make_square(0, 4)));              // a5 shielded behind it
    CHECK(bb_get(rat, make_square(7, 0)));               // h1 still reachable

    // Bishop on d4 with a blocker on f6: the NE ray stops at f6.
    uint64_t occ2 = 1ULL << make_square(5, 5);           // f6
    uint64_t bat = bishop_attacks(make_square(3, 3), occ2);
    CHECK(bb_get(bat, make_square(5, 5)));               // f6 (capturable)
    CHECK(!bb_get(bat, make_square(6, 6)));              // g7 shielded

    // Knight on b1 hits exactly a3, c3, d2.
    uint64_t nat = knight_attacks(make_square(1, 0));
    CHECK(popcount(nat) == 3);
    CHECK(bb_get(nat, make_square(0, 2)));               // a3
    CHECK(bb_get(nat, make_square(2, 2)));               // c3
    CHECK(bb_get(nat, make_square(3, 1)));               // d2

    // A white pawn on d4 attacks c5 and e5; a black pawn on d4 attacks c3 and e3.
    CHECK(pawn_attacks(Color::White, make_square(3, 3))
          == ((1ULL << make_square(2, 4)) | (1ULL << make_square(4, 4))));
    CHECK(pawn_attacks(Color::Black, make_square(3, 3))
          == ((1ULL << make_square(2, 2)) | (1ULL << make_square(4, 2))));
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
