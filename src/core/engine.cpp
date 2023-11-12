#include "engine.hpp"

namespace core
{
	void ComputeEngine::destroy()
	{
		if (manage_resources_ && !algorithms_.empty()) {
			spdlog::debug("ComputeEngine::destroy() explicitly freeing algorithms");
			for (auto &weak_algorithm : algorithms_) {
				if (const auto algorithm = weak_algorithm.lock()) {
					algorithm->destroy();
				}
			}
			algorithms_.clear();
		}

		if (manage_resources_ && !buffers_.empty()) {
			spdlog::debug("ComputeEngine::destroy() explicitly freeing buffers");
			for (auto &weak_buffer : buffers_) {
				if (const auto buffer = weak_buffer.lock()) {
					buffer->destroy();
				}
			}
			buffers_.clear();
		}

		if (manage_resources_ && !sequence_.empty()) {
			spdlog::debug("ComputeEngine::destroy() explicitly freeing sequences");
			for (auto &weak_sequence : sequence_) {
				if (const auto sequence = weak_sequence.lock()) {
					sequence->destroy();
				}
			}
			sequence_.clear();
		}
	}
}
