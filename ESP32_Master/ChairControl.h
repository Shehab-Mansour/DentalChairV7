#pragma once

// ════════════════════════════════════════════════
//  ChairControl.h  v7.0
//  يرسل عبر SlaveComm → Base
//  ADC يجي من Base callback
//  Limits: يمنع الحركة لو تخطت الحدود
// ════════════════════════════════════════════════

#include "shard.h"

class ChairControl {
public:
    void begin();
    void update();

    void moveUp();
    void moveDown();
    void moveForward();
    void moveBack();
    void stopAll();

    void    goToPosition(uint8_t slot);
    bool    isMoving()     const;
    bool    isAutoMoving() const { return _autoMove; }
    uint8_t autoProgress() const { return _autoProgress; }

    // ADC يتحدث من Comm callback
    void     setADC(uint16_t ud, uint16_t fb) { _cachedUD = ud; _cachedFB = fb; }
    uint16_t readUpDown()  const { return _cachedUD; }
    uint16_t readFwdBack() const { return _cachedFB; }

    // Limits management
    void captureLimit(const char *name);  // يحفظ القيمة الحالية كـ limit
    bool checkLimits(bool up, bool fwd) const;

    // Position management
    void    saveCurrentPosition(uint8_t slot);
    void    deletePosition(uint8_t slot);
    void    setPositionHotkey(uint8_t slot, uint8_t hotkey);
    void    setPositionLabel(uint8_t slot, const char *label);
    uint8_t slotForHotkey(uint8_t hotkey) const;

private:
    uint16_t _cachedUD = 2048;
    uint16_t _cachedFB = 2048;

    bool _goingUp   = false;
    bool _goingDown = false;
    bool _goingFwd  = false;
    bool _goingBack = false;
    bool _motorOn   = false;

    bool     _autoMove     = false;
    uint8_t  _targetSlot   = 0;
    uint16_t _targetUD     = 0;
    uint16_t _targetFB     = 0;
    uint8_t  _autoProgress = 0;

    void _sendNow(const char *cmd);
    void _send(const char *cmd);
    void _motorNeeded(bool on);
};

extern ChairControl Chair;
