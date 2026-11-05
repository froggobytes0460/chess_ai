#ifndef ZOBRIST_KEY_HPP
#define ZOBRIST_KEY_HPP

#include <array>
#include <cstdint>

void initZobrist();
auto getZobristKeysForPieces()
    -> const std::array<std::array<std::array<uint64_t, 64>, 6>, 2>&;
auto getZobristKeysForCastlingRights() -> const std::array<uint64_t, 16>&;
auto getZobristKeysForEnPassant() -> const std::array<uint64_t, 8>&;
auto getZobristKeyForSideToMove() -> const uint64_t&;

#endif  // !ZOBRIST_KEY_HPP
