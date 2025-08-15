export class ControlPanel {
    constructor(container, smithChart, controls) {
        this.container = container;
        this.smithChart = smithChart;
        this.controls = controls;
        
        this.render();
        this.setupEventListeners();
    }

    render() {
        this.container.innerHTML = `
            <h3>📊 Smith Chart Controls</h3>
            <button id="reset-view">🔄 Reset View</button>
            <label>
                Grid Density: <span id="grid-value">1.0</span>
                <input type="range" id="grid-density" class="slider" min="0.5" max="2" step="0.1" value="1">
            </label>
            <label>
                Zoom: <span id="zoom-value">1.0</span>
                <input type="range" id="zoom-slider" class="slider" min="0.5" max="10" step="0.1" value="1">
            </label>
            
            <h3 style="margin-top: 2rem;">📝 Instructions</h3>
            <ul style="font-size: 0.8rem; line-height: 1.5;">
                <li>🖱️ Drag to pan</li>
                <li>⚙️ Scroll to zoom</li>
                <li>🔄 Double-click to reset</li>
            </ul>
        `;
    }

    setupEventListeners() {
        const resetBtn = this.container.querySelector('#reset-view');
        const gridSlider = this.container.querySelector('#grid-density');
        const gridValue = this.container.querySelector('#grid-value');
        const zoomSlider = this.container.querySelector('#zoom-slider');
        const zoomValue = this.container.querySelector('#zoom-value');

        resetBtn.addEventListener('click', () => {
            this.smithChart.resetView();
            zoomSlider.value = 1;
            zoomValue.textContent = '1.0';
        });

        gridSlider.addEventListener('input', (e) => {
            const density = parseFloat(e.target.value);
            this.smithChart.setGridDensity(density);
            gridValue.textContent = density.toFixed(1);
        });

        zoomSlider.addEventListener('input', (e) => {
            const zoom = parseFloat(e.target.value);
            this.smithChart.setZoom(zoom);
            zoomValue.textContent = zoom.toFixed(1);
        });

        this.controls.setZoomChangeCallback((zoom) => {
            zoomValue.textContent = zoom.toFixed(1);
            zoomSlider.value = zoom;
        });

        this.controls.setResetViewCallback(() => {
            zoomValue.textContent = '1.0';
            zoomSlider.value = 1;
        });
    }
}