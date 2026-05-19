#pragma once

// ════════════════════════════════════════════════
//  OutputControl.h  v7.0
//  LED / Water / Basin / Suction
//  يرسل للـ Assistant عبر SlaveComm
// ════════════════════════════════════════════════

#include "shard.h"

class OutputControl {
public:
    void begin();
    void update();

    void ledOn();
    void ledOff();
    void ledToggle();

    void waterOn();
    void waterOff();
    void waterTimed();

    void basinOn();
    void basinOff();
    void basinTimed();

    void suctionOn();      // v7
    void suctionOff();     // v7
    void suctionTimed();   // v7

    void beepAssist(uint16_t freq = 1000, uint16_t durationMs = 80);
    void beepBase(uint16_t freq = 1000, uint16_t durationMs = 80);
    // void beepBase(uint16_t durationMs = 80);
    // void beepAssist(uint16_t durationMs = 80);
    void allOff();

    bool isLedOn()     const { return _ledState; }
    bool isWaterOn()   const { return notif.waterOn; }
    bool isBasinOn()   const { return notif.basinOn; }
    bool isSuctionOn() const { return notif.suctionOn; }

private:
    bool     _ledState    = false;
    bool     _waterManual = false;
    bool     _basinManual = false;
    bool     _suctManual  = false;
    uint32_t _waterEnd    = 0;
    uint32_t _basinEnd    = 0;
    uint32_t _suctEnd     = 0;

    void _sendAssist(const char *cmd);
};

extern OutputControl Output;
