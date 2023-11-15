#pragma once

#include <glm/glm.hpp>

#include "brt.hpp"

namespace oct {

inline void CalculateEdgeCountHelper(const int i,
                                     int* edge_count,
                                     const brt::InnerNode* inners) {
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

  glm::vec3 cornor;
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
  void SetChild(const int child, const int my_child_idx);

  /**
   * @brief Set the Leaf object
   *
   * @param leaf: index of point that will become the leaf child
   * @param my_child_idx: which of my children it will be [0-7]
   */
  void SetLeaf(const int leaf, const int my_child_idx);
};

inline void oct::OctNode::SetChild(const int child, const int my_child_idx) {
  // children_array[my_child_idx] = child;
  children[my_child_idx] = child;
  child_node_mask |= (1 << my_child_idx);
  //   atomicOr(&child_node_mask, 1 << my_child_idx);
}

inline void oct::OctNode::SetLeaf(const int leaf, const int my_child_idx) {
  children[my_child_idx] = leaf;
  child_leaf_mask |= (1 << my_child_idx);
  // atomicOr(&child_leaf_mask, 1 << my_child_idx);
}

}  // namespace oct