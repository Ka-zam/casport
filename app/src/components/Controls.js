export class Controls {
    constructor(canvas, smithChart) {
        this.canvas = canvas;
        this.smithChart = smithChart;
        
        this.isPanning = false;
        this.lastMousePos = [0, 0];
        this.startPan = [0, 0];
        
        this.setupEventListeners();
    }

    setupEventListeners() {
        this.canvas.addEventListener('wheel', this.handleWheel.bind(this));
        this.canvas.addEventListener('mousedown', this.handleMouseDown.bind(this));
        this.canvas.addEventListener('mousemove', this.handleMouseMove.bind(this));
        window.addEventListener('mouseup', this.handleMouseUp.bind(this));
        this.canvas.addEventListener('dblclick', this.handleDoubleClick.bind(this));
    }

    handleWheel(e) {
        e.preventDefault();
        const delta = e.deltaY * -0.001;
        const uniforms = this.smithChart.getUniforms();
        const newZoom = Math.max(0.5, Math.min(10, uniforms.zoom * (1 + delta)));
        this.smithChart.setZoom(newZoom);
        this.onZoomChange?.(newZoom);
    }

    handleMouseDown(e) {
        if (e.button === 0) {
            this.isPanning = true;
            this.lastMousePos = [e.clientX, e.clientY];
            const uniforms = this.smithChart.getUniforms();
            this.startPan = [...uniforms.pan];
            this.canvas.style.cursor = 'grabbing';
        }
    }

    handleMouseMove(e) {
        if (this.isPanning) {
            const uniforms = this.smithChart.getUniforms();
            const dx = -(e.clientX - this.lastMousePos[0]) / this.canvas.width * 2;
            const dy = (e.clientY - this.lastMousePos[1]) / this.canvas.height * 2;
            const newPanX = this.startPan[0] + dx / uniforms.zoom;
            const newPanY = this.startPan[1] + dy / uniforms.zoom;
            this.smithChart.setPan(newPanX, newPanY);
        }
    }

    handleMouseUp() {
        this.isPanning = false;
        this.canvas.style.cursor = 'grab';
    }

    handleDoubleClick() {
        this.smithChart.resetView();
        this.onResetView?.();
    }

    setZoomChangeCallback(callback) {
        this.onZoomChange = callback;
    }

    setResetViewCallback(callback) {
        this.onResetView = callback;
    }
}