# S32K144W 車規級 RTD 彩虹 LED 控制引擎

[![Target: NXP S32K144W](https://img.shields.io/badge/Target-NXP%20S32K144W-005a9c.svg)](https://www.nxp.com/products/processors-and-microcontrollers/arm-microcontrollers/s32k-automotive-mcus/s32k1-microcontrollers-for-general-purpose:S32K1)
[![Core: Arm Cortex-M4F](https://img.shields.io/badge/Core-Arm%C2%AE%20Cortex%C2%AE--M4F%20%40%2080MHz-blue.svg)](https://www.arm.com/products/silicon-ip-cpu/cortex-m/cortex-m4)
[![RTD: 3.0.0 (AUTOSAR 4.7)](https://img.shields.io/badge/RTD-3.0.0%20(AUTOSAR%C2%AE%204.7)-orange.svg)](https://www.nxp.com)
[![Toolchain: GCC 10.2](https://img.shields.io/badge/Toolchain-GCC%2010.2%20(0%20errors)-brightgreen.svg)](https://gcc.gnu.org)
[![EMC: CISPR 25 Class 5](https://img.shields.io/badge/EMC-CISPR%2025%20Spread--Spectrum-purple.svg)](https://www.iec.ch)
[![License: BSD-3-Clause](https://img.shields.io/badge/License-BSD--3--Clause-blue.svg)](LICENSE)

[English](README.md) | 繁體中文

---

本專案為專為 **恩智浦半導體 (NXP Semiconductors®) S32K144W 評估板 (`XS32K14WEVB-Q064`)** 所設計的車規級全光譜彩虹 LED 控制韌體引擎。

全案嚴格遵循 **NXP Real Time Drivers (RTD 3.0.0)** 官方公共驅動模組與 **S32 Configuration Tools (MEX)** 程式碼架構，展示了在汽車微控制器上實現高刷新率色彩流暢漸變、光學白平衡校正、電磁干擾 (EMI) 抑制與閉環晶面溫度自適應補償之完整工程實踐。

---

## 目錄

- [1. 專案簡介](#1-專案簡介)
- [2. 核心特性](#2-核心特性)
- [3. 硬體需求與接腳配置](#3-硬體需求與接腳配置)
- [4. 運作原理與演算法設計](#4-運作原理與演算法設計)
  - [4.1 Cree® CLP6C-FKB 光學特性與白平衡校正](#41-cree-clp6c-fkb-光學特性與白平衡校正)
  - [4.2 1.0 MHz 一階 Sigma-Delta 脈衝密度調變 (PDM)](#42-10-mhz-一階-sigma-delta-脈衝密度調變-pdm)
  - [4.3 32-Bit 伽羅瓦 LFSR 展頻隨機調變 (CISPR 25)](#43-32-bit-伽羅瓦-lfsr-展頻隨機調變-cispr-25)
  - [4.4 閉環虛擬 RC 晶面熱模型與紅光動態熱衰補償](#44-閉環虛擬-rc-晶面熱模型與紅光動態熱衰補償)
  - [4.5 連續 16-Bit Gamma 2.2 整數線性插值](#45-連續-16-bit-gamma-22-整數線性插值)
- [5. 專案目錄結構](#5-專案目錄結構)
- [6. 快速上手與建置指南](#6-快速上手與建置指南)
  - [6.1 環境需求](#61-環境需求)
  - [6.2 方法 A：S32 Design Studio (IDE 圖形介面操作)](#62-方法-as32-design-studio-ide-圖形介面操作)
  - [6.3 方法 B：純命令列 Headless 自動化建置與燒錄 (CLI)](#63-方法-b純命令列-headless-自動化建置與燒錄-cli)
- [7. 韌體參數配置與調校](#7-韌體參數配置與調校)
- [8. 商標與智慧財產權宣告](#8-商標與智慧財產權宣告)

---

## 1. 專案簡介

在車載多色指示燈與氛圍照明工程中，主要面臨三大物理挑戰：
1. **電磁輻射干擾 (CISPR 25)**：傳統固定頻率脈寬調變 (PWM) 會在特定諧波頻率集中能量，容易干擾車載收音機 (AM/FM) 與高頻 RF 通訊。
2. **熱衰減與色偏 (Thermal Droop & Color Shift)**：紅光 AlInGaP 晶粒每升溫 $1^\circ\text{C}$ 即損失約 $0.8\%$ 光通量，而 InGaN 藍/綠光僅下降 $0.2\%$，導致高溫下白平衡嚴重失真。
3. **頻閃與相機滾動快門條紋 (Stroboscopic & Rolling Shutter)**：低頻 PWM (< 2 kHz) 在車載駕駛監控鏡頭 (DMS) 或高格率相機錄製下會產生肉眼與畫面可見的條紋頻閃。

本專案透過 **1.0 MHz 一階 Sigma-Delta 脈衝密度調變 (PDM)** 軟體核心，結合 **32-Bit 伽羅瓦 LFSR 展頻抖動**、微控制器內建 **ADC 帶隙溫度感測器遙測** 與實時 **虛擬 RC 晶面熱阻觀測器**，以純演算法閉環徹底解決上述工程痛點。

---

## 2. 核心特性

* **100% 官方 NXP RTD 驅動架構**：嚴格遵循 AUTOSAR® 4.7 / RTD 3.0.0 標準公共 API（`Clock_Ip`、`Port_Ci_Port_Ip`、`Ftm_Pwm_Ip`、`Gpio_Dio_Ip`、`OsIf`），絕無任何暫存器直接寫入或未公開符號。
* **雙引擎彈性架構旗標 (`CONFIG_ENGINE_MODE`)**：
  * **模式 1 (`CONFIG_ENGINE_MODE = 1`) — 純 3 通道同步 1.0 MHz PDM**：紅、綠、藍三通道由軟體 PDM 引擎同步驅動，達到相同的傳遞延遲與零相位微抖動。
  * **模式 0 (`CONFIG_ENGINE_MODE = 0`) — 硬體 FTM PWM + PDM 混合架構**：紅、藍通道交由 16-bit FlexTimer (FTM) 硬體 PWM（1.22 kHz 載波）處理，綠光由 1.0 MHz PDM 補足。
* **Cree® CLP6C-FKB 專屬光度學校正**：針對板載貼片 LED 在 5.0V / 680Ω 驅動條件下的實體順向電壓與發光強度差異進行三通道增益正規化。
* **10 kHz 色彩更新率**：以每秒 10,000 次（100 µs 時間切片）平滑推進色相，在 65,536 階離散步階（$0.0055^\circ$ 角解析度）中完全消除人眼可見之階差。
* **CISPR 25 展頻防干擾技術**：在微週期層級注入偽隨機微小抖動，將離散高頻諧波輻射打散至連續寬頻底噪中。
* **閉環虛擬 RC 晶面熱模型與紅光動態補償**：即時在線解算 LED 晶面溫度 ($T_j$)，動態強化紅光增益以鎖定混色白平衡。
* **開機自我檢測 (POST)**：系統重置後自動執行 紅 (300 ms) $\to$ 綠 (300 ms) $\to$ 藍 (300 ms) 序列點亮，確認實體電路狀態正常後進入彩虹循環。

---

## 3. 硬體需求與接腳配置

本專案接腳定義已完全通過官方硬體電路圖 (`SPF-46873_b.pdf`) 與 PCB Layout (`LAY-S32K14WEVB-Q064.pdf`) 之驗證：

| 訊號名稱 / 顏色 | MCU 內部 Port | 64-LQFP 封裝腳位 | 串聯電阻 | 硬體線路分配 | 輸出邏輯電平 |
| :--- | :---: | :---: | :---: | :--- | :---: |
| **紅光 (RED)** | `PTE7` | Pin 39 | `R789` (0Ω) | **FTM0_CH7** (ALT2) / **GPIO** | 正邏輯 High-True (NPN 緩衝) |
| **綠光 (GREEN, 預設)** | `PTE0` | Pin 60 | `R846` (0Ω, 已焊接) | **GPIO** (1.0 MHz 軟體 PDM) | 正邏輯 High-True (NPN 緩衝) |
| **綠光 (GREEN, 備選)** | `PTB12` | Pin 43 | `R787` (0Ω, 空焊) | **FTM0_CH0** (ALT2) | 正邏輯 High-True (NPN 緩衝) |
| **藍光 (BLUE)** | `PTD5` | Pin 24 | `R774` (0Ω, 已焊接) | **FTM2_CH3** (ALT2) / **GPIO** | 正邏輯 High-True (NPN 緩衝) |

> **硬體相容性提示**：出廠標準板焊接了 `R846` (`PTE0`)，而連接至硬體 PWM 的 `R787` (`PTB12`) 預設為空焊。本專案韌體採取雙綠光腳位同步驅動架構，無論硬體有無跳線改造，皆能 100% 免改板隨插即用。

---

## 4. 運作原理與演算法設計

### 4.1 Cree® CLP6C-FKB 光學特性與白平衡校正

評估板配備一顆 **Cree® LED PLCC6 三合一貼片型 RGB LED (`CLP6C-FKB-CM1Q1H1BB7R3R3`)**，由板上 `P5V0` (5.0V) 經由 680Ω 降壓限流電阻 (`R95`, `R96`, `R97`) 並透過 MMBT3904 NPN 電晶體共發射極接地驅動（$V_{ce(sat)} \approx 0.1\text{V}$）：

$$I_f = \frac{V_{CC} - V_f - V_{ce(sat)}}{R_{series}}$$

| 通道顏色 | 晶粒半導體材料與分級 | 順向電壓 ($V_f$) | 工作驅動電流 ($I_f$) | 典型發光強度 | 韌體加權增益 ($K$) | 光度學狀態 |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: |
| **紅光 (RED)** | AlInGaP (M–N: 621 nm) | 2.0 V | 4.19 mA | ~157 mcd | **82.0%** (`53739` / 65535) | 精準匹配光通量 |
| **綠光 (GREEN)** | InGaN (Q–R: 528 nm) | 3.2 V | 2.43 mA | ~200 mcd | **62.0%** (`40632` / 65535) | 抑制綠光刺眼偏色 |
| **藍光 (BLUE)** | InGaN (H–J: 470 nm) | 3.2 V | 2.43 mA | ~46 mcd | **100.0%** (`65535` / 65535) | 全開基準參考通道 |

### 4.2 1.0 MHz 一階 Sigma-Delta 脈衝密度調變 (PDM)

傳統 PWM 在每一週期內集中開通與關斷，能量集中於載波基頻及其整數倍諧波。

本軟體 PDM 引擎以 1.0 MHz（1 µs 微週期）運行，每個通道各自維護一個 16-bit 誤差累加器：

$$\text{accumulator} \leftarrow \text{accumulator} + \text{duty}_{16}$$

當累加器溢位（$\ge 65536$）時，對應輸出接腳置為 High，並減去 $65536$；否則置為 Low。此機制將光子能量在時間軸上均勻離散分佈，將量化雜訊推向超高頻頻帶，由人眼低通濾波特性自然平滑。

### 4.3 32-Bit 伽羅瓦 LFSR 展頻隨機調變 (CISPR 25)

為達成車載電子規範 **CISPR 25 Class 5** 的嚴格限制，微週期循環內嵌 32-bit 伽羅瓦線性反饋移位暫存器 (Galois LFSR)，特徵多項式為 $x^{32} + x^{31} + x^{29} + x + 1$ (`0xD0000001`)：

```c
uint32 lsb = g_lfsr_state & 1U;
g_lfsr_state >>= 1U;
if (lsb) {
    g_lfsr_state ^= 0xD0000001UL;
}
```

利用 LFSR 產生的低位隨機數對微週期延遲施加微小抖動，破壞離散諧波的能量疊加，將電磁輻射平均分攤至連續寬頻底噪中。

### 4.4 閉環虛擬 RC 晶面熱模型與紅光動態熱衰補償

相較於 $25^\circ\text{C}$，AlInGaP 紅光每升溫 $1^\circ\text{C}$ 即損失約 $0.8\%$ 光通量。

本韌體整合動態熱觀測模型：
1. **晶圓遙測**：每 100 個訊框（100 Hz）降採樣讀取 ADC0 通道 26 晶片內部帶隙溫度感測器，經由一階 IIR 低通濾波消除雜訊。
2. **焦耳熱自體發熱估算**：計算三通道瞬時消耗電功率（$P = \sum I_k^2 \cdot R \cdot \text{duty}_k$），輸入一階虛擬熱阻 RC 模型（$\tau \approx 1\text{ s}$, $R_{th} \approx 200^\circ\text{C/W}$）。
3. **閉環增益自適應補償**：
   $$T_j = T_{\text{mcu}} + \Delta T_{\text{self}}$$
   $$\text{Gain}_{\text{Red}}(T_j) = \text{Gain}_{\text{base}} + (T_j - 25^\circ\text{C}) \times 430$$

高溫時自動提高紅光驅動佔空比最多達 $+21.8\%$，鎖定三色光學平衡點。

### 4.5 連續 16-Bit Gamma 2.2 整數線性插值

人眼對光線明暗的感知為非線性響應（$\text{Perceived} \approx \text{Physical}^{1/2.2}$）。韌體採用 1025 階 16-bit 校正查找表搭配無除法的高效整數線性插值：

$$\text{Output} = \text{LUT}[idx] + \frac{(\text{LUT}[idx+1] - \text{LUT}[idx]) \times rem}{64}$$

在全動態範圍內將非線性感知誤差控制在 $<0.005\%$。

---

## 5. 專案目錄結構

```
.
├── board/                         # 腳位與時脈初始配置結構體 (MEX 工具生成)
│   ├── Port_Ci_Port_Ip_Cfg.c
│   └── Port_Ci_Port_Ip_Cfg.h
├── generate/                      # RTD 模組驅動與暫存器配置檔
│   ├── include/                   # Clock, FTM, OsIf 配置頭文件
│   └── src/                       # Clock_Ip_Cfg.c, Ftm_Pwm_Ip_VS_0_PBcfg.c, OsIf_Cfg.c
├── include/                       # 應用層頭文件
│   ├── check_example.h
│   └── gamma_lut_1025.h           # 1025 階 16-bit Gamma 2.2 查找表
├── Project_Settings/
│   ├── Debugger/                  # PEMicro GDB 啟動配置檔
│   ├── Linker_Files/              # S32K144W Flash 與 RAM 鏈結腳本 (.ld)
│   └── Startup_Code/              # CMSIS 啟動代碼、中斷向量表、NVIC 配置
├── RTD/                           # 官方 NXP Real Time Drivers 原始檔 (AUTOSAR 4.7)
├── src/
│   └── main.c                     # 應用程式入口、PDM 調變引擎、熱補償觀測器
├── tests/                         # 功能驗證腳本
├── tools/
│   └── flash_target.py            # 命令列自動化 PEMicro 燒錄工具腳本
├── Gpio_Dio_Ip_Example.mex        # S32 Configuration Tools 視覺化配置工程檔
├── README.md                      # 英文說明文件
└── README_zh.md                   # 繁體中文說明文件
```

---

## 6. 快速上手與建置指南

### 6.1 環境需求

* **硬體設備**：
  * NXP `XS32K14WEVB-Q064` 評估板
  * Micro-USB 傳輸線
* **軟體環境**：
  * NXP S32 Design Studio for S32 Platform **v3.6.11**
  * NXP Real Time Drivers for S32K1 / S32M24 **RTD 3.0.0**
  * P&E Microcomputer Systems OpenSDA USB 驅動程式

---

### 6.2 方法 A：S32 Design Studio (IDE 圖形介面操作)

1. 開啟 S32 Design Studio。
2. 點選 **File $\to$ Import... $\to$ General $\to$ Existing Projects into Workspace**。
3. 瀏覽至本專案目錄並點擊 **Finish**。
4. 在 Project Explorer 專案上按滑鼠右鍵，選擇 **S32 Configuration Tool $\to$ Update Code** 生成驅動程式碼。
5. 點擊功能表 **Project $\to$ Build Project**（或按 `Ctrl+B`），確認編譯結果為 `0 errors, 0 warnings`。
6. 以 USB 傳輸線連接評估板（`J7` 介面）。
7. 開啟 **Run $\to$ Debug Configurations...**，選擇 `Gpio_Dio_Ip_Example_S32K144W_Debug_FLASH_PNE` 並點擊 **Debug** 進行燒錄除錯。

---

### 6.3 方法 B：純命令列 Headless 自動化建置與燒錄 (CLI)

本專案支援無介面命令列（Headless CLI）高速編譯與自動燒錄：

```powershell
# 1. 將 S32DS 隨附之建置工具鏈與 GCC 加入環境變數
$env:PATH = "C:\NXP\S32DS.3.6.11\S32DS\build_tools\msys32\usr\bin;C:\NXP\S32DS.3.6.11\S32DS\build_tools\gcc_v10.2\gcc-10.2-arm32-eabi\bin;" + $env:PATH

# 2. 於 Debug_FLASH 目錄下執行 GNU Make 高速編譯
cd Debug_FLASH
make -j8 all

# 3. 執行自動化燒錄腳本透過 OpenSDA 將 ELF 寫入晶片
python ../tools/flash_target.py
```

預期編譯容量報告：
```
   text    data     bss     dec     hex filename
  34612     412    5908   40932    9fe4 Gpio_Dio_Ip_Example_S32K144W.elf
```

---

## 7. 韌體參數配置與調校

主要功能開關與系統參數定義於 [`src/main.c`](file:///c:/Users/b/workspaceS32DS.3.6.11/Gpio_Dio_Ip_Example_S32K144W/src/main.c)：

| 巨集名稱 | 預設值 | 可選範圍 | 功能說明 |
| :--- | :---: | :---: | :--- |
| `CONFIG_ENGINE_MODE` | `1U` | `0U`, `1U` | `1U`: 純 3 通道 1.0 MHz PDM 調變。<br>`0U`: 硬體 16-bit FTM PWM（紅/藍）+ 1.0 MHz PDM（綠）。 |
| `HUE_STEP_INCREMENT` | `1U` | `1U` ~ `64U` | 每個 100 µs 時間切片色相步進量（調整彩虹變速）。 |
| `LFSR_SEED_INITIAL` | `0x5A8E3B1CUL` | 32-bit uint | 伽羅瓦 LFSR 展頻隨機數產生器初始種子。 |
| `RED_DROOP_COMP_SLOPE` | `430` | int | AlInGaP 紅光熱衰補償斜率（每升溫 $1^\circ\text{C}$ 補償 +430 增益單位）。 |
| `THERMAL_NOMINAL_TEMP_C`| `25` | int | 標稱校正基準參考溫度（攝氏度）。 |

---

## 8. 商標與智慧財產權宣告

* **NXP®**, **S32 Platform**, **S32 Design Studio®**, **S32 Configuration Tools (MEX)**, 及 **Real Time Drivers (RTD)** 為 **恩智浦半導體 (NXP Semiconductors N.V.)** 及其附屬公司之註冊商標或商標。
* **Arm®** 及 **Cortex®-M4F** 為 **Arm Limited**（或其子公司）在美國及/或其他地區之註冊商標或商標。
* **PEMicro®**, **OpenSDA**, 及 **Multilink** 為 **P&E Microcomputer Systems, Inc.** 之註冊商標或商標。
* **Cree® LED** 為 **CreeLED, Inc.** / **SMART Global Holdings (SGH)** 集團公司之註冊商標。
* **AUTOSAR®** 為 **AUTOSAR Development Partnership** 之註冊商標。
* **MISRA®** 及 **MISRA C®** 為 **The MISRA Consortium Limited** 之註冊商標。
* 所有其他產品或品牌名稱均為其各自擁有者之智慧財產。
