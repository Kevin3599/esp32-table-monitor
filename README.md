# Trade on Table — ESP32 桌面股票监控器

一款基于 ESP32 的桌面股票 / 加密货币实时行情监控设备，通过 OLED 屏幕滚动显示多只自选标的的实时价格与涨跌幅，并提供 Web 配置页面，无需修改代码即可灵活配置 WiFi 和自选列表。

---
<img width="2386" height="1282" alt="b5c41e2c875e5b54b2347e4ab81741da" src="https://github.com/user-attachments/assets/5acec394-4efe-475e-b914-41b8f6882c0d" />

## 功能特性

- **实时行情**：通过 [Finnhub](https://finnhub.io/) API 每 15 秒刷新一次股票 / 加密货币价格
- **多标的支持**：最多同时监控 5 只标的（股票 + 加密货币均可）
- **K 线历史**：本地缓存最近 30 个价格点，用于简易走势展示
- **OLED 显示**：128×64 SSD1306 屏幕，清晰显示代码、价格、涨跌幅
- **旋转编码器**：KY-040 编码器旋转切换标的，按下编码器按钮或长按 BOOT 按钮进入配置模式
- **Web 配置**：设备开机后以 AP 热点（`StockMonitor` / `12345678`）提供配置页面，扫描连接后填写 WiFi 和股票代码，保存后自动重启
- **掉电记忆**：配置信息保存在 ESP32 NVS Flash，断电不丢失

---

## 硬件清单

| 元件 | 规格 | 备注 |
|------|------|------|
| 主控板 | ESP32 开发板（ESP32-DevKitC 或兼容） | — |
| OLED 屏幕 | 0.96" / 1.3" SSD1306，I²C，128×64 | — |
| 旋转编码器 | KY-040 | 含按钮 |

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
| VCC | 3.3V |
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

- Finnhub 免费套餐每分钟有 API 调用次数限制，监控标的越多消耗越快，建议不超过 5 只。
- 加密货币代码需使用 Finnhub 支持的交易所前缀格式（如 `BINANCE:`）。
- 上传固件前务必先执行 **Upload Filesystem Image**，否则 Web 页面无法加载。

---

## License

MIT
