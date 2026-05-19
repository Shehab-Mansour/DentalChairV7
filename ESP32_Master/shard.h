#pragma once

// ════════════════════════════════════════════════
//  shard.h  —  Shared definitions  v7.0
//  ESP32 Master Controller
// ════════════════════════════════════════════════

#include <Arduino.h>

// OLED
#define OLED_SDA        21
#define OLED_SCL        22
#define OLED_ADDR       0x3C
#define OLED_W          128
#define OLED_H          64

// Keypad
#define KP_ROW1         13
#define KP_ROW2         12
#define KP_ROW3         14
#define KP_ROW4         27
#define KP_COL1         26
#define KP_COL2         25
#define KP_COL3         33
#define KP_COL4         32

// SD Card
#define SD_MOSI         23
#define SD_MISO         19
#define SD_SCK          18
#define SD_CS           5

// UART to ESP8266 Base (تحت الكرسي)
#define BASE_TX         17
#define BASE_RX         16
#define BASE_BAUD       115200

// UART to ESP8266 Assistant (الأسيستانت بانل)
#define ASST_TX         4
#define ASST_RX         2
#define ASST_BAUD       115200

// Timing
#define DEFAULT_WATER_TIME_MS    3000
#define DEFAULT_BASIN_TIME_MS    5000
#define DEFAULT_SUCTION_TIME_MS  5000
#define DEFAULT_HOLD_MS          300
#define CHAIR_MOVE_INTERVAL_MS   40
#define DEAD_BAND                80
#define DISPLAY_REFRESH_MS       80
#define DEBOUNCE_MS              35
#define SLAVE_TIMEOUT_MS         3000

// Misc
#define MAX_POSITIONS            10
#define WIFI_CONNECT_TIMEOUT_MS  10000UL
#define WIFI_BOOT_TIMEOUT_MS      6000UL
#define WEB_SERVER_PORT           80
#define ADC_MAX                   4095

// ──────────────────────────────────────────────
//  Chair safety limits (v7)
// ──────────────────────────────────────────────
struct ChairLimits {
    uint16_t udMax    = 0;
    uint16_t udMin    = 0;
    uint16_t fbMax    = 0;
    uint16_t fbMin    = 0;
    bool     udMaxSet = false;
    bool     udMinSet = false;
    bool     fbMaxSet = false;
    bool     fbMinSet = false;
};

// ──────────────────────────────────────────────
//  Notification status
// ──────────────────────────────────────────────
struct NotifStatus {
    bool wifiConnected = false;
    bool ledOn         = false;
    bool waterOn       = false;
    bool basinOn       = false;
    bool suctionOn     = false;
    bool chairMoving   = false;
    bool timeSynced    = false;
    bool baseOnline    = false;
    bool assistOnline  = false;
};
extern NotifStatus notif;

// ──────────────────────────────────────────────
//  Chair position
// ──────────────────────────────────────────────
struct ChairPosition {
    uint8_t  slot;
    uint16_t updown;
    uint16_t fwdback;
    bool     saved;
    uint8_t  hotkey;
    char     label[12];
};

// ──────────────────────────────────────────────
//  Settings (v7: limits + suction + assistant slots)
// ──────────────────────────────────────────────
struct Settings {
    uint32_t    waterTimeMs;
    uint32_t    basinTimeMs;
    uint32_t    suctionTimeMs;
    uint16_t    holdThresholdMs;
    bool        use24h;
    int8_t      tzOffsetHours;
    bool        autoSyncTime;
    ChairLimits limits;
    uint8_t     assistPos1Slot;   // slot for assistant Pos1 button
    uint8_t     assistPos2Slot;   // slot for assistant Pos2 button
};

// ──────────────────────────────────────────────
//  Globals
// ──────────────────────────────────────────────
extern Settings      gSettings;
extern ChairPosition gPositions[MAX_POSITIONS];

// ──────────────────────────────────────────────
//  App state machine
// ──────────────────────────────────────────────
enum AppState : uint8_t {
    APP_HOME = 0,
    APP_MENU,
    APP_SETTINGS,
    APP_MOVING,
    APP_AUTO_MOVING,
    APP_WIFI_CONN,
    APP_SAVING,
    APP_ERROR,
    APP_LIMITS,
};
extern AppState gAppState;

// ──────────────────────────────────────────────
//  Helper
// ──────────────────────────────────────────────
inline bool elapsed(uint32_t &last, uint32_t interval) {
    uint32_t now = millis();
    if (now - last >= interval) { last = now; return true; }
    return false;
}
