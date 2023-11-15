#pragma OPENCL EXTENSION cl_khr_subgroups : enable

void kernel foo(global float* in_data, global float* out_data, float n) {
  //   const uint = 64;

  local float sdata[1024];

  const uint g_num_elements = convert_uint(n);

  const uint i = get_global_id(0);
  const uint tid = get_local_id(0);
  const uint group_size = get_local_size(0);

  // load shared mem
  sdata[tid] = (i < g_num_elements) ? in_data[i] : 0.0;
  barrier(CLK_LOCAL_MEM_FENCE);

  // do reduction in shared mem
  for (uint s = 1; s < group_size; s *= 2) {
    if (tid % (2 * s) == 0) {
      sdata[tid] += sdata[tid + s];
    }
    barrier(CLK_LOCAL_MEM_FENCE);
  }

  // write result for this block to global mem
  if (tid == 0) {
    out_data[get_group_id(0)] = sdata[0];
  }

  //   out_data[i] = in_data[i];
}