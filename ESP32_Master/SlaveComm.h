#pragma once

// ════════════════════════════════════════════════
//  SlaveComm.h  —  Serial Communication Manager v7.0
//
//  يتحكم في الاتصال مع الـ 2 ESP8266:
//  • Base    (UART1: TX=17, RX=16)
//  • Assistant (UART2: TX=4, RX=2)
//
//  Protocol: [DST:CMD:VAL]\n
//  ACK:      [ACK:CMD]\n
//
//  Features:
//  • Non-blocking packet send
//  • ACK timeout + retry (3 times)
//  • Incoming packet callback
//  • Online/offline detection via ping
//  • Queue للرسائل المنتظرة
// ════════════════════════════════════════════════

#include "shard.h"
#include "protocol.h"

// ──────────────────────────────────────────────
//  Callback type: يُستدعى لما يجي حاجة من الـ slaves
// ──────────────────────────────────────────────
typedef void (*SlaveCallback)(char dst, const char *cmd, int32_t val);

// ──────────────────────────────────────────────
class SlaveComm {
public:
    void begin(SlaveCallback cb);
    void update();   // call every loop

    // // Send to specific slave (non-blocking, queued)
    // void sendToBase(const char *cmd, int32_t val = 0);
    // void sendToAssist(const char *cmd, int32_t val = 0);
    
    void sendToBase(const char *cmd, uint16_t freq, uint16_t duration);
    void sendToAssist(const char *cmd, uint16_t freq, uint16_t duration);

    // Immediate send (for time-critical motor commands)
    void sendToBaseNow(const char *cmd, int32_t val = 0);
    void sendToAssistNow(const char *cmd, int32_t val = 0);

    bool isBaseOnline()   const { return _baseOnline; }
    bool isAssistOnline() const { return _assistOnline; }


private:
    SlaveCallback _cb = nullptr;

    HardwareSerial *_baseSerial   = &Serial1;
    HardwareSerial *_assistSerial = &Serial2;

    // ── Per-slave state ────────────────────────
    struct SlaveState {
        char     rxBuf[PKT_MAXLEN + 1] = {};
        uint8_t  rxLen   = 0;
        bool     online  = false;
        uint32_t lastPing    = 0;
        uint32_t lastContact = 0;

        // Queue: 4-deep
        static constexpr uint8_t QSIZE = 4;
        char     queue[QSIZE][PKT_MAXLEN + 1] = {};
        uint8_t  qHead = 0, qTail = 0;
        bool     waitingAck  = false;
        uint32_t ackSentMs   = 0;
        uint8_t  retries     = 0;
        char     lastSent[PKT_MAXLEN + 1] = {};
        HardwareSerial *serial = nullptr;
        char     id = '?';

        bool qEmpty()  const { return qHead == qTail; }
        bool qFull()   const { return ((qTail + 1) % QSIZE) == qHead; }
        void enqueue(const char *pkt) {
            if (qFull()) return;
            strncpy(queue[qTail], pkt, PKT_MAXLEN);
            queue[qTail][PKT_MAXLEN] = '\0';
            qTail = (qTail + 1) % QSIZE;
        }
        const char *dequeue() {
            if (qEmpty()) return nullptr;
            const char *p = queue[qHead];
            qHead = (qHead + 1) % QSIZE;
            return p;
        }
    };

    SlaveState _base;
    SlaveState _assist;

    void _buildPacket(char *out, char dst, const char *cmd, int32_t val);
    void _processRx(SlaveState &s);
    void _processPacket(SlaveState &s, const char *pkt);
    void _tickQueue(SlaveState &s);
    void _sendRaw(SlaveState &s, const char *pkt);
    void _pingSlaves();

    bool _baseOnline   = false;
    bool _assistOnline = false;

    static constexpr uint32_t PING_INTERVAL_MS = 3000;
    static constexpr uint32_t OFFLINE_MS        = 8000;
    static constexpr uint8_t  MAX_RETRIES       = 3;
};

extern SlaveComm Comm;
