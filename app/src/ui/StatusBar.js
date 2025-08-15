export class StatusBar {
    constructor(container) {
        this.container = container;
        this.fpsElement = null;
        this.gpuInfoElement = null;
        
        this.render();
    }

    render() {
        this.container.innerHTML = `
            <h1>🎯 RF Design Tool - Smith Chart</h1>
            <div class="status">
                <span id="fps-counter">FPS: 0</span>
                <span id="gpu-info">Initializing...</span>
            </div>
        `;
        
        this.fpsElement = this.container.querySelector('#fps-counter');
        this.gpuInfoElement = this.container.querySelector('#gpu-info');
    }

    updateFPS(fps) {
        if (this.fpsElement) {
            this.fpsElement.textContent = `FPS: ${fps}`;
        }
    }

    updateGPUInfo(info) {
        if (this.gpuInfoElement) {
            const description = info.description || 'GPU Ready';
            this.gpuInfoElement.textContent = `✅ ${description}`;
        }
    }

    setError(message) {
        if (this.gpuInfoElement) {
            this.gpuInfoElement.textContent = `❌ ${message}`;
        }
    }
}