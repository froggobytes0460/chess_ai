#include <algorithm>
#include <array>
#include <chess_ai/zobrist_key.hpp>
#include <cstddef>
#include <cstdint>
#include <random>

namespace {
constexpr uint64_t RANDOM_SEED = 0X00EF306E22F98FD4;

// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)
std::array<std::array<std::array<uint64_t, 64>, 6>, 2> keys_piece{};
std::array<uint64_t, 16> keys_castling_rights{};
std::array<uint64_t, 8> keys_en_passant{};
uint64_t key_side_to_move{};
// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

template <typename T>
void fillArrayWithPrngRandom(std::mt19937_64& prng, T* first_elem_ptr,
                             size_t num_elements) {
  std::generate_n(first_elem_ptr, num_elements,
                  [&prng]() -> uint64_t { return prng(); });
}

template <typename T>
auto arrayIsZero(T* first_elem_ptr, size_t num_elements) -> bool {
  return std::all_of(first_elem_ptr, first_elem_ptr + num_elements,
                     [](const T val) -> bool { return val == 0; });
}
}  // namespace

void initZobrist() {
  // NOLINTNEXTLINE(bugprone-random-generator-seed)
  std::mt19937_64 prng(RANDOM_SEED);

  // key_side_to_move
  key_side_to_move = prng();

  fillArrayWithPrngRandom(
      prng, keys_piece[0][0].data(),
      keys_piece.size() * keys_piece[0].size() * keys_piece[0][0].size());
  fillArrayWithPrngRandom(prng, keys_castling_rights.data(),
                          keys_castling_rights.size());
  fillArrayWithPrngRandom(prng, keys_en_passant.data(), keys_en_passant.size());
}

auto getZobristKeysForPieces()
    -> const std::array<std::array<std::array<uint64_t, 64>, 6>, 2>& {
  if (arrayIsZero(
          keys_piece[0][0].data(),
          keys_piece.size() * keys_piece[0].size() * keys_piece[0][0].size())) {
    initZobrist();
  }
  return keys_piece;
}

auto getZobristKeysForCastlingRights() -> const std::array<uint64_t, 16>& {
  if (arrayIsZero(keys_castling_rights.data(), keys_castling_rights.size())) {
    initZobrist();
  }
  return keys_castling_rights;
}

auto getZobristKeysForEnPassant() -> const std::array<uint64_t, 8>& {
  if (arrayIsZero(keys_en_passant.data(), keys_en_passant.size())) {
    initZobrist();
  }
  return keys_en_passant;
}

auto getZobristKeyForSideToMove() -> const uint64_t& {
  if (key_side_to_move == 0ULL) {
    initZobrist();
  }
  return key_side_to_move;
}
