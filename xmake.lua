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

if is_plat("windows") then
elseif is_plat("linux") then
    before_build(function(target)
        os.exec("python3 compile_shaders.py")
        -- os.exec("./compile_shaders.sh")
    end)
end

after_build(function(target)
    platform = os.host()
    arch = os.arch()
    build_path = ""
    if is_mode("release") then
        build_path = "$(buildir)/" .. platform .. "/" .. arch .. "/release/"
    else
        build_path = "$(buildir)/" .. platform .. "/" .. arch .. "/debug/"
    end
    os.cp("shaders/compiled_shaders/**.spv", build_path)
    print("Copied compiled shaders to " .. build_path)
end)

target("app")
set_default(true)
set_kind("binary")
add_includedirs("include")
add_headerfiles("include/*.hpp", "include/**/*.hpp")
add_files("examples/main.cpp", "src/**/*.cpp")
add_packages("vk-bootstrap", "vulkan-memory-allocator", "spirv-cross", "glm",
             "vulkansdk", "spdlog", "cli11")

target("brt")
set_kind("binary")
add_includedirs("include")
add_files("examples/02_brt.cpp", "src/**/*.cpp")
add_headerfiles("examples/*.hpp", "include/**/*.hpp")
add_packages("vk-bootstrap", "vulkan-memory-allocator", "spirv-cross", "glm",
             "vulkansdk", "spdlog")

