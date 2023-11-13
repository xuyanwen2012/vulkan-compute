#pragma once

#include <glm/glm.hpp>

namespace morton {

inline glm::uint expand_bit(glm::uint a) {
  glm::uint x = a & 0x000003FF;
  x = (x | (x << 16)) & 0x030000FF;
  x = (x | (x << 8)) & 0x0300F00F;
  x = (x | (x << 4)) & 0x030C30C3;
  x = (x | (x << 2)) & 0x09249249;
  return x;
}

inline glm::uint encode(glm::uint i, glm::uint j, glm::uint k) {
  return expand_bit(i) | expand_bit(j) << 1 | expand_bit(k) << 2;
}

inline void foo(
    glm::vec4 *in_xyz, glm::uint *out, float n, float min_coord, float range) {
  glm::uint kCodeLen = 31;

  for (int index = 0; index < n; ++index) {
    float x = in_xyz[index].x;
    float y = in_xyz[index].y;
    float z = in_xyz[index].z;
    glm::uint bit_scale = 0xFFFFFFFFu >> (32 - (kCodeLen / 3));  // 1023
    float bit_scale_f = (bit_scale);                             // 1023

    glm::uint i = (bit_scale_f * ((x - min_coord) / range));
    glm::uint j = (bit_scale_f * ((y - min_coord) / range));
    glm::uint k = (bit_scale_f * ((z - min_coord) / range));

    out[index] = encode(i, j, k);
  }
}

}  // namespace morton