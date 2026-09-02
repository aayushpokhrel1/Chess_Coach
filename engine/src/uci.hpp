#pragma once
#include <string>
#include "board.hpp"
#include "move.hpp"

// Parse a UCI move string ("e2e4", "e7e8q") into a legal Move for board b.
// Returns a Move with from == NO_SQUARE if the string matches no legal move.
Move move_from_uci(Board& b, const std::string& uci);

// Engine state carried across UCI commands (the current game position).
struct UciState {
    Board board = start_position();
};

// Handle one UCI input line; returns the text to print (possibly empty or
// multiple lines, with no trailing newline). Updates state for position /
// ucinewgame. Unknown commands return "".
std::string handle_command(UciState& state, const std::string& line);
