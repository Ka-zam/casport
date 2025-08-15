export class LayoutGrid {
    constructor(container) {
        this.container = container;
        this.panels = {
            rlPlot: null,
            smithChart: null,
            networkBuilder: null,
            frequencyPanel: null
        };
        
        this.createLayout();
    }

    createLayout() {
        this.container.innerHTML = `
            <div class="layout-grid">
                <!-- Tweakpane Settings in upper left corner -->
                <div class="settings-overlay" id="frequency-panel-container"></div>
                
                <!-- Resizable plots section -->
                <div class="plots-section">
                    <div class="panel panel-rl-plot resizable">
                        <div class="panel-header">
                            <h3>📈 Return Loss / VSWR</h3>
                            <div class="resize-handle resize-horizontal"></div>
                        </div>
                        <div class="panel-content" id="rl-plot-container"></div>
                    </div>
                    
                    <div class="panel panel-smith-chart resizable">
                        <div class="panel-header">
                            <h3>📊 Smith Chart</h3>
                            <div class="resize-handle resize-vertical"></div>
                        </div>
                        <div class="panel-content" id="smith-chart-container">
                            <canvas id="smith-chart"></canvas>
                        </div>
                    </div>
                </div>
                
                <!-- Network builder section -->
                <div class="network-section">
                    <div class="panel panel-network">
                        <div class="panel-header">
                            <h3>🔧 Network Builder</h3>
                        </div>
                        <div class="panel-content" id="network-builder-container"></div>
                    </div>
                </div>
            </div>
        `;
        
        this.addLayoutStyles();
    }

    addLayoutStyles() {
        if (document.getElementById('layout-grid-styles')) return;
        
        const styles = `
            .layout-grid {
                display: grid;
                grid-template-rows: 2fr 1fr;
                grid-template-columns: 1fr;
                gap: 1rem;
                height: 100vh;
                padding: 1rem;
                box-sizing: border-box;
                position: relative;
                grid-template-areas:
                    "plots"
                    "network";
            }
            
            .settings-overlay {
                position: absolute;
                top: 20px;
                left: 20px;
                z-index: 1000;
                background: rgba(255, 255, 255, 0.95);
                border-radius: 8px;
                box-shadow: 0 4px 12px rgba(0, 0, 0, 0.15);
                backdrop-filter: blur(10px);
                max-width: 300px;
                max-height: calc(100vh - 200px);
                overflow: auto;
            }
            
            .plots-section {
                grid-area: plots;
                display: grid;
                grid-template-columns: 1fr 1fr;
                gap: 1rem;
                position: relative;
            }
            
            .network-section {
                grid-area: network;
                display: flex;
                flex-direction: column;
            }
            
            .panel {
                background: white;
                border-radius: 8px;
                border: 1px solid #e0e0e0;
                display: flex;
                flex-direction: column;
                overflow: hidden;
                box-shadow: 0 2px 4px rgba(0,0,0,0.1);
            }
            
            .panel-header {
                background: #f5f5f5;
                padding: 0.75rem 1rem;
                border-bottom: 1px solid #e0e0e0;
                flex-shrink: 0;
            }
            
            .panel-header h3 {
                margin: 0;
                font-size: 0.9rem;
                font-weight: 600;
                color: #333;
            }
            
            .panel-content {
                flex: 1;
                padding: 0;
                overflow: auto;
                position: relative;
            }
            
            .resizable {
                resize: both;
                overflow: hidden;
                min-width: 300px;
                min-height: 250px;
            }
            
            .resize-handle {
                position: absolute;
                background: #ccc;
                transition: background-color 0.2s;
            }
            
            .resize-handle:hover {
                background: #4A90E2;
            }
            
            .resize-horizontal {
                right: 0;
                top: 50%;
                width: 4px;
                height: 40px;
                margin-top: -20px;
                cursor: ew-resize;
            }
            
            .resize-vertical {
                bottom: 0;
                left: 50%;
                width: 40px;
                height: 4px;
                margin-left: -20px;
                cursor: ns-resize;
            }
            
            /* Responsive layout for smaller screens */
            @media (max-width: 1200px) {
                .layout-grid {
                    grid-template-columns: 1fr;
                    grid-template-rows: auto auto auto;
                    grid-template-areas: 
                        "plots"
                        "settings"
                        "network";
                }
                
                .plots-section {
                    grid-template-columns: 1fr;
                    grid-template-rows: 1fr 1fr;
                }
            }
            
            @media (max-width: 768px) {
                .layout-grid {
                    padding: 0.5rem;
                    gap: 0.5rem;
                }
            }
        `;
        
        const styleSheet = document.createElement('style');
        styleSheet.id = 'layout-grid-styles';
        styleSheet.textContent = styles;
        document.head.appendChild(styleSheet);
    }

    getContainer(panelName) {
        switch(panelName) {
            case 'rlPlot':
                return document.getElementById('rl-plot-container');
            case 'smithChart':
                return document.getElementById('smith-chart-container');
            case 'networkBuilder':
                return document.getElementById('network-builder-container');
            case 'frequencyPanel':
                return document.getElementById('frequency-panel-container');
            default:
                return null;
        }
    }

    getCanvas() {
        return document.getElementById('smith-chart');
    }

    setPanelContent(panelName, content) {
        const container = this.getContainer(panelName);
        if (container && typeof content === 'string') {
            container.innerHTML = content;
        }
    }

    showLoading(panelName) {
        this.setPanelContent(panelName, '<div class="loading">Loading...</div>');
    }

    hideLoading(panelName) {
        const container = this.getContainer(panelName);
        if (container) {
            const loading = container.querySelector('.loading');
            if (loading) {
                loading.remove();
            }
        }
    }
}