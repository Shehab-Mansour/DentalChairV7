// ════════════════════════════════════════════════
//  ESP8266_Assistant.ino  —  Assistant Panel Controller v7.0
//  ────────────────────────────────────────────
//  المسؤوليات:
//  1. تشغيل/إيقاف: LED / Water / Basin / Suction solenoids
//  2. قراءة 5 زراير الأسيستانت بانل
//  3. قراءة Suction سويتش (السويتش اللي في الأسيستانت)
//  4. إرسال Button events للـ Master
//  5. استقبال أوامر من الـ Master
//
//  Pinout (ESP8266 NodeMCU / Wemos D1 Mini):
//   D1 (GPIO5)   = SOL_WATER   (Solenoid مياه)
//   D2 (GPIO4)   = SOL_BASIN   (Solenoid حوض)
//   D5 (GPIO14)  = SOL_SUCTION (Solenoid ساكشن)
//   D6 (GPIO12)  = RELAY_LED   (ريليه كشاف)
//   D7 (GPIO13)  = BUZZER_PIN  (بازر اختياري)
//
//  Assistant Panel Buttons (INPUT_PULLUP):
//   D3 (GPIO0)   = BTN_LED
//   D4 (GPIO2)   = BTN_WATER
//   D8 (GPIO15)  = BTN_BASIN    ⚠️ pull-down عند البوت
//   D0 (GPIO16)  = BTN_SUCTION_SW (السويتش الخاص)
//   A0 (ADC)     = *** غير مستخدم هنا ***
//
//  زراير البوزيشن (على I2C expander أو مباشر):
//   GPIO9        = BTN_POS1    (ESP8266-12F فقط)
//   GPIO10       = BTN_POS2
//
//  ⚠️ لو ماعندكش GPIO9/10، استخدم I2C expander مثل PCF8574
//     أو اعمل shift register
// ════════════════════════════════════════════════

#include "protocol.h"

// ── Output pins ───────────────────────────────
#define SOL_WATER    5    // D1
#define SOL_BASIN    4    // D2
#define SOL_SUCTION  14   // D5
#define RELAY_LED    12   // D6
#define BUZZER_PIN   13   // D7

// لو الريليه/السولينويد ACTIVE_LOW (الأكثر شيوعاً)
#define OUT_ON    LOW
#define OUT_OFF   HIGH

// ── Button pins ───────────────────────────────
#define BTN_LED_PIN      0    // D3 GPIO0
#define BTN_WATER_PIN    2    // D4 GPIO2
#define BTN_BASIN_PIN   15    // D8 GPIO15
#define BTN_SUCT_PIN    16    // D0 GPIO16
#define BTN_POS1_PIN     9    // GPIO9  (ESP8266-12F)
#define BTN_POS2_PIN    10    // GPIO10 (ESP8266-12F)

#define DEBOUNCE_MS 30

// ──────────────────────────────────────────────
//  State
// ──────────────────────────────────────────────
bool outState[4] = {false, false, false, false};
// 0=LED  1=WATER  2=BASIN  3=SUCTION

// Packet RX
char rxBuf[PKT_MAXLEN + 2] = {};
uint8_t rxLen = 0;

// Button struct
struct Btn {
    uint8_t    pin;
    bool       pullup;     // true=PULLUP (pressed=LOW), false=INPUT (pressed=HIGH)
    bool       lastRaw = false;
    bool       state   = false;
    uint32_t   changeMs = 0;
    const char *cmdPress;
    const char *cmdRelease;
};

Btn btns[6];

// ──────────────────────────────────────────────
//  Helpers
// ──────────────────────────────────────────────
void setOut(uint8_t pin, bool on) {
    digitalWrite(pin, on ? OUT_ON : OUT_OFF);
}

void sendToMaster(const char *cmd, int32_t val = 0) {
    char pkt[PKT_MAXLEN + 1];
    snprintf(pkt, PKT_MAXLEN, "[%c:%s:%ld]\n", DST_MASTER, cmd, (long)val);
    Serial.print(pkt);
}

// ──────────────────────────────────────────────
//  Packet processor
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

    // Send ACK
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

    const char *cmd = p2;

    if      (strcmp(cmd, CMD_LED_ON)      == 0) { setOut(RELAY_LED,   true);  outState[0]=true; }
    else if (strcmp(cmd, CMD_LED_OFF)     == 0) { setOut(RELAY_LED,   false); outState[0]=false; }
    else if (strcmp(cmd, CMD_WATER_ON)    == 0) { setOut(SOL_WATER,   true);  outState[1]=true; }
    else if (strcmp(cmd, CMD_WATER_OFF)   == 0) { setOut(SOL_WATER,   false); outState[1]=false; }
    else if (strcmp(cmd, CMD_BASIN_ON)    == 0) { setOut(SOL_BASIN,   true);  outState[2]=true; }
    else if (strcmp(cmd, CMD_BASIN_OFF)   == 0) { setOut(SOL_BASIN,   false); outState[2]=false; }
    else if (strcmp(cmd, CMD_SUCTION_ON)  == 0) { setOut(SOL_SUCTION, true);  outState[3]=true; }
    else if (strcmp(cmd, CMD_SUCTION_OFF) == 0) { setOut(SOL_SUCTION, false); outState[3]=false; }
    else if (strcmp(cmd, CMD_ALL_OFF)     == 0) {
        setOut(RELAY_LED,   false); setOut(SOL_WATER,   false);
        setOut(SOL_BASIN,   false); setOut(SOL_SUCTION, false);
        for (uint8_t i=0;i<4;i++) outState[i]=false;
    }
    else if (strcmp(cmd, CMD_BEEP)        == 0) {
        int32_t dur = p3 ? atol(p3) : 80;
        tone(BUZZER_PIN, 1000, (unsigned long)dur);
    }
    else if (strcmp(cmd, CMD_GOTO_POS) == 0) {
        // Master يبعت GOTO_POS confirm — نعمل beep صغير
        tone(BUZZER_PIN, 1200, 60);
    }
}

// ──────────────────────────────────────────────
//  setup()
// ──────────────────────────────────────────────
void setup() {
    Serial.begin(115200);

    // Output pins
    uint8_t outPins[] = {SOL_WATER, SOL_BASIN, SOL_SUCTION, RELAY_LED};
    for (uint8_t p : outPins) { pinMode(p, OUTPUT); setOut(p, false); }
    pinMode(BUZZER_PIN, OUTPUT); digitalWrite(BUZZER_PIN, LOW);

    // Buttons
    btns[0] = {BTN_LED_PIN,   true,  false,false,0, CMD_BTN_LED,     CMD_BTN_REL};
    btns[1] = {BTN_WATER_PIN, true,  false,false,0, CMD_BTN_WATER,   CMD_BTN_REL};
    btns[2] = {BTN_BASIN_PIN, false, false,false,0, CMD_BTN_BASIN,   CMD_BTN_REL};  // D8 no pullup
    btns[3] = {BTN_SUCT_PIN,  true,  false,false,0, CMD_BTN_SUCTION, CMD_BTN_REL};
    btns[4] = {BTN_POS1_PIN,  true,  false,false,0, CMD_BTN_POS1,    CMD_BTN_REL};
    btns[5] = {BTN_POS2_PIN,  true,  false,false,0, CMD_BTN_POS2,    CMD_BTN_REL};

    for (auto &b : btns) {
        if (b.pullup) pinMode(b.pin, INPUT_PULLUP);
        else          pinMode(b.pin, INPUT);
    }

    tone(BUZZER_PIN, 1200, 80);
    Serial.println("ASSIST READY");
}

// ──────────────────────────────────────────────
//  loop()
// ──────────────────────────────────────────────
void loop() {
    uint32_t now = millis();

    // ── Receive from Master ───────────────────
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

    // ── Button debounce and event send ────────
    for (auto &b : btns) {
        bool raw = b.pullup ? (digitalRead(b.pin) == LOW)
                            : (digitalRead(b.pin) == HIGH);
        if (raw != b.lastRaw) {
            b.lastRaw  = raw;
            b.changeMs = now;
        }
        if ((now - b.changeMs) >= DEBOUNCE_MS && raw != b.state) {
            b.state = raw;
            if (raw) {
                sendToMaster(b.cmdPress);
            } else {
                sendToMaster(b.cmdRelease);
            }
        }
    }

    yield();
}
