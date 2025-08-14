# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Cascadix is a C++ library for RF circuit analysis using ABCD (chain) matrices. It supports both native compilation and WebAssembly for browser-based circuit analysis. The library focuses on 2-port network cascading, S-parameter calculations, and Smith chart visualization.

## Build Commands

### Native Build
```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### WebAssembly Build
```bash
mkdir build-wasm && cd build-wasm
emcmake cmake .. -DBUILD_WASM=ON
emmake make
```

### Run Tests
```bash
cd build
ctest --output-on-failure
# Or run individual tests:
./test_components
./test_cascading
./test_conversions
./test_frequency_sweep
./test_smith_chart
```

### Run All Tests Together
```bash
cd build
make run_tests
```

### Build and Run Examples
```bash
cd build
make example_filter
./apps/example_filter
```

## Architecture

The codebase follows a layered architecture optimized for both computational efficiency and WebGPU-based visualization:

### Core Components (`include/` and `src/`)
- **two_port.h/cc**: Base class for all 2-port networks with ABCD matrix operations, includes S↔ABCD conversions
- **components.h/cc**: RF component implementations (RLC elements, transmission lines, transformers, stubs)
- **frequency_sweep.h**: Frequency-domain analysis with linear/logarithmic sweeps
- **component_sweep.h**: Component value sweeping for optimization
- **smith_chart.h**: Smith chart calculations and arc coefficient generation for GPU rendering
- **monte_carlo.h**: Statistical analysis with component tolerances

### Stub Components
The library includes four types of transmission line stubs:
- **series_open_stub**: Open-ended transmission line in series (transforms to short at λ/4)
- **series_short_stub**: Short-ended transmission line in series (transforms to open at λ/4)
- **shunt_open_stub**: Open-ended transmission line to ground (high admittance at λ/4)
- **shunt_short_stub**: Short-ended transmission line to ground (low admittance at λ/4)

### Key Design Patterns
1. **Operator Overloading**: Networks cascade using `*` operator (ABCD matrix multiplication)
2. **Complex Arithmetic**: All calculations use `std::complex<double>` for accuracy
3. **GPU Optimization**: Arc coefficients computed in C++, curves rendered on GPU
4. **Batch Processing**: Monte Carlo and frequency sweeps optimized for parallel execution

### WebAssembly Integration (`wasm/`)
- **bindings.cc**: Emscripten bindings exposing C++ API to JavaScript
- **demo.html**: Complete web interface demonstrating all features
- Data passed as coefficients to minimize CPU-GPU transfer

## Testing Strategy

The project uses GoogleTest (automatically fetched by CMake). Tests cover:
- Component ABCD matrix calculations
- Network cascading operations
- Parameter conversions (S, Z, Y, H parameters)
- Frequency sweep functionality
- Smith chart computations

When adding new features, create corresponding tests in `tests/` following the existing pattern.

## Code Style

- **Naming**: snake_case for functions/variables, PascalCase for classes
- **Headers**: Use include guards, not #pragma once
- **Complex Numbers**: Always use std::complex<double>
- **Documentation**: Mathematical formulas documented in src/FORMULAS.md