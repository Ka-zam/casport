export class WebGPUContext {
    constructor(canvas) {
        this.canvas = canvas;
        this.device = null;
        this.context = null;
        this.format = null;
        this.adapterInfo = null;
    }

    async initialize() {
        if (!navigator.gpu) {
            throw new Error('WebGPU not supported in this browser');
        }

        const adapter = await navigator.gpu.requestAdapter();
        if (!adapter) {
            throw new Error('No GPU adapter found');
        }

        this.device = await adapter.requestDevice();
        this.adapterInfo = adapter.info || {};
        
        this.context = this.canvas.getContext('webgpu');
        this.format = navigator.gpu.getPreferredCanvasFormat();
        
        this.configureCanvas();
        
        this.device.lost.then((info) => {
            console.error(`WebGPU device was lost: ${info.message}`);
            if (info.reason !== 'destroyed') {
                this.initialize();
            }
        });

        return {
            device: this.device,
            context: this.context,
            format: this.format,
            adapterInfo: this.adapterInfo
        };
    }

    configureCanvas() {
        const devicePixelRatio = window.devicePixelRatio || 1;
        this.canvas.width = this.canvas.clientWidth * devicePixelRatio;
        this.canvas.height = this.canvas.clientHeight * devicePixelRatio;
        
        this.context.configure({
            device: this.device,
            format: this.format,
            alphaMode: 'premultiplied'
        });
    }

    resize() {
        this.configureCanvas();
        return {
            width: this.canvas.width,
            height: this.canvas.height
        };
    }

    getCurrentTexture() {
        return this.context.getCurrentTexture();
    }

    getDevice() {
        return this.device;
    }

    getFormat() {
        return this.format;
    }

    getResolution() {
        return [this.canvas.width, this.canvas.height];
    }

    getAdapterInfo() {
        return this.adapterInfo;
    }
}