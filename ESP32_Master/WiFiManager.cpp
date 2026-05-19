// ════════════════════════════════════════════════
//  WiFiManager.cpp  —  WiFi Manager  v6.0
//
//  إصلاحات v6:
//  • Boot connect: يجرّب كل شبكة محفوظة بالترتيب (لو الأولى فشلت)
//  • لو الشبكة مش موجودة في النطاق → يعدي عليها بهدوء
//  • connectToSSID: يعمل scan سريع أول لو الشبكة مش موجودة
//  • الـ Scan يشتغل بالفعل async → لا blocking
// ════════════════════════════════════════════════

#include "WiFiManager.h"
#include "Display.h"
#include "SDManager.h"
#include "TimeManager.h"
// #include "DentalWebServer.h"  // avoided circular dep — WebUI.update() handles start

WiFiMgr WifiManager;

char    gWifiNetworks[10][33] = {};
uint8_t gWifiCount   = 0;
uint8_t gWifiScanSel = 0;

// ──────────────────────────────────────────────
void WiFiMgr::begin() {
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.persistent(false);
    loadNetworks();
    if (_savedCount > 0) {
        // ابدأ scan async → update() يكمّل
        WiFi.scanNetworks(true);
        _bootConnecting = true;
        _bootDone       = false;
        _bootTryIdx     = -1;   // لم نختر شبكة بعد
        gAppState       = APP_WIFI_CONN;
    }
}

// ──────────────────────────────────────────────
void WiFiMgr::update() {

    // ── Boot auto-connect ────────────────────
    if (_bootConnecting && !_bootDone) {
        int n = WiFi.scanComplete();
        if (n == WIFI_SCAN_RUNNING) return;   // scan مازال شغّال

        // بعد انتهاء الـ scan: نرتّب المحاولات تنازلياً بالـ RSSI
        if (_bootTryIdx == -1) {
            // بناء قائمة أولويات: أقوى إشارة أولاً
            int bestRSSI = -1000;
            _bootTryIdx = -2;   // -2 = لا يوجد شبكة متاحة
            for (int i = 0; i < n; i++) {
                const char *ssid = WiFi.SSID(i).c_str();
                for (uint8_t j = 0; j < _savedCount; j++) {
                    if (strcmp(_saved[j].ssid, ssid) == 0 && WiFi.RSSI(i) > bestRSSI) {
                        bestRSSI    = WiFi.RSSI(i);
                        _bootTryIdx = (int8_t)j;
                    }
                }
            }
            if (_bootTryIdx < 0) {
                // لا توجد شبكة محفوظة في النطاق الآن
                _bootDone = true; _bootConnecting = false;
                gAppState = APP_HOME;
                Serial.println("[WiFi] No saved network in range");
                return;
            }
            // ابدأ الاتصال بأفضل شبكة
            Serial.printf("[WiFi] Boot trying: %s\n", _saved[_bootTryIdx].ssid);
            WiFi.begin(_saved[_bootTryIdx].ssid, _saved[_bootTryIdx].pass);
            _connState    = WCS_CONNECTING;
            _connStartMs  = millis();
            _connTimeout  = WIFI_BOOT_TIMEOUT_MS;
        }
    }

    // ── Connecting state ────────────────────
    if (_connState == WCS_CONNECTING) {
        if (WiFi.status() == WL_CONNECTED) {
            _connState          = WCS_CONNECTED;
            notif.wifiConnected = true;
            _bootDone           = true;
            _bootConnecting     = false;
            gAppState           = APP_HOME;
            Serial.printf("[WiFi] Connected: %s  IP: %s\n",
                          WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
            TimeMgr.beginNTP();
            if (gSettings.autoSyncTime) {
                if (TimeMgr.syncNTP()) Display.showPopup("Time Synced!", 1500);
                else                   Display.showPopup("WiFi OK, NTP fail", 1800);
            } else {
                Display.showPopup("WiFi Connected!", 1500);
            }
            // WebUI يبدأ في update() تلقائياً لما يتوصل

        } else if (millis() - _connStartMs >= _connTimeout) {
            // فشل — لو كنا في boot نجرّب الشبكة التالية
            Serial.printf("[WiFi] Timeout for: %s\n",
                          _bootTryIdx >= 0 ? _saved[_bootTryIdx].ssid : "?");
            if (_bootConnecting && !_bootDone) {
                // نبحث عن شبكة أخرى محفوظة في نفس الـ scan results
                int n2 = WiFi.scanComplete();
                bool found = false;
                if (n2 > 0) {
                    int bestRSSI = -1000;
                    int8_t nextIdx = -1;
                    for (int i = 0; i < n2; i++) {
                        const char *ssid = WiFi.SSID(i).c_str();
                        for (uint8_t j = 0; j < _savedCount; j++) {
                            if ((int8_t)j == _bootTryIdx) continue;  // تجاهل المجربة
                            if (strcmp(_saved[j].ssid, ssid) == 0 && WiFi.RSSI(i) > bestRSSI) {
                                bestRSSI = WiFi.RSSI(i);
                                nextIdx  = (int8_t)j;
                            }
                        }
                    }
                    if (nextIdx >= 0) {
                        _bootTryIdx  = nextIdx;
                        Serial.printf("[WiFi] Trying next: %s\n", _saved[nextIdx].ssid);
                        WiFi.begin(_saved[nextIdx].ssid, _saved[nextIdx].pass);
                        _connStartMs = millis();
                        found = true;
                    }
                }
                if (!found) {
                    _bootDone = true; _bootConnecting = false;
                    _connState = WCS_FAILED; notif.wifiConnected = false;
                    gAppState  = APP_HOME;
                    Serial.println("[WiFi] All boot attempts failed");
                    Display.showPopup("WiFi Not Found", 1500);
                }
            } else {
                // manual connect timeout
                _connState = WCS_FAILED; notif.wifiConnected = false;
                gAppState  = APP_HOME;
                Display.showPopup("WiFi Failed!", 1500);
            }
        }
    }

    // ── Auto reconnect check ───────────────
    static uint32_t lastCheck = 0;
    if (elapsed(lastCheck, 5000)) {
        bool connected = (WiFi.status() == WL_CONNECTED);
        if (notif.wifiConnected != connected) {
            notif.wifiConnected = connected;
            Display.markDirty();
            // لو انقطع وكان شغّال → يحاول إعادة الاتصال تلقائياً (setAutoReconnect)
            if (!connected) Serial.println("[WiFi] Lost connection");
        }
    }
}

// ──────────────────────────────────────────────
void WiFiMgr::startScan() {
    gWifiCount = 0; gWifiScanSel = 0;
    WiFi.scanNetworks(true);
    _scanning = true;
    Display.markDirty();
}

void WiFiMgr::updateScan() {
    if (!_scanning) return;
    int n = WiFi.scanComplete();
    if (n == WIFI_SCAN_RUNNING) { Display.markDirty(); return; }
    _scanning = false;
    if (n <= 0) { gWifiCount = 0; Display.markDirty(); return; }
    gWifiCount = (uint8_t)min(n, 10);
    for (uint8_t i = 0; i < gWifiCount; i++) {
        strncpy(gWifiNetworks[i], WiFi.SSID(i).c_str(), 32);
        gWifiNetworks[i][32] = '\0';
    }
    Display.markDirty();
}

// ──────────────────────────────────────────────
void WiFiMgr::connectTo(uint8_t scanIdx, const char *pass) {
    if (scanIdx >= gWifiCount) return;
    connectToSSID(gWifiNetworks[scanIdx], pass);
}

void WiFiMgr::connectToSSID(const char *ssid, const char *pass) {
    saveNetwork(ssid, pass);   // احفظ أولاً
    WiFi.begin(ssid, pass);
    _connState    = WCS_CONNECTING;
    _connStartMs  = millis();
    _connTimeout  = WIFI_CONNECT_TIMEOUT_MS;
    _bootConnecting = false;   // manual connect — لا نجرّب شبكات أخرى تلقائياً
    gAppState     = APP_WIFI_CONN;
    Display.showPopup("Connecting...", 10000);
}

// ──────────────────────────────────────────────
void WiFiMgr::disconnect() {
    WiFi.disconnect(false);
    _connState          = WCS_IDLE;
    notif.wifiConnected = false;
    gAppState           = APP_HOME;
    Display.showPopup("Disconnected", 1200);
    Display.markDirty();
    Serial.println("[WiFi] Disconnected by user");
}

// ──────────────────────────────────────────────
bool WiFiMgr::isConnected()    const { return WiFi.status() == WL_CONNECTED; }
const char* WiFiMgr::currentSSID() const { return isConnected() ? WiFi.SSID().c_str() : ""; }
String WiFiMgr::getLocalIP()   const { return isConnected() ? WiFi.localIP().toString() : "Not connected"; }

int8_t WiFiMgr::getCurrentNetIdx() const {
    if (!isConnected()) return -1;
    const char *cur = WiFi.SSID().c_str();
    for (uint8_t i = 0; i < _savedCount; i++)
        if (strcmp(_saved[i].ssid, cur) == 0) return (int8_t)i;
    return -1;
}

bool WiFiMgr::getPassword(const char *ssid, char *out, uint8_t outLen) const {
    for (uint8_t i = 0; i < _savedCount; i++) {
        if (strcmp(_saved[i].ssid, ssid) == 0) {
            strncpy(out, _saved[i].pass, outLen - 1);
            out[outLen - 1] = '\0';
            return true;
        }
    }
    out[0] = '\0';
    return false;
}

void WiFiMgr::updatePassword(const char *ssid, const char *newPass) {
    for (uint8_t i = 0; i < _savedCount; i++) {
        if (strcmp(_saved[i].ssid, ssid) == 0) {
            strncpy(_saved[i].pass, newPass, 63);
            _saved[i].pass[63] = '\0';
            SDMgr.saveWifi(_saved, _savedCount);
            return;
        }
    }
}

void WiFiMgr::deleteNetwork(uint8_t idx) {
    if (idx >= _savedCount) return;
    for (uint8_t i = idx; i < _savedCount - 1; i++) _saved[i] = _saved[i + 1];
    _savedCount--;
    SDMgr.saveWifi(_saved, _savedCount);
}

void WiFiMgr::saveNetwork(const char *ssid, const char *pass) {
    for (uint8_t i = 0; i < _savedCount; i++) {
        if (strcmp(_saved[i].ssid, ssid) == 0) {
            strncpy(_saved[i].pass, pass, 63); _saved[i].pass[63] = '\0';
            SDMgr.saveWifi(_saved, _savedCount); return;
        }
    }
    if (_savedCount >= 20) return;
    strncpy(_saved[_savedCount].ssid, ssid, 32); _saved[_savedCount].ssid[32] = '\0';
    strncpy(_saved[_savedCount].pass, pass, 63); _saved[_savedCount].pass[63] = '\0';
    _savedCount++;
    SDMgr.saveWifi(_saved, _savedCount);
}

void WiFiMgr::loadNetworks() {
    _savedCount = SDMgr.loadWifi(_saved, 20);
    Serial.printf("[WiFi] Loaded %d saved networks\n", _savedCount);
}
