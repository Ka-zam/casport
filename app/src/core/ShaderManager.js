export class ShaderManager {
    constructor(device) {
        this.device = device;
        this.shaderModules = new Map();
    }

    async loadShaderFile(path) {
        const response = await fetch(path);
        if (!response.ok) {
            throw new Error(`Failed to load shader: ${path}`);
        }
        return await response.text();
    }

    createShaderModule(label, code) {
        const module = this.device.createShaderModule({
            label,
            code
        });
        
        this.shaderModules.set(label, module);
        return module;
    }

    async loadAndCreateModule(label, path) {
        const code = await this.loadShaderFile(path);
        return this.createShaderModule(label, code);
    }

    getModule(label) {
        return this.shaderModules.get(label);
    }

    createRenderPipeline(config) {
        const {
            label,
            shaderModule,
            vertexEntry = 'vs_main',
            fragmentEntry = 'fs_main',
            format,
            topology = 'triangle-list'
        } = config;

        return this.device.createRenderPipeline({
            label,
            layout: 'auto',
            vertex: {
                module: shaderModule,
                entryPoint: vertexEntry
            },
            fragment: {
                module: shaderModule,
                entryPoint: fragmentEntry,
                targets: [{
                    format
                }]
            },
            primitive: {
                topology
            }
        });
    }
}