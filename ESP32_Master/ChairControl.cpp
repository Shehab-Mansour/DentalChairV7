// ════════════════════════════════════════════════
//  ChairControl.cpp  v7.0
// ════════════════════════════════════════════════

#include "ChairControl.h"
#include "SlaveComm.h"
#include "Display.h"
#include "SDManager.h"

ChairControl Chair;

void ChairControl::begin()  { stopAll(); }

void ChairControl::_sendNow(const char *cmd) { Comm.sendToBaseNow(cmd); }
void ChairControl::_send(const char *cmd)    { Comm.sendToBase(cmd); }

void ChairControl::_motorNeeded(bool on) {
    if (on == _motorOn) return;
    _motorOn = on;
    _sendNow(on ? CMD_MOTOR_ON : CMD_MOTOR_OFF);
}

// ──────────────────────────────────────────────
//  Limits check
//  يرجع true لو الحركة مسموح بيها
// ──────────────────────────────────────────────
bool ChairControl::checkLimits(bool up, bool fwd) const {
    const ChairLimits &L = gSettings.limits;
    uint16_t ud = _cachedUD, fb = _cachedFB;

    if (up  && L.udMaxSet && ud >= L.udMax) { Display.showPopup("Max UP reached!",  1000); return false; }
    if (!up && L.udMinSet && ud <= L.udMin) { Display.showPopup("Max DOWN reached!", 1000); return false; }
    if (fwd  && L.fbMaxSet && fb >= L.fbMax) { Display.showPopup("Max FWD reached!",  1000); return false; }
    if (!fwd && L.fbMinSet && fb <= L.fbMin) { Display.showPopup("Max BACK reached!", 1000); return false; }
    return true;
}

// ──────────────────────────────────────────────
//  Manual movement
// ──────────────────────────────────────────────
void ChairControl::moveUp() {
    if (!checkLimits(true, true)) return;
    if (_goingDown) { _sendNow(CMD_DOWN_OFF); _goingDown = false; }
    if (!_goingUp)  { _goingUp = true; _motorNeeded(true); _sendNow(CMD_UP_ON); }
    notif.chairMoving = true;
}
void ChairControl::moveDown() {
    if (!checkLimits(false, true)) return;
    if (_goingUp)    { _sendNow(CMD_UP_OFF); _goingUp = false; _motorNeeded(_goingFwd); }
    if (!_goingDown) { _goingDown = true; _sendNow(CMD_DOWN_ON); }
    notif.chairMoving = true;
}
void ChairControl::moveForward() {
    if (!checkLimits(true, true)) return;
    if (_goingBack) { _sendNow(CMD_BACK_OFF); _goingBack = false; }
    if (!_goingFwd) { _goingFwd = true; _motorNeeded(true); _sendNow(CMD_FWD_ON); }
    notif.chairMoving = true;
}
void ChairControl::moveBack() {
    if (!checkLimits(true, false)) return;
    if (_goingFwd)   { _sendNow(CMD_FWD_OFF); _goingFwd = false; _motorNeeded(_goingUp); }
    if (!_goingBack) { _goingBack = true; _sendNow(CMD_BACK_ON); }
    notif.chairMoving = true;
}

void ChairControl::stopAll() {
    bool wasActive = _goingUp||_goingDown||_goingFwd||_goingBack||_motorOn||_autoMove;
    _goingUp=_goingDown=_goingFwd=_goingBack=false;
    _autoMove = false;
    if (_motorOn || wasActive) { _motorOn = false; _sendNow(CMD_ALL_OFF); }
    notif.chairMoving = false;
    Display.setProgress(0, false);
    _autoProgress = 0;
}

bool ChairControl::isMoving() const {
    return _goingUp||_goingDown||_goingFwd||_goingBack||_autoMove;
}

// ──────────────────────────────────────────────
//  Capture current ADC as a limit
// ──────────────────────────────────────────────
void ChairControl::captureLimit(const char *name) {
    ChairLimits &L = gSettings.limits;
    if      (strcmp(name, CMD_SET_LIM_UD_MAX) == 0) { L.udMax = _cachedUD; L.udMaxSet = true; }
    else if (strcmp(name, CMD_SET_LIM_UD_MIN) == 0) { L.udMin = _cachedUD; L.udMinSet = true; }
    else if (strcmp(name, CMD_SET_LIM_FB_MAX) == 0) { L.fbMax = _cachedFB; L.fbMaxSet = true; }
    else if (strcmp(name, CMD_SET_LIM_FB_MIN) == 0) { L.fbMin = _cachedFB; L.fbMinSet = true; }
    SDMgr.saveSettings(gSettings);

    // إبعت الليميتات للـ Base عشان هو كمان يفرض عليهم
    Comm.sendToBase(CMD_SET_LIM_UD_MAX, L.udMax);
    Comm.sendToBase(CMD_SET_LIM_UD_MIN, L.udMin);
    Comm.sendToBase(CMD_SET_LIM_FB_MAX, L.fbMax);
    Comm.sendToBase(CMD_SET_LIM_FB_MIN, L.fbMin);
}

// ──────────────────────────────────────────────
//  Position management
// ──────────────────────────────────────────────
void ChairControl::saveCurrentPosition(uint8_t slot) {
    if (slot<1||slot>MAX_POSITIONS) return;
    uint8_t idx=slot-1;
    gPositions[idx].slot=slot;
    gPositions[idx].updown=_cachedUD;
    gPositions[idx].fwdback=_cachedFB;
    gPositions[idx].saved=true;
    if (gPositions[idx].label[0]=='\0') snprintf(gPositions[idx].label,7,"P%d",slot);
    SDMgr.savePositions(gPositions,MAX_POSITIONS);
}
void ChairControl::deletePosition(uint8_t slot) {
    if (slot<1||slot>MAX_POSITIONS) return;
    gPositions[slot-1].saved=false;
    gPositions[slot-1].hotkey=0;
    gPositions[slot-1].label[0]='\0';
    SDMgr.savePositions(gPositions,MAX_POSITIONS);
}
void ChairControl::setPositionHotkey(uint8_t slot, uint8_t hotkey) {
    if (slot<1||slot>MAX_POSITIONS) return;
    if (hotkey!=0) for (uint8_t i=0;i<MAX_POSITIONS;i++) if (gPositions[i].hotkey==hotkey) gPositions[i].hotkey=0;
    gPositions[slot-1].hotkey=hotkey;
    SDMgr.savePositions(gPositions,MAX_POSITIONS);
}
void ChairControl::setPositionLabel(uint8_t slot, const char *label) {
    if (slot<1||slot>MAX_POSITIONS) return;
    strncpy(gPositions[slot-1].label,label,11); gPositions[slot-1].label[11]='\0';
    SDMgr.savePositions(gPositions,MAX_POSITIONS);
}
uint8_t ChairControl::slotForHotkey(uint8_t hotkey) const {
    for (uint8_t i=0;i<MAX_POSITIONS;i++) if (gPositions[i].saved&&gPositions[i].hotkey==hotkey) return gPositions[i].slot;
    return 0;
}
void ChairControl::goToPosition(uint8_t slot) {
    if (slot<1||slot>MAX_POSITIONS||!gPositions[slot-1].saved) return;
    stopAll();
    _targetSlot=slot;
    _targetUD=gPositions[slot-1].updown;
    _targetFB=gPositions[slot-1].fwdback;
    _autoMove=true;
    notif.chairMoving=true;
    gAppState=APP_AUTO_MOVING;
}

// ──────────────────────────────────────────────
//  update() — auto-move state machine
// ──────────────────────────────────────────────
void ChairControl::update() {
    if (!_autoMove) return;
    static uint32_t lastMove = 0;
    if (!elapsed(lastMove, CHAIR_MOVE_INTERVAL_MS)) return;

    int diffUD = (int)_cachedUD - (int)_targetUD;
    int diffFB = (int)_cachedFB - (int)_targetFB;
    bool doneUD = (abs(diffUD) <= DEAD_BAND);
    bool doneFB = (abs(diffFB) <= DEAD_BAND);

    // Progress
    {
        static uint32_t initDist = 0;
        static uint8_t  prevSlot = 0;
        if (_targetSlot != prevSlot) { initDist=(uint32_t)(abs(diffUD)+abs(diffFB)); prevSlot=_targetSlot; }
        uint32_t rem = (uint32_t)(abs(diffUD)+abs(diffFB));
        if (initDist>0) _autoProgress=(uint8_t)((initDist-min(rem,initDist))*100/initDist);
        else _autoProgress=100;
        Display.setProgress(_autoProgress,true);
    }

    // UD axis
    if (!doneUD) {
        if (diffUD<0) {
            if (_goingDown){_sendNow(CMD_DOWN_OFF);_goingDown=false;}
            _motorNeeded(true);
            if (!_goingUp){_sendNow(CMD_UP_ON);_goingUp=true;}
        } else {
            if (_goingUp){_sendNow(CMD_UP_OFF);_goingUp=false;_motorNeeded(_goingFwd);}
            if (!_goingDown){_sendNow(CMD_DOWN_ON);_goingDown=true;}
        }
    } else {
        if (_goingUp)  {_sendNow(CMD_UP_OFF);  _goingUp=false;}
        if (_goingDown){_sendNow(CMD_DOWN_OFF);_goingDown=false;}
        _motorNeeded(_goingFwd);
    }

    // FB axis
    if (!doneFB) {
        if (diffFB<0) {
            if (_goingBack){_sendNow(CMD_BACK_OFF);_goingBack=false;}
            _motorNeeded(true);
            if (!_goingFwd){_sendNow(CMD_FWD_ON);_goingFwd=true;}
        } else {
            if (_goingFwd){_sendNow(CMD_FWD_OFF);_goingFwd=false;_motorNeeded(_goingUp);}
            if (!_goingBack){_sendNow(CMD_BACK_ON);_goingBack=true;}
        }
    } else {
        if (_goingFwd) {_sendNow(CMD_FWD_OFF); _goingFwd=false;}
        if (_goingBack){_sendNow(CMD_BACK_OFF);_goingBack=false;}
        _motorNeeded(_goingUp);
    }

    if (doneUD && doneFB) {
        stopAll();
        gAppState=APP_HOME;
        Display.showPopup("Position Reached!",1200);
    }
}
