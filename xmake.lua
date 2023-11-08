set_project("Vulkan Compute")

add_requires("vk-bootstrap", "vulkan-memory-allocator", "spirv-cross")
add_requires("glm", "vulkansdk", {system = true})

add_rules("mode.debug", "mode.release")

set_languages("c++20")
set_warnings("all")

if is_mode("debug") then
    set_symbols("debug")
    set_optimize("none")
end

if is_mode("release") then
    set_symbols("hidden")
    set_optimize("fastest")
end

target("vulkan-compute")
set_default(true)
set_kind("binary")
add_files("src/*.cpp", "src/core/*.cpp")
add_packages("vk-bootstrap", "vulkan-memory-allocator", "spirv-cross", "glm",
             "vulkansdk")
