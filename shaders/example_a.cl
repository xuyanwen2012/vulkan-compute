// clang-format off

// RUN: clspv --spv-version=1.5 --cl-std=CLC++ -inline-entry-points example_a.cl -o example_a.spv
// 
// RUN: clspv-reflection --target-env spv1.5 example_a.spv
//
// .cinit --output-format=c

// clang-format on

struct MyPushConstant {
  uint n;
};

__kernel void example_a(__global float4 *a, __global float4 *c,
                        const MyPushConstant push_constant) {
  uint n = push_constant.n;

  uint index = get_global_id(0);
  if (index >= n)
    return;

  c[index] = a[index] * 2.0f;
}
