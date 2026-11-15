#ifndef MOVE_HPP
#define MOVE_HPP

#include <array>
#include <chess_ai/types.hpp>
#include <cstdint>

// Move flags encoding special move semantics
// (promotion/castle/en-passant/double-push), laid out so bit 3 marks a capture
// and bit 2 marks a promotion, mirroring the common 4-bit move-flag convention
// (e.g. chess programming wiki's encoding).
enum class MoveFlag : uint8_t {
  QUIET = 0b0000,
  DOUBLE_PUSH = 0b0001,
  CASTLE_KINGSIDE = 0b0010,
  CASTLE_QUEENSIDE = 0b0011,
  CAPTURE = 0b0100,
  EN_PASSANT = 0b0101,
  PROMOTION_KNIGHT = 0b1000,
  PROMOTION_BISHOP = 0b1001,
  PROMOTION_ROOK = 0b1010,
  PROMOTION_QUEEN = 0b1011,
  PROMOTION_CAPTURE_KNIGHT = 0b1100,
  PROMOTION_CAPTURE_BISHOP = 0b1101,
  PROMOTION_CAPTURE_ROOK = 0b1110,
  PROMOTION_CAPTURE_QUEEN = 0b1111,
};

// A chess move packed into 16 bits: from (6 bits) | to (6 bits) | flags (4
// bits).
class Move {
 private:
  uint16_t bits_;

  static constexpr uint16_t SQUARE_MASK = 0x3F;
  static constexpr uint16_t TO_SHIFT = 6;
  static constexpr uint16_t FLAG_SHIFT = 12;

  // Low two flag bits of a promotion select the piece, in this order.
  static constexpr uint8_t PROMOTION_TYPE_MASK = 0b11;
  static constexpr std::array<PieceType, 4> PROMOTION_TYPES{
      PieceType::KNIGHT,
      PieceType::BISHOP,
      PieceType::ROOK,
      PieceType::QUEEN,
  };

 public:
  constexpr Move() : bits_(0) {}

  constexpr Move(Square from, Square to, MoveFlag flag)
      : bits_(
            static_cast<uint16_t>(static_cast<uint16_t>(from) & SQUARE_MASK) |
            static_cast<uint16_t>((static_cast<uint16_t>(to) & SQUARE_MASK)
                                  << TO_SHIFT) |
            static_cast<uint16_t>(static_cast<uint16_t>(flag) << FLAG_SHIFT)) {}

  [[nodiscard]] constexpr auto from() const -> Square {
    return static_cast<Square>(bits_ & SQUARE_MASK);
  }

  [[nodiscard]] constexpr auto to() const -> Square {
    return static_cast<Square>((bits_ >> TO_SHIFT) & SQUARE_MASK);
  }

  [[nodiscard]] constexpr auto flag() const -> MoveFlag {
    return static_cast<MoveFlag>(bits_ >> FLAG_SHIFT);
  }

  [[nodiscard]] constexpr auto isCapture() const -> bool {
    return (static_cast<uint8_t>(flag()) &
            static_cast<uint8_t>(MoveFlag::CAPTURE)) != 0;
  }

  [[nodiscard]] constexpr auto isPromotion() const -> bool {
    return (static_cast<uint8_t>(flag()) &
            static_cast<uint8_t>(MoveFlag::PROMOTION_KNIGHT)) != 0;
  }

  [[nodiscard]] constexpr auto isCastle() const -> bool {
    return flag() == MoveFlag::CASTLE_KINGSIDE ||
           flag() == MoveFlag::CASTLE_QUEENSIDE;
  }

  // Returns the piece type a pawn promotes to. Only valid if `isPromotion()`.
  [[nodiscard]] constexpr auto promotionType() const -> PieceType {
    return PROMOTION_TYPES[static_cast<uint8_t>(flag()) & PROMOTION_TYPE_MASK];
  }

  [[nodiscard]] constexpr auto raw() const -> uint16_t { return bits_; }

  constexpr auto operator==(const Move& other) const -> bool {
    return bits_ == other.bits_;
  }
  constexpr auto operator!=(const Move& other) const -> bool {
    return bits_ != other.bits_;
  }
};

#endif  // !MOVE_HPP
