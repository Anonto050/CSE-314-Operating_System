#!/bin/bash

# Create the output directory if it doesn't exist
mkdir -p output_dir

# Define the input folder
folder="input_dir"

# Loop through each file in the input directory recursively
find "$folder" -type f -name "*.*" | while read file; do

    # Get the base name of the file without the extension
    name=$(basename "$file" | cut -d'.' -f1)

    # Get the length of the name
    number=${#name}

    # Create the subdirectory in the output directory based on the length of the name
    mkdir -p "output_dir/$number"

    # Copy the file to the appropriate subdirectory
    cp "$file" "output_dir/$number/"

done

# Loop through each subdirectory in the output directory
for dir in output_dir/*; do
    if [ -d "$dir" ]; then
        # Change to the subdirectory
        cd "$dir" || exit

        # Get a list of files sorted by size
        files=($(ls -S))

        # Rename the files based on their sorted order
        for i in "${!files[@]}"; do
            original_name="${files[$i]}"
            extension="${original_name##*.}"
            mv "$original_name" "${i}_its_${original_name%.*}.${extension}"
        done

        # Change back to the previous directory
        cd - > /dev/null || exit
    fi
done
