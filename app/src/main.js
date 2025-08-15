import { WebGPUContext } from './core/WebGPUContext.js';
import { SmithChart } from './components/SmithChart.js';
import { Controls } from './components/Controls.js';
import { RLPlot } from './components/RLPlot.js';
import { NetworkBuilder } from './ui/NetworkBuilder.js';
import { LayoutGrid } from './ui/LayoutGrid.js';
import { PlotSettingsPanel } from './ui/PlotSettingsPanel.js';
import { CascadixWrapperEnhanced } from './utils/CascadixWrapperEnhanced.js';
import './styles/main.css';

class RFDesignTool {
    constructor() {
        this.layoutGrid = null;
        this.webGPUContext = null;
        this.smithChart = null;
        this.controls = null;
        this.rlPlot = null;
        this.networkBuilder = null;
        this.plotSettingsPanel = null;
        this.animationId = null;
    }

    async initialize() {
        try {
            // Initialize Cascadix WASM
            this.cascadix = new CascadixWrapperEnhanced();
            await this.cascadix.init();

            // Create the layout grid
            const app = document.getElementById('app');
            this.layoutGrid = new LayoutGrid(app);

            // Initialize RL Plot
            const rlPlotContainer = this.layoutGrid.getContainer('rlPlot');
            this.rlPlot = new RLPlot(rlPlotContainer);

            // Initialize Smith Chart
            const canvas = this.layoutGrid.getCanvas();
            this.webGPUContext = new WebGPUContext(canvas);
            await this.webGPUContext.initialize();

            this.smithChart = new SmithChart(this.webGPUContext);
            await this.smithChart.initialize();

            this.controls = new Controls(canvas, this.smithChart);

            // Initialize Enhanced Network Builder with HTML/CSS interface
            const networkContainer = this.layoutGrid.getContainer('networkBuilder');
            this.networkBuilder = new NetworkBuilder(
                networkContainer,
                this.cascadix,
                (networkData) => this.onNetworkChange(networkData)
            );

            // Set up trace callbacks
            this.networkBuilder.setTraceCallback((trace) => {
                this.smithChart.addTrace(trace);
                this.rlPlot.updateData(trace);
            });

            this.networkBuilder.setClearCallback(() => {
                this.smithChart.clearTraces();
                this.rlPlot.clearData();
            });

            // Initialize Plot Settings Panel with Tweakpane
            const settingsContainer = this.layoutGrid.getContainer('frequencyPanel');
            this.plotSettingsPanel = new PlotSettingsPanel(settingsContainer);

            // Set up analysis and settings callbacks
            this.plotSettingsPanel.setAnalyzeCallback((params) => {
                this.performAnalysis(params);
            });

            this.plotSettingsPanel.setSettingChangeCallback((category, settings) => {
                this.onSettingsChange(category, settings);
            });

            // Set up resize handler
            window.addEventListener('resize', () => {
                this.smithChart.resize();
                this.rlPlot.resize();
            });

            this.startRenderLoop();
            
            // Perform initial analysis with default settings
            setTimeout(() => {
                this.performAnalysis({
                    freqStart: 1e9,
                    freqStop: 3e9,
                    numPoints: 201
                });
            }, 500);

        } catch (err) {
            console.error('Failed to initialize:', err);
            alert('Failed to initialize: ' + err.message);
        }
    }

    onNetworkChange(networkData) {
        console.log('Network changed:', networkData);
        
        // Update display with current network configuration
        if (networkData.components.length > 0) {
            // Automatically run a frequency sweep to show the current network
            const params = {
                freqStart: 1e9,
                freqStop: 3e9,
                numPoints: 101
            };
            
            this.networkBuilder.runFrequencySweep(params);
        }
    }
    
    onSettingsChange(category, settings) {
        console.log(`Settings changed in ${category}:`, settings);
        
        // Handle different setting categories
        switch (category) {
            case 'display':
                this.updateDisplaySettings(settings);
                break;
            case 'analysis':
                // Analysis settings changed
                break;
            case 'frequency':
                // Frequency settings changed
                break;
        }
    }

    updateDisplaySettings(settings) {
        // Update Smith chart display settings
        if (settings.smithChart) {
            // Apply Smith chart settings
        }
        
        // Update Return Loss plot settings
        if (settings.returnLoss) {
            // Apply RL plot settings
        }
    }

    performAnalysis(params) {
        console.log('Main.performAnalysis called with params:', params);
        
        // Use the enhanced network builder for analysis
        this.networkBuilder.runFrequencySweep(params);
    }

    startRenderLoop() {
        const render = (timestamp) => {
            this.smithChart.render(timestamp);
            this.animationId = requestAnimationFrame(render);
        };
        this.animationId = requestAnimationFrame(render);
    }

    destroy() {
        if (this.animationId) {
            cancelAnimationFrame(this.animationId);
        }
    }
}

const app = new RFDesignTool();
app.initialize();