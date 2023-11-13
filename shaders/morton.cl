// clang-format off

// RUN: clspv --spv-version=1.5 --cl-std=CLC++ -inline-entry-points morton.cl -o compiled_shaders/morton.spv
// 
// RUN: clspv-reflection --target-env spv1.5 compiled_shaders/morton.spv

// clang-format on

// Forward declaration
uint expand_bit(uint a);
uint encode(uint i, uint j, uint k);

struct MyPushConstant {
  uint n;
  float min_coord;
  float range;
};

uint expand_bit(uint a) {
  uint x = a & 0x000003FF;
  x = (x | (x << 16)) & 0x030000FF;
  x = (x | (x << 8)) & 0x0300F00F;
  x = (x | (x << 4)) & 0x030C30C3;
  x = (x | (x << 2)) & 0x09249249;
  return x;
}

uint encode(uint i, uint j, uint k) {
  return expand_bit(i) | expand_bit(j) << 1 | expand_bit(k) << 2;
}

__kernel void foo(__global float4 *in_xyz, __global uint *out,
                  const struct MyPushConstant push_constant) {
  const uint kCodeLen = 31;

  uint n = push_constant.n;
  float min_coord = push_constant.min_coord;
  float range = push_constant.range;

  uint index = get_global_id(0);
  if (index >= n)
    return;

  // printf("index: %d\n", index);

  float x = in_xyz[index].x;
  float y = in_xyz[index].y;
  float z = in_xyz[index].z;
  uint bit_scale = 0xFFFFFFFFu >> (32 - (kCodeLen / 3)); // 1023
  float bit_scale_f = convert_float(bit_scale);          // 1023

  uint i = convert_uint(bit_scale_f * ((x - min_coord) / range));
  uint j = convert_uint(bit_scale_f * ((y - min_coord) / range));
  uint k = convert_uint(bit_scale_f * ((z - min_coord) / range));

  out[index] = encode(i, j, k);
}
