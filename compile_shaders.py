#!/usr/bin/env python

import os
import subprocess

input_folder = "shaders"
output_folder = "shaders/compiled_shaders"


def compile_with_clspv(cl_shader: str):
    """
    Compiles a given OpenCL shader to SPIR-V using clspv.

    Args:
        cl_shader (str): The filename of the OpenCL shader to compile.

    Returns:
        None
    """
    input_path = os.path.join(input_folder, cl_shader)
    output_path = os.path.join(output_folder, os.path.splitext(cl_shader)[0] + ".spv")

    clspv_command = [
        "clspv",
        "-w",
        "-O0",
        "--spv-version=1.3",
        "--cl-std=CL2.0",
        "-inline-entry-points",
        input_path,
        "-o",
        output_path,
    ]
    print(" ".join(clspv_command))
    subprocess.run(clspv_command)


def compile_with_glslang(gl_shader: str):
    """
    Compiles a GLSL shader to SPIR-V using the glslangValidator tool.

    Args:
        gl_shader (str): The filename of the GLSL shader to compile.

    Returns:
        None
    """
    input_path = os.path.join(input_folder, gl_shader)
    output_path = os.path.join(output_folder, os.path.splitext(gl_shader)[0] + ".spv")

    glslang_command = [
        "glslangValidator",
        "-V",
        "--target-env",
        "spirv1.5",
        input_path,
        "-o",
        output_path,
    ]
    print(" ".join(glslang_command))
    subprocess.run(glslang_command)


if __name__ == "__main__":
    os.makedirs(output_folder, exist_ok=True)

    for shader in os.listdir(input_folder):
        if shader.endswith(".comp"):
            compile_with_glslang(shader)
        elif shader.endswith(".cl"):
            compile_with_clspv(shader)
        else:
            continue

    # Copy the compiled shaders to the binary folder.
    # if on linux, copy to the linux binary folder
    if os.name == "posix":
        os.system("cp shaders/compiled_shaders/* build/linux/x86_64/debug/")
    elif os.name == "nt":
        # os.system("copy shaders/compiled_shaders/* build/windows/x86_64/debug/")
        pass
    else:
        print("OS not supported")

    print("Shaders compiled successfully.")
