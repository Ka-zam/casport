#!/bin/bash

# Simple web server script for Cascadix WebAssembly demo
# Starts a local HTTP server in the build-wasm/wasm directory

PORT=${1:-8000}
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/../build-wasm/wasm"

echo "🚀 Starting Cascadix WebAssembly demo server..."

# Check if build directory exists
if [ ! -d "$BUILD_DIR" ]; then
    echo "❌ Build directory not found: $BUILD_DIR"
    echo "   Please build the WebAssembly target first:"
    echo "   mkdir build-wasm"
    echo "   cd build-wasm"
    echo "   emcmake cmake .. -DBUILD_WASM=ON"
    echo "   emmake make"
    exit 1
fi

# Check if WASM files exist
if [ ! -f "$BUILD_DIR/cascadix_wasm.js" ] || [ ! -f "$BUILD_DIR/cascadix_wasm.wasm" ]; then
    echo "❌ WebAssembly files not found in: $BUILD_DIR"
    echo "   Please build the WebAssembly target first:"
    echo "   cd build-wasm && emmake make"
    exit 1
fi

echo "📁 Serving directory: $BUILD_DIR"
echo "🌐 Server URL: http://localhost:$PORT/"
echo "⏹️  Press Ctrl+C to stop"
echo

cd "$BUILD_DIR" && python3 -m http.server "$PORT"