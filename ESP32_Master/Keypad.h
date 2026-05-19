#pragma once

// ════════════════════════════════════════════════
//  Keypad.h  —  Matrix Keypad 4×4
//  v4.0 improvements:
//  • Debounce احترافي (state machine)
//  • Short press / Long press
//  • Key repeat أثناء الـ hold
//  • منع ghost clicks
//  • لا delay في scan
// ════════════════════════════════════════════════

#include "shard.h"

#define KEY_NONE  '\0'

// ──────────────────────────────────────────────
//  أحداث الكيباد — بدل char بسيط
// ──────────────────────────────────────────────
enum KeyEvent : uint8_t {
    EVT_NONE = 0,
    EVT_SHORT_PRESS,   // ضغطة قصيرة (release قبل holdThreshold)
    EVT_LONG_PRESS,    // بداية الـ hold (بعد holdThreshold مرة واحدة)
    EVT_HOLD,          // استمرار الـ hold (كل KEY_REPEAT_MS)
    EVT_RELEASE,       // رفع الإصبع بعد hold
};

struct KeyState {
    char     key;      // الزرار المضغوط
    KeyEvent event;    // نوع الحدث
};

// ──────────────────────────────────────────────
class KeypadHandler {
public:
    void     begin();
    KeyState scan();    // استدعاء كل loop — لا blocking

private:
    // Timing
    static constexpr uint32_t REPEAT_MS    = 120;  // key repeat interval أثناء hold
    static constexpr uint32_t SETTLE_US    = 8;    // وقت استقرار الـ pin (microseconds)

    // Internal scan — يرجع الزرار الفيزيائي بدون debounce
    char _rawScan();

    // Debounce state machine
    enum DebState : uint8_t { IDLE, MAYBE_DOWN, DOWN, MAYBE_UP };
    DebState _state     = IDLE;
    char     _rawKey    = KEY_NONE;
    char     _curKey    = KEY_NONE;
    uint32_t _stateTs   = 0;   // timestamp دخول الحالة الحالية
    uint32_t _holdTs    = 0;   // timestamp بداية الـ hold
    uint32_t _repeatTs  = 0;   // timestamp آخر repeat
    bool     _longFired = false;

    static const uint8_t ROWS[4];
    static const uint8_t COLS[4];
    static const char    KEYS[4][4];
};

extern KeypadHandler Keypad;
