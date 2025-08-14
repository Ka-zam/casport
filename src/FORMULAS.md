# Mathematical Formulas for Cascaded 2-Port Networks

## 1. ABCD Matrix (Chain Matrix) Definition

The ABCD matrix, also known as the chain matrix or transmission matrix, relates the input voltage and current to the output voltage and current of a 2-port network:

```
┌────┐   ┌─────┐   ┌────┐
│ V₁ │   │ A B │   │ V₂ │
│    │ = │     │ × │    │
│ I₁ │   │ C D │   │-I₂ │
└────┘   └─────┘   └────┘
```

Note the negative sign on I₂, which follows the convention that current flows into both ports.

## 2. Cascading Networks

When cascading two or more 2-port networks, the total ABCD matrix is the product of individual matrices:

```
[ABCD]ₜₒₜₐₗ = [ABCD]₁ × [ABCD]₂ × ... × [ABCD]ₙ
```

Matrix multiplication for two cascaded networks:
```
A_new = A₁·A₂ + B₁·C₂
B_new = A₁·B₂ + B₁·D₂
C_new = C₁·A₂ + D₁·C₂
D_new = C₁·B₂ + D₁·D₂
```

## 3. Elementary Component Matrices

### 3.1 Series Impedance
An impedance Z in series:
```
┌───┐   ┌───────┐
│ A B │   │ 1   Z │
│     │ = │       │
│ C D │   │ 0   1 │
└───┘   └───────┘
```

### 3.2 Shunt Admittance
An admittance Y to ground:
```
┌───┐   ┌───────┐
│ A B │   │ 1   0 │
│     │ = │       │
│ C D │   │ Y   1 │
└───┘   └───────┘
```

### 3.3 Ideal Transformer
Turns ratio n:1 (n = N₁/N₂):
```
┌───┐   ┌───────┐
│ A B │   │ n   0 │
│     │ = │       │
│ C D │   │ 0  1/n│
└───┘   └───────┘
```

### 3.4 Transmission Line
For a transmission line with:
- Length: l
- Characteristic impedance: Z₀
- Propagation constant: γ = α + jβ

```
┌───┐   ┌─────────────────────────┐
│ A B │   │ cosh(γl)    Z₀·sinh(γl) │
│     │ = │                         │
│ C D │   │ sinh(γl)/Z₀   cosh(γl)  │
└───┘   └─────────────────────────┘
```

For lossless line (α = 0, γ = jβ):
```
┌───┐   ┌─────────────────────────┐
│ A B │   │ cos(βl)    jZ₀·sin(βl)  │
│     │ = │                         │
│ C D │   │ jsin(βl)/Z₀   cos(βl)   │
└───┘   └─────────────────────────┘
```

where β = ω√(LC) = ω/v_p = 2π/λ

## 4. Frequency-Dependent Components

### 4.1 Series Components

**Series Resistor (R):**
```
Z = R
┌───┐   ┌───────┐
│ A B │   │ 1   R │
│     │ = │       │
│ C D │   │ 0   1 │
└───┘   └───────┘
```

**Series Inductor (L):**
```
Z = jωL
┌───┐   ┌─────────┐
│ A B │   │ 1   jωL │
│     │ = │         │
│ C D │   │ 0    1  │
└───┘   └─────────┘
```

**Series Capacitor (C):**
```
Z = 1/(jωC) = -j/(ωC)
┌───┐   ┌──────────────┐
│ A B │   │ 1   -j/(ωC) │
│     │ = │             │
│ C D │   │ 0      1    │
└───┘   └──────────────┘
```

### 4.2 Shunt Components

**Shunt Resistor (R):**
```
Y = 1/R = G
┌───┐   ┌───────┐
│ A B │   │ 1   0 │
│     │ = │       │
│ C D │   │ G   1 │
└───┘   └───────┘
```

**Shunt Inductor (L):**
```
Y = 1/(jωL) = -j/(ωL)
┌───┐   ┌──────────────┐
│ A B │   │ 1       0    │
│     │ = │              │
│ C D │   │ -j/(ωL)  1   │
└───┘   └──────────────┘
```

**Shunt Capacitor (C):**
```
Y = jωC
┌───┐   ┌─────────┐
│ A B │   │ 1    0  │
│     │ = │         │
│ C D │   │ jωC  1  │
└───┘   └─────────┘
```

## 5. Network Properties

### 5.1 Reciprocity
For reciprocal networks (no active components or non-reciprocal materials):
```
det(ABCD) = AD - BC = 1
```

### 5.2 Symmetry
For symmetric networks (identical when viewed from either port):
```
A = D
```

### 5.3 Lossless Networks
For lossless networks at a single frequency:
- A and D are purely real
- B and C are purely imaginary
- |det(ABCD)| = 1

## 6. Impedance Calculations

### 6.1 Input Impedance
With load impedance Z_L at port 2:
```
Z_in = (A·Z_L + B)/(C·Z_L + D)
```

### 6.2 Output Impedance
With source impedance Z_S at port 1:
```
Z_out = (D·Z_S + B)/(C·Z_S + A)
```

### 6.3 Characteristic Impedance
For symmetric networks (A = D):
```
Z_0 = √(B/C)
```

## 7. Parameter Conversions

### 7.1 ABCD to S-parameters
With reference impedance Z₀:

```
S₁₁ = (A + B/Z₀ - C·Z₀ - D)/(A + B/Z₀ + C·Z₀ + D)
S₁₂ = 2·(AD - BC)/(A + B/Z₀ + C·Z₀ + D)
S₂₁ = 2/(A + B/Z₀ + C·Z₀ + D)
S₂₂ = (-A + B/Z₀ - C·Z₀ + D)/(A + B/Z₀ + C·Z₀ + D)
```

For reciprocal networks (AD - BC = 1): S₁₂ = S₂₁

### 7.2 ABCD to Z-parameters
```
Z₁₁ = A/C
Z₁₂ = (AD - BC)/C = 1/C (for reciprocal)
Z₂₁ = 1/C
Z₂₂ = D/C
```

### 7.3 ABCD to Y-parameters
```
Y₁₁ = D/B
Y₁₂ = -(AD - BC)/B = -1/B (for reciprocal)
Y₂₁ = -1/B
Y₂₂ = A/B
```

### 7.4 S-parameters to ABCD
With reference impedance Z₀:

```
Δ = S₁₁·S₂₂ - S₁₂·S₂₁

A = ((1 + S₁₁)·(1 - S₂₂) + S₁₂·S₂₁)/(2·S₂₁)
B = Z₀·((1 + S₁₁)·(1 + S₂₂) - S₁₂·S₂₁)/(2·S₂₁)
C = ((1 - S₁₁)·(1 - S₂₂) - S₁₂·S₂₁)/(2·S₂₁·Z₀)
D = ((1 - S₁₁)·(1 + S₂₂) + S₁₂·S₂₁)/(2·S₂₁)
```

## 8. Parallel Connection

For two networks in parallel:

```
[Y]_total = [Y]₁ + [Y]₂
```

To combine ABCD matrices in parallel:
1. Convert both to Y-parameters
2. Add Y-parameter matrices
3. Convert result back to ABCD

## 9. Series Connection

For two networks in series:

```
[Z]_total = [Z]₁ + [Z]₂
```

To combine ABCD matrices in series:
1. Convert both to Z-parameters
2. Add Z-parameter matrices
3. Convert result back to ABCD

## 10. Common Network Examples

### 10.1 L-Network (Series-Shunt)
Series impedance Z₁ followed by shunt admittance Y₂:
```
┌───┐   ┌──────────────┐
│ A B │   │ 1+Z₁Y₂   Z₁ │
│     │ = │              │
│ C D │   │   Y₂      1  │
└───┘   └──────────────┘
```

### 10.2 T-Network
Series Z₁, shunt Y₂, series Z₃:
```
┌───┐   ┌────────────────────────────────┐
│ A B │   │ 1+Y₂Z₁      Z₁+Z₃+Y₂Z₁Z₃      │
│     │ = │                                │
│ C D │   │    Y₂           1+Y₂Z₃         │
└───┘   └────────────────────────────────┘
```

### 10.3 Π-Network
Shunt Y₁, series Z₂, shunt Y₃:
```
┌───┐   ┌────────────────────────────────┐
│ A B │   │ 1+Y₃Z₂           Z₂            │
│     │ = │                                │
│ C D │   │ Y₁+Y₃+Y₁Y₃Z₂    1+Y₁Z₂        │
└───┘   └────────────────────────────────┘
```

## 11. Voltage and Power Calculations

### 11.1 Voltage Transfer Function
```
H_v = V₂/V₁ = 1/(A + B/Z_L)
```

### 11.2 Current Transfer Function
```
H_i = I₂/I₁ = 1/(C·Z_L + D)
```

### 11.3 Power Gain (with matched loads)
```
G_p = |S₂₁|² = |2/(A + B/Z₀ + C·Z₀ + D)|²
```

### 11.4 Return Loss
```
RL = -20·log₁₀|S₁₁| dB
```

### 11.5 Insertion Loss
```
IL = -20·log₁₀|S₂₁| dB
```

### 11.6 VSWR (Voltage Standing Wave Ratio)
```
VSWR = (1 + |S₁₁|)/(1 - |S₁₁|)
```

## 12. Complex Number Operations in C++

Using `std::complex<double>`:

```cpp
#include <complex>
using namespace std;

complex<double> z(3.0, 4.0);  // 3 + 4j
double mag = abs(z);           // magnitude: 5.0
double phase = arg(z);         // phase in radians
complex<double> conj_z = conj(z);  // conjugate: 3 - 4j

// Hyperbolic functions for transmission lines
complex<double> gamma_l = complex<double>(0.1, 1.5);
complex<double> cosh_gl = cosh(gamma_l);
complex<double> sinh_gl = sinh(gamma_l);
```

## 13. Practical Considerations

### 13.1 Frequency Scaling
For frequency f in Hz:
```
ω = 2πf
```

### 13.2 Impedance Normalization
Often convenient to normalize to system impedance Z₀:
```
z = Z/Z₀  (normalized impedance)
y = Y·Z₀  (normalized admittance)
```

### 13.3 Decibel Conversions
```
Power (dB) = 10·log₁₀(P₂/P₁)
Voltage (dB) = 20·log₁₀(V₂/V₁)
```

### 13.4 Stability Considerations
For active networks, check stability using:
- Rollett's K-factor: K = (1 - |S₁₁|² - |S₂₂|² + |Δ|²)/(2|S₁₂||S₂₁|) > 1
- |Δ| < 1, where Δ = S₁₁·S₂₂ - S₁₂·S₂₁