#include "doctest.h"
#include "board.hpp"
#include "eval.hpp"

TEST_CASE("material score is zero at the start") {
    CHECK(material_score(start_position()) == 0);
    CHECK(evaluate(start_position()) == 0);
}

TEST_CASE("material score counts the centipawn difference (White's view)") {
    // White king+queen vs black king: White is up a queen.
    Board upQ = board_from_fen("4k3/8/8/8/8/8/8/3QK3 w - - 0 1");
    CHECK(material_score(upQ) == 900);
    // Black king+rook vs white king: White is down a rook.
    Board downR = board_from_fen("r3k3/8/8/8/8/8/8/4K3 w - - 0 1");
    CHECK(material_score(downR) == -500);
}

TEST_CASE("evaluate is from the side-to-move perspective") {
    // Up a queen is good for White. Same position, different mover.
    Board whiteToMove = board_from_fen("4k3/8/8/8/8/8/8/3QK3 w - - 0 1");
    Board blackToMove = board_from_fen("4k3/8/8/8/8/8/8/3QK3 b - - 0 1");
    CHECK(evaluate(whiteToMove) > 0);                 // good for the mover (White)
    CHECK(evaluate(blackToMove) < 0);                 // bad for the mover (Black)
    CHECK(evaluate(whiteToMove) == -evaluate(blackToMove)); // exact negatives
}

TEST_CASE("a knight is worth more in the center than in the corner") {
    // Same material (lone white knight + kings); only the knight's square differs.
    Board center = board_from_fen("4k3/8/8/8/3N4/8/8/4K3 w - - 0 1"); // Nd4
    Board corner = board_from_fen("4k3/8/8/8/8/8/8/N3K3 w - - 0 1");   // Na1
    CHECK(evaluate(center) > evaluate(corner));
}

TEST_CASE("piece-square tables keep the start position balanced") {
    CHECK(evaluate(start_position()) == 0);
}

TEST_CASE("piece-square tables move the score off pure material") {
    // An advanced, centralized white pawn should read higher than its raw 100.
    Board b = board_from_fen("4k3/8/8/3P4/8/8/8/4K3 w - - 0 1"); // white pawn d5
    CHECK(evaluate(b) != material_score(b));
}

TEST_CASE("v2 positional terms are symmetric at the start") {
    CHECK(positional_eval(start_position()) == 0);   // mirror position, all terms cancel
}

TEST_CASE("v2 mobility rewards an active piece") {
    // Equal material (a queen each). White's queen is centralized and open, Black's
    // is boxed in the corner, so the positional term favors White.
    Board b = board_from_fen("q3k3/8/8/8/3Q4/8/8/4K3 w - - 0 1"); // Qd4 vs Qa8
    CHECK(positional_eval(b) > 0);
}

TEST_CASE("v2 rewards the bishop pair") {
    // White keeps both bishops, Black has one; positional term favors White.
    Board b = board_from_fen("2b1k3/8/8/8/8/8/8/2B1KB2 w - - 0 1"); // White Bc1,Bf1 vs Black Bc8
    CHECK(positional_eval(b) > 0);
}

TEST_CASE("v2 penalizes doubled and isolated pawns") {
    // White's a-pawns are doubled AND isolated; Black's a7/b7 are healthy. Only the
    // pawn-structure term differs (no pieces), so White reads worse positionally.
    Board b = board_from_fen("4k3/pp6/8/8/8/P7/P7/4K3 w - - 0 1"); // White a2,a3 vs Black a7,b7
    CHECK(positional_eval(b) < 0);
}

TEST_CASE("v2 rewards a passed pawn") {
    // White pawn on e6, no black pawns: a passed pawn. Toggling the passed-pawn weight
    // off must lower White's positional score, which isolates exactly this term.
    Board b = board_from_fen("4k3/8/4P3/8/8/8/8/4K3 w - - 0 1");
    int with_term = positional_eval(b);
    eval_set_weight("Passed", 0);
    int without = positional_eval(b);
    eval_set_weight("Passed", 12);   // restore the default for later tests
    CHECK(with_term > without);
}

TEST_CASE("a more advanced passed pawn is worth more") {
    // Both are passers (no black pawns); positional_eval excludes the PST, so the only
    // difference is how far each has advanced.
    Board near = board_from_fen("4k3/4P3/8/8/8/8/8/4K3 w - - 0 1"); // e7 (7th rank)
    Board far  = board_from_fen("4k3/8/8/8/4P3/8/8/4K3 w - - 0 1"); // e4 (4th rank)
    CHECK(positional_eval(near) > positional_eval(far));
}

TEST_CASE("v2 gives no passed bonus when an enemy pawn blocks the file") {
    // White e6 pawn, but a black d7 pawn covers the passer's path on the adjacent file:
    // neither side has a passer, so toggling the passed weight changes nothing.
    Board b = board_from_fen("4k3/3p4/4P3/8/8/8/8/4K3 w - - 0 1");
    int with_term = positional_eval(b);
    eval_set_weight("Passed", 0);
    int without = positional_eval(b);
    eval_set_weight("Passed", 12);
    CHECK(with_term == without);
}

// The rook-file term's contribution alone: full score minus the score with both
// rook-file weights zeroed, so every other term cancels.
static int rook_file_contrib(Board& b) {
    int on = positional_eval(b);
    eval_set_weight("RookOpen", 0);
    eval_set_weight("RookHalf", 0);
    int off = positional_eval(b);
    eval_set_weight("RookOpen", 20);   // restore defaults
    eval_set_weight("RookHalf", 10);
    return on - off;
}

TEST_CASE("v2 rewards a rook on an open file") {
    // White rook on e1, no pawns anywhere: the e-file is fully open.
    Board b = board_from_fen("4k3/8/8/8/8/8/8/4RK2 w - - 0 1");
    CHECK(rook_file_contrib(b) == 20);   // the full open-file bonus
}

TEST_CASE("an open file is worth more to a rook than a half-open file") {
    Board open = board_from_fen("4k3/8/8/8/8/8/8/4RK2 w - - 0 1");   // e-file fully open
    Board half = board_from_fen("4k3/4p3/8/8/8/8/8/4RK2 w - - 0 1"); // black e7 pawn: half-open
    CHECK(rook_file_contrib(open) > rook_file_contrib(half));         // 20 vs 10
    CHECK(rook_file_contrib(half) == 10);
}

TEST_CASE("no rook-file bonus when a friendly pawn blocks the file") {
    // White rook e1 but a white e2 pawn sits on the same file: the rook is blocked.
    Board b = board_from_fen("4k3/8/8/8/8/8/4P3/4RK2 w - - 0 1");
    CHECK(rook_file_contrib(b) == 0);
}

TEST_CASE("v2 king safety penalizes an exposed king, but only with enemy queens on") {
    // White king on g1 with NO pawn cover; Black king on g8 behind f7/g7/h7. Queens on.
    Board exposed = board_from_fen("3q2k1/5ppp/8/8/8/8/8/3Q2K1 w - - 0 1");
    CHECK(positional_eval(exposed) < 0);              // White's bare king is punished

    // Same shelter difference but the queens are gone: the term switches off, so White
    // is no longer dinged for the exposed king (endgame kings should be active).
    Board endgame = board_from_fen("6k1/5ppp/8/8/8/8/8/6K1 w - - 0 1");
    CHECK(positional_eval(endgame) > positional_eval(exposed));
}

TEST_CASE("phased eval flips the king's square preference toward the endgame") {
    // Bare kings (phase 0, full endgame). Compare a central white king to a corner one.
    Board central = board_from_fen("7k/8/8/8/4K3/8/8/8 w - - 0 1"); // white Ke4 (centre)
    Board corner  = board_from_fen("7k/8/8/8/8/8/8/K7 w - - 0 1");   // white Ka1 (corner)
    eval_set_weight("PhaseKing", 0);              // middlegame table only: tuck the king away
    CHECK(evaluate(central) < evaluate(corner));  // MG penalizes the central king
    eval_set_weight("PhaseKing", 1);              // restore phased (default)
    CHECK(evaluate(central) > evaluate(corner));  // endgame rewards centralizing the king
}
