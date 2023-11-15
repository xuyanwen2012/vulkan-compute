#pragma once

#include <type_traits>
#include <vector>

template <typename T>
concept NumericT = std::is_arithmetic_v<std::remove_cvref_t<T>>;

/**
 * @brief Make a vector of floats that can be used as push constants. The
 * first 3 floats are 'region_offset'. In CLSPV, the first 20 bytes of push
 * constants is reserved. You can assume zeros for now. This function takes the
 * actual push constants.
 *
 * Check  https://github.com/google/clspv/blob/main/docs/OpenCLCOnVulkan.md
 * for more details.
 * @tparam Args The types of the push constants.
 * @param args The push constants.
 */
template <NumericT... Args>
[[nodiscard]] constexpr std::vector<float> make_clspv_push_const(
    Args &&...args) {
  return std::vector{
      0.0f, 0.0f, 0.0f, 0.0f, static_cast<float>(std::forward<Args>(args))...};
}
