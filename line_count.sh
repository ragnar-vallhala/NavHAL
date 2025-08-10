#!/bin/bash

# Count lines of code in the project and sort by line count (ascending)
# Ignores build artifacts and hidden folders

find . \
  -type f \
  \( -name "*.c" -o -name "*.h" -o -name "*.cpp" -o -name "*.hpp" \
     -o -name "*.s" -o -name "*.S" -o -name "*.py" -o -name "*.sh" \) \
  ! -path "./build/*" \
  ! -path "./.git/*" \
  -print0 | xargs -0 wc -l | sort -n
