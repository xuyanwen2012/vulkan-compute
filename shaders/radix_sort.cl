
// clang-format off

// RUN: clspv -O0 --spv-version=1.5 --cl-std=CLC++ -inline-entry-points radix_sort.cl -o compiled_shaders/radix_sort.spv
// 
// RUN: clspv-reflection --target-env spv1.5 compiled_shaders/radix_sort.spv
//
// clang-format on

#pragma OPENCL EXTENSION cl_khr_subgroups : enable

const uint WORKGROUP_SIZE = 256;
const uint RADIX_SORT_BINS = 256;
const uint SUBGROUP_SIZE = 64;  // 32 NVIDIA; 64 AMD
const uint BITS = 32;           //  sorting uint32_t
const uint ITERATIONS = 4;      // sorting 8 bits per iteration
// struct BinFlags {
//   uint flags[WORKGROUP_SIZE / BITS];
// };

typedef struct {
  uint flags[WORKGROUP_SIZE / BITS];
} BinFlags;

#define ELEMENT_IN(index, iteration) \
  (iteration % 2 == 0 ? g_elements_in[index] : g_elements_out[index])

kernel void foo(global uint *g_elements_in,
                global uint *g_elements_out,
                const float n) {
  local uint histogram[RADIX_SORT_BINS];
  local uint sums[RADIX_SORT_BINS / SUBGROUP_SIZE];  // subgroup reductions
  local uint local_offsets[RADIX_SORT_BINS];
  local uint global_offsets[RADIX_SORT_BINS];
  local BinFlags bin_flags[RADIX_SORT_BINS];

  // uint index = get_global_id(0);
  // if (index >= g_num_elements) return;

  uint g_num_elements = convert_uint(n);
  const uint lID = get_local_id(0);            // thread ID in workgroup (CUDA)
  const uint sID = get_sub_group_id();         // warp ID in workgroup (CUDA)
  const uint lsID = get_sub_group_local_id();  // thread ID in warp (CUDA)

  for (uint iteration = 0; iteration < ITERATIONS; iteration++) {
    uint shift = 8 * iteration;

    // initialize histogram
    if (lID < RADIX_SORT_BINS) {
      histogram[lID] = 0u;
    }
    // work_group_barrier(CLK_LOCAL_MEM_FENCE);
    barrier(CLK_LOCAL_MEM_FENCE);

    for (uint ID = lID; ID < g_num_elements; ID += WORKGROUP_SIZE) {
      // determine the bin
      const uint bin =
          (ELEMENT_IN(ID, iteration) >> shift) & (RADIX_SORT_BINS - 1);
      // increment the histogram
      atom_add(&histogram[bin], 1u);
    }
    barrier(CLK_LOCAL_MEM_FENCE);

    // // subgroup (warp-level?) reductions and subgroup prefix sums
    // if (lID < RADIX_SORT_BINS) {
    //   uint histogram_count = histogram[lID];
    //   uint sum = sub_group_reduce_add(histogram_count);
    //   uint prefix_sum = sub_group_scan_exclusive_add(histogram_count);
    //   local_offsets[lID] = prefix_sum;
    //   if (lsID == 0) {
    //     // one thread inside the warp/subgroup enters this section
    //     sums[sID] = sum;
    //   }
    // }
    // barrier(CLK_LOCAL_MEM_FENCE);

    // // global prefix sums (offsets)
    // if (sID == 0) {
    //   uint offset = 0;
    //   for (uint i = lsID; i < RADIX_SORT_BINS; i += SUBGROUP_SIZE) {
    //     global_offsets[i] = offset + local_offsets[i];
    //     offset += sums[i / SUBGROUP_SIZE];
    //   }
    // }
    // barrier(CLK_LOCAL_MEM_FENCE);

    //     ==== scatter keys according to global offsets =====

    // const uint flags_bin = lID / BITS;
    // const uint flags_bit = 1 << (lID % BITS);

    // for (uint blockID = 0; blockID < g_num_elements;
    //      blockID += WORKGROUP_SIZE) {
    //   barrier(CLK_LOCAL_MEM_FENCE);

    //   const uint ID = blockID + lID;

    //   // initialize bin flags
    //   if (lID < RADIX_SORT_BINS) {
    //     for (uint i = 0; i < WORKGROUP_SIZE / BITS; i++) {
    //       bin_flags[lID].flags[i] = 0U;  // init all bin flags to 0
    //     }
    //   }
    //   barrier(CLK_LOCAL_MEM_FENCE);

    //   uint element_in = 0;
    //   uint binID = 0;
    //   uint binOffset = 0;
    //   if (ID < g_num_elements) {
    //     // element_in = ELEMENT_IN(ID, iteration);
    //     if (iteration % 2 == 0) {
    //       element_in = g_elements_in[ID];
    //     } else {
    //       element_in = g_elements_out[ID];
    //     }

    //     binID = (element_in >> shift) & uint(RADIX_SORT_BINS - 1);
    //     // offset for group
    //     binOffset = global_offsets[binID];
    //     // add bit to flag
    //     uint tmp = atom_add(&bin_flags[binID].flags[flags_bin], flags_bit);
    //     bin_flags[binID].flags[flags_bin] = tmp;
    //   }
    //   barrier(CLK_LOCAL_MEM_FENCE);
    //   // work_group_barrier(CLK_LOCAL_MEM_FENCE);

    //   if (ID < g_num_elements) {
    //     // calculate output index of element
    //     uint prefix = 0;
    //     uint count = 0;
    //     for (uint i = 0; i < WORKGROUP_SIZE / BITS; i++) {
    //       const uint bits = bin_flags[binID].flags[i];
    //       const uint full_count = popcount(bits);
    //       const uint partial_count = popcount(bits & (flags_bit - 1));
    //       prefix += (i < flags_bin) ? full_count : 0U;
    //       prefix += (i == flags_bin) ? partial_count : 0U;
    //       count += full_count;
    //     }

    //     if (iteration % 2 == 0) {
    //       g_elements_out[binOffset + prefix] = element_in;
    //     } else {
    //       g_elements_in[binOffset + prefix] = element_in;
    //     }
    //     if (prefix == count - 1) {
    //       global_offsets[binID] = atom_add(&global_offsets[binID],
    //       flags_bit);
    //     }
    //   }
    // }
    //
  }
}
