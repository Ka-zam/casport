# Cascadix WebAssembly

This directory contains the WebAssembly bindings and web interface for the Cascadix RF circuit analysis library.

## Files

- `bindings.cc` - Emscripten bindings that expose the C++ API to JavaScript
- `cascadix_demo.html` - Complete web interface demonstrating all library features
- `test_wasm.js` - Node.js test script for WASM functionality

## Building

To build the WebAssembly version:

```bash
mkdir build-wasm
cd build-wasm
emcmake cmake .. -DBUILD_WASM=ON
emmake make
```

This will generate:
- `cascadix_wasm.js` - JavaScript wrapper
- `cascadix_wasm.wasm` - WebAssembly binary

## Usage

### In Browser

1. Build the WASM files as described above
2. Copy `cascadix_demo.html` to the build output directory
3. Serve the directory with a web server (required for WASM):
   ```bash
   cd build-wasm/wasm
   python3 -m http.server 8000
   ```
4. Open http://localhost:8000/cascadix_demo.html

### In Node.js

```javascript
const CascadixModule = require('./cascadix_wasm.js');
const fs = require('fs');

async function example() {
    const wasmBinary = fs.readFileSync('./cascadix_wasm.wasm');
    const Module = await CascadixModule({ wasmBinary });
    
    // Create components
    const r1 = Module.seriesResistor(50.0);
    const l1 = Module.seriesInductor(10e-9, 1e9);
    
    // Cascade networks
    const network = Module.cascade(r1, l1);
    
    // Calculate S-parameters
    const s = Module.getSParameters(network, 50.0);
    console.log('S21:', s.s21);
}
```

## Features

The web interface includes:

1. **Circuit Design Tab**
   - LC Filter Designer (Butterworth)
   - Transmission Line Calculator
   - Quarter-wave transformer design

2. **Network Analysis Tab**
   - Custom network builder with drag-and-drop components
   - Attenuator designer (Pi and Tee topologies)
   - Full ABCD and S-parameter analysis

3. **Frequency Sweep Tab**
   - Linear and logarithmic frequency sweeps
   - Support for various network types
   - Export sweep data to CSV

4. **Performance Tab**
   - Component cascading benchmarks
   - Frequency sweep performance testing
   - S-parameter calculation speed tests

5. **Test Suite Tab**
   - Automated testing of core functionality
   - Validation of mathematical correctness

## API Reference

### Components
- `seriesResistor(r)` - Series resistor
- `seriesInductor(l, freq)` - Series inductor
- `seriesCapacitor(c, freq)` - Series capacitor
- `shuntResistor(r)` - Shunt resistor
- `shuntInductor(l, freq)` - Shunt inductor
- `shuntCapacitor(c, freq)` - Shunt capacitor
- `transmissionLine(length, z0, freq)` - Transmission line

### Networks
- `cascade(network1, network2)` - Cascade two networks
- `butterworthLC3(fc, z0)` - 3rd order Butterworth filter
- `piAttenuator(atten_db, z0)` - Pi attenuator
- `tAttenuator(atten_db, z0)` - Tee attenuator

### Analysis
- `getSParameters(network, z0)` - Calculate S-parameters
- `getInputImpedance(network, z_load)` - Calculate input impedance
- `getA/B/C/D(network)` - Get ABCD matrix elements

### Frequency Sweeps
- `FrequencySweep(start, stop, points, type)` - Create frequency sweep
- `getFrequencies()` - Get frequency points
- Types: `LINEAR`, `LOG`

## Performance

WebAssembly provides near-native performance:
- 30-component network with 201-point frequency sweep: ~22ms
- ~9000 frequency points/second for complex networks
- S-parameter calculations: ~0.07 μs per calculation