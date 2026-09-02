#pragma once
#include <cstdint>
#include <map>
#include <string>
#include "board.hpp"

uint64_t perft(Board& b, int depth);
std::map<std::string, uint64_t> perft_divide(Board& b, int depth);