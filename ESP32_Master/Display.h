#pragma once

// ════════════════════════════════════════════════
//  Display.h  —  OLED SH1106  128×64  UI Engine
//  v5.0 additions:
//  • SCREEN_WIFI_DETAIL    — شبكة متصل بها + IP + أوبشن
//  • SCREEN_WIFI_SAVED     — قائمة الشبكات المحفوظة
//  • SCREEN_WIFI_SETTINGS  — إعدادات WiFi (autoSync, etc)
//  • SCREEN_DATE_SET       — ضبط التاريخ يدوياً
//  • SCREEN_CLOCK_SET محسّن — يضم الوقت + التاريخ + autoSync
// ════════════════════════════════════════════════

#include "shard.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

enum Screen : uint8_t {
    SCREEN_HOME = 0,
    SCREEN_MENU,
    SCREEN_WIFI_SCAN,
    SCREEN_WIFI_PASS,
    SCREEN_WIFI_DETAIL,    // v5
    SCREEN_WIFI_SAVED,     // v5
    SCREEN_WIFI_SETTINGS,  // v5
    SCREEN_WATER_TIME,
    SCREEN_BASIN_TIME,
    SCREEN_SUCTION_TIME,   // v7
    SCREEN_HOLD_TIME,
    SCREEN_CLOCK_SET,
    SCREEN_DATE_SET,       // v5
    SCREEN_POSITIONS,
    SCREEN_LIMITS,         // v7: safety limits setup
    SCREEN_SLAVE_STATUS,   // v7: Base + Assistant online status
    SCREEN_COUNT
};

struct Popup {
    char     msg[22];
    uint32_t untilMs;
    bool     active;
};

class DisplayManager {
public:
    void    begin();
    void    update();
    void    setScreen(Screen s);
    Screen  currentScreen() const { return _screen; }
    void    markDirty()           { _dirty = true; }

    // Menu
    void    setMenuIndex(uint8_t idx) { _menuIdx = idx; _dirty = true; }
    uint8_t menuIndex()          const { return _menuIdx; }

    // Generic list index (WiFi saved / WiFi scan / positions)
    void    setListIndex(uint8_t idx) { _listIdx = idx; _dirty = true; }
    uint8_t listIndex()          const { return _listIdx; }

    // Keyboard state
    void    setKbState(const char *pass, uint8_t passLen,
                       uint8_t row, uint8_t col, bool shift);

    // Time/value editor
    void    setEditValue(uint32_t ms, const char *title);

    // Clock set (time fields)
    void    setClockEditState(uint8_t field, int8_t h, uint8_t m,
                              bool use24h, int8_t tz, bool autoSync);  // v5: added autoSync

    // Date set  v5
    void    setDateEditState(uint8_t field, uint16_t year,
                             uint8_t month, uint8_t day);

    // WiFi detail  v5
    void    setWifiDetailState(const char *ssid, const char *ip,
                               const char *pass, uint8_t selItem,
                               bool passVisible);

    // WiFi saved list  v5
    void    setWifiSavedState(uint8_t selIdx, uint8_t total);

    // WiFi settings  v5
    void    setWifiSettingsState(uint8_t selItem, bool autoSync);

    // Position editor
    void    setPositionSlot(uint8_t slot)    { _posSlot    = slot;    _dirty = true; }
    void    setPositionEdit(bool editing)    { _posEditing = editing; _dirty = true; }

    // Limits screen (v7)
    void    setLimitsState(uint8_t selItem, uint16_t ud, uint16_t fb);
    // Slave status screen (v7)
    void    setSlaveStatusState(bool baseOk, bool assistOk,
                                uint16_t ud, uint16_t fb);

    // Popup
    void    showPopup(const char *msg, uint16_t durationMs = 1500);

    // Progress bar
    void    setProgress(uint8_t pct, bool visible);

private:
    Adafruit_SH1106G _oled{ OLED_W, OLED_H, &Wire, -1 };

    Screen   _screen   = SCREEN_HOME;
    bool     _dirty    = true;
    uint8_t  _menuIdx  = 0;
    uint8_t  _listIdx  = 0;

    // Keyboard
    static constexpr uint8_t PASS_MAXLEN = 64;
    char     _kbPass[PASS_MAXLEN + 1] = {};
    uint8_t  _kbPassLen = 0;
    uint8_t  _kbRow = 0, _kbCol = 0;
    bool     _kbShift = true;

    // Time editor
    uint32_t _editValue = 0;
    char     _editTitle[20] = {};

    // Clock editor
    uint8_t  _clkField  = 0;
    int8_t   _clkH      = 0;
    uint8_t  _clkM      = 0;
    bool     _clk24h    = true;
    int8_t   _clkTZ     = 2;
    bool     _clkAutoSync = true;

    // Date editor  v5
    uint8_t  _dtField   = 0;
    uint16_t _dtYear    = 2024;
    uint8_t  _dtMonth   = 1;
    uint8_t  _dtDay     = 1;

    // WiFi detail  v5
    char     _wdSSID[33]    = {};
    char     _wdIP[16]      = {};
    char     _wdPass[64]    = {};
    uint8_t  _wdSel         = 0;
    bool     _wdPassVisible = false;

    // WiFi saved list  v5
    uint8_t  _wsSel   = 0;
    uint8_t  _wsTotal = 0;

    // WiFi settings  v5
    uint8_t  _wstSel      = 0;
    bool     _wstAutoSync = true;

    // Position editor
    uint8_t  _posSlot    = 0;
    bool     _posEditing = false;

    // Limits screen (v7)
    uint8_t  _limSel  = 0;
    uint16_t _limUD   = 0;
    uint16_t _limFB   = 0;

    // Slave status (v7)
    bool     _slvBaseOk  = false;
    bool     _slvAssOk   = false;

    // Popup
    Popup    _popup = {};

    // Progress bar
    uint8_t  _progressPct  = 0;
    bool     _progressShow = false;

    // Draw helpers
    void _drawHome();
    void _drawNotifBar();
    void _drawMenu();
    void _drawWifiScan();
    void _drawWifiPass();
    void _drawWifiDetail();     // v5
    void _drawWifiSaved();      // v5
    void _drawWifiSettings();   // v5
    void _drawTimeEditor();
    void _drawClockSet();
    void _drawDateSet();        // v5
    void _drawPositions();
    void _drawLimits();        // v7
    void _drawSlaveStatus();   // v7
    void _drawPopup();
    void _drawProgressBar(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t pct);
    void _invertRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h);
};

extern DisplayManager Display;
