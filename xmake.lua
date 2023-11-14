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
    before_build(
        function(target)
            os.exec("./compile_shaders.sh")
        end
    )
end

after_build(
    function(target)
        platform = os.host()
        build_path = ""
        if is_mode("release") then
            build_path = "$(buildir)/" .. platform .. "/x86_64/release/"
        else
            build_path = "$(buildir)/" .. platform .. "/x86_64/debug/"
        end
        os.cp("shaders/compiled_shaders/**.spv", build_path)
        print("Copied compiled shaders to " .. build_path)
    end
)

target("app")
    set_default(true)
    set_kind("binary")
    add_files("src/*.cpp", "src/core/*.cpp")
    add_headerfiles("src/core/*.hpp")
    add_packages("vk-bootstrap", "vulkan-memory-allocator", "spirv-cross", "glm", "vulkansdk")
    add_packages("spdlog", "cli11")
