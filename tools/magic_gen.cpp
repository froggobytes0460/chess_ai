#include <array>
#include <chess_ai/bitops.hpp>
#include <chess_ai/types.hpp>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <ios>
#include <iostream>
#include <random>
#include <utility>
#include <vector>

namespace {
constexpr uint64_t RANDOM_SEED{0X2B6A0D2EB7393D0F};

auto rookMask(Square sq) -> BitBoard {
  BitBoard mask{};
  int file = static_cast<int>(sq) % 8;
  int rank = static_cast<int>(sq) / 8;

  // North / South along the file, stopping before rank 7 / rank 0.
  for (int r = rank + 1; r <= 6; ++r) {
    setBit(mask, static_cast<Square>((r * 8) + file));
  }
  for (int r = rank - 1; r >= 1; --r) {
    setBit(mask, static_cast<Square>((r * 8) + file));
  }

  // East / West along the rank, stopping before file h / file a.
  for (int f = file + 1; f <= 6; ++f) {
    setBit(mask, static_cast<Square>((rank * 8) + f));
  }
  for (int f = file - 1; f >= 1; --f) {
    setBit(mask, static_cast<Square>((rank * 8) + f));
  }

  return mask;
}

auto rookAttackSlow(Square sq, BitBoard occupied) -> BitBoard {
  BitBoard attacks{};
  int file = static_cast<int>(sq) % 8;
  int rank = static_cast<int>(sq) / 8;

  constexpr std::array<std::pair<int, int>, 4> DIRS = {
      {{0, 1}, {0, -1}, {1, 0}, {-1, 0}},
  };

  for (const auto& [delta_file, delta_rank] : DIRS) {
    int target_file = file + delta_file;
    int target_rank = rank + delta_rank;
    while (target_file >= 0 && target_file <= 7 && target_rank >= 0 &&
           target_rank <= 7) {
      auto target = static_cast<Square>((target_rank * 8) + target_file);
      setBit(attacks, target);
      if (getBit(occupied, target)) {
        break;
      }
      target_file += delta_file;
      target_rank += delta_rank;
    }
  }

  return attacks;
}

auto bishopMask(Square sq) -> BitBoard {
  BitBoard mask{};
  int file = static_cast<int>(sq) % 8;
  int rank = static_cast<int>(sq) / 8;

  constexpr std::array<std::pair<int, int>, 4> DIRS = {
      {{1, 1}, {1, -1}, {-1, 1}, {-1, -1}},
  };

  for (const auto& [delta_file, delta_rank] : DIRS) {
    int target_file = file + delta_file;
    int target_rank = rank + delta_rank;
    // Stop one square before the edge: the square after this step must
    // still be on the board.
    while (target_file + delta_file >= 0 && target_file + delta_file <= 7 &&
           target_rank + delta_rank >= 0 && target_rank + delta_rank <= 7) {
      setBit(mask, static_cast<Square>((target_rank * 8) + target_file));
      target_file += delta_file;
      target_rank += delta_rank;
    }
  }

  return mask;
}

auto bishopAttackSlow(Square sq, BitBoard occupied) -> BitBoard {
  BitBoard attacks{};
  int file = static_cast<int>(sq) % 8;
  int rank = static_cast<int>(sq) / 8;

  constexpr std::array<std::pair<int, int>, 4> DIRS = {
      {{1, 1}, {1, -1}, {-1, 1}, {-1, -1}},
  };

  for (const auto& [delta_file, delta_rank] : DIRS) {
    int target_file = file + delta_file;
    int target_rank = rank + delta_rank;
    while (target_file >= 0 && target_file <= 7 && target_rank >= 0 &&
           target_rank <= 7) {
      auto target = static_cast<Square>((target_rank * 8) + target_file);
      setBit(attacks, target);
      if (getBit(occupied, target)) {
        break;
      }
      target_file += delta_file;
      target_rank += delta_rank;
    }
  }

  return attacks;
}

auto findMagic(int n, const std::vector<BitBoard>& occupancies,
               const std::vector<BitBoard>& attacks) -> uint64_t {
  // NOLINTNEXTLINE(bugprone-random-generator-seed) Need deterministic output
  std::mt19937_64 rng(RANDOM_SEED);
  std::vector<BitBoard> used(uint64_t{1} << n);

  std::vector<uint32_t> filled(uint64_t{1} << n, 0);
  uint32_t curr_epoch = 0;

  for (;;) {
    // AND a couple random `uint64_t` to generate sparse bits
    uint64_t magic = rng() & rng() & rng();
    curr_epoch++;

    if (curr_epoch == 0) {
      filled.assign(uint64_t{1} << n, 0);
      curr_epoch = 1;
    }

    bool collision = false;
    for (size_t i = 0; i < occupancies.size(); ++i) {
      size_t idx = (occupancies[i] * magic) >> (64 - n);

      if (filled[idx] != curr_epoch) {
        filled[idx] = curr_epoch;
        used[idx] = attacks[i];
      } else if (used[idx] != attacks[i]) {
        collision = true;
        break;
      }
    }
    if (!collision) {
      return magic;
    }
  }
}

template <auto maskFn, auto attackSlowFn>
auto findMagicForSquare(Square sq) -> uint64_t {
  BitBoard mask = maskFn(sq);
  std::vector<BitBoard> subsets;
  BitBoard subset = 0;
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-do-while)
  do {
    subsets.push_back(subset);
    subset = (subset - mask) & mask;
  } while (subset != 0);

  std::vector<BitBoard> attacks;
  attacks.reserve(subsets.size());
  for (const BitBoard subset : subsets) {
    attacks.push_back(attackSlowFn(sq, subset));
  }

  return findMagic(popCount(mask), subsets, attacks);
}
}  // namespace

// NOLINTNEXTLINE(bugprone-exception-escape) offline tool, crash-on-oom is fine
auto main() -> int {
  std::cout << "constexpr std::array<uint64_t, 64> ROOK_MAGICS = {\n";
  for (int sq = 0; sq < 64; ++sq) {
    uint64_t magic =
        findMagicForSquare<rookMask, rookAttackSlow>(static_cast<Square>(sq));
    std::cout << "    0x" << std::setfill('0') << std::setw(16) << std::hex
              << std::uppercase << magic << "ULL,\n";
  }
  std::cout << "};\n" << std::dec << std::nouppercase;

  std::cout << "constexpr std::array<uint64_t, 64> BISHOP_MAGICS = {\n";
  for (int sq = 0; sq < 64; ++sq) {
    uint64_t magic = findMagicForSquare<bishopMask, bishopAttackSlow>(
        static_cast<Square>(sq));
    std::cout << "    0x" << std::setfill('0') << std::setw(16) << std::hex
              << std::uppercase << magic << "ULL,\n";
  }
  std::cout << "};\n" << std::dec << std::nouppercase;
  return 0;
}
