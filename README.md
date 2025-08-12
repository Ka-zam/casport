# Cascadix

⚡ A blazing-fast C++ library for analyzing cascaded 2-port networks using ABCD (chain) matrices. Supports both native compilation and WebAssembly for browser-based RF/microwave circuit analysis.

> **Note**: GitHub repository is currently named `casport` but the library is `cascadix`

## Features

- **ABCD Matrix Operations**: Full support for chain matrix multiplication and cascading
- **Component Library**: Series/shunt RLC components, transmission lines, transformers
- **Parameter Conversions**: S-parameters, Z-parameters, Y-parameters, H-parameters
- **Operator Overloading**: Intuitive `*` operator for cascading networks
- **Complex Arithmetic**: Full complex number support for frequency-domain analysis
- **WebAssembly Support**: Run in browsers via Emscripten compilation
- **Type-Safe**: Modern C++ with `std::complex<double>` throughout

## Quick Start

```cpp
#include "cascadix.h"
using namespace cascadix;

// Create a 50Ω transmission line (90° at 1 GHz)
double freq = 1e9;  // 1 GHz
double z0 = 50.0;
double length = 0.075;  // λ/4 at 1 GHz
auto tline = transmission_line(length, z0, freq);

// Add series inductor (10 nH)
auto inductor = series_inductor(10e-9, freq);

// Cascade them
auto network = tline * inductor;

// Calculate S-parameters
auto s_params = network.to_s_parameters(50.0);
std::cout << "S11: " << s_params.s11 << std::endl;
std::cout << "S21: " << s_params.s21 << std::endl;

// Get input impedance with 50Ω load
auto z_in = network.input_impedance(50.0);
std::cout << "Input impedance: " << z_in << " Ω" << std::endl;
```

## Building

### Native Build

```bash
mkdir build && cd build
cmake ..
make
```

### WebAssembly Build

```bash
mkdir build-wasm && cd build-wasm
emcmake cmake ..
emmake make
# Open wasm/cascadix_wasm.js in browser
```

### Running Tests

```bash
cd build
make test
```

## API Overview

### Core Classes

- `two_port`: Base class representing a 2-port network with ABCD matrix
- `series_impedance`: General series element with impedance Z
- `shunt_admittance`: General shunt element with admittance Y
- `transmission_line`: Coaxial/microstrip transmission line segment
- `ideal_transformer`: Ideal transformer with turns ratio

### Frequency-Dependent Components

- `series_resistor(r, freq)`: Series resistance
- `series_inductor(l, freq)`: Series inductance with jωL impedance
- `series_capacitor(c, freq)`: Series capacitance with 1/(jωC) impedance
- `shunt_resistor(r, freq)`: Shunt resistance
- `shunt_inductor(l, freq)`: Shunt inductance
- `shunt_capacitor(c, freq)`: Shunt capacitance

### Operations

- **Cascading**: `network1 * network2` - Matrix multiplication of ABCD matrices
- **Parallel**: `network1 || network2` - Parallel combination (future)
- **Analysis**: `input_impedance()`, `output_impedance()`, `to_s_parameters()`

## Mathematical Background

The library implements ABCD matrix (chain matrix) analysis for 2-port networks:

```
[V1]   [A  B] [V2]
[I1] = [C  D] [-I2]
```

For cascaded networks: `[ABCD]_total = [ABCD]_1 × [ABCD]_2`

See [src/FORMULAS.md](src/FORMULAS.md) for complete mathematical documentation.

## Performance

Cascadix is optimized for speed:
- **Native**: Cascade 1000 components in <1ms
- **WebAssembly**: Near-native performance in browsers
- **Small footprint**: ~43KB WASM binary

Benchmark results (typical performance):
- Cascade 1000 resistors: ~200 μs
- Cascade 1000 inductors: ~800 μs  
- 10,000 S-parameter calculations: ~500 μs

## Examples

### LC Filter Design

```cpp
// Design a 3rd-order Butterworth lowpass filter
// Cutoff at 1 GHz, 50Ω system
double fc = 1e9;
double z0 = 50.0;

// Normalized Butterworth values: L1=L3=0.7654, C2=1.8478
auto l1 = series_inductor(6.1e-9, fc);     // 0.7654 * z0/(2π*fc)
auto c2 = shunt_capacitor(58.8e-12, fc);   // 1.8478 / (z0*2π*fc)
auto l3 = series_inductor(6.1e-9, fc);

auto filter = l1 * c2 * l3;
```

### Matching Network

```cpp
// Match 100Ω to 50Ω at 2.4 GHz
double freq = 2.4e9;

// L-network: series L, shunt C
auto l_match = series_inductor(3.3e-9, freq);
auto c_match = shunt_capacitor(1.3e-12, freq);

auto matching_network = l_match * c_match;
auto z_in = matching_network.input_impedance(100.0);
// z_in should be close to 50Ω
```

### Transmission Line Stub

```cpp
// Quarter-wave open stub for filtering
double freq = 5e9;
double z0_stub = 75.0;
double length = 0.015;  // λ/4 at 5 GHz

auto stub = transmission_line(length, z0_stub, freq);
auto open_circuit = shunt_admittance(0.0);  // Y = 0 for open
auto stub_network = stub * open_circuit;

// Transforms to short circuit at input
```

## Contributing

Contributions are welcome! Please ensure:
- All tests pass
- Code follows existing style (snake_case)
- Mathematical accuracy is maintained
- Documentation is updated

## License

MIT License - see LICENSE file for details

## Why Cascadix?

The name combines "cascade" (the fundamental operation for 2-port networks) with the "-ix" suffix suggesting matrix operations, making it memorable and searchable.

## References

- Pozar, D.M., "Microwave Engineering", 4th Edition
- Gonzalez, G., "Microwave Transistor Amplifiers", 2nd Edition
- [ABCD-parameters on Wikipedia](https://en.wikipedia.org/wiki/Two-port_network#ABCD-parameters)
