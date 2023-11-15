#pragma OPENCL EXTENSION cl_khr_subgroups : enable

#define CODE_BIT 32

typedef struct {
  int delta;
  int left;
  int right;
  int parent;
} InnerNode;

int sign(int val) { return (0 < val) - (val < 0); }

int div2ceil(int val) { return (val + 1) >> 1; }

int make_leaf(int index) {
  return index ^ ((-1 ^ index) & 1u << (CODE_BIT - 1));
}

int make_internal(int index) { return index; }

int delta(uint *morton_keys, int i, int j) {
  const uint li = morton_keys[i];
  const uint lj = morton_keys[j];
  return clz(li ^ lj) - 1;
}

int delta_safe(uint *morton_keys, int key_num, int i, int j) {
  return (j < 0 || j >= key_num) ? -1 : delta(morton_keys, i, j);
}

// Ported from
// https://github.com/xuyanwen2012/redwood-mapping/blob/quickly_change/bench_gpu/brt.cuh
//
kernel void foo(global uint *g_morton_keys,
                global InnerNode *inner_nodes,
                float n) {
  const int i = get_global_id(0);
  const uint num_keys = convert_uint(n);
  if (i >= num_keys) return;

  int direction = sign(delta(g_morton_keys, i, i + 1) -
                       delta_safe(g_morton_keys, num_keys, i, i - 1));

  int delta_min = delta_safe(g_morton_keys, num_keys, i, i - direction);

  int I_max = 2;
  while (delta_safe(g_morton_keys, num_keys, i, i + I_max * direction) >
         delta_min) {
    I_max <<= 2;
  }

  // Find the other end using binary search.
  int I = 0;
  for (int t = I_max / 2; t; t /= 2) {
    if (delta_safe(g_morton_keys, num_keys, i, i + (I + t) * direction) >
        delta_min) {
      I += t;
    }
  }

  int j = i + I * direction;

  // Find the split position using binary search.
  int delta_node = delta_safe(g_morton_keys, num_keys, i, j);
  int s = 0;
  int t = I;

  do {
    t = div2ceil(t);
    if (delta_safe(g_morton_keys, num_keys, i, i + (s + t) * direction) >
        delta_node) {
      s += t;
    }
  } while (t > 1);

  int split = i + s * direction + min(direction, 0);

  int left = min(i, j) == split ? make_leaf(min(i, j)) : make_internal(split);
  int right =
      max(i, j) == split + 1 ? make_leaf(split + 1) : make_internal(split + 1);

  inner_nodes[i].delta = delta_node;
  inner_nodes[i].left = left;
  inner_nodes[i].right = right;

  if (min(i, j) != split) {
    inner_nodes[left].parent = i;
  }
  if (max(i, j) != split + 1) {
    inner_nodes[right].parent = i;
  }
}
