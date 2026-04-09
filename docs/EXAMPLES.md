# Examples 應用範例

## 範例 1：基本位移追蹤

```python
import serial
import json
import time


def send_command(ser, command, wait=0.1, max_lines=4):
    ser.write((command + "\r\n").encode())
    time.sleep(wait)
    lines = []
    for _ in range(max_lines):
        line = ser.readline().decode(errors="ignore").strip()
        if line:
            lines.append(line)
    return lines

# 開啟串口
ser = serial.Serial('COM3', 115200, timeout=0.5)

# 測試連線
print(send_command(ser, 'at'))

# 重置並設定零點（會回應 '.' 與 'ok'）
print(send_command(ser, 'at0g+', wait=0.4))

# 啟用 JSON 輸出
print(send_command(ser, 'atj+'))

# 設定輸出間隔 100ms
print(send_command(ser, 'atrpt+100'))

# 啟用座標模式
print(send_command(ser, 'atx+'))

# 讀取位置數據
while True:
    line = ser.readline().decode(errors='ignore').strip()
    if line.startswith('{'):
        data = json.loads(line)
        print(
            f"X={data['X']:.1f} cm, Y={data['Y']:.1f} cm, "
            f"A={data['A']:.1f}°, cA={data['cA']:.1f}°"
        )
```

## 範例 2：羅盤校正程序

```python
import serial
import time

ser = serial.Serial('COM3', 115200, timeout=1)

# 啟動校正
ser.write(b'at0c+8\r\n')
print("開始校正，請緩慢旋轉模組...")

# 立即回應：開始提示
print(ser.readline().decode(errors='ignore').strip())

# 韌體會在校正完成前阻塞約 20 秒，完成後才回傳結果
ser.timeout = 25
print(ser.readline().decode(errors='ignore').strip())
ser.timeout = 1

# 查詢校正結果
ser.write(b'ati+c\r\n')
time.sleep(0.1)
for _ in range(4):
  line = ser.readline().decode(errors='ignore').strip()
  if line:
    print(line)
```

## 範例 3：機器人導航應用

```python
import json
import math
import serial
import time


def send_command(ser, command, wait=0.1, max_lines=4):
  ser.write((command + "\r\n").encode())
  time.sleep(wait)
  lines = []
  for _ in range(max_lines):
    line = ser.readline().decode(errors="ignore").strip()
    if line:
      lines.append(line)
  return lines


class PositionTracker:
  def __init__(self, port="COM3"):
    self.ser = serial.Serial(port, 115200, timeout=0.5)
    self.init_sensor()

  def init_sensor(self):
    print(send_command(self.ser, "at"))
    print(send_command(self.ser, "at0g+", wait=0.4))
    print(send_command(self.ser, "atj+"))
    print(send_command(self.ser, "atrpt+100"))
    print(send_command(self.ser, "atc+"))

  def get_position(self):
    line = self.ser.readline().decode(errors="ignore").strip()
    if line.startswith("{"):
      return json.loads(line)
    return None

  def navigate_to(self, target_x, target_y):
    while True:
      pos = self.get_position()
      if pos is None:
        continue

      dx = target_x - pos["X"]
      dy = target_y - pos["Y"]
      distance = math.sqrt(dx ** 2 + dy ** 2)
      target_angle = math.degrees(math.atan2(dy, dx))

      angle_diff = target_angle - pos["A"]
      if angle_diff > 180:
        angle_diff -= 360
      elif angle_diff < -180:
        angle_diff += 360

      print(
        f"Distance: {distance:.1f} cm, Angle diff: {angle_diff:.1f}°, "
        f"Compass: {pos['cA']:.1f}°"
      )

      if distance < 5:
        print("目標已到達！")
        break

      # control_motors(distance, angle_diff)


tracker = PositionTracker("COM3")
tracker.navigate_to(100, 50)
```

**說明**：此範例使用 UART/USB 的 `ATC+` 輸出，若要使用 Hub5168+ 做 I2C 主控測試，請改用 `test_hub5168_i2c/test_hub5168_i2c.ino` 的 `ssr`、`a+`、`cd`、`ca` 等命令流程。

## 範例 4：Arduino UART + OLED 顯示

本範例展示如何使用 Arduino 透過 UART 連接 HUB-PAA5163D 模組，並將位置資訊顯示在 OLED 螢幕上。

**硬體連接**：

- HUB-PAA5163D PA2 (TX) → Arduino RX (Pin 10)
- HUB-PAA5163D PA3 (RX) → Arduino TX (Pin 11)
- HUB-PAA5163D GND → Arduino GND
- OLED SDA → Arduino A4 (SDA)
- OLED SCL → Arduino A5 (SCL)
- OLED VCC → Arduino 3.3V 或 5V
- OLED GND → Arduino GND

**所需函式庫**：

- `SoftwareSerial`：Arduino 內建
- `Adafruit_SSD1306`：OLED 顯示
- `Adafruit_GFX`：圖形庫

```cpp
#include <SoftwareSerial.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// OLED 顯示器設定
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// HUB-PAA5163D 軟體串口設定
#define RX_PIN 10
#define TX_PIN 11
SoftwareSerial hubSerial(RX_PIN, TX_PIN);

// 數據結構
struct SensorData {
  float x;
  float y;
  float angle;
  float compassAngle;
  float roll;
  float pitch;
  bool valid;
};

SensorData currentData = {0, 0, 0, 0, 0, 0, false};
String inputBuffer = "";

void setup() {
  Serial.begin(115200);  // 除錯用
  hubSerial.begin(115200);  // HUB-PAA5163D 通訊
  
  // 初始化 OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("OLED init failed!"));
    while(1);
  }
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("HUB-PAA5163D"));
  display.println(F("Initializing..."));
  display.display();
  
  delay(2000);
  
  // 初始化 HUB-PAA5163D
  initHub5163();
  
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println(F("Ready!"));
  display.display();
  delay(1000);
}

void loop() {
  // 讀取 HUB-PAA5163D 數據
  while (hubSerial.available()) {
    char c = hubSerial.read();
  
    if (c == '\n' || c == '\r') {
      if (inputBuffer.length() > 0) {
        parseData(inputBuffer);
        inputBuffer = "";
      }
    } else {
      inputBuffer += c;
    }
  }
  
  // 更新顯示
  if (currentData.valid) {
    updateDisplay();
  }
  
  delay(50);
}

void initHub5163() {
  // 確保 PA3 (RX) 為高電平以啟用 USART1
  // 注意：需要在硬體上將 PA3 透過電阻上拉至 3.3V
  
  delay(500);
  
  // 測試連線
  hubSerial.println("at");
  delay(100);
  
  // 重置並設定零點
  hubSerial.println("at0g+");
  delay(1000);
  
  // 啟用 JSON 輸出
  hubSerial.println("atj+");
  delay(100);
  
  // 啟用完整模式（含 Roll/Pitch）
  hubSerial.println("atc+");
  delay(100);
  
  // 設定更新間隔為 100ms
  hubSerial.println("atrpt+100");
  delay(100);
  
  Serial.println("HUB-PAA5163D initialized");
}

void parseData(String data) {
  // 解析 JSON 格式: {"A":45.2,"X":12.5,"Y":-8.3,"R":2.1,"P":-1.5,"cA":92.3,"tA":89.8}
  if (data.startsWith("{") && data.endsWith("}")) {
    // 簡易 JSON 解析（生產環境建議使用 ArduinoJson 函式庫）
    currentData.angle = extractValue(data, "\"A\":");
    currentData.x = extractValue(data, "\"X\":");
    currentData.y = extractValue(data, "\"Y\":");
    currentData.roll = extractValue(data, "\"R\":");
    currentData.pitch = extractValue(data, "\"P\":");
    currentData.compassAngle = extractValue(data, "\"cA\":");
    currentData.valid = true;
  
    // 除錯輸出
    Serial.print("X:");
    Serial.print(currentData.x);
    Serial.print(" Y:");
    Serial.print(currentData.y);
    Serial.print(" A:");
    Serial.println(currentData.angle);
  } else {
    // 解析文字格式: A=45.2 X=12.5 Y=-8.3 R=2.1 P=-1.5 cA=92.3
    currentData.angle = extractTextValue(data, "A=");
    currentData.x = extractTextValue(data, "X=");
    currentData.y = extractTextValue(data, "Y=");
    currentData.roll = extractTextValue(data, "R=");
    currentData.pitch = extractTextValue(data, "P=");
    currentData.compassAngle = extractTextValue(data, "cA=");
    currentData.valid = true;
  }
}

float extractValue(String json, String key) {
  int pos = json.indexOf(key);
  if (pos == -1) return 0.0;
  
  pos += key.length();
  int endPos = json.indexOf(',', pos);
  if (endPos == -1) {
    endPos = json.indexOf('}', pos);
  }
  
  String value = json.substring(pos, endPos);
  return value.toFloat();
}

float extractTextValue(String text, String key) {
  int pos = text.indexOf(key);
  if (pos == -1) return 0.0;
  
  pos += key.length();
  int endPos = text.indexOf(' ', pos);
  if (endPos == -1) {
    endPos = text.length();
  }
  
  String value = text.substring(pos, endPos);
  return value.toFloat();
}

void updateDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  // 標題
  display.setCursor(0, 0);
  display.println(F("=== HUB-PAA5163D ==="));
  
  // 座標資訊
  display.setCursor(0, 12);
  display.print(F("X: "));
  display.print(currentData.x, 1);
  display.println(F(" cm"));
  
  display.setCursor(0, 22);
  display.print(F("Y: "));
  display.print(currentData.y, 1);
  display.println(F(" cm"));
  
  // 角度資訊
  display.setCursor(0, 32);
  display.print(F("Angle: "));
  display.print(currentData.angle, 1);
  display.println(F(" deg"));
  
  // 羅盤角度
  display.setCursor(0, 42);
  display.print(F("Compass: "));
  display.print(currentData.compassAngle, 1);
  display.println(F(" deg"));
  
  // Roll & Pitch（小字體）
  display.setCursor(0, 52);
  display.print(F("R:"));
  display.print(currentData.roll, 1);
  display.print(F(" P:"));
  display.print(currentData.pitch, 1);
  
  // 繪製方向指示箭頭（右上角）
  drawDirectionArrow(110, 16, currentData.compassAngle);
  
  display.display();
}

void drawDirectionArrow(int cx, int cy, float angle) {
  // 將角度轉換為弧度（North = 0度）
  float rad = (angle - 90) * PI / 180.0;
  
  // 箭頭長度
  int len = 12;
  
  // 計算箭頭終點
  int x1 = cx + len * cos(rad);
  int y1 = cy + len * sin(rad);
  
  // 繪製圓圈
  display.drawCircle(cx, cy, 14, SSD1306_WHITE);
  
  // 繪製箭頭主線
  display.drawLine(cx, cy, x1, y1, SSD1306_WHITE);
  
  // 繪製箭頭尖端
  float arrowAngle = 25 * PI / 180.0;  // 箭頭角度
  int arrowLen = 4;
  
  float angle1 = rad + PI - arrowAngle;
  float angle2 = rad + PI + arrowAngle;
  
  int ax1 = x1 + arrowLen * cos(angle1);
  int ay1 = y1 + arrowLen * sin(angle1);
  int ax2 = x1 + arrowLen * cos(angle2);
  int ay2 = y1 + arrowLen * sin(angle2);
  
  display.drawLine(x1, y1, ax1, ay1, SSD1306_WHITE);
  display.drawLine(x1, y1, ax2, ay2, SSD1306_WHITE);
  
  // 標記北方 "N"
  display.setCursor(cx - 3, cy - 22);
  display.setTextSize(1);
  display.print("N");
}
```

**使用說明**：

1. **硬體準備**：

   - 使用 10kΩ 電阻將 HUB-PAA5163D 的 PA3 (RX) 上拉至 3.3V
   - 確認所有 GND 接地共地
   - OLED 使用 I2C 介面（地址通常為 0x3C）
2. **函式庫安裝**：

   ```
   Arduino IDE -> 工具 -> 管理函式庫
   搜尋並安裝：
   - Adafruit SSD1306
   - Adafruit GFX Library
   ```
3. **上傳程式**：

   - 將程式上傳至 Arduino
   - 打開序列埠監控視窗（115200 baud）觀察除錯訊息
4. **顯示內容**：

   - X/Y 座標（單位：cm）
   - IMU 角度（-180° ~ +180°）
   - 羅盤航向角（0° ~ 360°）
   - Roll/Pitch 角度
   - 方向指示箭頭（右上角）
5. **進階功能**：

   如需更強大的 JSON 解析，可使用 ArduinoJson 函式庫：

   ```cpp
   #include <ArduinoJson.h>

   void parseDataWithJson(String data) {
     StaticJsonDocument<200> doc;
     DeserializationError error = deserializeJson(doc, data);

     if (error) {
       Serial.print("JSON parse failed: ");
       Serial.println(error.c_str());
       return;
     }

     currentData.angle = doc["A"] | 0.0;
     currentData.x = doc["X"] | 0.0;
     currentData.y = doc["Y"] | 0.0;
     currentData.roll = doc["R"] | 0.0;
     currentData.pitch = doc["P"] | 0.0;
     currentData.compassAngle = doc["cA"] | 0.0;
     currentData.valid = true;
   }
   ```
6. **常見問題**：

   - 若無數據：檢查 PA3 是否正確上拉至高電平
   - 若 OLED 無顯示：確認 I2C 地址（可能是 0x3D）
   - 若數據不穩定：增加電源濾波電容、檢查接地

## 範例 5：STM32 (Maple) + UART + OLED 顯示

本範例展示如何使用 STM32 微控制器（使用 Maple/STM32duino 框架）透過硬體 UART 連接 HUB-PAA5163D 模組，並將資訊顯示在 OLED 上。相較於 Arduino，STM32 提供更多硬體 UART、更快的處理速度和原生 3.3V 介面。

**硬體連接**：

- HUB-PAA5163D PA2 (TX) → STM32 PA10 (USART1_RX)
- HUB-PAA5163D PA3 (RX) → STM32 PA9 (USART1_TX)
- HUB-PAA5163D GND → STM32 GND
- OLED SDA → STM32 PB7 (I2C1_SDA)
- OLED SCL → STM32 PB6 (I2C1_SCL)
- OLED VCC → STM32 3.3V
- OLED GND → STM32 GND

**開發環境**：

- Arduino IDE + STM32duino (STM32 Core)
- 或 PlatformIO + framework = arduino, platform = ststm32

**所需函式庫**：

- `HardwareSerial`：STM32 內建（多個硬體 UART）
- `Adafruit_SSD1306`：OLED 顯示
- `Adafruit_GFX`：圖形庫
- `ArduinoJson`：(可選) 更強大的 JSON 解析

**Arduino IDE 安裝 STM32 支援**：

```
檔案 -> 偏好設定 -> 額外的開發板管理員網址：
https://github.com/stm32duino/BoardManagerFiles/raw/main/package_stmicroelectronics_index.json

工具 -> 開發板 -> 開發板管理員 -> 搜尋 "STM32" -> 安裝
```

```cpp
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>  // 可選，用於更好的 JSON 解析

// OLED 顯示器設定
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// HUB-PAA5163D 硬體 UART (STM32 USART1: PA9/PA10)
// STM32 有多個硬體 UART，不需要 SoftwareSerial
HardwareSerial hubSerial(USART1); // 或使用 Serial1

// 數據結構
struct SensorData {
  float x;
  float y;
  float angle;
  float compassAngle;
  float rawCompassAngle;
  float roll;
  float pitch;
  unsigned long lastUpdate;
  bool valid;
};

SensorData currentData = {0, 0, 0, 0, 0, 0, 0, 0, false};
String inputBuffer = "";
unsigned long lastDisplayUpdate = 0;
const unsigned long DISPLAY_INTERVAL = 50;  // 20 FPS

void setup() {
  // 除錯串口（USB Serial）
  Serial.begin(115200);
  delay(1000);
  Serial.println("STM32 HUB-PAA5163D Reader");
  
  // HUB-PAA5163D 通訊 UART
  hubSerial.begin(115200);
  
  // I2C 初始化（STM32 預設 I2C1: PB6/PB7）
  Wire.begin();
  Wire.setClock(400000);  // 400kHz Fast Mode
  
  // 初始化 OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("OLED init failed!"));
    while(1) {
      delay(100);
    }
  }
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("HUB-PAA5163D"));
  display.println(F("STM32 Reader"));
  display.println(F("Initializing..."));
  display.display();
  
  delay(2000);
  
  // 初始化 HUB-PAA5163D
  initHub5163();
  
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println(F("Ready!"));
  display.display();
  delay(1000);
  
  Serial.println("Initialization complete");
}

void loop() {
  // 讀取 HUB-PAA5163D 數據
  while (hubSerial.available()) {
    char c = hubSerial.read();
  
    if (c == '\n' || c == '\r') {
      if (inputBuffer.length() > 0) {
        parseJsonData(inputBuffer);  // 使用 JSON 解析
        inputBuffer = "";
      }
    } else {
      inputBuffer += c;
    }
  }
  
  // 定時更新顯示（避免閃爍）
  if (millis() - lastDisplayUpdate >= DISPLAY_INTERVAL) {
    lastDisplayUpdate = millis();
    if (currentData.valid) {
      updateDisplay();
    }
  }
}

void initHub5163() {
  Serial.println("Initializing HUB-PAA5163D...");
  
  // 確保 PA3 (RX) 為高電平以啟用 USART1
  // 注意：需要在硬體上將 PA3 透過 10kΩ 電阻上拉至 3.3V
  
  delay(500);
  
  // 測試連線
  hubSerial.println("at");
  Serial.println("Sent: at");
  delay(200);
  
  // 重置並設定零點
  hubSerial.println("at0g+");
  Serial.println("Sent: at0g+");
  delay(1500);
  
  // 啟用 JSON 輸出（更易解析）
  hubSerial.println("atj+");
  Serial.println("Sent: atj+");
  delay(200);
  
  // 啟用完整模式（含 Roll/Pitch 和原始羅盤角度）
  hubSerial.println("atc+");
  Serial.println("Sent: atc+");
  delay(200);
  
  // 設定更新間隔為 50ms（20Hz，適合顯示）
  hubSerial.println("atrpt+50");
  Serial.println("Sent: atrpt+50");
  delay(200);
  
  Serial.println("HUB-PAA5163D initialized successfully");
}

void parseJsonData(String data) {
  // 使用 ArduinoJson 函式庫解析
  // JSON 格式: {"A":45.2,"X":12.5,"Y":-8.3,"R":2.1,"P":-1.5,"cA":92.3,"tA":89.8}
  
  if (!data.startsWith("{")) {
    return;  // 非 JSON 格式，忽略
  }
  
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, data);
  
  if (error) {
    Serial.print("JSON parse failed: ");
    Serial.println(error.c_str());
    return;
  }
  
  // 提取數據
  currentData.angle = doc["A"] | 0.0f;
  currentData.x = doc["X"] | 0.0f;
  currentData.y = doc["Y"] | 0.0f;
  currentData.roll = doc["R"] | 0.0f;
  currentData.pitch = doc["P"] | 0.0f;
  currentData.compassAngle = doc["cA"] | 0.0f;
  currentData.rawCompassAngle = doc["tA"] | 0.0f;
  currentData.lastUpdate = millis();
  currentData.valid = true;
  
  // 除錯輸出（可選）
  /*
  Serial.print("X:");
  Serial.print(currentData.x, 1);
  Serial.print(" Y:");
  Serial.print(currentData.y, 1);
  Serial.print(" A:");
  Serial.print(currentData.angle, 1);
  Serial.print(" cA:");
  Serial.println(currentData.compassAngle, 1);
  */
}

void updateDisplay() {
  display.clearDisplay();
  
  // ===== 標題區域 =====
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("=== HUB-PAA5163D ==="));
  
  // ===== 座標資訊 =====
  display.setCursor(0, 12);
  display.print(F("X: "));
  if (currentData.x >= 0) display.print(F(" "));
  display.print(currentData.x, 1);
  display.println(F(" cm"));
  
  display.setCursor(0, 22);
  display.print(F("Y: "));
  if (currentData.y >= 0) display.print(F(" "));
  display.print(currentData.y, 1);
  display.println(F(" cm"));
  
  // ===== 角度資訊 =====
  display.setCursor(0, 32);
  display.print(F("Ang: "));
  if (currentData.angle >= 0) display.print(F(" "));
  display.print(currentData.angle, 1);
  display.print(F(" "));
  display.write(247);  // 度數符號
  
  // ===== 羅盤資訊 =====
  display.setCursor(0, 42);
  display.print(F("Cmp: "));
  if (currentData.compassAngle < 100) display.print(F(" "));
  if (currentData.compassAngle < 10) display.print(F(" "));
  display.print(currentData.compassAngle, 0);
  display.print(F(" "));
  display.write(247);
  
  // ===== Roll & Pitch =====
  display.setCursor(0, 52);
  display.print(F("R:"));
  display.print(currentData.roll, 0);
  display.print(F(" P:"));
  display.print(currentData.pitch, 0);
  
  // ===== 資料更新指示 =====
  unsigned long timeSinceUpdate = millis() - currentData.lastUpdate;
  if (timeSinceUpdate < 500) {
    display.setCursor(60, 52);
    display.print(F("[LIVE]"));
  }
  
  // ===== 方向指示箭頭（右上角）=====
  drawCompass(110, 16, currentData.compassAngle);
  
  display.display();
}

void drawCompass(int cx, int cy, float angle) {
  // 繪製羅盤指示器
  // 將角度轉換為弧度（North = 0度，順時針）
  float rad = (angle - 90) * PI / 180.0;
  
  // 箭頭長度
  int len = 11;
  
  // 計算箭頭終點
  int x1 = cx + len * cos(rad);
  int y1 = cy + len * sin(rad);
  
  // 繪製外圈
  display.drawCircle(cx, cy, 14, SSD1306_WHITE);
  
  // 繪製內圈
  display.drawCircle(cx, cy, 2, SSD1306_WHITE);
  
  // 繪製箭頭主線（加粗效果）
  display.drawLine(cx, cy, x1, y1, SSD1306_WHITE);
  display.drawLine(cx+1, cy, x1+1, y1, SSD1306_WHITE);
  display.drawLine(cx, cy+1, x1, y1+1, SSD1306_WHITE);
  
  // 繪製箭頭尖端
  float arrowAngle = 30 * PI / 180.0;  // 箭頭角度
  int arrowLen = 5;
  
  float angle1 = rad + PI - arrowAngle;
  float angle2 = rad + PI + arrowAngle;
  
  int ax1 = x1 + arrowLen * cos(angle1);
  int ay1 = y1 + arrowLen * sin(angle1);
  int ax2 = x1 + arrowLen * cos(angle2);
  int ay2 = y1 + arrowLen * sin(angle2);
  
  display.drawLine(x1, y1, ax1, ay1, SSD1306_WHITE);
  display.drawLine(x1, y1, ax2, ay2, SSD1306_WHITE);
  display.fillTriangle(x1, y1, ax1, ay1, ax2, ay2, SSD1306_WHITE);
  
  // 標記北方 "N"（小字體）
  display.setTextSize(1);
  display.setCursor(cx - 2, cy - 23);
  display.print("N");
}

// ===== 可選：不使用 ArduinoJson 的簡易解析函式 =====
void parseTextData(String data) {
  // 簡易文字格式解析（不需要 ArduinoJson 函式庫）
  // 格式: A=45.2 X=12.5 Y=-8.3 R=2.1 P=-1.5 cA=92.3 tA=89.8
  
  if (data.indexOf("A=") >= 0) {
    currentData.angle = extractValue(data, "A=");
    currentData.x = extractValue(data, "X=");
    currentData.y = extractValue(data, "Y=");
    currentData.roll = extractValue(data, "R=");
    currentData.pitch = extractValue(data, "P=");
    currentData.compassAngle = extractValue(data, "cA=");
    currentData.rawCompassAngle = extractValue(data, "tA=");
    currentData.lastUpdate = millis();
    currentData.valid = true;
  }
}

float extractValue(String text, String key) {
  int pos = text.indexOf(key);
  if (pos == -1) return 0.0;
  
  pos += key.length();
  int endPos = text.indexOf(' ', pos);
  if (endPos == -1) {
    endPos = text.length();
  }
  
  String value = text.substring(pos, endPos);
  return value.toFloat();
}
```

**使用說明**：

1. **硬體準備**：

   - 使用 10kΩ 電阻將 HUB-PAA5163D 的 PA3 (RX) 上拉至 3.3V
   - STM32 使用 3.3V 邏輯電平，與 HUB-PAA5163D 直接相容
   - 確認所有 GND 接地共地
   - OLED 使用 I2C1 介面（PB6/PB7）
2. **開發板設定**（Arduino IDE）：

   ```
   工具 -> 開發板 -> STM32 boards groups -> Generic STM32F1 series
   工具 -> Board part number -> BluePill F103C8
   工具 -> Upload method -> STLink (或 Serial)
   工具 -> USB support -> CDC (通用序列埠超類別)
   ```
3. **函式庫安裝**：

   ```
   Arduino IDE -> 工具 -> 管理函式庫
   搜尋並安裝：
   - Adafruit SSD1306
   - Adafruit GFX Library
   - ArduinoJson (v6.x)
   ```
4. **PlatformIO 設定**（platformio.ini）：

   ```ini
   [env:bluepill_f103c8]
   platform = ststm32
   board = bluepill_f103c8
   framework = arduino
   upload_protocol = stlink
   lib_deps = 
       adafruit/Adafruit SSD1306@^2.5.7
       adafruit/Adafruit GFX Library@^1.11.3
       bblanchon/ArduinoJson@^6.21.3
   build_flags = -DHAL_UART_MODULE_ENABLED
   ```
5. **上傳程式**：

   - 使用 ST-Link V2 或 USB 轉 TTL 上傳
   - 打開序列埠監控視窗（115200 baud）觀察初始化過程
   - USB Serial 用於除錯，USART1 用於 HUB-PAA5163D 通訊
6. **顯示內容**：

   - X/Y 座標（單位：cm，帶符號對齊）
   - IMU 角度（-180° ~ +180°）
   - 羅盤航向角（0° ~ 360°，整數顯示）
   - Roll/Pitch 角度
   - 即時資料指示 [LIVE]
   - 增強型方向指示箭頭（右上角，實心三角形）
7. **STM32 優勢**：

   - **多硬體 UART**：不需要 SoftwareSerial，更穩定可靠
   - **更快處理速度**：72MHz vs 16MHz (Arduino Uno)
   - **原生 3.3V**：與 HUB-PAA5163D 直接相容，無需電平轉換
   - **更多記憶體**：64KB Flash, 20KB RAM
   - **DMA 支援**：可用於高速資料傳輸
   - **硬體 I2C**：更穩定的 OLED 通訊
8. **進階應用**：

   **使用 DMA 加速 UART 接收**：

   ```cpp
   // 在 setup() 中啟用 DMA
   hubSerial.enableHalfDuplexRx();
   ```

   **多工處理（使用 FreeRTOS）**：

   ```cpp
   // 建立獨立任務處理 UART 和顯示
   xTaskCreate(uartTask, "UART", 256, NULL, 1, NULL);
   xTaskCreate(displayTask, "Display", 256, NULL, 1, NULL);
   ```
9. **常見問題**：

   - 若無數據：
     * 檢查 PA3 是否正確上拉至 3.3V
     * 確認 USART1 引腳正確（PA9=TX, PA10=RX）
     * 檢查波特率設定
   - 若 OLED 無顯示：
     * 確認 I2C 地址（0x3C 或 0x3D）
     * 檢查 I2C 引腳（PB6=SCL, PB7=SDA）
     * 確認 I2C 上拉電阻（通常為 4.7kΩ）
   - 若編譯錯誤：
     * 確認已安裝 STM32duino core
     * 檢查 ArduinoJson 版本（需 v6.x）
   - 若資料解析失敗：
     * 檢查 JSON 格式是否完整
     * 增加 StaticJsonDocument 大小（如改為 512）
10. **效能優化建議**：

    - 使用硬體 I2C 的 DMA 模式加速 OLED 更新
    - 降低顯示更新頻率（20-30 FPS 已足夠）
    - 使用緩衝區避免頻繁重繪
    - 考慮使用 sprintf 預格式化字串

---
