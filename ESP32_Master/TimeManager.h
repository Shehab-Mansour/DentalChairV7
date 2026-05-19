#pragma once

// ════════════════════════════════════════════════
//  TimeManager.h  —  DS3231 RTC + NTP  v5.0
//  v5 additions:
//  • setDate()  — ضبط التاريخ يدوياً
//  • getNow()   — يرجع DateTime للـ WebServer
//  • autoSync   — خيار sync تلقائي عند الاتصال
// ════════════════════════════════════════════════

#include "shard.h"
#include <RTClib.h>

extern char gTimeStr[10];
extern char gDateStr[12];

class TimeManager {
public:
    void     begin();
    void     beginNTP();
    void     update();
    bool     syncNTP();

    bool     isRTCReady() const { return _rtcOk; }
    DateTime getNow();

    void     setTime(uint8_t hour24, uint8_t minute);
    void     setDate(uint16_t year, uint8_t month, uint8_t day);  // v5 NEW
    void     setTZ(int8_t offsetHours);
    void     setFormat(bool use24h);
    void     setAutoSync(bool en);   // v5 NEW

private:
    RTC_DS3231 _rtc;
    bool       _rtcOk      = false;
    uint32_t   _lastUpdate = 0;
};

extern TimeManager TimeMgr;
