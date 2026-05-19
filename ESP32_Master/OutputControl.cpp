// ════════════════════════════════════════════════
//  OutputControl.cpp  v7.0
// ════════════════════════════════════════════════

#include "OutputControl.h"
#include "SlaveComm.h"

OutputControl Output;

void OutputControl::_sendAssist(const char *cmd) { Comm.sendToAssistNow(cmd); }

void OutputControl::begin()  { allOff(); }

// ── LED ──────────────────────────────────────
void OutputControl::ledOn()     { _ledState=true;  notif.ledOn=true;  _sendAssist(CMD_LED_ON); }
void OutputControl::ledOff()    { _ledState=false; notif.ledOn=false; _sendAssist(CMD_LED_OFF); }
void OutputControl::ledToggle() { _ledState ? ledOff() : ledOn(); }

// ── Water ─────────────────────────────────────
void OutputControl::waterOn()  { _waterManual=true;  notif.waterOn=true;  _sendAssist(CMD_WATER_ON); }
void OutputControl::waterOff() { _waterManual=false; _waterEnd=0; notif.waterOn=false; _sendAssist(CMD_WATER_OFF); }
void OutputControl::waterTimed() {
    _waterEnd=millis()+gSettings.waterTimeMs;
    _waterManual=false; notif.waterOn=true; _sendAssist(CMD_WATER_ON);
}

// ── Basin ─────────────────────────────────────
void OutputControl::basinOn()  { _basinManual=true;  notif.basinOn=true;  _sendAssist(CMD_BASIN_ON); }
void OutputControl::basinOff() { _basinManual=false; _basinEnd=0; notif.basinOn=false; _sendAssist(CMD_BASIN_OFF); }
void OutputControl::basinTimed() {
    _basinEnd=millis()+gSettings.basinTimeMs;
    _basinManual=false; notif.basinOn=true; _sendAssist(CMD_BASIN_ON);
}

// ── Suction (v7) ──────────────────────────────
void OutputControl::suctionOn()  { _suctManual=true;  notif.suctionOn=true;  _sendAssist(CMD_SUCTION_ON); }
void OutputControl::suctionOff() { _suctManual=false; _suctEnd=0; notif.suctionOn=false; _sendAssist(CMD_SUCTION_OFF); }
void OutputControl::suctionTimed() {
    _suctEnd=millis()+gSettings.suctionTimeMs;
    _suctManual=false; notif.suctionOn=true; _sendAssist(CMD_SUCTION_ON);
}

// ── Beep ──────────────────────────────────────
void OutputControl::beepBase(uint16_t freq, uint16_t durationMs) {
    Comm.sendToBase(CMD_BEEP, freq, durationMs);
}

void OutputControl::beepAssist(uint16_t freq, uint16_t durationMs) {
    Comm.sendToAssist(CMD_BEEP, freq, durationMs);
}
    // void OutputControl::beepBase(uint16_t durationMs) {
    //     Comm.sendToBase(CMD_BEEP, durationMs);
    // }

    // void OutputControl::beepAssist(uint16_t durationMs) {
    //     Comm.sendToAssist(CMD_BEEP, durationMs);
    // }


// ── All OFF ───────────────────────────────────
void OutputControl::allOff() {
    _ledState=_waterManual=_basinManual=_suctManual=false;
    _waterEnd=_basinEnd=_suctEnd=0;
    notif.ledOn=notif.waterOn=notif.basinOn=notif.suctionOn=false;
    _sendAssist(CMD_ALL_OFF);
}

// ── Timed shutoffs ────────────────────────────
void OutputControl::update() {
    uint32_t now=millis();
    if (_waterEnd>0 && now>=_waterEnd) waterOff();
    if (_basinEnd>0 && now>=_basinEnd) basinOff();
    if (_suctEnd >0 && now>=_suctEnd)  suctionOff();
}
