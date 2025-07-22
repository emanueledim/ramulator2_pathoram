#!/bin/bash

# Ramulator2 and PathORAM repo
REPO_URL="https://github.com/CMU-SAFARI/ramulator2"
REPO_PATCH_URL="https://github.com/emanueledim/ramulator2_pathoram.git"
DEST_DIR="ramulator2"

# Check git
if ! command -v git &> /dev/null; then
    echo "Git not found. Installation..."
    sudo apt update && sudo apt install -y git
fi

# Clone repository
if [ ! -d "$DEST_DIR" ]; then
    echo "Cloning ramulator2 repository..."
    git clone "$REPO_URL" "$DEST_DIR"
else
    echo "Ramulator2 repository already installed."
fi

# Patch folder
SRC="pathoram"

if [ ! -d "$SRC" ]; then
  echo "PathORAM patch not found."
  exit 1;
fi

# Copy patch files into gem5
cp -r "$SRC"/. "$DEST_DIR"

echo "Installation and patch applied."
