# Casport RF Design Tool - Unified Build System
# Builds Cascadix C++ library, WASM module, and web application

.PHONY: all dev build clean wasm wasm-clean install help test native cascadix-clean

# Default target
all: wasm dev

# Help target
help:
	@echo "Casport RF Design Tool - Build System"
	@echo "===================================="
	@echo ""
	@echo "Targets:"
	@echo "  all       - Build WASM and start dev server"
	@echo "  native    - Build native Cascadix library and tests"
	@echo "  wasm      - Build Cascadix WASM library"
	@echo "  dev       - Start Vite development server"
	@echo "  build     - Production build"
	@echo "  test      - Run C++ unit tests"
	@echo "  clean     - Clean all build artifacts"
	@echo "  install   - Install npm dependencies"
	@echo "  help      - Show this help"
	@echo ""
	@echo "Development workflow:"
	@echo "  1. make install   (first time only)"
	@echo "  2. make all       (builds WASM + starts dev server)"
	@echo "  3. Open http://localhost:5173"

# Install web app dependencies
install:
	@echo "Installing npm dependencies..."
	cd app && npm install

# Build native Cascadix library and run tests
native:
	@echo "Building native Cascadix library..."
	@mkdir -p build
	@cd build && cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON
	@cd build && make -j4
	@echo "Native build complete!"

# Run tests
test: native
	@echo "Running Cascadix tests..."
	@cd build && make test
	@echo "Tests complete!"

# Build WASM library
wasm:
	@echo "Building Cascadix WASM library..."
	@if ! command -v emcc >/dev/null 2>&1; then \
		echo "Error: Emscripten not found. Please install Emscripten first:"; \
		echo "  git clone https://github.com/emscripten-core/emsdk.git"; \
		echo "  cd emsdk && ./emsdk install latest && ./emsdk activate latest"; \
		echo "  source ./emsdk_env.sh"; \
		exit 1; \
	fi
	@mkdir -p build-wasm
	@cd build-wasm && \
		emcmake cmake .. \
			-DCMAKE_BUILD_TYPE=Release \
			-DBUILD_WASM=ON \
			-DBUILD_TESTS=OFF \
			-DBUILD_EXAMPLES=OFF \
			-DBUILD_SHARED_LIBS=OFF
	@cd build-wasm && emmake make -j4
	@echo "Copying WASM files to web app..."
	@mkdir -p app/public
	@cp build-wasm/wasm/cascadix_wasm.js app/public/
	@cp build-wasm/wasm/cascadix_wasm.wasm app/public/
	@echo "WASM build complete!"

# Start development server
dev:
	@echo "Starting Vite development server..."
	@echo "The app will be available at http://localhost:5173"
	@echo ""
	@echo "Hot reload is enabled for:"
	@echo "  - JavaScript/TypeScript files"
	@echo "  - CSS files"  
	@echo "  - HTML files"
	@echo ""
	@echo "Note: WASM changes require 'make wasm' and browser refresh"
	@echo ""
	cd app && npm run dev

# Production build
build: wasm
	@echo "Building for production..."
	cd app && npm run build
	@echo "Production build complete! Files are in app/dist/"

# Preview production build
preview: build
	@echo "Starting preview server..."
	cd app && npm run preview

# Clean all build artifacts
clean: cascadix-clean wasm-clean
	@echo "Cleaning web app build artifacts..."
	@rm -rf app/dist
	@rm -rf app/node_modules/.vite
	@echo "Clean complete!"

# Clean native build
cascadix-clean:
	@echo "Cleaning native build artifacts..."
	@rm -rf build
	@echo "Native clean complete!"

# Clean WASM build
wasm-clean:
	@echo "Cleaning WASM build artifacts..."
	@rm -rf build-wasm
	@rm -f app/public/cascadix_wasm.js
	@rm -f app/public/cascadix_wasm.wasm
	@echo "WASM clean complete!"

# Watch for WASM changes and rebuild automatically
watch-wasm:
	@echo "Watching for Cascadix source changes..."
	@echo "This will automatically rebuild WASM when C++ files change"
	@echo "Press Ctrl+C to stop"
	@while inotifywait -r -e modify,create,delete cascadix/src cascadix/include 2>/dev/null; do \
		echo "Source changed, rebuilding WASM..."; \
		make wasm; \
		echo "WASM rebuilt! Refresh your browser to see changes."; \
	done

# Development with WASM watching (requires inotify-tools)
dev-watch: wasm
	@echo "Starting development with WASM auto-rebuild..."
	@if ! command -v inotifywait >/dev/null 2>&1; then \
		echo "Warning: inotifywait not found. Install inotify-tools for auto WASM rebuild"; \
		echo "  sudo apt install inotify-tools  # On Ubuntu/Debian"; \
	fi
	@make watch-wasm &
	@make dev

# Quick test of WASM build
test-wasm: wasm
	@echo "Testing WASM module..."
	@cd app && node -e "
	const Module = require('./public/cascadix_wasm.js');
	Module().then(m => {
		console.log('✓ WASM module loaded successfully');
		console.log('Available functions:', Object.keys(m).filter(k => typeof m[k] === 'function').slice(0, 10));
	}).catch(e => {
		console.error('✗ WASM module failed to load:', e.message);
		process.exit(1);
	});"

# Check if Emscripten is available
check-emscripten:
	@if command -v emcc >/dev/null 2>&1; then \
		echo "✓ Emscripten found: $$(emcc --version | head -1)"; \
	else \
		echo "✗ Emscripten not found"; \
		echo "Please install Emscripten:"; \
		echo "  git clone https://github.com/emscripten-core/emsdk.git"; \
		echo "  cd emsdk && ./emsdk install latest && ./emsdk activate latest"; \
		echo "  source ./emsdk_env.sh"; \
		exit 1; \
	fi

# Show build status
status:
	@echo "Casport RF Design Tool - Build Status"
	@echo "===================================="
	@echo ""
	@echo "Emscripten:"
	@if command -v emcc >/dev/null 2>&1; then \
		echo "  ✓ Available: $$(emcc --version | head -1)"; \
	else \
		echo "  ✗ Not found"; \
	fi
	@echo ""
	@echo "Node.js:"
	@if command -v node >/dev/null 2>&1; then \
		echo "  ✓ Available: $$(node --version)"; \
	else \
		echo "  ✗ Not found"; \
	fi
	@echo ""
	@echo "WASM Files:"
	@if [ -f app/public/cascadix_wasm.js ]; then \
		echo "  ✓ cascadix_wasm.js ($$(du -h app/public/cascadix_wasm.js | cut -f1))"; \
	else \
		echo "  ✗ cascadix_wasm.js not found"; \
	fi
	@if [ -f app/public/cascadix_wasm.wasm ]; then \
		echo "  ✓ cascadix_wasm.wasm ($$(du -h app/public/cascadix_wasm.wasm | cut -f1))"; \
	else \
		echo "  ✗ cascadix_wasm.wasm not found"; \
	fi
	@echo ""
	@echo "Dependencies:"
	@if [ -d app/node_modules ]; then \
		echo "  ✓ npm packages installed"; \
	else \
		echo "  ✗ npm packages not installed (run 'make install')"; \
	fi
	@echo ""
	@echo "Native Library:"
	@if [ -f build/libcascadix.so ]; then \
		echo "  ✓ Native library built"; \
	else \
		echo "  ✗ Native library not built (run 'make native')"; \
	fi

# Setup development environment
setup:
	@echo "Setting up development environment..."
	@echo "1. Installing npm dependencies..."
	@make install
	@echo "2. Building native library..."
	@make native
	@echo "3. Running tests..."
	@make test
	@echo ""
	@echo "✓ Setup complete!"
	@echo ""
	@echo "Next steps:"
	@echo "  - Install Emscripten if needed: ./setup-emscripten.sh"
	@echo "  - Start development: make all"