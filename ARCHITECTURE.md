# Cascadix Architecture for Smith Chart Applications

## Overview

Cascadix is designed as a high-performance RF circuit analysis library optimized for WebGPU-based Smith chart visualization. The architecture separates computational concerns (handled by WASM) from rendering concerns (handled by WebGPU shaders).

## Core Design Principles

1. **Separation of Concerns**
   - **Cascadix/WASM**: RF calculations, network analysis, coefficient generation
   - **WebGPU**: Visualization, animation, user interaction
   - **Minimal Data Transfer**: Only coefficients, not point arrays

2. **Performance Optimization**
   - Batch processing for Monte Carlo simulations
   - GPU-friendly data layouts
   - Coefficient-based arc generation (compute on GPU)

3. **Flexibility**
   - Support for complex characteristic impedances
   - Multiple sweep types (frequency, component value)
   - Extensible component models with tolerances

## Architecture Layers

### 1. Component Layer (`components.h`)
Base classes and implementations for RF components:
- Series elements (R, L, C)
- Shunt elements (R, L, C)
- Transmission lines (with complex Z₀ support)
- Composite networks

### 2. Analysis Layer

#### Frequency Sweep (`frequency_sweep.h`)
- Fixed components, varying frequency
- Linear and logarithmic sweeps
- Batch S-parameter calculation

#### Component Sweep (`component_sweep.h`)
- Fixed frequency, varying component values
- Arc generation for Smith chart
- Start/stop value calculation for visualization

#### Monte Carlo (`monte_carlo.h`)
- Component tolerance modeling
- Statistical distributions (Gaussian, uniform, etc.)
- Batch processing for GPU efficiency

### 3. Smith Chart Layer (`smith_chart.h`)

#### Arc Coefficient Generation
Each component type produces specific arc patterns on the Smith chart:

**Series Components:**
- Constant resistance circles
- Varying reactance creates arc

**Shunt Components:**
- Constant conductance circles
- Varying susceptance creates arc

**Transmission Lines:**
- Rotation around specific center point
- Center = Z₀_line / Z₀_system (not always origin!)
- Complex Z₀ creates spirals (lossy lines)

#### Mathematical Coefficients for GPU

```cpp
struct ArcCoefficients {
    // Arc type identifier
    enum Type { SERIES_L, SERIES_C, SHUNT_L, SHUNT_C, TLINE };
    Type type;
    
    // Mathematical parameters (GPU will evaluate)
    float coeffs[8];  // Interpretation depends on type
    
    // For transmission lines
    vec2 center;      // Center of rotation (Smith chart coords)
    vec2 z0_norm;     // Normalized characteristic impedance
};
```

### 4. WebGPU Interface Layer

#### Data Transfer Structure
```cpp
struct SmithChartData {
    // Component arcs (coefficients only)
    ArcCoefficients arcs[MAX_ARCS];
    
    // Monte Carlo samples (batch processing)
    float component_values[MAX_SAMPLES * MAX_COMPONENTS];
    float probabilities[MAX_SAMPLES];
    
    // System parameters
    float z0_system;
    float frequency;
};
```

#### Shader Responsibilities
- **Vertex Shader**: Generate arc vertices from coefficients
- **Fragment Shader**: Apply aging, colors, styles
- **Compute Shader**: Batch impedance calculations

## Data Flow

### 1. User Input → Cascadix
```
User selects component → Calculate nominal value → Determine sweep range
```

### 2. Cascadix → GPU Coefficients
```
Component parameters → Arc coefficients → GPU uniform buffer
```

### 3. GPU Shader Processing
```
Coefficients → Vertex generation → Smith chart transformation → Rendering
```

### 4. Monte Carlo Flow
```
Tolerance specs → Generate samples → Batch calculate → GPU visualization
```

## Component Arc Behaviors

### Series Inductor
- **Arc Type**: Clockwise from load toward open circuit
- **Equation**: Z = R + jωL (L varies)
- **Smith Chart**: Constant resistance circle

### Series Capacitor
- **Arc Type**: Counter-clockwise from load toward short circuit
- **Equation**: Z = R - j/(ωC) (C varies)
- **Smith Chart**: Constant resistance circle

### Shunt Inductor
- **Arc Type**: Along constant conductance circle
- **Equation**: Y = G - j/(ωL) (L varies)
- **Smith Chart**: Constant conductance circle

### Shunt Capacitor
- **Arc Type**: Along constant conductance circle
- **Equation**: Y = G + jωC (C varies)
- **Smith Chart**: Constant conductance circle

### Transmission Line
- **Arc Type**: Rotation around Z₀_line/Z₀_system point
- **Center**: (Z₀_line/Z₀_system - 1) / (Z₀_line/Z₀_system + 1)
- **Special Cases**:
  - Z₀_line = Z₀_system: Rotates around origin
  - Z₀_line ≠ Z₀_system: Offset center
  - Complex Z₀: Creates spiral (lossy line)

## Transmission Line Architecture

### Complex Characteristic Impedance
```cpp
class transmission_line {
    complex z0;  // Can be complex for lossy lines
    double alpha;  // Attenuation constant (Np/m)
    double beta;   // Phase constant (rad/m)
    
    complex calculate_input_impedance(complex z_load, double length);
    ArcCoefficients get_arc_coefficients(double z0_system);
};
```

### Arc Center Calculation
The center of a transmission line arc depends on the impedance mismatch:

```cpp
complex get_arc_center(complex z0_line, double z0_system) {
    complex z_norm = z0_line / z0_system;
    complex gamma = (z_norm - 1.0) / (z_norm + 1.0);
    return gamma;  // Smith chart coordinates
}
```

## Monte Carlo Architecture

### Component Tolerance Model
```cpp
struct ComponentWithTolerance {
    double nominal;
    double tolerance;  // Percentage
    DistributionType distribution;
    double temperature_coefficient;
};
```

### Batch Processing
```cpp
class MonteCarloAnalyzer {
    // Generate samples in batch
    void generate_samples(int num_samples);
    
    // Calculate all networks in parallel
    void batch_calculate(NetworkBuilder builder);
    
    // GPU-friendly output
    float* get_impedance_buffer();  // Flattened for GPU
    float* get_probability_buffer();
};
```

## WebGPU Integration

### Uniform Buffer (Frequently Updated)
```javascript
const uniforms = {
    time: currentTime,  // For arc aging
    z0_system: 50.0,
    frequency: 2.4e9,
    viewMatrix: mat4x4
};
```

### Storage Buffer (Large Data)
```javascript
const storage = {
    arcCoefficients: new Float32Array(maxArcs * 8),
    monteCarloSamples: new Float32Array(numSamples * 2),
    probabilities: new Float32Array(numSamples)
};
```

### Compute Shader Pipeline
```wgsl
@compute @workgroup_size(256)
fn calculate_impedances(
    @builtin(global_invocation_id) id: vec3<u32>,
    component_values: array<f32>,
    output_impedances: array<vec2<f32>>
) {
    let values = load_component_values(id.x);
    let network = build_network(values);
    let z = calculate_impedance(network);
    output_impedances[id.x] = normalize_impedance(z);
}
```

## Performance Considerations

### Optimization Strategies
1. **Coefficient-based rendering**: Compute arcs on GPU, not CPU
2. **Batch operations**: Process multiple frequencies/values together
3. **Memory layout**: Structure-of-arrays for GPU efficiency
4. **Lazy evaluation**: Only compute what's visible

### Benchmarks
- 30 components × 201 frequency points: ~22ms
- 10,000 Monte Carlo samples: ~5ms
- Arc coefficient generation: <0.1ms per component

## Future Extensions

### Planned Features
1. **S-parameter file import** (Touchstone format)
2. **Noise analysis** coefficients
3. **Temperature variation** modeling
4. **Coupled line** support
5. **Active device** models

### API Evolution
The architecture supports extension through:
- New component types via inheritance
- Custom distribution functions
- Additional sweep dimensions (temperature, bias, etc.)
- Plugin system for user-defined components

## Implementation Files

### Core Library
- `include/component_sweep.h` - Component value sweeping
- `include/smith_chart.h` - Smith chart calculations
- `include/monte_carlo.h` - Statistical analysis
- `src/component_sweep.cc` - Implementation
- `src/smith_chart.cc` - Arc coefficient generation
- `src/monte_carlo.cc` - Batch processing

### WASM Bindings
- Extended bindings for all new features
- Efficient typed array interfaces
- Streaming calculation support

## Summary

This architecture enables:
1. **Efficient Smith chart visualization** via coefficient-based GPU rendering
2. **Real-time Monte Carlo** analysis with thousands of samples
3. **Accurate transmission line** modeling with complex Z₀
4. **Minimal CPU-GPU data transfer** for maximum performance
5. **Clear separation** between RF calculations and visualization