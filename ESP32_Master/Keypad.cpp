// ════════════════════════════════════════════════
//  Keypad.cpp  —  Matrix Keypad 4×4
//  v4.0  —  State machine debounce + long/short press
// ════════════════════════════════════════════════

#include "Keypad.h"

KeypadHandler Keypad;

const uint8_t KeypadHandler::ROWS[4] = { KP_ROW1, KP_ROW2, KP_ROW3, KP_ROW4 };
const uint8_t KeypadHandler::COLS[4] = { KP_COL1, KP_COL2, KP_COL3, KP_COL4 };
const char    KeypadHandler::KEYS[4][4] = {
    { '1', '2', '3', 'A' },
    { '4', '5', '6', 'B' },
    { '7', '8', '9', 'C' },
    { '*', '0', '#', 'D' }
};

// ──────────────────────────────────────────────
void KeypadHandler::begin() {
    for (uint8_t r = 0; r < 4; r++) {
        pinMode(ROWS[r], OUTPUT);
        digitalWrite(ROWS[r], HIGH);
    }
    for (uint8_t c = 0; c < 4; c++) {
        pinMode(COLS[c], INPUT_PULLUP);
    }
}

// ──────────────────────────────────────────────
//  _rawScan — مسح سريع بدون debounce
//  يرجع أول زرار مضغوط أو KEY_NONE
// ──────────────────────────────────────────────
char KeypadHandler::_rawScan() {
    for (uint8_t r = 0; r < 4; r++) {
        digitalWrite(ROWS[r], LOW);
        delayMicroseconds(SETTLE_US);
        for (uint8_t c = 0; c < 4; c++) {
            if (digitalRead(COLS[c]) == LOW) {
                digitalWrite(ROWS[r], HIGH);
                return KEYS[r][c];
            }
        }
        digitalWrite(ROWS[r], HIGH);
    }
    return KEY_NONE;
}

// ──────────────────────────────────────────────
//  scan() — State machine كاملة
//
//  State machine:
//
//    IDLE ──(raw != NONE)──> MAYBE_DOWN
//    MAYBE_DOWN ──(timeout < DEBOUNCE && raw==same)──> DOWN  → fire EVT_LONG_PRESS
//    MAYBE_DOWN ──(raw changed)──> IDLE
//    DOWN ──(raw==NONE)──> MAYBE_UP
//    DOWN ──(every REPEAT_MS)──> fire EVT_HOLD
//    MAYBE_UP ──(timeout >= DEBOUNCE)──> IDLE → fire EVT_SHORT_PRESS or EVT_RELEASE
//    MAYBE_UP ──(raw != NONE again)──> DOWN
//
// ──────────────────────────────────────────────
KeyState KeypadHandler::scan() {
    char     raw = _rawScan();
    uint32_t now = millis();

    switch (_state) {

        // ── IDLE: لا يوجد ضغط ──────────────────
        case IDLE:
            if (raw != KEY_NONE) {
                _rawKey   = raw;
                _stateTs  = now;
                _state    = MAYBE_DOWN;
            }
            return { KEY_NONE, EVT_NONE };

        // ── MAYBE_DOWN: ننتظر debounce ──────────
        case MAYBE_DOWN:
            if (raw != _rawKey) {
                // مش نفس الزرار — ghost press — تجاهل
                _state = IDLE;
                return { KEY_NONE, EVT_NONE };
            }
            if (now - _stateTs >= DEBOUNCE_MS) {
                // تأكد الضغط
                _curKey    = _rawKey;
                _holdTs    = now;
                _repeatTs  = now;
                _longFired = false;
                _state     = DOWN;
                // لا نُطلق حدث هنا — ننتظر النوع (short/long)
            }
            return { KEY_NONE, EVT_NONE };

        // ── DOWN: الزرار مضغوط ──────────────────
        case DOWN:
            if (raw == KEY_NONE) {
                // رُفع الإصبع — انتظر debounce على الرفع
                _stateTs = now;
                _state   = MAYBE_UP;
                return { KEY_NONE, EVT_NONE };
            }
            if (raw != _curKey) {
                // تغيير مفاجئ — أوقف الحالية
                _state = IDLE;
                if (_longFired) return { _curKey, EVT_RELEASE };
                return { _curKey, EVT_SHORT_PRESS };  // ضغطة قصيرة على المفتاح السابق
            }
            {
                uint32_t heldMs = now - _holdTs;
                // أول مرة يتجاوز holdThreshold → long press
                if (!_longFired && heldMs >= gSettings.holdThresholdMs) {
                    _longFired = true;
                    _repeatTs  = now;
                    return { _curKey, EVT_LONG_PRESS };
                }
                // repeat بعد long press
                if (_longFired && now - _repeatTs >= REPEAT_MS) {
                    _repeatTs = now;
                    return { _curKey, EVT_HOLD };
                }
            }
            return { KEY_NONE, EVT_NONE };

        // ── MAYBE_UP: ننتظر debounce على الرفع ──
        case MAYBE_UP:
            if (raw != KEY_NONE) {
                // عاد الضغط — bouncing أو ضغطة جديدة
                if (raw == _curKey) {
                    _state = DOWN;  // نفس الزرار — كمّل
                } else {
                    // زرار مختلف — خلّص الحالي أولاً
                    char prev = _curKey;
                    _state = IDLE;
                    if (_longFired) return { prev, EVT_RELEASE };
                    return { prev, EVT_SHORT_PRESS };
                }
                return { KEY_NONE, EVT_NONE };
            }
            if (now - _stateTs >= DEBOUNCE_MS) {
                // رفع مؤكد
                char prev = _curKey;
                _curKey   = KEY_NONE;
                _state    = IDLE;
                if (_longFired) return { prev, EVT_RELEASE };
                return { prev, EVT_SHORT_PRESS };
            }
            return { KEY_NONE, EVT_NONE };
    }

    return { KEY_NONE, EVT_NONE };  // unreachable
}
