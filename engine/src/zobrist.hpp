#pragma once
#include <cstdint>
#include "board.hpp"

// Zobrist hash of a position: a 64-bit key built by XORing one random number per
// piece-on-square, plus side-to-move, castling rights, and the en-passant file.
// The same position (however it was reached) always produces the same key, which
// is what lets the transposition table recognize a position it has already searched.
// Recomputed from the board for now; an incremental XOR version comes later.
uint64_t compute_hash(const Board& b);
