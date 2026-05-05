# Trade on Table — ESP32 桌面股票监控器 / ESP32 Desktop Stock Monitor

**[中文](#中文) | [English](#english)**

---

<a name="中文"></a>

# 中文

一款基于 ESP32 的桌面股票 / 加密货币实时行情监控设备，通过 OLED 屏幕滚动显示多只自选标的的实时价格与涨跌幅，并提供 Web 配置页面，无需修改代码即可灵活配置 WiFi 和自选列表。

---

## 功能特性

- **实时行情**：通过 [Finnhub](https://finnhub.io/) API 每 15 秒刷新一次股票 / 加密货币价格
- **多标的支持**：最多同时监控 5 只标的（股票 + 加密货币均可）
- **K 线历史**：本地缓存最近 30 个价格点，用于简易走势展示
- **OLED 显示**：128×64 SSD1306 屏幕，清晰显示代码、价格、涨跌幅
- **旋转编码器**：KY-040 编码器旋转切换标的，按下编码器按钮或长按 BOOT 按钮进入配置模式
- **Web 配置**：设备开机后以 AP 热点（`StockMonitor` / `12345678`）提供配置页面，扫描连接后填写 WiFi 和股票代码，保存后自动重启
- **掉电记忆**：配置信息保存在 ESP32 NVS Flash，断电不丢失

---

## 物料清单（BOM）

| # | 元件名称 | 规格型号 | 数量 | 备注 |
|---|----------|----------|------|------|
| 1 | ESP32 开发板 | ESP32-DevKitC（或兼容板） | 1 | 建议 38-pin 版本 |
| 2 | OLED 显示模块 | 0.96" SSD1306，I²C，128×64，3.3V/5V | 1 | 4 针 I²C 接口 |
| 3 | 旋转编码器 | KY-040（含按钮） | 1 | — |
| 4 | 面包板 | 400 孔或 830 孔 | 1 | 可选，或自制 PCB |
| 5 | 杜邦线 | 公对公 / 公对母，10 cm | 若干 | 用于连接各模块 |
| 6 | USB 数据线 | Micro-USB（与开发板匹配） | 1 | 供电 + 烧录 |
| 7 | 3D 打印外壳 | ESP32 + 0.96" OLED 外壳（见下方说明） | 1 | 可选，PLA 材质 |

---

## 3D 打印外壳

本项目使用以下 Thingiverse 模型作为设备外壳打印文件：

> **[ESP 32 case enclosure + 0.96 inch oled window](https://www.thingiverse.com/thing:4331477)**  
> 作者：[23Ro](https://www.thingiverse.com/23Ro)  
> 授权：GNU GPL  
> 链接：https://www.thingiverse.com/thing:4331477

该外壳专为 ESP32 开发板与 0.96" OLED 模块设计，带有 OLED 显示窗口，并保留了 Reset 与 BOOT 按钮的开孔，方便编程操作。

**建议打印参数（参考原作者设置）：**

| 参数 | 值 |
|------|----|
| 层高 | 0.175 mm |
| 填充率 | 20% |
| 底座（Raft） | 是 |
| 支撑 | 否 |
| 材质 | PLA |

---

## 接线说明

### OLED（I²C）

| OLED 引脚 | ESP32 引脚 |
|-----------|-----------|
| VCC | 3.3V |
| GND | GND |
| SDA | GPIO 21 |
| SCL | GPIO 22 |

### KY-040 旋转编码器

| 编码器引脚 | ESP32 引脚 |
|-----------|-----------|
| CLK | GPIO 34 |
| DT | GPIO 35 |
| SW（按钮） | GPIO 32 |
| + / VCC | 3.3V |
| GND | GND |

---

## 软件依赖

| 库 | 版本 |
|----|------|
| Adafruit SSD1306 | ^2.5.7 |
| Adafruit GFX Library | ^1.11.5 |
| ArduinoJson | ^7.0.0 |

> 依赖已在 `platformio.ini` 中声明，PlatformIO 会自动下载安装。

---

## 快速开始

### 1. 克隆 / 下载项目

```bash
git clone <仓库地址>
```

### 2. 用 PlatformIO 打开项目

在 VS Code 中安装 **PlatformIO IDE** 扩展，然后打开本项目文件夹。

### 3. 填写 API Key

编辑 `src/main.cpp`，将 `API_KEY` 替换为你自己的 [Finnhub API Key](https://finnhub.io/register)：

```cpp
const char* API_KEY = "你的 API Key";
```

### 4. 上传文件系统

先将 `data/` 目录中的 Web 页面资源上传到 SPIFFS：

```
PlatformIO: Upload Filesystem Image
```

### 5. 编译并烧录

```
PlatformIO: Upload
```

---

## 使用说明

1. **首次启动**：设备以 AP 模式启动，OLED 显示 `Config Mode`。
2. **连接热点**：手机 / 电脑连接 WiFi `StockMonitor`，密码 `12345678`。
3. **打开配置页**：浏览器访问 `192.168.4.1`，填写家庭 WiFi 名称、密码以及最多 5 只自选代码，点击"保存并连接"。
4. **正常工作**：设备重启后自动连接 WiFi，开始拉取行情并在 OLED 上滚动显示。
5. **切换标的**：旋转编码器左右旋转可切换当前显示的标的。
6. **重新配置**：长按 BOOT 按钮（GPIO 0）2 秒，或按下编码器按钮，设备重新进入 AP 配置模式。

### 支持的股票代码格式

| 类型 | 示例 |
|------|------|
| 美股 | `AAPL`、`TSLA`、`NVDA` |
| 加密货币 | `BINANCE:BTCUSDT`、`BINANCE:ETHUSDT`、`BINANCE:SOLUSDT` |

---

## 注意事项

- Finnhub 免费套餐每分钟 API 调用有次数限制，监控标的越多消耗越快，建议不超过 5 只。
- 加密货币代码需使用 Finnhub 支持的交易所前缀格式（如 `BINANCE:`）。
- 上传固件前务必先执行 **Upload Filesystem Image**，否则 Web 页面无法加载。

---

## 项目结构

```
Trade on Table/
├── src/
│   └── main.cpp          # 主程序
├── data/
│   ├── index.html        # Web 配置页面
│   ├── style.css         # 页面样式
│   └── success.html      # 保存成功提示页
├── include/
├── lib/
└── platformio.ini        # PlatformIO 工程配置
```

---

## License

MIT

---
---

<a name="english"></a>

# English

A desktop stock / cryptocurrency real-time ticker based on ESP32. It scrolls live price and change data for up to 5 watchlist symbols on an OLED display, and provides a browser-based configuration page — no code changes needed to set up Wi-Fi or update your watchlist.

---

## Features

- **Live quotes**: Price refreshes every 15 seconds via the [Finnhub](https://finnhub.io/) API
- **Multi-symbol**: Monitor up to 5 symbols simultaneously (stocks and/or crypto)
- **Price history**: Stores the last 30 price points locally for a simple trend chart
- **OLED display**: 128×64 SSD1306 screen showing symbol, price, and change %
- **Rotary encoder**: KY-040 encoder to scroll through symbols; press encoder button or long-press BOOT to enter config mode
- **Web config**: Device starts as a Wi-Fi AP (`StockMonitor` / `12345678`) with a config page; saves settings and reboots automatically
- **Persistent storage**: Config saved in ESP32 NVS Flash, survives power cycles

---

## Bill of Materials (BOM)

| # | Component | Specification | Qty | Notes |
|---|-----------|---------------|-----|-------|
| 1 | ESP32 Dev Board | ESP32-DevKitC (or compatible) | 1 | 38-pin version recommended |
| 2 | OLED Display Module | 0.96" SSD1306, I²C, 128×64, 3.3V/5V | 1 | 4-pin I²C interface |
| 3 | Rotary Encoder | KY-040 (with push button) | 1 | — |
| 4 | Breadboard | 400-tie or 830-tie | 1 | Optional — or use custom PCB |
| 5 | Jumper Wires | Male-to-male / male-to-female, ~10 cm | Several | For connecting modules |
| 6 | USB Cable | Micro-USB (match your dev board) | 1 | Power + flashing |
| 7 | 3D Printed Enclosure | ESP32 + 0.96" OLED case (see section below) | 1 | Optional — PLA |

---

## 3D Printed Enclosure

This project uses the following Thingiverse model as the device enclosure:

> **[ESP 32 case enclosure + 0.96 inch oled window](https://www.thingiverse.com/thing:4331477)**  
> Designer: [23Ro](https://www.thingiverse.com/23Ro)  
> License: GNU GPL  
> URL: https://www.thingiverse.com/thing:4331477

The enclosure is designed specifically for the ESP32 dev board with a 0.96" OLED display. It features an OLED window cutout and access holes for the Reset and BOOT buttons, making reprogramming convenient.

**Recommended print settings (from original designer):**

| Setting | Value |
|---------|-------|
| Layer height | 0.175 mm |
| Infill | 20% |
| Raft | Yes |
| Supports | No |
| Filament | PLA |

---

## Wiring

### OLED (I²C)

| OLED Pin | ESP32 Pin |
|----------|-----------|
| VCC | 3.3V |
| GND | GND |
| SDA | GPIO 21 |
| SCL | GPIO 22 |

### KY-040 Rotary Encoder

| Encoder Pin | ESP32 Pin |
|-------------|-----------|
| CLK | GPIO 34 |
| DT | GPIO 35 |
| SW (button) | GPIO 32 |
| + / VCC | 3.3V |
| GND | GND |

---

## Software Dependencies

| Library | Version |
|---------|---------|
| Adafruit SSD1306 | ^2.5.7 |
| Adafruit GFX Library | ^1.11.5 |
| ArduinoJson | ^7.0.0 |

> All dependencies are declared in `platformio.ini` and will be installed automatically by PlatformIO.

---

## Getting Started

### 1. Clone / download the project

```bash
git clone <repo-url>
```

### 2. Open with PlatformIO

Install the **PlatformIO IDE** extension in VS Code, then open this project folder.

### 3. Set your API key

Edit `src/main.cpp` and replace `API_KEY` with your own [Finnhub API Key](https://finnhub.io/register):

```cpp
const char* API_KEY = "your_api_key_here";
```

### 4. Upload the filesystem

Upload the web assets in `data/` to SPIFFS first:

```
PlatformIO: Upload Filesystem Image
```

### 5. Build & flash

```
PlatformIO: Upload
```

---

## Usage

1. **First boot**: Device starts in AP mode; OLED shows `Config Mode`.
2. **Connect to hotspot**: Join Wi-Fi `StockMonitor`, password `12345678`.
3. **Open config page**: Navigate to `192.168.4.1`, enter your home Wi-Fi credentials and up to 5 ticker symbols, then click "Save & Connect".
4. **Normal operation**: Device reboots, connects to Wi-Fi, and begins displaying live quotes on the OLED.
5. **Switch symbol**: Rotate the encoder left/right to cycle through your watchlist.
6. **Re-configure**: Long-press the BOOT button (GPIO 0) for 2 seconds, or press the encoder button, to re-enter AP config mode.

### Supported ticker formats

| Type | Example |
|------|---------|
| US Stocks | `AAPL`, `TSLA`, `NVDA` |
| Crypto | `BINANCE:BTCUSDT`, `BINANCE:ETHUSDT`, `BINANCE:SOLUSDT` |

---

## Notes

- The Finnhub free tier has a per-minute API call limit. The more symbols you monitor, the faster it is consumed — keep it to 5 or fewer.
- Crypto symbols must use the exchange-prefix format supported by Finnhub (e.g. `BINANCE:`).
- Always run **Upload Filesystem Image** before flashing the firmware, otherwise the web config page will not load.

---

## Project Structure

```
Trade on Table/
├── src/
│   └── main.cpp          # Main application
├── data/
│   ├── index.html        # Web config page
│   ├── style.css         # Page stylesheet
│   └── success.html      # Save-success page
├── include/
├── lib/
└── platformio.ini        # PlatformIO project config
```

---

## License

MIT
