// ════════════════════════════════════════════════
//  DentalChairV7.ino  —  ESP32 Master  v7.0
//  ────────────────────────────────────────────
//  Hardware (بدون تغيير):
//   OLED SH1106 128×64 I2C SDA=21 SCL=22
//   Keypad 4×4  rows=13,12,14,27  cols=26,25,33,32
//   SD Card SPI MOSI=23 MISO=19 SCK=18 CS=5
//   DS3231 RTC  same I2C bus
//
//  New in v7:
//   UART1 → ESP8266 Base    TX=17 RX=16 @ 115200
//   UART2 → ESP8266 Assist  TX=4  RX=2  @ 115200
//
//  Architecture:
//   ESP32 Master  →  SlaveComm  →  Base  (chair movement)
//   ESP32 Master  →  SlaveComm  →  Assist (LED/Water/Basin/Suction)
//   Base/Assist   →  SlaveComm callback  →  Master (ADC, buttons)
//
//  New features:
//   • Foot switch control (from Base)
//   • Assistant panel buttons (from Assist)
//   • Safety limits with OLED setup screen
//   • Suction solenoid output
//   • Slave online/offline status
// ════════════════════════════════════════════════

#include "shard.h"
#include "protocol.h"
#include "Keypad.h"
#include "Display.h"
#include "TimeManager.h"
#include "WiFiManager.h"
#include "SDManager.h"
#include "ChairControl.h"
#include "OutputControl.h"
#include "MenuController.h"
#include "DentalWebServer.h"
#include "SlaveComm.h"

// ──────────────────────────────────────────────
//  Global definitions
// ──────────────────────────────────────────────
NotifStatus   notif;
AppState      gAppState = APP_HOME;

Settings gSettings = {
    DEFAULT_WATER_TIME_MS,
    DEFAULT_BASIN_TIME_MS,
    DEFAULT_SUCTION_TIME_MS,
    DEFAULT_HOLD_MS,
    true,
    2,
    true,
    ChairLimits{},
    1,   // assistPos1Slot
    2    // assistPos2Slot
};
ChairPosition gPositions[MAX_POSITIONS];

// ──────────────────────────────────────────────
//  Slave communication callback
//  يُستدعى من SlaveComm كل ما يجي packet
// ──────────────────────────────────────────────
void onSlavePacket(char dst, const char *cmd, int32_t val) {
    // ── ADC من الـ Base ────────────────────────
    if (strcmp(cmd, CMD_ADC_UD) == 0) {
        Chair.setADC((uint16_t)val, Chair.readFwdBack());
        return;
    }
    if (strcmp(cmd, CMD_ADC_FB) == 0) {
        Chair.setADC(Chair.readUpDown(), (uint16_t)val);
        return;
    }

    // ── Foot switch من الـ Base ────────────────
    if (strcmp(cmd, CMD_FOOT_UP) == 0)   { Chair.moveUp();      gAppState = APP_MOVING; return; }
    if (strcmp(cmd, CMD_FOOT_DOWN) == 0) { Chair.moveDown();    gAppState = APP_MOVING; return; }
    if (strcmp(cmd, CMD_FOOT_FWD) == 0)  { Chair.moveForward(); gAppState = APP_MOVING; return; }
    if (strcmp(cmd, CMD_FOOT_BACK) == 0) { Chair.moveBack();    gAppState = APP_MOVING; return; }
    if (strcmp(cmd, CMD_FOOT_REL) == 0)  { Chair.stopAll();     gAppState = APP_HOME;   return; }

    // ── Limit hit من الـ Base ──────────────────
    if (strcmp(cmd, CMD_LIMIT_HIT) == 0) {
        Chair.stopAll();
        Display.showPopup("Safety Limit!", 1500);
        Output.beepBase(300);
        return;
    }

    // ── Assistant panel buttons ────────────────
    if (strcmp(cmd, CMD_BTN_LED) == 0)    { Output.ledToggle();      Display.markDirty(); return; }
    if (strcmp(cmd, CMD_BTN_WATER) == 0)  { Output.waterTimed();     Display.markDirty(); return; }
    if (strcmp(cmd, CMD_BTN_BASIN) == 0)  { Output.basinTimed();     Display.markDirty(); return; }
    if (strcmp(cmd, CMD_BTN_SUCTION) == 0){ Output.suctionTimed();   Display.markDirty(); return; }
    if (strcmp(cmd, CMD_BTN_POS1) == 0) {
        uint8_t slot = gSettings.assistPos1Slot;
        if (slot > 0) Chair.goToPosition(slot);
        return;
    }
    if (strcmp(cmd, CMD_BTN_POS2) == 0) {
        uint8_t slot = gSettings.assistPos2Slot;
        if (slot > 0) Chair.goToPosition(slot);
        return;
    }
}

// ──────────────────────────────────────────────
//  setup()
// ──────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    Wire.begin(OLED_SDA, OLED_SCL);
    Serial.println("\n=== Dental Chair v7.0 ===");

    // SD
    if (SDMgr.begin()) {
        SDMgr.loadSettings(gSettings);
        SDMgr.loadPositions(gPositions, MAX_POSITIONS);
        Serial.println("[SD] Settings loaded");
    }

    // OLED
    Display.begin();
    Display.setScreen(SCREEN_HOME);

    // Keypad
    Keypad.begin();

    // RTC
    TimeMgr.begin();
    TimeMgr.update();

    // Slave communication — يجب قبل Chair + Output
    Comm.begin(onSlavePacket);

    // Chair + Outputs
    Chair.begin();
    Output.begin();

    // بعت الـ limits للـ Base
    {
        ChairLimits &L = gSettings.limits;
        if (L.udMaxSet) Comm.sendToBase(CMD_SET_LIM_UD_MAX, L.udMax);
        if (L.udMinSet) Comm.sendToBase(CMD_SET_LIM_UD_MIN, L.udMin);
        if (L.fbMaxSet) Comm.sendToBase(CMD_SET_LIM_FB_MAX, L.fbMax);
        if (L.fbMinSet) Comm.sendToBase(CMD_SET_LIM_FB_MIN, L.fbMin);
    }

    // Menu
    Menu.begin();

    // WiFi
    WifiManager.begin();

    // Boot beep
    Output.beepBase(80);
    Output.beepAssist(80);

    Serial.println("=== Boot complete ===");
    Serial.println("[v7] Base:    TX=17 RX=16 @ 115200");
    Serial.println("[v7] Assist:  TX=4  RX=2  @ 115200");
}

// ──────────────────────────────────────────────
//  loop()
// ──────────────────────────────────────────────
void loop() {
    // Time
    TimeMgr.update();

    // NTP hourly
    {
        static uint32_t ntpLast = 0;
        if (gSettings.autoSyncTime && notif.wifiConnected && elapsed(ntpLast, 3600000UL))
            TimeMgr.syncNTP();
    }

    // WiFi
    WifiManager.update();

    // WebServer auto-start
    {
        static bool webStarted = false;
        if (!webStarted && WifiManager.isConnected()) {
            WebUI.begin();
            webStarted = true;
            char msg[22];
            snprintf(msg, sizeof(msg), "Web: %.18s", WifiManager.getLocalIP().c_str());
            Display.showPopup(msg, 3000);
        }
        if (webStarted && !WifiManager.isConnected()) webStarted = false;
    }
    WebUI.update();

    // WiFi scan update
    if (Display.currentScreen() == SCREEN_WIFI_SCAN)
        WifiManager.updateScan();

    // ── Slave Communication ───────────────────
    Comm.update();

    // ── Keypad ────────────────────────────────
    {
        KeyState ks = Keypad.scan();
        if (ks.event != EVT_NONE)
            Menu.handleKey(ks);
    }

    // ── Chair auto-move ───────────────────────
    Chair.update();

    // ── Timed outputs ─────────────────────────
    Output.update();

    // ── Limits refresh on limits screen ───────
    if (Display.currentScreen() == SCREEN_LIMITS) {
        static uint32_t limRefresh = 0;
        if (elapsed(limRefresh, 200))
            Display.setLimitsState(0, Chair.readUpDown(), Chair.readFwdBack());
    }

    // ── Display ───────────────────────────────
    Display.update();
}
