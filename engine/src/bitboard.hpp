#pragma once
#include <cstdint>
#include "board.hpp"
#include "types.hpp"

// Bitboard primitives. A bitboard is a uint64_t with one bit per square, bit
// index == square (a1=0, h8=63), so squares are set operations: union is |,
// intersection is &, "everything except" is ~.

inline void bb_set(uint64_t& b, Square s)   { b |= (1ULL << s); }
inline void bb_clear(uint64_t& b, Square s) { b &= ~(1ULL << s); }
inline bool bb_get(uint64_t b, Square s)    { return (b >> s) & 1ULL; }

// lsb: index of the least-significant set bit (undefined for 0, callers guard).
inline int lsb(uint64_t b) { return __builtin_ctzll(b); }
// msb: index of the most-significant set bit (undefined for 0).
inline int msb(uint64_t b) { return 63 - __builtin_clzll(b); }
// pop_lsb: return the lsb index and clear it (the standard "for each bit" step).
inline int pop_lsb(uint64_t& b) { int s = __builtin_ctzll(b); b &= b - 1; return s; }
inline int popcount(uint64_t b) { return __builtin_popcountll(b); }

// Fill bb/occ/occ_all from squares[] (the FEN parser calls this once).
void bb_rebuild(Board& b);

// Debug/test invariant: do the bitboards agree with squares[]? Rebuilds into a
// copy and compares, so a make/unmake sync bug is caught before perft would notice.
bool bb_matches_squares(const Board& b);
