import { ShaderManager } from '../core/ShaderManager.js';
import { UniformBuffer } from '../core/UniformBuffer.js';

export class SmithChart {
    constructor(webGPUContext) {
        this.context = webGPUContext;
        this.device = webGPUContext.getDevice();
        this.shaderManager = new ShaderManager(this.device);
        this.uniformBuffer = new UniformBuffer(this.device);
        
        this.pipeline = null;
        this.bindGroup = null;
        
        this.frameCount = 0;
        this.lastFpsUpdate = performance.now();
        this.fpsCallback = null;
    }

    async initialize() {
        const shaderModule = await this.shaderManager.loadAndCreateModule(
            'Smith Chart Shader',
            '/src/shaders/smithChart.wgsl'
        );
        
        this.pipeline = this.shaderManager.createRenderPipeline({
            label: 'Smith Chart Pipeline',
            shaderModule,
            format: this.context.getFormat()
        });
        
        this.bindGroup = this.device.createBindGroup({
            label: 'Bind Group',
            layout: this.pipeline.getBindGroupLayout(0),
            entries: [{
                binding: 0,
                resource: { buffer: this.uniformBuffer.getBuffer() }
            }]
        });
        
        const [width, height] = this.context.getResolution();
        this.uniformBuffer.setResolution(width, height);
    }

    render(timestamp) {
        this.uniformBuffer.setTime(timestamp * 0.001);
        
        const commandEncoder = this.device.createCommandEncoder();
        const textureView = this.context.getCurrentTexture().createView();
        
        const renderPass = commandEncoder.beginRenderPass({
            colorAttachments: [{
                view: textureView,
                clearValue: { r: 0.05, g: 0.05, b: 0.08, a: 1.0 },
                loadOp: 'clear',
                storeOp: 'store'
            }]
        });
        
        renderPass.setPipeline(this.pipeline);
        renderPass.setBindGroup(0, this.bindGroup);
        renderPass.draw(6);
        renderPass.end();
        
        this.device.queue.submit([commandEncoder.finish()]);
        
        this.updateFPS();
    }

    updateFPS() {
        this.frameCount++;
        const now = performance.now();
        if (now - this.lastFpsUpdate >= 1000) {
            const fps = this.frameCount;
            if (this.fpsCallback) {
                this.fpsCallback(fps);
            }
            this.frameCount = 0;
            this.lastFpsUpdate = now;
        }
    }

    setFPSCallback(callback) {
        this.fpsCallback = callback;
    }

    resize() {
        const { width, height } = this.context.resize();
        this.uniformBuffer.setResolution(width, height);
    }

    setZoom(zoom) {
        this.uniformBuffer.setZoom(zoom);
    }

    setPan(x, y) {
        this.uniformBuffer.setPan(x, y);
    }

    setGridDensity(density) {
        this.uniformBuffer.setGridDensity(density);
    }

    getUniforms() {
        return this.uniformBuffer.getUniforms();
    }

    resetView() {
        this.uniformBuffer.setZoom(1.0);
        this.uniformBuffer.setPan(0.0, 0.0);
    }

    setImpedancePoints(impedances, z0 = 50) {
        // Convert impedances to Smith chart coordinates
        const points = impedances.map((z, index) => {
            // Calculate reflection coefficient
            const zNorm = { real: z.real / z0, imag: z.imag / z0 };
            const gamma = this.impedanceToGamma(zNorm);
            
            // Convert to Smith chart coordinates
            return {
                x: gamma.real,
                y: gamma.imag,
                size: index === impedances.length - 1 ? 1.5 : 1.0, // Load point is bigger
                highlight: index === impedances.length - 1 ? 1.0 : 0.0
            };
        });
        
        this.uniformBuffer.update({
            pointCount: points.length,
            points: points
        });
    }

    impedanceToGamma(zNorm) {
        // Γ = (Z - 1) / (Z + 1)
        const numeratorReal = zNorm.real - 1;
        const numeratorImag = zNorm.imag;
        const denominatorReal = zNorm.real + 1;
        const denominatorImag = zNorm.imag;
        
        const denomMag = denominatorReal * denominatorReal + denominatorImag * denominatorImag;
        
        return {
            real: (numeratorReal * denominatorReal + numeratorImag * denominatorImag) / denomMag,
            imag: (numeratorImag * denominatorReal - numeratorReal * denominatorImag) / denomMag
        };
    }

    // Enhanced trace support for point streams
    addTrace(traceData) {
        // Store trace data for rendering
        if (!this.traces) {
            this.traces = new Map();
        }
        
        const traceId = traceData.label || `trace_${this.traces.size}`;
        this.traces.set(traceId, traceData);
        
        // Update WebGPU buffers with new trace data
        this.updateTraceBuffers();
    }
    
    removeTrace(traceId) {
        if (this.traces) {
            this.traces.delete(traceId);
            this.updateTraceBuffers();
        }
    }
    
    clearTraces() {
        if (this.traces) {
            this.traces.clear();
            this.updateTraceBuffers();
        }
    }
    
    updateTraceBuffers() {
        if (!this.traces || this.traces.size === 0) {
            this.uniformBuffer.update({ pointCount: 0 });
            return;
        }
        
        // Combine all trace points
        let allPoints = [];
        let totalPoints = 0;
        
        for (const [traceId, trace] of this.traces) {
            const points = trace.points;
            const color = this.hexToRgb(trace.color || '#00FF80');
            
            // Convert point array [x0,y0,x1,y1,...] to point objects
            for (let i = 0; i < points.length; i += 2) {
                allPoints.push({
                    x: points[i],
                    y: points[i + 1],
                    r: color.r,
                    g: color.g,
                    b: color.b,
                    a: trace.opacity || 1.0,
                    size: trace.type === 'cloud' ? 0.5 : 1.0
                });
                totalPoints++;
            }
        }
        
        // Update uniform buffer
        this.uniformBuffer.update({
            pointCount: totalPoints,
            tracePoints: allPoints
        });
    }
    
    hexToRgb(hex) {
        const result = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex);
        return result ? {
            r: parseInt(result[1], 16) / 255,
            g: parseInt(result[2], 16) / 255,
            b: parseInt(result[3], 16) / 255
        } : { r: 0, g: 1, b: 0.5 };
    }
    
    clearImpedancePoints() {
        this.uniformBuffer.update({
            pointCount: 0,
            points: []
        });
    }
}