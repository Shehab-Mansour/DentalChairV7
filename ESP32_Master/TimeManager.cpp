// ════════════════════════════════════════════════
//  TimeManager.cpp  —  DS3231 + NTP  v5.0
// ════════════════════════════════════════════════

#include "TimeManager.h"
#include <WiFiUdp.h>
#include <NTPClient.h>

TimeManager TimeMgr;

char gTimeStr[10] = "--:--";
char gDateStr[12] = "--/--/----";

static WiFiUDP   _ntpUDP;
static NTPClient _ntpClient(_ntpUDP, "pool.ntp.org", 7200, 60000);

void TimeManager::begin() {
    _rtcOk = _rtc.begin();
    if (_rtcOk && _rtc.lostPower())
        _rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    if (!_rtcOk) Serial.println("[RTC] Not found");
}

void TimeManager::beginNTP() {
    _ntpClient.setTimeOffset(gSettings.tzOffsetHours * 3600L);
    _ntpClient.begin();
}

void TimeManager::update() {
    if (!_rtcOk) return;
    if (!elapsed(_lastUpdate, 1000)) return;
    DateTime now = _rtc.now();
    snprintf(gDateStr, sizeof(gDateStr), "%02d/%02d/%04d",
             now.day(), now.month(), now.year());
    if (gSettings.use24h) {
        snprintf(gTimeStr, sizeof(gTimeStr), "%02d:%02d",
                 now.hour(), now.minute());
    } else {
        uint8_t h = now.hour();
        bool pm = (h >= 12);
        h = h % 12; if (h == 0) h = 12;
        snprintf(gTimeStr, sizeof(gTimeStr), "%02d:%02d%s",
                 h, now.minute(), pm ? "PM" : "AM");
    }
}

bool TimeManager::syncNTP() {
    _ntpClient.setTimeOffset(gSettings.tzOffsetHours * 3600L);
    _ntpClient.forceUpdate();
    uint32_t epoch = _ntpClient.getEpochTime();
    if (epoch < 1700000000UL) {
        Serial.println("[NTP] Sync failed"); return false;
    }
    if (_rtcOk) _rtc.adjust(DateTime(epoch));
    notif.timeSynced = true;
    Serial.println("[NTP] Synced");
    return true;
}

DateTime TimeManager::getNow() {
    if (_rtcOk) return _rtc.now();
    return DateTime(2024, 1, 1, 0, 0, 0);
}

void TimeManager::setTime(uint8_t hour24, uint8_t minute) {
    if (!_rtcOk) return;
    DateTime now = _rtc.now();
    _rtc.adjust(DateTime(now.year(), now.month(), now.day(), hour24, minute, 0));
}

// v5: ضبط التاريخ بشكل منفصل عن الوقت
void TimeManager::setDate(uint16_t year, uint8_t month, uint8_t day) {
    if (!_rtcOk) return;
    DateTime now = _rtc.now();
    // تحقق من صحة القيم
    if (year < 2020 || year > 2099) return;
    if (month < 1 || month > 12) return;
    if (day < 1 || day > 31) return;
    _rtc.adjust(DateTime(year, month, day, now.hour(), now.minute(), now.second()));
    Serial.printf("[RTC] Date set: %04d/%02d/%02d\n", year, month, day);
}

void TimeManager::setTZ(int8_t offsetHours) {
    gSettings.tzOffsetHours = offsetHours;
    _ntpClient.setTimeOffset(offsetHours * 3600L);
}

void TimeManager::setFormat(bool use24h) {
    gSettings.use24h = use24h;
}

void TimeManager::setAutoSync(bool en) {
    gSettings.autoSyncTime = en;
}
