#pragma once

#include <array>
#include <cstdint>
#include <glm/glm.hpp>

using CodeT = uint32_t;
constexpr int kCodeLen = 31;

namespace morton {

constexpr std::array<uint_fast32_t, 6> kMagicbit3DMasks32Encode{
    0x000003ff, 0, 0x30000ff, 0x0300f00f, 0x30c30c3, 0x9249249};

constexpr uint32_t expand_bit(const uint32_t a) {
  uint32_t x = a & kMagicbit3DMasks32Encode[0];
  x = (x | x << 16) & kMagicbit3DMasks32Encode[2];
  x = (x | x << 8) & kMagicbit3DMasks32Encode[3];
  x = (x | x << 4) & kMagicbit3DMasks32Encode[4];
  x = (x | x << 2) & kMagicbit3DMasks32Encode[5];
  return x;
}

constexpr std::array<uint_fast32_t, 6> kMagicbit3DMasks32Decode{
    0, 0x000003ff, 0x30000ff, 0x0300f00f, 0x30c30c3, 0x9249249};

constexpr uint32_t compress_bit(const uint32_t a) {
  uint32_t x = a & kMagicbit3DMasks32Decode[5];
  x = (x ^ x >> 2) & kMagicbit3DMasks32Decode[4];
  x = (x ^ x >> 4) & kMagicbit3DMasks32Decode[3];
  x = (x ^ x >> 8) & kMagicbit3DMasks32Decode[2];
  x = (x ^ x >> 16) & kMagicbit3DMasks32Decode[1];
  return x;
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

constexpr std::array<uint32_t, 3> decode_v2(const uint32_t m) {
  return {compress_bit(m), compress_bit(m >> 1), compress_bit(m >> 2)};
}

constexpr CodeT single_point_to_code_v2(
    float x, float y, float z, const float min_coord, const float range) {
  x = (x - min_coord) / range;
  y = (y - min_coord) / range;
  z = (z - min_coord) / range;

  return encode(static_cast<uint32_t>(x * 1024),
                static_cast<uint32_t>(y * 1024),
                static_cast<uint32_t>(z * 1024));
}

constexpr std::array<float, 3> single_code_to_point_v2(const CodeT code,
                                                       const float min_coord,
                                                       const float range) {
  const auto raw = decode_v2(code);
  return {static_cast<float>(raw[0]) / 1024.0f * range + min_coord,
          static_cast<float>(raw[1]) / 1024.0f * range + min_coord,
          static_cast<float>(raw[2]) / 1024.0f * range + min_coord};
}

// Note, for all points
inline void point_to_morton32(const glm::vec4 *in_xyz,
                              uint32_t *out,
                              const size_t n,
                              const float min_coord,
                              const float range) {
  for (auto index = 0u; index < n; ++index) {
    const float x = in_xyz[index].x;
    const float y = in_xyz[index].y;
    const float z = in_xyz[index].z;
    out[index] = single_point_to_code_v2(x, y, z, min_coord, range);
  }
}

}  // namespace morton