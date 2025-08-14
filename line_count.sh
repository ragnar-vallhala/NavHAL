#!/bin/bash

# Find all source files, count LOC, and index them sorted by LOC

echo "---------------------------------"
echo "Counting lines of code for NAVHAL"
echo "---------------------------------"

# Directly count number of files
total_files=$(find . \
  -type f \
  \( -name "*.c" -o -name "*.h" -o -name "*.cpp" -o -name "*.hpp"  -o -name "*.txt"\
     -o -name "*.s" -o -name "*.S" -o -name "*.py" -o -name "*.sh" \) \
  ! -path "./build/*" \
  ! -path "./.git/*" \
  ! -path "./extern/*" \
  | wc -l)


# Number and sort by LOC
index=1
find . \
  -type f \
  \( -name "*.c" -o -name "*.h" -o -name "*.cpp" -o -name "*.hpp" -o -name "*.txt" \
     -o -name "*.s" -o -name "*.S" -o -name "*.py" -o -name "*.sh" \) \
  ! -path "./build/*" \
  ! -path "./.git/*" \
  ! -path "./extern/*" \
  -print0 |
  xargs -0 wc -l |
  sort -n |
  while read -r line; do
    printf "%3d. %s\n" "$index" "$line"
    index=$((index + 1))
  done

echo "Total files: $total_files"
echo "---------------------------------"
