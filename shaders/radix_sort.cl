
// clang-format off

// RUN: clspv --spv-version=1.5 --cl-std=CLC++ -inline-entry-points radix_sort.cl -o compiled_shaders/radix_sort.spv
// 
// RUN: clspv-reflection --target-env spv1.5 compiled_shaders/radix_sort.spv
//
// clang-format on

// const uint WARP_SIZE = 32;

// const uint WORKGROUP_SIZE = 256;
// const uint RADIX_SORT_BINS = 256;
// const uint SUBGROUP_SIZE = 64;  // 32 NVIDIA; 64 AMD

// const uint BITS = 32;       //  sorting uint32_t
// const uint ITERATIONS = 4;  // sorting 8 bits per iteration

// // reqd_work_group_size

// struct BinFlags {
//   uint flags[WORKGROUP_SIZE / BITS];
// };

#pragma OPENCL EXTENSION cl_khr_subgroups : enable

kernel void foo(global uint *in, global uint *out, const uint n) {
  uint index = get_global_id(0);
  if (index >= convert_uint(n)) return;

  in[index] = index;

  uint lID = get_local_id(0);
  uint sID = get_sub_group_id();
  uint lsID = get_sub_group_local_id();

  out[index] = lsID;
}
