#!/bin/bash

shader_dir="shaders"

if [ ! -d "$shader_dir" ]; then
    echo "Error: Directory '$shader_dir' not found."
    exit 1
fi

for cl_file in "$shader_dir"/*.cl; do
    if [ -f "$cl_file" ]; then
        file_name=$(basename "$cl_file" .cl)

        spv_output="$shader_dir/${file_name}.spv"

        # Run clspv command for the current file
        clspv --spv-version=1.5 --cl-std=CLC++ -inline-entry-points "$cl_file" -o "$spv_output"

        if [ $? -eq 0 ]; then
            echo "Converted $cl_file to $spv_output"
        else
            echo "Error converting $cl_file to $spv_output"
        fi
    fi
done
