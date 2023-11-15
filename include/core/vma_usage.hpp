#pragma once

#include "vk_mem_alloc.h"

namespace core {

/**
 * @brief Global VMA allocator. Only 1 instance of VmaAllocator will be created
 * for this project.
 */
extern VmaAllocator g_allocator;

}  // namespace core