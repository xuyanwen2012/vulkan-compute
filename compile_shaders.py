#!/usr/bin/env python

import os
import subprocess

input_folder = "shaders"
output_folder = "shaders/compiled_shaders"

if __name__ == "__main__":
    os.makedirs(output_folder, exist_ok=True)

    # Compile .cl shaders to .spv
    cl_shaders = [
        shader for shader in os.listdir(input_folder) if shader.endswith(".cl")
    ]
    for cl_shader in cl_shaders:
        input_path = os.path.join(input_folder, cl_shader)
        output_path = os.path.join(
            output_folder, os.path.splitext(cl_shader)[0] + ".spv"
        )

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

    # Compile .comp shaders to .spv
    comp_shaders = [
        shader for shader in os.listdir(input_folder) if shader.endswith(".comp")
    ]
    for comp_shader in comp_shaders:
        input_path = os.path.join(input_folder, comp_shader)
        output_path = os.path.join(
            output_folder, os.path.splitext(comp_shader)[0] + ".spv"
        )

        glslang_command = [
            "glslangValidator",
            "-V",
            "--target-env",
            "spirv1.3",
            input_path,
            "-o",
            output_path,
        ]

        subprocess.run(glslang_command)

    print("Shaders compiled successfully.")
