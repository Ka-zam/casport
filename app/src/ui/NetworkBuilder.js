export class NetworkBuilder {
    constructor(container, cascadixWrapper, onNetworkChange = null) {
        this.container = container;
        this.cascadix = cascadixWrapper;
        this.onNetworkChange = onNetworkChange;
        
        this.components = [];
        this.connections = [];
        this.selectedComponent = null;
        this.draggedElement = null;
        this.ghostElement = null;
        this.snapToGrid = true;
        this.gridSize = 20;
        this.editingComponent = null;
        
        this.globalParams = {
            frequency: 2.4e9,  // 2.4 GHz default
            z0_system: 50.0,   // 50Ω system impedance
            z_load: 50.0       // 50Ω load impedance
        };
        
        this.createNetworkCanvas();
        this.setupPaletteDragDrop();
        this.setupCanvasInteractions();
        this.setupKeyboardShortcuts();
    }
    
    createNetworkCanvas() {
        this.container.innerHTML = `
            <div class="network-builder">
                <!-- Component Palette -->
                <div class="component-palette">
                    <div class="palette-item" draggable="true" data-type="series_resistor">
                        <div class="symbol">R</div>
                        <div class="label">Resistor</div>
                    </div>
                    <div class="palette-item" draggable="true" data-type="series_capacitor">
                        <div class="symbol">C</div>
                        <div class="label">Capacitor</div>
                    </div>
                    <div class="palette-item" draggable="true" data-type="series_inductor">
                        <div class="symbol">L</div>
                        <div class="label">Inductor</div>
                    </div>
                    <div class="palette-item" draggable="true" data-type="transmission_line">
                        <div class="symbol">TL</div>
                        <div class="label">T-Line</div>
                    </div>
                    <div class="palette-item" draggable="true" data-type="shunt_resistor">
                        <div class="symbol">R⟂</div>
                        <div class="label">Shunt R</div>
                    </div>
                    <div class="palette-item" draggable="true" data-type="shunt_capacitor">
                        <div class="symbol">C⟂</div>
                        <div class="label">Shunt C</div>
                    </div>
                    <div class="palette-item" draggable="true" data-type="shunt_inductor">
                        <div class="symbol">L⟂</div>
                        <div class="label">Shunt L</div>
                    </div>
                </div>
                
                <!-- Network Canvas -->
                <div class="network-canvas">
                    <div class="snap-grid"></div>
                    <div class="drop-zone"></div>
                </div>
                
                <!-- Value Editor Popup -->
                <div class="value-editor">
                    <h4 style="margin: 0 0 12px 0;">Component Properties</h4>
                    <input type="number" id="componentValue" placeholder="Value" step="any">
                    <input type="number" id="componentTolerance" placeholder="Tolerance (%)" value="5">
                    <button onclick="this.saveComponentValue()" class="save-btn">Save</button>
                    <button onclick="this.cancelEdit()" class="cancel-btn">Cancel</button>
                </div>
            </div>
        `;
        
        // Add CSS styles
        this.addNetworkBuilderStyles();
    }
    
    addNetworkBuilderStyles() {
        if (document.getElementById('network-builder-styles')) return;
        
        const styles = `
            .network-builder {
                position: relative;
                width: 100%;
                height: 100%;
                min-height: 400px;
                background: linear-gradient(to bottom, #f8f9fa, #e9ecef);
                border-radius: 12px;
                overflow: hidden;
                box-shadow: 0 4px 6px rgba(0, 0, 0, 0.1);
            }
            
            .component-palette {
                position: absolute;
                top: 20px;
                left: 20px;
                display: flex;
                flex-wrap: wrap;
                gap: 12px;
                background: white;
                padding: 12px;
                border-radius: 8px;
                box-shadow: 0 2px 12px rgba(0, 0, 0, 0.08);
                z-index: 100;
                max-width: calc(100% - 40px);
            }
            
            .palette-item {
                width: 60px;
                height: 60px;
                background: white;
                border: 2px solid #dee2e6;
                border-radius: 8px;
                cursor: grab;
                display: flex;
                flex-direction: column;
                align-items: center;
                justify-content: center;
                transition: all 0.2s cubic-bezier(0.4, 0, 0.2, 1);
                position: relative;
            }
            
            .palette-item:hover {
                transform: translateY(-2px);
                box-shadow: 0 4px 12px rgba(0, 0, 0, 0.15);
                border-color: #4A90E2;
            }
            
            .palette-item:active {
                cursor: grabbing;
                transform: scale(1.05);
            }
            
            .palette-item .symbol {
                font-size: 16px;
                font-weight: bold;
                color: #2c3e50;
            }
            
            .palette-item .label {
                font-size: 9px;
                color: #6c757d;
                margin-top: 2px;
            }
            
            .palette-item.dragging {
                opacity: 0.5;
            }
            
            .ghost-component {
                position: fixed;
                pointer-events: none;
                z-index: 1000;
                opacity: 0.8;
                transform: translate(-50%, -50%);
                transition: transform 0.1s ease-out;
            }
            
            .network-canvas {
                position: absolute;
                top: 120px;
                left: 20px;
                right: 20px;
                bottom: 20px;
                background: white;
                border-radius: 8px;
                box-shadow: inset 0 2px 4px rgba(0, 0, 0, 0.06);
                overflow: hidden;
            }
            
            .snap-grid {
                position: absolute;
                top: 0;
                left: 0;
                right: 0;
                bottom: 0;
                background-image: 
                    linear-gradient(rgba(74, 144, 226, 0.1) 1px, transparent 1px),
                    linear-gradient(90deg, rgba(74, 144, 226, 0.1) 1px, transparent 1px);
                background-size: 20px 20px;
                opacity: 0;
                transition: opacity 0.3s;
                pointer-events: none;
            }
            
            .snap-grid.visible {
                opacity: 1;
            }
            
            .drop-zone {
                position: absolute;
                border: 2px dashed #4A90E2;
                background: rgba(74, 144, 226, 0.05);
                border-radius: 8px;
                pointer-events: none;
                opacity: 0;
                transition: all 0.3s;
            }
            
            .drop-zone.active {
                opacity: 1;
                animation: breathe 2s infinite;
            }
            
            @keyframes breathe {
                0%, 100% { transform: scale(1); }
                50% { transform: scale(1.05); }
            }
            
            .network-component {
                position: absolute;
                width: 80px;
                height: 60px;
                background: white;
                border: 2px solid #4A90E2;
                border-radius: 8px;
                display: flex;
                flex-direction: column;
                align-items: center;
                justify-content: center;
                cursor: move;
                transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
                z-index: 10;
            }
            
            .network-component:hover {
                box-shadow: 0 6px 20px rgba(74, 144, 226, 0.3);
                transform: translateY(-1px);
            }
            
            .network-component.selected {
                border-color: #28a745;
                box-shadow: 0 0 0 3px rgba(40, 167, 69, 0.2);
            }
            
            .value-editor {
                position: absolute;
                background: white;
                border-radius: 8px;
                box-shadow: 0 4px 20px rgba(0, 0, 0, 0.15);
                padding: 16px;
                z-index: 200;
                min-width: 200px;
                opacity: 0;
                transform: scale(0.9) translateY(-10px);
                transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
                pointer-events: none;
            }
            
            .value-editor.visible {
                opacity: 1;
                transform: scale(1) translateY(0);
                pointer-events: all;
            }
            
            .value-editor input {
                width: 100%;
                padding: 8px;
                border: 1px solid #dee2e6;
                border-radius: 4px;
                margin: 4px 0;
                transition: border-color 0.2s;
                box-sizing: border-box;
            }
            
            .value-editor input:focus {
                outline: none;
                border-color: #4A90E2;
            }
            
            .value-editor button {
                width: 48%;
                padding: 8px;
                border: none;
                border-radius: 4px;
                cursor: pointer;
                margin: 8px 1% 0 1%;
            }
            
            .save-btn {
                background: #4A90E2;
                color: white;
            }
            
            .cancel-btn {
                background: #6c757d;
                color: white;
            }
            
            .network-component.deleting {
                animation: fadeOutScale 0.3s forwards;
            }
            
            @keyframes fadeOutScale {
                to {
                    opacity: 0;
                    transform: scale(0.8);
                }
            }
            
            @keyframes fadeInScale {
                from {
                    opacity: 0;
                    transform: scale(0.8);
                }
                to {
                    opacity: 1;
                    transform: scale(1);
                }
            }
        `;
        
        const styleSheet = document.createElement('style');
        styleSheet.id = 'network-builder-styles';
        styleSheet.textContent = styles;
        document.head.appendChild(styleSheet);
    }
    
    setupPaletteDragDrop() {
        const paletteItems = this.container.querySelectorAll('.palette-item');
        
        paletteItems.forEach(item => {
            // Drag start
            item.addEventListener('dragstart', (e) => {
                item.classList.add('dragging');
                
                // Create ghost element that follows cursor
                this.createGhostElement(item);
                
                // Store component type
                e.dataTransfer.effectAllowed = 'copy';
                e.dataTransfer.setData('componentType', item.dataset.type);
                
                // Custom drag image (invisible)
                const emptyImg = new Image();
                e.dataTransfer.setDragImage(emptyImg, 0, 0);
                
                // Show snap grid
                this.container.querySelector('.snap-grid').classList.add('visible');
            });
            
            // Drag end
            item.addEventListener('dragend', (e) => {
                item.classList.remove('dragging');
                this.removeGhostElement();
                this.container.querySelector('.snap-grid').classList.remove('visible');
            });
        });
        
        // Canvas drop zone
        const canvas = this.container.querySelector('.network-canvas');
        
        canvas.addEventListener('dragover', (e) => {
            e.preventDefault();
            e.dataTransfer.dropEffect = 'copy';
            
            // Update ghost position
            if (this.ghostElement) {
                const pos = this.getSnappedPosition(e.clientX, e.clientY);
                this.ghostElement.style.left = pos.x + 'px';
                this.ghostElement.style.top = pos.y + 'px';
            }
            
            // Show drop zone
            this.showDropZone(e);
        });
        
        canvas.addEventListener('drop', (e) => {
            e.preventDefault();
            
            const type = e.dataTransfer.getData('componentType');
            const rect = canvas.getBoundingClientRect();
            const pos = this.getSnappedPosition(
                e.clientX - rect.left,
                e.clientY - rect.top
            );
            
            this.addComponentAtPosition(type, pos);
            this.hideDropZone();
        });
        
        canvas.addEventListener('dragleave', () => {
            this.hideDropZone();
        });
    }
    
    addComponentAtPosition(type, position) {
        const defaults = {
            'series_resistor': { value: 50, unit: 'Ω', symbol: 'R' },
            'series_inductor': { value: 10e-9, unit: 'H', symbol: 'L' },
            'series_capacitor': { value: 1e-12, unit: 'F', symbol: 'C' },
            'shunt_resistor': { value: 50, unit: 'Ω', symbol: 'R⟂' },
            'shunt_inductor': { value: 10e-9, unit: 'H', symbol: 'L⟂' },
            'shunt_capacitor': { value: 1e-12, unit: 'F', symbol: 'C⟂' },
            'transmission_line': { value: 0.01, unit: 'm', symbol: 'TL' }
        };
        
        const defaultData = defaults[type] || { value: 1.0, unit: '', symbol: '?' };
        
        const component = document.createElement('div');
        component.className = 'network-component';
        component.dataset.type = type;
        component.dataset.id = Date.now(); // Simple ID
        
        // Component visual
        component.innerHTML = `
            <div class="symbol">${defaultData.symbol}</div>
            <div class="value">${this.formatValue(defaultData.value, defaultData.unit)}</div>
        `;
        
        // Position
        component.style.left = position.x + 'px';
        component.style.top = position.y + 'px';
        
        // Add to canvas
        this.container.querySelector('.network-canvas').appendChild(component);
        
        // Animate in
        component.style.animation = 'fadeInScale 0.3s';
        
        // Make draggable and editable
        this.makeComponentInteractive(component);
        
        // Store in array
        const componentData = {
            id: component.dataset.id,
            type: type,
            element: component,
            value: defaultData.value,
            tolerance: 5.0,
            unit: defaultData.unit,
            position: position,
            enabled: true
        };
        
        this.components.push(componentData);
        
        // Notify change
        this.updateNetwork();
    }
    
    formatValue(value, unit) {
        if (value >= 1e9) return `${(value/1e9).toFixed(1)}G${unit}`;
        if (value >= 1e6) return `${(value/1e6).toFixed(1)}M${unit}`;
        if (value >= 1e3) return `${(value/1e3).toFixed(1)}k${unit}`;
        if (value >= 1) return `${value.toFixed(1)}${unit}`;
        if (value >= 1e-3) return `${(value*1e3).toFixed(1)}m${unit}`;
        if (value >= 1e-6) return `${(value*1e6).toFixed(1)}μ${unit}`;
        if (value >= 1e-9) return `${(value*1e9).toFixed(1)}n${unit}`;
        if (value >= 1e-12) return `${(value*1e12).toFixed(1)}p${unit}`;
        return `${value.toExponential(1)}${unit}`;
    }
    
    makeComponentInteractive(component) {
        let isDragging = false;
        let startX, startY, initialX, initialY;
        
        component.addEventListener('mousedown', (e) => {
            isDragging = true;
            startX = e.clientX;
            startY = e.clientY;
            initialX = component.offsetLeft;
            initialY = component.offsetTop;
            
            component.classList.add('selected');
            component.style.zIndex = 1000;
            this.selectedComponent = component;
        });
        
        document.addEventListener('mousemove', (e) => {
            if (!isDragging) return;
            
            const dx = e.clientX - startX;
            const dy = e.clientY - startY;
            
            const pos = this.getSnappedPosition(
                initialX + dx,
                initialY + dy
            );
            
            component.style.left = pos.x + 'px';
            component.style.top = pos.y + 'px';
        });
        
        document.addEventListener('mouseup', () => {
            if (isDragging) {
                isDragging = false;
                component.style.zIndex = 10;
                this.updateComponentPosition(component);
            }
        });
        
        // Double-click to edit
        component.addEventListener('dblclick', (e) => {
            e.stopPropagation();
            this.editComponent(component);
        });
        
        // Right-click to delete
        component.addEventListener('contextmenu', (e) => {
            e.preventDefault();
            this.deleteComponent(component);
        });
    }
    
    setupCanvasInteractions() {
        // Click outside to deselect
        this.container.querySelector('.network-canvas').addEventListener('click', (e) => {
            if (e.target.classList.contains('network-canvas')) {
                this.deselectAll();
            }
        });
        
        // Save component value when enter is pressed
        const valueEditor = this.container.querySelector('.value-editor');
        valueEditor.addEventListener('keydown', (e) => {
            if (e.key === 'Enter') {
                this.saveComponentValue();
            }
            if (e.key === 'Escape') {
                this.cancelEdit();
            }
        });
        
        // Wire up save and cancel buttons
        const saveBtn = valueEditor.querySelector('.save-btn');
        const cancelBtn = valueEditor.querySelector('.cancel-btn');
        
        saveBtn.addEventListener('click', () => this.saveComponentValue());
        cancelBtn.addEventListener('click', () => this.cancelEdit());
    }
    
    setupKeyboardShortcuts() {
        document.addEventListener('keydown', (e) => {
            if (e.key === 'Delete' && this.selectedComponent) {
                this.deleteComponent(this.selectedComponent);
            }
            if (e.key === 'g' || e.key === 'G') {
                this.snapToGrid = !this.snapToGrid;
                this.container.querySelector('.snap-grid').classList.toggle('visible');
            }
        });
    }
    
    createGhostElement(originalElement) {
        this.ghostElement = originalElement.cloneNode(true);
        this.ghostElement.classList.add('ghost-component');
        document.body.appendChild(this.ghostElement);
        
        // Follow mouse
        const followMouse = (e) => {
            if (this.ghostElement) {
                this.ghostElement.style.left = e.clientX + 'px';
                this.ghostElement.style.top = e.clientY + 'px';
            }
        };
        
        document.addEventListener('dragover', followMouse);
        this.ghostElement._followMouse = followMouse;
    }
    
    removeGhostElement() {
        if (this.ghostElement) {
            if (this.ghostElement._followMouse) {
                document.removeEventListener('dragover', this.ghostElement._followMouse);
            }
            this.ghostElement.remove();
            this.ghostElement = null;
        }
    }
    
    getSnappedPosition(x, y) {
        if (this.snapToGrid) {
            return {
                x: Math.round(x / this.gridSize) * this.gridSize,
                y: Math.round(y / this.gridSize) * this.gridSize
            };
        }
        return { x, y };
    }
    
    showDropZone(e) {
        const dropZone = this.container.querySelector('.drop-zone');
        const canvas = this.container.querySelector('.network-canvas');
        const rect = canvas.getBoundingClientRect();
        
        const pos = this.getSnappedPosition(
            e.clientX - rect.left - 40,
            e.clientY - rect.top - 30
        );
        
        dropZone.style.left = pos.x + 'px';
        dropZone.style.top = pos.y + 'px';
        dropZone.style.width = '80px';
        dropZone.style.height = '60px';
        dropZone.classList.add('active');
    }
    
    hideDropZone() {
        this.container.querySelector('.drop-zone').classList.remove('active');
    }
    
    deselectAll() {
        this.container.querySelectorAll('.network-component').forEach(comp => {
            comp.classList.remove('selected');
        });
        this.selectedComponent = null;
    }
    
    editComponent(component) {
        const editor = this.container.querySelector('.value-editor');
        const rect = component.getBoundingClientRect();
        const containerRect = this.container.getBoundingClientRect();
        
        // Position editor near component
        editor.style.left = (rect.right - containerRect.left + 10) + 'px';
        editor.style.top = (rect.top - containerRect.top) + 'px';
        
        // Load current values
        const comp = this.components.find(c => c.element === component);
        if (comp) {
            editor.querySelector('#componentValue').value = comp.value;
            editor.querySelector('#componentTolerance').value = comp.tolerance;
        }
        
        // Show editor
        editor.classList.add('visible');
        
        // Store reference
        this.editingComponent = component;
        
        // Focus on value input
        setTimeout(() => {
            editor.querySelector('#componentValue').focus();
        }, 100);
    }
    
    saveComponentValue() {
        const valueInput = this.container.querySelector('#componentValue');
        const toleranceInput = this.container.querySelector('#componentTolerance');
        
        const value = parseFloat(valueInput.value);
        const tolerance = parseFloat(toleranceInput.value);
        
        if (isNaN(value) || value <= 0) {
            alert('Please enter a valid positive value');
            return;
        }
        
        if (isNaN(tolerance) || tolerance < 0) {
            alert('Please enter a valid tolerance percentage');
            return;
        }
        
        const comp = this.components.find(c => c.element === this.editingComponent);
        if (comp) {
            comp.value = value;
            comp.tolerance = tolerance;
            
            // Update display
            comp.element.querySelector('.value').textContent = 
                this.formatValue(value, comp.unit);
        }
        
        this.cancelEdit();
        this.updateNetwork();
    }
    
    cancelEdit() {
        this.container.querySelector('.value-editor').classList.remove('visible');
        this.editingComponent = null;
    }
    
    deleteComponent(component) {
        component.classList.add('deleting');
        
        setTimeout(() => {
            component.remove();
            this.components = this.components.filter(c => c.element !== component);
            this.updateNetwork();
        }, 300);
        
        if (this.selectedComponent === component) {
            this.selectedComponent = null;
        }
    }
    
    updateComponentPosition(component) {
        const comp = this.components.find(c => c.element === component);
        if (comp) {
            comp.position = {
                x: component.offsetLeft,
                y: component.offsetTop
            };
        }
    }
    
    updateNetwork() {
        if (this.onNetworkChange) {
            const networkData = {
                components: this.components.filter(comp => comp.enabled),
                globalParams: this.globalParams
            };
            this.onNetworkChange(networkData);
        }
    }
    
    // Analysis methods for backward compatibility
    async runFrequencySweep(params) {
        try {
            const networkBuilder = (frequency) => {
                return this.cascadix.buildNetworkAtFrequency(
                    this.components.filter(comp => comp.enabled),
                    frequency
                );
            };
            
            const points = await this.cascadix.generateFrequencySweep(
                networkBuilder,
                params.freqStart,
                params.freqStop,
                params.numPoints,
                this.globalParams.z_load,
                this.globalParams.z0_system
            );
            
            this.displayTrace(points, 'Frequency Sweep', '#00FF80');
            
        } catch (error) {
            console.error('Frequency sweep failed:', error);
        }
    }
    
    displayTrace(points, label, color, type = 'line') {
        // This would be called to update the Smith chart display
        if (this.onTraceGenerated) {
            this.onTraceGenerated({
                points: points,
                label: label,
                color: color,
                type: type
            });
        }
    }
    
    setTraceCallback(callback) {
        this.onTraceGenerated = callback;
    }
    
    setClearCallback(callback) {
        this.onTraceCleared = callback;
    }
    
    getNetworkConfig() {
        return {
            components: this.components.filter(comp => comp.enabled),
            globalParams: this.globalParams
        };
    }
    
    destroy() {
        // Remove event listeners and clean up
        if (this.ghostElement) {
            this.removeGhostElement();
        }
        
        // Remove styles
        const styles = document.getElementById('network-builder-styles');
        if (styles) {
            styles.remove();
        }
    }
}