#pragma once
#include <cstdint>
#include <map>
#include <string>
#include "board.hpp"

uint64_t perft(const Board& b, int depth);
std::map<std::string, uint64_t> perft_divide(const Board& b, int depth);