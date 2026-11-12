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

if [ -n "${WITH_TESTS:-}" ]; then
    get_sources \
        "https://github.com/catchorg/Catch2.git" \
        "$THIRD_PARTY_DIR/Catch2" \
        "v3.15.1"
fi

if [ "${WITH_LAB7_DEPS:-0}" = "1" ]; then
    PREFIX="${LAB7_DEPS_PREFIX:-$THIRD_PARTY_DIR/install}"
    mkdir -p "$PREFIX" "$THIRD_PARTY_DIR/build"
    PREFIX="$(cd "$PREFIX" && pwd)"

    get_sources "https://github.com/zeromq/libzmq.git" "$THIRD_PARTY_DIR/libzmq" "v4.3.5"
    get_sources "https://github.com/zeromq/cppzmq.git" "$THIRD_PARTY_DIR/cppzmq" "v4.10.0"
    get_sources "https://github.com/jbeder/yaml-cpp.git" "$THIRD_PARTY_DIR/yaml-cpp" "0.8.0"
    get_sources "https://github.com/nlohmann/json.git" "$THIRD_PARTY_DIR/json" "v3.11.3"

    build_dependency() {
        local name="$1"
        shift
        cmake -S "$THIRD_PARTY_DIR/$name" -B "$THIRD_PARTY_DIR/build/$name" \
            -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$PREFIX" \
            -DCMAKE_PREFIX_PATH="$PREFIX" "$@"
        cmake --build "$THIRD_PARTY_DIR/build/$name" --parallel
        cmake --install "$THIRD_PARTY_DIR/build/$name"
    }

    build_dependency libzmq -DBUILD_TESTS=OFF -DWITH_DOCS=OFF -DWITH_LIBSODIUM=OFF
    build_dependency cppzmq -DCPPZMQ_BUILD_TESTS=OFF
    build_dependency yaml-cpp -DYAML_CPP_BUILD_TESTS=OFF -DYAML_CPP_BUILD_TOOLS=OFF
    build_dependency json -DJSON_BuildTests=OFF
    
    echo "Зависимости установлены в $PREFIX"
fi
