#include <gtest/gtest.h>

#include <array>
#include <chess_ai/board/position.hpp>
#include <chess_ai/core/types.hpp>
#include <cstdint>
#include <string>
#include <tuple>

namespace {
constexpr const char* K_START_POS =
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
}  // namespace

// --- Piece placement parsing (parameterized: one case per piece/color) ---

struct PieceBoardCase {
  Color color;
  PieceType type;
  BitBoard expected;
};

class StartPosPieceBoardTest : public ::testing::TestWithParam<PieceBoardCase> {
};

TEST_P(StartPosPieceBoardTest, MatchesKnownBitboard) {
  Position pos(K_START_POS);
  EXPECT_EQ(pos.pieces(GetParam().color, GetParam().type), GetParam().expected);
}

auto nameStartPosPieceBoardCase(
    const ::testing::TestParamInfo<PieceBoardCase>& info) -> std::string {
  static constexpr auto K_PIECE_NAMES = std::to_array<const char*>({
      "King",
      "Queen",
      "Rook",
      "Bishop",
      "Knight",
      "Pawn",
  });
  std::string color = info.param.color == Color::WHITE ? "White" : "Black";
  return color + K_PIECE_NAMES[static_cast<unsigned>(info.param.type)];
}

INSTANTIATE_TEST_SUITE_P(
    StartPos, StartPosPieceBoardTest,
    ::testing::Values(PieceBoardCase{Color::WHITE, PieceType::PAWN,
                                     uint64_t{0x000000000000FF00}},
                      PieceBoardCase{Color::BLACK, PieceType::PAWN,
                                     uint64_t{0x00FF000000000000}},
                      PieceBoardCase{Color::WHITE, PieceType::ROOK,
                                     uint64_t{0x0000000000000081}},
                      PieceBoardCase{Color::BLACK, PieceType::ROOK,
                                     uint64_t{0x8100000000000000}},
                      PieceBoardCase{Color::WHITE, PieceType::KNIGHT,
                                     uint64_t{0x0000000000000042}},
                      PieceBoardCase{Color::WHITE, PieceType::BISHOP,
                                     uint64_t{0x0000000000000024}},
                      PieceBoardCase{Color::WHITE, PieceType::QUEEN,
                                     uint64_t{0x0000000000000008}},
                      PieceBoardCase{Color::WHITE, PieceType::KING,
                                     uint64_t{0x0000000000000010}},
                      PieceBoardCase{Color::BLACK, PieceType::KING,
                                     uint64_t{0x1000000000000000}}),
    nameStartPosPieceBoardCase);

TEST(PositionTest, EmptyBoardAllZero) {
  Position pos("8/8/8/8/8/8/8/8 w - - 0 1");
  for (int c = 0; c < 2; ++c) {
    for (int p = 0; p < 6; ++p) {
      EXPECT_EQ(pos.pieces(static_cast<Color>(c), static_cast<PieceType>(p)),
                uint64_t{0})
          << "color=" << c << " piece=" << p;
    }
  }
}

TEST(PositionTest, SparsePiecePlacement) {
  // White king e1, black king e8, white rook a1, black knight g6.
  Position pos("4k3/8/6n1/8/8/8/8/R3K3 w - - 0 1");
  EXPECT_EQ(pos.pieces(Color::WHITE, PieceType::KING),
            uint64_t{0x0000000000000010});  // e1
  EXPECT_EQ(pos.pieces(Color::BLACK, PieceType::KING),
            uint64_t{0x1000000000000000});  // e8
  EXPECT_EQ(pos.pieces(Color::WHITE, PieceType::ROOK),
            uint64_t{0x0000000000000001});  // a1
  EXPECT_EQ(pos.pieces(Color::BLACK, PieceType::KNIGHT),
            uint64_t{0x0000400000000000});  // g6
}

// --- Side to move (parameterized) ---

class SideToMoveTest
    : public ::testing::TestWithParam<std::tuple<char, Color>> {};

TEST_P(SideToMoveTest, ParsedCorrectly) {
  auto [fen_char, expected] = GetParam();
  std::string fen = K_START_POS;
  fen[fen.find(" w ") + 1] = fen_char;
  Position pos(fen);
  EXPECT_EQ(pos.sideToMove(), expected);
}

// NOLINTNEXTLINE(modernize-use-string-view)
auto nameSideToMoveCase(
    const ::testing::TestParamInfo<std::tuple<char, Color>>& info)
    -> std::string {
  return std::get<1>(info.param) == Color::WHITE ? "White" : "Black";
}

INSTANTIATE_TEST_SUITE_P(SideToMove, SideToMoveTest,
                         ::testing::Values(std::make_tuple('w', Color::WHITE),
                                           std::make_tuple('b', Color::BLACK)),
                         nameSideToMoveCase);

// --- Castling rights (parameterized) ---

struct CastlingCase {
  const char* fen_field;
  uint8_t expected;
};

class CastlingRightsTest : public ::testing::TestWithParam<CastlingCase> {};

TEST_P(CastlingRightsTest, ParsedCorrectly) {
  std::string fen =
      std::string("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w ") +
      GetParam().fen_field + " - 0 1";
  Position pos(fen);
  EXPECT_EQ(pos.castlingRightsBits(), GetParam().expected);
}

auto nameCastlingCase(const ::testing::TestParamInfo<CastlingCase>& info)
    -> std::string {
  std::string field = info.param.fen_field;
  return field == "-" ? "None" : field;
}

INSTANTIATE_TEST_SUITE_P(
    Castling, CastlingRightsTest,
    ::testing::Values(
        CastlingCase{"KQkq",
                     static_cast<uint8_t>(CastlingRights::WHITE_KINGSIDE) |
                         static_cast<uint8_t>(CastlingRights::WHITE_QUEENSIDE) |
                         static_cast<uint8_t>(CastlingRights::BLACK_KINGSIDE) |
                         static_cast<uint8_t>(CastlingRights::BLACK_QUEENSIDE)},
        CastlingCase{"-", 0},
        CastlingCase{
            "Kq", static_cast<uint8_t>(CastlingRights::WHITE_KINGSIDE) |
                      static_cast<uint8_t>(CastlingRights::BLACK_QUEENSIDE)}),
    nameCastlingCase);

// --- En passant ---

TEST(PositionTest, EnPassantNone) {
  Position pos(K_START_POS);
  EXPECT_EQ(pos.enPassantSquare(), Square::NONE);
}

TEST(PositionTest, EnPassantSet) {
  Position pos("rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1");
  EXPECT_EQ(pos.enPassantSquare(), Square::E3);
}

// --- Halfmove clock / fullmove number (parameterized) ---

struct CounterCase {
  const char* fen_suffix;
  uint16_t expected_halfmove;
  uint16_t expected_fullmove;
};

class CountersTest : public ::testing::TestWithParam<CounterCase> {};

TEST_P(CountersTest, ParsedCorrectly) {
  std::string fen =
      std::string("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq ") +
      GetParam().fen_suffix;
  Position pos(fen);
  EXPECT_EQ(pos.halfMoveClock(), GetParam().expected_halfmove);
  EXPECT_EQ(pos.fullMoveCounter(), GetParam().expected_fullmove);
}

auto nameCounterCase(const ::testing::TestParamInfo<CounterCase>& info)
    -> std::string {
  return "Halfmove" + std::to_string(info.param.expected_halfmove) +
         "Fullmove" + std::to_string(info.param.expected_fullmove);
}

INSTANTIATE_TEST_SUITE_P(Counters, CountersTest,
                         ::testing::Values(CounterCase{"- 0 1", 0, 1},
                                           CounterCase{"- 5 23", 5, 23}),
                         nameCounterCase);

// --- Derived occupancy ---

TEST(PositionTest, OccupancyMatchesStartPos) {
  Position pos(K_START_POS);
  EXPECT_EQ(pos.occupied(Color::WHITE), uint64_t{0x000000000000FFFF});
  EXPECT_EQ(pos.occupied(Color::BLACK), uint64_t{0xFFFF000000000000});
  EXPECT_EQ(pos.occupiedAll(), uint64_t{0xFFFF00000000FFFF});
}

TEST(PositionTest, OccupancyEmptyBoard) {
  Position pos("8/8/8/8/8/8/8/8 w - - 0 1");
  EXPECT_EQ(pos.occupied(Color::WHITE), uint64_t{0});
  EXPECT_EQ(pos.occupied(Color::BLACK), uint64_t{0});
  EXPECT_EQ(pos.occupiedAll(), uint64_t{0});
}

// --- toFEN round-trip (parameterized: one case per FEN variant) ---

class RoundTripTest : public ::testing::TestWithParam<const char*> {};

TEST_P(RoundTripTest, MatchesInputFen) {
  std::string fen = GetParam();
  Position pos(fen);
  EXPECT_EQ(pos.toFEN(), fen);
}

INSTANTIATE_TEST_SUITE_P(
    FenVariants, RoundTripTest,
    ::testing::Values(
        K_START_POS,
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
        "rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1",
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w Kq - 0 1",
        "4k3/8/8/8/8/8/8/4K3 w - - 0 1",
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 17 42",
        "4k3/8/6n1/8/8/8/8/R3K3 w - - 3 10"));
