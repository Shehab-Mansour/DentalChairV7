#pragma once

// ════════════════════════════════════════════════
//  WiFiManager.h  —  WiFi Manager  v5.0
//  v5 additions:
//  • disconnect()        — قطع الاتصال الحالي
//  • getPassword(ssid)   — جلب الباسورد المحفوظ
//  • updatePassword()    — تعديل الباسورد
//  • getLocalIP()        — عنوان IP
//  • getCurrentNetIdx()  — index الشبكة المتصل بها في الـ saved list
//  • deleteNetwork()     — حذف شبكة من المحفوظات
//  • getSavedCount()     — عدد الشبكات المحفوظة
//  • getSaved()          — قراءة saved network
// ════════════════════════════════════════════════

#include "shard.h"
class DentalWebServer;  // forward declare
extern DentalWebServer WebSrv;
#include <WiFi.h>

extern char    gWifiNetworks[10][33];
extern uint8_t gWifiCount;
extern uint8_t gWifiScanSel;

struct SavedNetwork {
    char ssid[33];
    char pass[64];
};

enum WiFiConnState : uint8_t {
    WCS_IDLE = 0,
    WCS_CONNECTING,
    WCS_CONNECTED,
    WCS_FAILED,
};

class WiFiMgr {
public:
    void          begin();
    void          update();

    void          startScan();
    void          updateScan();

    void          connectTo(uint8_t scanIdx, const char *pass);
    void          connectToSSID(const char *ssid, const char *pass);   // v5
    void          disconnect();                                         // v5
    WiFiConnState getConnectState() const { return _connState; }

    bool          isConnected()         const;
    const char*   currentSSID()         const;
    String        getLocalIP()          const;   // v5
    int8_t        getCurrentNetIdx()    const;   // v5: index in _saved[], -1 if none
    uint8_t       getSavedCount()       const { return _savedCount; }
    const SavedNetwork* getSaved(uint8_t i) const {  // v5
        return (i < _savedCount) ? &_saved[i] : nullptr;
    }
    bool          getPassword(const char *ssid, char *out, uint8_t outLen) const; // v5
    void          updatePassword(const char *ssid, const char *newPass);           // v5
    void          deleteNetwork(uint8_t savedIdx);                                 // v5

    void          saveNetwork(const char *ssid, const char *pass);
    void          loadNetworks();

private:
    SavedNetwork  _saved[20];
    uint8_t       _savedCount  = 0;
    bool          _scanning    = false;

    WiFiConnState _connState    = WCS_IDLE;
    uint32_t      _connStartMs  = 0;
    uint32_t      _connTimeout  = WIFI_CONNECT_TIMEOUT_MS;

    bool     _bootConnecting  = false;
    bool     _bootDone        = false;
    int8_t   _bootTryIdx      = -1;   // index of current boot attempt in _saved[]
};

extern WiFiMgr WifiManager;
