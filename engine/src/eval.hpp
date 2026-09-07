#pragma once
#include "board.hpp"

int piece_value(PieceType t);
int material_score(const Board& b);  // centipawns, positive favors White
int positional_eval(const Board& b); // v2 terms (mobility + bishop pair + pawn structure), White +
int evaluate(const Board& b);        // centipawns, positive favors the side to move
