#pragma once

// ════════════════════════════════════════════════
//  SDManager.h  —  SD Card persistence  v4.0
//
//  Files:
//    /wifi.csv       →  ssid,password
//    /settings.txt   →  key=value
//    /positions.txt  →  slot,updown,fwdback,hotkey,label
//
//  v4.0 improvements:
//  • char[] بدل String — لا heap fragmentation
//  • atomic write: يكتب لـ .tmp ثم يعمل rename
//  • تحقق من size قبل القراءة
// ════════════════════════════════════════════════

#include "shard.h"
#include <SD.h>
#include <SPI.h>

struct SavedNetwork;  // forward declare من WiFiManager.h

class SDManager {
public:
    bool    begin();
    bool    isReady() const { return _ready; }

    void    saveWifi(const SavedNetwork *nets, uint8_t count);
    uint8_t loadWifi(SavedNetwork *nets, uint8_t maxCount);

    void    saveSettings(const Settings &s);
    bool    loadSettings(Settings &s);

    void    savePositions(const ChairPosition *pos, uint8_t count);
    bool    loadPositions(ChairPosition *pos, uint8_t count);

private:
    bool _ready = false;

    // Atomic write: اكتب لـ .tmp ثم rename
    bool _atomicWrite(const char *path, const char *tmpPath,
                      void (*writer)(File &, const void *), const void *data);
};

extern SDManager SDMgr;
