# S32K144W Automotive RTD Hardware FTM PWM Rainbow LED Engine

High-precision, automotive production-grade Rainbow LED firmware designed for the **NXP S32K144W-Q064 Evaluation Board (`XS32K14WEVB-Q064`)**.

This project implements a smooth, full-spectrum $0^\circ \sim 360^\circ$ HSV rainbow color-sweep engine utilizing official **NXP Real Time Drivers (RTD)** public APIs and **S32 Configuration Tools (MEX)** code generation.

---

## 🌟 Key Features

* **100% Official NXP RTD Architecture**: Built strictly with standard AUTOSAR 4.7 / RTD 3.0.0 public driver layers (`Clock_Ip`, `Port_Ci_Port_Ip`, `Ftm_Pwm_Ip`, `Gpio_Dio_Ip`, `OsIf`).
* **Zero Bare-Metal Register Hacking**: Completely eliminates non-portable direct register manipulations (`S32_SysTick`, `S32_SCB`, raw pointer offsets) in full compliance with automotive MISRA-C and defensive coding standards.
* **Dual-Channel Green Resistor Compatibility**: Accommodates both EVB hardware stuffing variants (`PTE0` via default factory 0Ω jumper `R846`, and `PTB12` via `R787`).
* **10,000-Tick Perceptual Gamma 2.2 Correction**: Incorporates a precomputed 256-entry Gamma 2.2 look-up table scaled precisely to $0 \sim 10,000$ timer period ticks, ensuring perceptually linear color blending without RTD duty cycle overflow.
* **Power-On Self-Test (POST)**: Flashes Red (300 ms) $\to$ Green (300 ms) $\to$ Blue (300 ms) upon MCU reset for instant physical hardware health verification before transitioning into seamless rainbow streaming.
* **Clean Open-Source Repository Layout**: Complies with NXP software licensing policies by excluding proprietary binary drivers and build artifacts. All driver configurations regenerate on demand via S32 Configuration Tools.

---

## 🔌 Hardware Mapping (S32K144W-Q064 EVB)

Verified against official schematic (`SPF-46873_b.pdf`) and PCB layout (`LAY-S32K14WEVB-Q064.pdf`):

| LED Channel | MCU Pin | Package Pin | Jumper / Resistor | Driver Mode | Output Polarity |
| :---: | :---: | :---: | :---: | :---: | :---: |
| **RED** | `PTE7` | Pin 39 | `R789` (0Ω) | **FTM0_CH7** (ALT2) | High-True (NPN buffer) |
| **GREEN (Default)** | `PTE0` | Pin 60 | `R846` (0Ω, Factory Populated) | **GPIO** (ALT1, 5 kHz RTD Modulation) | High-True (NPN buffer) |
| **GREEN (Alt)** | `PTB12` | Pin 43 | `R787` (0Ω, Optional) | **FTM0_CH0** (ALT2) | High-True (NPN buffer) |
| **BLUE** | `PTD5` | Pin 24 | `R774` (0Ω, Factory Populated) | **FTM2_CH3** (ALT2) | High-True (NPN buffer) |

---

## 🛠️ Prerequisites

* **IDE**: [NXP S32 Design Studio for S32 Platform v3.6](https://www.nxp.com/design/design-center/software/development-software/s32-design-studio-ide:S32DS-IDE)
* **SDK**: NXP Platform SDK / RTD for S32K1 / S32M24 (RTD 3.0.0 or compatible)
* **Hardware**: S32K144W-Q064 Evaluation Board connected via micro-USB (OpenSDA PEMicro)

---

## 🚀 How to Import and Build in S32DS

Following standard NXP project distribution conventions:

1. **Clone the Repository**:
   ```bash
   git clone https://github.com/Max97k/s32k144w-rtd-rainbow-led.git
   ```

2. **Import into S32 Design Studio**:
   * Open S32DS.
   * Go to **File $\to$ Import... $\to$ General $\to$ Existing Projects into Workspace**.
   * Browse to the cloned directory, select `Gpio_Dio_Ip_Example_S32K144W`, and click **Finish**.

3. **Generate Configuration Code**:
   * Select the project in **Project Explorer**.
   * In the top menu or toolbar, click:  
     👉 **S32 Configuration Tool $\to$ Update Code**  
   * S32 Configuration Tools will parse `Gpio_Dio_Ip_Example.mex` and generate all local driver configurations under `generate/` and `board/`.

4. **Build the Project**:
   * Click **Project $\to$ Build Project** (or click the Hammer icon).
   * Confirm the build completes with **0 Errors, 0 Warnings**.
   * Target ELF is generated at `Debug_FLASH/Gpio_Dio_Ip_Example_S32K144W.elf`.

5. **Flash & Debug**:
   * Go to **Run $\to$ Debug Configurations...**
   * Under **GDB PEMicro Interface Debugging**, select `Gpio_Dio_Ip_Example_S32K144W_Debug_FLASH_PNE`.
   * Click **Debug** to flash the target and start execution.

---

## 📄 License & Compliance

* Application logic and rainbow engine: Open-source under permissive terms.
* NXP Real Time Drivers (RTD) and configuration tools are subject to the [NXP Software License Agreement](https://www.nxp.com).
