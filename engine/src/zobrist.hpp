#pragma once
#include <cstdint>
#include "board.hpp"

// Zobrist hash of a position: a 64-bit key built by XORing one random number per
// piece-on-square, plus side-to-move, castling rights, and the en-passant file.
// The same position (however it was reached) always produces the same key, which
// is what lets the transposition table recognize a position it has already searched.
// compute_hash rebuilds the key from scratch (O(64)); make/unmake keep an incremental
// copy on the Board (b.hash) in step with it, and this stays the independent oracle a
// differential test checks that copy against.
uint64_t compute_hash(const Board& b);

// Individual Zobrist terms, so make/unmake (and null-move) can fold a single change
// into b.hash by XOR instead of rescanning the board. XOR is self-inverse, so the
// same call toggles a term on or off.
uint64_t zobrist_piece(Color c, PieceType t, Square s);  // c in {White,Black}, t a real piece
uint64_t zobrist_side();                                 // toggled when the side to move flips
uint64_t zobrist_castle_bit(int bit_index);              // 0..3 == WK, WQ, BK, BQ
uint64_t zobrist_ep_file(int file);                      // 0..7
