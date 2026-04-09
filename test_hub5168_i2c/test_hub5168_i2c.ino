/**
 * @file test_hub5168_i2c.ino
 * @brief Hub5168 (BW16) Arduino I2C Master 測試程序
 * @description 用於測試 HT32F491x3 (I2C Slave 地址 0x52) 的運動追蹤功能
 * 
 * 硬體連接:
 * - BW16 SDA (PA26) <-> HT32 PA0 (I2C2 SDA)
 * - BW16 SCL (PA25) <-> HT32 PA1 (I2C2 SCL)
 * - 共地 GND
 * - 兩個引腳需上拉電阻 (4.7K) 到 3.3V
 * 
 * I2C Slave 寄存器結構 (總共 85 bytes):
 * Offset | Size | Name               | Description
 * -------|------|--------------------|---------------------------------
 * 0      | 1    | id                 | 固定值 0xA5
 * 1      | 1    | controlBits        | 控制位元
 * 2      | 2    | reportIntervalMs   | 報告間隔時間 (ms)
 * 4      | 1    | highI              | 感測器高度 (mm)
 * 5      | 4    | X                  | X 座標 (cm)
 * 9      | 4    | Y                  | Y 座標 (cm)
 * 13     | 4    | dX                 | X 偏移 (cm)
 * 17     | 4    | dY                 | Y 偏移 (cm)
 * 21     | 4    | angleZ             | Z 角度 (度)
 * 25     | 4    | compassHeading     | 羅盤航向角 (度)
 * 29     | 4    | rawCompassHeading  | 原始羅盤角度 (度)
 * 33     | 4    | roll               | Roll 角度 (度)
 * 37     | 4    | pitch              | Pitch 角度 (度)
 * 41     | 4    | compassMinX        | 羅盤校正最小值 X
 * 45     | 4    | compassMaxX        | 羅盤校正最大值 X
 * 49     | 4    | compassMinY        | 羅盤校正最小值 Y
 * 53     | 4    | compassMaxY        | 羅盤校正最大值 Y
 * 57     | 4    | compassMinZ        | 羅盤校正最小值 Z
 * 61     | 4    | compassMaxZ        | 羅盤校正最大值 Z
 * 
 * controlBits 定義:
 * bit0: movePin enable(1)/disable(0)
 * bit1: set zero point (1=in progress, 0=finished)
 * bit2: start calibrate compass (1=in progress, 0=finished)
 * bit3: calibrate compass with old max/min (1=in progress, 0=finished)
 * bit4: set compass calibration data (1=in progress, 0=finished)
 * bit5: PAA5163 NRST pin state (1=high, 0=low)
 */

#include <Arduino.h> 
#include <Wire.h>

// I2C 配置
// I2C 地址說明：
//   - 7-bit 地址：0x52 (軟體層面使用)
//   - 8-bit 寫入地址：0xA4 (0x52 << 1 | 0) - 物理線路上傳輸
//   - 8-bit 讀取地址：0xA5 (0x52 << 1 | 1) - 物理線路上傳輸
// Arduino Wire 庫使用 7-bit 地址，會自動處理 R/W 位
#define I2C_SLAVE_ADDR      0x52      // HT32 I2C Slave 7-bit 地址
#define I2C_SLAVE_ADDR_W    0xA4      // 8-bit 寫入地址 (0x52 << 1 | 0)
#define I2C_SLAVE_ADDR_R    0xA5      // 8-bit 讀取地址 (0x52 << 1 | 1)
#define I2C_SPEED           200000    // 200kHz

// 寄存器偏移地址 (參考 main.cpp i2c_slave_regs_t 結構體)
#define REG_ID                    0    // uint8_t: 固定 ID 值 0xA5
#define REG_CONTROL_BITS          1    // uint8_t: control bits
#define REG_REPORT_INTERVAL_MS    2    // uint16_t (2 bytes): 報告間隔時間/ms
#define REG_HIGH_I                4    // uint8_t: sensor 高度/mm
#define REG_X                     5    // float32_t (4 bytes): X 座標/cm
#define REG_Y                     9    // float32_t (4 bytes): Y 座標/cm
#define REG_DX                    13   // float32_t (4 bytes): X 偏移/cm
#define REG_DY                    17   // float32_t (4 bytes): Y 偏移/cm
#define REG_ANGLE_Z               21   // float32_t (4 bytes): Z 角度/度 -180~+180
#define REG_COMPASS_HEADING       25   // float32_t (4 bytes): 電子羅盤航向角/度 0~360
#define REG_RAW_COMPASS_HEADING   29   // float32_t (4 bytes): IMU 陀螺儀角度/度
#define REG_ROLL                  33   // float32_t (4 bytes): roll 角度/度
#define REG_PITCH                 37   // float32_t (4 bytes): pitch 角度/度
#define REG_COMPASS_MIN_X         41   // uint16_t (2 bytes): 電子羅盤校正最小值 X
#define REG_COMPASS_MAX_X         43   // uint16_t (2 bytes): 電子羅盤校正最大值 X
#define REG_COMPASS_MIN_Y         45   // uint16_t (2 bytes): 電子羅盤校正最小值 Y
#define REG_COMPASS_MAX_Y         47   // uint16_t (2 bytes): 電子羅盤校正最大值 Y
#define REG_COMPASS_MIN_Z         49   // uint16_t (2 bytes): 電子羅盤校正最小值 Z
#define REG_COMPASS_MAX_Z         51   // uint16_t (2 bytes): 電子羅盤校正最大值 Z

// controlBits 位元定義
#define CTRL_BIT_MOVE_PIN_EN        (1 << 0)
#define CTRL_BIT_SET_ZERO_PROGRESS  (1 << 1)
#define CTRL_BIT_CMPS_CAL_START     (1 << 2)
#define CTRL_BIT_CMPS_CAL_OLD_START (1 << 3)
#define CTRL_BIT_SET_CMPS_CAL_DATA  (1 << 4)
#define CTRL_BIT_PAA_NRST_PIN       (1 << 5)
#define CTRL_BIT_UPDATE_SENSORS     (1 << 6)

// 全域變數
uint32_t lastReadTime = 0;
uint32_t readInterval = 0xFFFFFFFF;  // 讀取間隔 (ms)，預設停用自動讀取
bool jsonOutput = false;              // JSON 格式輸出

// 函式宣告
void printMenu();
void handleCommand();
uint8_t readByte(uint8_t regAddr);
uint16_t readWord(uint8_t regAddr);
float readFloat(uint8_t regAddr);
void writeByte(uint8_t regAddr, uint8_t value);
void writeWord(uint8_t regAddr, uint16_t value);
void writeFloat(uint8_t regAddr, float value);
void readAllData();
void setZeroPoint();
void calibrateCompass();
void calibrateCompassOld();
void setReportInterval(uint16_t ms);
void setSensorHeight(uint8_t height);
void enableMovePin(bool enable);
void setControlBit(uint8_t bit, bool value);
bool updateGoOK();

void setup() {
    Serial.begin(115200);
    while (!Serial) {
        delay(10);
    }
  
    Serial.println("\r\n\r\n===========================================");
    Serial.println("Hub5168 I2C Master Test Program");
    Serial.println("Target: HUB-PAA5163D Motion Tracking Module (Addr: 0x52)");
    Serial.println("===========================================\r\n");
  
    // 初始化 I2C
    Wire.begin();
    Wire.setClock(I2C_SPEED);
  
    delay(500);
    
    // 嘗試讀取 ID
    uint8_t id = readByte(REG_ID);
    if (id == 0xA5) {
        Serial.println("✓ Successfully connected to PAA5163D Motion Tracking Module!");
        char buf[32];
        sprintf(buf, "  ID: 0x%02X\r\n\r\n", id);
        Serial.print(buf);
    } else {
        Serial.print("⚠ ID read error: 0x");
        Serial.print(id, HEX);
        Serial.println(" (expected: 0xA5)\r\n");
    }
  
    printMenu();
}

void loop() {
    // 處理串列命令
    if (Serial.available()) {
        handleCommand();
    }
    
    // 定期讀取數據
    if ((millis() - lastReadTime) >= readInterval) {
        lastReadTime = millis();
        if (updateGoOK()) {
            readAllData();
        } else {
            Serial.println("✗ Sensor data update timeout");
        }
    }
}

void printMenu() {
    Serial.println("\n========== Command Menu ==========");
    Serial.println("Data Reading:");
    Serial.println("  ssr     - Read all sensor data");
    Serial.println("  inf     - Read device info");
    Serial.println("  id      - Read device ID");
    Serial.println("  ctrl    - Read control bits");
    Serial.println("  x       - Read X");
    Serial.println("  y       - Read Y");
    Serial.println("  dxy     - Read dX and dY");
    Serial.println("  Az      - Read angle Z");
    Serial.println("  ch      - Read compass heading");
    Serial.println("  rch     - Read raw compass heading");
    Serial.println("  rll     - Read roll");
    Serial.println("  ptch    - Read pitch");   
    Serial.println("  cd      - Read compass calibration data");
    Serial.println("  ca      - Set compass calibration data");
    Serial.println("");
    Serial.println("Control Commands:");
    Serial.println("  z  - Set zero point (reset coordinates and angles)");
    Serial.println("  c  - Start compass calibration (rotate for 20 seconds)");
    Serial.println("  o  - Calibrate compass with old values (rotate for 20 seconds)");
    Serial.println("  m+ - Enable motion indicator output");
    Serial.println("  m- - Disable motion indicator output");
    Serial.println("");
    Serial.println("Configuration Commands:");
    Serial.println("  t<ms>  - Set report interval (e.g. t200)");
    Serial.println("  h<mm>  - Set sensor height (e.g. h10)");
    Serial.println("  a+     - Enable auto-read");
    Serial.println("  a-     - Disable auto-read");
    Serial.println("  j+     - Enable JSON format output");
    Serial.println("  j-     - Disable JSON format output");
    Serial.println("");
    Serial.println("Other:");
    Serial.println("  ?  - Show this menu");
    Serial.println("==================================\r\n");
}

char buf[256];

void handleCommand() {
    String cmd_line = Serial.readStringUntil('\n');  // 改用 '\n' 作為終止符，更通用

    cmd_line.trim();  // 移除所有前後空白字符，包括 '\r', '\n', 空格等
    cmd_line.toLowerCase();

    int spaceIndex = cmd_line.indexOf(' ');
    String cmd = (spaceIndex == -1) ? cmd_line : cmd_line.substring(0, spaceIndex);
  
    if (cmd == "?") {
        printMenu();
    }
    else if (cmd == "ssr") {
        if (updateGoOK()) {
            readAllData();
        } else {
            Serial.println("✗ Sensor data update timeout");
        }
    }
      else if (cmd == "inf") {
        Serial.println("\n===== Device Info =====");
        uint8_t id = readByte(REG_ID);
        uint8_t ctrl = readByte(REG_CONTROL_BITS);
        uint16_t interval = readWord(REG_REPORT_INTERVAL_MS);
        uint8_t height = readByte(REG_HIGH_I) + 5;
        
        sprintf(buf, "ID: 0x%02X %s\r\n", id, (id == 0xA5) ? "✓" : "✗");
        Serial.print(buf);
        sprintf(buf, "Control Bits: 0x%02X\r\n", ctrl);
        Serial.print(buf);
        sprintf(buf, "Motion Output: %s\r\n", (ctrl & CTRL_BIT_MOVE_PIN_EN) ? "Enabled" : "Disabled");
        Serial.print(buf);
        sprintf(buf, "Set Zero Point: %s\r\n", (ctrl & CTRL_BIT_SET_ZERO_PROGRESS) ? "In Progress" : "Completed");
        Serial.print(buf);
        sprintf(buf, "Compass Calibration: %s\r\n", (ctrl & CTRL_BIT_CMPS_CAL_START) ? "In Progress" : "Completed");
        Serial.print(buf);
        sprintf(buf, "Report Interval: %d ms\r\n", interval);
        Serial.print(buf);
        sprintf(buf, "Sensor Height: %d mm\r\n", height);  // 實際高度 = 註冊值 + 10 mm
        Serial.print(buf);
        Serial.println("=======================\r\n");
    }
      else if (cmd == "id") {
        uint8_t id = readByte(REG_ID);
        sprintf(buf, "DEVICE ID: 0x%02X %s\r\n", id, (id == 0xA5) ? "✓" : "✗");
        Serial.print(buf);
    }
    else if (cmd == "ctrl") {
        uint8_t ctrl = readByte(REG_CONTROL_BITS);
        sprintf(buf, "Control Bits: 0x%02X\r\n", ctrl);
        Serial.print(buf);
        sprintf(buf, "Motion Output: %s\r\n", (ctrl & CTRL_BIT_MOVE_PIN_EN) ? "Enabled" : "Disabled");
        Serial.print(buf);
        sprintf(buf, "Set Zero Point: %s\r\n", (ctrl & CTRL_BIT_SET_ZERO_PROGRESS) ? "In Progress" : "Completed");
        Serial.print(buf);
        sprintf(buf, "Compass Calibration: %s\r\n", (ctrl & CTRL_BIT_CMPS_CAL_START) ? "In Progress" : "Completed");
        Serial.print(buf);
    }
      else if (cmd == "x") {
        if (updateGoOK()) {
            union {
                uint8_t rawBytes[4];
                float fValue;
            } u;
            u.fValue = readFloat(REG_X);
            Serial.print("X ");
            Serial.print(u.fValue, 2);
            Serial.print("cm  ");
            Serial.print("Raw: ");
            for (int i = 0; i < 4; i++) {
                Serial.print(u.rawBytes[i], HEX);
                Serial.print(" ");
            }
            Serial.print("\r\n");
        } else {
            Serial.println("✗ Sensor data update timeout");
        }
    }
      else if (cmd == "y") {
        if (updateGoOK()) {
            union {
                uint8_t rawBytes[4];
                float fValue;
            } u;
            u.fValue = readFloat(REG_Y);
            Serial.print("Y ");
            Serial.print(u.fValue, 2);
            Serial.print("cm  ");
            Serial.print("Raw: ");
            for (int i = 0; i < 4; i++) {
                Serial.print(u.rawBytes[i], HEX);
                Serial.print(" ");
            }
            Serial.print("\r\n");
        } else {
            Serial.println("✗ Sensor data update timeout");
        }
    }
      else if (cmd == "dxy") {
        if (updateGoOK()) {
            union {
                uint8_t rawBytes[4];
                float fValue;
            } u;
            u.fValue = readFloat(REG_DX);
            float ddx = u.fValue;
            Serial.print("dX ");
            Serial.print(ddx, 2);
            Serial.print("cpi  ");
            Serial.print("Raw: ");
            for (int i = 0; i < 4; i++) {
                Serial.print(u.rawBytes[i], HEX);
                Serial.print(" ");
            }
            Serial.print("\r\n");
            u.fValue = readFloat(REG_DY);
            float ddy = u.fValue;
            Serial.print("dY ");
            Serial.print(ddy, 2);
            Serial.print("cpi  ");
            Serial.print("Raw: ");
            for (int i = 0; i < 4; i++) {
                Serial.print(u.rawBytes[i], HEX);
                Serial.print(" ");
            }
            Serial.print("\r\n");
        } else {
            Serial.println("✗ Sensor data update timeout");
        }
    }
      else if (cmd == "az") {
        if (updateGoOK()) {
            union {
                uint8_t rawBytes[4];
                float fValue;
            } u;
            u.fValue = readFloat(REG_ANGLE_Z);
            float az = u.fValue;
            Serial.print("Angle Z ");
            Serial.print(az, 2);
            Serial.print("°  ");
            Serial.print("Raw: ");
            for (int i = 0; i < 4; i++) {
                Serial.print(u.rawBytes[i], HEX);
                Serial.print(" ");
            }
            Serial.print("\r\n");
        } else {
            Serial.println("✗ Sensor data update timeout");
        }
    }
      else if (cmd == "ch") {
        if (updateGoOK()) {
            union {
                uint8_t rawBytes[4];
                float fValue;
            } u;
            u.fValue = readFloat(REG_COMPASS_HEADING);
            float ch = u.fValue;
            Serial.print("Compass Heading ");
            Serial.print(ch, 2);
            Serial.print("°  ");
            Serial.print("Raw: ");
            for (int i = 0; i < 4; i++) {
                Serial.print(u.rawBytes[i], HEX);
                Serial.print(" ");
            }
            Serial.print("\r\n");
        } else {
            Serial.println("✗ Sensor data update timeout");
        }
    }
      else if (cmd == "rch") {
        if (updateGoOK()) {
            union {
                uint8_t rawBytes[4];
                float fValue;
            } u;
            u.fValue = readFloat(REG_RAW_COMPASS_HEADING);
            float rch = u.fValue;
            Serial.print("Raw Compass Heading ");
            Serial.print(rch, 2);
            Serial.print("°  ");
            Serial.print("Raw: ");
            for (int i = 0; i < 4; i++) {
                Serial.print(u.rawBytes[i], HEX);
                Serial.print(" ");
            }
            Serial.print("\r\n");
        } else {
            Serial.println("✗ Sensor data update timeout");
        }
    }
      else if (cmd == "rll") {
        if (updateGoOK()) {
            union {
                uint8_t rawBytes[4];
                float fValue;
            } u;
            u.fValue = readFloat(REG_ROLL);
            float rll = u.fValue;
            Serial.print("Roll ");
            Serial.print(rll, 2);
            Serial.print("°  ");
            Serial.print("Raw: ");
            for (int i = 0; i < 4; i++) {
                Serial.print(u.rawBytes[i], HEX);
                Serial.print(" ");
            }
            Serial.print("\r\n");
        } else {
            Serial.println("✗ Sensor data update timeout");
        }
    }
      else if (cmd == "ptch") {
        if (updateGoOK()) {
            union {
                uint8_t rawBytes[4];
                float fValue;
            } u;
            u.fValue = readFloat(REG_PITCH);
            float ptch = u.fValue;
            Serial.print("Pitch ");
            Serial.print(ptch, 2);
            Serial.print("°  ");
            Serial.print("Raw: ");
            for (int i = 0; i < 4; i++) {
                Serial.print(u.rawBytes[i], HEX);
                Serial.print(" ");
            }
            Serial.print("\r\n");
        } else {
            Serial.println("✗ Sensor data update timeout");
        }
    }
    else if (cmd == "cd") {
        union {
            uint8_t rawBytes[12];
            struct {
                int16_t minX;
                int16_t maxX;
                int16_t minY;
                int16_t maxY;
                int16_t minZ;
                int16_t maxZ;
            };
        } u;
        u.minX = readWord(REG_COMPASS_MIN_X);
        u.maxX = readWord(REG_COMPASS_MAX_X);
        u.minY = readWord(REG_COMPASS_MIN_Y);
        u.maxY = readWord(REG_COMPASS_MAX_Y);
        u.minZ = readWord(REG_COMPASS_MIN_Z);
        u.maxZ = readWord(REG_COMPASS_MAX_Z);
        
        Serial.println("Calibration Data:");
        sprintf(buf, "  compassMinX:        %5d               [%02X %02X]\n", u.minX, u.rawBytes[0], u.rawBytes[1]);
        Serial.print(buf);
        sprintf(buf, "  compassMaxX:        %5d               [%02X %02X]\n", u.maxX, u.rawBytes[2], u.rawBytes[3]);
        Serial.print(buf);
        sprintf(buf, "  compassMinY:        %5d               [%02X %02X]\n", u.minY, u.rawBytes[4], u.rawBytes[5]);
        Serial.print(buf);
        sprintf(buf, "  compassMaxY:        %5d               [%02X %02X]\n", u.maxY, u.rawBytes[6], u.rawBytes[7]);
        Serial.print(buf);
        sprintf(buf, "  compassMinZ:        %5d               [%02X %02X]\n", u.minZ, u.rawBytes[8], u.rawBytes[9]);
        Serial.print(buf);
        sprintf(buf, "  compassMaxZ:        %5d               [%02X %02X]\n", u.maxZ, u.rawBytes[10], u.rawBytes[11]);
        Serial.print(buf);
    }
    else if (cmd == "ca") {                     // write compass calibration data to device, e.g. "ca 100 2000 150 2100 50 1900" or "ca 100,2000,150,2100,50,1900"
        // 解析命令列參數（支援空格或逗號分隔）
        int values[6] = {0};
        int valueCount = 0;
        int startPos = cmd_line.indexOf(' ') + 1;
        
        while (startPos > 0 && valueCount < 6) {
            // 找出下一個分隔符（空格或逗號）
            int nextSpace = cmd_line.indexOf(' ', startPos);
            int nextComma = cmd_line.indexOf(',', startPos);
            int nextSep = -1;
            
            if (nextSpace == -1 && nextComma == -1) {
                nextSep = -1;
            } else if (nextSpace == -1) {
                nextSep = nextComma;
            } else if (nextComma == -1) {
                nextSep = nextSpace;
            } else {
                nextSep = (nextSpace < nextComma) ? nextSpace : nextComma;
            }
            
            String valueStr;
            if (nextSep == -1) {
                valueStr = cmd_line.substring(startPos);
                startPos = -1;
            } else {
                valueStr = cmd_line.substring(startPos, nextSep);
                startPos = nextSep + 1;
            }
            valueStr.trim();
            if (valueStr.length() > 0) {
                values[valueCount++] = valueStr.toInt();
            }
        }
        
        if (valueCount != 6) {
            Serial.println("✗ Usage: ca <minX> <maxX> <minY> <maxY> <minZ> <maxZ>");
            Serial.println("  Example: ca 100 2000 150 2100 50 1900");
        } else {
            union {
                uint8_t rawBytes[12];
                struct {
                    int16_t minX;
                    int16_t maxX;
                    int16_t minY;
                    int16_t maxY;
                    int16_t minZ;
                    int16_t maxZ;
                };
            } u;
            
            u.minX = (int16_t)values[0];
            u.maxX = (int16_t)values[1];
            u.minY = (int16_t)values[2];
            u.maxY = (int16_t)values[3];
            u.minZ = (int16_t)values[4];
            u.maxZ = (int16_t)values[5];
            
            // 寫入校正數據
            writeWord(REG_COMPASS_MIN_X, u.minX);
            writeWord(REG_COMPASS_MAX_X, u.maxX);
            writeWord(REG_COMPASS_MIN_Y, u.minY);
            writeWord(REG_COMPASS_MAX_Y, u.maxY);
            writeWord(REG_COMPASS_MIN_Z, u.minZ);
            writeWord(REG_COMPASS_MAX_Z, u.maxZ);
            
            // 設定 CTRL_BIT_SET_CMPS_CAL_DATA 位元
            Serial.println("Setting compass calibration data...");
            uint8_t ctrl = readByte(REG_CONTROL_BITS);
            ctrl |= CTRL_BIT_SET_CMPS_CAL_DATA;
            writeByte(REG_CONTROL_BITS, ctrl);
            
            // 等待寫入完成 (最多 3 秒)
            for (int i = 0; i < 300; i++) {
                delay(10);
                ctrl = readByte(REG_CONTROL_BITS);
                if ((ctrl & CTRL_BIT_SET_CMPS_CAL_DATA) == 0) {
                    Serial.println("✓ Compass calibration data written to device");
                    sprintf(buf, "  X: %d ~ %d\n", u.minX, u.maxX);
                    Serial.print(buf);
                    sprintf(buf, "  Y: %d ~ %d\n", u.minY, u.maxY);
                    Serial.print(buf);
                    sprintf(buf, "  Z: %d ~ %d\n", u.minZ, u.maxZ);
                    Serial.print(buf);
                    break;
                }
            }
            if ((ctrl & CTRL_BIT_SET_CMPS_CAL_DATA) != 0) Serial.println("⚠ Compass calibration data write timeout");
        }
        return;
    }
    else if (cmd == "z") {
        setZeroPoint();
    }
    else if (cmd == "c") {
        calibrateCompass();
    }
    else if (cmd == "o") {
        calibrateCompassOld();
    }
    else if (cmd == "m+") {
        enableMovePin(true);
    }
    else if (cmd == "m-") {
        enableMovePin(false);
    }
      else if (cmd.startsWith("t")) {
        int interval = cmd.substring(1).toInt();
        if (interval >= 50 && interval <= 1000) {
            setReportInterval(interval);
        } else {
            Serial.println("✗ Interval range: 50-1000 ms");
        }
    }
    else if (cmd.startsWith("h")) {
        int height = cmd.substring(1).toInt();
        if (height >= 5 && height <= 40) {
            setSensorHeight(height - 5);
        } else {
            Serial.println("✗ Height range: 5-40 mm");
        }
    }
    else if (cmd == "a+") {
        readInterval = 500;
        Serial.println("✓ Auto-read enabled (500ms)");
    }
    else if (cmd == "a-") {
        readInterval = 0xFFFFFFFF;
        Serial.println("✓ Auto-read disabled");
    } 
    else if (cmd == "j+") {
        jsonOutput = true;
        Serial.println("✓ JSON format output enabled");
    }
    else if (cmd == "j-") {
        jsonOutput = false;
        Serial.println("✓ JSON format output disabled");
    }
    else {
        Serial.println("✗ Unknown command, enter ? to view menu");
    }
}

// ========== I2C 讀寫函式 ==========

bool updateGoOK() {
    unsigned long startTime;
    uint8_t ctrl = readByte(REG_CONTROL_BITS);
    ctrl |= CTRL_BIT_UPDATE_SENSORS;
    writeByte(REG_CONTROL_BITS, ctrl);
    startTime = millis();
    do {
        delay(10);
        ctrl = readByte(REG_CONTROL_BITS);
            if ((millis() - startTime) > 800) {  // 800ms timeout
                return false;
            }
    } while (ctrl & CTRL_BIT_UPDATE_SENSORS);
    return true;
}

uint8_t readByte(uint8_t regAddr) {
    Wire.beginTransmission(I2C_SLAVE_ADDR);
    Wire.write(regAddr);
    if (Wire.endTransmission() != 0) {
        return 0;
    }
    Wire.requestFrom(I2C_SLAVE_ADDR, 1);
    uint8_t data = 0;
    uint32_t startTime = micros();

    while (Wire.available() == 0) {
        if (micros() - startTime > 45) return 0;  // timeout: (1/200K)*9*1 = 45us
        delayMicroseconds(10); // 修正某些情況下讀取不到資料的問題
    }
    data = Wire.read();
    return data;
}

uint16_t readWord(uint8_t regAddr) {
    Wire.beginTransmission(I2C_SLAVE_ADDR);
    Wire.write(regAddr);
    if (Wire.endTransmission() != 0) {
        return 0;
    }
    Wire.requestFrom(I2C_SLAVE_ADDR, 2);
    union{
        uint8_t bytes[2];
        uint16_t value;
    } data;

    unsigned long startTime = micros();
    while (Wire.available() < 2) {
        if (micros() - startTime > 135) return 0;  // timeout: (1/200K)*9*3 = 135us
        delayMicroseconds(10); // 修正某些情況下讀取不到資料的問題
    }
    data.bytes[0] = Wire.read();
    data.bytes[1] = Wire.read();
    return data.value;
}

float readFloat(uint8_t regAddr) {
    Wire.beginTransmission(I2C_SLAVE_ADDR);
    Wire.write(regAddr);
    if (Wire.endTransmission() != 0) {
        return 0;
    }
    Wire.requestFrom(I2C_SLAVE_ADDR, 4);

    union {
        uint8_t bytes[4];
        float value;
    } data;

    unsigned long startTime = micros();
    while (Wire.available() < 4) {
        if (micros() - startTime > 225) return 0;  // timeout: (1/200K)*9*5 = 225us
        delayMicroseconds(10); // 修正某些情況下讀取不到資料的問題
    }
    data.bytes[0] = Wire.read();
    data.bytes[1] = Wire.read();
    data.bytes[2] = Wire.read();
    data.bytes[3] = Wire.read();
    return data.value;
}

void writeByte(uint8_t regAddr, uint8_t value) {
    Wire.beginTransmission(I2C_SLAVE_ADDR);
    Wire.write(regAddr);
    Wire.write(value);
    Wire.endTransmission();
    delay(10);  // 等待寫入完成
}

void writeWord(uint8_t regAddr, uint16_t value) {
    Wire.beginTransmission(I2C_SLAVE_ADDR);
    Wire.write(regAddr);
    Wire.write(value & 0xFF);
    Wire.write((value >> 8) & 0xFF);
    Wire.endTransmission();
    delay(10);
}

void writeFloat(uint8_t regAddr, float value) {
    union {
        float f;
        uint8_t bytes[4];
    } data;
    data.f = value;
    
    Wire.beginTransmission(I2C_SLAVE_ADDR);
    Wire.write(regAddr);
    Wire.write(data.bytes, 4);
    Wire.endTransmission();
    delay(10);
}

// ========== 功能函式 ==========

void readAllData() {
    float x = readFloat(REG_X);
    float y = readFloat(REG_Y);
    float dx = readFloat(REG_DX);
    float dy = readFloat(REG_DY);
    float angleZ = readFloat(REG_ANGLE_Z);
    float compassHeading = readFloat(REG_COMPASS_HEADING);
    float rawCompassHeading = readFloat(REG_RAW_COMPASS_HEADING);
    float roll = readFloat(REG_ROLL);
    float pitch = readFloat(REG_PITCH);
    
    if (jsonOutput) {
        Serial.print("{\"X\":");
        Serial.print(x, 2);
        Serial.print(",\"Y\":");
        Serial.print(y, 2);
        Serial.print(",\"dX\":");
        Serial.print(dx, 2);
        Serial.print(",\"dY\":");
        Serial.print(dy, 2);
        Serial.print(",\"angleZ\":");
        Serial.print(angleZ, 2);
        Serial.print(",\"compass\":");
        Serial.print(compassHeading, 2);
        Serial.print(",\"rawCompass\":");
        Serial.print(rawCompassHeading, 2);
        Serial.print(",\"roll\":");
        Serial.print(roll, 2);
        Serial.print(",\"pitch\":");
        Serial.print(pitch, 2);
        Serial.print("}\n");
    } else {
        Serial.println("\n===== Sensor Data =====");
        Serial.print("Position: X=");
        Serial.print(x, 2);
        Serial.print(" cm, Y=");
        Serial.print(y, 2);
        Serial.println(" cm");
        Serial.print("Offset: dX=");
        Serial.print(dx, 2);
        Serial.print(" cpi, dY=");
        Serial.print(dy, 2);
        Serial.println(" cpi");
        Serial.print("Angle: Z=");
        Serial.print(angleZ, 2);
        Serial.println("°");
        Serial.print("Compass: ");
        Serial.print(compassHeading, 2);
        Serial.print("° (Raw: ");
        Serial.print(rawCompassHeading, 2);
        Serial.println("°)");
        Serial.print("Attitude: Roll=");
        Serial.print(roll, 2);
        Serial.print("°, Pitch=");
        Serial.print(pitch, 2);
        Serial.println("°");
        Serial.println("=======================\n");
    }
}

void setZeroPoint() {
    Serial.println("Setting zero point...");
    uint8_t ctrl = readByte(REG_CONTROL_BITS);
    ctrl |= CTRL_BIT_SET_ZERO_PROGRESS;
    writeByte(REG_CONTROL_BITS, ctrl);
    
    // 等待完成 (最多 3 秒)
    for (int i = 0; i < 30; i++) {
        delay(100);
        ctrl = readByte(REG_CONTROL_BITS);
        if ((ctrl & CTRL_BIT_SET_ZERO_PROGRESS) == 0) {
            Serial.println("✓ Zero point set completed");
            return;
        }
    }
    Serial.println("⚠ Zero point setting timeout");
}

void calibrateCompass() {
    Serial.println("Starting compass calibration (new method)...");
    Serial.println("Please rotate device for 20 seconds!");
    uint8_t ctrl = readByte(REG_CONTROL_BITS);
    ctrl |= CTRL_BIT_CMPS_CAL_START;
    writeByte(REG_CONTROL_BITS, ctrl);
    
    // 等待完成 (最多 25 秒)
    for (int i = 0; i < 250; i++) {
        delay(100);
        if (i % 10 == 0) {
            Serial.print(".");
        }
        ctrl = readByte(REG_CONTROL_BITS);
        if ((ctrl & CTRL_BIT_CMPS_CAL_START) == 0) {
            Serial.println("\n✓ Compass calibration completed");
            
            // 讀取校正值
            uint16_t minX = (uint16_t)readWord(REG_COMPASS_MIN_X);
            uint16_t maxX = (uint16_t)readWord(REG_COMPASS_MAX_X);
            uint16_t minY = (uint16_t)readWord(REG_COMPASS_MIN_Y);
            uint16_t maxY = (uint16_t)readWord(REG_COMPASS_MAX_Y);
            uint16_t minZ = (uint16_t)readWord(REG_COMPASS_MIN_Z);
            uint16_t maxZ = (uint16_t)readWord(REG_COMPASS_MAX_Z);
            
            char buf[64];
            Serial.println("Calibration Data:");
            sprintf(buf, "  X: %5u ~ %5u\n", minX, maxX);
            Serial.print(buf);
            sprintf(buf, "  Y: %5u ~ %5u\n", minY, maxY);
            Serial.print(buf);
            sprintf(buf, "  Z: %5u ~ %5u\n", minZ, maxZ);
            Serial.print(buf);
            return;
        }
    }
    Serial.println("\n⚠ Compass calibration timeout");
}

void calibrateCompassOld() {
    Serial.println("Starting compass calibration (old method - using existing max/min values)...");
    Serial.println("Please rotate device for 20 seconds!");
    uint8_t ctrl = readByte(REG_CONTROL_BITS);
    ctrl |= CTRL_BIT_CMPS_CAL_OLD_START;
    writeByte(REG_CONTROL_BITS, ctrl);
    
    // 等待完成 (最多 25 秒)
    for (int i = 0; i < 250; i++) {
        delay(100);
        if (i % 10 == 0) {
            Serial.print(".");
        }
        ctrl = readByte(REG_CONTROL_BITS);
        if ((ctrl & CTRL_BIT_CMPS_CAL_OLD_START) == 0) {
            Serial.println("\n✓ Compass calibration completed");
            
            // 讀取校正值
            uint16_t minX = (uint16_t)readWord(REG_COMPASS_MIN_X);
            uint16_t maxX = (uint16_t)readWord(REG_COMPASS_MAX_X);
            uint16_t minY = (uint16_t)readWord(REG_COMPASS_MIN_Y);
            uint16_t maxY = (uint16_t)readWord(REG_COMPASS_MAX_Y);
            uint16_t minZ = (uint16_t)readWord(REG_COMPASS_MIN_Z);
            uint16_t maxZ = (uint16_t)readWord(REG_COMPASS_MAX_Z);
            
            char buf[64];
            Serial.println("Calibration Data:");
            sprintf(buf, "  X: %5u ~ %5u\n", minX, maxX);
            Serial.print(buf);
            sprintf(buf, "  Y: %5u ~ %5u\n", minY, maxY);
            Serial.print(buf);
            sprintf(buf, "  Z: %5u ~ %5u\n", minZ, maxZ);
            Serial.print(buf);
            return;
        }
    }
    Serial.println("\n⚠ Compass calibration timeout");
}

void setReportInterval(uint16_t ms) {
    writeWord(REG_REPORT_INTERVAL_MS, ms);
    sprintf(buf, "✓ Report interval set to: %u ms\n", ms);
    Serial.print(buf);
}

void setSensorHeight(uint8_t height) {
    writeByte(REG_HIGH_I, height);
    sprintf(buf, "✓ Sensor height set to: %u mm\n", height + 5);
    Serial.print(buf);
}

void enableMovePin(bool enable) {
    uint8_t ctrl = readByte(REG_CONTROL_BITS);
    if (enable) {
        ctrl |= CTRL_BIT_MOVE_PIN_EN;
        Serial.println("✓ Motion indicator output enabled");
    } else {
        ctrl &= ~CTRL_BIT_MOVE_PIN_EN;
        Serial.println("✓ Motion indicator output disabled");
    }
    writeByte(REG_CONTROL_BITS, ctrl);
}

void setControlBit(uint8_t bit, bool value) {
    uint8_t ctrl = readByte(REG_CONTROL_BITS);
    if (value) {
        ctrl |= bit;
    } else {
        ctrl &= ~bit;
    }
    writeByte(REG_CONTROL_BITS, ctrl);
}
