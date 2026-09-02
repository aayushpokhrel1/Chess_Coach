#include "doctest.h"
#include "board.hpp"
#include "move.hpp"

// Apply one move in place and read back the resulting FEN.
static std::string after(const char* fen, const Move& m) {
    Board b = board_from_fen(fen);
    make_move(b, m);
    return fen_from_board(b);
}

// Make then unmake; the board must return to exactly the input FEN.
static std::string roundtrip(const char* fen, const Move& m) {
    Board b = board_from_fen(fen);
    Undo u = make_move(b, m);
    unmake_move(b, m, u);
    return fen_from_board(b);
}

TEST_CASE("to_uci formats squares and promotion") {
    CHECK(to_uci(Move{make_square(4,1), make_square(4,3),
                      PieceType::None, MoveFlag::DoublePawnPush}) == "e2e4");
    CHECK(to_uci(Move{make_square(4,6), make_square(4,7),
                      PieceType::Queen, MoveFlag::Promotion}) == "e7e8q");
}

TEST_CASE("quiet move updates side, clock, and squares") {
    // 1.Nf3 from the start position.
    Move nf3{make_square(6,0), make_square(5,2), PieceType::None, MoveFlag::Normal};
    CHECK(after("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", nf3)
          == "rnbqkbnr/pppppppp/8/8/8/5N2/PPPPPPPP/RNBQKB1R b KQkq - 1 1");
}

TEST_CASE("double pawn push sets the en passant target") {
    Move e4{make_square(4,1), make_square(4,3), PieceType::None, MoveFlag::DoublePawnPush};
    CHECK(after("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", e4)
          == "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1");
}

TEST_CASE("capture resets the halfmove clock") {
    // White pawn on e4 takes black pawn on d5.
    Move exd5{make_square(4,3), make_square(3,4), PieceType::None, MoveFlag::Normal};
    CHECK(after("rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 2", exd5)
          == "rnbqkbnr/ppp1pppp/8/3P4/8/8/PPPP1PPP/RNBQKBNR b KQkq - 0 2");
}

TEST_CASE("en passant capture removes the passed pawn") {
    // White e5xd6 e.p.; the captured black pawn sits on d5, not d6.
    Move epc{make_square(4,4), make_square(3,5), PieceType::None, MoveFlag::EnPassant};
    CHECK(after("rnbqkbnr/ppp1pppp/8/3pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 3", epc)
          == "rnbqkbnr/ppp1pppp/3P4/8/8/8/PPPP1PPP/RNBQKBNR b KQkq - 0 3");
}

TEST_CASE("promotion replaces the pawn") {
    // White pawn a7-a8 promoting to a queen.
    Move promo{make_square(0,6), make_square(0,7), PieceType::Queen, MoveFlag::Promotion};
    CHECK(after("8/P7/8/8/8/8/8/4k2K w - - 0 1", promo)
          == "Q7/8/8/8/8/8/8/4k2K b - - 0 1");
}

TEST_CASE("kingside castling moves the rook and strips rights") {
    Move ok{make_square(4,0), make_square(6,0), PieceType::None, MoveFlag::Castle};
    CHECK(after("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1", ok)
          == "r3k2r/8/8/8/8/8/8/R4RK1 b kq - 1 1");
}

TEST_CASE("moving a rook strips only its own castling right") {
    Move ra1b1{make_square(0,0), make_square(1,0), PieceType::None, MoveFlag::Normal};
    CHECK(after("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1", ra1b1)
          == "r3k2r/8/8/8/8/8/8/1R2K2R b Kkq - 1 1");
}

TEST_CASE("make then unmake restores the position exactly") {
    Move nf3{make_square(6,0), make_square(5,2), PieceType::None, MoveFlag::Normal};
    Move e4{make_square(4,1), make_square(4,3), PieceType::None, MoveFlag::DoublePawnPush};
    Move exd5{make_square(4,3), make_square(3,4), PieceType::None, MoveFlag::Normal};
    Move epc{make_square(4,4), make_square(3,5), PieceType::None, MoveFlag::EnPassant};
    Move promo{make_square(0,6), make_square(0,7), PieceType::Queen, MoveFlag::Promotion};
    Move castle{make_square(4,0), make_square(6,0), PieceType::None, MoveFlag::Castle};
    Move rookmove{make_square(0,0), make_square(1,0), PieceType::None, MoveFlag::Normal};

    const char* start = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    CHECK(roundtrip(start, nf3) == start);
    CHECK(roundtrip(start, e4)  == start);

    const char* cap = "rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 2";
    CHECK(roundtrip(cap, exd5) == cap);

    const char* ep = "rnbqkbnr/ppp1pppp/8/3pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 3";
    CHECK(roundtrip(ep, epc) == ep);

    const char* pr = "8/P7/8/8/8/8/8/4k2K w - - 0 1";
    CHECK(roundtrip(pr, promo) == pr);

    const char* cr = "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1";
    CHECK(roundtrip(cr, castle)   == cr);
    CHECK(roundtrip(cr, rookmove) == cr);
}
