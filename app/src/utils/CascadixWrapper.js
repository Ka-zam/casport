// Wrapper for Cascadix WASM module
let Module = null;
let moduleReady = false;

// Load the WASM module
export async function initCascadix() {
    if (moduleReady) return Module;
    
    try {
        // Dynamically load the WASM module script
        return new Promise((resolve, reject) => {
            const script = document.createElement('script');
            script.src = '/cascadix_wasm.js';
            
            // The cascadix_wasm.js defines CascadixModule
            script.onload = () => {
                // Wait for CascadixModule to be available
                if (typeof CascadixModule !== 'undefined') {
                    // CascadixModule is the emscripten module factory
                    CascadixModule().then((instance) => {
                        Module = instance;
                        moduleReady = true;
                        console.log('Cascadix WASM module loaded successfully');
                        console.log('Available functions:', Object.keys(Module).filter(k => typeof Module[k] === 'function').slice(0, 20));
                        
                        // Test basic functionality
                        try {
                            if (Module.seriesResistor) {
                                const r = Module.seriesResistor(50);
                                console.log('Test: Created series resistor', r);
                            }
                        } catch (e) {
                            console.error('Test failed:', e);
                        }
                        
                        resolve(Module);
                    }).catch(reject);
                } else {
                    reject(new Error('CascadixModule not defined after loading script'));
                }
            };
            
            script.onerror = () => {
                reject(new Error('Failed to load cascadix_wasm.js'));
            };
            
            document.head.appendChild(script);
        });
    } catch (error) {
        console.error('Failed to load Cascadix WASM:', error);
        throw error;
    }
}

// Helper to create complex numbers
export function complex(real, imag = 0) {
    return { real, imag };
}

// Helper to format complex numbers
export function formatComplex(c, precision = 2) {
    if (!c) return '0';
    const real = c.real.toFixed(precision);
    const imag = Math.abs(c.imag).toFixed(precision);
    
    if (Math.abs(c.imag) < Math.pow(10, -precision)) {
        return real;
    }
    return c.imag >= 0 ? `${real}+j${imag}` : `${real}-j${imag}`;
}

// Create a cascaded network from components
export class CascadixNetwork {
    constructor() {
        this.components = [];
        this.frequency = 2e9; // Default 2 GHz
    }

    setFrequency(freqGHz) {
        this.frequency = freqGHz * 1e9;
    }

    addComponent(type, value) {
        if (!Module) {
            console.error('Cascadix module not initialized');
            return;
        }

        let component;
        let description;

        switch(type) {
            case 'L-series':
                component = Module.seriesInductor(value * 1e-9, this.frequency); // nH to H
                description = `Series L: ${value} nH`;
                break;
            case 'C-series':
                component = Module.seriesCapacitor(value * 1e-12, this.frequency); // pF to F
                description = `Series C: ${value} pF`;
                break;
            case 'R-series':
                component = Module.seriesResistor(value);
                description = `Series R: ${value} Ω`;
                break;
            case 'L-shunt':
                component = Module.shuntInductor(value * 1e-9, this.frequency);
                description = `Shunt L: ${value} nH`;
                break;
            case 'C-shunt':
                component = Module.shuntCapacitor(value * 1e-12, this.frequency);
                description = `Shunt C: ${value} pF`;
                break;
            case 'R-shunt':
                component = Module.shuntResistor(value);
                description = `Shunt R: ${value} Ω`;
                break;
            default:
                console.error('Unknown component type:', type);
                return;
        }

        this.components.push({ component, description, type, value });
    }

    clearComponents() {
        this.components = [];
    }

    getCascadedNetwork() {
        if (!Module || this.components.length === 0) return null;

        let network = this.components[0].component;
        for (let i = 1; i < this.components.length; i++) {
            network = Module.cascade(network, this.components[i].component);
        }
        return network;
    }

    getInputImpedance(loadZ) {
        const network = this.getCascadedNetwork();
        if (!network) return loadZ;

        const z_load = complex(loadZ.real, loadZ.imag);
        return Module.getInputImpedance(network, z_load);
    }

    getSParameters(z0 = 50) {
        const network = this.getCascadedNetwork();
        if (!network) {
            // Return unity matrix S-parameters
            return {
                s11: complex(0, 0),
                s12: complex(1, 0),
                s21: complex(1, 0),
                s22: complex(0, 0)
            };
        }

        return Module.getSParameters(network, z0);
    }

    // Get impedance at each point in the network
    getImpedancePoints(loadZ) {
        if (!Module) return [loadZ];

        const points = [loadZ]; // Start with load
        let currentZ = complex(loadZ.real, loadZ.imag);

        // Work backwards through components (from load to source)
        for (let i = this.components.length - 1; i >= 0; i--) {
            const comp = this.components[i];
            
            // Create a partial network from this component to the load
            let partialNetwork = comp.component;
            for (let j = i + 1; j < this.components.length; j++) {
                partialNetwork = Module.cascade(partialNetwork, this.components[j].component);
            }
            
            // Get input impedance looking into this partial network
            currentZ = Module.getInputImpedance(partialNetwork, complex(loadZ.real, loadZ.imag));
            points.unshift(currentZ); // Add to front (building from source to load)
        }

        return points;
    }

    // Calculate reflection coefficient from impedance
    impedanceToGamma(z, z0 = 50) {
        // Γ = (Z - Z0) / (Z + Z0)
        const numerator = complex(z.real - z0, z.imag);
        const denominator = complex(z.real + z0, z.imag);
        
        const denomMag = denominator.real * denominator.real + denominator.imag * denominator.imag;
        
        return complex(
            (numerator.real * denominator.real + numerator.imag * denominator.imag) / denomMag,
            (numerator.imag * denominator.real - numerator.real * denominator.imag) / denomMag
        );
    }

    // Calculate VSWR from reflection coefficient
    gammaToVSWR(gamma) {
        const rho = Math.sqrt(gamma.real * gamma.real + gamma.imag * gamma.imag);
        if (rho >= 1) return Infinity;
        return (1 + rho) / (1 - rho);
    }

    // Calculate Return Loss from reflection coefficient
    gammaToReturnLoss(gamma) {
        const rho = Math.sqrt(gamma.real * gamma.real + gamma.imag * gamma.imag);
        if (rho === 0) return Infinity; // Perfect match = infinite return loss
        // Return loss is positive (it's a loss)
        return -20 * Math.log10(rho);
    }

    // Perform frequency sweep analysis
    performFrequencySweep(loadZ, startFreqGHz, stopFreqGHz, numPoints) {
        console.log('CascadixNetwork.performFrequencySweep called');
        
        const frequencies = [];
        const returnLosses = [];
        const vswrs = [];
        const impedances = [];

        // If no WASM module or no components, return matched load response
        if (!Module || this.components.length === 0) {
            console.log('Using matched load response (no components or WASM not ready)');
            for (let i = 0; i < numPoints; i++) {
                const freq = startFreqGHz + (stopFreqGHz - startFreqGHz) * i / (numPoints - 1);
                frequencies.push(freq);
                
                // Calculate reflection for load impedance
                const gamma = this.impedanceToGamma(loadZ);
                const vswr = this.gammaToVSWR(gamma);
                const rl = this.gammaToReturnLoss(gamma);
                
                returnLosses.push(rl);
                vswrs.push(vswr);
                impedances.push(loadZ);
            }
        } else {
            // Try to use real WASM calculations
            try {
                for (let i = 0; i < numPoints; i++) {
                    const freq = startFreqGHz + (stopFreqGHz - startFreqGHz) * i / (numPoints - 1);
                    frequencies.push(freq);
                    
                    // Build network at this frequency
                    let network = null;
                    
                    for (const comp of this.components) {
                        let element = null;
                        const freqHz = freq * 1e9;
                        
                        switch(comp.type) {
                            case 'L-series':
                                element = Module.seriesInductor(comp.value * 1e-9, freqHz);
                                break;
                            case 'C-series':
                                element = Module.seriesCapacitor(comp.value * 1e-12, freqHz);
                                break;
                            case 'R-series':
                                element = Module.seriesResistor(comp.value);
                                break;
                            case 'L-shunt':
                                element = Module.shuntInductor(comp.value * 1e-9, freqHz);
                                break;
                            case 'C-shunt':
                                element = Module.shuntCapacitor(comp.value * 1e-12, freqHz);
                                break;
                            case 'R-shunt':
                                element = Module.shuntResistor(comp.value);
                                break;
                        }
                        
                        if (element) {
                            network = network ? Module.cascade(network, element) : element;
                        }
                    }
                    
                    if (network) {
                        // Get input impedance
                        const zIn = Module.getInputImpedance(network, {real: loadZ.real, imag: loadZ.imag});
                        impedances.push(zIn);
                        
                        // Calculate reflection coefficient
                        const gamma = this.impedanceToGamma(zIn);
                        const vswr = this.gammaToVSWR(gamma);
                        const rl = this.gammaToReturnLoss(gamma);
                        
                        returnLosses.push(rl);
                        vswrs.push(vswr);
                    }
                }
                console.log('Used real WASM calculations');
            } catch (e) {
                console.error('WASM calculation failed, using fallback:', e);
                // Fallback to simple calculation
                for (let i = frequencies.length; i < numPoints; i++) {
                    const freq = startFreqGHz + (stopFreqGHz - startFreqGHz) * i / (numPoints - 1);
                    frequencies.push(freq);
                    returnLosses.push(-Infinity);
                    vswrs.push(1.0);
                    impedances.push(complex(50, 0));
                }
            }
        }

        console.log('Sweep data:', {
            frequencies: frequencies.length,
            returnLosses: returnLosses.slice(0, 5),
            vswrs: vswrs.slice(0, 5),
            impedanceSample: impedances.slice(0, 3)
        });

        return {
            frequencies,
            returnLosses,
            vswrs,
            impedances
        };
    }
}

// Export a singleton instance
export const cascadixNetwork = new CascadixNetwork();