# HUB-PAA5163D / HUB5168+ I2C 測試程序

## 概述

此程序使用 Hub5168+ Arduino 板透過 I2C 介面測試 HUB-PAA5163D 運動追蹤模組的功能。

## 硬體需求

- Hub5168 (BW16) 開發板
- HUB-PAA5163D 運動追蹤模組
- 連接線
- 2 個 4.7K 上拉電阻

## 硬體連接

```
Hub5168+                HUB-PAA5163D
────────────────────    ──────────────────
PA26 (SDA) ────────────> PA0 (I2C2 SDA)
PA25 (SCL) ────────────> PA1 (I2C2 SCL)
GND        ────────────> GND
3.3V ──┬─── 4.7K ──────> SDA
       └─── 4.7K ──────> SCL
```

### 重要提醒

1. **上拉電阻**: SDA 和 SCL 都需要 4.7K 上拉電阻連接到 3.3V
2. **共地**: 確保兩個設備共地
3. **電壓**: 確認使用 3.3V，避免損壞設備
4. **I2C Slave 啟用**: HUB-PAA5163D 的 PA0/PA1 在開機時需要都是 HIGH 才會啟用 I2C Slave 模式

## I2C Slave 寄存器映射表

| 偏移 | 大小 | 名稱              | 型態     | 說明                   |
| ---- | ---- | ----------------- | -------- | ---------------------- |
| 0    | 1    | id                | uint8_t  | 固定值 0xA5            |
| 1    | 1    | controlBits       | uint8_t  | 控制位元               |
| 2    | 2    | reportIntervalMs  | uint16_t | 報告間隔 (ms)          |
| 4    | 1    | highI             | uint8_t  | 感測器高度 (mm)        |
| 5    | 4    | X                 | float    | X 座標 (cm)            |
| 9    | 4    | Y                 | float    | Y 座標 (cm)            |
| 13   | 4    | dX                | float    | X 偏移 (cm)            |
| 17   | 4    | dY                | float    | Y 偏移 (cm)            |
| 21   | 4    | angleZ            | float    | Z 角度 (度, -180~+180) |
| 25   | 4    | compassHeading    | float    | 羅盤航向角 (度, 0~360) |
| 29   | 4    | rawCompassHeading | float    | 原始羅盤角度 (度)      |
| 33   | 4    | roll              | float    | Roll 角度 (度)         |
| 37   | 4    | pitch             | float    | Pitch 角度 (度)        |
| 41   | 4    | compassMinX       | float    | 羅盤校正最小值 X       |
| 45   | 4    | compassMaxX       | float    | 羅盤校正最大值 X       |
| 49   | 4    | compassMinY       | float    | 羅盤校正最小值 Y       |
| 53   | 4    | compassMaxY       | float    | 羅盤校正最大值 Y       |
| 57   | 4    | compassMinZ       | float    | 羅盤校正最小值 Z       |
| 61   | 4    | compassMaxZ       | float    | 羅盤校正最大值 Z       |

**總大小**: 65 bytes

## controlBits 位元定義

| 位元 | 名稱               | 說明                                |
| ---- | ------------------ | ----------------------------------- |
| bit0 | MOVE_PIN_EN        | 運動指示輸出啟用 (1=啟用, 0=停用)   |
| bit1 | SET_ZERO_PROGRESS  | 設定零點 (1=進行中, 0=完成)         |
| bit2 | CMPS_CAL_START     | 開始羅盤校正 (1=進行中, 0=完成)     |
| bit3 | CMPS_CAL_OLD_START | 使用舊值校正羅盤 (1=進行中, 0=完成) |
| bit4 | SET_CMPS_CAL_DATA  | 設定羅盤校正資料 (1=進行中, 0=完成) |
| bit5 | PAA_NRST_PIN       | PAA5163 重置腳位狀態 (1=高, 0=低)   |

## 使用說明

### 1. 編譯與上傳

1. 開啟 Arduino IDE
2. 選擇板子: "Realtek RTL8720DN (BW16)"
3. 開啟 `test_hub5168_i2c.ino`
4. 編譯並上傳到 HUB5168+

### 2. 序列監視器設定

- 鮑率: 115200
- 換行設定: "Newline" 或 "Both NL & CR"

### 3. 命令列表

#### 資料讀取命令

- `r` - 讀取所有感測器數據
- `i` - 讀取設備資訊
- `?` - 顯示命令選單

#### 控制命令

- `z` - 設定零點 (重置座標與角度)
- `c` - 開始羅盤校正 (需旋轉設備 20 秒)
- `o` - 使用舊值校正羅盤 (需旋轉設備 20 秒)
- `m+` - 啟用運動指示輸出
- `m-` - 停用運動指示輸出

#### 設定命令

- `t<ms>` - 設定報告間隔，例: `t200` 設定為 200ms (範圍: 50-1000)
- `h<mm>` - 設定感測器高度，例: `h10` 設定為 10mm (範圍: 5-40)
- `a+` - 啟用自動讀取 (每 500ms 一次)
- `a-` - 停用自動讀取
- `j+` - 啟用 JSON 格式輸出
- `j-` - 停用 JSON 格式輸出

### 4. 典型使用流程

#### 初次使用校正流程

```
1. 上傳程序並開啟序列監視器
2. 檢查是否偵測到 I2C 設備 (0x52)
3. 輸入 'i' 查看設備資訊
4. 輸入 'z' 設定當前位置為零點
5. 輸入 'c' 開始羅盤校正
   - 在 20 秒內緩慢旋轉設備 360 度
   - 等待校正完成訊息
6. 輸入 'a+' 啟用自動讀取
7. 移動設備觀察座標變化
```

#### 測試運動追蹤

```
1. 輸入 'z' 重置座標
2. 輸入 'a+' 啟用自動讀取
3. 移動設備觀察:
   - X, Y 座標變化
   - angleZ 角度變化
   - compass 羅盤方向
   - roll, pitch 姿態角
```

#### 調整參數

```
# 設定感測器高度為 10mm
h10

# 設定報告間隔為 100ms
t100

# 啟用運動指示輸出
m+
```

## 輸出格式範例

### 標準格式

```
===== 感測器數據 =====
座標: X=12.34 cm, Y=56.78 cm
偏移: dX=0.12 cm, dY=0.34 cm
角度: Z=45.6°
羅盤: 123.4° (原始: 125.6°)
姿態: Roll=2.3°, Pitch=-1.5°
=====================
```

### JSON 格式 (啟用 j+)

```json
{"X":12.34,"Y":56.78,"dX":0.12,"dY":0.34,"angleZ":45.6,"compass":123.4,"rawCompass":125.6,"roll":2.3,"pitch":-1.5}
```

## 故障排除

### 問題 1: 未偵測到 I2C 設備

**可能原因:**

- 硬體連接錯誤
- 缺少上拉電阻
- HUB-PAA5163D 未啟用 I2C Slave 模式

**解決方法:**

1. 檢查 SDA/SCL 連接
2. 確認上拉電阻已安裝
3. 確認 HUB-PAA5163D 的 PA0/PA1 在開機時為 HIGH
4. 檢查共地連接

### 問題 2: 讀取 ID 異常

**可能原因:**

- I2C 通訊錯誤
- HUB-PAA5163D 程序未正確運行

**解決方法:**

1. 重新啟動 HUB-PAA5163D
2. 確認 HUB-PAA5163D 已燒錄正確的 firmware
3. 檢查 I2C 速度設定 (預設 400kHz)

### 問題 3: 校正逾時

**可能原因:**

- HUB-PAA5163D 忙碌中
- I2C 通訊不穩定

**解決方法:**

1. 等待 HUB-PAA5163D 完成當前操作
2. 重試校正命令
3. 降低 I2C 速度 (修改 I2C_SPEED)

### 問題 4: 數據不更新

**可能原因:**

- 報告間隔設定過長
- HUB-PAA5163D 未啟動運動追蹤

**解決方法:**

1. 檢查報告間隔設定 (輸入 'i')
2. 確認 HUB-PAA5163D 上的感測器正常工作
3. 嘗試設定零點 (輸入 'z')

## 進階功能

### 自訂校正資料

可以透過修改程序直接寫入校正資料:

```cpp
writeFloat(REG_COMPASS_MIN_X, -500.0);
writeFloat(REG_COMPASS_MAX_X, 500.0);
// ... 其他軸
// 設定完成後觸發應用
setControlBit(CTRL_BIT_SET_CMPS_CAL_DATA, true);
```

### 監控特定資料

修改 `readAllData()` 函數以只讀取需要的資料:

```cpp
void readAllData() {
  float x = readFloat(REG_X);
  float y = readFloat(REG_Y);
  float angleZ = readFloat(REG_ANGLE_Z);
  
  Serial.printf("Pos: (%.2f, %.2f) @ %.1f°\n", x, y, angleZ);
}
```

## 技術細節

### I2C 通訊協定

1. **寫入**: Master 發送寄存器地址 + 資料
2. **讀取**: Master 發送寄存器地址，然後讀取資料
3. **位址寬度**: 8-bit
4. **字節序**: Little-endian (LSB first)

### Float 資料格式

使用 IEEE 754 單精度浮點數格式 (32-bit):

- 4 bytes, Little-endian
- 範圍: ±3.4E38
- 精度: ~7 位有效數字

## 參考資料

- HUB-PAA5163D 資料手冊
- HUB5168+ 資料手冊
- I2C 規格書

## 授權

此測試程序僅供測試與開發使用。

## 版本歷史

- v1.0 (2026-01-09): 初始版本
  - 基本 I2C 讀寫功能
  - 感測器數據讀取
  - 羅盤校正功能
  - 零點設定
  - JSON 輸出支援
