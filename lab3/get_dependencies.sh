#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
THIRD_PARTY_DIR="$SCRIPT_DIR/third_party"

get_sources() {
    local url="$1"
    local dest="$2"
    local version="$3"

    if [ ! -d "$dest" ]; then
        echo "Cloning $url in $dest"
        git clone --branch "$version" --depth 1 "$url" "$dest"
        git -C "$dest" submodule update --init --recursive
    else
        echo "Already exists: $dest"
    fi
}

if [ -n "$WITH_TESTS" ]; then
    get_sources \
        "https://github.com/catchorg/Catch2.git" \
        "$THIRD_PARTY_DIR/Catch2" \
        "v3.15.1"
fi