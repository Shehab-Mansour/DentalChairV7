#pragma once

// ════════════════════════════════════════════════
//  MenuController.h  v5.0
// ════════════════════════════════════════════════

#include "shard.h"
#include "Display.h"
#include "Keypad.h"
#include "SlaveComm.h"
#include "ChairControl.h"

class MenuController {
public:
    void begin();
    void handleKey(KeyState ks);

private:
    void _homeKey(KeyState ks);
    void _menuKey(KeyState ks);
    void _wifiScanKey(KeyState ks);
    void _wifiPassKey(KeyState ks);
    void _wifiDetailKey(KeyState ks);
    void _wifiSavedKey(KeyState ks);
    void _wifiSettingsKey(KeyState ks);
    void _waterTimeKey(KeyState ks);
    void _basinTimeKey(KeyState ks);
    void _suctionTimeKey(KeyState ks);   // v7
    void _holdTimeKey(KeyState ks);
    void _clockSetKey(KeyState ks);
    void _dateSetKey(KeyState ks);
    void _positionsKey(KeyState ks);
    void _limitsKey(KeyState ks);       // v7
    void _slaveStatusKey(KeyState ks);  // v7

    // Keyboard (WiFi pass / edit pass)
    static constexpr uint8_t PASS_MAXLEN = 64;
    char    _passBuffer[PASS_MAXLEN + 1] = {};
    uint8_t _passLen  = 0;
    uint8_t _kbRow    = 0;
    uint8_t _kbCol    = 0;
    bool    _kbShift  = true;
    bool    _kbEditMode = false;       // true = editing existing password
    char    _kbEditSSID[33] = {};      // ssid whose password we're editing

    void _kbBackspace();
    char _kbCurrentChar() const;
    bool _kbIsSymPage()   const { return _kbCol >= 10; }
    uint8_t _kbPageCol()  const { return _kbIsSymPage() ? _kbCol - 10 : _kbCol; }
    void _kbUpdateDisplay();
    void _kbReset(bool editMode = false);

    // Clock editor (5 fields)
    uint8_t _clkField   = 0;
    int8_t  _clkH       = 12;
    uint8_t _clkM       = 0;
    int8_t  _clkTZ      = 2;
    bool    _clkAutoSync = true;

    // Date editor
    uint8_t  _dtField  = 0;
    uint16_t _dtYear   = 2024;
    uint8_t  _dtMonth  = 1;
    uint8_t  _dtDay    = 1;

    // WiFi detail
    uint8_t _wdSel         = 0;
    bool    _wdPassVisible = false;

    // WiFi settings
    uint8_t _wstSel = 0;

    // WiFi saved
    uint8_t _savedSel = 0;

    // Position editor
    uint8_t _posSlot    = 0;
    bool    _posWaitKey = false;

    // Limits (v7)
    uint8_t _limSel = 0;
    static const char* LIM_NAMES[4];

    void _enterMenu(Screen s);
    void _back();
    void _submitWifiPassword();
    void _submitEditPassword();
    void _refreshWifiDetail();
};

extern MenuController Menu;
extern char kbGetChar(bool shift, bool symPage, uint8_t r, uint8_t c);
