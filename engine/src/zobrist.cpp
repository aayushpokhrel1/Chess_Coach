#include "zobrist.hpp"
#include <random>

namespace {
// One random 64-bit number for every ingredient of a position. Filled once at
// startup from a FIXED seed, so every build and every run agrees on the keys.
uint64_t Z_PIECE[12][64];  // [color*6 + type][square]
uint64_t Z_SIDE;           // XORed in when Black is to move
uint64_t Z_CASTLE[4];      // one per castling-rights bit (WK, WQ, BK, BQ)
uint64_t Z_EP[8];          // one per en-passant file (only the file matters)

bool init_tables() {
    std::mt19937_64 rng(0x9E3779B97F4A7C15ULL);  // fixed seed: reproducible keys
    for (auto& row : Z_PIECE)
        for (auto& x : row) x = rng();
    Z_SIDE = rng();
    for (auto& x : Z_CASTLE) x = rng();
    for (auto& x : Z_EP) x = rng();
    return true;
}
const bool z_ready = init_tables();  // runs before main()
}

uint64_t zobrist_piece(Color c, PieceType t, Square s) {
    return Z_PIECE[static_cast<int>(c) * 6 + static_cast<int>(t)][s];
}
uint64_t zobrist_side()                { return Z_SIDE; }
uint64_t zobrist_castle_bit(int i)     { return Z_CASTLE[i]; }
uint64_t zobrist_ep_file(int file)     { return Z_EP[file]; }

uint64_t compute_hash(const Board& b) {
    uint64_t h = 0;
    for (Square sq = 0; sq < 64; sq++) {
        const Piece& p = b.squares[sq];
        if (p.type == PieceType::None) continue;
        int idx = static_cast<int>(p.color) * 6 + static_cast<int>(p.type);  // 0..11
        h ^= Z_PIECE[idx][sq];
    }
    if (b.side_to_move == Color::Black) h ^= Z_SIDE;
    for (int i = 0; i < 4; i++)
        if (b.castling_rights & (1 << i)) h ^= Z_CASTLE[i];
    if (b.en_passant != NO_SQUARE) h ^= Z_EP[file_of(b.en_passant)];
    return h;
}
