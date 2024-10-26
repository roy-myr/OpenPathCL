#!/bin/bash

# Output header file
OUTPUT_FILE="$1"

# Add header comment
echo "// Embedded image data" > "$OUTPUT_FILE"

# List of images
IMAGES=("favicon.png" "marker.svg" "polygon.svg" "rectangle.svg")

# Get the directory of the script
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# Loop through images and convert them
for img in "${IMAGES[@]}"; do
    imgname=$(basename "$img" .${img##*.})
    xxd -i -n "$imgname" "${SCRIPT_DIR}/templates/images/$img" >> "$OUTPUT_FILE"
    echo "" >> "$OUTPUT_FILE"
done
