#include "doctest.h"
#include "board.hpp"
#include "move.hpp"
#include "movegen.hpp"
#include "search.hpp"

TEST_CASE("search grabs a free queen") {
    // White rook on d1 can capture an undefended black queen on d3.
    Board b = board_from_fen("4k3/8/8/8/8/3q4/8/3RK3 w - - 0 1");
    SearchResult r = search(b, 2);
    CHECK(r.best.from == make_square(3, 0)); // d1
    CHECK(r.best.to   == make_square(3, 2)); // d3
    CHECK(r.score > 400);                    // clearly ahead (a rook up after being down)
}

TEST_CASE("search finds mate in one") {
    // White: rook on h1, rook on g7 holds rank 7; Rh1-h8 is mate.
    Board b = board_from_fen("k7/6R1/8/8/8/8/8/K6R w - - 0 1");
    SearchResult r = search(b, 2);
    CHECK(r.best.from == make_square(7, 0)); // h1
    CHECK(r.best.to   == make_square(7, 7)); // h8
    CHECK(r.score > 29000);                  // a mate score
}

TEST_CASE("stalemate scores zero and reports no move") {
    // Black to move, king h8 has no legal move and is not in check.
    Board b = board_from_fen("7k/5Q2/6K1/8/8/8/8/8 b - - 0 1");
    SearchResult r = search(b, 3);
    CHECK(r.score == 0);
    CHECK(r.best.from == NO_SQUARE);
}

TEST_CASE("alpha-beta returns the same value as full-width minimax, with fewer nodes") {
    // A busy midgame position (after 1.e4 e5) so pruning has something to cut.
    Board b = board_from_fen("rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2");

    // Isolate alpha-beta here: the TT (deeper leaf scores via transpositions) and
    // null-move pruning (a forward-pruning heuristic) both legitimately shift a
    // fixed-depth value. Those are the optimisations working, not a pruning bug, so
    // turn both off for this exact-equality check; killer/history stay on because
    // they only reorder moves and never change the value.
    search_use_tt(false);
    search_use_null(false);
    int full = search_minimax(b, 3);
    long full_nodes = nodes_searched();

    int pruned = search(b, 3).score;
    long pruned_nodes = nodes_searched();
    search_use_tt(true);
    search_use_null(true);

    CHECK(pruned == full);              // pruning must not change the value
    CHECK(pruned_nodes < full_nodes);   // but it must visit fewer nodes
}

TEST_CASE("the transposition table cuts nodes at a fixed depth") {
    Board b = board_from_fen("rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2");

    search_use_tt(false);
    search(b, 4);
    long without = nodes_searched();

    search_use_tt(true);
    search(b, 4);
    long with = nodes_searched();

    CHECK(with < without);   // reusing searched positions (and TT-move ordering) saves nodes
}

TEST_CASE("killer and history ordering cuts nodes at a fixed depth") {
    Board b = board_from_fen("rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2");

    // Isolate ordering from the TT (both cut nodes). With the TT off, the only
    // variable is quiet-move ordering, so any node drop is the heuristics working.
    search_use_tt(false);

    search_use_order_heur(false);
    search(b, 5);
    long without = nodes_searched();

    search_use_order_heur(true);
    search(b, 5);
    long with = nodes_searched();

    search_use_tt(true);
    search_use_order_heur(true);

    CHECK(with < without);   // trying killers / high-history quiets first prunes more
}

TEST_CASE("null-move pruning cuts nodes at a fixed depth") {
    Board b = board_from_fen("rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2");

    // Isolate null-move from the TT (both prune). With the TT off, the node drop
    // is null-move pruning giving up its free tempo and still failing high.
    search_use_tt(false);

    search_use_null(false);
    search(b, 5);
    long without = nodes_searched();

    search_use_null(true);
    search(b, 5);
    long with = nodes_searched();

    search_use_tt(true);
    search_use_null(true);

    CHECK(with < without);   // pruning nodes where even a free pass beats beta
}

TEST_CASE("search returns a principal variation starting with the best move") {
    Board b = board_from_fen("4k3/8/8/8/8/3q4/8/3RK3 w - - 0 1"); // Rd1xd3 wins the queen
    SearchResult r = search(b, 3);
    REQUIRE(r.pv.size() >= 1);
    CHECK(r.pv[0].from == r.best.from);
    CHECK(r.pv[0].to   == r.best.to);
    CHECK(r.depth == 3);
}

TEST_CASE("iterative deepening reaches the same evaluation as a single fixed-depth search") {
    Board b = board_from_fen("rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2");
    SearchResult id    = search(b, 3);
    SearchResult fixed = search_to_depth(b, 3);
    // Score is the real invariant. The exact best move need NOT match: the killer /
    // history heuristics make ID (which accumulates them over depths 1..3) and a lone
    // depth-3 pass order moves differently, so among several equally-optimal moves the
    // tie can break either way. Both are still optimal, which the equal score proves.
    CHECK(id.score == fixed.score);
}

TEST_CASE("search finds mate in two") {
    // Doubled rooks on the e-file: 1.Re8+ Rxe8 2.Rxe8# (f7/g7/h7 seal the back rank).
    // This mate is all checks and captures, so quiescence resolves it even at shallow
    // nominal depth; the search must report a forced-mate score.
    Board b = board_from_fen("r5k1/5ppp/8/8/8/8/4RPPP/4R1K1 w - - 0 1");
    CHECK(search(b, 4).score > 29000);   // forced mate found
}

TEST_CASE("quiescence declines a poisoned capture") {
    // Philidor: after 1.e4 e5 2.Nf3 d6, White to move. Material is equal. Nf3xe5 wins
    // the e5 pawn on the surface (static eval right after it reads about +160 for White),
    // but d6xe5 wins the knight straight back (the true value is about -200). A depth-1
    // search without quiescence would grab Nxe5 and think it is up material; with
    // quiescence the recapture is seen, so White declines it and stays near equal.
    Board b = board_from_fen("rnbqkbnr/ppp2ppp/3p4/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 0 3");
    SearchResult r = search(b, 1);
    // Comfortably below the ~+160 poisoned-pawn illusion (the bound is loose because
    // eval v2's mobility/structure terms give White a small honest edge here, ~94cp).
    CHECK(r.score < 120);                // the pawn "win" is seen to cost the knight
    CHECK(to_uci(r.best) != "f3e5");     // so Nxe5 is not chosen
}

TEST_CASE("timed search returns a legal move under a tiny budget") {
    Board b = start_position();
    SearchLimits lim;
    lim.max_depth = 64;
    lim.budget_ms = 5;
    SearchResult r = search_timed(b, lim);
    REQUIRE(r.best.from != NO_SQUARE);
    bool legal = false;
    for (const Move& m : generate_legal(b))
        if (m.from == r.best.from && m.to == r.best.to) legal = true;
    CHECK(legal);
}

TEST_CASE("timed search with a depth cap and no clock matches a fixed-depth search") {
    Board b = start_position();
    SearchLimits lim;
    lim.max_depth = 3;
    lim.budget_ms = 0;   // no time limit: pure depth
    SearchResult r = search_timed(b, lim);
    CHECK(r.score == search_to_depth(b, 3).score);
}

TEST_CASE("timed search finds mate in two with room to think") {
    Board b = board_from_fen("r5k1/5ppp/8/8/8/8/4RPPP/4R1K1 w - - 0 1");
    SearchLimits lim;
    lim.max_depth = 6;
    lim.budget_ms = 2000;
    SearchResult r = search_timed(b, lim);
    CHECK(r.score > 29000);
}
