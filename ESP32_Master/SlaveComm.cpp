// ════════════════════════════════════════════════
//  SlaveComm.cpp  —  Serial Communication Manager v7.0
// ════════════════════════════════════════════════

#include "SlaveComm.h"
#include "Display.h"

SlaveComm Comm;

// ──────────────────────────────────────────────
void SlaveComm::begin(SlaveCallback cb) {
    _cb = cb;

    Serial1.begin(BASE_BAUD,  SERIAL_8N1, BASE_RX,  BASE_TX);
    Serial2.begin(ASST_BAUD,  SERIAL_8N1, ASST_RX,  ASST_TX);

    _base.serial   = &Serial1;
    _base.id       = DST_BASE;
    _assist.serial = &Serial2;
    _assist.id     = DST_ASSISTANT;

    Serial.println("[Comm] SlaveComm started");
}

// ──────────────────────────────────────────────
//  _buildPacket  →  "[DST:CMD:VAL]\n"
// ──────────────────────────────────────────────
void SlaveComm::_buildPacket(char *out, char dst, const char *cmd, int32_t val) {
    snprintf(out, PKT_MAXLEN, "[%c:%s:%ld]\n", dst, cmd, (long)val);
}

// ──────────────────────────────────────────────
//  sendToBase / sendToAssist  —  enqueue
// ──────────────────────────────────────────────
// void SlaveComm::sendToBase(const char *cmd, int32_t val) {
//     char pkt[PKT_MAXLEN + 1];
//     _buildPacket(pkt, DST_BASE, cmd, val);
//     _base.enqueue(pkt);
// }
// void SlaveComm::sendToAssist(const char *cmd, int32_t val) {
//     char pkt[PKT_MAXLEN + 1];
//     _buildPacket(pkt, DST_ASSISTANT, cmd, val);
//     _assist.enqueue(pkt);
// }


void SlaveComm::sendToBase(const char *cmd, uint16_t freq, uint16_t duration) {
    char packet[40];
    snprintf(packet, sizeof(packet),
             "[B:%s:%u,%u]\n",
             cmd, freq, duration);

    BaseSerial.print(packet);
}

void SlaveComm::sendToAssist(const char *cmd, uint16_t freq, uint16_t duration) {
    char packet[40];
    snprintf(packet, sizeof(packet),
             "[A:%s:%u,%u]\n",
             cmd, freq, duration);

    AssistSerial.print(packet);
}

// ──────────────────────────────────────────────
//  sendToBaseNow / sendToAssistNow  —  immediate (motor-critical)
// ──────────────────────────────────────────────
void SlaveComm::sendToBaseNow(const char *cmd, int32_t val) {
    char pkt[PKT_MAXLEN + 1];
    _buildPacket(pkt, DST_BASE, cmd, val);
    _sendRaw(_base, pkt);
}
void SlaveComm::sendToAssistNow(const char *cmd, int32_t val) {
    char pkt[PKT_MAXLEN + 1];
    _buildPacket(pkt, DST_ASSISTANT, cmd, val);
    _sendRaw(_assist, pkt);
}

// ──────────────────────────────────────────────
//  _sendRaw — actually write to serial
// ──────────────────────────────────────────────
void SlaveComm::_sendRaw(SlaveState &s, const char *pkt) {
    s.serial->print(pkt);
    strncpy(s.lastSent, pkt, PKT_MAXLEN);
    s.waitingAck = true;
    s.ackSentMs  = millis();
    Serial.print("[Comm→"); Serial.print(s.id); Serial.print("] "); Serial.print(pkt);
}

// ──────────────────────────────────────────────
//  update() — main tick
// ──────────────────────────────────────────────
void SlaveComm::update() {
    _processRx(_base);
    _processRx(_assist);
    _tickQueue(_base);
    _tickQueue(_assist);
    _pingSlaves();

    // Update online status
    uint32_t now = millis();
    bool bOnline = (now - _base.lastContact   < OFFLINE_MS);
    bool aOnline = (now - _assist.lastContact < OFFLINE_MS);

    if (_baseOnline != bOnline) {
        _baseOnline = bOnline;
        notif.baseOnline = bOnline;
        Display.markDirty();
        Serial.print("[Comm] Base "); Serial.println(bOnline ? "ONLINE" : "OFFLINE");
    }
    if (_assistOnline != aOnline) {
        _assistOnline = aOnline;
        notif.assistOnline = aOnline;
        Display.markDirty();
        Serial.print("[Comm] Assistant "); Serial.println(aOnline ? "ONLINE" : "OFFLINE");
    }
}

// ──────────────────────────────────────────────
//  _processRx — read bytes and parse packets
// ──────────────────────────────────────────────
void SlaveComm::_processRx(SlaveState &s) {
    while (s.serial->available()) {
        char c = s.serial->read();
        if (c == '\n') {
            s.rxBuf[s.rxLen] = '\0';
            if (s.rxLen > 0) _processPacket(s, s.rxBuf);
            s.rxLen = 0;
        } else if (c == '[' ) {
            s.rxLen = 0;
            s.rxBuf[s.rxLen++] = c;
        } else if (s.rxLen > 0 && s.rxLen < PKT_MAXLEN) {
            s.rxBuf[s.rxLen++] = c;
        }
    }
}

// ──────────────────────────────────────────────
//  _processPacket — parse "[DST:CMD:VAL]" or "[ACK:CMD]"
// ──────────────────────────────────────────────
void SlaveComm::_processPacket(SlaveState &s, const char *pkt) {
    Serial.print("[Comm←"); Serial.print(s.id); Serial.print("] "); Serial.println(pkt);

    s.lastContact = millis();

    // Remove surrounding [ ]
    if (pkt[0] != '[') return;
    char buf[PKT_MAXLEN + 1];
    strncpy(buf, pkt + 1, PKT_MAXLEN);
    // Remove trailing ]
    char *end = strrchr(buf, ']');
    if (end) *end = '\0';

    // Split by ':'
    char *part1 = strtok(buf,  ":");
    char *part2 = strtok(nullptr, ":");
    char *part3 = strtok(nullptr, ":");
    if (!part1 || !part2) return;

    // ACK handling
    if (strcmp(part1, CMD_ACK) == 0) {
        s.waitingAck = false;
        s.retries    = 0;
        return;
    }

    // PONG (response to ping)
    if (strcmp(part2, CMD_PONG) == 0) {
        s.waitingAck = false;
        return;
    }

    // DST:CMD:VAL — forward to callback
    char dst = part1[0];
    const char *cmd = part2;
    int32_t val = part3 ? atol(part3) : 0;

    // Send ACK back
    char ack[PKT_MAXLEN + 1];
    snprintf(ack, sizeof(ack), "[%s:%s]\n", CMD_ACK, cmd);
    s.serial->print(ack);

    if (_cb) _cb(dst, cmd, val);
}

// ──────────────────────────────────────────────
//  _tickQueue — send next queued packet when free
// ──────────────────────────────────────────────
void SlaveComm::_tickQueue(SlaveState &s) {
    if (s.waitingAck) {
        // Check timeout
        if (millis() - s.ackSentMs >= SLAVE_TIMEOUT_MS) {
            if (s.retries < MAX_RETRIES) {
                s.retries++;
                s.serial->print(s.lastSent);
                s.ackSentMs = millis();
                Serial.print("[Comm] Retry "); Serial.print(s.retries);
                Serial.print(" → "); Serial.print(s.id); Serial.println();
            } else {
                // Give up on this packet
                s.waitingAck = false;
                s.retries    = 0;
                Serial.print("[Comm] GIVE UP → "); Serial.println(s.id);
            }
        }
        return;
    }

    if (!s.qEmpty()) {
        const char *pkt = s.dequeue();
        if (pkt) _sendRaw(s, pkt);
    }
}

// ──────────────────────────────────────────────
//  _pingSlaves — sends PING every PING_INTERVAL_MS
// ──────────────────────────────────────────────
void SlaveComm::_pingSlaves() {
    static uint32_t lastPing = 0;
    if (!elapsed(lastPing, PING_INTERVAL_MS)) return;

    char pkt[PKT_MAXLEN + 1];
    _buildPacket(pkt, DST_BASE, CMD_PING, 0);
    _base.serial->print(pkt);

    _buildPacket(pkt, DST_ASSISTANT, CMD_PING, 0);
    _assist.serial->print(pkt);
}
