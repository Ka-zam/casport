export class NetworkAnalysis {
    constructor() {
        this.z0 = 50; // Characteristic impedance
    }

    // Complex number operations
    static complexAdd(a, b) {
        return { real: a.real + b.real, imag: a.imag + b.imag };
    }

    static complexSubtract(a, b) {
        return { real: a.real - b.real, imag: a.imag - b.imag };
    }

    static complexMultiply(a, b) {
        return {
            real: a.real * b.real - a.imag * b.imag,
            imag: a.real * b.imag + a.imag * b.real
        };
    }

    static complexDivide(a, b) {
        const denominator = b.real * b.real + b.imag * b.imag;
        return {
            real: (a.real * b.real + a.imag * b.imag) / denominator,
            imag: (a.imag * b.real - a.real * b.imag) / denominator
        };
    }

    static complexMagnitude(z) {
        return Math.sqrt(z.real * z.real + z.imag * z.imag);
    }

    static complexPhase(z) {
        return Math.atan2(z.imag, z.real);
    }

    // Convert impedance to reflection coefficient
    impedanceToGamma(z, z0 = 50) {
        const zNorm = { real: z.real / z0, imag: z.imag / z0 };
        const numerator = NetworkAnalysis.complexSubtract(zNorm, { real: 1, imag: 0 });
        const denominator = NetworkAnalysis.complexAdd(zNorm, { real: 1, imag: 0 });
        return NetworkAnalysis.complexDivide(numerator, denominator);
    }

    // Convert reflection coefficient to impedance
    gammaToImpedance(gamma, z0 = 50) {
        const one = { real: 1, imag: 0 };
        const numerator = NetworkAnalysis.complexAdd(one, gamma);
        const denominator = NetworkAnalysis.complexSubtract(one, gamma);
        const zNorm = NetworkAnalysis.complexDivide(numerator, denominator);
        return {
            real: zNorm.real * z0,
            imag: zNorm.imag * z0
        };
    }

    // Calculate VSWR from reflection coefficient
    gammaToVSWR(gamma) {
        const rho = NetworkAnalysis.complexMagnitude(gamma);
        if (rho >= 1) return Infinity;
        return (1 + rho) / (1 - rho);
    }

    // Calculate Return Loss from reflection coefficient
    gammaToReturnLoss(gamma) {
        const rho = NetworkAnalysis.complexMagnitude(gamma);
        if (rho === 0) return -Infinity;
        return -20 * Math.log10(rho);
    }

    // Series impedance addition
    addSeriesImpedance(z1, z2) {
        return NetworkAnalysis.complexAdd(z1, z2);
    }

    // Parallel impedance addition
    addParallelImpedance(z1, z2) {
        const product = NetworkAnalysis.complexMultiply(z1, z2);
        const sum = NetworkAnalysis.complexAdd(z1, z2);
        return NetworkAnalysis.complexDivide(product, sum);
    }

    // Calculate series inductor impedance at frequency
    seriesInductor(L, freq) {
        const omega = 2 * Math.PI * freq * 1e9; // Convert GHz to Hz
        return { real: 0, imag: omega * L * 1e-9 }; // L in nH
    }

    // Calculate series capacitor impedance at frequency
    seriesCapacitor(C, freq) {
        const omega = 2 * Math.PI * freq * 1e9; // Convert GHz to Hz
        return { real: 0, imag: -1 / (omega * C * 1e-12) }; // C in pF
    }

    // Calculate series resistor impedance
    seriesResistor(R) {
        return { real: R, imag: 0 };
    }

    // Transform impedance through a network of components
    transformImpedance(loadZ, components, frequency) {
        let z = loadZ;
        
        // Work backwards from load to source
        for (let i = components.length - 1; i >= 0; i--) {
            const comp = components[i];
            let componentZ;
            
            switch (comp.type) {
                case 'L-series':
                    componentZ = this.seriesInductor(comp.value, frequency);
                    z = this.addSeriesImpedance(z, componentZ);
                    break;
                case 'C-series':
                    componentZ = this.seriesCapacitor(comp.value, frequency);
                    z = this.addSeriesImpedance(z, componentZ);
                    break;
                case 'R-series':
                    componentZ = this.seriesResistor(comp.value);
                    z = this.addSeriesImpedance(z, componentZ);
                    break;
                case 'L-shunt':
                    componentZ = this.seriesInductor(comp.value, frequency);
                    z = this.addParallelImpedance(z, componentZ);
                    break;
                case 'C-shunt':
                    componentZ = this.seriesCapacitor(comp.value, frequency);
                    z = this.addParallelImpedance(z, componentZ);
                    break;
                case 'R-shunt':
                    componentZ = this.seriesResistor(comp.value);
                    z = this.addParallelImpedance(z, componentZ);
                    break;
            }
        }
        
        return z;
    }

    // Analyze network over frequency sweep
    analyzeFrequencySweep(loadZ, components, freqStart, freqStop, numPoints) {
        const results = {
            frequencies: [],
            impedances: [],
            gammas: [],
            vswrs: [],
            returnLosses: []
        };
        
        for (let i = 0; i < numPoints; i++) {
            const freq = freqStart + (freqStop - freqStart) * i / (numPoints - 1);
            const z = this.transformImpedance(loadZ, components, freq);
            const gamma = this.impedanceToGamma(z, this.z0);
            const vswr = this.gammaToVSWR(gamma);
            const rl = this.gammaToReturnLoss(gamma);
            
            results.frequencies.push(freq);
            results.impedances.push(z);
            results.gammas.push(gamma);
            results.vswrs.push(vswr);
            results.returnLosses.push(rl);
        }
        
        return results;
    }

    // Format complex number as string
    static formatComplex(z, precision = 1) {
        const sign = z.imag >= 0 ? '+' : '';
        return `${z.real.toFixed(precision)}${sign}j${z.imag.toFixed(precision)}`;
    }

    // Parse complex number from string (e.g., "50+j25" or "50-j25")
    static parseComplex(str) {
        const match = str.match(/^([-+]?\d*\.?\d+)\s*([+-])\s*j\s*([-+]?\d*\.?\d+)$/);
        if (match) {
            const real = parseFloat(match[1]);
            const sign = match[2] === '+' ? 1 : -1;
            const imag = sign * Math.abs(parseFloat(match[3]));
            return { real, imag };
        }
        
        // Try to parse as real only
        const real = parseFloat(str);
        if (!isNaN(real)) {
            return { real, imag: 0 };
        }
        
        return { real: 50, imag: 0 }; // Default
    }
}