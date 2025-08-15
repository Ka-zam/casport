export class RLPlot {
    constructor(container) {
        this.container = container;
        this.canvas = null;
        this.ctx = null;
        
        // Sample data for now
        this.frequencies = [];
        this.returnLoss = [];
        this.vswr = [];
        
        this.init();
    }

    init() {
        // Create canvas element directly
        this.canvas = document.createElement('canvas');
        this.canvas.id = 'rl-plot-canvas';
        this.canvas.style.width = '100%';
        this.canvas.style.height = '100%';
        this.canvas.style.display = 'block';
        
        // Clear container and append canvas
        this.container.innerHTML = '';
        this.container.appendChild(this.canvas);
        
        this.ctx = this.canvas.getContext('2d');
        
        // Ensure we have valid dimensions
        this.resize();
        // Don't generate sample data - wait for real data
        this.render();
        
        window.addEventListener('resize', () => this.resize());
    }

    resize() {
        const rect = this.container.getBoundingClientRect();
        // Ensure we have valid dimensions
        const width = Math.max(rect.width, 100);
        const height = Math.max(rect.height, 100);
        
        this.canvas.width = width;
        this.canvas.height = height;
        
        console.log('RL Plot resized:', width, 'x', height);
        this.render();
    }

    render() {
        if (!this.ctx) return;
        
        const { width, height } = this.canvas;
        const padding = { top: 40, right: 60, bottom: 60, left: 60 };
        const plotWidth = width - padding.left - padding.right;
        const plotHeight = height - padding.top - padding.bottom;
        
        // Clear canvas
        this.ctx.fillStyle = '#faf8f4';
        this.ctx.fillRect(0, 0, width, height);
        
        // Draw grid
        this.drawGrid(padding, plotWidth, plotHeight);
        
        // Draw RL curve (Return Loss is positive, higher is better)
        console.log('Drawing curve with', this.returnLoss.length, 'points, first 5:', this.returnLoss.slice(0, 5));
        this.drawCurve(this.returnLoss, padding, plotWidth, plotHeight, '#2a2a2e', 0, 60);
        
        // Draw axes labels
        this.drawLabels(padding, plotWidth, plotHeight);
    }

    drawGrid(padding, plotWidth, plotHeight) {
        this.ctx.strokeStyle = '#d0d0d0';
        this.ctx.lineWidth = 0.5;
        
        // Vertical grid lines
        for (let i = 0; i <= 10; i++) {
            const x = padding.left + (i * plotWidth / 10);
            this.ctx.beginPath();
            this.ctx.moveTo(x, padding.top);
            this.ctx.lineTo(x, padding.top + plotHeight);
            this.ctx.stroke();
        }
        
        // Horizontal grid lines
        for (let i = 0; i <= 6; i++) {
            const y = padding.top + (i * plotHeight / 6);
            this.ctx.beginPath();
            this.ctx.moveTo(padding.left, y);
            this.ctx.lineTo(padding.left + plotWidth, y);
            this.ctx.stroke();
        }
        
        // Draw axes
        this.ctx.strokeStyle = '#2a2a2e';
        this.ctx.lineWidth = 2;
        this.ctx.beginPath();
        this.ctx.moveTo(padding.left, padding.top);
        this.ctx.lineTo(padding.left, padding.top + plotHeight);
        this.ctx.lineTo(padding.left + plotWidth, padding.top + plotHeight);
        this.ctx.stroke();
    }

    drawCurve(data, padding, plotWidth, plotHeight, color, minY, maxY) {
        if (!data || data.length === 0) return;
        
        this.ctx.strokeStyle = color;
        this.ctx.lineWidth = 2;
        this.ctx.beginPath();
        
        let hasInfinity = false;
        
        for (let i = 0; i < data.length; i++) {
            const x = padding.left + (i * plotWidth / (data.length - 1));
            // Handle -Infinity and Infinity values
            let val = data[i];
            if (val === -Infinity) {
                val = minY;
                hasInfinity = true;
            }
            if (val === Infinity) val = maxY;
            
            const normalizedY = (val - minY) / (maxY - minY);
            const y = padding.top + plotHeight * (1 - normalizedY);
            
            if (i === 0) {
                this.ctx.moveTo(x, y);
            } else {
                this.ctx.lineTo(x, y);
            }
        }
        
        this.ctx.stroke();
        
        // If we have perfect match, show a dashed line at the top
        if (hasInfinity) {
            this.ctx.strokeStyle = '#8a8a9e';
            this.ctx.lineWidth = 1;
            this.ctx.setLineDash([5, 5]);
            this.ctx.beginPath();
            const y = padding.top;
            this.ctx.moveTo(padding.left, y);
            this.ctx.lineTo(padding.left + plotWidth, y);
            this.ctx.stroke();
            this.ctx.setLineDash([]);
            
            // Add text
            this.ctx.fillStyle = '#8a8a9e';
            this.ctx.font = '12px Courier New';
            this.ctx.textAlign = 'center';
            this.ctx.fillText('Perfect Match (∞ dB)', padding.left + plotWidth/2, y + 15);
        }
    }

    drawLabels(padding, plotWidth, plotHeight) {
        const { width, height } = this.canvas;
        
        this.ctx.fillStyle = '#2a2a2e';
        this.ctx.font = '12px Courier New';
        this.ctx.textAlign = 'center';
        
        // X-axis labels (frequency)
        for (let i = 0; i <= 10; i++) {
            const freq = 1.0 + (2.0 * i / 10);
            const x = padding.left + (i * plotWidth / 10);
            this.ctx.fillText(freq.toFixed(1), x, padding.top + plotHeight + 20);
        }
        
        // Y-axis labels (RL in dB - positive values, higher is better)
        this.ctx.textAlign = 'right';
        for (let i = 0; i <= 6; i++) {
            const rl = 60 * (1 - i / 6);  // 60 at top, 0 at bottom
            const y = padding.top + plotHeight * (i / 6);
            this.ctx.fillText(rl.toFixed(0), padding.left - 10, y + 5);
        }
        
        // Axis titles
        this.ctx.textAlign = 'center';
        this.ctx.fillText('Frequency (GHz)', padding.left + plotWidth / 2, height - 20);
        
        this.ctx.save();
        this.ctx.translate(20, padding.top + plotHeight / 2);
        this.ctx.rotate(-Math.PI / 2);
        this.ctx.fillText('Return Loss (dB)', 0, 0);
        this.ctx.restore();
    }

    updateData(frequencies, returnLoss, vswr) {
        console.log('RLPlot.updateData called with:', {
            frequencies: frequencies?.length || 0,
            returnLoss: returnLoss?.length || 0,
            vswr: vswr?.length || 0,
            sample: returnLoss?.slice(0, 5)
        });
        
        this.frequencies = frequencies || [];
        this.returnLoss = returnLoss || [];
        this.vswr = vswr || [];
        this.render();
    }
}