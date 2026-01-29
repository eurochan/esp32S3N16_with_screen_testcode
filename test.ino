#include <Arduino_GFX_Library.h>
#include <WiFi.h>
#include <Preferences.h> 
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "time.h" 
#include "QMI8658.h" 

// ============================================================
// ⚙️ 大佬配置區域 (User Configuration)
// 喺呢度改完就算，唔使落去搞啲邏輯代碼，方便快捷！🚀
// ============================================================
namespace Config {
    // --- 1. 🔌 硬件接線 (LilyGo T-Display S3) ---
    // 呢啲係板子定死咗嘅，通常唔使郁佢
    constexpr int PIN_SCLK = 40;
    constexpr int PIN_MOSI = 45;
    constexpr int PIN_MISO = -1;
    constexpr int PIN_CS   = 42;
    constexpr int PIN_DC   = 41;
    constexpr int PIN_RST  = 39;
    
    // 👇 呢兩個係控制開關嘅主角
    constexpr int PIN_BL   = 46; // 💡 屏幕背光引腳
    constexpr int PIN_LED  = 15; // 🟢 主板粒綠色 LED (LilyGo S3 默認係 15)

    // --- 2. 📡 藍牙設置 ---
    // 手機搜藍牙嗰陣顯示嘅名，鍾意改乜就改乜
    const char* BLE_NAME           = "LilyGo-Config";
    const char* UUID_SERVICE       = "4fafc201-1fb5-459e-8fcc-c5c9c331914b";
    const char* UUID_RX            = "beb5483e-36e1-4688-b7f5-ea07361b26a8";

    // --- 3. ⏰ NTP & 時間設置 ---
    const char* NTP_SERVER_1       = "ntp1.aliyun.com"; // 首選阿里雲，夠快
    const char* NTP_SERVER_2       = "pool.ntp.org";    // 備用池
    constexpr long  GMT_OFFSET_SEC = 8 * 3600;          // 🌏 時區：UTC+8 (香港/北京時間)
    constexpr int   DAYLIGHT_OFFSET= 0;                 // 夏令時？唔使理佢

    // --- 4. ⚡ 刷新率設置 (毫秒 ms) ---
    constexpr int RATE_SENSOR_MS   = 50;    // 陀螺儀幾耐望一次？(越細越靈敏，但也越食電)
    constexpr int RATE_SCREEN_MS   = 1000;  // 個芒幾耐更新一次時間？(1秒一次夠做啦)
    constexpr int WIFI_CHECK_MS    = 10000; // 斷網之後，隔幾耐試下連返？

    // --- 5. 🔄 喚醒/息屏閾值 (遲滯區間) ---
    // 💡 邏輯：負數 -> 拿高開屏，正數 -> 垂低熄屏
    // ⚠️ 貼士：中間留個位 (-0.2 到 0.2) 唔好郁，費事手震嗰陣個芒閃黎閃去
    constexpr float THRESHOLD_TURN_ON  = -0.2; // Y 細過呢個數 (拿高) -> 即刻醒！👀
    constexpr float THRESHOLD_TURN_OFF =  0.2; // Y 大過呢個數 (垂低) -> 瞓覺！💤

    // --- 6. 🎨 界面佈局 (UI Layout) ---
    // [主區域] 顯示時間
    constexpr int UI_MAIN_X        = 10;    // 左邊留幾多空位
    constexpr int UI_MAIN_Y        = 40;    // 頂頭留幾多空位
    constexpr int UI_MAIN_SIZE     = 3;     // 字體大細 (2號字)

    // [狀態欄] 顯示 IP / WiFi 狀態
    constexpr int UI_STATUS_X      = 10;    
    constexpr int UI_STATUS_Y      = 100;   // 擺低啲，費事撞到上面個時間
    constexpr int UI_STATUS_SIZE   = 1;     // 🔍 特登整大隻啲，老花都睇得清！
}
// ============================================================

// 🖥️ 初始化屏幕驅動，呢堆嘢照抄就得
Arduino_DataBus *bus = new Arduino_ESP32SPI(
    Config::PIN_DC, Config::PIN_CS, Config::PIN_SCLK, Config::PIN_MOSI, Config::PIN_MISO);
Arduino_GFX *gfx = new Arduino_ST7789(bus, Config::PIN_RST, 1, true, 172, 320, 34, 0, 34, 0);

Preferences preferences;         // 💾 用嚟記住 WiFi 密碼嘅
BLECharacteristic *pCharacteristic;
String wifi_ssid = "";
String wifi_pass = "";
bool newCredentialsReceived = false; // 📶 係咪收到新密碼？

// --- 🏃‍♂️ 運行時變量 (Runtime Variables) ---
unsigned long lastSensorTime = 0; 
unsigned long lastScreenTime = 0; 
unsigned long lastWiFiCheck = 0;
bool isScreenOn = true;         // 記錄依家個芒係著緊定熄緊
bool forceScreenUpdate = false; // 係咪要強制刷新畫面？

// 🛠️ 輔助函數：一鍵控制屏幕同 LED 燈
// active = true (開工), active = false (收工)
void setDeviceActive(bool active) {
    isScreenOn = active;
    if (active) {
        digitalWrite(Config::PIN_BL, HIGH);  // 💡 開背光
        digitalWrite(Config::PIN_LED, HIGH); // 🟢 開綠燈 (同步！)
    } else {
        digitalWrite(Config::PIN_BL, LOW);   // 🌑 熄背光
        digitalWrite(Config::PIN_LED, LOW);  // ⚫ 熄綠燈 (慳電！)
    }
}

// 🎨 屏幕畫圖函數
void updateDisplay(String msg) {
    // 🌚 如果個芒已經熄咗，就慳返啖氣，唔好浪費 CPU 畫圖啦
    if (!isScreenOn) return; 

    gfx->fillScreen(0x0000);   // 刷黑底
    gfx->setTextColor(0xFFFF); // 用白字
    
    // 🕒 寫時間
    gfx->setCursor(Config::UI_MAIN_X, Config::UI_MAIN_Y);
    gfx->setTextSize(Config::UI_MAIN_SIZE);
    gfx->println(msg);
    
    // 📶 寫 WiFi 狀態
    gfx->setCursor(Config::UI_STATUS_X, Config::UI_STATUS_Y);
    gfx->setTextSize(Config::UI_STATUS_SIZE); 
    
    if(WiFi.status() == WL_CONNECTED) {
        gfx->println(WiFi.localIP()); // 連到就顯示 IP
    } else {
        gfx->println("Searching..."); // 未連到就話搵緊
    }
}

// 🕰️ 獲取當前時間字符串 (HH:MM:SS)
String getTimeString() {
    struct tm timeinfo;
    if(WiFi.status() != WL_CONNECTED) return "--:--:--"; // 冇網就顯示橫線
    if(!getLocalTime(&timeinfo)){
        return "--:--:--"; // 攞唔到時間都係橫線
    }
    char timeStringBuff[50];
    strftime(timeStringBuff, sizeof(timeStringBuff), "%H:%M:%S", &timeinfo);
    return String(timeStringBuff);
}

// 🦷 藍牙回調：手機發 WiFi 密碼過嚟嗰陣會入呢度
class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        String rxValue = pCharacteristic->getValue(); 
        if (rxValue.length() > 0) {
            // 格式預期係 "SSID,PASSWORD"
            int commaIndex = rxValue.indexOf(',');
            if (commaIndex > 0) {
                String new_ssid = rxValue.substring(0, commaIndex);
                String new_pass = rxValue.substring(commaIndex + 1);
                new_ssid.trim(); new_pass.trim(); // 去除頭尾空格
                
                // 💾 寫入永久存儲區 (NVS)
                preferences.begin("wifi-config", false);
                preferences.putString("ssid", new_ssid);
                preferences.putString("pass", new_pass);
                preferences.end();

                wifi_ssid = new_ssid;
                wifi_pass = new_pass;
                newCredentialsReceived = true; // 🚩 舉旗，話俾主循環知有新嘢
            }
        }
    }
};

// 🦷 初始化藍牙
void initBLE() {
    BLEDevice::init(Config::BLE_NAME);
    BLEServer *pServer = BLEDevice::createServer();
    BLEService *pService = pServer->createService(Config::UUID_SERVICE);
    pCharacteristic = pService->createCharacteristic(Config::UUID_RX, BLECharacteristic::PROPERTY_WRITE);
    pCharacteristic->setCallbacks(new MyCallbacks());
    pService->start();
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(Config::UUID_SERVICE);
    pAdvertising->setScanResponse(true);
    BLEDevice::startAdvertising();
}

// 📶 WiFi 保活：斷咗線幫你連返
void maintainWiFi() {
    if (wifi_ssid == "") return; // 冇密碼就唔使搞啦
    if (WiFi.status() != WL_CONNECTED) {
        if (millis() - lastWiFiCheck > Config::WIFI_CHECK_MS) {
            lastWiFiCheck = millis();
            Serial.println("WiFi lost. Reconnecting... (斷網重連中)");
            WiFi.reconnect(); 
        }
    }
}

// 🚀 開機 Setup
void setup() {
    Serial.begin(115200);
    
    // 🔌 初始化引腳
    pinMode(Config::PIN_BL, OUTPUT);
    pinMode(Config::PIN_LED, OUTPUT); 
    
    // 💡 一開機先著燈，話俾人知我醒咗
    setDeviceActive(true);
    
    gfx->begin();
    updateDisplay("Init... (啟動中)");

    QMI8658_Init(); // 啟動陀螺儀

    // 📖 讀取存儲嘅 WiFi 密碼
    preferences.begin("wifi-config", true);
    wifi_ssid = preferences.getString("ssid", "");
    wifi_pass = preferences.getString("pass", "");
    preferences.end();

    initBLE(); // 啟動藍牙
    
    WiFi.mode(WIFI_STA); 
    WiFi.setAutoReconnect(true); 
    // 🌍 對時
    configTime(Config::GMT_OFFSET_SEC, Config::DAYLIGHT_OFFSET, Config::NTP_SERVER_1, Config::NTP_SERVER_2);

    if (wifi_ssid != "") {
        updateDisplay("WiFi... (連線中)");
        WiFi.begin(wifi_ssid.c_str(), wifi_pass.c_str());
    }
}

// 🔄 死循環 Loop
void loop() {
    unsigned long now = millis();

    // 1️⃣ 處理藍牙新密碼
    if (newCredentialsReceived) {
        newCredentialsReceived = false;
        setDeviceActive(true); // 有新嘢一定要著芒睇下
        updateDisplay("Updating... (更新中)");
        WiFi.disconnect();
        WiFi.begin(wifi_ssid.c_str(), wifi_pass.c_str());
    }

    // 2️⃣ 檢查 WiFi 狀態
    maintainWiFi();

    // 3️⃣ 傳感器任務 (每 50ms 做一次)
    if (now - lastSensorTime > Config::RATE_SENSOR_MS) {
        lastSensorTime = now;
        QMI8658_Loop();
        
        // 🖨️ 串口打印調試信息 (Y軸角度 | 屏幕狀態)
        Serial.printf("Accel Y: %.2f | Screen: %s\n", Accel.y, isScreenOn ? "ON" : "OFF");

        // 👇 核心邏輯：睇下個 Y 軸點樣郁
        
        // 情況 A：依家著緊燈，但你將個裝置垂低咗 (Y > 0.2)
        if (isScreenOn && Accel.y > Config::THRESHOLD_TURN_OFF) {
            setDeviceActive(false); // 💤 熄芒 + 熄綠燈
        } 
        // 情況 B：依家熄緊燈，但你拿高咗個裝置 (Y < -0.2)
        else if (!isScreenOn && Accel.y < Config::THRESHOLD_TURN_ON) {
            setDeviceActive(true);  // 💡 開芒 + 開綠燈
            forceScreenUpdate = true; // ⚡ 醒咗即刻刷新畫面，唔好等！
        }
    }

    // 4️⃣ 屏幕刷新任務 (每 1秒 做一次，或者被強制喚醒時做)
    if (isScreenOn && ((now - lastScreenTime > Config::RATE_SCREEN_MS) || forceScreenUpdate)) {
        lastScreenTime = now;
        forceScreenUpdate = false; 

        String statusMsg = "";
        statusMsg += "TIME: " + getTimeString(); 
        
        updateDisplay(statusMsg);
    }
}
