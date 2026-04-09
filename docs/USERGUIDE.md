# Quickstart & User Guide

此文件包含以下部分：

1. 硬體連接方式
2. UART/USB 介面 - AT 指令參考
3. I2C Slave 介面使用
4. 數據輸出格式
---

## 硬體連接

#### 方式 A：USB 連接（UART 模式）

1. 使用 USB Type-C 線連接模組至電腦
2. 等待驅動程式安裝完成（Windows 會自動識別為 CDC 串口設備）
3. 確認 COM 埠號（透過裝置管理員）
4. （可選）連接 USART1 引腳至外部設備

#### 方式 B：I2C 連接（I2C Slave 模式）

1. 將模組的 PA0 (SDA) 和 PA1 (SCL) 連接至主控 MCU 的 I2C 引腳
2. 在 SDA 和 SCL 上接 4.7kΩ 上拉電阻至 3.3V
3. 連接 GND 共地
4. 供電（USB 或外部 3.3V-5V）
5. 當主控 MCU 開始 I2C 通訊時，模組自動切換至 I2C Slave 模式

### 2. 串口設定

- **波特率**：115200
- **數據位**：8
- **停止位**：1
- **校驗位**：無
- **流控制**：無

### 3. 測試連線

#### UART 模式測試

使用串口終端軟體（如 PuTTY、Tera Term、Arduino Serial Monitor）：

```
發送: at
回應: ok
```

#### I2C 模式測試

使用 I2C Master 設備（如 Arduino、STM32、BW16）讀取 ID 寄存器：

```cpp
Wire.begin();
Wire.setClock(200000);  // 200kHz

// 讀取寄存器 0x00 (ID)
Wire.beginTransmission(0x52);
Wire.write(0x00);
Wire.endTransmission();
Wire.requestFrom(0x52, 1);
uint8_t id = Wire.read();  // 應返回 0xA5
```

### 4. 基本操作流程

```bash
# 1. 測試連線
at
# 回應: ok

# 2. 重置並設定零點
at0g+
# 回應: .
#       ok

# 3. 啟動原始模式輸出
atr+
# 回應: ok
# 開始輸出: A=0.0 dX=0.0 dY=0.0 cA=12.3

# 4. 停止輸出
at-
# 回應: stop
```

---

## UART/USB 介面 - AT 指令參考

**適用於**：UART/USB 通訊模式

所有指令均**不區分大小寫**，以 `\r` 或 `\n` 結尾。

### 基本指令

#### `AT`

測試連線是否正常。

**格式**：`AT`

**回應**：`ok`

**範例**：

```
發送: at
回應: ok
```

---

#### `AT?`

顯示韌體版本與所有可用 AT 指令摘要。

**格式**：`AT?`

**回應**：

- 第一行顯示版本字串（例如：`HUB PAA5163D 1.2.3`）
- 後續顯示指令清單與簡要說明

**範例**：

```
發送: at?
回應: HUB PAA5163D 1.2.3
  Available AT commands:
  ...
```

---

### 運行模式指令

#### `ATR+`

啟動**原始模式**輸出。

**格式**：`ATR+`

**回應**：`ok`

**輸出格式**：

```
A=<角度> dX=<原始X位移> dY=<原始Y位移> cA=<羅盤角度>
```

**範例**：

```
發送: atr+
回應: ok
輸出: A=45.2 dX=125.0 dY=-83.5 cA=92.3
```

---

#### `ATX+`

啟動**座標模式**輸出。

**格式**：`ATX+`

**回應**：`ok`

**輸出格式**：

```
A=<角度> X=<X座標> Y=<Y座標> cA=<羅盤角度>
```

**範例**：

```
發送: atx+
回應: ok
輸出: A=45.2 X=12.5 Y=-8.3 cA=92.3
```

---

#### `ATC+`

啟動**完整模式**輸出（含 Roll/Pitch）。

**格式**：`ATC+`

**回應**：`ok`

**輸出格式**：

```
A=<角度> X=<X座標> Y=<Y座標> R=<Roll> P=<Pitch> cA=<羅盤角度> tA=<原始羅盤>
```

**範例**：

```
發送: atc+
回應: ok
輸出: A=45.2 X=12.5 Y=-8.3 R=2.1 P=-1.5 cA=92.3 tA=89.8
```

---

#### `AT-`

停止數據輸出。

**格式**：`AT-`

**回應**：`stop`

**範例**：

```
發送: at-
回應: stop
```

---

### 校正與設定指令

#### `AT0G+`

重置所有感測器數據並設定零點。

**格式**：`AT0G+`

**功能**：

- 重置 IMU 陀螺儀和加速度計
- 重置 PAA5163 光流感測器
- 重置 X/Y 座標為 0
- 使用向量平均法設定羅盤零點（讀取10次，每20ms一次）

**回應**：

```
.
ok
```

**範例**：

```
發送: at0g+
回應: .
      ok
```

**注意**：此過程約需 200ms，期間請保持模組靜止。

---

#### `AT0C+8`

校正電子羅盤（標準模式）。

**格式**：`AT0C+8`

**功能**：啟動 20 秒羅盤校正程序，使用橢圓擬合演算法。

**操作步驟**：

1. 發送 `AT0C+8` 指令
2. 在 20 秒內緩慢旋轉模組，至少完整旋轉一圈
3. 等待校正完成

**回應**：

```
Start rolling 20 sec, get new cmpsCalData
ok minX=<值> maxX=<值> minY=<值> maxY=<值> minZ=<值> maxZ=<值>
```

**範例**：

```
發送: at0c+8
回應: Start rolling 20 sec, get new cmpsCalData
      ok minX=-1234 maxX=1156 minY=-1289 maxY=1098 minZ=-1456 maxZ=1234
```

**注意**：

- 校正數據會自動儲存至 EEPROM
- 建議在干擾較少的環境進行校正
- 旋轉時應包含三個軸向

---

#### `AT0M+8`

校正電子羅盤（最大最小值模式）。

**格式**：`AT0M+8`

**功能**：啟動 20 秒羅盤校正程序，使用最大最小值演算法。

**操作步驟**：同 `AT0C+8`

**回應**：

```
Start rolling 20 sec, get updated cmpsCalData
ok minX=<值> maxX=<值> minY=<值> maxY=<值> minZ=<值> maxZ=<值>
```

---

#### `ATI+C`

查詢感測器高度、報告間隔及當前羅盤校正數據。

**格式**：`ATI+C`

**回應**：

```
Sensor height: <高度> mm
Report interval: <間隔> ms
Compass calibration data:
<minX>,<maxX>,<minY>,<maxY>,<minZ>,<maxZ>
```

**範例**：

```
發送: ati+c
回應: Sensor height: 15 mm
      Report interval: 200 ms
      Compass calibration data:
      -1234,1156,-1289,1098,-1456,1234
```

---

#### `ATI&<參數>`

手動設定羅盤校正數據。

**格式**：`ATI&<minX>,<maxX>,<minY>,<maxY>,<minZ>,<maxZ>`

**參數**：6 個整數值，以逗號分隔

**回應**：

- 成功：`Compass calibration data updated.`
- 失敗：`err`

**範例**：

```
發送: ati&-1234,1156,-1289,1098,-1456,1234
回應: Compass calibration data updated.
```

---

### 參數設定指令

#### `ATH+[高度]`

設定感測器高度。

**格式**：

- `ATH+<數值>`：設定為指定高度（mm）

**參數範圍**：5 ~ 40 (mm)

**回應**：`Set <高度> mm sensor of height.`

**範例**：

```
發送: ath+15
回應: Set 15 mm sensor of height.
```

**說明**：

- 高度設定影響位移精度校正
- 預設高度：5mm
- 建議根據實際安裝高度進行設定

---

#### `ATH-`

降低感測器高度設定。

**格式**：`ATH-`

**功能**：將高度減少 1mm

**回應**：`Set <高度> mm sensor of height.`

**範例**：

```
發送: ath-
回應: Set 14 mm sensor of height.
```

---

#### `ATRPT+<時間>`

設定數據報告間隔時間。

**格式**：`ATRPT+<毫秒>`

**參數範圍**：50 ~ 1000 (ms)

**預設值**：200 ms

**回應**：

- 成功：`Set report time <時間> ms.`
- 失敗：`err`

**範例**：

```
發送: atrpt+100
回應: Set report time 100 ms.
```

---

### 硬體控制指令

#### `ATM+`

啟用 movPin（PB1）移動指示輸出。

**格式**：`ATM+`

**回應**：

- 啟用成功：`Movement index output enabled`
- 已啟用：`Movement index output already enabled`

---

#### `ATM-`

停用 movPin（PB1）移動指示輸出。

**格式**：`ATM-`

**回應**：

- 停用成功：`Movement index output disabled`
- 已停用：`Movement index output already disabled`

---

#### `ATNRST+OMT`

對 PAA5163 執行硬體 NRST 重置，並重新初始化感測器。

**格式**：`ATNRST+OMT`

**回應**：

```
.
ok
```

---

#### `AT@UPGRADE+`

設定 DFU 旗標並觸發系統重啟，轉入 Bootloader DFU 升級模式。

**格式**：`AT@UPGRADE+`

**回應**：`Entering DFU mode...`

**注意**：

- 指令送出後裝置會立即重啟
- 主應用程式不會返回當前運行狀態

---

### 輸出控制指令

#### `ATJ+`

啟用 JSON 格式輸出。

**格式**：`ATJ+`

**回應**：`JSON output enabled`

**輸出範例**（原始模式）：

```json
{"A":45.2,"dX":125.0,"dY":-83.5,"cA":92.3}
```

**輸出範例**（座標模式）：

```json
{"A":45.2,"X":12.5,"Y":-8.3,"cA":92.3}
```

**輸出範例**（完整模式）：

```json
{"A":45.2,"X":12.5,"Y":-8.3,"R":2.1,"P":-1.5,"cA":92.3,"tA":89.8}
```

---

#### `ATJ-`

停用 JSON 格式輸出（恢復文字格式）。

**格式**：`ATJ-`

**回應**：`JSON output disabled`

---

### 指令摘要表

| 指令            | 功能                     | 參數                          |
| --------------- | ------------------------ | ----------------------------- |
| `AT`          | 測試連線                 | 無                            |
| `AT?`         | 顯示版本與指令清單       | 無                            |
| `ATR+`        | 啟動原始模式             | 無                            |
| `ATX+`        | 啟動座標模式             | 無                            |
| `ATC+`        | 啟動完整模式             | 無                            |
| `AT-`         | 停止輸出                 | 無                            |
| `AT0G+`       | 重置並設定零點           | 無                            |
| `AT0C+8`      | 校正羅盤（標準）         | 無                            |
| `AT0M+8`      | 校正羅盤（最大最小值）   | 無                            |
| `ATI+C`       | 查詢高度、間隔及校正數據 | 無                            |
| `ATI&`        | 設定校正數據             | minX,maxX,minY,maxY,minZ,maxZ |
| `ATH+`        | 設定/增加高度            | [5-40]                        |
| `ATH-`        | 降低高度                 | 無                            |
| `ATRPT+`      | 設定報告間隔             | 50-1000                       |
| `ATM+`        | 啟用 movPin 輸出         | 無                            |
| `ATM-`        | 停用 movPin 輸出         | 無                            |
| `ATJ+`        | 啟用JSON輸出             | 無                            |
| `ATJ-`        | 停用JSON輸出             | 無                            |
| `ATNRST+OMT`  | 硬體重置 PAA5163         | 無                            |
| `AT@UPGRADE+` | 進入 DFU 韌體升級模式    | 無                            |

---

## I2C Slave 介面使用

**適用於**：I2C Slave 通訊模式

### 概述

當模組偵測到 I2C 通訊時，會自動切換至 I2C Slave 模式：

- UART 串口自動關閉（節省資源）
- 所有感測器數據透過 I2C 寄存器訪問
- 控制命令透過寫入控制位元寄存器實現
- 運行模式自動切換為座標模式（含 X/Y/angleZ/compass）

### I2C 寄存器映射

**I2C Slave 地址**：0x52 (7-bit)
**寄存器總大小**：53 bytes
**位址寬度**：8-bit

| 偏移 | 大小 | 類型      | 名稱              | 說明                    | 範圍      |
| ---- | ---- | --------- | ----------------- | ----------------------- | --------- |
| 0x00 | 1    | uint8_t   | id                | 固定 ID 值              | 0xA5      |
| 0x01 | 1    | uint8_t   | controlBits       | 控制位元（見下表）      | -         |
| 0x02 | 2    | uint16_t  | reportIntervalMs  | 報告間隔時間 (ms)       | 50-1000   |
| 0x04 | 1    | uint8_t   | highI             | 感測器高度 (實際高度-5) | 0-35      |
| 0x05 | 4    | float32_t | X                 | X 座標 (cm)             | -         |
| 0x09 | 4    | float32_t | Y                 | Y 座標 (cm)             | -         |
| 0x0D | 4    | float32_t | dX                | X 偏移 (cpi)            | -         |
| 0x11 | 4    | float32_t | dY                | Y 偏移 (cpi)            | -         |
| 0x15 | 4    | float32_t | angleZ            | Z 軸角度 (度)           | -180~+180 |
| 0x19 | 4    | float32_t | compassHeading    | 羅盤航向角 (度)         | 0~360     |
| 0x1D | 4    | float32_t | rawCompassHeading | 原始羅盤角度 (度)       | -         |
| 0x21 | 4    | float32_t | roll              | Roll 角度 (度)          | -         |
| 0x25 | 4    | float32_t | pitch             | Pitch 角度 (度)         | -         |
| 0x29 | 2    | int16_t   | compassMinX       | 羅盤校正最小值 X        | -         |
| 0x2B | 2    | int16_t   | compassMaxX       | 羅盤校正最大值 X        | -         |
| 0x2D | 2    | int16_t   | compassMinY       | 羅盤校正最小值 Y        | -         |
| 0x2F | 2    | int16_t   | compassMaxY       | 羅盤校正最大值 Y        | -         |
| 0x31 | 2    | int16_t   | compassMinZ       | 羅盤校正最小值 Z        | -         |
| 0x33 | 2    | int16_t   | compassMaxZ       | 羅盤校正最大值 Z        | -         |

**注意事項**：

- **highI**：寄存器值 = 實際高度 - 5（例如：highI=10 → 實際高度=15mm）
- **float32_t**：IEEE 754 單精度浮點格式，Little-Endian
- **int16_t/uint16_t**：有符號/無符號16位整數，Little-Endian
- 羅盤校正值為有符號整數（int16_t），範圍約 -32768 ~ +32767

### controlBits 位元定義

| Bit | 名稱               | R/W | 說明                                                        |
| --- | ------------------ | --- | ----------------------------------------------------------- |
| 0   | MOVE_PIN_EN        | R/W | 移動指示輸出啟用 `<br>`寫1=啟用 PB1輸出，寫0=停用         |
| 1   | SET_ZERO_PROGRESS  | R/W | 設定零點 `<br>`寫1=開始設定，讀0=完成                     |
| 2   | CMPS_CAL_START     | R/W | 開始羅盤校正（新方法）`<br>`寫1=開始20秒校正，讀0=完成    |
| 3   | CMPS_CAL_OLD_START | R/W | 羅盤校正（舊值基礎）`<br>`寫1=開始20秒校正，讀0=完成      |
| 4   | SET_CMPS_CAL_DATA  | R/W | 設定羅盤校正數據 `<br>`寫1=套用校正值，讀0=完成           |
| 5   | PAA_NRST_PIN       | R/W | PAA5163 NRST 引腳狀態 `<br>`1=high, 0=low（用於硬體重置） |
| 6   | UPDATE_SENSORS     | R/W | 更新感測器數據 `<br>`寫1=觸發更新，讀0=完成               |
| 7   | -                  | -   | 保留                                                        |

### I2C 操作流程

以下範例基於 Arduino Wire 庫，可適用於 Arduino、ESP32、STM32 等平台。

#### 1. 讀取感測器數據（標準流程）

```cpp
#define I2C_ADDR 0x52
#define REG_CONTROL_BITS 0x01
#define CTRL_BIT_UPDATE_SENSORS (1 << 6)

bool updateSensors() {
    // 步驟1: 觸發感測器數據更新
    uint8_t ctrl = i2c_read_byte(I2C_ADDR, REG_CONTROL_BITS);
    ctrl |= CTRL_BIT_UPDATE_SENSORS;
    i2c_write_byte(I2C_ADDR, REG_CONTROL_BITS, ctrl);
  
    // 步驟2: 等待更新完成（輪詢，最多800ms超時）
    unsigned long startTime = millis();
    while (millis() - startTime < 800) {
        delay(10);
        ctrl = i2c_read_byte(I2C_ADDR, REG_CONTROL_BITS);
        if (!(ctrl & CTRL_BIT_UPDATE_SENSORS)) {
            return true;  // 更新完成
        }
    }
    return false;  // 超時
}

void readSensorData() {
    if (updateSensors()) {
        // 步驟3: 讀取感測器數據
        float x = i2c_read_float(I2C_ADDR, 0x05);
        float y = i2c_read_float(I2C_ADDR, 0x09);
        float angleZ = i2c_read_float(I2C_ADDR, 0x15);
        float compass = i2c_read_float(I2C_ADDR, 0x19);
  
        Serial.print("X: "); Serial.print(x, 2);
        Serial.print(" Y: "); Serial.print(y, 2);
        Serial.print(" Angle: "); Serial.print(angleZ, 1);
        Serial.print(" Compass: "); Serial.println(compass, 1);
    } else {
        Serial.println("Sensor update timeout!");
    }
}
```

#### 2. 設定零點

```cpp
#define CTRL_BIT_SET_ZERO_PROGRESS (1 << 1)

void setZeroPoint() {
    Serial.println("Setting zero point...");
  
    // 設定 bit 1
    uint8_t ctrl = i2c_read_byte(I2C_ADDR, REG_CONTROL_BITS);
    ctrl |= CTRL_BIT_SET_ZERO_PROGRESS;
    i2c_write_byte(I2C_ADDR, REG_CONTROL_BITS, ctrl);
  
    // 等待完成（最多 3 秒）
    unsigned long startTime = millis();
    while (millis() - startTime < 3000) {
        delay(100);
        ctrl = i2c_read_byte(I2C_ADDR, REG_CONTROL_BITS);
        if (!(ctrl & CTRL_BIT_SET_ZERO_PROGRESS)) {
            Serial.println("Zero point set completed!");
            return;
        }
    }
    Serial.println("Zero point setting timeout!");
}
```

#### 3. 校正電子羅盤

```cpp
#define CTRL_BIT_CMPS_CAL_START (1 << 2)
#define REG_COMPASS_MIN_X 0x29

void calibrateCompass() {
    Serial.println("Starting compass calibration...");
    Serial.println("Please rotate device for 20 seconds!");
  
    // 設定 bit 2 開始校正
    uint8_t ctrl = i2c_read_byte(I2C_ADDR, REG_CONTROL_BITS);
    ctrl |= CTRL_BIT_CMPS_CAL_START;
    i2c_write_byte(I2C_ADDR, REG_CONTROL_BITS, ctrl);
  
    // 等待校正完成（最多 25 秒）
    unsigned long startTime = millis();
    while (millis() - startTime < 25000) {
        delay(100);
        ctrl = i2c_read_byte(I2C_ADDR, REG_CONTROL_BITS);
        if (!(ctrl & CTRL_BIT_CMPS_CAL_START)) {
            Serial.println("Compass calibration completed!");
  
            // 讀取校正結果
            int16_t minX = (int16_t)i2c_read_word(I2C_ADDR, REG_COMPASS_MIN_X);
            int16_t maxX = (int16_t)i2c_read_word(I2C_ADDR, REG_COMPASS_MIN_X + 2);
            int16_t minY = (int16_t)i2c_read_word(I2C_ADDR, REG_COMPASS_MIN_X + 4);
            int16_t maxY = (int16_t)i2c_read_word(I2C_ADDR, REG_COMPASS_MIN_X + 6);
            int16_t minZ = (int16_t)i2c_read_word(I2C_ADDR, REG_COMPASS_MIN_X + 8);
            int16_t maxZ = (int16_t)i2c_read_word(I2C_ADDR, REG_COMPASS_MIN_X + 10);
  
            Serial.println("Calibration Data:");
            Serial.print("  X: "); Serial.print(minX); 
            Serial.print(" ~ "); Serial.println(maxX);
            Serial.print("  Y: "); Serial.print(minY); 
            Serial.print(" ~ "); Serial.println(maxY);
            Serial.print("  Z: "); Serial.print(minZ); 
            Serial.print(" ~ "); Serial.println(maxZ);
            return;
        }
        if ((millis() - startTime) % 1000 == 0) {
            Serial.print(".");  // 每秒顯示進度
        }
    }
    Serial.println("\nCompass calibration timeout!");
}
```

#### 4. 手動設定羅盤校正數據

```cpp
#define CTRL_BIT_SET_CMPS_CAL_DATA (1 << 4)

void setCompassCalData(int16_t minX, int16_t maxX, 
                       int16_t minY, int16_t maxY,
                       int16_t minZ, int16_t maxZ) {
    // 寫入校正數據到寄存器
    i2c_write_word(I2C_ADDR, 0x29, minX);
    i2c_write_word(I2C_ADDR, 0x2B, maxX);
    i2c_write_word(I2C_ADDR, 0x2D, minY);
    i2c_write_word(I2C_ADDR, 0x2F, maxY);
    i2c_write_word(I2C_ADDR, 0x31, minZ);
    i2c_write_word(I2C_ADDR, 0x33, maxZ);
  
    Serial.println("Setting compass calibration data...");
  
    // 設定 bit 4 以套用數據
    uint8_t ctrl = i2c_read_byte(I2C_ADDR, REG_CONTROL_BITS);
    ctrl |= CTRL_BIT_SET_CMPS_CAL_DATA;
    i2c_write_byte(I2C_ADDR, REG_CONTROL_BITS, ctrl);
  
    // 等待完成（最多 3 秒）
    unsigned long startTime = millis();
    while (millis() - startTime < 3000) {
        delay(10);
        ctrl = i2c_read_byte(I2C_ADDR, REG_CONTROL_BITS);
        if (!(ctrl & CTRL_BIT_SET_CMPS_CAL_DATA)) {
            Serial.println("Calibration data applied!");
            return;
        }
    }
    Serial.println("Calibration data write timeout!");
}

// 使用範例
void setup() {
    // 設定校正數據：minX, maxX, minY, maxY, minZ, maxZ
    setCompassCalData(-1234, 1156, -1289, 1098, -1456, 1234);
}
```

#### 5. 控制移動指示輸出 (MOV Pin)

```cpp
#define CTRL_BIT_MOVE_PIN_EN (1 << 0)

void enableMovePin(bool enable) {
    uint8_t ctrl = i2c_read_byte(I2C_ADDR, REG_CONTROL_BITS);
  
    if (enable) {
        ctrl |= CTRL_BIT_MOVE_PIN_EN;
        Serial.println("Motion indicator enabled");
    } else {
        ctrl &= ~CTRL_BIT_MOVE_PIN_EN;
        Serial.println("Motion indicator disabled");
    }
  
    i2c_write_byte(I2C_ADDR, REG_CONTROL_BITS, ctrl);
}

// PB1 引腳會在偵測到移動時輸出高電平
```

#### 6. 設定感測器高度

```cpp
#define REG_HIGH_I 0x04

void setSensorHeight(uint8_t heightMm) {
    if (heightMm < 5 || heightMm > 40) {
        Serial.println("Height range: 5-40 mm");
        return;
    }
  
    // 寄存器值 = 實際高度 - 5
    uint8_t regValue = heightMm - 5;
    i2c_write_byte(I2C_ADDR, REG_HIGH_I, regValue);
  
    Serial.print("Sensor height set to: ");
    Serial.print(heightMm);
    Serial.println(" mm");
}
```

#### 7. 設定報告間隔

```cpp
#define REG_REPORT_INTERVAL_MS 0x02

void setReportInterval(uint16_t intervalMs) {
    if (intervalMs < 50 || intervalMs > 1000) {
        Serial.println("Interval range: 50-1000 ms");
        return;
    }
  
    i2c_write_word(I2C_ADDR, REG_REPORT_INTERVAL_MS, intervalMs);
  
    Serial.print("Report interval set to: ");
    Serial.print(intervalMs);
    Serial.println(" ms");
}
```

### I2C 輔助函式實作

以下是完整的 I2C 讀寫函式實作（Arduino Wire 庫）：

```cpp
// 讀取單個位元組
uint8_t i2c_read_byte(uint8_t addr, uint8_t reg) {
    Wire.beginTransmission(addr);
    Wire.write(reg);
    if (Wire.endTransmission() != 0) {
        return 0;
    }
  
    Wire.requestFrom(addr, 1);
    unsigned long startTime = micros();
    while (Wire.available() == 0) {
        if (micros() - startTime > 100) return 0;  // 100us 超時
        delayMicroseconds(10);
    }
  
    return Wire.available() ? Wire.read() : 0;
}

// 寫入單個位元組
void i2c_write_byte(uint8_t addr, uint8_t reg, uint8_t value) {
    Wire.beginTransmission(addr);
    Wire.write(reg);
    Wire.write(value);
    Wire.endTransmission();
    delay(10);  // 等待寫入完成
}

// 讀取 16-bit 字（Little-Endian）
uint16_t i2c_read_word(uint8_t addr, uint8_t reg) {
    Wire.beginTransmission(addr);
    Wire.write(reg);
    if (Wire.endTransmission() != 0) {
        return 0;
    }
  
    Wire.requestFrom(addr, 2);
    unsigned long startTime = micros();
    while (Wire.available() < 2) {
        if (micros() - startTime > 200) return 0;  // 200us 超時
        delayMicroseconds(10);
    }
  
    if (Wire.available() >= 2) {
        uint8_t low = Wire.read();
        uint8_t high = Wire.read();
        return (high << 8) | low;
    }
    return 0;
}

// 寫入 16-bit 字（Little-Endian）
void i2c_write_word(uint8_t addr, uint8_t reg, uint16_t value) {
    Wire.beginTransmission(addr);
    Wire.write(reg);
    Wire.write(value & 0xFF);         // 低位元組
    Wire.write((value >> 8) & 0xFF);  // 高位元組
    Wire.endTransmission();
    delay(10);
}

// 讀取浮點數（IEEE 754，Little-Endian）
float i2c_read_float(uint8_t addr, uint8_t reg) {
    Wire.beginTransmission(addr);
    Wire.write(reg);
    if (Wire.endTransmission() != 0) {
        return 0.0f;
    }
  
    Wire.requestFrom(addr, 4);
    unsigned long startTime = micros();
    while (Wire.available() < 4) {
        if (micros() - startTime > 300) return 0.0f;  // 300us 超時
        delayMicroseconds(10);
    }
  
    if (Wire.available() >= 4) {
        union {
            uint8_t bytes[4];
            float value;
        } data;
        data.bytes[0] = Wire.read();
        data.bytes[1] = Wire.read();
        data.bytes[2] = Wire.read();
        data.bytes[3] = Wire.read();
        return data.value;
    }
    return 0.0f;
}

// 寫入浮點數（IEEE 754，Little-Endian）
void i2c_write_float(uint8_t addr, uint8_t reg, float value) {
    union {
        float f;
        uint8_t bytes[4];
    } data;
    data.f = value;
  
    Wire.beginTransmission(addr);
    Wire.write(reg);
    Wire.write(data.bytes, 4);
    Wire.endTransmission();
    delay(10);
}
```

### I2C 完整應用範例

參考專案中的 `test_hub5168_i2c/test_hub5168_i2c.ino` 與 `test_hub5168_i2c/README.md`，這是目前實際使用的 Hub5168+ (BW16) I2C Master 測試程序。

**建議接線**：

- Hub5168+ `PA26 (SDA)` → HUB-PAA5163D `PA0 (I2C2 SDA)`
- Hub5168+ `PA25 (SCL)` → HUB-PAA5163D `PA1 (I2C2 SCL)`
- `GND` ↔ `GND`
- `SDA`、`SCL` 都需以 `4.7kΩ` 上拉到 `3.3V`

**程式內建命令**（依 `test_hub5168_i2c.ino` 實作）：

| 命令                                 | 功能                                         |
| ------------------------------------ | -------------------------------------------- |
| `ssr`                              | 觸發 `UPDATE_SENSORS` 後讀取全部感測器資料 |
| `inf`                              | 讀取裝置資訊                                 |
| `id`                               | 讀取裝置 ID (`0xA5`)                       |
| `ctrl`                             | 讀取 controlBits                             |
| `cd`                               | 讀取羅盤校正資料                             |
| `ca minX maxX minY maxY minZ maxZ` | 手動寫入羅盤校正資料                         |
| `z`                                | 設定零點                                     |
| `c`                                | 啟動羅盤校正（新方法）                       |
| `o`                                | 啟動羅盤校正（沿用舊值）                     |
| `m+` / `m-`                      | 啟用 / 停用 movPin 輸出                      |
| `t<ms>`                            | 設定報告間隔，例如 `t200`                  |
| `h<mm>`                            | 設定高度，例如 `h10`                       |
| `a+` / `a-`                      | 啟用 / 停用自動讀取（預設 500ms）            |
| `j+` / `j-`                      | 啟用 / 停用 JSON 格式輸出                    |
| `?`                                | 顯示命令選單                                 |

**測試程式輸出格式**：

- 文字模式：`Position / Offset / Angle / Compass / Attitude`
- JSON 模式：`{"X":...,"Y":...,"dX":...,"dY":...,"angleZ":...,"compass":...,"rawCompass":...,"roll":...,"pitch":...}`

若要直接在現場驗證 I2C 通訊，建議優先使用這個測試程式，而不是自行從零撰寫主控端程式。

---

## Data Format 數據輸出格式

### 文字格式

#### 模式 1：原始模式 (ATR+)

```
A=<角度> dX=<原始X位移> dY=<原始Y位移> cA=<羅盤角度>
```

**參數說明**：

- `A`: IMU Z軸角度，範圍 -180° ~ +180°
- `dX`: 原始X軸累積位移（CPI單位）
- `dY`: 原始Y軸累積位移（CPI單位）
- `cA`: 電子羅盤航向角，範圍 0° ~ 360°

#### 模式 2：座標模式 (ATX+)

```
A=<角度> X=<X座標> Y=<Y座標> cA=<羅盤角度>
```

**參數說明**：

- `A`: IMU Z軸角度，範圍 -180° ~ +180°
- `X`: 經角度補償的X座標 (cm)
- `Y`: 經角度補償的Y座標 (cm)
- `cA`: 電子羅盤航向角，範圍 0° ~ 360°

#### 模式 3：完整模式 (ATC+)

```
A=<角度> X=<X座標> Y=<Y座標> R=<Roll> P=<Pitch> cA=<羅盤角度> tA=<原始羅盤>
```

**參數說明**：

- `A`: IMU Z軸角度，範圍 -180° ~ +180°
- `X`: 經角度補償的X座標 (cm)
- `Y`: 經角度補償的Y座標 (cm)
- `R`: Roll 角度（翻滾角）
- `P`: Pitch 角度（俯仰角）
- `cA`: 經傾斜補償的羅盤航向角，範圍 0° ~ 360°
- `tA`: 原始羅盤讀數（未補償）

### JSON 格式 (啟用 ATJ+ 後)

#### 原始模式

```json
{"A":45.2,"dX":125.0,"dY":-83.5,"cA":92.3}
```

#### 座標模式

```json
{"A":45.2,"X":12.5,"Y":-8.3,"cA":92.3}
```

#### 完整模式

```json
{"A":45.2,"X":12.5,"Y":-8.3,"R":2.1,"P":-1.5,"cA":92.3,"tA":89.8}
```

### 更新頻率

- **位移數據**：每 8ms 更新一次（125 Hz）
- **IMU數據**：每 2ms 更新一次（500 Hz）
- **羅盤數據**：每 20ms 更新一次（50 Hz）
- **數據輸出**：根據 `ATRPT+` 設定（預設 200ms）

---