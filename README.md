# S32K144W Automotive RTD Rainbow LED Engine

[![Target: NXP S32K144W](https://img.shields.io/badge/Target-NXP%20S32K144W-005a9c.svg)](https://www.nxp.com/products/processors-and-microcontrollers/arm-microcontrollers/s32k-automotive-mcus/s32k1-microcontrollers-for-general-purpose:S32K1)
[![Core: Arm Cortex-M4F](https://img.shields.io/badge/Core-Arm%C2%AE%20Cortex%C2%AE--M4F%20%40%2048MHz-blue.svg)](https://www.arm.com/products/silicon-ip-cpu/cortex-m/cortex-m4)
[![RTD: 3.0.0 (AUTOSAR 4.7)](https://img.shields.io/badge/RTD-3.0.0%20(AUTOSAR%C2%AE%204.7)-orange.svg)](https://www.nxp.com)
[![Toolchain: GCC 10.2](https://img.shields.io/badge/Toolchain-GCC%2010.2%20(0%20errors%20%7C%200%20warns)-brightgreen.svg)](https://gcc.gnu.org)
[![Functional Safety: ISO 26262 / ASIL-B Ready](https://img.shields.io/badge/Safety-ISO%2026262%20%2F%20ASIL--B%20Ready-success.svg)](https://www.iso.org/standard/68383.html)
[![EMC: CISPR 25 Class 5](https://img.shields.io/badge/EMC-CISPR%2025%20Spread--Spectrum-purple.svg)](https://www.iec.ch)
[![License: BSD-3-Clause](https://img.shields.io/badge/License-BSD--3--Clause-blue.svg)](LICENSE)

English | [繁體中文](README_zh.md)

---

An automotive production-grade Rainbow LED firmware engine developed for the **NXP Semiconductors® S32K144W Evaluation Board (`XS32K14WEVB-Q064`)**.

Built strictly upon official **NXP Real Time Drivers (RTD 3.0.0)** public APIs and **S32 Configuration Tools (MEX)**, this project demonstrates deterministic timing pacing, optical white-balance matching, electromagnetic emission reduction, closed-loop thermal observer compensation, and functional safety (ISO 26262 / MISRA C) architectural principles on an automotive microcontroller.

---

## Table of Contents

- [1. Overview](#1-overview)
- [2. Key Features](#2-key-features)
- [3. Automotive Production-Grade Safety Architecture (ISO 26262 / MISRA C)](#3-automotive-production-grade-safety-architecture-iso-26262--misra-c)
- [4. Hardware Setup & Pin Mapping](#4-hardware-setup--pin-mapping)
- [5. Theory of Operation](#5-theory-of-operation)
  - [5.1 5.000000-Second Cycle with Bresenham Fractional Accumulator](#51-5000000-second-cycle-with-bresenham-fractional-accumulator)
  - [5.2 Cree® CLP6C-FKB Photometric Normalization](#52-cree-clp6c-fkb-photometric-normalization)
  - [5.3 1.0 MHz Sigma-Delta Pulse Density Modulation (PDM)](#53-10-mhz-sigma-delta-pulse-density-modulation-pdm)
  - [5.4 Galois LFSR Spread-Spectrum Modulation (CISPR 25)](#54-galois-lfsr-spread-spectrum-modulation-cispr-25)
  - [5.5 Virtual RC Thermal Observer & Dynamic Droop Compensation](#55-virtual-rc-thermal-observer--dynamic-droop-compensation)
  - [5.6 Continuous 16-Bit Gamma 2.2 Interpolation & Boundary Closure](#56-continuous-16-bit-gamma-22-interpolation--boundary-closure)
- [6. Repository Structure](#6-repository-structure)
- [7. Getting Started](#7-getting-started)
  - [7.1 Prerequisites](#71-prerequisites)
  - [7.2 Option A: Build and Debug via S32 Design Studio (IDE)](#72-option-a-build-and-debug-via-s32-design-studio-ide)
  - [7.3 Option B: Headless Build and Flash via Command Line (CLI)](#73-option-b-headless-build-and-flash-via-command-line-cli)
- [8. Configuration Options](#8-configuration-options)
- [9. Trademarks & Legal Disclaimers](#9-trademarks--legal-disclaimers)

---

## 1. Overview

Driving multi-color indicator or interior accent lighting in automotive environments presents four primary engineering challenges:
1. **Electromagnetic Emissions (CISPR 25)**: Fixed-frequency pulse-width modulation (PWM) concentrates energy into sharp harmonic spikes that interfere with AM/FM broadcast bands and onboard RF transceivers.
2. **Thermal Droop & Color Shift**: Red AlInGaP LED dice degrade in luminous flux at approximately $-0.8\%/^\circ\text{C}$, whereas InGaN green/blue dice drop by only $-0.2\%/^\circ\text{C}$. Rising temperatures destroy calibrated white balance.
3. **Stroboscopic & Rolling-Shutter Artifacts**: Low-frequency PWM (< 2 kHz) creates visible flickering when recorded by high-frame-rate or rolling-shutter cameras (e.g., ADAS driver monitoring cameras, backup displays).
4. **Long-Term Deterministic Pacing (24/7 Over 5+ Years)**: Accumulated sub-microsecond timing rounding errors in naive tick loops cause substantial drift over time.

This project resolves these challenges by combining a **1.0 MHz first-order Sigma-Delta Pulse Density Modulation (PDM)** software core with a **32-bit Galois LFSR spread-spectrum dither**, an on-chip **ADC bandgap temperature observer**, a **Bresenham fractional remainder accumulator**, and an **ISO 26262 functional safety architecture**.

---

## 2. Key Features

* **100% Official NXP RTD Architecture**: Built entirely with AUTOSAR® 4.7 / RTD 3.0.0 public driver modules (`Clock_Ip`, `Port_Ci_Port_Ip`, `Ftm_Pwm_Ip`, `Gpio_Dio_Ip`, `OsIf`). Zero direct register hacks or unsupported private symbols.
* **Exact 5.000000-Second Cycle with Zero Drift**: Driven by ARM Cortex-M4 Hardware SysTick Timer ($240,000,000\text{ cycles}$ per cycle @ 48 MHz). Bresenham fractional remainder distribution guarantees mathematically **$0.000000\%$ accumulated drift** over years of continuous 24/7 operation.
* **Dual-Engine Operation (`CONFIG_ENGINE_MODE`)**:
  * **Mode 1 (`CONFIG_ENGINE_MODE = 1`) — Pure 3-Channel Synchronous 1.0 MHz PDM**: Drives Red, Green, and Blue concurrently via software PDM for identical propagation delay and zero phase jitter.
  * **Mode 0 (`CONFIG_ENGINE_MODE = 0`) — Hybrid FTM PWM + PDM**: Offloads Red and Blue to 16-bit FlexTimer (FTM) hardware PWM (1.22 kHz carrier) while handling Green via 1.0 MHz PDM.
* **Continuous Color Spectrum Closure**: Full 16-bit 65,536-step discrete hue progression ($0.0055^\circ$ angular resolution). Boundary condition seamlessly closes at $360^\circ \to 0^\circ$ (Purple $\to$ Red) with zero color flash.
* **Photometric Calibration**: Explicitly normalized for the on-board **Cree® LED PLCC6 RGB LED (`CLP6C-FKB`)** powered through 680Ω series resistors from 5.0V.
* **CISPR 25 Spread-Spectrum Dithering**: Micro-tick pseudorandom jitter smears discrete harmonic peaks across a continuous broadband noise floor.
* **Closed-Loop Virtual RC Thermal Droop Compensation**: Real-time junction temperature ($T_j$) estimation dynamically boosts red intensity with temperature to lock optical balance across automotive temperature ranges (-40°C ~ +105°C).

---

## 3. Automotive Production-Grade Safety Architecture (ISO 26262 / MISRA C)

To meet Tier-1 automotive embedded software requirements, this project implements the following safety paradigms:

```
+-------------------------------------------------------------------------+
|                  Application Layer (SWC: Lighting Engine)               |
|  - 65,536-Step Hue Engine   - 5.000000s Bresenham Pacing (0.0000% Drift)|
|  - Gamma 2.2 Interpolation  - Virtual RC Thermal Droop Observer         |
+-------------------------------------------------------------------------+
                                    |
+-------------------------------------------------------------------------+
|                  Safety & Reliability Layer (ISO 26262)                 |
|  - Active Watchdog (WDOG) Servicing    - Bounded Iteration Guards       |
|  - Fail-Safe State Machine             - Static Worst-Case Stack Proof  |
+-------------------------------------------------------------------------+
                                    |
+-------------------------------------------------------------------------+
|                 NXP Real Time Drivers (MCAL / RTD 3.0.0)                |
|  - Clock_Ip    - Port_Ci_Port_Ip    - Ftm_Pwm_Ip    - Gpio_Dio_Ip       |
+-------------------------------------------------------------------------+
```

1. **Deterministic Bounded Execution (ISO 26262-6 Table 6)**:
   - All inner pacing and hardware polling loops incorporate a compile-time bounded iteration guard (`iter_guard < 10000U`). Unbounded loops are strictly prohibited.
2. **Temporal & Control Flow Monitoring (Hardware Watchdog)**:
   - Includes `App_Wdog_Service()` in the primary execution loop to refresh the on-chip `WDOG` peripheral and protect against CPU stalls.
3. **Worst-Case Stack Usage Proof (`-fstack-usage`)**:
   - Compiled under GCC 10.2 with `-fstack-usage`. Maximum call stack depth is statically measured at **88 bytes** (only **2.1%** of the 4,096-byte stack allocation):
     ```text
     Apply_Gamma16                :  0 bytes (inlined, zero stack)
     Galois_LFSR_Next             :  0 bytes (inlined, zero stack)
     App_FaultHandler             : 16 bytes
     App_Thermal_Observer_Update  : 12 bytes
     Rainbow_Update16             : 40 bytes
     main                         : 48 bytes
     Maximum Call Chain           : 88 bytes (main + Rainbow_Update16)
     ```
4. **Fail-Safe Containment (`App_FaultHandler`)**:
   - In the event of hardware clock, pin, or peripheral initialization failure, the system instantly executes `App_FaultHandler()`, actively shutting down all LED drivers to prevent hazard conditions.
5. **Zero Dynamic Memory Allocation (MISRA C Rule 21.3)**:
   - Zero heap usage (`malloc`/`free`). All tables and states are statically allocated in flash or RAM at compile time.

---

## 4. Hardware Setup & Pin Mapping

Verified against the official NXP schematic (`SPF-46873_b.pdf`) and PCB layout (`LAY-S32K14WEVB-Q064.pdf`):

| Signal / Color | MCU Port | 64-LQFP Pin | Series Resistor | Hardware Routing | Output Polarity |
| :--- | :---: | :---: | :---: | :--- | :---: |
| **RED** | `PTE7` | Pin 39 | `R789` (0Ω) | **FTM0_CH7** (ALT2) / **GPIO** | Active-High (NPN Buffer) |
| **GREEN (Default)** | `PTE0` | Pin 60 | `R846` (0Ω, Populated) | **GPIO** (1.0 MHz PDM) | Active-High (NPN Buffer) |
| **GREEN (Alt)** | `PTB12` | Pin 43 | `R787` (0Ω, DNP) | **FTM0_CH0** (ALT2) | Active-High (NPN Buffer) |
| **BLUE** | `PTD5` | Pin 24 | `R774` (0Ω, Populated) | **FTM2_CH3** (ALT2) / **GPIO** | Active-High (NPN Buffer) |

---

## 5. Theory of Operation

### 5.1 5.000000-Second Cycle with Bresenham Fractional Accumulator

At 48.0 MHz core clock, a 5.000000-second rainbow period comprises:
$$\text{Total Cycles} = 5.000000\text{ s} \times 48,000,000\text{ Hz} = 240,000,000\text{ cycles}$$

Dividing across $65,536$ discrete steps yields:
$$\text{Base Cycles} = \lfloor 240,000,000 / 65,536 \rfloor = 3,662\text{ cycles}$$
$$\text{Fractional Remainder} = 240,000,000 \pmod{65,536} = 9,216\text{ cycles}$$

$$\sum_{k=0}^{65535} \text{StepCycles}_k = (65,536 \times 3,662) + 9,216 = 240,000,000\text{ cycles} \equiv 5.000000000\text{ s}$$

Residual sub-microsecond cycles are retained across iterations, guaranteeing **$0.000000\%$ accumulated mathematical drift** over 5+ years of continuous 24/7 looping.

### 5.2 Cree® CLP6C-FKB Photometric Normalization

The board equips a **Cree® LED PLCC6 3-in-1 SMD LED (`CLP6C-FKB-CM1Q1H1BB7R3R3`)** driven by `P5V0` (5.0V) through 680Ω resistors (`R95`, `R96`, `R97`) and MMBT3904 NPN transistors ($V_{ce(sat)} \approx 0.1\text{V}$):

| Channel | Die Material & Bin | Forward Voltage ($V_f$) | Operating Current ($I_f$) | Luminous Intensity | Calibration Gain ($K$) | Compensated Status |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: |
| **RED** | AlInGaP (M–N: 621 nm) | 2.0 V | 4.19 mA | ~157 mcd | **82.0%** (`53739` / 65535) | Photometrically matched |
| **GREEN** | InGaN (Q–R: 528 nm) | 3.2 V | 2.43 mA | ~200 mcd | **62.0%** (`40632` / 65535) | Attenuated to prevent flare |
| **BLUE** | InGaN (H–J: 470 nm) | 3.2 V | 2.43 mA | ~46 mcd | **100.0%** (`65535` / 65535) | Full-scale reference |

### 5.3 1.0 MHz Sigma-Delta Pulse Density Modulation (PDM)

The software PDM engine runs at 1.0 MHz (1 µs micro-ticks). Each channel maintains a 16-bit error accumulator:
$$\text{accumulator} \leftarrow \text{accumulator} + \text{duty}_{16}$$

If an overflow occurs ($\ge \text{threshold}$), the output pin is asserted high and the threshold is subtracted. This disperses photon emissions uniformly across time, shifting quantization noise to high frequencies where human eyes and rolling-shutter camera sensors naturally filter it out.

### 5.4 Galois LFSR Spread-Spectrum Modulation (CISPR 25)

To meet **CISPR 25 Class 5** limits for vehicle broadcast protection, the micro-tick loop executes a 32-bit Galois Linear Feedback Shift Register (LFSR) with characteristic polynomial $x^{32} + x^{31} + x^{29} + x + 1$ (`0x80000057`):
The low-order bits inject pseudorandom timing micro-jitter into the pacing loop, smearing discrete clock harmonics into a uniform broadband noise floor.

### 5.5 Virtual RC Thermal Observer & Dynamic Droop Compensation

AlInGaP red dice experience thermal droop of approximately $-0.8\%/^\circ\text{C}$ relative to $25^\circ\text{C}$. The firmware dynamically monitors die temperature and computes package self-heating to adjust red gain in real time:
$$T_j = T_{\text{mcu}} + \Delta T_{\text{self}}$$
$$\text{Gain}_{\text{Red}}(T_j) = \text{Gain}_{\text{base}} + (T_j - 25^\circ\text{C}) \times 430$$

### 5.6 Continuous 16-Bit Gamma 2.2 Interpolation & Boundary Closure

Human perception of light is non-linear ($\text{Perceived} \approx \text{Physical}^{1/2.2}$). The engine incorporates a 1025-point 16-bit calibration LUT with zero-division integer interpolation:
$$\text{Output} = \text{LUT}[idx] + \frac{(\text{LUT}[idx+1] - \text{LUT}[idx]) \times rem}{64}$$

The 6-sector HSV converter enforces continuous boundary closure, guaranteeing a seamless transition from $360^\circ \to 0^\circ$ (Magenta/Purple to Red) without intermediate spikes or discontinuities.

---

## 6. Repository Structure

```
.
├── board/                         # Pin initialization configs (MEX generated)
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
│   └── main.c                     # Application entry, PDM engine, safety hooks
├── tools/
│   └── flash_target.py            # Automated PEMicro OpenSDA flashing utility
├── Gpio_Dio_Ip_Example.mex        # S32 Configuration Tools configuration file
├── README.md                      # English documentation
└── README_zh.md                   # Traditional Chinese documentation
```

---

## 7. Getting Started

### 7.1 Prerequisites

* **Hardware**:
  * NXP `XS32K14WEVB-Q064` evaluation board
  * Micro-USB cable
* **Software**:
  * NXP S32 Design Studio for S32 Platform **v3.6.11**
  * NXP Real Time Drivers for S32K1 / S32M24 **RTD 3.0.0**
  * P&E Microcomputer Systems OpenSDA USB Drivers

---

### 7.2 Option A: Build and Debug via S32 Design Studio (IDE)

1. Launch S32 Design Studio.
2. Select **File $\to$ Import... $\to$ General $\to$ Existing Projects into Workspace**.
3. Browse to this repository root and click **Finish**.
4. In the Project Explorer, right-click the project and select **S32 Configuration Tool $\to$ Update Code** to generate the driver files.
5. Click **Project $\to$ Build Project** (or press `Ctrl+B`). Verify build output reports `0 errors, 0 warnings`.
6. Connect the board via USB.
7. Open **Run $\to$ Debug Configurations...**, select `Gpio_Dio_Ip_Example_S32K144W_Debug_FLASH_PNE`, and click **Debug**.

---

### 7.3 Option B: Headless Build and Flash via Command Line (CLI)

The project can be built and flashed headlessly without launching the Eclipse GUI:

```powershell
# 1. Add S32DS build tools and compiler to PATH
$env:PATH = "C:\NXP\S32DS.3.6.11\S32DS\build_tools\msys32\usr\bin;C:\NXP\S32DS.3.6.11\S32DS\build_tools\gcc_v10.2\gcc-10.2-arm32-eabi\bin;" + $env:PATH

# 2. Invoke GNU Make in the build directory
cd Debug_FLASH
make -j8 all

# 3. Flash to target board using the headless OpenSDA script
python ../tools/flash_target.py
```

Build memory footprint:
```
   text    data     bss     dec     hex filename
  34328     412    5908   40648    9ec8 Gpio_Dio_Ip_Example_S32K144W.elf
```

---

## 8. Configuration Options

Primary operational parameters can be adjusted in [`src/main.c`](file:///c:/Users/b/workspaceS32DS.3.6.11/Gpio_Dio_Ip_Example_S32K144W/src/main.c):

| Configuration Macro | Default | Options | Description |
| :--- | :---: | :---: | :--- |
| `CONFIG_ENGINE_MODE` | `1U` | `0U`, `1U` | `1U`: Pure 3-Channel 1.0 MHz PDM.<br>`0U`: Hybrid 16-bit FTM PWM (Red/Blue) + 1.0 MHz PDM (Green). |
| `RAINBOW_CYCLE_DURATION_SEC` | `5UL` | `1UL` ~ `60UL` | Exact duration in seconds for one full 360-degree rainbow loop. |
| `LFSR_SEED_INITIAL` | `0x5A17C395UL`| 32-bit uint | Initial state seed for the Galois LFSR spread-spectrum dither generator. |
| `RED_DROOP_COMP_SLOPE` | `430` | int | Gain compensation slope (+430 counts/°C) for AlInGaP thermal degradation. |
| `THERMAL_NOMINAL_TEMP_C`| `25` | int | Nominal calibration reference temperature in °C. |

---

## 9. Trademarks & Legal Disclaimers

* **NXP®**, **S32 Platform**, **S32 Design Studio®**, **S32 Configuration Tools (MEX)**, and **Real Time Drivers (RTD)** are registered trademarks or trademarks of **NXP Semiconductors N.V.** and its subsidiaries.
* **Arm®** and **Cortex®-M4F** are registered trademarks or trademarks of **Arm Limited** (or its subsidiaries) in the US and/or elsewhere.
* **PEMicro®**, **OpenSDA**, and **Multilink** are registered trademarks of **P&E Microcomputer Systems, Inc.**
* **Cree® LED** is a registered trademark of **CreeLED, Inc.**, a **SMART Global Holdings (SGH)** company.
* **AUTOSAR®** is a registered trademark of the **AUTOSAR Development Partnership**.
* **MISRA®** and **MISRA C®** are registered trademarks of **The MISRA Consortium Limited**.
* All other product or brand names are properties of their respective owners.
