#!/bin/bash

if [ $# -ne 3 ]; then
  echo "Usage: $0 input_dir virus_list.txt output_dir"
  exit 1
fi

input_dir=$1
virus_list=$2
output_dir=$3

# Create the output directory if it doesn't exist
mkdir -p "$output_dir"

# Read the virus list into an array
mapfile -t viruses < "$virus_list"

# Function to replace virus names with ***
replace_virus_name() {
  local filename=$1
  for virus in "${viruses[@]}"; do
    filename=${filename//$virus/***}
  done
  echo "$filename"
}

# Find and copy files
find "$input_dir" -type f | while read -r file; do
  base_name=$(basename "$file")
  for virus in "${viruses[@]}"; do
    if [[ $base_name == *"$virus"* ]]; then
      new_name=$(replace_virus_name "$base_name")
      cp "$file" "$output_dir/$new_name"
      break
    fi
  done
done

# Rename files in the output directory based on modification time
cd "$output_dir" || exit
files=($(ls -t))
for i in "${!files[@]}"; do
  original_name="${files[$i]}"
  extension="${original_name##*.}"
  mv "$original_name" "${i}_its_${original_name%.*}.${extension}"
done
