import { Pane } from 'tweakpane';

export class PlotSettingsPanel {
    constructor(container) {
        this.container = container;
        this.settings = {
            // Frequency Analysis
            frequency: {
                startFreq: 1.0,  // GHz
                stopFreq: 3.0,   // GHz
                points: 201,
                sweepType: 'linear'
            },
            
            // Plot Display
            display: {
                smithChart: {
                    showGrid: true,
                    showLabels: true,
                    gridOpacity: 0.3,
                    backgroundColor: '#f8f9fa'
                },
                returnLoss: {
                    showVSWR: true,
                    showdB: true,
                    yAxisMax: 0,
                    yAxisMin: -40
                }
            },
            
            // Analysis Options
            analysis: {
                sweepType: 'frequency',
                monteCarloSamples: 1000,
                tolerance: 5.0,
                autoUpdate: true
            }
        };
        
        this.onAnalyze = null;
        this.onSettingChange = null;
        this.frequencyInfo = ''; // Initialize for Tweakpane binding
        this.init();
    }

    init() {
        // Create Tweakpane instance
        this.pane = new Pane({
            container: this.container,
            title: 'Plot Settings',
            expanded: true
        });

        this.setupFrequencyControls();
        this.setupDisplayControls();
        this.setupAnalysisControls();
        this.setupActionButtons();
    }

    setupFrequencyControls() {
        const freqFolder = this.pane.addFolder({
            title: '📊 Frequency Analysis',
            expanded: true
        });

        freqFolder.addBinding(this.settings.frequency, 'startFreq', {
            label: 'Start (GHz)',
            min: 0.1,
            max: 100,
            step: 0.1
        }).on('change', () => this.onFrequencyChange());

        freqFolder.addBinding(this.settings.frequency, 'stopFreq', {
            label: 'Stop (GHz)',
            min: 0.1,
            max: 100,
            step: 0.1
        }).on('change', () => this.onFrequencyChange());

        freqFolder.addBinding(this.settings.frequency, 'points', {
            label: 'Points',
            min: 11,
            max: 1001,
            step: 10
        }).on('change', () => this.onFrequencyChange());

        freqFolder.addBinding(this.settings.frequency, 'sweepType', {
            label: 'Sweep Type',
            options: {
                Linear: 'linear',
                Logarithmic: 'log'
            }
        }).on('change', () => this.onFrequencyChange());

        // Initialize frequency info first
        this.updateFrequencyInfo();
        
        // Add calculated info display
        this.freqInfo = freqFolder.addBinding(this, 'frequencyInfo', {
            label: 'Info',
            readonly: true,
            multiline: true,
            rows: 3
        });
    }

    setupDisplayControls() {
        const displayFolder = this.pane.addFolder({
            title: '🎨 Display Settings',
            expanded: false
        });

        // Smith Chart settings
        const smithFolder = displayFolder.addFolder({
            title: 'Smith Chart',
            expanded: true
        });

        smithFolder.addBinding(this.settings.display.smithChart, 'showGrid', {
            label: 'Show Grid'
        }).on('change', () => this.onDisplayChange());

        smithFolder.addBinding(this.settings.display.smithChart, 'showLabels', {
            label: 'Show Labels'
        }).on('change', () => this.onDisplayChange());

        smithFolder.addBinding(this.settings.display.smithChart, 'gridOpacity', {
            label: 'Grid Opacity',
            min: 0,
            max: 1,
            step: 0.1
        }).on('change', () => this.onDisplayChange());

        smithFolder.addBinding(this.settings.display.smithChart, 'backgroundColor', {
            label: 'Background',
            view: 'color'
        }).on('change', () => this.onDisplayChange());

        // Return Loss settings
        const rlFolder = displayFolder.addFolder({
            title: 'Return Loss Plot',
            expanded: true
        });

        rlFolder.addBinding(this.settings.display.returnLoss, 'showVSWR', {
            label: 'Show VSWR'
        }).on('change', () => this.onDisplayChange());

        rlFolder.addBinding(this.settings.display.returnLoss, 'showdB', {
            label: 'Show dB'
        }).on('change', () => this.onDisplayChange());

        rlFolder.addBinding(this.settings.display.returnLoss, 'yAxisMax', {
            label: 'Y Max (dB)',
            min: -10,
            max: 10,
            step: 1
        }).on('change', () => this.onDisplayChange());

        rlFolder.addBinding(this.settings.display.returnLoss, 'yAxisMin', {
            label: 'Y Min (dB)',
            min: -60,
            max: -10,
            step: 1
        }).on('change', () => this.onDisplayChange());
    }

    setupAnalysisControls() {
        const analysisFolder = this.pane.addFolder({
            title: '🔬 Analysis Options',
            expanded: false
        });

        analysisFolder.addBinding(this.settings.analysis, 'sweepType', {
            label: 'Analysis Type',
            options: {
                'Frequency Sweep': 'frequency',
                'Component Sweep': 'component',
                'Monte Carlo': 'montecarlo'
            }
        }).on('change', () => this.onAnalysisChange());

        analysisFolder.addBinding(this.settings.analysis, 'monteCarloSamples', {
            label: 'MC Samples',
            min: 100,
            max: 10000,
            step: 100
        }).on('change', () => this.onAnalysisChange());

        analysisFolder.addBinding(this.settings.analysis, 'tolerance', {
            label: 'Tolerance (%)',
            min: 0.1,
            max: 20,
            step: 0.1
        }).on('change', () => this.onAnalysisChange());

        analysisFolder.addBinding(this.settings.analysis, 'autoUpdate', {
            label: 'Auto Update'
        }).on('change', () => this.onAnalysisChange());
    }

    setupActionButtons() {
        const actions = {
            analyze: () => this.runAnalysis(),
            optimize: () => this.runOptimization(),
            export: () => this.exportSettings(),
            reset: () => this.resetSettings()
        };

        this.pane.addButton({
            title: '🔍 Analyze Network'
        }).on('click', actions.analyze);

        this.pane.addButton({
            title: '⚡ Optimize'
        }).on('click', actions.optimize);

        this.pane.addButton({
            title: '💾 Export Settings'
        }).on('click', actions.export);

        this.pane.addButton({
            title: '🔄 Reset to Defaults'
        }).on('click', actions.reset);
    }

    onFrequencyChange() {
        this.updateFrequencyInfo();
        if (this.settings.analysis.autoUpdate) {
            this.runAnalysis();
        }
        this.notifySettingChange('frequency', this.settings.frequency);
    }

    onDisplayChange() {
        this.notifySettingChange('display', this.settings.display);
    }

    onAnalysisChange() {
        this.notifySettingChange('analysis', this.settings.analysis);
    }

    updateFrequencyInfo() {
        const { startFreq, stopFreq, points } = this.settings.frequency;
        const center = (startFreq + stopFreq) / 2;
        const span = stopFreq - startFreq;
        const resolution = span / (points - 1) * 1000; // Convert to MHz
        
        this.frequencyInfo = `Center: ${center.toFixed(2)} GHz\nSpan: ${span.toFixed(2)} GHz\nRes: ${resolution.toFixed(1)} MHz`;
        
        // In Tweakpane 4.x, the binding updates automatically when the property changes
        if (this.pane) {
            this.pane.refresh();
        }
    }

    runAnalysis() {
        if (this.onAnalyze) {
            const analysisParams = {
                freqStart: this.settings.frequency.startFreq * 1e9,  // Convert to Hz
                freqStop: this.settings.frequency.stopFreq * 1e9,
                numPoints: this.settings.frequency.points,
                sweepType: this.settings.analysis.sweepType,
                monteCarloSamples: this.settings.analysis.monteCarloSamples,
                tolerance: this.settings.analysis.tolerance
            };
            
            this.onAnalyze(analysisParams);
        }
    }

    runOptimization() {
        console.log('Running optimization with settings:', this.settings);
        // Implement optimization logic here
    }

    exportSettings() {
        const settingsJson = JSON.stringify(this.settings, null, 2);
        const blob = new Blob([settingsJson], { type: 'application/json' });
        const url = URL.createObjectURL(blob);
        
        const a = document.createElement('a');
        a.href = url;
        a.download = 'plot-settings.json';
        document.body.appendChild(a);
        a.click();
        document.body.removeChild(a);
        URL.revokeObjectURL(url);
    }

    resetSettings() {
        // Reset to default values
        this.settings = {
            frequency: {
                startFreq: 1.0,
                stopFreq: 3.0,
                points: 201,
                sweepType: 'linear'
            },
            display: {
                smithChart: {
                    showGrid: true,
                    showLabels: true,
                    gridOpacity: 0.3,
                    backgroundColor: '#f8f9fa'
                },
                returnLoss: {
                    showVSWR: true,
                    showdB: true,
                    yAxisMax: 0,
                    yAxisMin: -40
                }
            },
            analysis: {
                sweepType: 'frequency',
                monteCarloSamples: 1000,
                tolerance: 5.0,
                autoUpdate: true
            }
        };

        // Refresh the pane
        this.pane.dispose();
        this.init();
    }

    notifySettingChange(category, settings) {
        if (this.onSettingChange) {
            this.onSettingChange(category, settings);
        }
    }

    setAnalyzeCallback(callback) {
        this.onAnalyze = callback;
    }

    setSettingChangeCallback(callback) {
        this.onSettingChange = callback;
    }

    getSettings() {
        return this.settings;
    }

    loadSettings(settings) {
        this.settings = { ...this.settings, ...settings };
        this.pane.dispose();
        this.init();
    }

    destroy() {
        if (this.pane) {
            this.pane.dispose();
        }
    }
}