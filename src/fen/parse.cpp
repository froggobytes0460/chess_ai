#include <charconv>
#include <chess_ai/bitops.hpp>
#include <chess_ai/position.hpp>
#include <chess_ai/types.hpp>
#include <string_view>

namespace {
// Returns the {color, piece} pair for a FEN piece character, or
// {Color::WHITE, PieceType::KING} with `ok` set false if `c` is not a valid
// piece character.
struct FenPiece {
  Color color;
  PieceType type;
  bool ok;
};

inline auto getPieceFromFenChar(char c) -> FenPiece {
  switch (c) {
    case 'K':
      return {.color = Color::WHITE, .type = PieceType::KING, .ok = true};
    case 'Q':
      return {.color = Color::WHITE, .type = PieceType::QUEEN, .ok = true};
    case 'R':
      return {.color = Color::WHITE, .type = PieceType::ROOK, .ok = true};
    case 'B':
      return {.color = Color::WHITE, .type = PieceType::BISHOP, .ok = true};
    case 'N':
      return {.color = Color::WHITE, .type = PieceType::KNIGHT, .ok = true};
    case 'P':
      return {.color = Color::WHITE, .type = PieceType::PAWN, .ok = true};
    case 'k':
      return {.color = Color::BLACK, .type = PieceType::KING, .ok = true};
    case 'q':
      return {.color = Color::BLACK, .type = PieceType::QUEEN, .ok = true};
    case 'r':
      return {.color = Color::BLACK, .type = PieceType::ROOK, .ok = true};
    case 'b':
      return {.color = Color::BLACK, .type = PieceType::BISHOP, .ok = true};
    case 'n':
      return {.color = Color::BLACK, .type = PieceType::KNIGHT, .ok = true};
    case 'p':
      return {.color = Color::BLACK, .type = PieceType::PAWN, .ok = true};
    default:
      return {.color = Color::WHITE, .type = PieceType::KING, .ok = false};
  }
}

void parsePiecePlacement(std::string_view field,
                         std::array<std::array<BitBoard, 6>, 2>& pieces) {
  int rank = 7;
  int file = 0;
  for (const char c : field) {
    if (c == '/') {
      --rank;
      file = 0;
      continue;
    }
    if (c >= '1' && c <= '8') {
      file += c - '0';
      continue;
    }
    FenPiece piece = getPieceFromFenChar(c);
    auto sq = static_cast<Square>((rank << 3) + file);
    setBit(pieces[static_cast<unsigned>(piece.color)]
                 [static_cast<unsigned>(piece.type)],
           sq);
    ++file;
  }
}

auto parseCastlingRights(std::string_view field) -> uint8_t {
  uint8_t bits = 0;
  for (char c : field) {
    switch (c) {
      case 'K':
        bits |= static_cast<uint8_t>(CastlingRights::WHITE_KINGSIDE);
        break;
      case 'Q':
        bits |= static_cast<uint8_t>(CastlingRights::WHITE_QUEENSIDE);
        break;
      case 'k':
        bits |= static_cast<uint8_t>(CastlingRights::BLACK_KINGSIDE);
        break;
      case 'q':
        bits |= static_cast<uint8_t>(CastlingRights::BLACK_QUEENSIDE);
        break;
      default:
        break;
    }
  }
  return bits;
}

auto parseEnPassant(std::string_view field) -> Square {
  if (field == "-") {
    return Square::NONE;
  }
  int file = field[0] - 'a';
  int rank = field[1] - '1';
  return static_cast<Square>((rank << 3) + file);
}

auto parseU16(std::string_view field) -> uint16_t {
  uint16_t value = 0;
  std::from_chars(field.data(), field.data() + field.size(), value);
  return value;
}

// Returns the next whitespace-delimited field, advancing `rest` past it.
auto nextField(std::string_view& rest) -> std::string_view {
  std::size_t end = rest.find(' ');
  std::string_view field = rest.substr(0, end);
  rest = (end == std::string_view::npos) ? std::string_view{}
                                         : rest.substr(end + 1);
  return field;
}
}  // namespace

Position::Position(std::string_view fen)
    : pieces_{},  // NOLINT(readability-trailing-comma)
      occupied_{},
      occupied_all_{},
      full_move_counter_{},
      half_move_clock_{},
      castling_rights_bits_{},
      side_to_move_{Color::WHITE},
      en_passant_{Square::NONE} {
  std::string_view rest = fen;

  parsePiecePlacement(nextField(rest), pieces_);
  side_to_move_ = (nextField(rest) == "w") ? Color::WHITE : Color::BLACK;
  castling_rights_bits_ = parseCastlingRights(nextField(rest));
  en_passant_ = parseEnPassant(nextField(rest));
  half_move_clock_ = parseU16(nextField(rest));
  full_move_counter_ = parseU16(nextField(rest));

  for (int col = 0; col < 2; ++col) {
    for (int piece = 0; piece < 6; ++piece) {
      occupied_[col] |= pieces_[col][piece];
    }
    occupied_all_ |= occupied_[col];
  }
}
