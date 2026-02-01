/**
 * ESP32 股票监控器
 * 
 * 功能：
 *   - Web 配置页面（WiFi + 股票代码）
 *   - OLED 显示股票实时价格
 *   - 使用 Finnhub API
 * 
 * 接线:
 *   OLED VCC → 3.3V
 *   OLED GND → GND
 *   OLED SDA → GPIO21
 *   OLED SCL → GPIO22
 */

#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SPIFFS.h>

// OLED 配置
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Finnhub API Key
const char* API_KEY = "d5ucekpr01qr4f88n4n0d5ucekpr01qr4f88n4ng";

// AP 模式配置
const char* AP_SSID = "StockMonitor";
const char* AP_PASS = "12345678";

// 存储配置
Preferences prefs;
String wifiSSID = "";
String wifiPass = "";
String stocks[5] = {"", "", "", "", ""};
int stockCount = 0;

// Web 服务器
WebServer server(80);

// 股票数据
struct StockData {
  String symbol;
  float price;
  float change;
  float changePercent;
  float priceHistory[30];  // 存储30个历史价格用于K线
  int historyCount;
  bool valid;
};
StockData stockData[5];

int currentStock = 0;
unsigned long lastUpdate = 0;
const unsigned long UPDATE_INTERVAL = 15000; // 15秒更新一次

bool configMode = false;

// KY-040 旋转编码器控制
#define ENC_CLK 34   // 编码器 CLK
#define ENC_DT  35   // 编码器 DT
#define ENC_SW  32   // 编码器按钮
volatile int encoderPos = 0;  // 编码器位置计数
int lastEncoderPos = 0;
unsigned long lastEncoderTime = 0;
const unsigned long DEBOUNCE_TIME = 50; // KY-040 需要较长防抖时间

// 按钮控制 (编码器按钮 + BOOT按钮)
#define BTN_PIN 0  // BOOT 按钮
unsigned long btnPressTime = 0;
bool btnPressed = false;
unsigned long encBtnPressTime = 0;
bool encBtnPressed = false;
const unsigned long LONG_PRESS = 2000; // 长按2秒进入配置

// 从 SPIFFS 读取文件
String readFile(const char* path) {
  File file = SPIFFS.open(path, "r");
  if (!file) {
    Serial.printf("Failed to open %s\n", path);
    return "";
  }
  String content = file.readString();
  file.close();
  return content;
}

void setupOLED() {
  Wire.begin(21, 22);
  Wire.setClock(100000);
  
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED init failed");
    return;
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.display();
}

void showMessage(const char* line1, const char* line2 = "", const char* line3 = "") {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(line1);
  if (strlen(line2) > 0) {
    display.setCursor(0, 20);
    display.println(line2);
  }
  if (strlen(line3) > 0) {
    display.setCursor(0, 40);
    display.println(line3);
  }
  display.display();
}

void loadConfig() {
  prefs.begin("stock", true);
  wifiSSID = prefs.getString("ssid", "");
  wifiPass = prefs.getString("pass", "");
  stocks[0] = prefs.getString("s1", "");
  stocks[1] = prefs.getString("s2", "");
  stocks[2] = prefs.getString("s3", "");
  stocks[3] = prefs.getString("s4", "");
  stocks[4] = prefs.getString("s5", "");
  prefs.end();
  
  stockCount = 0;
  for (int i = 0; i < 5; i++) {
    if (stocks[i].length() > 0) {
      stockCount++;
    }
  }
}

void saveConfig() {
  prefs.begin("stock", false);
  prefs.putString("ssid", wifiSSID);
  prefs.putString("pass", wifiPass);
  prefs.putString("s1", stocks[0]);
  prefs.putString("s2", stocks[1]);
  prefs.putString("s3", stocks[2]);
  prefs.putString("s4", stocks[3]);
  prefs.putString("s5", stocks[4]);
  prefs.end();
}

void handleRoot() {
  String html = readFile("/index.html");
  if (html.length() > 0) {
    server.send(200, "text/html", html);
  } else {
    server.send(200, "text/html", "<h1>Stock Monitor</h1><p>SPIFFS Error</p>");
  }
}

void handleStyle() {
  String css = readFile("/style.css");
  if (css.length() > 0) {
    server.send(200, "text/css", css);
  } else {
    server.send(404, "text/plain", "Not found");
  }
}

void handleSave() {
  wifiSSID = server.arg("ssid");
  wifiPass = server.arg("pass");
  stocks[0] = server.arg("stock1");
  stocks[1] = server.arg("stock2");
  stocks[2] = server.arg("stock3");
  stocks[3] = server.arg("stock4");
  stocks[4] = server.arg("stock5");
  
  // 转大写
  for (int i = 0; i < 5; i++) {
    stocks[i].toUpperCase();
    stocks[i].trim();
  }
  
  saveConfig();
  
  String html = readFile("/success.html");
  if (html.length() > 0) {
    server.send(200, "text/html", html);
  } else {
    server.send(200, "text/html", "<h1>Success!</h1>");
  }
  delay(2000);
  ESP.restart();
}

void startAPMode() {
  configMode = true;
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  
  server.on("/", handleRoot);
  server.on("/style.css", handleStyle);
  server.on("/save", HTTP_POST, handleSave);
  server.begin();
  
  Serial.println("AP Mode: " + String(AP_SSID));
  Serial.println("IP: 192.168.4.1");
  
  showMessage("Config Mode", "WiFi: StockMonitor", "Open 192.168.4.1");
}

bool connectWiFi() {
  if (wifiSSID.length() == 0) return false;
  
  showMessage("Connecting WiFi...", wifiSSID.c_str());
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifiSSID.c_str(), wifiPass.c_str());
  
  int timeout = 20;
  while (WiFi.status() != WL_CONNECTED && timeout > 0) {
    delay(500);
    Serial.print(".");
    timeout--;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected: " + WiFi.localIP().toString());
    showMessage("WiFi Connected!", WiFi.localIP().toString().c_str());
    delay(1000);
    return true;
  }
  
  Serial.println("\nWiFi Failed");
  return false;
}

void fetchStockData(int index) {
  if (stocks[index].length() == 0) {
    stockData[index].valid = false;
    return;
  }
  
  HTTPClient http;
  String url = "https://finnhub.io/api/v1/quote?symbol=" + stocks[index] + "&token=" + API_KEY;
  
  http.begin(url);
  int httpCode = http.GET();
  
  if (httpCode == 200) {
    String payload = http.getString();
    
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, payload);
    
    if (!error) {
      stockData[index].symbol = stocks[index];
      float newPrice = doc["c"].as<float>();
      stockData[index].change = doc["d"].as<float>();
      stockData[index].changePercent = doc["dp"].as<float>();
      stockData[index].valid = (newPrice > 0);
      
      // 保存历史价格用于K线
      if (stockData[index].valid) {
        // 移动历史数据
        if (stockData[index].historyCount >= 30) {
          for (int i = 0; i < 29; i++) {
            stockData[index].priceHistory[i] = stockData[index].priceHistory[i + 1];
          }
          stockData[index].priceHistory[29] = newPrice;
        } else {
          stockData[index].priceHistory[stockData[index].historyCount] = newPrice;
          stockData[index].historyCount++;
        }
        stockData[index].price = newPrice;
      }
    }
  } else {
    stockData[index].valid = false;
  }
  
  http.end();
}

// 绘制迷你 K 线图 (无边框)
void drawMiniChart(int index, int x, int y, int w, int h) {
  if (stockData[index].historyCount < 2) {
    display.setCursor(x, y + h/2 - 4);
    display.setTextSize(1);
    display.print("Wait...");
    return;
  }
  
  // 找最大最小值
  float minPrice = stockData[index].priceHistory[0];
  float maxPrice = stockData[index].priceHistory[0];
  for (int i = 1; i < stockData[index].historyCount; i++) {
    if (stockData[index].priceHistory[i] < minPrice) minPrice = stockData[index].priceHistory[i];
    if (stockData[index].priceHistory[i] > maxPrice) maxPrice = stockData[index].priceHistory[i];
  }
  
  // 防止除零
  float range = maxPrice - minPrice;
  if (range < 0.01) range = 0.01;
  
  // 绘制折线 (无边框)
  int count = stockData[index].historyCount;
  float xStep = (float)(w - 2) / (count - 1);
  
  for (int i = 1; i < count; i++) {
    int x1 = x + (int)((i - 1) * xStep);
    int x2 = x + (int)(i * xStep);
    int y1 = y + h - 1 - (int)((stockData[index].priceHistory[i - 1] - minPrice) / range * (h - 2));
    int y2 = y + h - 1 - (int)((stockData[index].priceHistory[i] - minPrice) / range * (h - 2));
    display.drawLine(x1, y1, x2, y2, SSD1306_WHITE);
  }
}

// 获取显示用的符号名（去掉交易所前缀）
String getDisplaySymbol(String symbol) {
  int colonPos = symbol.indexOf(':');
  if (colonPos > 0) {
    return symbol.substring(colonPos + 1);
  }
  return symbol;
}

// 单只股票详情页面（带 K 线）
void displaySingleStock(int index) {
  if (stocks[index].length() == 0 || !stockData[index].valid) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("Stock ");
    display.print(index + 1);
    display.println(": No Data");
    display.display();
    return;
  }
  
  display.clearDisplay();
  
  // 股票代码 (左上) - 去掉交易所前缀
  String displaySymbol = getDisplaySymbol(stockData[index].symbol);
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.print(displaySymbol);
  
  // 涨跌箭头 (代码右侧)
  int symbolWidth = displaySymbol.length() * 12;  // 每个字符约12像素宽
  display.setTextSize(1);
  if (stockData[index].changePercent > 0) {
    // 上涨箭头 ▲
    display.setCursor(symbolWidth + 4, 4);
    display.write(0x18);  // 或用 drawTriangle
    display.drawTriangle(
      symbolWidth + 6, 2,       // 顶点
      symbolWidth + 2, 10,      // 左下
      symbolWidth + 10, 10,     // 右下
      SSD1306_WHITE
    );
  } else if (stockData[index].changePercent < 0) {
    // 下跌箭头 ▼
    display.drawTriangle(
      symbolWidth + 6, 10,      // 底点
      symbolWidth + 2, 2,       // 左上
      symbolWidth + 10, 2,      // 右上
      SSD1306_WHITE
    );
  }
  
  // 页码 (右上角)
  display.setTextSize(1);
  display.setCursor(110, 0);
  display.print(index + 1);
  display.print("/");
  display.print(stockCount);
  
  // 当前价格 (左侧)
  display.setTextSize(1);
  display.setCursor(0, 22);
  display.print("$");
  display.setTextSize(2);
  if (stockData[index].price >= 10000) {
    display.print(stockData[index].price, 0);
  } else if (stockData[index].price >= 1000) {
    display.print(stockData[index].price, 1);
  } else {
    display.print(stockData[index].price, 2);
  }
  
  // 涨跌幅 (左下)
  display.setTextSize(1);
  display.setCursor(0, 44);
  if (stockData[index].changePercent >= 0) {
    display.print("+");
  }
  display.print(stockData[index].changePercent, 2);
  display.print("%");
  
  // 涨跌额
  display.setCursor(0, 56);
  if (stockData[index].change >= 0) {
    display.print("+");
  }
  if (abs(stockData[index].change) >= 1000) {
    display.print(stockData[index].change, 0);
  } else {
    display.print(stockData[index].change, 2);
  }
  
  // 绘制 K 线图 (右侧区域)
  drawMiniChart(index, 75, 20, 52, 42);
  
  display.display();
}

void displayStock(int index) {
  // 改为列表模式显示
  display.clearDisplay();
  
  // 标题栏
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("SYMBOL");
  display.setCursor(48, 0);
  display.print("PRICE");
  display.setCursor(93, 0);
  display.print("CHG%");
  
  // 分隔线
  display.drawLine(0, 9, 127, 9, SSD1306_WHITE);
  
  // 显示股票列表
  int y = 12;
  
  for (int i = 0; i < 5; i++) {
    if (stocks[i].length() == 0) continue;
    
    display.setCursor(0, y);
    
    // 股票代码
    String sym = stocks[i];
    if (sym.length() > 5) sym = sym.substring(0, 5);
    display.print(sym);
    
    if (stockData[i].valid) {
      // 价格
      display.setCursor(42, y);
      if (stockData[i].price >= 1000) {
        display.print(stockData[i].price, 0);
      } else if (stockData[i].price >= 100) {
        display.print(stockData[i].price, 1);
      } else {
        display.print(stockData[i].price, 2);
      }
      
      // 涨跌幅
      display.setCursor(88, y);
      if (stockData[i].changePercent >= 0) {
        display.print("+");
      }
      display.print(stockData[i].changePercent, 1);
    } else {
      display.setCursor(50, y);
      display.print("--");
    }
    
    y += 10;
  }
  
  // 底部状态
  display.drawLine(0, 54, 127, 54, SSD1306_WHITE);
  display.setCursor(0, 56);
  unsigned long elapsed = (millis() - lastUpdate) / 1000;
  display.print("Updated ");
  display.print(elapsed);
  display.print("s ago");
  
  display.display();
}

// 列表模式显示
void displayStockList() {
  displayStock(0);
}

// 详情模式显示单只股票
void displayStockDetail(int index) {
  if (stocks[index].length() == 0 || !stockData[index].valid) {
    showMessage(stocks[index].c_str(), "No Data");
    return;
  }
  
  display.clearDisplay();
  
  // 股票代码 - 大号
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.print(stockData[index].symbol);
  
  // 涨跌箭头
  display.setCursor(90, 0);
  if (stockData[index].change >= 0) {
    display.print("UP");
  } else {
    display.print("DN");
  }
  
  // 价格 - 大号
  display.setTextSize(2);
  display.setCursor(0, 22);
  display.print("$");
  if (stockData[index].price >= 1000) {
    display.print(stockData[index].price, 1);
  } else {
    display.print(stockData[index].price, 2);
  }
  
  // 涨跌金额
  display.setTextSize(1);
  display.setCursor(0, 45);
  if (stockData[index].change >= 0) {
    display.print("+");
  }
  display.print(stockData[index].change, 2);
  
  // 涨跌百分比
  display.setCursor(55, 45);
  display.print("(");
  if (stockData[index].changePercent >= 0) {
    display.print("+");
  }
  display.print(stockData[index].changePercent, 2);
  display.print("%)");
  
  // 页码指示
  display.setCursor(0, 56);
  display.print(index + 1);
  display.print("/");
  display.print(stockCount);
  display.print(" [BTN:Next]");
  
  display.display();
}

void setup() {
  Serial.begin(115200);
  delay(200);
  
  // 初始化 SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS mount failed");
  }
  
  setupOLED();
  showMessage("Starting...");
  
  loadConfig();
  
  // 初始化引脚
  pinMode(BTN_PIN, INPUT_PULLUP);  // BOOT 按钮
  pinMode(ENC_CLK, INPUT_PULLUP);  // 编码器 CLK
  pinMode(ENC_DT, INPUT_PULLUP);   // 编码器 DT
  pinMode(ENC_SW, INPUT_PULLUP);   // 编码器按钮
  
  // 如果没有配置或长按 BOOT 键，进入配置模式
  if (wifiSSID.length() == 0 || digitalRead(BTN_PIN) == LOW) {
    startAPMode();
    return;
  }
  
  if (!connectWiFi()) {
    startAPMode();
    return;
  }
  
  showMessage("Fetching data...");
  
  // 初始化历史数据
  for (int i = 0; i < 5; i++) {
    stockData[i].historyCount = 0;
  }
  
  // 初始获取所有股票数据
  for (int i = 0; i < 5; i++) {
    if (stocks[i].length() > 0) {
      fetchStockData(i);
    }
  }
}

// 切换到下一只股票
void nextStock() {
  if (stockCount == 0) return;
  int start = currentStock;
  do {
    currentStock = (currentStock + 1) % 5;
    if (stocks[currentStock].length() > 0) break;
  } while (currentStock != start);
}

// 切换到上一只股票
void prevStock() {
  if (stockCount == 0) return;
  int start = currentStock;
  do {
    currentStock = (currentStock + 4) % 5;  // +4 等于 -1
    if (stocks[currentStock].length() > 0) break;
  } while (currentStock != start);
}

// KY-040 编码器读取 (简单稳定版)
void readEncoder() {
  static int lastCLK = HIGH;
  
  int currentCLK = digitalRead(ENC_CLK);
  
  // 只在 CLK 下降沿时检测
  if (lastCLK == HIGH && currentCLK == LOW) {
    // 读取 DT 判断方向
    if (digitalRead(ENC_DT) == HIGH) {
      // DT 高电平 = 顺时针
      nextStock();
      Serial.println("CW -> Next");
    } else {
      // DT 低电平 = 逆时针
      prevStock();
      Serial.println("CCW -> Prev");
    }
  }
  
  lastCLK = currentCLK;
}

void loop() {
  static int lastDisplayedStock = -1;
  static unsigned long lastDisplayUpdate = 0;
  
  if (configMode) {
    server.handleClient();
    return;
  }
  
  // 更新数据
  bool dataUpdated = false;
  if (millis() - lastUpdate > UPDATE_INTERVAL) {
    for (int i = 0; i < 5; i++) {
      if (stocks[i].length() > 0) {
        fetchStockData(i);
      }
    }
    lastUpdate = millis();
    dataUpdated = true;
  }
  
  // 读取旋转编码器
  readEncoder();
  
  // 编码器按钮检测
  if (digitalRead(ENC_SW) == LOW) {
    if (!encBtnPressed) {
      encBtnPressed = true;
      encBtnPressTime = millis();
    } else if (millis() - encBtnPressTime > LONG_PRESS) {
      // 长按进入配置模式
      showMessage("Entering", "Config Mode...");
      delay(1000);
      ESP.restart();
    }
  } else {
    encBtnPressed = false;
  }
  
  // BOOT 按钮检测 (备用)
  if (digitalRead(BTN_PIN) == LOW) {
    if (!btnPressed) {
      btnPressed = true;
      btnPressTime = millis();
    } else if (millis() - btnPressTime > LONG_PRESS) {
      showMessage("Entering", "Config Mode...");
      delay(1000);
      ESP.restart();
    }
  } else {
    if (btnPressed && millis() - btnPressTime < LONG_PRESS) {
      nextStock();
    }
    btnPressed = false;
  }
  
  // 只在切换股票或数据更新时才刷新显示
  if (stockCount > 0) {
    if (currentStock != lastDisplayedStock || dataUpdated || millis() - lastDisplayUpdate > 1000) {
      displaySingleStock(currentStock);
      lastDisplayedStock = currentStock;
      lastDisplayUpdate = millis();
    }
  } else {
    showMessage("No stocks", "configured", "Hold btn to setup");
  }
  
  // 不用 delay，让循环尽可能快
}