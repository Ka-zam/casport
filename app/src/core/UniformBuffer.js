export class UniformBuffer {
    constructor(device, size = 512) {
        this.device = device;
        this.size = size;
        this.buffer = null;
        this.data = new Float32Array(128); // Increased for points data
        this.uniforms = {
            resolution: [0, 0],
            zoom: 1.0,
            pan: [0.0, 0.0],
            time: 0.0,
            gridDensity: 1.0,
            admittanceMode: 0.0,
            pointCount: 0,
            points: []  // Array of {x, y, size, highlight}
        };
        
        this.createBuffer();
    }

    createBuffer() {
        this.buffer = this.device.createBuffer({
            label: 'Uniforms',
            size: this.size,
            usage: GPUBufferUsage.UNIFORM | GPUBufferUsage.COPY_DST
        });
    }

    update(updates = {}) {
        Object.assign(this.uniforms, updates);
        
        this.data.set(this.uniforms.resolution, 0);
        this.data[2] = this.uniforms.zoom;
        this.data.set(this.uniforms.pan, 4);
        this.data[6] = this.uniforms.time;
        this.data[7] = this.uniforms.gridDensity;
        this.data[8] = this.uniforms.admittanceMode;
        this.data[9] = this.uniforms.pointCount;
        // Padding at indices 10, 11, 12
        
        // Points start at index 16 (aligned to vec4)
        const pointsStartIndex = 16;
        for (let i = 0; i < 16; i++) {
            if (i < this.uniforms.points.length) {
                const point = this.uniforms.points[i];
                this.data[pointsStartIndex + i * 4] = point.x || 0;
                this.data[pointsStartIndex + i * 4 + 1] = point.y || 0;
                this.data[pointsStartIndex + i * 4 + 2] = point.size || 1;
                this.data[pointsStartIndex + i * 4 + 3] = point.highlight || 0;
            } else {
                this.data[pointsStartIndex + i * 4] = 0;
                this.data[pointsStartIndex + i * 4 + 1] = 0;
                this.data[pointsStartIndex + i * 4 + 2] = 0;
                this.data[pointsStartIndex + i * 4 + 3] = 0;
            }
        }
        
        this.device.queue.writeBuffer(this.buffer, 0, this.data);
    }

    setResolution(width, height) {
        this.uniforms.resolution = [width, height];
        this.update();
    }

    setZoom(zoom) {
        this.uniforms.zoom = zoom;
        this.update();
    }

    setPan(x, y) {
        this.uniforms.pan = [x, y];
        this.update();
    }

    setTime(time) {
        this.uniforms.time = time;
        this.update();
    }

    setGridDensity(density) {
        this.uniforms.gridDensity = density;
        this.update();
    }

    getBuffer() {
        return this.buffer;
    }

    getUniforms() {
        return { ...this.uniforms };
    }
}