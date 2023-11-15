typedef struct {
  int delta;
  int left;
  int right;
  int parent;
} InnerNode;

void kernel foo(global InnerNode* brt_nodes, global int* edge_count, float n) {
  const uint g_num_elements = convert_uint(n);
  const uint i = get_global_id(0);
  if (i > g_num_elements) return;

  const int my_depth = brt_nodes[i].delta / 3;
  const int parent_depth = brt_nodes[brt_nodes[i].parent].delta / 3;
  edge_count[i] = my_depth - parent_depth;
}
