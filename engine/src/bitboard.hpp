#pragma once
#include <cstdint>
#include "board.hpp"
#include "types.hpp"
#include "zobrist.hpp"

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

// Place / remove / move a piece, keeping squares[] and the bitboards in lockstep.
// add_piece requires a real piece and remove_piece an occupied square (they index
// bb[color][type], so an empty piece would be out of range); make/unmake only ever
// call them that way.
inline void add_piece(Board& b, Square s, Piece p) {
    b.squares[s] = p;
    int c = static_cast<int>(p.color), t = static_cast<int>(p.type);
    bb_set(b.bb[c][t], s);
    bb_set(b.occ[c], s);
    bb_set(b.occ_all, s);
    b.hash ^= zobrist_piece(p.color, p.type, s);   // toggle this piece-square term
}
inline void remove_piece(Board& b, Square s) {
    Piece p = b.squares[s];
    int c = static_cast<int>(p.color), t = static_cast<int>(p.type);
    bb_clear(b.bb[c][t], s);
    bb_clear(b.occ[c], s);
    bb_clear(b.occ_all, s);
    b.hash ^= zobrist_piece(p.color, p.type, s);   // toggle it back off
    b.squares[s] = Piece{Color::None, PieceType::None};
}
inline void move_piece(Board& b, Square from, Square to) {
    Piece p = b.squares[from];
    remove_piece(b, from);
    add_piece(b, to, p);
}

// Attack sets. The leaper tables (knight/king/pawn) depend only on the square;
// the sliders take the full occupancy so rays stop at the first blocker (which is
// included, so a capture is generated; the caller masks off its own pieces).
uint64_t knight_attacks(Square s);
uint64_t king_attacks(Square s);
uint64_t pawn_attacks(Color c, Square s);
uint64_t bishop_attacks(Square s, uint64_t occ);
uint64_t rook_attacks(Square s, uint64_t occ);
uint64_t queen_attacks(Square s, uint64_t occ);

// Fill bb/occ/occ_all from squares[] (the FEN parser calls this once).
void bb_rebuild(Board& b);

// Debug/test invariant: do the bitboards agree with squares[]? Rebuilds into a
// copy and compares, so a make/unmake sync bug is caught before perft would notice.
bool bb_matches_squares(const Board& b);
