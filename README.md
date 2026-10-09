# S32K144W Automotive RTD Rainbow LED Engine

[![Target: NXP S32K144W](https://img.shields.io/badge/Target-NXP%20S32K144W-005a9c.svg)](https://www.nxp.com/products/processors-and-microcontrollers/arm-microcontrollers/s32k-automotive-mcus/s32k1-microcontrollers-for-general-purpose:S32K1)
[![Core: Arm Cortex-M4F](https://img.shields.io/badge/Core-Arm%C2%AE%20Cortex%C2%AE--M4F%20%40%2080MHz-blue.svg)](https://www.arm.com/products/silicon-ip-cpu/cortex-m/cortex-m4)
[![RTD: 3.0.0 (AUTOSAR 4.7)](https://img.shields.io/badge/RTD-3.0.0%20(AUTOSAR%C2%AE%204.7)-orange.svg)](https://www.nxp.com)
[![Toolchain: GCC 10.2](https://img.shields.io/badge/Toolchain-GCC%2010.2%20(0%20errors)-brightgreen.svg)](https://gcc.gnu.org)
[![EMC: CISPR 25 Class 5](https://img.shields.io/badge/EMC-CISPR%2025%20Spread--Spectrum-purple.svg)](https://www.iec.ch)
[![License: BSD-3-Clause](https://img.shields.io/badge/License-BSD--3--Clause-blue.svg)](LICENSE)

English | [繁體中文](README_zh.md)

---

An automotive production-grade Rainbow LED firmware engine developed for the **NXP Semiconductors® S32K144W Evaluation Board (`XS32K14WEVB-Q064`)**.

Built strictly upon official **NXP Real Time Drivers (RTD 3.0.0)** public APIs and **S32 Configuration Tools (MEX)**, this project demonstrates high-rate color streaming, optical white-balance matching, electromagnetic emission reduction, and closed-loop junction temperature compensation on an automotive microcontroller.

---

## Table of Contents

- [1. Overview](#1-overview)
- [2. Key Features](#2-key-features)
- [3. Hardware Setup & Pin Mapping](#3-hardware-setup--pin-mapping)
- [4. Theory of Operation](#4-theory-of-operation)
  - [4.1 Cree® CLP6C-FKB Photometric Normalization](#41-cree-clp6c-fkb-photometric-normalization)
  - [4.2 1.0 MHz Sigma-Delta Pulse Density Modulation (PDM)](#42-10-mhz-sigma-delta-pulse-density-modulation-pdm)
  - [4.3 Galois LFSR Spread-Spectrum Modulation](#43-galois-lfsr-spread-spectrum-modulation)
  - [4.4 Virtual RC Thermal Observer & Dynamic Droop Compensation](#44-virtual-rc-thermal-observer--dynamic-droop-compensation)
  - [4.5 Continuous 16-Bit Gamma 2.2 Interpolation](#45-continuous-16-bit-gamma-22-interpolation)
- [5. Repository Structure](#5-repository-structure)
- [6. Getting Started](#6-getting-started)
  - [6.1 Prerequisites](#61-prerequisites)
  - [6.2 Option A: Build and Debug via S32 Design Studio (IDE)](#62-option-a-build-and-debug-via-s32-design-studio-ide)
  - [6.3 Option B: Headless Build and Flash via Command Line (CLI)](#63-option-b-headless-build-and-flash-via-command-line-cli)
- [7. Configuration Options](#7-configuration-options)
- [8. Trademarks & Legal Disclaimers](#8-trademarks--legal-disclaimers)

---

## 1. Overview

Driving multi-color indicator or interior accent lighting in automotive environments presents three primary physical challenges:
1. **Electromagnetic Emissions (CISPR 25)**: Fixed-frequency pulse-width modulation (PWM) concentrates energy into sharp harmonic spikes that interfere with AM/FM broadcast bands and onboard RF transceivers.
2. **Thermal Droop & Color Shift**: Red AlInGaP LED dice degrade in luminous flux at approximately $-0.8\%/^\circ\text{C}$, whereas InGaN green/blue dice drop by only $-0.2\%/^\circ\text{C}$. Rising temperatures destroy calibrated white balance.
3. **Stroboscopic & Rolling-Shutter Artifacts**: Low-frequency PWM (< 2 kHz) creates visible flickering when recorded by high-frame-rate or rolling-shutter cameras (e.g., ADAS driver monitoring cameras, backup displays).

This project resolves these challenges by combining a **1.0 MHz first-order Sigma-Delta Pulse Density Modulation (PDM)** software core with a **32-bit Galois LFSR spread-spectrum dither**, an on-chip **ADC bandgap temperature observer**, and a real-time **virtual RC thermal observer**.

---

## 2. Key Features

* **100% Official NXP RTD Architecture**: Built entirely with AUTOSAR® 4.7 / RTD 3.0.0 public driver modules (`Clock_Ip`, `Port_Ci_Port_Ip`, `Ftm_Pwm_Ip`, `Gpio_Dio_Ip`, `OsIf`). Zero direct register hacks or unsupported private symbols.
* **Dual-Engine Operation (`CONFIG_ENGINE_MODE`)**:
  * **Mode 1 (`CONFIG_ENGINE_MODE = 1`) — Pure 3-Channel Synchronous 1.0 MHz PDM**: Drives Red, Green, and Blue concurrently via software PDM for identical propagation delay and zero phase jitter.
  * **Mode 0 (`CONFIG_ENGINE_MODE = 0`) — Hybrid FTM PWM + PDM**: Offloads Red and Blue to 16-bit FlexTimer (FTM) hardware PWM (1.22 kHz carrier) while handling Green via 1.0 MHz PDM.
* **Photometric Calibration**: Explicitly normalized for the on-board **Cree® LED PLCC6 RGB LED (`CLP6C-FKB`)** powered through 680Ω series resistors from 5.0V.
* **10 kHz Color Refresh Rate**: Transitions hue at 10,000 updates per second (100 µs frame slices) across 65,536 discrete hue steps ($0.0055^\circ$ angular resolution).
* **CISPR 25 Spread-Spectrum Dithering**: Micro-tick pseudorandom jitter smears discrete harmonic peaks across a continuous broadband noise floor.
* **Closed-Loop Virtual RC Thermal Droop Compensation**: Real-time junction temperature ($T_j$) estimation dynamically boosts red intensity with temperature to lock optical balance.
* **Power-On Self-Test (POST)**: Autonomous RGB sequencing (Red 300 ms $\to$ Green 300 ms $\to$ Blue 300 ms) upon reset before entering the continuous rainbow loop.

---

## 3. Hardware Setup & Pin Mapping

Verified against the official NXP schematic (`SPF-46873_b.pdf`) and PCB layout (`LAY-S32K14WEVB-Q064.pdf`):

| Signal / Color | MCU Port | 64-LQFP Pin | Series Resistor | Hardware Routing | Output Polarity |
| :--- | :---: | :---: | :---: | :--- | :---: |
| **RED** | `PTE7` | Pin 39 | `R789` (0Ω) | **FTM0_CH7** (ALT2) / **GPIO** | Active-High (NPN Buffer) |
| **GREEN (Default)** | `PTE0` | Pin 60 | `R846` (0Ω, Populated) | **GPIO** (1.0 MHz PDM) | Active-High (NPN Buffer) |
| **GREEN (Alt)** | `PTB12` | Pin 43 | `R787` (0Ω, DNP) | **FTM0_CH0** (ALT2) | Active-High (NPN Buffer) |
| **BLUE** | `PTD5` | Pin 24 | `R774` (0Ω, Populated) | **FTM2_CH3** (ALT2) / **GPIO** | Active-High (NPN Buffer) |

> **Hardware Compatibility Note**: Factory EVB units populate `R846` (`PTE0`) and leave `R787` (`PTB12`) unpopulated. The firmware dual-drives both pins simultaneously, ensuring full out-of-the-box operation on both unmodified and modified boards.

---

## 4. Theory of Operation

### 4.1 Cree® CLP6C-FKB Photometric Normalization

The evaluation board equips a **Cree® LED PLCC6 3-in-1 SMD LED (`CLP6C-FKB-CM1Q1H1BB7R3R3`)** driven by `P5V0` (5.0V) through 680Ω resistors (`R95`, `R96`, `R97`) and MMBT3904 NPN transistors ($V_{ce(sat)} \approx 0.1\text{V}$).

Due to semiconductor bandgap differences ($V_f$) and raw luminous intensity bins, driving each die with identical electrical duty results in an overwhelming green spike and an almost invisible blue channel:

$$I_f = \frac{V_{CC} - V_f - V_{ce(sat)}}{R_{series}}$$

| Channel | Die Material & Bin | Forward Voltage ($V_f$) | Operating Current ($I_f$) | Luminous Intensity | Calibration Gain ($K$) | Compensated Status |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: |
| **RED** | AlInGaP (M–N: 621 nm) | 2.0 V | 4.19 mA | ~157 mcd | **82.0%** (`53739` / 65535) | Photometrically matched |
| **GREEN** | InGaN (Q–R: 528 nm) | 3.2 V | 2.43 mA | ~200 mcd | **62.0%** (`40632` / 65535) | Attenuated to prevent green flare |
| **BLUE** | InGaN (H–J: 470 nm) | 3.2 V | 2.43 mA | ~46 mcd | **100.0%** (`65535` / 65535) | Full-scale reference |

### 4.2 1.0 MHz Sigma-Delta Pulse Density Modulation (PDM)

Traditional PWM clusters high states into a single continuous pulse per period, producing strong low-frequency harmonics. 

The software PDM engine runs at 1.0 MHz (1 µs micro-ticks). Each channel maintains a 16-bit error accumulator:

$$\text{accumulator} \leftarrow \text{accumulator} + \text{duty}_{16}$$

If an overflow occurs ($\ge 65536$), the output pin is asserted high and $65536$ is subtracted; otherwise, it is pulled low. This disperses photon emissions uniformly across time, shifting quantization noise to high frequencies where human eyes and rolling-shutter sensors naturally filter it out.

### 4.3 Galois LFSR Spread-Spectrum Modulation

To meet **CISPR 25 Class 5** limits for vehicle broadcast protection, the micro-tick loop executes a 32-bit Galois Linear Feedback Shift Register (LFSR) with characteristic polynomial $x^{32} + x^{31} + x^{29} + x + 1$ (`0xD0000001`):

```c
uint32 lsb = g_lfsr_state & 1U;
g_lfsr_state >>= 1U;
if (lsb) {
    g_lfsr_state ^= 0xD0000001UL;
}
```

The low-order bits inject pseudorandom timing micro-jitter into the delay loop, smearing discrete clock harmonics into a uniform broadband noise floor.

### 4.4 Virtual RC Thermal Observer & Dynamic Droop Compensation

AlInGaP red dice experience thermal droop of approximately $-0.8\%/^\circ\text{C}$ relative to $25^\circ\text{C}$. 

The firmware implements a real-time thermal observer:
1. **Ambient Telemetry**: Decimates and reads the MCU on-chip bandgap temperature sensor via ADC0 Channel 26, filtered through a 1st-order IIR low-pass filter.
2. **Joule Self-Heating Observer**: Computes instantaneous electrical power dissipation ($P = \sum I_k^2 \cdot R \cdot \text{duty}_k$) and feeds a virtual thermal RC model ($\tau \approx 1\text{ s}$, $R_{th} \approx 200^\circ\text{C/W}$).
3. **Closed-Loop Gain Adaptation**:
   $$T_j = T_{\text{mcu}} + \Delta T_{\text{self}}$$
   $$\text{Gain}_{\text{Red}}(T_j) = \text{Gain}_{\text{base}} + (T_j - 25^\circ\text{C}) \times 430$$

This boosts the red channel duty by up to $+21.8\%$ under elevated temperatures to maintain constant optical white balance.

### 4.5 Continuous 16-Bit Gamma 2.2 Interpolation

Human perception of light is non-linear ($\text{Perceived} \approx \text{Physical}^{1/2.2}$). The engine incorporates a 1025-point 16-bit calibration LUT with zero-division integer interpolation:

$$\text{Output} = \text{LUT}[idx] + \frac{(\text{LUT}[idx+1] - \text{LUT}[idx]) \times rem}{64}$$

Constrains luminance quantization error to $<0.005\%$ across the entire dynamic range.

---

## 5. Repository Structure

```
.
├── board/                         # Pin and clock initialization configs (MEX generated)
│   ├── Port_Ci_Port_Ip_Cfg.c
│   └── Port_Ci_Port_Ip_Cfg.h
├── generate/                      # RTD module drivers and configuration structures
│   ├── include/                   # Clock, FTM, OsIf configuration headers
│   └── src/                       # Clock_Ip_Cfg.c, Ftm_Pwm_Ip_VS_0_PBcfg.c, OsIf_Cfg.c
├── include/                       # Application headers
│   ├── check_example.h
│   └── gamma_lut_1025.h           # 1025-point 16-bit Gamma 2.2 lookup table
├── Project_Settings/
│   ├── Debugger/                  # PEMicro GDB launch configurations
│   ├── Linker_Files/              # S32K144W Flash & RAM linker scripts (.ld)
│   └── Startup_Code/              # CMSIS startup, vector table, NVIC configuration
├── RTD/                           # NXP Real Time Drivers source files (AUTOSAR 4.7)
├── src/
│   └── main.c                     # Application entry, PDM engine, thermal observer
├── tests/                         # Verification test scripts
├── Gpio_Dio_Ip_Example.mex        # S32 Configuration Tools configuration file
└── README.md                      # Project documentation
```

---

## 6. Getting Started

### 6.1 Prerequisites

* **Hardware**:
  * NXP `XS32K14WEVB-Q064` evaluation board
  * Micro-USB cable
* **Software**:
  * NXP S32 Design Studio for S32 Platform **v3.6.11**
  * NXP Real Time Drivers for S32K1 / S32M24 **RTD 3.0.0**
  * P&E Microcomputer Systems OpenSDA USB Drivers

---

### 6.2 Option A: Build and Debug via S32 Design Studio (IDE)

1. Launch S32 Design Studio.
2. Select **File $\to$ Import... $\to$ General $\to$ Existing Projects into Workspace**.
3. Browse to this repository root and click **Finish**.
4. In the Project Explorer, right-click the project and select **S32 Configuration Tool $\to$ Update Code** to generate the driver files.
5. Click **Project $\to$ Build Project** (or press `Ctrl+B`). Verify build output reports `0 errors, 0 warnings`.
6. Connect the board via USB (`J7`).
7. Open **Run $\to$ Debug Configurations...**, select `Gpio_Dio_Ip_Example_S32K144W_Debug_FLASH_PNE`, and click **Debug**.

---

### 6.3 Option B: Headless Build and Flash via Command Line (CLI)

The project can be built and flashed headlessly without launching the Eclipse GUI:

```powershell
# 1. Add S32DS build tools and compiler to PATH
$env:PATH = "C:\NXP\S32DS.3.6.11\S32DS\build_tools\msys32\usr\bin;C:\NXP\S32DS.3.6.11\S32DS\build_tools\gcc_v10.2\gcc-10.2-arm32-eabi\bin;" + $env:PATH

# 2. Invoke GNU Make in the build directory
cd Debug_FLASH
make -j8 all

# 3. Flash to target board using the headless PEMicro script
python ../tests/flash_target.py
```

Expected build size output:
```
   text    data     bss     dec     hex filename
  34612     412    5908   40932    9fe4 Gpio_Dio_Ip_Example_S32K144W.elf
```

---

## 7. Configuration Options

Primary operational parameters can be adjusted in [`src/main.c`](file:///c:/Users/b/workspaceS32DS.3.6.11/Gpio_Dio_Ip_Example_S32K144W/src/main.c):

| Configuration Macro | Default | Options | Description |
| :--- | :---: | :---: | :--- |
| `CONFIG_ENGINE_MODE` | `1U` | `0U`, `1U` | `1U`: Pure 3-Channel 1.0 MHz PDM.<br>`0U`: Hybrid 16-bit FTM PWM (Red/Blue) + 1.0 MHz PDM (Green). |
| `HUE_STEP_INCREMENT` | `1U` | `1U` ~ `64U` | Hue advance per 100 µs frame slice (controls rainbow cycle speed). |
| `LFSR_SEED_INITIAL` | `0x5A8E3B1CUL` | 32-bit uint | Initial state seed for the Galois LFSR spread-spectrum dither generator. |
| `RED_DROOP_COMP_SLOPE` | `430` | int | Gain compensation slope (+430 counts/°C) for AlInGaP thermal degradation. |
| `THERMAL_NOMINAL_TEMP_C`| `25` | int | Nominal calibration reference temperature in °C. |

---

## 8. Trademarks & Legal Disclaimers

* **NXP®**, **S32 Platform**, **S32 Design Studio®**, **S32 Configuration Tools (MEX)**, and **Real Time Drivers (RTD)** are registered trademarks or trademarks of **NXP Semiconductors N.V.** and its subsidiaries.
* **Arm®** and **Cortex®-M4F** are registered trademarks or trademarks of **Arm Limited** (or its subsidiaries) in the US and/or elsewhere.
* **PEMicro®**, **OpenSDA**, and **Multilink** are registered trademarks of **P&E Microcomputer Systems, Inc.**
* **Cree® LED** is a registered trademark of **CreeLED, Inc.**, a **SMART Global Holdings (SGH)** company.
* **AUTOSAR®** is a registered trademark of the **AUTOSAR Development Partnership**.
* **MISRA®** and **MISRA C®** are registered trademarks of **The MISRA Consortium Limited**.
* All other product or brand names are properties of their respective owners.
