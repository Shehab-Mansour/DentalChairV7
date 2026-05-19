// ════════════════════════════════════════════════
//  ESP8266_Base.ino  —  Chair Base Controller v7.0
//  ────────────────────────────────────────────
//  المسؤوليات:
//  1. تحكم في الريليهات (UP/DOWN/FWD/BACK/MOTOR)
//  2. قراءة ADC (البوتنشيومتر UP/DOWN فقط)
//     * FB pot مش موجود هنا — بيتعامل معاه Master
//  3. استقبال أوامر الـ Foot Switch (4 سويتشات)
//  4. تطبيق الـ Safety Limits محلياً
//  5. إرسال ADC للـ Master كل 100ms
//  6. إرسال Foot Switch events للـ Master
//
//  Pinout (ESP8266 NodeMCU / Wemos D1 Mini):
//   D1 (GPIO5)  = RELAY_UP
//   D2 (GPIO4)  = RELAY_DOWN
//   D5 (GPIO14) = RELAY_FWD
//   D6 (GPIO12) = RELAY_BACK
//   D7 (GPIO13) = RELAY_MOTOR
//   A0 (ADC)    = POT_UPDOWN
//   D3 (GPIO0)  = FOOT_UP    (INPUT_PULLUP)
//   D4 (GPIO2)  = FOOT_DOWN  (INPUT_PULLUP)
//   D8 (GPIO15) = FOOT_FWD   (INPUT_PULLUP)
//   RX (GPIO3)  = FOOT_BACK  (INPUT_PULLUP)
//   TX (GPIO1)  → Serial to ESP32 (TX→RX16, RX→TX17)
//
//  ⚠️ D8/GPIO15 يحتاج pull-down للبوت — شوف datasheet
// ════════════════════════════════════════════════

#include "protocol.h"

// ── Relay pins ───────────────────────────────
#define RELAY_UP      5   // D1
#define RELAY_DOWN    4   // D2
#define RELAY_FWD    14   // D5
#define RELAY_BACK   12   // D6
#define RELAY_MOTOR  13   // D7

// Relay logic: ACTIVE_LOW أو ACTIVE_HIGH — عدّل حسب موديل الريليه
#define RELAY_ON   HIGH
#define RELAY_OFF  LOW

// ── ADC ──────────────────────────────────────
#define POT_UPDOWN   A0   // ESP8266 عنده ADC واحد بس
#define ADC_SAMPLES  8

// ── Foot switch pins ─────────────────────────
#define FOOT_UP    0    // D3 GPIO0
#define FOOT_DOWN  2    // D4 GPIO2
#define FOOT_FWD   14   // D5 — shared with relay? use different pin in real HW
// ⚠️ استخدم GPIO مختلف للـ foot switches في الـ PCB الحقيقي
// مثال: GPIO9, GPIO10 (على الـ ESP8266-12F)
// هنا للتوضيح فقط:
#define FOOT_BACK  15   // D8 GPIO15 (active HIGH مع external pull-down)
#define BUZZER_PIN 16  // D0 — اختياري


// ── Timing ───────────────────────────────────
#define ADC_SEND_MS      100
#define DEBOUNCE_MS       30
#define PING_INTERVAL_MS 3000
#define DEAD_BAND         80

// ──────────────────────────────────────────────
//  State
// ──────────────────────────────────────────────
struct RelayState {
    bool up    = false;
    bool down  = false;
    bool fwd   = false;
    bool back  = false;
    bool motor = false;
} relay;

struct Limits {
    uint16_t udMax = 0; bool udMaxSet = false;
    uint16_t udMin = 0; bool udMinSet = false;
    uint16_t fbMax = 0; bool fbMaxSet = false;
    uint16_t fbMin = 0; bool fbMinSet = false;
} limits;

// Foot switch debounce
struct FootSW {
    uint8_t  pin;
    bool     activeHigh;  // false = PULLUP (pressed=LOW)
    bool     lastState = false;
    bool     curState  = false;
    uint32_t changeMs  = 0;
    const char *cmdOn;
    const char *cmdOff;
} feet[4];

uint16_t cachedADC = 0;

// Packet RX buffer
char rxBuf[PKT_MAXLEN + 2] = {};
uint8_t rxLen = 0;

// ──────────────────────────────────────────────
//  Helpers
// ──────────────────────────────────────────────
void setRelay(uint8_t pin, bool on) {
    digitalWrite(pin, on ? RELAY_ON : RELAY_OFF);
}

void motorNeeded(bool on) {
    if (relay.motor == on) return;
    relay.motor = on;
    setRelay(RELAY_MOTOR, on);
}

void sendToMaster(const char *cmd, int32_t val = 0) {
    char pkt[PKT_MAXLEN + 1];
    snprintf(pkt, PKT_MAXLEN, "[%c:%s:%ld]\n", DST_MASTER, cmd, (long)val);
    Serial.print(pkt);
}

uint16_t readADC() {
    uint32_t sum = 0;
    for (uint8_t i = 0; i < ADC_SAMPLES; i++) sum += analogRead(POT_UPDOWN);
    return (uint16_t)(sum / ADC_SAMPLES);
}

// ──────────────────────────────────────────────
//  Relay commands (with limit check)
// ──────────────────────────────────────────────
void doUp() {
    if (limits.udMaxSet && cachedADC >= limits.udMax) {
        sendToMaster(CMD_LIMIT_HIT, 1);
        return;
    }
    if (relay.down) { setRelay(RELAY_DOWN, RELAY_OFF); relay.down = false; }
    if (!relay.up)  { motorNeeded(true); setRelay(RELAY_UP, RELAY_ON); relay.up = true; }
}
void doDown() {
    if (limits.udMinSet && cachedADC <= limits.udMin) {
        sendToMaster(CMD_LIMIT_HIT, 2);
        return;
    }
    if (relay.up)   { setRelay(RELAY_UP, RELAY_OFF); relay.up = false; motorNeeded(relay.fwd); }
    if (!relay.down){ setRelay(RELAY_DOWN, RELAY_ON); relay.down = true; }
}
void doFwd() {
    if (relay.back) { setRelay(RELAY_BACK, RELAY_OFF); relay.back = false; }
    if (!relay.fwd) { motorNeeded(true); setRelay(RELAY_FWD, RELAY_ON); relay.fwd = true; }
}
void doBack() {
    if (relay.fwd)  { setRelay(RELAY_FWD, RELAY_OFF); relay.fwd = false; motorNeeded(relay.up); }
    if (!relay.back){ setRelay(RELAY_BACK, RELAY_ON); relay.back = true; }
}
void doAllOff() {
    setRelay(RELAY_UP,    RELAY_OFF); relay.up    = false;
    setRelay(RELAY_DOWN,  RELAY_OFF); relay.down  = false;
    setRelay(RELAY_FWD,   RELAY_OFF); relay.fwd   = false;
    setRelay(RELAY_BACK,  RELAY_OFF); relay.back  = false;
    setRelay(RELAY_MOTOR, RELAY_OFF); relay.motor = false;
}

// ──────────────────────────────────────────────
//  Packet parser
// ──────────────────────────────────────────────
void processPacket(const char *pkt) {
    if (pkt[0] != '[') return;
    char buf[PKT_MAXLEN + 1];
    strncpy(buf, pkt + 1, PKT_MAXLEN);
    char *end = strrchr(buf, ']'); if (end) *end = '\0';

    char *p1 = strtok(buf, ":");
    char *p2 = strtok(nullptr, ":");
    char *p3 = strtok(nullptr, ":");
    if (!p1 || !p2) return;

    // ACK to master
    char ack[PKT_MAXLEN];
    snprintf(ack, sizeof(ack), "[%s:%s]\n", CMD_ACK, p2);
    Serial.print(ack);

    // PING → PONG
    if (strcmp(p2, CMD_PING) == 0) {
        char pong[PKT_MAXLEN];
        snprintf(pong, sizeof(pong), "[%c:%s:0]\n", DST_MASTER, CMD_PONG);
        Serial.print(pong);
        return;
    }

    int32_t val = p3 ? atol(p3) : 0;
    const char *cmd = p2;

    if      (strcmp(cmd, CMD_UP_ON)     == 0) doUp();
    else if (strcmp(cmd, CMD_UP_OFF)    == 0) { setRelay(RELAY_UP,  RELAY_OFF); relay.up=false; }
    else if (strcmp(cmd, CMD_DOWN_ON)   == 0) doDown();
    else if (strcmp(cmd, CMD_DOWN_OFF)  == 0) { setRelay(RELAY_DOWN,RELAY_OFF); relay.down=false; }
    else if (strcmp(cmd, CMD_FWD_ON)    == 0) doFwd();
    else if (strcmp(cmd, CMD_FWD_OFF)   == 0) { setRelay(RELAY_FWD, RELAY_OFF); relay.fwd=false; motorNeeded(relay.up); }
    else if (strcmp(cmd, CMD_BACK_ON)   == 0) doBack();
    else if (strcmp(cmd, CMD_BACK_OFF)  == 0) { setRelay(RELAY_BACK,RELAY_OFF); relay.back=false; }
    else if (strcmp(cmd, CMD_MOTOR_ON)  == 0) motorNeeded(true);
    else if (strcmp(cmd, CMD_MOTOR_OFF) == 0) motorNeeded(false);
    else if (strcmp(cmd, CMD_ALL_OFF)   == 0) doAllOff();
    // else if (strcmp(cmd, CMD_BEEP)      == 0) tone(BUZZER_PIN, 1000, (int)val);
    else if (strcmp(cmd, CMD_BEEP) == 0) {

    int freq = 1000;
    int duration = 80;

    if (p3) {

        char valBuf[32];
        strncpy(valBuf, p3, sizeof(valBuf));
        valBuf[sizeof(valBuf) - 1] = '\0';

        char *comma = strchr(valBuf, ',');

        if (comma) {

            *comma = '\0';

            freq = atoi(valBuf);
            duration = atoi(comma + 1);

        } else {

            duration = atoi(valBuf);
        }
    }

    tone(BUZZER_PIN, freq, duration);
}
    // Limits
    else if (strcmp(cmd, CMD_SET_LIM_UD_MAX) == 0) { limits.udMax=(uint16_t)val; limits.udMaxSet=true; }
    else if (strcmp(cmd, CMD_SET_LIM_UD_MIN) == 0) { limits.udMin=(uint16_t)val; limits.udMinSet=true; }
    else if (strcmp(cmd, CMD_SET_LIM_FB_MAX) == 0) { limits.fbMax=(uint16_t)val; limits.fbMaxSet=true; }
    else if (strcmp(cmd, CMD_SET_LIM_FB_MIN) == 0) { limits.fbMin=(uint16_t)val; limits.fbMinSet=true; }
}

// ──────────────────────────────────────────────
//  setup()
// ──────────────────────────────────────────────

void setup() {
    // Serial to ESP32 Master
    Serial.begin(115200);

    // Relay pins
    uint8_t relayPins[] = {RELAY_UP, RELAY_DOWN, RELAY_FWD, RELAY_BACK, RELAY_MOTOR};
    for (uint8_t p : relayPins) { pinMode(p, OUTPUT); setRelay(p, RELAY_OFF); }

    // Buzzer (optional)
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);

    // Foot switches
    feet[0] = {FOOT_UP,   false, false, false, 0, CMD_FOOT_UP,   CMD_FOOT_REL};
    feet[1] = {FOOT_DOWN, false, false, false, 0, CMD_FOOT_DOWN, CMD_FOOT_REL};
    feet[2] = {FOOT_FWD,  false, false, false, 0, CMD_FOOT_FWD,  CMD_FOOT_REL};
    feet[3] = {FOOT_BACK, true,  false, false, 0, CMD_FOOT_BACK, CMD_FOOT_REL};

    for (auto &f : feet) {
        if (f.activeHigh) pinMode(f.pin, INPUT);
        else              pinMode(f.pin, INPUT_PULLUP);
    }

    // Initial ADC
    cachedADC = readADC();

    tone(BUZZER_PIN, 1200, 80);

    Serial.println("BASE READY");
}

// ──────────────────────────────────────────────
//  loop()
// ──────────────────────────────────────────────
void loop() {
    uint32_t now = millis();

    // ── Receive packets from Master ───────────
    while (Serial.available()) {
        char c = Serial.read();
        if (c == '\n') {
            rxBuf[rxLen] = '\0';
            if (rxLen > 0) processPacket(rxBuf);
            rxLen = 0;
        } else if (c == '[') {
            rxLen = 0; rxBuf[rxLen++] = c;
        } else if (rxLen > 0 && rxLen < PKT_MAXLEN) {
            rxBuf[rxLen++] = c;
        }
    }

    // ── Read ADC and send to Master ───────────
    {
        static uint32_t lastADC = 0;
        if (now - lastADC >= ADC_SEND_MS) {
            lastADC = now;
            uint16_t raw = readADC();
            // فقط لو تغيرت القيمة بشكل ملحوظ
            if (abs((int)raw - (int)cachedADC) > DEAD_BAND / 2) {
                cachedADC = raw;
                sendToMaster(CMD_ADC_UD, cachedADC);
            }
            // Real-time limit enforcement
            if (relay.up   && limits.udMaxSet && cachedADC >= limits.udMax) {
                setRelay(RELAY_UP, RELAY_OFF); relay.up = false;
                motorNeeded(relay.fwd);
                sendToMaster(CMD_LIMIT_HIT, 1);
            }
            if (relay.down && limits.udMinSet && cachedADC <= limits.udMin) {
                setRelay(RELAY_DOWN, RELAY_OFF); relay.down = false;
                sendToMaster(CMD_LIMIT_HIT, 2);
            }
        }
    }

    // ── Foot switch debounce ──────────────────
    for (auto &f : feet) {
        bool raw = f.activeHigh ? (digitalRead(f.pin) == HIGH)
                                 : (digitalRead(f.pin) == LOW);
        if (raw != f.lastState) {
            f.lastState  = raw;
            f.changeMs   = now;
        }
        if ((now - f.changeMs) >= DEBOUNCE_MS && raw != f.curState) {
            f.curState = raw;
            if (raw) {
                sendToMaster(f.cmdOn);
            } else {
                // تحقق لو كل الأزرار مفيش منهم مضغوط
                bool anyDown = false;
                for (auto &ff : feet) if (ff.curState) { anyDown = true; break; }
                if (!anyDown) sendToMaster(CMD_FOOT_REL);
            }
        }
    }

    yield();  // ESP8266 WDT
}
