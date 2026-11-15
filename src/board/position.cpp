#include <array>
#include <cassert>
#include <chess_ai/board/position.hpp>
#include <chess_ai/core/bitops.hpp>
#include <chess_ai/core/types.hpp>
#include <cstdint>

#include "zobrist_key.hpp"

namespace {
auto enPassantKey(Square sq) -> uint64_t {
  return getZobristKeysForEnPassant()[static_cast<unsigned>(sq) % 8];
}
}  // namespace

Position::Position()
    : pieces_{},  // NOLINT(readability-trailing-comma)
      board_{},
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
      BitBoard bb{pieces_[color][piece]};
      while (bb != 0) {
        auto sq = static_cast<unsigned>(popLSB(bb));
        key ^= piece_keys[color][piece][sq];
      }
    }
  }

  key ^= getZobristKeysForCastlingRights()[castling_rights_bits_];

  if (en_passant_ != Square::NONE) {
    key ^= enPassantKey(en_passant_);
  }

  if (side_to_move_ == Color::BLACK) {
    key ^= getZobristKeyForSideToMove();
  }

  return key;
}

void Position::putPiece(Piece piece, Square sq) {
  assert(piece != Piece::NONE);
  assert(pieceAt(sq) == Piece::NONE);

  const auto col = static_cast<unsigned>(colorOf(piece));
  const auto pt = static_cast<unsigned>(typeOf(piece));

  setBit(pieces_[col][pt], sq);
  setBit(occupied_[col], sq);
  setBit(occupied_all_, sq);

  board_[static_cast<unsigned>(sq)] = piece;

  zobrist_key_ ^= getZobristKeysForPieces()[col][pt][static_cast<unsigned>(sq)];
}

void Position::removePiece(Square sq) {
  assert(pieceAt(sq) != Piece::NONE);

  const Piece piece = board_[static_cast<unsigned>(sq)];

  const auto col = static_cast<unsigned>(colorOf(piece));
  const auto pt = static_cast<unsigned>(typeOf(piece));

  unsetBit(pieces_[col][pt], sq);
  unsetBit(occupied_[col], sq);
  unsetBit(occupied_all_, sq);

  board_[static_cast<unsigned>(sq)] = Piece::NONE;

  zobrist_key_ ^= getZobristKeysForPieces()[col][pt][static_cast<unsigned>(sq)];
}

void Position::movePiece(Square from, Square to) {
  assert(pieceAt(to) == Piece::NONE);
  assert(pieceAt(from) != Piece::NONE);

  const Piece piece = board_[static_cast<unsigned>(from)];
  const auto col = static_cast<unsigned>(colorOf(piece));
  const auto pt = static_cast<unsigned>(typeOf(piece));

  const BitBoard mask{squareBitboard(from) | squareBitboard(to)};

  pieces_[col][pt] ^= mask;
  occupied_[col] ^= mask;
  occupied_all_ ^= mask;

  board_[static_cast<unsigned>(from)] = Piece::NONE;
  board_[static_cast<unsigned>(to)] = piece;

  const std::array<uint64_t, 64>& keys = getZobristKeysForPieces()[col][pt];
  zobrist_key_ ^=
      keys[static_cast<unsigned>(from)] ^ keys[static_cast<unsigned>(to)];
}

void Position::setEnPassant(Square sq) {
  if (en_passant_ != Square::NONE) {
    zobrist_key_ ^= enPassantKey(en_passant_);
  }
  en_passant_ = sq;
  if (en_passant_ != Square::NONE) {
    zobrist_key_ ^= enPassantKey(en_passant_);
  }
}

void Position::setCastlingRights(uint8_t rights) {
  const std::array<uint64_t, 16>& keys = getZobristKeysForCastlingRights();
  zobrist_key_ ^= keys[castling_rights_bits_] ^ keys[rights];
  castling_rights_bits_ = rights;
}
