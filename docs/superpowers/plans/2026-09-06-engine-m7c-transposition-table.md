# Engine M7c: Transposition Table (then bitboards) Implementation Plan

> Coached, task-by-task. Delegate mechanical transcription to the free worker; do the
> logic-heavy search integration by hand. Build/test via PowerShell (PATH refreshed).

**Goal:** Cache searched positions so the engine stops re-searching transpositions, making
each node cheaper and letting it reach deeper in the same time (fixing the measured depth-3 ==
depth-4 plateau). Then begin the bitboard board representation (Phase 2).

**Spec:** inline (design approved in chat 2026-09-06).

**Decisions (approved):** TT first, bitboards as Phase 2. Zobrist hash is RECOMPUTED from the
board for now (simple, correct); optimize to incremental XOR in make/unmake later, guarded by a
differential test that incremental == recomputed. Always-replace TT, full-key verified on probe,
~2^20 entries, main-search only (not quiescence).

## Global Constraints

- A correct TT returns the SAME scores as before, just faster. The existing differential test
  (`search` vs the full-width `negamax_full` oracle, test_search.cpp) is the proof and MUST stay
  green. Add a node-count check (TT reduces nodes at a fixed depth) to prove the TT actually fires.
- Store the full 64-bit key and verify it on probe, so collisions cannot cause wrong hits.
- Mate scores encode distance as +-(MATE - ply); adjust by `ply` on store/probe so a mate cached
  at one ply is not misread at another.
- Clear the TT at each public search entry (search / search_timed / search_minimax) for
  deterministic tests.
- No dashes in prose/comments/commits.
- Reuse `Board`, `Move`, `make_move/unmake_move`, `order_moves`, `nodes_searched()`.

---

## Phase 1: Transposition table (TT)

### Task 1: Zobrist hashing (compute_hash from the board)
**Files:** create `engine/src/zobrist.hpp`, `engine/src/zobrist.cpp`; add zobrist.cpp to
`engine/CMakeLists.txt` (the `engine` library sources).
**Interface:** `uint64_t compute_hash(const Board& b);`
- Fixed-seed `mt19937_64` fills: `Z_PIECE[12][64]` (index `color*6 + type`), `Z_SIDE`,
  `Z_CASTLE[4]` (per castling bit), `Z_EP[8]` (per en-passant file).
- `compute_hash` XORs: each occupied square's piece key; `Z_SIDE` if Black to move; `Z_CASTLE[i]`
  for each set castling bit; `Z_EP[file_of(en_passant)]` if `en_passant != NO_SQUARE`.
- Test: same position via two move orders (1.e4 e5 2.Nf3 vs 1.Nf3 e5 2.e4, both boards from FEN)
  hashes equal; a different position hashes different; startpos hash is stable.

### Task 2: The transposition table (probe/store/clear)
**Files:** create `engine/src/tt.hpp`, `engine/src/tt.cpp`; add tt.cpp to CMakeLists.
**Interface:**
```cpp
enum class TTFlag : uint8_t { None, Exact, Lower, Upper };
struct TTEntry { uint64_t key; int32_t score; Move move; int16_t depth; TTFlag flag; };
void tt_clear();
bool tt_probe(uint64_t key, int depth, int ply, int alpha, int beta, int& score, Move& move);
void tt_store(uint64_t key, int depth, int ply, int score, TTFlag flag, const Move& move);
```
- Fixed array of `1 << 20` entries; index `key & (SIZE - 1)`; always-replace.
- `tt_probe`: if slot.key == key: fill `move` (for ordering) always; if `slot.depth >= depth`,
  convert the stored score back from mate-relative-to-ply, then return true when Exact, or
  Lower with score >= beta, or Upper with score <= alpha (a usable cutoff). Else return false.
- `tt_store`: adjust mate scores to be ply-independent, then write the slot.
- Test: store then probe round-trips a value+move; a shallower stored depth does not cut a deeper
  probe; key mismatch misses.

### Task 3: Wire the TT into negamax (BY HAND, coached)
**Files:** modify `engine/src/search.cpp`.
- At `negamax` entry: `uint64_t key = compute_hash(b);` then `tt_probe`; on a usable hit return the
  score. Keep a `Move tt_move` from the probe.
- Capture `alpha_orig = alpha` before the loop. Order the TT move first (rotate it to front after
  `order_moves`, like `search_to_depth` does with its hint move).
- Track the best move. After the loop, choose the flag: `best <= alpha_orig` -> Upper;
  `best >= beta` -> Lower; else Exact. `tt_store(key, depth, ply, best, flag, best_move)`.
- Clear the TT at each public entry point. Do NOT probe/store in `quiesce` or `negamax_full`
  (the oracle must stay TT-free so it remains an independent check).
- Test: the differential test (`search` == full-width) still passes at depths 3-4 on the standard
  positions; a new test asserts `nodes_searched()` with the TT < a recorded no-TT baseline at a
  fixed depth (proves the TT fires). 68+ existing cases stay green.

### Task 4: Re-calibrate the play levels
- Re-run `web/scripts/gauntlet.mjs` (the engine is now stronger/deeper per unit time), update the
  Elo labels in `web/src/play.ts`. Possibly the depth-3/4 levels now separate.

---

## Phase 2: Bitboards (begins after Phase 1 is proven + re-calibrated)

Large rewrite of the board representation (a `uint64_t` per piece type per color) and move
generation. Scoped as its own plan when we reach it. Zobrist and the TT are unaffected in
interface (they key off the position, however it is represented). Not started until the TT is
committed and the levels re-calibrated.

## Self-Review
- Proof is the existing differential test (values unchanged) + a node-count drop (TT fires).
- Mate-score ply adjustment and full-key verification are the two correctness gotchas; both called
  out per task. Recompute-first keeps make/unmake untouched this phase.
- Phase 2 (bitboards) is deferred and self-contained; TT/zobrist interfaces survive it.
