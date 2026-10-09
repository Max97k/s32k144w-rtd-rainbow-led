# S32K144W Automotive RTD Hardware FTM PWM Rainbow LED Engine

High-precision, automotive production-grade Rainbow LED firmware designed for the **NXP S32K144W-Q064 Evaluation Board (`XS32K14WEVB-Q064`)**.

This project implements a smooth, full-spectrum $0^\circ \sim 360^\circ$ HSV rainbow color-sweep engine utilizing official **NXP Real Time Drivers (RTD)** public APIs and **S32 Configuration Tools (MEX)** code generation.

---

## 💻 Hardware & Software Environment

### Target Hardware
* **Evaluation Board**: NXP XS32K14WEVB-Q064 (Revision B)
* **Microcontroller**: NXP S32K144W (Arm® Cortex®-M4F @ 80 MHz, 512 KB Flash, 64 KB SRAM, Package: 64 LQFP, Silicon Mask: `0N77P`)
* **Debug Interface**: On-board OpenSDA (PEMicro Multilink CDC/GDB)

### Software & Toolchain
* **IDE**: NXP S32 Design Studio for S32 Platform (S32DS) **Version 3.6.11** (Build `240725`)
* **SDK / Driver Layer**: NXP Platform SDK / Real Time Drivers (RTD) for S32K1 / S32M24 **RTD 3.0.0** (AUTOSAR 4.7 specification)
* **Configuration Tool**: S32 Configuration Tools (MEX Tool Framework **v1.6.0**)
* **Compiler / Toolchain**: NXP GCC **10.2.0** (`arm-none-eabi-gcc`)
* **Debugger Engine**: PEMicro GDB Server / GDB PEMicro Interface Debugging

---

## 🔌 Hardware Mapping & Schematic Verification

Verified against official schematic (`SPF-46873_b.pdf`) and PCB layout (`LAY-S32K14WEVB-Q064.pdf`):

| LED Channel | MCU Pin | Package Pin | Jumper / Resistor | Driver Mode | Output Polarity |
| :---: | :---: | :---: | :---: | :---: | :---: |
| **RED** | `PTE7` | Pin 39 | `R789` (0Ω) | **FTM0_CH7** (ALT2) | High-True (NPN buffer) |
| **GREEN (Default)** | `PTE0` | Pin 60 | `R846` (0Ω, Factory Populated) | **GPIO** (ALT1, 5 kHz RTD Modulation) | High-True (NPN buffer) |
| **GREEN (Alt)** | `PTB12` | Pin 43 | `R787` (0Ω, Optional) | **FTM0_CH0** (ALT2) | High-True (NPN buffer) |
| **BLUE** | `PTD5` | Pin 24 | `R774` (0Ω, Factory Populated) | **FTM2_CH3** (ALT2) | High-True (NPN buffer) |

*Note: Factory-assembled EVBs populate `R846` (`PTE0`), leaving `R787` (`PTB12`) unpopulated. The firmware dual-drives both channels simultaneously to guarantee 100% out-of-the-box compatibility across all board revisions.*

---

## 🌟 Key Features

* **100% Official NXP RTD Architecture**: Built strictly with standard AUTOSAR 4.7 / RTD 3.0.0 public driver layers (`Clock_Ip`, `Port_Ci_Port_Ip`, `Ftm_Pwm_Ip`, `Gpio_Dio_Ip`, `OsIf`).
* **Zero Bare-Metal Register Hacking**: Completely eliminates non-portable direct register manipulations (`S32_SysTick`, `S32_SCB`, raw pointer offsets) in full compliance with automotive MISRA-C and defensive coding standards.
* **10,000-Tick Perceptual Gamma 2.2 Correction**: Incorporates a precomputed 256-entry Gamma 2.2 look-up table scaled precisely to $0 \sim 10,000$ timer period ticks, ensuring perceptually linear color blending without RTD duty cycle overflow.
* **Power-On Self-Test (POST)**: Flashes Red (300 ms) $\to$ Green (300 ms) $\to$ Blue (300 ms) upon MCU reset for instant physical hardware health verification before transitioning into seamless rainbow streaming.
* **Clean Open-Source Repository Layout**: Complies with NXP software licensing policies by excluding proprietary binary drivers and build artifacts. All driver configurations regenerate on demand via S32 Configuration Tools.

---

## 🚀 How to Import and Build in S32DS

1. **Clone the Repository**:
   ```bash
   git clone https://github.com/Max97k/s32k144w-rtd-rainbow-led.git
   ```

2. **Import into S32 Design Studio**:
   * Open S32DS.
   * Go to **File $\to$ Import... $\to$ General $\to$ Existing Projects into Workspace**.
   * Browse to the cloned directory, select `Gpio_Dio_Ip_Example_S32K144W`, and click **Finish**.

3. **Generate Driver Configuration Code**:
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

## 👥 Credits & AI Pair Programming Metrics

This project was co-engineered through an interactive pair-programming session between human developer and AI coding agent.

* **Developer**: [@Max97k](https://github.com/Max97k)
* **AI Pair Programmer**: Google DeepMind **Antigravity** (Advanced Agentic Coding)

### 📊 Development & Token Metrics
* **Total Interactive Trajectory Steps**: `1,613` Steps
* **Total LLM Tokens Consumed**: `7,855,266` Tokens (~7.85 Million Tokens)
  * **Input Prompt Tokens**: `7,490,212`
  * **Output Completion Tokens**: `365,054`
  * **Context Cache Read Tokens**: `115,605,850` (~115.6M Tokens)

---

## 📄 License & Compliance

* Application logic and rainbow engine: Open-source under permissive terms.
* NXP Real Time Drivers (RTD) and configuration tools are subject to the [NXP Software License Agreement](https://www.nxp.com).
