// clang-format off

// RUN: clspv --spv-version=1.5 --cl-std=CLC++ -inline-entry-points float_doubler.cl -o compiled_shaders/float_doubler.spv
// 
// RUN: clspv-reflection --target-env spv1.5 compiled_shaders/float_doubler.spv
//
// .cinit --output-format=c

// clang-format on

__kernel void foo(__global float *in, __global float *out, const uint n) {
  uint index = get_global_id(0);
  if (index >= n) return;

  out[index] = in[index] * 2.0f;
}
