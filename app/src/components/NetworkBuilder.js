import { initCascadix, cascadixNetwork, complex, formatComplex } from '../utils/CascadixWrapper.js';

export class NetworkBuilder {
    constructor(container, smithChart = null, rlPlot = null) {
        this.container = container;
        this.smithChart = smithChart;
        this.rlPlot = rlPlot;
        this.components = [];
        this.selectedComponent = null;
        this.frequency = 2.0; // Default 2 GHz
        this.cascadixReady = false;
        
        this.init();
        this.initializeCascadix();
    }
    
    async initializeCascadix() {
        try {
            const module = await initCascadix();
            this.cascadixReady = true;
            cascadixNetwork.setFrequency(this.frequency);
            console.log('Cascadix initialized in NetworkBuilder', module);
            console.log('Available functions:', Object.keys(module).slice(0, 20));
            this.updateCalculations(); // Update once cascadix is ready
        } catch (error) {
            console.error('Failed to initialize Cascadix:', error);
        }
    }

    init() {
        this.render();
        this.setupEventListeners();
        // Initial calculation to show load point on Smith Chart
        setTimeout(() => this.updateCalculations(), 100);
    }

    render() {
        this.container.innerHTML = `
            <div class="network-builder">
                <div class="component-palette">
                    <div class="palette-title">Component Library</div>
                    <div class="palette-items">
                        <div class="component-item" data-type="L-series" draggable="true">
                            <span class="component-symbol">━L━</span>
                            <span class="component-label">Series L</span>
                        </div>
                        <div class="component-item" data-type="C-series" draggable="true">
                            <span class="component-symbol">━C━</span>
                            <span class="component-label">Series C</span>
                        </div>
                        <div class="component-item" data-type="R-series" draggable="true">
                            <span class="component-symbol">━R━</span>
                            <span class="component-label">Series R</span>
                        </div>
                        <div class="component-item" data-type="L-shunt" draggable="true">
                            <span class="component-symbol">┴L┴</span>
                            <span class="component-label">Shunt L</span>
                        </div>
                        <div class="component-item" data-type="C-shunt" draggable="true">
                            <span class="component-symbol">┴C┴</span>
                            <span class="component-label">Shunt C</span>
                        </div>
                        <div class="component-item" data-type="R-shunt" draggable="true">
                            <span class="component-symbol">┴R┴</span>
                            <span class="component-label">Shunt R</span>
                        </div>
                    </div>
                </div>
                
                <div class="network-chain">
                    <div class="generator">
                        <div class="terminal-box">
                            <span class="terminal-label">Generator</span>
                            <span class="terminal-value">50Ω</span>
                        </div>
                    </div>
                    
                    <div class="chain-area" id="chain-area">
                        <div class="chain-placeholder">
                            Drag components here to build network
                        </div>
                    </div>
                    
                    <div class="load">
                        <div class="terminal-box">
                            <span class="terminal-label">Load</span>
                            <input type="text" class="terminal-input" id="load-input" 
                                   placeholder="50+j0" value="50+j0">
                        </div>
                    </div>
                </div>
                
                <div class="network-info">
                    <div class="info-item">
                        <span class="info-label">Input Z:</span>
                        <span class="info-value" id="input-impedance">50.0 + j0.0 Ω</span>
                    </div>
                    <div class="info-item">
                        <span class="info-label">VSWR:</span>
                        <span class="info-value" id="vswr-value">1.00</span>
                    </div>
                    <div class="info-item">
                        <span class="info-label">RL:</span>
                        <span class="info-value" id="rl-value">-∞ dB</span>
                    </div>
                </div>
            </div>
        `;
        
        this.addStyles();
    }

    addStyles() {
        const style = document.createElement('style');
        style.textContent = `
            .network-builder {
                display: flex;
                flex-direction: column;
                height: 100%;
                padding: 1rem;
            }
            
            .component-palette {
                background: #f0ece4;
                border: 1px solid #2a2a2e;
                border-radius: 4px;
                padding: 0.5rem;
                margin-bottom: 1rem;
            }
            
            .palette-title {
                font-weight: bold;
                margin-bottom: 0.5rem;
                color: #2a2a2e;
            }
            
            .palette-items {
                display: flex;
                gap: 0.5rem;
                flex-wrap: wrap;
            }
            
            .component-item {
                background: #faf8f4;
                border: 1px solid #2a2a2e;
                border-radius: 3px;
                padding: 0.25rem 0.5rem;
                cursor: grab;
                display: flex;
                flex-direction: column;
                align-items: center;
                font-size: 0.8rem;
            }
            
            .component-item:hover {
                background: #fff;
                box-shadow: 0 2px 4px rgba(0,0,0,0.1);
            }
            
            .component-symbol {
                font-family: monospace;
                font-size: 1rem;
                font-weight: bold;
            }
            
            .network-chain {
                flex: 1;
                display: flex;
                align-items: center;
                gap: 1rem;
                padding: 1rem;
                background: #fff;
                border: 2px solid #2a2a2e;
                border-radius: 4px;
                overflow-x: auto;
            }
            
            .generator, .load {
                flex-shrink: 0;
            }
            
            .terminal-box {
                background: #2a2a2e;
                color: #faf8f4;
                padding: 1rem;
                border-radius: 4px;
                display: flex;
                flex-direction: column;
                align-items: center;
                min-width: 80px;
            }
            
            .terminal-label {
                font-size: 0.75rem;
                opacity: 0.8;
                margin-bottom: 0.25rem;
            }
            
            .terminal-value {
                font-weight: bold;
                font-size: 1.1rem;
            }
            
            .terminal-input {
                background: #faf8f4;
                color: #2a2a2e;
                border: 1px solid #4a4a5e;
                padding: 0.25rem;
                border-radius: 2px;
                width: 80px;
                text-align: center;
                font-family: monospace;
                font-weight: bold;
            }
            
            .chain-area {
                flex: 1;
                min-height: 100px;
                display: flex;
                align-items: center;
                gap: 0.5rem;
                padding: 0 1rem;
                border-left: 2px solid #2a2a2e;
                border-right: 2px solid #2a2a2e;
                position: relative;
            }
            
            .chain-placeholder {
                position: absolute;
                width: 100%;
                text-align: center;
                color: #8a8a9e;
                pointer-events: none;
            }
            
            .chain-area.has-components .chain-placeholder {
                display: none;
            }
            
            .network-component {
                background: #f0ece4;
                border: 2px solid #2a2a2e;
                border-radius: 4px;
                padding: 0.5rem;
                min-width: 60px;
                text-align: center;
                cursor: pointer;
                position: relative;
            }
            
            .network-component.selected {
                background: #e0dcd4;
                box-shadow: 0 2px 8px rgba(0,0,0,0.2);
            }
            
            .component-type {
                font-weight: bold;
                font-size: 1.2rem;
                font-family: monospace;
            }
            
            .component-value {
                font-size: 0.9rem;
                margin-top: 0.25rem;
                color: #4a4a5e;
            }
            
            .network-info {
                display: flex;
                gap: 2rem;
                margin-top: 1rem;
                padding: 0.75rem;
                background: #f0ece4;
                border-radius: 4px;
            }
            
            .info-item {
                display: flex;
                gap: 0.5rem;
            }
            
            .info-label {
                color: #4a4a5e;
            }
            
            .info-value {
                font-weight: bold;
                font-family: monospace;
            }
        `;
        document.head.appendChild(style);
    }

    setupEventListeners() {
        // Drag and drop
        const items = this.container.querySelectorAll('.component-item');
        const chainArea = document.getElementById('chain-area');
        
        items.forEach(item => {
            item.addEventListener('dragstart', (e) => {
                e.dataTransfer.effectAllowed = 'copy';
                e.dataTransfer.setData('component-type', item.dataset.type);
            });
        });
        
        chainArea.addEventListener('dragover', (e) => {
            e.preventDefault();
            e.dataTransfer.dropEffect = 'copy';
        });
        
        chainArea.addEventListener('drop', (e) => {
            e.preventDefault();
            const type = e.dataTransfer.getData('component-type');
            this.addComponent(type);
        });
        
        // Load impedance input
        const loadInput = document.getElementById('load-input');
        loadInput.addEventListener('input', () => {
            this.updateCalculations();
        });
    }

    addComponent(type) {
        const chainArea = document.getElementById('chain-area');
        chainArea.classList.add('has-components');
        
        const component = document.createElement('div');
        component.className = 'network-component';
        component.innerHTML = `
            <div class="component-type">${type.split('-')[0]}</div>
            <div class="component-value">1.0</div>
        `;
        
        const placeholder = chainArea.querySelector('.chain-placeholder');
        if (placeholder) {
            chainArea.insertBefore(component, placeholder);
        }
        
        this.components.push({ type, value: 1.0, element: component });
        this.updateCalculations();
    }

    updateCalculations() {
        if (!this.cascadixReady) return;
        
        const inputZ = document.getElementById('input-impedance');
        const vswrValue = document.getElementById('vswr-value');
        const rlValue = document.getElementById('rl-value');
        
        // Get load impedance from input
        const loadInput = document.getElementById('load-input');
        const loadStr = loadInput.value || '50+j0';
        const loadZ = this.parseComplex(loadStr);
        
        // Clear and rebuild cascadix network
        cascadixNetwork.clearComponents();
        cascadixNetwork.setFrequency(this.frequency);
        
        // Add all components to cascadix
        for (const comp of this.components) {
            cascadixNetwork.addComponent(comp.type, comp.value);
        }
        
        // Get impedance points for Smith Chart
        const impedancePoints = cascadixNetwork.getImpedancePoints(loadZ);
        
        // Get input impedance (what the generator sees)
        const inputImpedance = cascadixNetwork.getInputImpedance(loadZ);
        
        // Calculate reflection coefficient and derived values
        const gamma = cascadixNetwork.impedanceToGamma(inputImpedance);
        const vswr = cascadixNetwork.gammaToVSWR(gamma);
        const rl = cascadixNetwork.gammaToReturnLoss(gamma);
        
        // Update display
        inputZ.textContent = formatComplex(inputImpedance) + ' Ω';
        vswrValue.textContent = vswr === Infinity ? '∞' : vswr.toFixed(2);
        rlValue.textContent = rl === -Infinity ? '-∞ dB' : rl.toFixed(1) + ' dB';
        
        // Update Smith Chart
        if (this.smithChart) {
            this.smithChart.setImpedancePoints(impedancePoints);
        }
    }
    
    parseComplex(str) {
        // Parse complex number from string (e.g., "50+j25" or "50-j25")
        const match = str.match(/^([-+]?\d*\.?\d+)\s*([+-])\s*j\s*([-+]?\d*\.?\d+)$/);
        if (match) {
            const real = parseFloat(match[1]);
            const sign = match[2] === '+' ? 1 : -1;
            const imag = sign * Math.abs(parseFloat(match[3]));
            return complex(real, imag);
        }
        
        // Try to parse as real only
        const real = parseFloat(str);
        if (!isNaN(real)) {
            return complex(real, 0);
        }
        
        return complex(50, 0); // Default
    }

    setFrequency(freq) {
        this.frequency = freq;
        this.updateCalculations();
    }

    performFrequencySweep(startFreq, stopFreq, numPoints) {
        console.log('performFrequencySweep called:', { startFreq, stopFreq, numPoints, cascadixReady: this.cascadixReady });
        
        if (!this.cascadixReady) {
            console.log('Cascadix not ready, skipping frequency sweep');
            return null;
        }
        
        // Get load impedance
        const loadInput = document.getElementById('load-input');
        const loadStr = loadInput?.value || '50+j0';
        const loadZ = this.parseComplex(loadStr);
        console.log('Load impedance:', loadZ);
        
        // Clear and rebuild cascadix network with current components
        cascadixNetwork.clearComponents();
        for (const comp of this.components) {
            cascadixNetwork.addComponent(comp.type, comp.value);
        }
        console.log('Components added:', this.components.length);
        
        // Perform frequency sweep
        const sweepData = cascadixNetwork.performFrequencySweep(
            loadZ, 
            startFreq, 
            stopFreq, 
            numPoints
        );
        console.log('Sweep data:', sweepData ? 'Generated' : 'Failed');
        
        // Update RL plot if available
        if (this.rlPlot && sweepData) {
            this.rlPlot.updateData(
                sweepData.frequencies,
                sweepData.returnLosses,
                sweepData.vswrs
            );
        }
        
        return sweepData;
    }

    getComponents() {
        return this.components;
    }

    getLoadImpedance() {
        const loadInput = document.getElementById('load-input');
        const loadStr = loadInput.value || '50+j0';
        return this.parseComplex(loadStr);
    }
}