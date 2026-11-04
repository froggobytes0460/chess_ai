#include <chess_ai/position.hpp>
#include <chess_ai/types.hpp>
#include <string_view>

Position::Position(std::string_view fen)
    : pieces_{},
      occupied_{},
      occupied_all_{},
      full_move_counter_{},
      half_move_clock_{},
      castling_rights_bits_{},
      side_to_move_{Color::WHITE},
      en_passant_{Square::NONE} {
  (void)fen;
}
