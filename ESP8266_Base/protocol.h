// ════════════════════════════════════════════════
//  protocol.h  —  Shared Protocol v7.0
//  يُستخدم في الـ 3 MCUs بدون تعديل
//
//  Packet format:  [DST:CMD:VAL]\n
//  Example:        [B:UP_ON:0]\n
//                  [A:LED_ON:0]\n
//                  [M:ADC_UD:2048]\n
//
//  DST: B=Base  A=Assistant  M=Master
//  ACK: [ACK:CMD]\n
//  ERR: [ERR:CMD]\n
// ════════════════════════════════════════════════

#pragma once

// ── Destinations ─────────────────────────────
#define DST_MASTER    'M'
#define DST_BASE      'B'
#define DST_ASSISTANT 'A'

// ── Commands: Master → Base ───────────────────
// Chair movement
#define CMD_UP_ON       "UP_ON"
#define CMD_UP_OFF      "UP_OFF"
#define CMD_DOWN_ON     "DOWN_ON"
#define CMD_DOWN_OFF    "DOWN_OFF"
#define CMD_FWD_ON      "FWD_ON"
#define CMD_FWD_OFF     "FWD_OFF"
#define CMD_BACK_ON     "BACK_ON"
#define CMD_BACK_OFF    "BACK_OFF"
#define CMD_MOTOR_ON    "MOTOR_ON"
#define CMD_MOTOR_OFF   "MOTOR_OFF"
#define CMD_ALL_OFF     "ALL_OFF"
// Limits
#define CMD_SET_LIM_UD_MAX  "LIM_UD_MAX"
#define CMD_SET_LIM_UD_MIN  "LIM_UD_MIN"
#define CMD_SET_LIM_FB_MAX  "LIM_FB_MAX"
#define CMD_SET_LIM_FB_MIN  "LIM_FB_MIN"
#define CMD_GET_LIMITS      "GET_LIMITS"

// ── Commands: Master → Assistant ─────────────
#define CMD_LED_ON      "LED_ON"
#define CMD_LED_OFF     "LED_OFF"
#define CMD_WATER_ON    "WATER_ON"
#define CMD_WATER_OFF   "WATER_OFF"
#define CMD_BASIN_ON    "BASIN_ON"
#define CMD_BASIN_OFF   "BASIN_OFF"
#define CMD_SUCTION_ON  "SUCTION_ON"
#define CMD_SUCTION_OFF "SUCTION_OFF"
#define CMD_GOTO_POS    "GOTO_POS"    // VAL = slot number

// ── Commands: Base → Master (reports) ─────────
#define CMD_ADC_UD      "ADC_UD"      // VAL = 0-4095
#define CMD_ADC_FB      "ADC_FB"      // VAL = 0-4095
#define CMD_FOOT_UP     "FOOT_UP"     // foot switch pressed
#define CMD_FOOT_DOWN   "FOOT_DOWN"
#define CMD_FOOT_FWD    "FOOT_FWD"
#define CMD_FOOT_BACK   "FOOT_BACK"
#define CMD_FOOT_REL    "FOOT_REL"    // all foot released
#define CMD_LIMIT_HIT   "LIMIT_HIT"   // VAL = limit name

// ── Commands: Assistant → Master (reports) ────
#define CMD_BTN_LED     "BTN_LED"
#define CMD_BTN_WATER   "BTN_WATER"
#define CMD_BTN_BASIN   "BTN_BASIN"
#define CMD_BTN_SUCTION "BTN_SUCTION"
#define CMD_BTN_POS1    "BTN_POS1"
#define CMD_BTN_POS2    "BTN_POS2"
#define CMD_BTN_REL     "BTN_REL"     // button released

// ── ACK / ERR ─────────────────────────────────
#define CMD_ACK         "ACK"
#define CMD_ERR         "ERR"
#define CMD_PING        "PING"
#define CMD_PONG        "PONG"
#define CMD_BEEP        "BEEP"        // VAL = duration ms

// ── Packet limits ─────────────────────────────
#define PKT_MAXLEN  32   // max packet chars including \n
