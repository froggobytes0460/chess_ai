#ifndef CHESS_AI_EVAL_EVAL_HPP
#define CHESS_AI_EVAL_EVAL_HPP

#include <chess_ai/board/position.hpp>
#include <cstdint>

[[nodiscard]] auto evaluate(const Position& pos) -> int32_t;

#endif  // !CHESS_AI_EVAL_EVAL_HPP
