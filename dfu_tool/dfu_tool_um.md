# DFU Flash Tool 使用手冊

## 1. 文件目的

本手冊說明 HUB-PAA5163D 的 DFU 圖形化升級工具使用方式，對象為測試、產線、FAE 與維護人員。

工具主要用途如下：

- 將韌體 `.bin` 檔下載到裝置
- 從裝置上傳目前韌體內容並保存成 `.bin`
- 透過序列埠送出 `at@upgrade+` 指令，讓裝置自動切換進入 DFU 模式
- 顯示影像標頭資訊，例如版本、長度、CRC32、建置時間

## 2. 適用裝置

| 項目                 | 內容           |
| -------------------- | -------------- |
| 裝置                 | HUB-PAA5163D   |
| DFU USB VID          | `04D9`       |
| DFU USB PID          | `DF11`       |
| 應用程式預設起始位址 | `0x08004000` |
| 預設上傳大小         | `0x1C000`    |

常用 Alternate Setting：

- `alt=0`: 應用程式 Flash
- `alt=1`: 內部 Flash

## 3. 系統需求

### 3.1 作業系統

- Linux (ubuntu 佳)
- Windows 11 以上

### 3.2 軟體需求

- linux libusb 系統函式庫

### 3.3 USB 驅動需求

Linux:

- 需要 `libusb-1.0`
- 建議設定 `udev rule`，避免每次都以 `sudo` 執行

Windows:

- 需替 DFU 裝置安裝 WinUSB 或 libusb 相容驅動
- 建議使用 Zadig 安裝驅動

## 4. 安裝方式

### 4.1 Linux 安裝

本工具 Linux 版本採單一執行檔交付，不需安裝 Python 模組。

在目標機確認 `libusb` 後，直接執行：

```bash
chmod +x dfu_tool/dfu_gui
./dfu_tool/dfu_gui
```

若尚未安裝 `libusb`：

```bash
sudo apt install libusb-1.0-0
```

如果尚未設定 USB 權限，可新增 `udev` 規則：

```bash
sudo tee /etc/udev/rules.d/49-dfu.rules <<'EOF'
SUBSYSTEM=="usb", ATTR{idVendor}=="04d9", ATTR{idProduct}=="df11", MODE="0666", GROUP="plugdev"
EOF
sudo udevadm control --reload-rules
sudo udevadm trigger
sudo usermod -aG plugdev $USER
```

加入 `plugdev` 後通常需要重新登入才會生效。

### 4.2 Windows 安裝

Windows 版本亦採單一執行檔交付。

直接執行：

```cmd
dfu_tool\dfu_gui_win.exe
```

再使用 Zadig 將 DFU 裝置驅動切換為 WinUSB 或 libusb 相容驅動。

### 4.3 可執行檔版本

本專案現場部署以單一執行檔為標準：

- Linux: `dfu_tool/dfu_gui`
- Windows: `dfu_tool/dfu_gui_win.exe`

## 5. 啟動方式

### 5.1 單一執行檔啟動

```bash
./dfu_tool/dfu_gui
```

Windows：

```cmd
dfu_tool\dfu_gui_win.exe
```

### 5.2 啟動後畫面區域

主畫面分為四個部分：

- 上方標題列
- 裝置列與 DFU 觸發區
- 中央頁籤區
- 下方輸出日誌區

中央頁籤共有三頁：

- `下載 (Download)`
- `上傳 (Upload)`
- `設定`

## 6. 介面說明

### 6.1 裝置列

裝置列包含下列元件：

- `DFU 裝置` 下拉選單：顯示已偵測到的 DFU 裝置
- `重新偵測`：重新掃描 USB DFU 裝置
- `進入 DFU 模式`：透過序列埠送出 `at@upgrade+`
- `串口 (AT)` 下拉選單：選擇要觸發 DFU 的序列埠
- `掃描串口`：重新掃描可用序列埠

說明：

- 若尚未找到 DFU 裝置，工具會嘗試自動透過 HUB 串口送出進入 DFU 的 AT 指令
- 若使用者手動指定串口，也可直接以該串口重試
- 成功後工具會自動重新偵測 DFU 裝置

### 6.2 下載頁

![](./pic/download.png)

下載頁主要欄位如下：

- `韌體檔案 (.bin)`：要寫入裝置的二進位檔案
- `Flash 位址`：預設為 `0x08004000`
- `Alt Setting`：選擇目標 Flash 區域
- `下載後自動重設 (leave)`：下載完成後要求裝置離開 DFU 模式
- `開始下載`：執行燒錄
- `取消`：中止目前操作

工具會自動嘗試讀取檔案前 24 bytes 的 image header，顯示以下資訊：

- Magic
- Image Length
- CRC32
- Vector Address
- Version
- Build Time

若不是 HUB+ 格式或檔案太小，畫面會顯示警告，但仍可由使用者自行判斷是否繼續操作。

### 6.3 上傳頁

![](./pic/upload.png)

上傳頁主要欄位如下：

- `儲存至 (.bin)`：輸出檔名與保存位置
- `起始位址`：預設為 `0x08004000`
- `讀取大小`：預設 `0x1C000`
- `Alt Setting`：選擇讀取來源區域
- `開始上傳`：讀出裝置內容
- `取消`：中止目前操作

使用 `瀏覽...` 選擇輸出路徑後，工具會先嘗試從裝置讀取 image header。若讀取成功且格式正確，會自動把 `讀取大小` 改成 header 內的影像長度。

### 6.4 設定頁

![](./pic/set.png)

設定頁可調整下列預設值：

- VID
- PID
- 預設 Flash 位址
- 預設 Alt Setting

Windows 版本如支援 USB backend manager，還可選擇 USB 驅動後端。

按下 `儲存設定` 後，設定會寫入：

- `dfu_tool/dfu_gui_config.json`

## 7. 標準操作流程

### 7.1 流程 A: 下載新韌體到裝置

1. 確認裝置已接上 USB。
2. 讓裝置進入 DFU 模式。
3. 啟動工具並確認上方 `DFU 裝置` 已出現目標裝置。
4. 切到 `下載 (Download)` 頁。
5. 選擇欲燒錄的 `.bin` 檔案。
6. 確認 `Flash 位址` 與 `Alt Setting` 是否正確。
7. 視需要決定是否勾選 `下載後自動重設 (leave)`。
8. 按下 `開始下載`。
9. 等待進度條完成，並在下方日誌確認結果為成功。

### 7.2 流程 B: 從裝置備份韌體

1. 確認裝置已進入 DFU 模式。
2. 切到 `上傳 (Upload)` 頁。
3. 按 `瀏覽...` 指定輸出檔案路徑，例如 `firmware_upload.bin`。
4. 檢查工具是否已正確讀出 image header 與影像長度。
5. 如未讀出 header，手動確認 `讀取大小`。
6. 按下 `開始上傳`。
7. 待完成後，到指定路徑確認輸出檔是否存在。

### 7.3 流程 C: 使用 AT 指令自動切換 DFU

適用情境：裝置目前仍在一般韌體模式，且可透過 USB CDC 或 UART 與 PC 通訊。

1. 在工具上方 `串口 (AT)` 選擇 HUB 對應的序列埠。
2. 按下 `進入 DFU 模式`。
3. 工具會送出 `at@upgrade+`。
4. 裝置重新枚舉後，工具會自動重新偵測 DFU 裝置。
5. 若成功偵測，再進行下載或上傳操作。

## 8. 如何讓裝置進入 DFU 模式

可用以下任一方式：

### 8.1 方式一: 透過裝置韌體 AT 指令

- 裝置在一般應用程式模式下運作
- PC 已可透過對應串口與裝置通訊
- 使用工具中的 `進入 DFU 模式` 功能即可

### 8.2 方式二: 透過硬體流程

- 成功後重新插拔或等待 USB 重新枚舉
- 再按工具上的 `重新偵測`

若不確定板端 DFU 進入條件，請依專案的 bootloader 文件或硬體定義操作。

## 9. 下載與上傳的欄位建議

### 9.1 一般應用程式燒錄

- `Flash 位址`: `0x08004000`
- `Alt Setting`: `0`

### 9.2 完整 Flash 操作（不建意)

- 請先確認 bootloader 規劃
- 通常使用 `Alt Setting = 1`
- 不建議未確認記憶體分區前直接覆蓋整顆 Flash

### 9.3 上傳大小

建議優先使用 image header 自動帶出的長度。若必須手動輸入，請確認輸入值為：

- 十進位，例如 `65536`
- 或十六進位，例如 `0x10000`

## 10. 執行結果與輸出檔案

### 10.1 日誌視窗

下方 `輸出日誌` 會顯示：

- DFU 裝置偵測結果
- AT 指令觸發紀錄
- 下載或上傳的執行訊息
- 成功或失敗結論

### 10.2 設定檔

工具會自動儲存設定到：

- `dfu_tool/dfu_gui_config.json`

### 10.3 執行記錄檔

封裝版或部分執行環境會將標準輸出與錯誤寫入：

- `dfu_tool/dfu_gui.log`

若 GUI 視窗沒有顯示完整錯誤，可一併檢查此檔案。

## 11. 常見問題與排除

### 11.1 看不到 DFU 裝置

請依序確認：

- 裝置是否真的已進入 DFU 模式
- USB 線是否可傳輸資料，不是純充電線
- Linux 是否已安裝 `libusb` 並設定 `udev`
- Windows 是否已安裝 WinUSB 或 libusb 相容驅動
- VID/PID 設定是否仍為 `04D9:DF11`

### 11.2 點選進入 DFU 模式沒有反應

請確認：

- 已安裝 `pyserial`
- 選到正確的 HUB 串口
- 裝置目前韌體確實支援 `at@upgrade+`
- 該串口沒有被其他終端工具占用

### 11.3 上傳大小不正確

可能原因如下：

- 影像不是 HUB+ 格式
- image header 無法讀取
- 起始位址或 alt 設定錯誤

處理方式：

- 先確認 `起始位址` 與 `Alt Setting`
- 再手動輸入正確大小後重新上傳

### 11.4 下載後裝置無法啟動

請確認：

- 燒錄檔案是否正確對應目前硬體版本
- Flash 位址是否正確
- 是否誤寫入錯誤的 alt 區域
- 韌體 image header 是否完整且版本正確

## 12. 使用注意事項

- 下載前務必確認燒錄檔案與目標板型相符
- 若要操作完整 Flash，請先確認 bootloader 保護區與分區規劃
- 不建議在系統不穩定或 USB 接觸不良時執行下載
- 若現場使用 Windows，請先完成驅動安裝再交付操作人員

## 13. 建議操作習慣

- 先做一次上傳備份，再執行下載更新
- 保留每次燒錄使用的 `.bin`、版本號與日期紀錄
- 若工具自動讀出 image header，先比對版本資訊再燒錄
- 下載完成後，觀察狀態列與日誌是否同時顯示成功

## 14. 版本資訊

本手冊建立日期：2026-03-11

建議後續若 GUI 欄位、按鈕名稱、預設位址或 DFU 流程有修改，應同步更新本文件。
