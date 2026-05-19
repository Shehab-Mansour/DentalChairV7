#pragma once

// ════════════════════════════════════════════════
//  WebServer.h  —  Embedded Web UI  v5.0 NEW
//
//  صفحة ويب على نفس الـ IP بتاع الـ ESP32
//  تُفتح من أي جهاز على نفس الشبكة
//  Port 80  →  http://<IP>/
//
//  Pages:
//   /           → Dashboard (status + quick actions)
//   /settings   → All settings (water/basin/hold/clock/date)
//   /wifi       → WiFi management
//   /positions  → Chair positions
//   /api/*      → REST API (JSON)
//
//  API endpoints:
//   GET  /api/status      → JSON status
//   GET  /api/settings    → JSON settings
//   POST /api/settings    → update settings
//   GET  /api/wifi        → scan + saved + current
//   POST /api/wifi/connect
//   POST /api/wifi/disconnect
//   POST /api/wifi/delete
//   GET  /api/positions   → all positions
//   POST /api/positions/save
//   POST /api/positions/delete
//   POST /api/action      → led/water/basin/stop
// ════════════════════════════════════════════════

#include "shard.h"
#include <WebServer.h>

class DentalWebServer {
public:
    void begin();
    void update();          // call every loop
    bool isRunning() const  { return _running; }

private:
    WebServer _server{ WEB_SERVER_PORT };
    bool      _running = false;

    // Page handlers
    void _handleRoot();
    void _handleSettings();
    void _handleWifi();
    void _handlePositions();

    // API handlers
    void _apiStatus();
    void _apiGetSettings();
    void _apiPostSettings();
    void _apiWifiScan();
    void _apiWifiConnect();
    void _apiWifiDisconnect();
    void _apiWifiDelete();
    void _apiGetPositions();
    void _apiSavePosition();
    void _apiDeletePosition();
    void _apiAction();

    void _notFound();
    void _sendCORS();

    // HTML helpers — بناء dynamic HTML بدون String كبيرة
    void _sendPageStart(const char *title, const char *activeTab);
    void _sendPageEnd();
    void _sendNav(const char *activeTab);
};

extern DentalWebServer WebUI;  // used in DentalChairV5.ino
