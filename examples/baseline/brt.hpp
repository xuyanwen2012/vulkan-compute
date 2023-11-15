#pragma once

#include <cstdint>
#include <iomanip>
#include <ostream>

#include "morton.hpp"

namespace brt {

struct InnerNode {
  int32_t delta_node;
  int32_t left = -1;
  int32_t right = -1;
  int32_t parent = -1;
};

inline std::ostream& operator<<(std::ostream& os, const InnerNode& node) {
  os << std::left << std::setw(10) << "\tDelta: " << std::right << std::setw(10)
     << node.delta_node << "\n";
  os << std::left << std::setw(10) << "\tLeft: " << std::right << std::setw(10)
     << node.left << "\n";
  os << std::left << std::setw(10) << "\tRight: " << std::right << std::setw(10)
     << node.right << "\n";
  os << std::left << std::setw(10) << "\tParent: " << std::right
     << std::setw(10) << node.parent << "\n";
  return os;
}

constexpr int sign(const int val) { return (0 < val) - (val < 0); }

constexpr int divide2_ceil(const int val) { return (val + 1) >> 1; }

constexpr int make_leaf(const int index) {
  return index ^ ((-1 ^ index) & 1u << (sizeof(CodeT) * 8 - 1));
}

constexpr int make_internal(const int index) { return index; }

inline int delta(const CodeT* morton_keys, const int i, const int j) {
  constexpr auto unused_bits = 1;
  const auto li = morton_keys[i];
  const auto lj = morton_keys[j];
  return __builtin_clz(li ^ lj) - unused_bits;
}

inline int delta_safe(const CodeT* morton_keys,
                      const int key_num,
                      const int i,
                      const int j) {
  return (j < 0 || j >= key_num) ? -1 : delta(morton_keys, i, j);
}

constexpr int min(const int a, const int b) { return a < b ? a : b; }
constexpr int max(const int a, const int b) { return a > b ? a : b; }

inline void process_inner_node_helper(const int num_keys,
                                      const CodeT* morton_keys,
                                      const int i,
                                      InnerNode* brt_nodes) {
  const int direction = sign(delta(morton_keys, i, i + 1) -
                             delta_safe(morton_keys, num_keys, i, i - 1));

  const int delta_min = delta_safe(morton_keys, num_keys, i, i - direction);

  int I_max = 2;
  while (delta_safe(morton_keys, num_keys, i, i + I_max * direction) >
         delta_min) {
    I_max <<= 2;
  }

  // Find the other end using binary search.
  int I = 0;
  for (int t = I_max / 2; t; t /= 2) {
    if (delta_safe(morton_keys, num_keys, i, i + (I + t) * direction) >
        delta_min) {
      I += t;
    }
  }

  const int j = i + I * direction;

  // Find the split position using binary search.
  const int delta_node = delta_safe(morton_keys, num_keys, i, j);
  int s = 0;
  int t = I;

  do {
    t = divide2_ceil(t);
    if (delta_safe(morton_keys, num_keys, i, i + (s + t) * direction) >
        delta_node) {
      s += t;
    }
  } while (t > 1);

  const int split = i + s * direction + min(direction, 0);

  const int left =
      min(i, j) == split ? make_leaf(min(i, j)) : make_internal(split);
  const int right =
      max(i, j) == split + 1 ? make_leaf(split + 1) : make_internal(split + 1);

  brt_nodes[i].delta_node = delta_node;
  brt_nodes[i].left = left;
  brt_nodes[i].right = right;

  if (min(i, j) != split) {
    brt_nodes[left].parent = i;
  }
  if (max(i, j) != split + 1) {
    brt_nodes[right].parent = i;
  }
}

};  // namespace brt