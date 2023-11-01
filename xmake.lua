add_rules("mode.debug", "mode.release")
set_languages("c++20")
set_encodings("utf-8")
add_requires("vulkansdk", "glfw", "glm", "tinyobjloader")
add_requires("imgui", {configs = {glfw_vulkan = true}})
add_requires("spdlog", {configs = {std_format = true}})

if is_mode("debug") then
    add_defines("SATURN_DEBUG")
elseif is_mode("release") then
    add_defines("SATURN_RELEASE")
end

target("SaturnEngine")
    set_kind("binary")
    add_files("engine/src/**.cpp")
    set_pcxxheader("engine/src/engine_pch.hpp")
    
    if is_plat("windows") then
        add_defines("ENGINE_ROOT_DIR=\"" .. (os.projectdir():gsub("\\", "\\\\")) .. "\\\\engine\"")
    else 
        add_defines("ENGINE_ROOT_DIR=\"" .. (os.projectdir():gsub("\\", "/")) .. "/engine\"")
    end

    add_packages("vulkansdk", "glfw", "glm", "tinyobjloader", "imgui", "spdlog")
    add_includedirs("deps", "engine/src/")

    before_run(function (target)
        print("[shader] glsl to spirv..")
        local vulkan_sdk = find_package("vulkansdk")
        local glslang_validator_dir = vulkan_sdk["bindir"].."\\glslangValidator.exe"
        for _, shader_path in ipairs(os.files("$(projectdir)/engine/shaders/**|*.spv")) do
            os.runv(glslang_validator_dir,{"-V", shader_path,"-o", shader_path..".spv"})
            print("[shader] done: "..shader_path)
        end
    end)
