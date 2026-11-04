#ifndef POSITION_HPP
#define POSITION_HPP

#include <array>
#include <chess_ai/types.hpp>
#include <cstddef>
#include <cstdint>
#include <string_view>

class Position {
 private:
  std::array<std::array<BitBoard, 6>, 2> pieces_;
  std::array<BitBoard, 2> occupied_;
  BitBoard occupied_all_;
  uint16_t full_move_counter_;
  uint16_t half_move_clock_;
  uint8_t castling_rights_bits_;
  Color side_to_move_;
  Square en_passant_;

  friend class PositionTest;

 public:
  Position();
  explicit Position(std::string_view fen);
  auto toFEN() -> std::string;
};

#endif  // !POSITION_HPP
