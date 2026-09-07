# Engine M7c Phase 2: Bitboards (hybrid + classical rays) Implementation Plan

> Coached, task-by-task. Build/test via PowerShell (PATH refreshed). No dashes in
> prose/comments/commits.

**Goal:** Speed up move generation and attack detection (the search hot path) by adding
piece bitboards to the board, so set-wise operations replace per-square array walking.
More nodes per second means more depth in the same time, and it unblocks the "coach uses
our engine for analysis" endgame. It is also the systems-programming lesson: attack tables,
occupancy, bitscans, XOR-free hybrid maintenance.

**Spec:** inline (design approved in chat 2026-09-07).

**Decisions (approved):** HYBRID board (keep `squares[64]` as the "what piece is here"
source of truth; ADD bitboards for set operations), so FEN, eval, and make/unmake's capture
logic keep working and only movegen + attack detection are rewritten. Sliding attacks by
CLASSICAL RAYS + bitscan (`__builtin_ctzll`/`clzll`), NOT magic bitboards (a later "make it
faster" pass). Zobrist/TT are unaffected (they key off the position via `squares[]`). Eval
stays on `squares[]` this phase; a bitboard mobility term is Eval v2's job.

## Global Constraints (the proof)

- **Bitboards are a pure representation change: the generated move set MUST be identical.**
  Perft counts (start, Kiwipete, Position 3) are the definitive proof and MUST stay exactly
  equal to the published numbers. All search tests stay green.
- **squares[] and the bitboards must never disagree.** A debug helper `bb_matches_squares(b)`
  is the invariant; make/unmake and FEN keep both in sync. Tested directly (T1, T2) so a
  desync is caught even before perft would notice.
- Reuse `Board`, `Move`, `make_move/unmake_move`, `generate_legal` (the make/unmake + in_check
  legality filter is unchanged; only pseudo-legal generation and `is_square_attacked` change).
- Bit index = square 0..63 (a1=0, h8=63), matching `make_square(file, rank)`.

---

### Task 1: Bitboard state + helpers + FEN sync
**Files:** `engine/src/board.hpp` (add state), new `engine/src/bitboard.hpp`/`.cpp` (helpers),
`engine/src/board.cpp` (rebuild in FEN), `engine/CMakeLists.txt` (add bitboard.cpp).
- Add to `Board`: `uint64_t bb[2][6]` (color, PieceType) and `uint64_t occ[2]`, `uint64_t occ_all`.
- `bitboard.hpp`: inline `bb_set/bb_clear/bb_get(uint64_t&, Square)`, `lsb(uint64_t)` via
  `__builtin_ctzll`, `pop_lsb(uint64_t&)`, `popcount`. `bb_rebuild(Board&)` fills bb/occ from
  `squares[]`. `bb_matches_squares(const Board&)` returns whether they agree (test/debug guard).
- `board_from_fen` calls `bb_rebuild` before returning.
- **Test:** after parsing several FENs (start, Kiwipete, an ep/castling position),
  `bb_matches_squares(b)` is true; `popcount(occ_all)` equals the piece count.

### Task 2: Keep bitboards in sync through make/unmake
**Files:** `engine/src/move.cpp`.
- Route every square mutation in `make_move`/`unmake_move` through helpers that touch BOTH
  representations: `remove_piece(b, sq)`, `add_piece(b, sq, piece)`, `move_piece(b, from, to)`.
  Cover the four special cases the same way they already are for `squares[]`: capture (remove
  victim), castling (move the rook), en passant (remove the pawn behind the target), promotion
  (remove the pawn, add the promoted piece).
- **Test:** across a list of varied moves (quiet, capture, ep, castle, promotion), after
  `make_move` `bb_matches_squares(b)` is true, and after `unmake_move` the board (both reps)
  round-trips. Perft still matches (make/unmake now churns bitboards on every node).

### Task 3: Attack tables + classical-ray sliding attacks
**Files:** `engine/src/bitboard.hpp`/`.cpp`.
- Precompute at startup (static init): `KNIGHT_ATTACKS[64]`, `KING_ATTACKS[64]`,
  `PAWN_ATTACKS[2][64]`, and `RAYS[8][64]` (one mask per compass direction from each square).
- `bishop_attacks(sq, occ)`, `rook_attacks(sq, occ)`, `queen_attacks(sq, occ)`: for each of the
  piece's ray directions, take `RAYS[dir][sq] & occ`; if any blocker, find the nearest one with
  `ctzll` (positive rays) or `clzll` (negative rays) and trim the ray at (and including) it;
  OR the trimmed rays together.
- **Test:** rook on a1 empty board attacks the a-file + rank 1 (14 squares); add a blocker on a4
  and the a-file attack stops at a4 (inclusive); a bishop on d4 with a blocker on f6 stops there.

### Task 4: Rewrite `is_square_attacked` with bitboards
**Files:** `engine/src/movegen.cpp`.
- `is_square_attacked(b, sq, by)` = OR of: `PAWN_ATTACKS[!by][sq] & their pawns` (attackers of sq
  are where an enemy pawn would capture INTO sq), `KNIGHT_ATTACKS[sq] & their knights`,
  `KING_ATTACKS[sq] & their king`, `bishop_attacks(sq, occ_all) & (their bishops|queens)`,
  `rook_attacks(sq, occ_all) & (their rooks|queens)`; return whether any bit is set.
- Keep the old array version as `is_square_attacked_ref` temporarily for a differential test.
- **Test:** the new and reference versions agree for every square and both colors on several
  positions (start, Kiwipete). Existing attack/in_check tests stay green. Remove the ref after.

### Task 5: Rewrite `generate_pseudo_legal` with bitboards
**Files:** `engine/src/movegen.cpp`.
- For each piece type of the side to move, iterate its bitboard (`pop_lsb`), compute the target
  bitboard (`attacks & ~occ[side]` for pieces; specialized for pawns), and emit a Move per target
  bit. Pawns: single/double pushes via shifts against `~occ_all`, captures via
  `PAWN_ATTACKS[side][sq] & (occ[enemy] | ep-bit)`, promotions when reaching the last rank.
- Castling: reuse the existing generation (it already checks empty squares + `is_square_attacked`,
  which is now the fast one).
- `generate_legal` is unchanged (still make/unmake + `in_check`).
- **Test (definitive):** perft for start (to depth 5/6), Kiwipete, and Position 3 all match the
  published counts exactly. All existing move-count tests green.

### Task 6: Measure + clean up
- Remove `is_square_attacked_ref` and any now-dead array ray helpers.
- Measure: perft nodes/sec and search depth-6 startpos time vs the pre-bitboard number
  (~26K nodes / 0.64s after null-move) for a concrete speedup figure.
- Confirm full `ctest` green and the engine still plays over UCI.

## Self-Review
- Proof ladder: bb/squares consistency (T1, T2) -> attack correctness vs the reference (T4)
  -> perft exact match (T5). Each task fails loudly if the representation drifts.
- Hybrid keeps FEN/eval/make's capture logic on `squares[]`, so the blast radius is movegen +
  attacks + the make/unmake mirror only.
- Classical rays are the simple correct method; magic bitboards are a later optimization and do
  not change any interface. Zobrist/TT untouched.
