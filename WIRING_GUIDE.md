# Dental Chair v7.0 — Complete Wiring Guide
## توصيل الـ 3 MCUs + الكهرباء

---

## نظرة عامة على المنظومة

```
┌─────────────────────────────────────┐
│           ESP32 Master              │
│   (OLED + Keypad + SD + RTC + WiFi) │
│                                     │
│  UART1 TX=17 ──────────────────────►│ ESP8266 BASE
│  UART1 RX=16 ◄──────────────────────│ (تحت الكرسي)
│                                     │
│  UART2 TX=4  ──────────────────────►│ ESP8266 ASSISTANT
│  UART2 RX=2  ◄──────────────────────│ (الأسيستانت بانل)
└─────────────────────────────────────┘
```

**⚠️ مهم:** كل الـ 3 MCUs على نفس الـ GND مشترك

---

## 1. ESP32 Master ← → ESP8266 Base

### التوصيل
| ESP32 Master | Wire  | ESP8266 Base |
|-------------|-------|-------------|
| GPIO17 (TX) | ─────►| RX (GPIO3)  |
| GPIO16 (RX) | ◄─────| TX (GPIO1)  |
| GND         | ─────►| GND         |
| 3.3V        | ─────►| 3.3V (VCC)  |

### ⚠️ مهم: Level Shifter
- ESP32 = 3.3V logic ✓
- ESP8266 = 3.3V logic ✓
- **لا يحتاج level shifter** — مباشر

---

## 2. ESP32 Master ← → ESP8266 Assistant

| ESP32 Master | Wire  | ESP8266 Assistant |
|-------------|-------|------------------|
| GPIO4 (TX)  | ─────►| RX (GPIO3)       |
| GPIO2 (RX)  | ◄─────| TX (GPIO1)       |
| GND         | ─────►| GND              |
| 3.3V        | ─────►| 3.3V (VCC)       |

---

## 3. ESP8266 Base — Pinout الكامل

```
ESP8266 Base (NodeMCU 12E / Wemos D1 Mini)
═══════════════════════════════════════════
OUTPUT SIDE:
  D1  GPIO5  → RELAY_UP    (صعود الكرسي)
  D2  GPIO4  → RELAY_DOWN  (نزول الكرسي)
  D5  GPIO14 → RELAY_FWD   (أمام)
  D6  GPIO12 → RELAY_BACK  (خلف)
  D7  GPIO13 → RELAY_MOTOR (موتور هيدروليك)
  D0  GPIO16 → BUZZER      (بازر اختياري)

INPUT SIDE:
  A0         ← POT_UPDOWN  (بوتنشيومتر صعود/نزول 0-3.3V)

FOOT SWITCHES (INPUT_PULLUP):
  D3  GPIO0  ← FOOT_UP    (ضغط = LOW)
  D4  GPIO2  ← FOOT_DOWN  (ضغط = LOW)
  D5  GPIO14 ← FOOT_FWD   ⚠️ conflict مع RELAY_FWD
               → استخدم GPIO9 أو GPIO10 بدلها في ESP8266-12F

SERIAL TO ESP32:
  TX  GPIO1  → ESP32 RX (GPIO16)
  RX  GPIO3  ← ESP32 TX (GPIO17)
```

### ⚠️ تعارض Pins في الـ Base
GPIO14 يُستخدم للـ relay وكمان للـ foot switch — **عدّل الكود** واستخدم:
```cpp
// في ESP8266_Base.ino — بدّل الـ foot switch لـ:
#define FOOT_FWD    9    // GPIO9  (ESP8266-12F فقط)
#define FOOT_BACK  10    // GPIO10 (ESP8266-12F فقط)
```
لو بتستخدم **NodeMCU** (مش 12F) استخدم **PCF8574 I2C expander** للـ extra inputs.

---

## 4. ESP8266 Assistant — Pinout الكامل

```
ESP8266 Assistant (NodeMCU 12E / Wemos D1 Mini)
═══════════════════════════════════════════════
OUTPUT SIDE:
  D1  GPIO5  → SOL_WATER    (سولينويد مياه)
  D2  GPIO4  → SOL_BASIN    (سولينويد حوض)
  D5  GPIO14 → SOL_SUCTION  (سولينويد ساكشن)
  D6  GPIO12 → RELAY_LED    (كشاف)
  D7  GPIO13 → BUZZER_PIN   (بازر)

ASSISTANT PANEL BUTTONS (INPUT_PULLUP):
  D3  GPIO0  ← BTN_LED      (زرار كشاف)
  D4  GPIO2  ← BTN_WATER    (زرار مياه)
  D8  GPIO15 ← BTN_BASIN    (زرار حوض) ⚠️ external 10K pull-down للبوت
  D0  GPIO16 ← BTN_SUCTION  (سويتش ساكشن الخاص)
  
POSITION BUTTONS (ESP8266-12F فقط):
  GPIO9     ← BTN_POS1
  GPIO10    ← BTN_POS2

SERIAL TO ESP32:
  TX  GPIO1  → ESP32 RX (GPIO2)
  RX  GPIO3  ← ESP32 TX (GPIO4)
```

---

## 5. توصيل الريليهات للـ Base

```
12V DC Supply
      +
      │
   ┌──┴──┐
   │Relay│  → COM
   │Coil │  → NO  → Solenoid / Motor +
   └──┬──┘  → NC  (مش متوصل)
      │
   IN (Signal) ← ESP8266 GPIO (D1-D7)
   GND ← ESP8266 GND
```

**Circuit لكل ريليه:**
```
ESP8266 GPIO ─── 1K resistor ─── Base of NPN (2N2222)
                                    Collector ─── Relay Coil -
                                    Emitter  ─── GND
Relay Coil + ─── 12V
Flyback diode: parallel with coil, Cathode → 12V, Anode → Collector
```

---

## 6. توصيل السولينويدات (Assistant)

نفس الدائرة بس على 12V (أو 24V حسب السولينويد):
```
سولينويد مياه:
  COM  ← 12V+
  NO   → [Relay NO] → 12V+ 
  → وتر الكهربا الأرضي للسولينويد على GND

Arduino sketch logic: ACTIVE_LOW relay → OUT_ON = LOW
```

---

## 7. بوتنشيومتر الكرسي (ADC)

**Base — UD Pot (صعود/نزول):**
```
3.3V ─── Pin 1 (Pot)
          Pin 2 (Wiper) ─── A0 (ESP8266 Base)
GND  ─── Pin 3 (Pot)

ملاحظة: ESP8266 ADC = 0-1V فقط بالـ NodeMCU
         لو الـ Pot بتدي 0-3.3V → ضع voltage divider
         R1=3.3K من Wiper، R2=1K لـ GND
         ADC = R2/(R1+R2) × Vpot = max 0.75V (safe)
```

---

## 8. Foot Switch (4 سويتشات)

كل سويتش:
```
One side ─── GPIO (INPUT_PULLUP)
Other side ─── GND
Press = LOW signal
```

---

## 9. Assistant Panel Buttons

نفس توصيل الـ foot switch:
```
BTN_LED_PIN, BTN_WATER_PIN, BTN_SUCT_PIN:
  One side ─── GPIO (INPUT_PULLUP)  
  Other side ─── GND

BTN_BASIN_PIN (D8/GPIO15):
  ⚠️ GPIO15 يجب يكون LOW عند البوت
  One side ─── GPIO15
  Other side ─── GND
  External 10K pull-down دايماً متوصل
  Button press → HIGH (استخدم pullup=false في الكود)
```

---

## 10. Power Distribution

```
╔═══════════════════════════════════╗
║  Power Supply 12V DC              ║
║  (أو 24V حسب السولينويدات)        ║
╠═══════════════════════════════════╣
║  12V → Relay Coils (Base)         ║
║  12V → Solenoid Outputs (Assist)  ║
║  12V → Hydraulic Motor            ║
╠═══════════════════════════════════╣
║  12V → DC-DC Buck → 5V            ║
║  5V  → ESP32 VIN                  ║
║  5V  → ESP8266 VIN ×2             ║
╠═══════════════════════════════════╣
║  GND مشترك بين الكل               ║
╚═══════════════════════════════════╝
```

---

## 11. Baud Rate Summary

| Connection        | Baud   |
|------------------|--------|
| ESP32 ↔ Base     | 115200 |
| ESP32 ↔ Assist   | 115200 |
| ESP32 → PC Debug | 115200 |
| ESP8266 → PC Debug | مش ممكن (Serial مشغول بالـ ESP32) |

---

## 12. خطوات الـ Programming

1. **ESP8266 Base** أولاً — Upload `ESP8266_Base.ino`
2. **ESP8266 Assistant** — Upload `ESP8266_Assistant.ino`
3. **ESP32 Master** أخيراً — Upload `DentalChairV7.ino`
4. افتح Serial Monitor على ESP32 @ 115200
5. تأكد تشوف `[Comm] Base ONLINE` و `[Comm] Assistant ONLINE`

---

## 13. اختبار أولي بدون توصيل الريليهات

1. وصّل الـ Serial فقط بين الـ 3 boards
2. شوف Serial Monitor على ESP32
3. اضغط أي زرار في الـ Keypad
4. تأكد الـ packets بتتبعت ويجي ACK

**Expected output:**
```
[Comm→B] [B:UP_ON:0]
[Comm←B] [ACK:UP_ON]
[Comm←B] [M:ADC_UD:1024]
[Comm←A] [M:PONG:0]
```
