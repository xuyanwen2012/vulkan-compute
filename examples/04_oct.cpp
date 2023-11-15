#include <bitset>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <vector>

#include "baseline/brt.hpp"
#include "baseline/morton.hpp"
#include "baseline/oct.hpp"
#include "common.hpp"
#include "core/engine.hpp"
#include "helpers.hpp"

// write a ostream function
std::ostream& operator<<(std::ostream& os, const oct::OctNode& node) {
  os << "Body Mass: " << std::setw(10) << node.body.mass << "\n";
  os << "Corner: " << std::setw(10) << node.cornor.x << " " << std::setw(10)
     << node.cornor.y << " " << std::setw(10) << node.cornor.z << " "
     << std::setw(10) << node.cornor.w << "\n";
  os << "Cell Size: " << std::setw(10) << node.cell_size << "\n";
  os << "Children: ";
  for (int i = 0; i < 8; ++i) {
    os << std::setw(3) << node.children[i] << " ";
  }
  os << "\n";
  os << "Child Node Mask: " << std::bitset<8>(node.child_node_mask) << "\n";
  os << "Child Leaf Mask: " << std::bitset<8>(node.child_leaf_mask) << "\n";
  return os;
}

int main(int argc, char** argv) {
  // example params
  constexpr auto n = 1024;
  constexpr auto num_brt_nodes = n - 1;
  constexpr auto min_coord = 0.0f;
  constexpr auto range = 1024.0f;

  // auto code =
  //     morton::single_point_to_code(1.0f, 3.0f, 100.0f, min_coord, range);
  // std::cout << "Code: " << code << "\n";

  // float dec_x, dec_y, dec_z;
  // morton::single_code_to_point(code, dec_x, dec_y, dec_z, min_coord, range);
  // std::cout << "Point: " << dec_x << " " << dec_y << " " << dec_z << "\n";
  // {
  //   auto code = morton::encode(1, 3, 100);
  //   std::cout << "Code: " << code << "\n";

  //   uint32_t dec_x, dec_y, dec_z;
  //   morton::decode(code, dec_x, dec_y, dec_z);
  //   std::cout << "Point: " << dec_x << " " << dec_y << " " << dec_z << "\n";
  // }

  // {
  //   auto code =
  //       morton::single_point_to_code_v2(1.0f, 3.0f, 100.0f, min_coord,
  //       range);
  //   std::cout << "Code: " << code << "\n";

  //   float x = 1.0f;
  //   float y = 3.0f;
  //   float z = 100.0f;
  //   x = (x - min_coord) * 1024.0f / range;
  //   y = (y - min_coord) * 1024.0f / range;
  //   z = (z - min_coord) * 1024.0f / range;
  //   std::cout << "Point(range): " << x << " " << y << " " << z << "\n";
  //   std::cout << "Point(range, uint): " << (uint32_t)x << " " << (uint32_t)y
  //             << " " << (uint32_t)z << "\n";

  //   auto p = morton::single_code_to_point_v2(code, min_coord, range);
  //   std::cout << "Point: " << p[0] << " " << p[1] << " " << p[2] << "\n";
  //   exit(0);
  // }

  setup_log_level("debug");
  spdlog::info("Current working directory: {}",
               std::filesystem::current_path().string());

  core::ComputeEngine engine{};

  auto u_mortons = load_pod_data_from_file<uint32_t>("sorted_mortons_1024.bin");
  auto u_brt_nodes =
      load_pod_data_from_file<brt::InnerNode>("brt_nodes_1024.bin");

  assert(u_mortons.size() == n);
  assert(u_brt_nodes.size() == num_brt_nodes);

  std::vector<int32_t> cpu_edge_counts(n);
  std::vector<int32_t> cpu_oc_offset(n);

  cpu_edge_counts[0] = 1u;
  for (int i = 0; i < num_brt_nodes; ++i) {
    oct::CalculateEdgeCountHelper(
        i, cpu_edge_counts.data(), u_brt_nodes.data());
  }

  std::partial_sum(cpu_edge_counts.begin(),
                   cpu_edge_counts.end(),
                   cpu_oc_offset.begin() + 1);
  cpu_oc_offset[0] = 0;

  // ---------------------------------------------------------------------------
  //                          Edge Count
  // ---------------------------------------------------------------------------

  // const auto brt_nodes_buf = engine.buffer(n * sizeof(brt::InnerNode));
  // const auto edge_count_buf = engine.buffer(n * sizeof(int32_t));

  // auto brt_node_ptr = brt_nodes_buf->get_data_mut<brt::InnerNode>();
  // auto edge_count_ptr = edge_count_buf->get_data_mut<int32_t>();

  // std::ranges::copy(u_brt_nodes, brt_node_ptr);
  // edge_count_buf->tmp_fill_zero(n * sizeof(int32_t));

  // std::vector params{brt_nodes_buf, edge_count_buf};
  // auto algo = engine.algorithm("edge_count.spv",
  //                              params,
  //                              256,  // num_threads
  //                              true,
  //                              make_clspv_push_const(n));

  // const auto seq = engine.sequence();

  // seq->simple_record_commands(*algo, n);
  // seq->launch_kernel_async();

  // // ... do something else

  // seq->sync();

  // // Verify edge count results
  // for (int i = 0; i < num_brt_nodes; ++i) {
  //   if (cpu_edge_counts[i] != edge_count_ptr[i]) {
  //     spdlog::error("Mismatch at {} : {} vs {}",
  //                   i,
  //                   cpu_edge_counts[i],
  //                   edge_count_ptr[i]);
  //   }
  // }

  // ---------------------------------------------------------------------------
  //                          Build Octree
  // ---------------------------------------------------------------------------

  // build cpu octree for verification

  const auto num_oc_nodes = cpu_oc_offset.back();
  spdlog::info("Number of octree: {}", num_oc_nodes);

  std::vector<oct::OctNode> cpu_oct_nodes(num_oc_nodes);

  int root_level = u_brt_nodes[0].delta_node / 3;
  Code_t root_prefix = u_mortons[0] >> (kCodeLen - (root_level * 3));

  auto p = morton::single_code_to_point_v2(
      root_prefix << (kCodeLen - (root_level * 3)), min_coord, range);
  cpu_oct_nodes[0].cornor = {p[0], p[1], p[2], 0.0f};
  cpu_oct_nodes[0].cell_size = range;

  const auto oc_nodes_buff = engine.buffer(n * sizeof(oct::OctNode));

  // skip 1
  for (auto i = 1; i < num_oc_nodes; ++i) {
    oct::MakeNodesHelper(i,
                         cpu_oct_nodes.data(),
                         cpu_oc_offset.data(),
                         cpu_edge_counts.data(),
                         u_mortons.data(),
                         u_brt_nodes.data(),
                         min_coord,
                         range,
                         root_level);
  }

  // Print out the first 10 nodes
  for (auto i = 0; i < 10; ++i) {
    std::cout << "Node " << i << " :\n" << cpu_oct_nodes[i] << "\n";
  }

  spdlog::info("Done");
  return EXIT_SUCCESS;
}
