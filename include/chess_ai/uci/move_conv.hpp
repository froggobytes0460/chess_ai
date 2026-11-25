#ifndef CHESS_AI_UCI_MOVE_CONV_HPP
#define CHESS_AI_UCI_MOVE_CONV_HPP

#include <chess_ai/board/position.hpp>
#include <chess_ai/core/move.hpp>
#include <optional>
#include <string>
#include <string_view>

[[nodiscard]] auto moveToUci(Move move) -> std::string;
[[nodiscard]] auto uciToMove(const Position& pos, std::string_view uci)
    -> std::optional<Move>;

#endif  // !CHESS_AI_UCI_MOVE_CONV_HPP
