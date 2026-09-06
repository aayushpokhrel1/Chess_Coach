#pragma once

#include <cstdint>
#include "move.hpp"

enum class TTFlag : uint8_t { None, Exact, Lower, Upper };

void tt_clear();

// tt_probe returns true ONLY when a stored entry gives a usable cutoff; it also fills move for ordering whenever the stored key matches, regardless of depth
bool tt_probe(uint64_t key, int depth, int ply, int alpha, int beta, int& score, Move& move);

// tt_store always replaces the slot
void tt_store(uint64_t key, int depth, int ply, int score, TTFlag flag, const Move& move);