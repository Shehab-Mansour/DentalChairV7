// ════════════════════════════════════════════════
//  MenuController.cpp  v7.0
// ════════════════════════════════════════════════

#include "MenuController.h"
#include "ChairControl.h"
#include "OutputControl.h"
#include "WiFiManager.h"
#include "SDManager.h"
#include "TimeManager.h"
#include "SlaveComm.h"
#include "protocol.h"

MenuController Menu;

void MenuController::begin() {
    memset(_passBuffer, 0, sizeof(_passBuffer));
    _passLen = 0;
}

// ──────────────────────────────────────────────
//  DISPATCHER
// ──────────────────────────────────────────────
void MenuController::handleKey(KeyState ks) {
    if (ks.event == EVT_NONE) return;
    switch (Display.currentScreen()) {
        case SCREEN_HOME:          _homeKey(ks);         break;
        case SCREEN_MENU:          _menuKey(ks);         break;
        case SCREEN_WIFI_SCAN:     _wifiScanKey(ks);     break;
        case SCREEN_WIFI_PASS:     _wifiPassKey(ks);     break;
        case SCREEN_WIFI_DETAIL:   _wifiDetailKey(ks);   break;
        case SCREEN_WIFI_SAVED:    _wifiSavedKey(ks);    break;
        case SCREEN_WIFI_SETTINGS: _wifiSettingsKey(ks); break;
        case SCREEN_WATER_TIME:    _waterTimeKey(ks);    break;
        case SCREEN_BASIN_TIME:    _basinTimeKey(ks);    break;
        case SCREEN_SUCTION_TIME:  _suctionTimeKey(ks);  break;
        case SCREEN_HOLD_TIME:     _holdTimeKey(ks);     break;
        case SCREEN_CLOCK_SET:     _clockSetKey(ks);     break;
        case SCREEN_DATE_SET:      _dateSetKey(ks);        break;
        case SCREEN_POSITIONS:     _positionsKey(ks);      break;
        case SCREEN_LIMITS:        _limitsKey(ks);         break;
        case SCREEN_SLAVE_STATUS:  _slaveStatusKey(ks);    break;
        default: break;
    }
}

// ══════════════════════════════════════════════
//  HOME
// ══════════════════════════════════════════════
void MenuController::_homeKey(KeyState ks) {
    char key = ks.key;
    if (ks.event == EVT_LONG_PRESS || ks.event == EVT_HOLD) {
        switch (key) {
            case '2': Chair.moveUp();      break;
            case '8': Chair.moveDown();    break;
            case '4': Chair.moveForward(); break;
            case '6': Chair.moveBack();    break;
            default: break;
        }
        gAppState = APP_MOVING;
        return;
    }
    if (ks.event == EVT_RELEASE) { Chair.stopAll(); gAppState = APP_HOME; return; }
    if (ks.event != EVT_SHORT_PRESS) return;
    switch (key) {
        case 'A': Output.ledToggle();               Output.beepAssist(1200, 60); break;
        case 'B': Output.waterTimed();              Output.beepAssist(1000, 60); break;
        case 'C': Output.basinTimed();              Output.beepAssist(900,  60); break;
        case 'D': Output.suctionTimed();            Output.beepAssist(1100, 60); break;  // v7 suction
        case '#': Output.allOff(); Chair.stopAll(); Output.beepAssist(400,  200); break;
        case '*': Display.setScreen(SCREEN_MENU);   break;
        default:
            if (key >= '1' && key <= '9') {
                uint8_t slot = Chair.slotForHotkey(key - '0');
                if (slot > 0) { Chair.goToPosition(slot); Output.beepAssist(1100, 80); }
                else Display.showPopup("No hotkey set", 1000);
            }
            break;
    }
}

// ══════════════════════════════════════════════
//  MAIN MENU (7 items)
// ══════════════════════════════════════════════
void MenuController::_menuKey(KeyState ks) {
    if (ks.event != EVT_SHORT_PRESS && ks.event != EVT_HOLD) return;
    uint8_t idx = Display.menuIndex();
    switch (ks.key) {
        case '8': if (idx < 9) Display.setMenuIndex(idx + 1); Output.beepAssist(800, 20); break;
        case '2': if (idx > 0) Display.setMenuIndex(idx - 1); Output.beepAssist(800, 20); break;
        case '0':
            if (ks.event != EVT_SHORT_PRESS) break;
            switch (idx) {
                case 0: _enterMenu(SCREEN_WIFI_SETTINGS); break;
                case 1: _enterMenu(SCREEN_WATER_TIME);    break;
                case 2: _enterMenu(SCREEN_BASIN_TIME);    break;
                case 3: _enterMenu(SCREEN_SUCTION_TIME);  break;
                case 4: _enterMenu(SCREEN_HOLD_TIME);     break;
                case 5: _enterMenu(SCREEN_CLOCK_SET);     break;
                case 6: _enterMenu(SCREEN_DATE_SET);      break;
                case 7: _enterMenu(SCREEN_POSITIONS);     break;
                case 8: _enterMenu(SCREEN_LIMITS);        break;
                case 9: _enterMenu(SCREEN_SLAVE_STATUS);  break;
            }
            break;
        case '*': if (ks.event == EVT_SHORT_PRESS) Display.setScreen(SCREEN_HOME); break;
        default: break;
    }
}

// ══════════════════════════════════════════════
//  WIFI SCAN
// ══════════════════════════════════════════════
void MenuController::_wifiScanKey(KeyState ks) {
    if (ks.event != EVT_SHORT_PRESS && ks.event != EVT_HOLD) return;
    switch (ks.key) {
        case '8':
            if (Display.listIndex() < gWifiCount - 1) Display.setListIndex(Display.listIndex() + 1);
            break;
        case '2':
            if (Display.listIndex() > 0) Display.setListIndex(Display.listIndex() - 1);
            break;
        case '0':
            if (ks.event != EVT_SHORT_PRESS) break;
            // If this is the current network → go to detail
            if (gWifiCount > 0 &&
                strcmp(gWifiNetworks[Display.listIndex()], WifiManager.currentSSID()) == 0) {
                _enterMenu(SCREEN_WIFI_DETAIL);
            } else {
                // Connect to selected
                gWifiScanSel = Display.listIndex();
                _kbReset(false);
                Display.setScreen(SCREEN_WIFI_PASS);
            }
            break;
        case 'A':
            // Detail for currently connected network
            if (ks.event == EVT_SHORT_PRESS && WifiManager.isConnected()) {
                _enterMenu(SCREEN_WIFI_DETAIL);
            }
            break;
        case '*': if (ks.event == EVT_SHORT_PRESS) _back(); break;
        default: break;
    }
}

// ══════════════════════════════════════════════
//  WIFI PASSWORD  (new + edit modes share same keyboard)
// ══════════════════════════════════════════════
char MenuController::_kbCurrentChar() const {
    if (_kbRow >= 3) return '\0';
    return kbGetChar(_kbShift, _kbIsSymPage(), _kbRow, _kbPageCol());
}
void MenuController::_kbUpdateDisplay() {
    Display.setKbState(_passBuffer, _passLen, _kbRow, _kbCol, _kbShift);
}
void MenuController::_kbBackspace() {
    if (_passLen > 0) { _passLen--; _passBuffer[_passLen] = '\0'; }
}
void MenuController::_kbReset(bool editMode) {
    memset(_passBuffer, 0, sizeof(_passBuffer));
    _passLen = 0; _kbRow = 0; _kbCol = 0; _kbShift = true;
    _kbEditMode = editMode;
    _kbUpdateDisplay();
}

void MenuController::_submitWifiPassword() {
    WifiManager.connectTo(gWifiScanSel, _passBuffer);
    Display.setScreen(SCREEN_HOME);
}
void MenuController::_submitEditPassword() {
    WifiManager.updatePassword(_kbEditSSID, _passBuffer);
    Output.beepAssist(1200, 100);
    Display.showPopup("Password Updated!", 1200);
    _enterMenu(SCREEN_WIFI_DETAIL);
}

void MenuController::_wifiPassKey(KeyState ks) {
    if (ks.event != EVT_SHORT_PRESS && ks.event != EVT_HOLD) return;
    bool symPage = _kbIsSymPage();
    uint8_t col  = _kbPageCol();
    switch (ks.key) {
        case '2': _kbRow = (_kbRow > 0) ? _kbRow - 1 : 3; break;
        case '8': _kbRow = (_kbRow < 3) ? _kbRow + 1 : 0; break;
        case '4': col = (col > 0) ? col - 1 : 9; _kbCol = symPage ? col + 10 : col; break;
        case '6': col = (col < 9) ? col + 1 : 0; _kbCol = symPage ? col + 10 : col; break;
        case '0':
            if (_kbRow < 3) {
                char ch = _kbCurrentChar();
                if (ch && ch != ' ' && _passLen < PASS_MAXLEN) {
                    _passBuffer[_passLen++] = ch; _passBuffer[_passLen] = '\0';
                }
            } else {
                switch (col) {
                    // 0=SPC  1=abc  2=ABC  3=123  4=DEL  5=OK
                    case 0: if (_passLen < PASS_MAXLEN) { _passBuffer[_passLen++] = ' '; _passBuffer[_passLen] = '\0'; } break;
                    case 1: _kbShift = false; _kbCol = _kbIsSymPage() ? _kbPageCol() : _kbPageCol(); break; // abc
                    case 2: _kbShift = true;  _kbCol = _kbIsSymPage() ? _kbPageCol() : _kbPageCol(); break; // ABC
                    case 3: _kbCol = _kbIsSymPage() ? _kbPageCol() : _kbPageCol() + 10; break; // 123
                    case 4: _kbBackspace(); break;
                    case 5:
                        if (_kbEditMode) _submitEditPassword();
                        else             _submitWifiPassword();
                        return;
                }
            }
            break;
        case 'A': _kbShift = !_kbShift;                               break;
        case 'B': _kbBackspace();                                      break;
        case 'C': _kbCol = symPage ? col : col + 10;                  break;
        case '#':
            if (_kbEditMode) _submitEditPassword();
            else             _submitWifiPassword();
            return;
        case '*': if (ks.event == EVT_SHORT_PRESS) { _back(); return; } break;
        default: break;
    }
    _kbUpdateDisplay();
}

// ══════════════════════════════════════════════
//  WIFI DETAIL  (v5)
//  sel: 0=pass row  1=Show/Hide  2=Edit Pass  3=Disconnect
// ══════════════════════════════════════════════
void MenuController::_refreshWifiDetail() {
    char pass[65] = {};
    WifiManager.getPassword(WifiManager.currentSSID(), pass, 65);
    String ip = WifiManager.getLocalIP();
    Display.setWifiDetailState(
        WifiManager.currentSSID(),
        ip.c_str(),
        pass, _wdSel, _wdPassVisible
    );
}

void MenuController::_wifiDetailKey(KeyState ks) {
    if (ks.event != EVT_SHORT_PRESS && ks.event != EVT_HOLD) return;
    switch (ks.key) {
        case '2': if (_wdSel > 0) { _wdSel--; _refreshWifiDetail(); } break;
        case '8': if (_wdSel < 3) { _wdSel++; _refreshWifiDetail(); } break;
        case '0':
            if (ks.event != EVT_SHORT_PRESS) break;
            switch (_wdSel) {
                case 0: // pass row — toggle show/hide
                case 1: _wdPassVisible = !_wdPassVisible; _refreshWifiDetail(); break;
                case 2: // Edit password → keyboard in edit mode
                    strncpy(_kbEditSSID, WifiManager.currentSSID(), 32);
                    _kbEditSSID[32] = '\0';
                    _kbReset(true);
                    Display.setScreen(SCREEN_WIFI_PASS);
                    break;
                case 3: // Disconnect
                    WifiManager.disconnect();
                    Display.setScreen(SCREEN_HOME);
                    break;
            }
            break;
        case '*': if (ks.event == EVT_SHORT_PRESS) _back(); break;
        default: break;
    }
}

// ══════════════════════════════════════════════
//  WIFI SAVED LIST  (v5)
// ══════════════════════════════════════════════
void MenuController::_wifiSavedKey(KeyState ks) {
    if (ks.event != EVT_SHORT_PRESS && ks.event != EVT_HOLD) return;
    uint8_t total = WifiManager.getSavedCount();
    switch (ks.key) {
        case '8':
            if (_savedSel < total - 1) { _savedSel++; Display.setWifiSavedState(_savedSel, total); }
            break;
        case '2':
            if (_savedSel > 0) { _savedSel--; Display.setWifiSavedState(_savedSel, total); }
            break;
        case '0':
            if (ks.event != EVT_SHORT_PRESS) break;
            if (total > 0) {
                const SavedNetwork *net = WifiManager.getSaved(_savedSel);
                if (!net) break;
                // Confirm delete
                char msg[22]; snprintf(msg, 22, "Deleted %.10s", net->ssid);
                WifiManager.deleteNetwork(_savedSel);
                if (_savedSel >= WifiManager.getSavedCount() && _savedSel > 0) _savedSel--;
                Display.showPopup(msg, 1200);
                Display.setWifiSavedState(_savedSel, WifiManager.getSavedCount());
            }
            break;
        case '*': if (ks.event == EVT_SHORT_PRESS) _back(); break;
        default: break;
    }
}

// ══════════════════════════════════════════════
//  WIFI SETTINGS  (v5)
//  items: 0=autoSync  1=Scan  2=Saved  3=CurrentConn/Detail
// ══════════════════════════════════════════════
void MenuController::_wifiSettingsKey(KeyState ks) {
    if (ks.event != EVT_SHORT_PRESS && ks.event != EVT_HOLD) return;
    switch (ks.key) {
        case '8': if (_wstSel < 3) { _wstSel++; Display.setWifiSettingsState(_wstSel, gSettings.autoSyncTime); } break;
        case '2': if (_wstSel > 0) { _wstSel--; Display.setWifiSettingsState(_wstSel, gSettings.autoSyncTime); } break;
        case '0':
            if (ks.event != EVT_SHORT_PRESS) break;
            switch (_wstSel) {
                case 0: // Toggle autoSync
                    gSettings.autoSyncTime = !gSettings.autoSyncTime;
                    TimeMgr.setAutoSync(gSettings.autoSyncTime);
                    SDMgr.saveSettings(gSettings);
                    Output.beepAssist(1100, 80);
                    Display.setWifiSettingsState(_wstSel, gSettings.autoSyncTime);
                    Display.showPopup(gSettings.autoSyncTime ? "AutoSync: ON" : "AutoSync: OFF", 1000);
                    break;
                case 1: // Scan Networks
                    WifiManager.startScan();
                    Display.setListIndex(0);
                    Display.setScreen(SCREEN_WIFI_SCAN);
                    break;
                case 2: // Saved Networks
                    _savedSel = 0;
                    Display.setWifiSavedState(0, WifiManager.getSavedCount());
                    Display.setScreen(SCREEN_WIFI_SAVED);
                    break;
                case 3: // Current connection detail
                    if (WifiManager.isConnected()) {
                        _wdSel = 0; _wdPassVisible = false;
                        _refreshWifiDetail();
                        Display.setScreen(SCREEN_WIFI_DETAIL);
                    } else {
                        Display.showPopup("Not connected", 1000);
                    }
                    break;
            }
            break;
        case '*': if (ks.event == EVT_SHORT_PRESS) _back(); break;
        default: break;
    }
}

// ══════════════════════════════════════════════
//  WATER TIME
// ══════════════════════════════════════════════
void MenuController::_waterTimeKey(KeyState ks) {
    if (ks.event != EVT_SHORT_PRESS && ks.event != EVT_HOLD) return;
    switch (ks.key) {
        case '2': gSettings.waterTimeMs += 1000; Display.setEditValue(gSettings.waterTimeMs, "Water Time"); break;
        case '8': if (gSettings.waterTimeMs > 1000) gSettings.waterTimeMs -= 1000; Display.setEditValue(gSettings.waterTimeMs, "Water Time"); break;
        case '0':
            if (ks.event != EVT_SHORT_PRESS) break;
            SDMgr.saveSettings(gSettings); Output.beepAssist(1200, 100);
            Display.showPopup("Saved!", 800); _back(); break;
        case '*': if (ks.event == EVT_SHORT_PRESS) _back(); break;
    }
}

// ══════════════════════════════════════════════
//  BASIN TIME
// ══════════════════════════════════════════════
void MenuController::_basinTimeKey(KeyState ks) {
    if (ks.event != EVT_SHORT_PRESS && ks.event != EVT_HOLD) return;
    switch (ks.key) {
        case '2': gSettings.basinTimeMs += 1000; Display.setEditValue(gSettings.basinTimeMs, "Basin Time"); break;
        case '8': if (gSettings.basinTimeMs > 1000) gSettings.basinTimeMs -= 1000; Display.setEditValue(gSettings.basinTimeMs, "Basin Time"); break;
        case '0':
            if (ks.event != EVT_SHORT_PRESS) break;
            SDMgr.saveSettings(gSettings); Output.beepAssist(1200, 100);
            Display.showPopup("Saved!", 800); _back(); break;
        case '*': if (ks.event == EVT_SHORT_PRESS) _back(); break;
    }
}

// ══════════════════════════════════════════════
//  SUCTION TIME  (v7)
// ══════════════════════════════════════════════
void MenuController::_suctionTimeKey(KeyState ks) {
    if (ks.event != EVT_SHORT_PRESS && ks.event != EVT_HOLD) return;
    switch (ks.key) {
        case '2': gSettings.suctionTimeMs += 1000; Display.setEditValue(gSettings.suctionTimeMs, "Suction Time"); break;
        case '8': if (gSettings.suctionTimeMs > 1000) gSettings.suctionTimeMs -= 1000; Display.setEditValue(gSettings.suctionTimeMs, "Suction Time"); break;
        case '0':
            if (ks.event != EVT_SHORT_PRESS) break;
            SDMgr.saveSettings(gSettings); Output.beepAssist(1200, 100);
            Display.showPopup("Saved!", 800); _back(); break;
        case '*': if (ks.event == EVT_SHORT_PRESS) _back(); break;
    }
}

// ══════════════════════════════════════════════
//  HOLD THRESHOLD
// ══════════════════════════════════════════════
void MenuController::_holdTimeKey(KeyState ks) {
    if (ks.event != EVT_SHORT_PRESS && ks.event != EVT_HOLD) return;
    switch (ks.key) {
        case '2': gSettings.holdThresholdMs = min((uint16_t)2000, (uint16_t)(gSettings.holdThresholdMs + 50)); Display.setEditValue(gSettings.holdThresholdMs, "Hold Threshold"); break;
        case '8': if (gSettings.holdThresholdMs > 50) gSettings.holdThresholdMs -= 50; Display.setEditValue(gSettings.holdThresholdMs, "Hold Threshold"); break;
        case '0':
            if (ks.event != EVT_SHORT_PRESS) break;
            SDMgr.saveSettings(gSettings); Output.beepAssist(1200, 100);
            Display.showPopup("Saved!", 800); _back(); break;
        case '*': if (ks.event == EVT_SHORT_PRESS) _back(); break;
    }
}

// ══════════════════════════════════════════════
//  CLOCK SETUP  (5 fields: H / M / TZ / Format / AutoSync)
// ══════════════════════════════════════════════
void MenuController::_clockSetKey(KeyState ks) {
    if (ks.event != EVT_SHORT_PRESS && ks.event != EVT_HOLD) return;
    switch (ks.key) {
        case '6': _clkField = (_clkField + 1) % 5; break;
        case '4': _clkField = (_clkField + 4) % 5; break;
        case '2':
            switch (_clkField) {
                case 0: _clkH = (_clkH + 1) % 24;                          break;
                case 1: _clkM = (_clkM + 1) % 60;                          break;
                case 2: if (_clkTZ < 14) _clkTZ++;                         break;
                case 3: gSettings.use24h = !gSettings.use24h;               break;
                case 4: _clkAutoSync = !_clkAutoSync;                       break;
            }
            break;
        case '8':
            switch (_clkField) {
                case 0: _clkH = (_clkH + 23) % 24;                         break;
                case 1: _clkM = (_clkM + 59) % 60;                         break;
                case 2: if (_clkTZ > -12) _clkTZ--;                        break;
                case 3: gSettings.use24h = !gSettings.use24h;               break;
                case 4: _clkAutoSync = !_clkAutoSync;                       break;
            }
            break;
        case '0':
            if (ks.event != EVT_SHORT_PRESS) break;
            TimeMgr.setTime(_clkH, _clkM);
            TimeMgr.setTZ(_clkTZ);
            TimeMgr.setFormat(gSettings.use24h);
            gSettings.autoSyncTime = _clkAutoSync;
            TimeMgr.setAutoSync(_clkAutoSync);
            SDMgr.saveSettings(gSettings);
            Output.beepAssist(1200, 100);
            Display.showPopup("Clock Saved!", 1000);
            _back(); return;
        case '*': if (ks.event == EVT_SHORT_PRESS) { _back(); return; } break;
        default: break;
    }
    Display.setClockEditState(_clkField, _clkH, _clkM, gSettings.use24h, _clkTZ, _clkAutoSync);
}

// ══════════════════════════════════════════════
//  DATE SETUP  (v5 NEW)
//  fields: 0=day  1=month  2=year
//  6=next field  4=prev  2=+1  8=-1  0=save  *=back
// ══════════════════════════════════════════════
void MenuController::_dateSetKey(KeyState ks) {
    if (ks.event != EVT_SHORT_PRESS && ks.event != EVT_HOLD) return;
    static const uint8_t daysInMonth[13] = {0,31,29,31,30,31,30,31,31,30,31,30,31};
    switch (ks.key) {
        case '6': _dtField = (_dtField + 1) % 3; break;
        case '4': _dtField = (_dtField + 2) % 3; break;
        case '2':
            switch (_dtField) {
                case 0: _dtDay   = (_dtDay   < daysInMonth[_dtMonth]) ? _dtDay + 1 : 1; break;
                case 1: _dtMonth = (_dtMonth < 12) ? _dtMonth + 1 : 1; if (_dtDay > daysInMonth[_dtMonth]) _dtDay = daysInMonth[_dtMonth]; break;
                case 2: if (_dtYear < 2099) _dtYear++; break;
            }
            break;
        case '8':
            switch (_dtField) {
                case 0: _dtDay   = (_dtDay   > 1) ? _dtDay - 1 : daysInMonth[_dtMonth]; break;
                case 1: _dtMonth = (_dtMonth > 1) ? _dtMonth - 1 : 12; if (_dtDay > daysInMonth[_dtMonth]) _dtDay = daysInMonth[_dtMonth]; break;
                case 2: if (_dtYear > 2020) _dtYear--; break;
            }
            break;
        case '0':
            if (ks.event != EVT_SHORT_PRESS) break;
            TimeMgr.setDate(_dtYear, _dtMonth, _dtDay);
            Output.beepAssist(1200, 100);
            Display.showPopup("Date Saved!", 1000);
            _back(); return;
        case '*': if (ks.event == EVT_SHORT_PRESS) { _back(); return; } break;
        default: break;
    }
    Display.setDateEditState(_dtField, _dtYear, _dtMonth, _dtDay);
}

// ══════════════════════════════════════════════
//  CHAIR POSITIONS
// ══════════════════════════════════════════════
void MenuController::_positionsKey(KeyState ks) {
    if (ks.event != EVT_SHORT_PRESS && ks.event != EVT_HOLD) return;
    if (_posWaitKey) {
        if (ks.event != EVT_SHORT_PRESS) return;
        if (ks.key >= '1' && ks.key <= '9') {
            Chair.setPositionHotkey(_posSlot + 1, ks.key - '0');
            Output.beepAssist(1300, 150); Display.showPopup("Hotkey Set!", 800);
        } else if (ks.key == '0') {
            Chair.setPositionHotkey(_posSlot + 1, 0);
            Display.showPopup("Hotkey Removed", 800);
        }
        _posWaitKey = false; Display.setPositionEdit(false); Display.markDirty();
        return;
    }
    switch (ks.key) {
        case '8': if (_posSlot < MAX_POSITIONS-1) { _posSlot++; Display.setPositionSlot(_posSlot); } break;
        case '2': if (_posSlot > 0) { _posSlot--; Display.setPositionSlot(_posSlot); } break;
        case '0':
            if (ks.event != EVT_SHORT_PRESS) break;
            Chair.saveCurrentPosition(_posSlot + 1);
            Output.beepAssist(1300, 150); Display.showPopup("Position Saved!", 800); Display.markDirty();
            break;
        case 'A':
            if (ks.event == EVT_SHORT_PRESS && gPositions[_posSlot].saved) {
                _posWaitKey = true; Display.setPositionEdit(true);
            }
            break;
        case '#':
            if (ks.event == EVT_SHORT_PRESS) {
                Chair.deletePosition(_posSlot + 1);
                Output.beepAssist(400, 200); Display.showPopup("Deleted!", 800); Display.markDirty();
            }
            break;
        case '*': if (ks.event == EVT_SHORT_PRESS) _back(); break;
        default:
            if (ks.event == EVT_SHORT_PRESS && ks.key >= '1' && ks.key <= '9') {
                uint8_t slot = Chair.slotForHotkey(ks.key - '0');
                if (slot > 0) { Chair.goToPosition(slot); Output.beepAssist(1100, 80); Display.setScreen(SCREEN_HOME); }
            }
            break;
    }
}

// ──────────────────────────────────────────────
//  Helpers
// ──────────────────────────────────────────────
void MenuController::_enterMenu(Screen s) {
    gAppState = APP_SETTINGS;
    switch (s) {
        case SCREEN_WIFI_SETTINGS:
            _wstSel = 0;
            Display.setWifiSettingsState(0, gSettings.autoSyncTime);
            break;
        case SCREEN_WIFI_SCAN:
            WifiManager.startScan();
            Display.setListIndex(0);
            break;
        case SCREEN_WIFI_DETAIL:
            _wdSel = 0; _wdPassVisible = false;
            _refreshWifiDetail();
            break;
        case SCREEN_WIFI_SAVED:
            _savedSel = 0;
            Display.setWifiSavedState(0, WifiManager.getSavedCount());
            break;
        case SCREEN_WATER_TIME:
            Display.setEditValue(gSettings.waterTimeMs, "Water Time"); break;
        case SCREEN_BASIN_TIME:
            Display.setEditValue(gSettings.basinTimeMs, "Basin Time"); break;
        case SCREEN_SUCTION_TIME:
            Display.setEditValue(gSettings.suctionTimeMs, "Suction Time"); break;
        case SCREEN_HOLD_TIME:
            Display.setEditValue(gSettings.holdThresholdMs, "Hold Threshold"); break;
        case SCREEN_CLOCK_SET:
            _clkField = 0; _clkTZ = gSettings.tzOffsetHours;
            _clkAutoSync = gSettings.autoSyncTime;
            {
                DateTime now = TimeMgr.getNow();
                _clkH = now.hour(); _clkM = now.minute();
            }
            Display.setClockEditState(_clkField, _clkH, _clkM, gSettings.use24h, _clkTZ, _clkAutoSync);
            break;
        case SCREEN_DATE_SET:
            _dtField = 0;
            {
                DateTime now = TimeMgr.getNow();
                _dtYear = now.year(); _dtMonth = now.month(); _dtDay = now.day();
            }
            Display.setDateEditState(_dtField, _dtYear, _dtMonth, _dtDay);
            break;
        case SCREEN_POSITIONS:
            _posSlot = 0; _posWaitKey = false;
            Display.setPositionSlot(0); Display.setPositionEdit(false);
            break;
        case SCREEN_LIMITS:
            _limSel = 0;
            Display.setLimitsState(0, Chair.readUpDown(), Chair.readFwdBack());
            gAppState = APP_LIMITS;
            break;
        case SCREEN_SLAVE_STATUS:
            Display.setSlaveStatusState(Comm.isBaseOnline(), Comm.isAssistOnline(),
                                         Chair.readUpDown(), Chair.readFwdBack());
            break;
        default: break;
    }
    Display.setScreen(s);
}

void MenuController::_back() {
    gAppState = APP_HOME;
    switch (Display.currentScreen()) {
        case SCREEN_WIFI_PASS:
            Display.setScreen(_kbEditMode ? SCREEN_WIFI_DETAIL : SCREEN_WIFI_SCAN);
            return;
        case SCREEN_WIFI_DETAIL:
        case SCREEN_WIFI_SAVED:
        case SCREEN_WIFI_SCAN:
            Display.setScreen(SCREEN_WIFI_SETTINGS);
            return;
        case SCREEN_WIFI_SETTINGS:
        case SCREEN_MENU:
            Display.setScreen(SCREEN_HOME);
            return;
        default:
            Display.setScreen(SCREEN_MENU);
            return;
    }
}

// ══════════════════════════════════════════════
//  LIM_NAMES table
// ══════════════════════════════════════════════
const char* MenuController::LIM_NAMES[4] = {
    CMD_SET_LIM_UD_MAX,
    CMD_SET_LIM_UD_MIN,
    CMD_SET_LIM_FB_MAX,
    CMD_SET_LIM_FB_MIN,
};

// ══════════════════════════════════════════════
//  LIMITS KEY HANDLER  (v7)
//  Items: 0=UD_MAX 1=UD_MIN 2=FB_MAX 3=FB_MIN 4=ClearAll
//  2/8: navigate  0: set current pos as limit  *: back
// ══════════════════════════════════════════════
void MenuController::_limitsKey(KeyState ks) {
    if (ks.event != EVT_SHORT_PRESS) return;

    uint16_t ud = Chair.readUpDown();
    uint16_t fb = Chair.readFwdBack();

    switch (ks.key) {
        case '8':
            if (_limSel < 4) _limSel++;
            Display.setLimitsState(_limSel, ud, fb);
            break;
        case '2':
            if (_limSel > 0) _limSel--;
            Display.setLimitsState(_limSel, ud, fb);
            break;
        case '0':
            if (_limSel < 4) {
                // حفظ القيمة الحالية كـ limit
                Chair.captureLimit(LIM_NAMES[_limSel]);
                Output.beepBase(120);
                char msg[22];
                snprintf(msg, sizeof(msg), "Limit Set: %d", (_limSel < 2) ? ud : fb);
                Display.showPopup(msg, 1200);
            } else {
                // Clear All
                ChairLimits &L = gSettings.limits;
                L.udMaxSet = L.udMinSet = L.fbMaxSet = L.fbMinSet = false;
                L.udMax = L.udMin = L.fbMax = L.fbMin = 0;
                SDMgr.saveSettings(gSettings);
                Output.beepBase(200);
                Display.showPopup("All Limits Cleared", 1200);
            }
            Display.setLimitsState(_limSel, ud, fb);
            break;
        case '*':
            gAppState = APP_HOME;
            _back();
            break;
        default: break;
    }
}

// ══════════════════════════════════════════════
//  SLAVE STATUS KEY HANDLER  (v7)
//  0: refresh  *: back
// ══════════════════════════════════════════════
void MenuController::_slaveStatusKey(KeyState ks) {
    if (ks.event != EVT_SHORT_PRESS) return;
    switch (ks.key) {
        case '0':
            Display.setSlaveStatusState(Comm.isBaseOnline(), Comm.isAssistOnline(),
                                         Chair.readUpDown(), Chair.readFwdBack());
            break;
        case '*':
            _back();
            break;
        default: break;
    }
}
