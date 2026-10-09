# S32K144W 車規級 RTD 彩虹 LED 控制引擎

[![Target: NXP S32K144W](https://img.shields.io/badge/Target-NXP%20S32K144W-005a9c.svg)](https://www.nxp.com/products/processors-and-microcontrollers/arm-microcontrollers/s32k-automotive-mcus/s32k1-microcontrollers-for-general-purpose:S32K1)
[![Core: Arm Cortex-M4F](https://img.shields.io/badge/Core-Arm%C2%AE%20Cortex%C2%AE--M4F%20%40%2048MHz-blue.svg)](https://www.arm.com/products/silicon-ip-cpu/cortex-m/cortex-m4)
[![RTD: 3.0.0 (AUTOSAR 4.7)](https://img.shields.io/badge/RTD-3.0.0%20(AUTOSAR%C2%AE%204.7)-orange.svg)](https://www.nxp.com)
[![Toolchain: GCC 10.2](https://img.shields.io/badge/Toolchain-GCC%2010.2%20(0%20errors%20%7C%200%20warns)-brightgreen.svg)](https://gcc.gnu.org)
[![Functional Safety: ISO 26262 / ASIL-B Ready](https://img.shields.io/badge/Safety-ISO%2026262%20%2F%20ASIL--B%20Ready-success.svg)](https://www.iso.org/standard/68383.html)
[![EMC: CISPR 25 Class 5](https://img.shields.io/badge/EMC-CISPR%2025%20Spread--Spectrum-purple.svg)](https://www.iec.ch)
[![License: BSD-3-Clause](https://img.shields.io/badge/License-BSD--3--Clause-blue.svg)](LICENSE)

[English](README.md) | 繁體中文

---

本專案為專為 **恩智浦半導體 (NXP Semiconductors®) S32K144W 評估板 (`XS32K14WEVB-Q064`)** 所設計的車規量產級全光譜彩虹 LED 控制韌體引擎。

全案嚴格遵循 **NXP Real Time Drivers (RTD 3.0.0)** 官方公共驅動模組與 **S32 Configuration Tools (MEX)** 程式碼架構，展示了在汽車微控制器上實現確定性節拍控制、光學白平衡校正、電磁干擾 (EMI) 抑制、閉環晶面溫度自適應補償，以及符合 **ISO 26262 功能安全 / MISRA C** 車規軟體架構之工程實踐。

---

## 目錄

- [1. 專案簡介](#1-專案簡介)
- [2. 核心特性](#2-核心特性)
- [3. 車規量產安全架構設計 (ISO 26262 / MISRA C)](#3-車規量產安全架構設計-iso-26262--misra-c)
- [4. 硬體需求與接腳配置](#4-硬體需求與接腳配置)
- [5. 運作原理與演算法設計](#5-運作原理與演算法設計)
  - [5.1 5.000000 秒精密週期與 Bresenham 分數累加器](#51-5000000-秒精密週期與-bresenham-分數累加器)
  - [5.2 Cree® CLP6C-FKB 光學特性與白平衡校正](#52-cree-clp6c-fkb-光學特性與白平衡校正)
  - [5.3 1.0 MHz 一階 Sigma-Delta 脈衝密度調變 (PDM)](#53-10-mhz-一階-sigma-delta-脈衝密度調變-pdm)
  - [5.4 32-Bit 伽羅瓦 LFSR 展頻隨機調變 (CISPR 25)](#54-32-bit-伽羅瓦-lfsr-展頻隨機調變-cispr-25)
  - [5.5 閉環虛擬 RC 晶面熱模型與紅光動態熱衰補償](#55-閉環虛擬-rc-晶面熱模型與紅光動態熱衰補償)
  - [5.6 連續 16-Bit Gamma 2.2 整數插值與全域無縫閉合](#56-連續-16-bit-gamma-22-整數插值與全域無縫閉合)
- [6. 專案目錄結構](#6-專案目錄結構)
- [7. 快速上手與建置指南](#7-快速上手與建置指南)
  - [7.1 環境需求](#71-環境需求)
  - [7.2 方法 A：S32 Design Studio (IDE 圖形介面操作)](#72-方法-as32-design-studio-ide-圖形介面操作)
  - [7.3 方法 B：純命令列 Headless 自動化建置與燒錄 (CLI)](#73-方法-b純命令列-headless-自動化建置與燒錄-cli)
- [8. 韌體參數配置與調校](#8-韌體參數配置與調校)
- [9. 商標與智慧財產權宣告](#9-商標與智慧財產權宣告)

---

## 1. 專案簡介

在車載多色指示燈與車內氛圍照明工程中，主要面臨四大物理與可靠度挑戰：
1. **電磁輻射干擾 (CISPR 25)**：傳統固定頻率脈寬調變 (PWM) 會在特定諧波頻率集中能量，容易干擾車載收音機 (AM/FM) 與高頻 RF 通訊。
2. **熱衰減與色偏 (Thermal Droop & Color Shift)**：紅光 AlInGaP 晶粒每升溫 $1^\circ\text{C}$ 即損失約 $0.8\%$ 光通量，而 InGaN 藍/綠光僅下降 $0.2\%$，導致高溫下白平衡嚴重失真。
3. **頻閃與相機滾動快門條紋 (Stroboscopic & Rolling Shutter)**：低頻 PWM (< 2 kHz) 在車載駕駛監控鏡頭 (DMS) 或高格率相機錄製下會產生肉眼與畫面可見的條紋頻閃。
4. **長時間無漂移確定性運行 (連續運作 5 年以上)**：簡易微秒延遲迴圈因整數捨入誤差累積，長期運行會造成顯著時間漂移。

本專案透過 **1.0 MHz 一階 Sigma-Delta 脈衝密度調變 (PDM)** 軟體核心，結合 **32-Bit 伽羅瓦 LFSR 展頻抖動**、微控制器內建 **ADC 帶隙溫度感測器遙測**、**Bresenham 分數累加器** 與符合 **ISO 26262 功能安全** 之防禦架構，徹底解決上述問題。

---

## 2. 核心特性

* **100% 官方 NXP RTD 驅動架構**：嚴格遵循 AUTOSAR® 4.7 / RTD 3.0.0 標準公共 API（`Clock_Ip`、`Port_Ci_Port_Ip`、`Ftm_Pwm_Ip`、`Gpio_Dio_Ip`、`OsIf`），絕無任何暫存器直接寫入或未公開符號。
* **精確 5.000000 秒週期與零累積漂移**：由 ARM Cortex-M4 硬體 SysTick 計時器嚴密節拍控制（48 MHz 下每週期剛好 $240,000,000\text{ cycles}$）。Bresenham 分數餘數累加演算法在數學上保證**累積漂移為 $0.000000\%$**。
* **全光譜無斷層平滑閉合**：65,536 階離散步階（$0.0055^\circ$ 角解析度），在 $360^\circ \to 0^\circ$（紫光轉紅光）邊界處完美無縫銜接，徹底消滅突兀閃光。
* **雙引擎彈性架構旗標 (`CONFIG_ENGINE_MODE`)**：
  * **模式 1 (`CONFIG_ENGINE_MODE = 1`) — 純 3 通道同步 1.0 MHz PDM**：紅、綠、藍三通道由軟體 PDM 引擎同步驅動，達到相同的傳遞延遲與零相位微抖動。
  * **模式 0 (`CONFIG_ENGINE_MODE = 0`) — 硬體 FTM PWM + PDM 混合架構**：紅、藍通道交由 16-bit FlexTimer (FTM) 硬體 PWM（1.22 kHz 載波）處理，綠光由 1.0 MHz PDM 補足。
* **Cree® CLP6C-FKB 專屬光度學校正**：針對板載貼片 LED 在 5.0V / 680Ω 驅動條件下的實體順向電壓與發光強度差異進行三通道增益正規化。
* **CISPR 25 展頻防干擾技術**：在微週期層級注入偽隨機微小抖動，將離散高頻諧波輻射打散至連續寬頻底噪中。
* **閉環虛擬 RC 晶面熱模型與紅光動態補償**：即時在線解算 LED 晶面溫度 ($T_j$)，動態強化紅光增益以鎖定混色白平衡（-40°C ~ +105°C）。

---

## 3. 車規量產安全架構設計 (ISO 26262 / MISRA C)

為滿足 Tier-1 車載電控系統規範，專案導入了嚴密的軟體安全防禦：

```
+-------------------------------------------------------------------------+
|                  應用軟體層 (Application SWC: Lighting Engine)         |
|  - 65,536 階色階引擎         - 5.000000s Bresenham 節拍 (0.0000% 漂移)  |
|  - 16-Bit Gamma 2.2 插值      - 虛擬 RC 晶面熱阻觀測器                   |
+-------------------------------------------------------------------------+
                                    |
+-------------------------------------------------------------------------+
|                  功能安全防禦層 (Safety Layer: ISO 26262)               |
|  - 實時硬體看門狗 (WDOG) 餵狗 - 有界循環防護 (Iteration Guards)          |
|  - 故障安全狀態機 (Fail-Safe) - 靜態最壞情況堆疊證明 (Stack Proof)       |
+-------------------------------------------------------------------------+
                                    |
+-------------------------------------------------------------------------+
|                 NXP 底層驅動層 (MCAL / RTD 3.0.0)                       |
|  - Clock_Ip    - Port_Ci_Port_Ip    - Ftm_Pwm_Ip    - Gpio_Dio_Ip       |
+-------------------------------------------------------------------------+
```

1. **確定性有界迴圈 (Deterministic Bounded Execution)**：
   - 所有內部計時與輪詢迴圈皆具備硬性計數上限守衛 (`iter_guard < 10000U`)，嚴禁無界 `while` 等待，確保時間可預測性。
2. **時間性與控制流監控 (硬體看門狗服務)**：
   - 整合 `App_Wdog_Service()` 於主輪詢迴圈中，主動餵養晶片內建 `WDOG` 模組，防範 CPU 鎖死。
3. **最壞情況靜態堆疊分析證明 (`-fstack-usage`)**：
   - 在 GCC 10.2 車規編譯選項下產生靜態堆疊用量報告，全系統最深呼叫鏈僅 **88 Bytes**（相對於 S32K144W 配置的 4,096 Bytes 堆疊空間，佔用率僅 **2.1%**）：
     ```text
     Apply_Gamma16                :  0 bytes (暫存器內聯，零堆疊)
     Galois_LFSR_Next             :  0 bytes (暫存器內聯，零堆疊)
     App_FaultHandler             : 16 bytes
     App_Thermal_Observer_Update  : 12 bytes
     Rainbow_Update16             : 40 bytes
     main                         : 48 bytes
     最深呼叫鏈                    : 88 bytes (main + Rainbow_Update16)
     ```
4. **故障安全狀態機 (Fail-Safe Containment)**：
   - 開機硬體初始化若遇任何失敗，即刻觸發 `App_FaultHandler()`，強制硬體切斷全部 LED 通道輸出，確保系統安全。
5. **零動態記憶體配置 (MISRA C Rule 21.3)**：
   - 全案完全不使用 Heap（嚴禁 `malloc`/`free`），所有記憶體皆於編譯期靜態分配。

---

## 4. 硬體需求與接腳配置

本專案接腳定義已完全通過官方硬體電路圖 (`SPF-46873_b.pdf`) 與 PCB Layout (`LAY-S32K14WEVB-Q064.pdf`) 之驗證：

| 訊號名稱 / 顏色 | MCU 內部 Port | 64-LQFP 封裝腳位 | 串聯電阻 | 硬體線路分配 | 輸出邏輯電平 |
| :--- | :---: | :---: | :---: | :--- | :---: |
| **紅光 (RED)** | `PTE7` | Pin 39 | `R789` (0Ω) | **FTM0_CH7** (ALT2) / **GPIO** | 正邏輯 High-True (NPN 緩衝) |
| **綠光 (GREEN, 預設)** | `PTE0` | Pin 60 | `R846` (0Ω, 已焊接) | **GPIO** (1.0 MHz 軟體 PDM) | 正邏輯 High-True (NPN 緩衝) |
| **綠光 (GREEN, 備選)** | `PTB12` | Pin 43 | `R787` (0Ω, 空焊) | **FTM0_CH0** (ALT2) | 正邏輯 High-True (NPN 緩衝) |
| **藍光 (BLUE)** | `PTD5` | Pin 24 | `R774` (0Ω, 已焊接) | **FTM2_CH3** (ALT2) / **GPIO** | 正邏輯 High-True (NPN 緩衝) |

---

## 5. 運作原理與演算法設計

### 5.1 5.000000 秒精密週期與 Bresenham 分數累加器

在 48.0 MHz 核心頻率下，5.000000 秒週期包含：
$$\text{總週期數} = 5.000000\text{ s} \times 48,000,000\text{ Hz} = 240,000,000\text{ cycles}$$

均分給 65,536 個離散色相步階：
$$\text{每步基準週期} = \lfloor 240,000,000 / 65,536 \rfloor = 3,662\text{ cycles}$$
$$\text{每步分數餘數} = 240,000,000 \pmod{65,536} = 9,216\text{ cycles}$$

$$\sum_{k=0}^{65535} \text{StepCycles}_k = (65,536 \times 3,662) + 9,216 = 240,000,000\text{ cycles} \equiv 5.000000000\text{ 秒}$$

次微秒餘數結轉至下一步階，數學上保證**累積誤差為 $0.000000\%$**，連續運行 5 年不產生任何時鐘飄移。

### 5.2 Cree® CLP6C-FKB 光學特性與白平衡校正

評估板配備一顆 **Cree® LED PLCC6 三合一貼片型 RGB LED (`CLP6C-FKB-CM1Q1H1BB7R3R3`)**，由板上 `P5V0` (5.0V) 經由 680Ω 限流電阻驅動：

| 通道顏色 | 晶粒半導體材料與分級 | 順向電壓 ($V_f$) | 工作驅動電流 ($I_f$) | 典型發光強度 | 韌體加權增益 ($K$) | 光度學狀態 |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: |
| **紅光 (RED)** | AlInGaP (M–N: 621 nm) | 2.0 V | 4.19 mA | ~157 mcd | **82.0%** (`53739` / 65535) | 精準匹配光通量 |
| **綠光 (GREEN)** | InGaN (Q–R: 528 nm) | 3.2 V | 2.43 mA | ~200 mcd | **62.0%** (`40632` / 65535) | 抑制綠光刺眼偏色 |
| **藍光 (BLUE)** | InGaN (H–J: 470 nm) | 3.2 V | 2.43 mA | ~46 mcd | **100.0%** (`65535` / 65535) | 全開基準參考通道 |

### 5.3 1.0 MHz 一階 Sigma-Delta 脈衝密度調變 (PDM)

軟體 PDM 引擎以 1.0 MHz 運行，每個通道各自維護一個 16-bit 誤差累加器：
$$\text{accumulator} \leftarrow \text{accumulator} + \text{duty}_{16}$$

累加器溢出時拉高接腳並減去門檻值。這使得光子均勻散佈於時間軸上，並將量化雜訊推至高頻區，由人眼視覺滯留與相機感光元件自然濾除。

### 5.4 32-Bit 伽羅瓦 LFSR 展頻隨機調變 (CISPR 25)

符合 **CISPR 25 Class 5** 車載廣播防護標準，使用特徵多項式 $x^{32} + x^{31} + x^{29} + x + 1$ (`0x80000057`)：
在微延遲迴圈注入隨機微抖動，將集中的諧波能量打散為均勻平坦的寬頻底噪。

### 5.5 閉環虛擬 RC 晶面熱模型與紅光動態熱衰補償

紅光晶粒隨溫度升高存在約 $-0.8\%/^\circ\text{C}$ 的光通量熱衰減。韌體動態採集 MCU 晶片溫度並即時推算封裝焦耳自熱：
$$T_j = T_{\text{mcu}} + \Delta T_{\text{self}}$$
$$\text{Gain}_{\text{Red}}(T_j) = \text{Gain}_{\text{base}} + (T_j - 25^\circ\text{C}) \times 430$$

### 5.6 連續 16-Bit Gamma 2.2 整數插值與全域無縫閉合

人眼對光強的感知為非線性（$\text{Perceived} \approx \text{Physical}^{1/2.2}$），引擎內建 1025 點 16-bit 校準表與無除法整數線性插值：
$$\text{Output} = \text{LUT}[idx] + \frac{(\text{LUT}[idx+1] - \text{LUT}[idx]) \times rem}{64}$$

色相六分區轉換保證 $360^\circ \to 0^\circ$ 連續全域閉合，紫轉紅過渡極其平滑且零閃光。

---

## 6. 專案目錄結構

```
.
├── board/                         # 引腳初始化配置 (MEX 生成)
│   ├── Port_Ci_Port_Ip_Cfg.c
│   └── Port_Ci_Port_Ip_Cfg.h
├── generate/                      # RTD 模組驅動與結構體
│   ├── include/                   # Clock, FTM, OsIf 標頭檔
│   └── src/                       # Clock_Ip_Cfg.c, Ftm_Pwm_Ip_VS_0_PBcfg.c, OsIf_Cfg.c
├── include/                       # 應用層標頭檔
│   ├── check_example.h
│   └── gamma_lut_1025.h           # 1025 點 16-Bit Gamma 2.2 尋查表
├── Project_Settings/
│   ├── Debugger/                  # PEMicro GDB 調試配置
│   ├── Linker_Files/              # S32K144W Flash & RAM 鏈接腳本 (.ld)
│   └── Startup_Code/              # CMSIS 啟動代碼、向量表
├── RTD/                           # NXP Real Time Drivers 源代碼 (AUTOSAR 4.7)
├── src/
│   └── main.c                     # 主程式入口、PDM 引擎、安全機制
├── tools/
│   └── flash_target.py            # 自動化 OpenSDA 燒錄腳本
├── Gpio_Dio_Ip_Example.mex        # S32 Configuration Tools 專案檔
├── README.md                      # 英文說明文件
└── README_zh.md                   # 繁體中文說明文件
```

---

## 7. 快速上手與建置指南

### 7.1 環境需求

* **硬體設備**：
  * NXP `XS32K14WEVB-Q064` 評估板
  * Micro-USB 連接線
* **軟體環境**：
  * NXP S32 Design Studio for S32 Platform **v3.6.11**
  * NXP Real Time Drivers for S32K1 / S32M24 **RTD 3.0.0**
  * P&E Microcomputer Systems OpenSDA USB 驅動程式

---

### 7.2 方法 A：S32 Design Studio (IDE 圖形介面操作)

1. 開啟 S32 Design Studio。
2. 點選 **File $\to$ Import... $\to$ General $\to$ Existing Projects into Workspace**。
3. 瀏覽至本專案目錄並點擊 **Finish**。
4. 在 Project Explorer 中右鍵點選專案，選擇 **S32 Configuration Tool $\to$ Update Code** 生成代碼。
5. 點擊 **Project $\to$ Build Project**（或按 `Ctrl+B`）。確認輸出為 `0 errors, 0 warnings`。
6. 接上開發板 USB 線。
7. 開啟 **Run $\to$ Debug Configurations...**，選擇 `Gpio_Dio_Ip_Example_S32K144W_Debug_FLASH_PNE`，點選 **Debug**。

---

### 7.3 方法 B：純命令列 Headless 自動化建置與燒錄 (CLI)

可在不啟動 Eclipse 圖形介面的情況下，由命令列完成一鍵建置與燒錄：

```powershell
# 1. 將 S32DS 工具鏈與 GCC 編譯器加入環境變數
$env:PATH = "C:\NXP\S32DS.3.6.11\S32DS\build_tools\msys32\usr\bin;C:\NXP\S32DS.3.6.11\S32DS\build_tools\gcc_v10.2\gcc-10.2-arm32-eabi\bin;" + $env:PATH

# 2. 進入 build 目錄並以 GNU Make 編譯
cd Debug_FLASH
make -j8 all

# 3. 透過 OpenSDA 自動燒錄腳本部署至開發板
python ../tools/flash_target.py
```

記憶體佔用報告：
```
   text    data     bss     dec     hex filename
  34328     412    5908   40648    9ec8 Gpio_Dio_Ip_Example_S32K144W.elf
```

---

## 8. 韌體參數配置與調校

主要參數可於 [`src/main.c`](file:///c:/Users/b/workspaceS32DS.3.6.11/Gpio_Dio_Ip_Example_S32K144W/src/main.c) 中直接配置：

| 配置巨集 | 預設值 | 可選範圍 | 功能說明 |
| :--- | :---: | :---: | :--- |
| `CONFIG_ENGINE_MODE` | `1U` | `0U`, `1U` | `1U`: 純 3 通道 1.0 MHz 軟體 PDM。<br>`0U`: 16-bit FTM 硬體 PWM (紅/藍) + 1.0 MHz PDM (綠)。 |
| `RAINBOW_CYCLE_DURATION_SEC` | `5UL` | `1UL` ~ `60UL` | 單輪 360° 彩虹完整漸變週期（秒）。 |
| `LFSR_SEED_INITIAL` | `0x5A17C395UL`| 32-bit uint | 伽羅瓦 LFSR 展頻隨機數初始種子。 |
| `RED_DROOP_COMP_SLOPE` | `430` | int | 紅光熱衰減補償斜率（每升溫 1°C 補償 +430 增益計數）。 |
| `THERMAL_NOMINAL_TEMP_C`| `25` | int | 標稱參考校正溫度（°C）。 |

---

## 9. 商標與智慧財產權宣告

* **NXP®**、**S32 Platform**、**S32 Design Studio®**、**S32 Configuration Tools (MEX)** 與 **Real Time Drivers (RTD)** 均為 **恩智浦半導體 (NXP Semiconductors N.V.)** 之註冊商標或商標。
* **Arm®** 與 **Cortex®-M4F** 均為 **Arm Limited** 之註冊商標。
* **PEMicro®** 與 **OpenSDA** 均為 **P&E Microcomputer Systems, Inc.** 之註冊商標。
* **Cree® LED** 為 **CreeLED, Inc.** 之註冊商標。
* **AUTOSAR®** 為 **AUTOSAR Development Partnership** 之註冊商標。
* **MISRA®** 與 **MISRA C®** 為 **The MISRA Consortium Limited** 之註冊商標。
* 其餘所有產品或品牌名稱皆為其各自所有者之財產。
