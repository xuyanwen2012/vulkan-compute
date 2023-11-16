set_project("Vulkan Compute")

-- Vulkan related
add_requires("vk-bootstrap", "vulkan-memory-allocator", "spirv-cross", "glm")
add_requires("vulkansdk", {system = true})

-- Others
add_requires("spdlog", "cli11")

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

-- if is_plat("linux") then
--     before_build(function(target)
--         os.exec("python3 compile_shaders.py")
--         -- os.exec("./compile_shaders.sh")
--     end)
-- end

-- after_build(function(target)
--     platform = os.host()
--     arch = os.arch()
--     build_path = ""
--     if is_mode("release") then
--         build_path = "$(buildir)/" .. platform .. "/" .. arch .. "/release/"
--     else
--         build_path = "$(buildir)/" .. platform .. "/" .. arch .. "/debug/"
--     end
--     os.cp("shaders/compiled_shaders/**.spv", build_path)
--     print("Copied compiled shaders to " .. build_path)
-- end)

function template()
    set_kind("binary")
    add_includedirs("include")
    add_headerfiles("examples/*.hpp", "examples/baseline/*.hpp")
    add_deps("lib")
    add_packages("vk-bootstrap", "vulkan-memory-allocator", "spirv-cross",
                 "glm", "vulkansdk", "spdlog")
    -- if is_plat("linux") then
    --     before_build(function(target)
    --         os.exec("python3 compile_shaders.py")
    --         -- os.exec("./compile_shaders.sh")
    --     end)
    -- end

    -- after_build(function(target)
    --     platform = os.host()
    --     arch = os.arch()
    --     build_path = ""
    --     if is_mode("release") then
    --         build_path = "$(buildir)/" .. platform .. "/" .. arch .. "/release/"
    --     else
    --         build_path = "$(buildir)/" .. platform .. "/" .. arch .. "/debug/"
    --     end
    --     os.cp("shaders/compiled_shaders/**.spv", build_path)
    --     print("Copied compiled shaders to " .. build_path)
    -- end)
end

target("lib")
set_default(true)
set_kind("static")
add_includedirs("include")
add_headerfiles("include/*.hpp", "include/**/*.hpp")
add_files("src/**/*.cpp")
add_packages("vk-bootstrap", "vulkan-memory-allocator", "spirv-cross", "glm",
             "vulkansdk", "spdlog")

for _, file in ipairs(os.files("examples/*.cpp")) do
    local name = path.basename(file)

    -- remove the number prefix (e.g., 01_XXXX.cpp) before in the name
    local target_name = name:gsub("%d%d_", "")

    target(target_name)
    template()
    add_files("examples/" .. name .. ".cpp")
end
