struct Uniforms {
    resolution: vec2<f32>,
    zoom: f32,
    pan: vec2<f32>,
    time: f32,
    gridDensity: f32,
    admittanceMode: f32,
    // Add space for impedance points (up to 16 points)
    pointCount: f32,
    padding: vec3<f32>,  // Padding for alignment
    points: array<vec4<f32>, 16>,  // x, y, size, highlight
}

@group(0) @binding(0) var<uniform> uniforms: Uniforms;

struct VertexOutput {
    @builtin(position) position: vec4<f32>,
    @location(0) uv: vec2<f32>,
}

@vertex
fn vs_main(@builtin(vertex_index) vertexIndex: u32) -> VertexOutput {
    var pos = array<vec2<f32>, 6>(
        vec2<f32>(-1.0, -1.0),
        vec2<f32>( 1.0, -1.0),
        vec2<f32>( 1.0,  1.0),
        vec2<f32>(-1.0, -1.0),
        vec2<f32>( 1.0,  1.0),
        vec2<f32>(-1.0,  1.0)
    );

    var output: VertexOutput;
    output.position = vec4<f32>(pos[vertexIndex], 0.0, 1.0);
    output.uv = pos[vertexIndex];
    return output;
}

fn drawCircle(p: vec2<f32>, center: vec2<f32>, radius: f32, thickness: f32) -> f32 {
    let d = length(p - center) - radius;
    return smoothstep(thickness, 0.0, abs(d));
}

fn drawArc(p: vec2<f32>, center: vec2<f32>, radius: f32, startAngle: f32, endAngle: f32, thickness: f32) -> f32 {
    let toPoint = p - center;
    let dist = length(toPoint);
    let angle = atan2(toPoint.y, toPoint.x);
    
    let onRadius = abs(dist - radius) < thickness;
    var inAngleRange = false;
    
    if (startAngle <= endAngle) {
        inAngleRange = angle >= startAngle && angle <= endAngle;
    } else {
        inAngleRange = angle >= startAngle || angle <= endAngle;
    }
    
    return select(0.0, smoothstep(thickness, 0.0, abs(dist - radius)), onRadius && inAngleRange);
}

fn drawConstantResistanceCircle(p: vec2<f32>, r: f32, thickness: f32) -> f32 {
    let center = vec2<f32>(r / (r + 1.0), 0.0);
    let radius = 1.0 / (r + 1.0);
    return drawCircle(p, center, radius, thickness);
}

fn drawConstantReactanceCircle(p: vec2<f32>, x: f32, thickness: f32) -> f32 {
    if (abs(x) < 0.001) {
        return select(0.0, 1.0, abs(p.y) < thickness && p.x >= -1.0 && p.x <= 1.0);
    }
    
    let center = vec2<f32>(1.0, 1.0 / x);
    let radius = abs(1.0 / x);
    
    // Draw full circle but clip to unit circle
    let circleIntensity = drawCircle(p, center, radius, thickness);
    let insideUnit = step(length(p), 1.0 + thickness);
    
    return circleIntensity * insideUnit;
}

@fragment
fn fs_main(input: VertexOutput) -> @location(0) vec4<f32> {
    let aspectRatio = uniforms.resolution.x / uniforms.resolution.y;
    var coord = input.uv;
    coord.x *= aspectRatio;
    
    coord = (coord + uniforms.pan) * uniforms.zoom;
    
    let thickness = 0.0015 / uniforms.zoom;
    let gridThickness = thickness * 0.8 * uniforms.gridDensity;
    let majorThickness = thickness * 1.2 * uniforms.gridDensity;
    
    var intensity = 0.0;
    var majorIntensity = 0.0;
    
    // Unit circle (outer boundary) - thicker
    majorIntensity += drawCircle(coord, vec2<f32>(0.0, 0.0), 1.0, thickness * 2.0);
    
    // Constant resistance circles
    let resistanceValues = array<f32, 10>(0.0, 0.2, 0.5, 1.0, 2.0, 3.0, 5.0, 10.0, 20.0, 50.0);
    for (var i = 0; i < 10; i++) {
        let r = resistanceValues[i];
        if (r > 0.0) {
            // Make r=1.0 (50 ohm) circle slightly thicker
            if (abs(r - 1.0) < 0.01) {
                majorIntensity += drawConstantResistanceCircle(coord, r, majorThickness) * 0.8;
            } else {
                intensity += drawConstantResistanceCircle(coord, r, gridThickness) * 0.6;
            }
        }
    }
    
    // Constant reactance circles (full circles, not just arcs)
    let reactanceValues = array<f32, 14>(-10.0, -5.0, -2.0, -1.0, -0.5, -0.2, 0.2, 0.5, 1.0, 2.0, 5.0, 10.0, 20.0, 50.0);
    for (var i = 0; i < 14; i++) {
        let x = reactanceValues[i];
        intensity += drawConstantReactanceCircle(coord, x, gridThickness) * 0.5;
    }
    
    // Real axis (horizontal center line) - darker
    majorIntensity += select(0.0, 0.7, abs(coord.y) < thickness * 1.5 && coord.x >= -1.0 && coord.x <= 1.0);
    
    // Paper tone color scheme
    let paperColor = vec3<f32>(0.98, 0.96, 0.92);  // Warm paper tone
    let inkColor = vec3<f32>(0.1, 0.1, 0.12);      // Dark ink
    let majorInkColor = vec3<f32>(0.05, 0.05, 0.08); // Darker ink for major lines
    
    // Apply grid intensity
    var color = paperColor;
    color = mix(color, inkColor, intensity);
    color = mix(color, majorInkColor, majorIntensity);
    
    // Draw impedance points and connecting lines
    var pointIntensity = 0.0;
    var lineIntensity = 0.0;
    let pointCount = i32(uniforms.pointCount);
    
    if (pointCount > 0) {
        // Draw connecting lines between points
        for (var i = 0; i < pointCount - 1; i++) {
            let p1 = uniforms.points[i].xy;
            let p2 = uniforms.points[i + 1].xy;
            
            // Calculate distance from point to line segment
            let lineVec = p2 - p1;
            let t = clamp(dot(coord - p1, lineVec) / dot(lineVec, lineVec), 0.0, 1.0);
            let closest = p1 + t * lineVec;
            let dist = length(coord - closest);
            
            lineIntensity = max(lineIntensity, smoothstep(thickness * 2.0, 0.0, dist));
        }
        
        // Draw points
        for (var i = 0; i < pointCount; i++) {
            let point = uniforms.points[i].xy;
            let size = uniforms.points[i].z * thickness * 3.0;
            let highlight = uniforms.points[i].w;
            
            let dist = length(coord - point);
            var pointMask = smoothstep(size, 0.0, dist);
            
            // Add extra highlight for load point (last point)
            if (i == pointCount - 1) {
                pointMask = max(pointMask, smoothstep(size * 2.0, size, dist) * 0.5);
            }
            
            pointIntensity = max(pointIntensity, pointMask * (0.8 + 0.2 * highlight));
        }
    }
    
    // Apply impedance trace
    let traceColor = vec3<f32>(0.8, 0.2, 0.2);  // Red trace
    color = mix(color, traceColor, lineIntensity * 0.8);
    
    // Apply impedance points
    let pointColor = vec3<f32>(0.9, 0.1, 0.1);  // Bright red points
    color = mix(color, pointColor, pointIntensity);
    
    return vec4<f32>(color, 1.0);
}