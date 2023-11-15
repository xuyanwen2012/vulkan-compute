#pragma once

#include <cstdint>
#include <glm/glm.hpp>

using Code_t = uint32_t;
constexpr int kCodeLen = 31;
constexpr uint32_t bit_scale = 0xFFFFFFFFu >> (32 - (kCodeLen / 3));  // 1023

namespace morton {

constexpr uint_fast32_t magicbit3D_masks32_encode[6] = {
    0x000003ff, 0, 0x30000ff, 0x0300f00f, 0x30c30c3, 0x9249249};

constexpr uint32_t expand_bit(const uint32_t a) {
  uint32_t x = a & magicbit3D_masks32_encode[0];
  x = (x | (x << 16)) & magicbit3D_masks32_encode[2];
  x = (x | (x << 8)) & magicbit3D_masks32_encode[3];
  x = (x | (x << 4)) & magicbit3D_masks32_encode[4];
  x = (x | (x << 2)) & magicbit3D_masks32_encode[5];
  return x;
}

constexpr uint_fast32_t magicbit3D_masks32_decode[6]{
    0, 0x000003ff, 0x30000ff, 0x0300f00f, 0x30c30c3, 0x9249249};

constexpr uint32_t compress_bit(const uint32_t a) {
  uint32_t x = a & magicbit3D_masks32_decode[5];
  x = (x ^ (x >> 2)) & magicbit3D_masks32_decode[4];
  x = (x ^ (x >> 4)) & magicbit3D_masks32_decode[3];
  x = (x ^ (x >> 8)) & magicbit3D_masks32_decode[2];
  x = (x ^ (x >> 16)) & magicbit3D_masks32_decode[1];
  return static_cast<uint32_t>(x);
}

constexpr uint32_t encode(const uint32_t i,
                          const uint32_t j,
                          const uint32_t k) {
  return expand_bit(i) | expand_bit(j) << 1 | expand_bit(k) << 2;
}

constexpr void decode(const uint32_t m, uint32_t &x, uint32_t &y, uint32_t &z) {
  x = compress_bit(m);
  y = compress_bit(m >> 1);
  z = compress_bit(m >> 2);
}

constexpr Code_t single_point_to_code(const float x,
                                      const float y,
                                      const float z,
                                      const float min_coord,
                                      const float range) {
  const uint32_t i = (bit_scale * ((x - min_coord) / range));
  const uint32_t j = (bit_scale * ((y - min_coord) / range));
  const uint32_t k = (bit_scale * ((z - min_coord) / range));
  return encode(i, j, k);
}

constexpr void single_code_to_point(const Code_t code,
                                    float &dec_x,
                                    float &dec_y,
                                    float &dec_z,
                                    const float min_coord,
                                    const float range) {
  uint32_t dec_raw_x = 0, dec_raw_y = 0, dec_raw_z = 0;
  decode(code, dec_raw_x, dec_raw_y, dec_raw_z);
  dec_x = (static_cast<float>(dec_raw_x) / bit_scale) * range + min_coord;
  dec_y = (static_cast<float>(dec_raw_y) / bit_scale) * range + min_coord;
  dec_z = (static_cast<float>(dec_raw_z) / bit_scale) * range + min_coord;
}

// Note, for all points
inline void point_to_morton32(const glm::vec4 *in_xyz,
                              uint32_t *out,
                              const float n,
                              const float min_coord,
                              const float range) {
  for (int index = 0; index < n; ++index) {
    const float x = in_xyz[index].x;
    const float y = in_xyz[index].y;
    const float z = in_xyz[index].z;
    out[index] = single_point_to_code(x, y, z, min_coord, range);
  }
}

}  // namespace morton