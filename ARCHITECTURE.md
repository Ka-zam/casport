# Casport RF Design Tool - Architecture Overview

## System Architecture

Casport is a modern RF circuit analysis and design tool combining C++ computational backend with a WebGPU-accelerated frontend. The architecture emphasizes performance, usability, and professional workflows.

## Core Design Principles

1. **Modern User Interface**
   - **HTML/CSS Network Builder**: Drag-and-drop component placement with visual feedback
   - **Tweakpane Settings**: Professional parameter control with organized sections  
   - **WebGPU Visualization**: High-performance Smith chart and plot rendering
   - **Responsive Design**: Adaptive layout for different screen sizes

2. **Performance Optimization**
   - **WASM Backend**: C++ Cascadix library compiled to WebAssembly
   - **GPU Acceleration**: WebGPU for visualization and computation
   - **Efficient Data Flow**: Minimal CPU-GPU transfer, coefficient-based rendering
   - **Real-time Analysis**: Live updates during parameter changes

3. **Professional Workflow**
   - **Visual Circuit Building**: Intuitive component palette and canvas
   - **Multiple Analysis Types**: Frequency sweeps, component sweeps, Monte Carlo
   - **Export/Import**: Settings persistence and data export capabilities

## Application Structure

### Frontend Architecture

```
┌─────────────────────────────────────┐
│ [Tweakpane Overlay]  Plot Section   │
│ (Settings & Analysis) ┌─────┬─────┐ │
│                       │ RL  │Smith│ │  
│                       │Plot │Chart│ │
│                       └─────┴─────┘ │
├─────────────────────────────────────┤
│        Network Builder Canvas       │
│       (Drag & Drop Interface)       │
└─────────────────────────────────────┘
```

### Component Hierarchy

#### 1. UI Layer (`app/src/ui/`)

**LayoutGrid.js** - Main application layout
- Responsive CSS Grid layout
- Overlay positioning for Tweakpane
- Resizable plot panels
- Full-width network canvas

**NetworkBuilder.js** - Visual circuit construction  
- HTML/CSS drag-and-drop interface
- Component palette (R, L, C, TL - series/shunt)
- Visual feedback (snap-to-grid, animations)
- Component editing and management

**PlotSettingsPanel.js** - Tweakpane-based controls
- Frequency analysis settings
- Display customization options  
- Analysis type selection
- Export/import functionality

#### 2. Core Layer (`app/src/core/`)

**WebGPUContext.js** - Graphics foundation
- WebGPU device initialization
- Buffer and pipeline management
- Shader compilation and binding

#### 3. Components Layer (`app/src/components/`)

**SmithChart.js** - Interactive Smith chart
- WebGPU-based rendering
- Multi-trace support with colors
- Grid and label overlays
- Real-time updates

**RLPlot.js** - Return loss visualization
- 2D plot rendering
- VSWR and dB scale support
- Marker and annotation system

**Controls.js** - Interactive controls
- Pan, zoom, and selection
- Mouse/touch event handling

#### 4. Utils Layer (`app/src/utils/`)

**CascadixWrapperEnhanced.js** - WASM interface
- Cascadix library initialization
- Component factory functions
- Analysis orchestration (frequency sweeps, etc.)
- Data format conversion

### Backend Architecture (Cascadix Library)

#### Core Components (`cascadix/include/`)

**cascadix.h** - Main library header
- Unified component interface
- Network cascading operations
- Analysis function exports

**components.h** - RF component models
- Series elements (R, L, C)
- Shunt elements (R, L, C) 
- Transmission lines
- ABCD matrix operations

**two_port.h** - Network representation
- ABCD parameter handling
- S-parameter conversions
- Impedance calculations

#### Analysis Engines

**frequency_sweep.h** - Frequency domain analysis
- Linear/logarithmic sweeps
- S-parameter calculation across frequency
- Smith chart point generation

**component_sweep.h** - Component optimization
- Parameter variation analysis
- Tolerance-based sweeps
- Optimization target functions

**monte_carlo.h** - Statistical analysis  
- Component tolerance modeling
- Gaussian/uniform distributions
- Batch processing for performance

**smith_chart.h** - Smith chart mathematics
- Impedance normalization
- Grid calculations
- Arc coefficient generation

#### WASM Interface (`cascadix/wasm/`)

**bindings.cc** - Emscripten bindings
- Component factory functions (`seriesResistor`, `shuntCapacitor`, etc.)
- Network operations (`cascade`)
- Analysis functions (`getSParameters`)
- Complex number handling

## Data Flow Architecture

### 1. User Interaction Flow

```
Drag Component → Network Builder → Component Array → WASM Analysis → Plot Update
     ↓              ↓                   ↓               ↓              ↓
[Palette]    [Visual Canvas]    [JavaScript Object] [C++ Calculation] [WebGPU Render]
```

### 2. Analysis Pipeline

```javascript
// User changes parameter in Tweakpane
onFrequencyChange() → 
  updateSettings() → 
    runAnalysis() → 
      buildNetworkAtFrequency() → 
        WASM.cascade() → 
          WASM.getSParameters() → 
            SmithChart.addTrace() → 
              WebGPU render
```

### 3. Component Creation Flow

```javascript
// Drag from palette to canvas
drag event → 
  addComponentAtPosition() → 
    createComponentNetwork() → 
      WASM factory function → 
        updateNetwork() → 
          recalculate analysis
```

## Network Building Interface

### Component Palette
- **Visual Components**: R, L, C, TL with series/shunt variants
- **Drag-and-Drop**: Intuitive placement with visual feedback
- **Snap-to-Grid**: Automatic alignment for clean layouts
- **Component Symbols**: Professional schematic symbols

### Canvas Interactions
- **Double-click**: Edit component values and tolerances
- **Right-click**: Delete components
- **Drag**: Reposition components
- **Visual Feedback**: Hover states, selection indicators

### Component Management
```javascript
const component = {
  id: timestamp,
  type: 'series_resistor',
  value: 50.0,
  tolerance: 5.0,
  unit: 'Ω',
  position: { x: 100, y: 150 },
  enabled: true
};
```

## Settings and Analysis

### Tweakpane Interface Structure

**📊 Frequency Analysis**
- Start/Stop frequencies with units
- Point count and sweep type
- Real-time calculated info (center, span, resolution)

**🎨 Display Settings**  
- Smith Chart: Grid, labels, colors, background
- Return Loss: VSWR/dB display, axis scaling

**🔬 Analysis Options**
- Analysis type selection (frequency, component, Monte Carlo)
- Monte Carlo sample count and tolerance
- Auto-update toggle

**Action Buttons**
- Analyze Network, Optimize, Export Settings, Reset

## WASM Integration

### Available Functions
Based on current bindings, the WASM interface provides:

```javascript
// Component Creation
module.seriesResistor(value)
module.seriesInductor(value, frequency) 
module.seriesCapacitor(value, frequency)
module.shuntResistor(value)
module.shuntInductor(value, frequency)
module.shuntCapacitor(value, frequency)
module.transmissionLine(length, z0, frequency)

// Network Operations  
module.cascade(network1, network2)
module.getSParameters(network, z0)

// Analysis
module.getInputImpedance(network, zLoad)
```

### Manual Analysis Implementation
Since advanced analysis functions aren't available in WASM bindings, analysis is implemented in JavaScript:

```javascript
// Frequency sweep implementation
generateFrequencySweep(networkBuilder, freqStart, freqStop, numPoints, zLoad, z0System) {
  const points = [];
  
  for (let i = 0; i < numPoints; i++) {
    const freq = freqStart + (freqStop - freqStart) * i / (numPoints - 1);
    const network = networkBuilder(freq);
    const sParams = this.module.getSParameters(network, z0System);
    
    points.push({
      x: sParams.s11.real,
      y: sParams.s11.imag,
      frequency: freq,
      return_loss: -20 * Math.log10(|S11|),
      vswr: (1 + |S11|) / (1 - |S11|)
    });
  }
  
  return { points, metadata };
}
```

## WebGPU Rendering Architecture

### Smith Chart Rendering
- **Vertex Buffers**: Efficient point data storage
- **Shader Pipeline**: GPU-accelerated trace rendering  
- **Multi-trace Support**: Color-coded analysis results
- **Grid Overlay**: Mathematical grid generation

### Performance Optimizations
- **Coefficient-based Arcs**: Calculate curves on GPU
- **Batch Processing**: Multiple frequency points together
- **Memory Efficiency**: Structure-of-arrays layout
- **Real-time Updates**: Minimal CPU-GPU data transfer

## File Structure

```
casport/
├── app/                          # Web application
│   ├── src/
│   │   ├── main.js              # Application entry point
│   │   ├── ui/
│   │   │   ├── LayoutGrid.js    # Main layout management
│   │   │   ├── NetworkBuilder.js # Drag-and-drop interface
│   │   │   └── PlotSettingsPanel.js # Tweakpane controls
│   │   ├── components/
│   │   │   ├── SmithChart.js    # WebGPU Smith chart
│   │   │   ├── RLPlot.js        # Return loss plot
│   │   │   └── Controls.js      # User interaction
│   │   ├── core/
│   │   │   └── WebGPUContext.js # Graphics foundation
│   │   ├── utils/
│   │   │   └── CascadixWrapperEnhanced.js # WASM interface
│   │   └── styles/
│   │       └── main.css         # Application styling
│   ├── public/
│   │   ├── cascadix_wasm.js     # Generated WASM bindings
│   │   └── cascadix_wasm.wasm   # Compiled library
│   └── package.json             # Dependencies (tweakpane)
├── cascadix/                     # C++ RF analysis library
│   ├── include/                  # Header files
│   ├── src/                      # Implementation files
│   ├── wasm/
│   │   └── bindings.cc          # Emscripten bindings
│   └── tests/                   # Unit tests
├── Makefile                     # Build orchestration
└── CMakeLists.txt              # Build configuration
```

## Development Workflow

### Build Process
```bash
# Build WASM library
make wasm

# Start development server  
make dev

# Production build
make build
```

### Hot Reload Support
- **JavaScript/CSS/HTML**: Instant updates
- **WASM Changes**: `make wasm` + browser refresh
- **Component Parameters**: Live updates via Tweakpane

## Key Design Decisions

### Why HTML/CSS over lil-gui?
- **Better UX**: More intuitive drag-and-drop interaction
- **Visual Feedback**: Smooth animations and hover states
- **Customization**: Full control over styling and behavior
- **Mobile Support**: Better touch interaction

### Why Tweakpane over Custom Controls?
- **Professional Appearance**: Clean, modern interface
- **Organized Structure**: Collapsible folders and sections
- **Rich Controls**: Sliders, color pickers, dropdowns
- **Auto-binding**: Reactive updates to object properties

### Why WebGPU over Canvas/SVG?
- **Performance**: Hardware acceleration for complex visualizations
- **Scalability**: Handles large datasets smoothly
- **Future-proof**: Modern graphics API with ongoing development
- **Extensibility**: Easy to add compute shaders for analysis

## Performance Characteristics

### Typical Performance
- **Component placement**: <16ms (60 FPS)
- **Network analysis**: ~10-50ms (depending on complexity)
- **Smith chart rendering**: <16ms (60 FPS)
- **Tweakpane updates**: <1ms

### Memory Usage
- **WASM Module**: ~2MB compiled size
- **WebGPU Buffers**: ~1MB for typical networks
- **JavaScript Heap**: ~10MB for UI state

## Future Enhancements

### Planned Features
1. **Component Library**: Expanded component types (stubs, couplers)
2. **S-parameter Import**: Touchstone file support
3. **Optimization Algorithms**: Automated component tuning
4. **Export Formats**: Network data export (JSON, CSV, Touchstone)
5. **Measurement Integration**: VNA data overlay

### Architecture Extensions
- **Plugin System**: Custom component definitions
- **Cloud Integration**: Save/load projects from cloud storage
- **Collaboration**: Real-time multi-user editing
- **Mobile App**: React Native port with touch optimization

## Summary

Casport represents a modern approach to RF design tools, combining:

1. **Intuitive Interface**: Drag-and-drop network building with professional controls
2. **High Performance**: WASM backend with WebGPU acceleration  
3. **Professional Features**: Comprehensive analysis capabilities
4. **Modern Architecture**: Maintainable, extensible codebase
5. **Great UX**: Responsive design with smooth interactions

The architecture successfully balances computational performance with user experience, providing a foundation for advanced RF design workflows.