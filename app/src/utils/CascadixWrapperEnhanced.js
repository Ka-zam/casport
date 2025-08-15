import { initCascadix, CascadixNetwork } from './CascadixWrapper.js';

export class CascadixWrapperEnhanced {
    constructor() {
        this.module = null;
        this.network = new CascadixNetwork();
        this.generators = new Map(); // Cache for network generators
    }
    
    async init() {
        this.module = await initCascadix();
        return this.module;
    }
    
    // Build network at specific frequency
    buildNetworkAtFrequency(components, frequency) {
        if (!this.module) {
            throw new Error('Cascadix module not initialized');
        }
        
        // Create network by cascading components
        let network = null;
        
        for (const component of components) {
            const componentNetwork = this.createComponentNetwork(component, frequency);
            if (network === null) {
                network = componentNetwork;
            } else {
                network = this.module.cascade(network, componentNetwork);
            }
        }
        
        // If no components, return a simple series resistor
        return network || this.module.seriesResistor(50.0);
    }
    
    // Create individual component network
    createComponentNetwork(component, frequency) {
        const { type, value } = component;
        
        switch (type) {
            case 'series_resistor':
                return this.module.seriesResistor(value);
                
            case 'series_inductor':
                return this.module.seriesInductor(value, frequency);
                
            case 'series_capacitor':
                return this.module.seriesCapacitor(value, frequency);
                
            case 'shunt_resistor':
                return this.module.shuntResistor(value);
                
            case 'shunt_inductor':
                return this.module.shuntInductor(value, frequency);
                
            case 'shunt_capacitor':
                return this.module.shuntCapacitor(value, frequency);
                
            case 'transmission_line':
                return this.module.transmissionLine(value, 50.0, frequency); // Default 50Ω
                
            default:
                console.warn(`Unknown component type: ${type}`);
                return this.module.seriesResistor(50.0); // Default fallback
        }
    }
    
    // Generate frequency sweep using enhanced point stream generator
    async generateFrequencySweep(networkBuilder, freqStart, freqStop, numPoints, zLoad, z0System) {
        if (!this.module) {
            throw new Error('Cascadix module not initialized');
        }
        
        try {
            // Manual frequency sweep using available WASM functions
            const points = [];
            const frequencies = [];
            
            // Generate frequency points
            for (let i = 0; i < numPoints; i++) {
                const freq = freqStart + (freqStop - freqStart) * i / (numPoints - 1);
                frequencies.push(freq);
            }
            
            // Calculate network parameters for each frequency
            frequencies.forEach((frequency, index) => {
                try {
                    // Build the network at this frequency
                    const network = networkBuilder(frequency);
                    
                    // Calculate S-parameters
                    const sParams = this.module.getSParameters(network, z0System);
                    
                    // Convert S11 to Smith chart coordinates
                    const s11 = { real: sParams.s11.real, imag: sParams.s11.imag };
                    
                    points.push({
                        x: s11.real,
                        y: s11.imag,
                        frequency: frequency,
                        s11_mag: Math.sqrt(s11.real * s11.real + s11.imag * s11.imag),
                        s11_phase: Math.atan2(s11.imag, s11.real) * 180 / Math.PI,
                        return_loss: -20 * Math.log10(Math.sqrt(s11.real * s11.real + s11.imag * s11.imag)),
                        vswr: (1 + Math.sqrt(s11.real * s11.real + s11.imag * s11.imag)) / 
                              (1 - Math.sqrt(s11.real * s11.real + s11.imag * s11.imag))
                    });
                } catch (err) {
                    console.warn(`Error at frequency ${frequency}: ${err.message}`);
                }
            });
            
            return {
                points: points,
                metadata: {
                    type: 'frequency_sweep',
                    startFreq: freqStart,
                    stopFreq: freqStop,
                    numPoints: numPoints,
                    z0: z0System
                }
            };
            
        } catch (error) {
            console.error('Error generating frequency sweep:', error);
            throw error;
        }
    }
    
    // Generate component sweep
    async generateComponentSweep(components, componentId, minValue, maxValue, numPoints, frequency, zLoad, z0System) {
        if (!this.module) {
            throw new Error('Cascadix module not initialized');
        }
        
        try {
            const component = components.find(comp => comp.id === componentId);
            if (!component) {
                throw new Error(`Component with ID ${componentId} not found`);
            }
            
            // Create component sweep
            const componentSweep = this.module.component_sweep(
                this.getComponentType(component.type),
                minValue,
                maxValue,
                numPoints,
                frequency,
                'LINEAR'
            );
            
            // Create enhanced generator
            const generator = this.module.smith_chart_generator_enhanced();
            
            // Generate point stream
            const pointStream = generator.generate_component_sweep_stream(
                componentSweep,
                { real: zLoad, imag: 0.0 },
                z0System
            );
            
            return this.convertPointStreamToJS(pointStream);
            
        } catch (error) {
            console.error('Error generating component sweep:', error);
            throw error;
        }
    }
    
    // Generate Monte Carlo analysis
    async generateMonteCarlo(components, numSamples, frequency, tolerance, zLoad, z0System) {
        if (!this.module) {
            throw new Error('Cascadix module not initialized');
        }
        
        try {
            // Create Monte Carlo sampler
            const sampler = this.module.monte_carlo_sampler();
            
            // Create component variations
            const variations = components.map(comp => {
                return this.module.component_variation(
                    comp.value,
                    comp.tolerance || 5.0, // Default 5% tolerance
                    'GAUSSIAN'
                );
            });
            
            // Network builder function for Monte Carlo
            const networkBuilder = (componentValues) => {
                let network = this.module.identity_two_port();
                
                for (let i = 0; i < components.length; i++) {
                    const comp = { ...components[i], value: componentValues[i] };
                    const componentNetwork = this.createComponentNetwork(comp, frequency);
                    network = this.module.cascade_two_port(network, componentNetwork);
                }
                
                return network;
            };
            
            // Generate impedance samples
            const impedances = sampler.generate_impedance_samples(
                networkBuilder,
                variations,
                numSamples,
                frequency,
                { real: zLoad, imag: 0.0 }
            );
            
            // Create enhanced generator and convert to point stream
            const generator = this.module.smith_chart_generator_enhanced();
            const pointStream = generator.generate_monte_carlo_stream(
                impedances,
                z0System
            );
            
            return this.convertPointStreamToJS(pointStream);
            
        } catch (error) {
            console.error('Error generating Monte Carlo analysis:', error);
            throw error;
        }
    }
    
    // Generate 2D mesh for frequency-component variation
    async generate2DMesh(components, componentId, freqStart, freqStop, freqPoints, 
                        compMin, compMax, compPoints, zLoad, z0System) {
        if (!this.module) {
            throw new Error('Cascadix module not initialized');
        }
        
        try {
            const component = components.find(comp => comp.id === componentId);
            if (!component) {
                throw new Error(`Component with ID ${componentId} not found`);
            }
            
            // Create frequency sweep
            const freqSweep = this.module.frequency_sweep(freqStart, freqStop, freqPoints, 'LOG');
            
            // Network builder for 2D mesh
            const networkBuilder = (frequency, componentValue) => {
                let network = this.module.identity_two_port();
                
                for (const comp of components) {
                    const value = (comp.id === componentId) ? componentValue : comp.value;
                    const compData = { ...comp, value: value };
                    const componentNetwork = this.createComponentNetwork(compData, frequency);
                    network = this.module.cascade_two_port(network, componentNetwork);
                }
                
                return network;
            };
            
            // Create enhanced generator
            const generator = this.module.smith_chart_generator_enhanced();
            
            // Generate 2D mesh
            const mesh = generator.generate_2d_mesh(
                networkBuilder,
                freqSweep,
                compMin,
                compMax,
                compPoints,
                { real: zLoad, imag: 0.0 },
                z0System
            );
            
            return this.convertMeshToJS(mesh);
            
        } catch (error) {
            console.error('Error generating 2D mesh:', error);
            throw error;
        }
    }
    
    // Generate animated sweep
    async generateAnimatedSweep(networkBuilder, freqStart, freqStop, numPoints, 
                               animationDuration, zLoad, z0System) {
        if (!this.module) {
            throw new Error('Cascadix module not initialized');
        }
        
        try {
            // Create frequency sweep
            const freqSweep = this.module.frequency_sweep(freqStart, freqStop, numPoints, 'LOG');
            
            // Create enhanced generator
            const generator = this.module.smith_chart_generator_enhanced();
            
            // Generate animated point stream
            const pointStream = generator.generate_animated_sweep(
                networkBuilder,
                freqSweep,
                { real: zLoad, imag: 0.0 },
                z0System,
                animationDuration
            );
            
            return this.convertPointStreamToJS(pointStream);
            
        } catch (error) {
            console.error('Error generating animated sweep:', error);
            throw error;
        }
    }
    
    // Helper: Convert WASM point stream to JavaScript object
    convertPointStreamToJS(pointStream) {
        return {
            points: Array.from(pointStream.points),
            values: Array.from(pointStream.values),
            timestamps: Array.from(pointStream.timestamps),
            metadata: {
                type: pointStream.metadata.type,
                color: pointStream.metadata.color_rgba,
                lineWidth: pointStream.metadata.line_width,
                opacity: pointStream.metadata.opacity,
                showMarkers: pointStream.metadata.show_markers,
                label: pointStream.metadata.label
            }
        };
    }
    
    // Helper: Convert WASM mesh to JavaScript object
    convertMeshToJS(mesh) {
        return {
            vertices: Array.from(mesh.vertices),
            values: Array.from(mesh.values),
            indices: Array.from(mesh.indices),
            rows: mesh.rows,
            cols: mesh.cols,
            metadata: {
                type: mesh.metadata.type,
                color: mesh.metadata.color_rgba,
                lineWidth: mesh.metadata.line_width,
                opacity: mesh.metadata.opacity
            }
        };
    }
    
    // Helper: Map component type string to WASM enum
    getComponentType(typeString) {
        const typeMap = {
            'series_resistor': 'SERIES_R',
            'series_inductor': 'SERIES_L',
            'series_capacitor': 'SERIES_C',
            'shunt_resistor': 'SHUNT_R',
            'shunt_inductor': 'SHUNT_L',
            'shunt_capacitor': 'SHUNT_C',
            'transmission_line': 'TRANSMISSION_LINE'
        };
        
        return typeMap[typeString] || 'SERIES_R';
    }
    
    // Calculate network parameters at single frequency
    async calculateNetworkParams(components, frequency, zLoad, z0System) {
        if (!this.module) {
            throw new Error('Cascadix module not initialized');
        }
        
        try {
            const network = this.buildNetworkAtFrequency(components, frequency);
            const zInput = network.input_impedance({ real: zLoad, imag: 0.0 });
            const sParams = network.to_s_parameters(z0System);
            
            return {
                inputImpedance: zInput,
                sParameters: sParams,
                vswr: sParams.vswr(),
                returnLoss: sParams.return_loss_db(),
                insertionLoss: sParams.insertion_loss_db()
            };
            
        } catch (error) {
            console.error('Error calculating network parameters:', error);
            throw error;
        }
    }
}