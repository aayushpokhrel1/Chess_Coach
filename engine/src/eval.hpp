#pragma once
#include <string>
#include "board.hpp"

// v2 weight tuning: set an eval weight by option name (MobKnight, MobBishop, MobRook,
// MobQueen, BishopPair, Doubled, Isolated, Shield). Returns true if the name matched.
// Defaults are the hand-set v2 values, so behaviour is unchanged until something calls
// this; the UCI layer wires it to `setoption` so A/B self-play can retune at runtime.
bool eval_set_weight(const std::string& name, int value);

int piece_value(PieceType t);
int material_score(const Board& b);  // centipawns, positive favors White
int positional_eval(const Board& b); // v2 terms (mobility + bishop pair + pawn structure), White +
int evaluate(const Board& b);        // centipawns, positive favors the side to move
