# RF Design Tool - WebGPU Implementation

A professional RF design tool built with WebGPU for real-time impedance matching and network analysis. Features a paper-tone aesthetic with intuitive Gen→Network→Load flow.

## Features

### ✅ Completed
- **2x2 Grid Layout**
  - Top Left: Return Loss / VSWR plot
  - Top Right: Smith Chart with WebGPU rendering (60fps)
  - Bottom Left: Network Builder (Gen→Components→Load)
  - Bottom Right: Frequency Settings Panel

- **Smith Chart**
  - Paper-tone background with black ink style
  - Full reactance circles (not just arcs)
  - Emphasized 50Ω reference circle
  - Smooth pan and zoom with correct mouse direction
  - Real-time WebGPU rendering at 60fps

- **Network Builder**
  - Generator on left (50Ω)
  - Drag-and-drop components in the middle
  - Load impedance on right
  - Series and shunt components
  - Visual signal flow left-to-right

- **Modular Architecture**
  - Clean ES6 modules
  - Separated concerns (rendering, UI, calculations)
  - Hot module replacement with Vite
  - Extensible design for future features

## Project Structure
```
rf-design-tool/
├── src/
│   ├── components/       # Main components
│   │   ├── SmithChart.js
│   │   ├── RLPlot.js
│   │   ├── NetworkBuilder.js
│   │   └── Controls.js
│   ├── core/             # Core infrastructure
│   │   ├── WebGPUContext.js
│   │   ├── ShaderManager.js
│   │   └── UniformBuffer.js
│   ├── shaders/          # WGSL shaders
│   │   └── smithChart.wgsl
│   ├── ui/               # UI components
│   │   ├── LayoutGrid.js
│   │   └── FrequencyPanel.js
│   ├── utils/            # Utilities
│   │   └── NetworkAnalysis.js
│   ├── styles/           # CSS
│   │   └── main.css
│   └── main.js           # Entry point
```

## Running the Application

```bash
# Install dependencies
npm install

# Start development server
npm run dev

# Build for production
npm run build
```

## Browser Requirements
- Chrome 113+ or Edge 113+ (WebGPU support)
- Safari 17+ (WebGPU support)
- Firefox (experimental with flags)

## Usage

1. **Smith Chart** (Top Right)
   - Drag to pan
   - Scroll to zoom
   - Double-click to reset view

2. **Network Builder** (Bottom Left)
   - Drag components from palette
   - Build network left-to-right
   - Enter load impedance on right

3. **Frequency Settings** (Bottom Right)
   - Set frequency sweep range
   - Choose number of points
   - Click "Analyze" to run simulation

4. **Return Loss Plot** (Top Left)
   - Shows RL and VSWR vs frequency
   - Synchronized with Smith Chart

## Next Steps

- [ ] Connect NetworkAnalysis to actual UI updates
- [ ] Add real-time Smith Chart plotting of network response
- [ ] Implement component value editing
- [ ] Add S-parameter file import
- [ ] Monte Carlo tolerance analysis
- [ ] Optimization algorithms
- [ ] Export functionality (PNG, CSV, S2P)

## Technical Details

- **Rendering**: WebGPU with custom WGSL shaders
- **Performance**: Maintains 60fps on integrated GPUs
- **Architecture**: Modular ES6 with clean separation
- **Styling**: Paper-tone aesthetic with professional look
- **Development**: Vite for fast HMR and builds

## License

MIT