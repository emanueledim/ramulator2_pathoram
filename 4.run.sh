#!/bin/bash

DEST="ramulator2"
FILENAME=""

usage() {
    echo "Usage: $0 -c FILENAME"
    echo "FILENAME: the name of the workload file to execute (.yaml)"
    exit 1
}

# Parse options
while [[ "$#" -gt 0 ]]; do
    case "$1" in
        -c)
            FILENAME="$2"
            shift 2
            ;;
        *)
            echo "Unknown option: $1"
            usage
            ;;
    esac
done

# Check if FILENAME is set
if [ -z "$FILENAME" ]; then
    echo "Error: FILENAME not provided."
    usage
fi

# Check if directory exists
if [ ! -d "$DEST" ]; then
    echo "Error: Directory '$DEST' not found!"
    exit 1
fi

# Run the program
cd "$DEST"
./ramulator2 -f "./$FILENAME"