#pragma once

#include <glm/glm.hpp>

namespace morton {

inline glm::uint expand_bit(const glm::uint a) {
  glm::uint x = a & 0x000003FF;
  x = (x | (x << 16)) & 0x030000FF;
  x = (x | (x << 8)) & 0x0300F00F;
  x = (x | (x << 4)) & 0x030C30C3;
  x = (x | (x << 2)) & 0x09249249;
  return x;
}

inline glm::uint encode(const glm::uint i,
                        const glm::uint j,
                        const glm::uint k) {
  return expand_bit(i) | expand_bit(j) << 1 | expand_bit(k) << 2;
}

inline void foo(const glm::vec4 *in_xyz,
                glm::uint *out,
                const float n,
                const float min_coord,
                const float range) {
  for (int index = 0; index < n; ++index) {
    constexpr glm::uint code_len = 31;
    const float x = in_xyz[index].x;
    const float y = in_xyz[index].y;
    const float z = in_xyz[index].z;
    constexpr glm::uint bit_scale =
        0xFFFFFFFFu >> (32 - (code_len / 3));   // 1023
    constexpr float bit_scale_f = (bit_scale);  // 1023

    const glm::uint i = (bit_scale_f * ((x - min_coord) / range));
    const glm::uint j = (bit_scale_f * ((y - min_coord) / range));
    const glm::uint k = (bit_scale_f * ((z - min_coord) / range));

    out[index] = encode(i, j, k);
  }
}

}  // namespace morton