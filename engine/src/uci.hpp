#pragma once
#include <string>
#include "board.hpp"
#include "move.hpp"

// Parse a UCI move string ("e2e4", "e7e8q") into a legal Move for board b.
// Returns a Move with from == NO_SQUARE if the string matches no legal move.
Move move_from_uci(Board& b, const std::string& uci);
