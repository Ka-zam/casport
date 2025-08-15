export class FrequencyPanel {
    constructor(container) {
        this.container = container;
        this.settings = {
            startFreq: 1.0,  // GHz
            stopFreq: 3.0,   // GHz
            points: 201,
            sweepType: 'linear'
        };
        
        this.init();
    }

    init() {
        this.render();
        this.setupEventListeners();
    }

    render() {
        this.container.innerHTML = `
            <div class="frequency-panel">
                <div class="freq-settings">
                    <div class="setting-group">
                        <label for="start-freq">Start Frequency (GHz)</label>
                        <input type="number" id="start-freq" value="${this.settings.startFreq}" 
                               min="0.1" max="100" step="0.1">
                    </div>
                    
                    <div class="setting-group">
                        <label for="stop-freq">Stop Frequency (GHz)</label>
                        <input type="number" id="stop-freq" value="${this.settings.stopFreq}" 
                               min="0.1" max="100" step="0.1">
                    </div>
                    
                    <div class="setting-group">
                        <label for="num-points">Number of Points</label>
                        <input type="number" id="num-points" value="${this.settings.points}" 
                               min="11" max="1001" step="10">
                    </div>
                    
                    <div class="setting-group">
                        <label for="sweep-type">Sweep Type</label>
                        <select id="sweep-type">
                            <option value="linear" selected>Linear</option>
                            <option value="log">Logarithmic</option>
                        </select>
                    </div>
                </div>
                
                <div class="analysis-controls">
                    <button id="analyze-btn" class="primary-btn">🔍 Analyze</button>
                    <button id="optimize-btn" class="secondary-btn">⚡ Optimize</button>
                </div>
                
                <div class="freq-info">
                    <div class="info-row">
                        <span class="info-label">Center:</span>
                        <span class="info-value" id="center-freq">2.0 GHz</span>
                    </div>
                    <div class="info-row">
                        <span class="info-label">Span:</span>
                        <span class="info-value" id="freq-span">2.0 GHz</span>
                    </div>
                    <div class="info-row">
                        <span class="info-label">Resolution:</span>
                        <span class="info-value" id="freq-res">10 MHz</span>
                    </div>
                </div>
                
                <div class="marker-controls">
                    <h4>Markers</h4>
                    <div class="marker-list">
                        <div class="marker-item">
                            <input type="checkbox" id="marker1" checked>
                            <label for="marker1">M1: 2.0 GHz</label>
                        </div>
                    </div>
                    <button id="add-marker-btn" class="small-btn">+ Add Marker</button>
                </div>
            </div>
        `;
        
        this.addStyles();
    }

    addStyles() {
        const style = document.createElement('style');
        style.textContent = `
            .frequency-panel {
                padding: 1rem;
                display: flex;
                flex-direction: column;
                gap: 1.5rem;
            }
            
            .freq-settings {
                display: grid;
                grid-template-columns: 1fr 1fr;
                gap: 1rem;
            }
            
            .setting-group {
                display: flex;
                flex-direction: column;
                gap: 0.25rem;
            }
            
            .setting-group label {
                font-size: 0.85rem;
                color: #4a4a5e;
                font-weight: bold;
            }
            
            .setting-group input,
            .setting-group select {
                padding: 0.5rem;
                border: 1px solid #2a2a2e;
                border-radius: 3px;
                background: #fff;
                font-family: monospace;
                font-size: 1rem;
            }
            
            .analysis-controls {
                display: flex;
                gap: 1rem;
            }
            
            .primary-btn {
                flex: 1;
                padding: 0.75rem;
                background: #2a2a2e;
                color: #faf8f4;
                border: none;
                border-radius: 4px;
                font-weight: bold;
                cursor: pointer;
                font-size: 1rem;
            }
            
            .primary-btn:hover {
                background: #3a3a3e;
            }
            
            .secondary-btn {
                flex: 1;
                padding: 0.75rem;
                background: #f0ece4;
                color: #2a2a2e;
                border: 2px solid #2a2a2e;
                border-radius: 4px;
                font-weight: bold;
                cursor: pointer;
                font-size: 1rem;
            }
            
            .secondary-btn:hover {
                background: #e0dcd4;
            }
            
            .freq-info {
                background: #f0ece4;
                padding: 1rem;
                border-radius: 4px;
            }
            
            .info-row {
                display: flex;
                justify-content: space-between;
                margin-bottom: 0.5rem;
            }
            
            .info-row:last-child {
                margin-bottom: 0;
            }
            
            .marker-controls {
                background: #f5f2ed;
                padding: 1rem;
                border-radius: 4px;
            }
            
            .marker-controls h4 {
                margin: 0 0 0.75rem 0;
                color: #2a2a2e;
                font-size: 0.9rem;
            }
            
            .marker-list {
                display: flex;
                flex-direction: column;
                gap: 0.5rem;
                margin-bottom: 0.75rem;
            }
            
            .marker-item {
                display: flex;
                align-items: center;
                gap: 0.5rem;
            }
            
            .small-btn {
                padding: 0.4rem 0.8rem;
                background: #fff;
                color: #2a2a2e;
                border: 1px solid #2a2a2e;
                border-radius: 3px;
                cursor: pointer;
                font-size: 0.85rem;
            }
            
            .small-btn:hover {
                background: #f0ece4;
            }
        `;
        document.head.appendChild(style);
    }

    setupEventListeners() {
        const startFreq = document.getElementById('start-freq');
        const stopFreq = document.getElementById('stop-freq');
        const numPoints = document.getElementById('num-points');
        
        [startFreq, stopFreq, numPoints].forEach(input => {
            input.addEventListener('input', () => this.updateInfo());
        });
        
        const analyzeBtn = document.getElementById('analyze-btn');
        analyzeBtn.addEventListener('click', () => this.analyze());
        
        this.updateInfo();
    }

    updateInfo() {
        const start = parseFloat(document.getElementById('start-freq').value);
        const stop = parseFloat(document.getElementById('stop-freq').value);
        const points = parseInt(document.getElementById('num-points').value);
        
        const center = (start + stop) / 2;
        const span = stop - start;
        const resolution = span / (points - 1) * 1000; // Convert to MHz
        
        document.getElementById('center-freq').textContent = `${center.toFixed(2)} GHz`;
        document.getElementById('freq-span').textContent = `${span.toFixed(2)} GHz`;
        document.getElementById('freq-res').textContent = `${resolution.toFixed(1)} MHz`;
    }

    analyze() {
        const start = parseFloat(document.getElementById('start-freq').value);
        const stop = parseFloat(document.getElementById('stop-freq').value);
        const points = parseInt(document.getElementById('num-points').value);
        
        this.settings = { startFreq: start, stopFreq: stop, points };
        
        // Trigger analysis event
        if (this.onAnalyze) {
            this.onAnalyze(this.settings);
        }
    }

    setAnalyzeCallback(callback) {
        this.onAnalyze = callback;
    }
}