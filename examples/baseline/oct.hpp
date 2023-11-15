#pragma once

#include <glm/glm.hpp>

#include "brt.hpp"
#include "morton.hpp"

namespace oct {

inline void CalculateEdgeCountHelper(const int i,
                                     int *edge_count,
                                     const brt::InnerNode *inners) {
  const int my_depth = inners[i].delta_node / 3;
  const int parent_depth = inners[inners[i].parent].delta_node / 3;
  edge_count[i] = my_depth - parent_depth;
}

struct Body {
  float mass;
};

struct OctNode {
  // Payload
  Body body;

  // glm::vec3 cornor;
  glm::vec4 cornor;  // i am not sure if I should ever use vec3
  float cell_size;

  // TODO: This is overkill number of pointers
  int children[8];

  /**
   * @brief For bit position i (from the right): If 1, children[i] is the index
   * of a child octree node. If 0, the ith child is either absent, or
   * children[i] is the index of a leaf.
   */
  int child_node_mask;

  /**
   * @brief For bit position i (from the right): If 1, children[i] is the index
   * of a leaf (in the corresponding points array). If 0, the ith child is
   * either absent, or an octree node.
   */
  int child_leaf_mask;

  /**
   * @brief Set a child
   *
   * @param child: index of octree node that will become the child
   * @param my_child_idx: which of my children it will be [0-7]
   */
  void SetChild(int child, int my_child_idx);

  /**
   * @brief Set the Leaf object
   *
   * @param leaf: index of point that will become the leaf child
   * @param my_child_idx: which of my children it will be [0-7]
   */
  void SetLeaf(int leaf, int my_child_idx);
};

inline void OctNode::SetChild(const int child, const int my_child_idx) {
  // children_array[my_child_idx] = child;
  children[my_child_idx] = child;
  child_node_mask |= (1 << my_child_idx);
  //   atomicOr(&child_node_mask, 1 << my_child_idx);
}

inline void OctNode::SetLeaf(const int leaf, const int my_child_idx) {
  children[my_child_idx] = leaf;
  child_leaf_mask |= (1 << my_child_idx);
  // atomicOr(&child_leaf_mask, 1 << my_child_idx);
}

[[nodiscard]] inline bool IsLeaf(const int internal_value) noexcept {
  // check the most significant bit, which is used as "is leaf node?"
  return internal_value >> (sizeof(int) * 8 - 1);
}

[[nodiscard]] inline int GetLeafIndex(const int internal_value) noexcept {
  // delete the last bit which tells if this is leaf or internal index
  return internal_value & ~(1 << (sizeof(int) * 8 - 1));
}

inline void MakeNodesHelper(const int i,
                            OctNode *nodes,
                            const int *node_offsets,
                            const int *edge_count,
                            const CodeT *morton_keys,
                            const brt::InnerNode *inners,
                            const float min_coord,
                            const float tree_range,
                            const int root_level) {
  int oct_idx = node_offsets[i];
  const int n_new_nodes = edge_count[i];
  for (int j = 0; j < n_new_nodes - 1; ++j) {
    const int level = inners[i].delta_node / 3 - j;
    const CodeT node_prefix = morton_keys[i] >> (kCodeLen - (3 * level));
    const int child_idx = static_cast<int>(node_prefix & 0b111);
    const int parent = oct_idx + 1;

    nodes[parent].SetChild(oct_idx, child_idx);

    // calculate corner point (LSB have already been shifted off)
    // float dec_x, dec_y, dec_z;
    auto p =
        morton::single_code_to_point_v2(node_prefix << (kCodeLen - (3 * level)),
                                        //  dec_x,
                                        //  dec_y,
                                        //  dec_z,
                                        min_coord,
                                        tree_range);
    nodes[oct_idx].cornor = {p[0], p[1], p[2], 0.0f};

    // each cell is half the size of the level above it
    nodes[oct_idx].cell_size =
        tree_range / static_cast<float>(1 << (level - root_level));

    oct_idx = parent;
  }

  if (n_new_nodes > 0) {
    int rt_parent = inners[i].parent;
    while (edge_count[rt_parent] == 0) {
      rt_parent = inners[rt_parent].parent;
    }
    const int oct_parent = node_offsets[rt_parent];
    const int top_level = inners[i].delta_node / 3 - n_new_nodes + 1;
    const CodeT top_node_prefix =
        morton_keys[i] >> (kCodeLen - (3 * top_level));
    const int child_idx = static_cast<int>(top_node_prefix & 0b111);

    nodes[oct_parent].SetChild(oct_idx, child_idx);

    // float dec_x, dec_y, dec_z;
    auto p = morton::single_code_to_point_v2(
        top_node_prefix << (kCodeLen - (3 * top_level)),
        // dec_x,
        // dec_y,
        // dec_z,
        min_coord,
        tree_range);
    nodes[oct_idx].cornor = {p[0], p[1], p[2], 0.0f};
    // nodes[oct_idx].cornor = {dec_x, dec_y, dec_z, 0.0f};

    nodes[oct_idx].cell_size =
        tree_range / static_cast<float>(1 << (top_level - root_level));
  }
}

}  // namespace oct