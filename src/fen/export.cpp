#include <array>
#include <chess_ai/bitops.hpp>
#include <chess_ai/position.hpp>
#include <chess_ai/types.hpp>
#include <string>

namespace {
inline auto getFenCharFromPiece(Color col, PieceType pt) -> char {
  switch (col) {
    case Color::WHITE:
      switch (pt) {
        case PieceType::KING:
          return 'K';
        case PieceType::QUEEN:
          return 'Q';
        case PieceType::ROOK:
          return 'R';
        case PieceType::BISHOP:
          return 'B';
        case PieceType::KNIGHT:
          return 'N';
        case PieceType::PAWN:
          return 'P';
      }
    case Color::BLACK:
      switch (pt) {
        case PieceType::KING:
          return 'k';
        case PieceType::QUEEN:
          return 'q';
        case PieceType::ROOK:
          return 'r';
        case PieceType::BISHOP:
          return 'b';
        case PieceType::KNIGHT:
          return 'n';
        case PieceType::PAWN:
          return 'p';
      }
  }
}

auto getPieceCharAt(const std::array<std::array<BitBoard, 6>, 2>& pieces,
                    Square sq) -> char {
  for (int col = 0; col < 2; ++col) {
    for (int piece = 0; piece < 6; ++piece) {
      if (getBit(pieces[col][piece], sq)) {
        return getFenCharFromPiece(static_cast<Color>(col),
                                   static_cast<PieceType>(piece));
      }
    }
  }
  return '\0';
}

void appendPiecePlacement(const std::array<std::array<BitBoard, 6>, 2>& pieces,
                          std::string& fen) {
  for (int rank = 7; rank >= 0; --rank) {
    int empty = 0;
    for (int file = 0; file < 8; ++file) {
      auto sq = static_cast<Square>((rank << 3) + file);
      char piece_char = getPieceCharAt(pieces, sq);
      if (piece_char == '\0') {
        ++empty;
        continue;
      }
      if (empty != 0) {
        fen += std::to_string(empty);
        empty = 0;
      }
      fen += piece_char;
    }
    if (empty != 0) {
      fen += std::to_string(empty);
    }
    if (rank != 0) {
      fen += '/';
    }
  }
}
}  // namespace

auto Position::toFEN() -> std::string {
  std::string fen;
  fen.reserve(90);

  appendPiecePlacement(pieces_, fen);

  // Side to move
  fen += ' ';
  fen += (side_to_move_ == Color::WHITE) ? 'w' : 'b';

  // Castling Rights
  fen += ' ';
  if (castling_rights_bits_ == 0) {
    fen += '-';
  } else {
    if ((castling_rights_bits_ &
         static_cast<unsigned>(CastlingRights::WHITE_KINGSIDE)) != 0) {
      fen += 'K';
    }
    if ((castling_rights_bits_ &
         static_cast<unsigned>(CastlingRights::WHITE_QUEENSIDE)) != 0) {
      fen += 'Q';
    }
    if ((castling_rights_bits_ &
         static_cast<unsigned>(CastlingRights::BLACK_KINGSIDE)) != 0) {
      fen += 'k';
    }
    if ((castling_rights_bits_ &
         static_cast<unsigned>(CastlingRights::BLACK_QUEENSIDE)) != 0) {
      fen += 'q';
    }
  }

  // En passant target
  fen += ' ';
  if (en_passant_ == Square::NONE) {
    fen += '-';
  } else {
    int sq = static_cast<int>(en_passant_);
    fen += static_cast<char>('a' + (sq % 8));
    fen += static_cast<char>('1' + (sq >> 3));
  }

  // Half move clock and full move counter.
  fen += ' ' + std::to_string(half_move_clock_);
  fen += ' ' + std::to_string(full_move_counter_);

  return fen;
}
