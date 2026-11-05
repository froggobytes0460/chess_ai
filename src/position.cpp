#include <chess_ai/bitops.hpp>
#include <chess_ai/position.hpp>
#include <chess_ai/types.hpp>
#include <chess_ai/zobrist_key.hpp>
#include <cstdint>

Position::Position()
    : pieces_{},  // NOLINT(readability-trailing-comma)
      occupied_{},
      occupied_all_{},
      full_move_counter_{},
      half_move_clock_{},
      castling_rights_bits_{},
      side_to_move_{Color::WHITE},
      en_passant_{Square::NONE} {
  // NOLINTNEXTLINE(cppcoreguidelines-prefer-member-initializer)
  zobrist_key_ = computeZobristKey();
}

auto Position::computeZobristKey() const -> uint64_t {
  uint64_t key{};

  const std::array<std::array<std::array<uint64_t, 64>, 6>, 2>& piece_keys =
      getZobristKeysForPieces();
  for (unsigned color = 0; color < 2; ++color) {
    for (unsigned piece = 0; piece < 6; ++piece) {
      BitBoard bb = pieces_[color][piece];
      while (bb != 0) {
        auto sq = static_cast<unsigned>(popLSB(bb));
        key ^= piece_keys[color][piece][sq];
      }
    }
  }

  key ^= getZobristKeysForCastlingRights()[castling_rights_bits_];

  if (en_passant_ != Square::NONE) {
    auto file = static_cast<unsigned>(en_passant_) % 8;
    key ^= getZobristKeysForEnPassant()[file];
  }

  if (side_to_move_ == Color::BLACK) {
    key ^= getZobristKeyForSideToMove();
  }

  return key;
}
