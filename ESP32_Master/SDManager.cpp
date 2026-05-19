// ════════════════════════════════════════════════
//  SDManager.cpp  —  SD Card persistence  v5.0
//  v5: added autoSyncTime to settings
// ════════════════════════════════════════════════

#include "SDManager.h"
#include "WiFiManager.h"

SDManager SDMgr;

bool SDManager::begin() {
    SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
    _ready = SD.begin(SD_CS, SPI, 4000000);
    Serial.println(_ready ? "[SD] Ready" : "[SD] FAILED");
    return _ready;
}

void SDManager::saveWifi(const SavedNetwork *nets, uint8_t count) {
    if (!_ready) return;
    SD.remove("/wifi.csv");
    File f = SD.open("/wifi.csv", FILE_WRITE);
    if (!f) return;
    for (uint8_t i = 0; i < count; i++) {
        f.print(nets[i].ssid); f.print(','); f.println(nets[i].pass);
    }
    f.close();
}

uint8_t SDManager::loadWifi(SavedNetwork *nets, uint8_t maxCount) {
    if (!_ready) return 0;
    File f = SD.open("/wifi.csv");
    if (!f) return 0;
    uint8_t count = 0;
    char line[100];
    while (f.available() && count < maxCount) {
        uint8_t len = 0;
        while (f.available() && len < 99) {
            char c = f.read(); if (c == '\n') break; if (c != '\r') line[len++] = c;
        }
        line[len] = '\0'; if (len == 0) continue;
        char *comma = strchr(line, ','); if (!comma) continue;
        uint8_t ssidLen = (uint8_t)(comma - line); if (ssidLen > 32) ssidLen = 32;
        memcpy(nets[count].ssid, line, ssidLen); nets[count].ssid[ssidLen] = '\0';
        strncpy(nets[count].pass, comma + 1, 63); nets[count].pass[63] = '\0';
        count++;
    }
    f.close(); return count;
}

void SDManager::saveSettings(const Settings &s) {
    if (!_ready) return;
    SD.remove("/settings.txt");
    File f = SD.open("/settings.txt", FILE_WRITE);
    if (!f) return;
    char buf[32];
    snprintf(buf, sizeof(buf), "waterMs=%lu\n",  (unsigned long)s.waterTimeMs);  f.print(buf);
    snprintf(buf, sizeof(buf), "basinMs=%lu\n",  (unsigned long)s.basinTimeMs);  f.print(buf);
    snprintf(buf, sizeof(buf), "suctMs=%lu\n",   (unsigned long)s.suctionTimeMs);f.print(buf);
    snprintf(buf, sizeof(buf), "holdMs=%u\n",    (unsigned)s.holdThresholdMs);   f.print(buf);
    snprintf(buf, sizeof(buf), "use24h=%d\n",    (int)s.use24h);                 f.print(buf);
    snprintf(buf, sizeof(buf), "tzOffset=%d\n",  (int)s.tzOffsetHours);          f.print(buf);
    snprintf(buf, sizeof(buf), "autoSync=%d\n",  (int)s.autoSyncTime);           f.print(buf);
    // Limits (v7)
    snprintf(buf, sizeof(buf), "udMax=%d\n",   (int)s.limits.udMax);    f.print(buf);
    snprintf(buf, sizeof(buf), "udMin=%d\n",   (int)s.limits.udMin);    f.print(buf);
    snprintf(buf, sizeof(buf), "fbMax=%d\n",   (int)s.limits.fbMax);    f.print(buf);
    snprintf(buf, sizeof(buf), "fbMin=%d\n",   (int)s.limits.fbMin);    f.print(buf);
    snprintf(buf, sizeof(buf), "udMaxSet=%d\n",(int)s.limits.udMaxSet); f.print(buf);
    snprintf(buf, sizeof(buf), "udMinSet=%d\n",(int)s.limits.udMinSet); f.print(buf);
    snprintf(buf, sizeof(buf), "fbMaxSet=%d\n",(int)s.limits.fbMaxSet); f.print(buf);
    snprintf(buf, sizeof(buf), "fbMinSet=%d\n",(int)s.limits.fbMinSet); f.print(buf);
    snprintf(buf, sizeof(buf), "aPos1=%d\n",   (int)s.assistPos1Slot);  f.print(buf);
    snprintf(buf, sizeof(buf), "aPos2=%d\n",   (int)s.assistPos2Slot);  f.print(buf);
    f.close();
}

bool SDManager::loadSettings(Settings &s) {
    if (!_ready) return false;
    File f = SD.open("/settings.txt");
    if (!f) return false;
    char line[40];
    while (f.available()) {
        uint8_t len = 0;
        while (f.available() && len < 39) {
            char c = f.read(); if (c == '\n') break; if (c != '\r') line[len++] = c;
        }
        line[len] = '\0'; if (len == 0) continue;
        char *eq = strchr(line, '='); if (!eq) continue;
        *eq = '\0'; const char *key = line, *val = eq + 1;
        if      (strcmp(key, "waterMs")  == 0) s.waterTimeMs     = (uint32_t)atol(val);
        else if (strcmp(key, "basinMs")  == 0) s.basinTimeMs     = (uint32_t)atol(val);
        else if (strcmp(key, "suctMs")   == 0) s.suctionTimeMs   = (uint32_t)atol(val);
        else if (strcmp(key, "holdMs")   == 0) s.holdThresholdMs = (uint16_t)atoi(val);
        else if (strcmp(key, "use24h")   == 0) s.use24h          = (atoi(val) != 0);
        else if (strcmp(key, "tzOffset") == 0) s.tzOffsetHours   = (int8_t)atoi(val);
        else if (strcmp(key, "autoSync") == 0) s.autoSyncTime    = (atoi(val) != 0);
        // Limits (v7)
        else if (strcmp(key, "udMax")    == 0) s.limits.udMax    = (uint16_t)atoi(val);
        else if (strcmp(key, "udMin")    == 0) s.limits.udMin    = (uint16_t)atoi(val);
        else if (strcmp(key, "fbMax")    == 0) s.limits.fbMax    = (uint16_t)atoi(val);
        else if (strcmp(key, "fbMin")    == 0) s.limits.fbMin    = (uint16_t)atoi(val);
        else if (strcmp(key, "udMaxSet") == 0) s.limits.udMaxSet = (atoi(val) != 0);
        else if (strcmp(key, "udMinSet") == 0) s.limits.udMinSet = (atoi(val) != 0);
        else if (strcmp(key, "fbMaxSet") == 0) s.limits.fbMaxSet = (atoi(val) != 0);
        else if (strcmp(key, "fbMinSet") == 0) s.limits.fbMinSet = (atoi(val) != 0);
        else if (strcmp(key, "aPos1")    == 0) s.assistPos1Slot  = (uint8_t)atoi(val);
        else if (strcmp(key, "aPos2")    == 0) s.assistPos2Slot  = (uint8_t)atoi(val);
    }
    f.close(); return true;
}

void SDManager::savePositions(const ChairPosition *pos, uint8_t count) {
    if (!_ready) return;
    SD.remove("/positions.txt");
    File f = SD.open("/positions.txt", FILE_WRITE);
    if (!f) return;
    char buf[48];
    for (uint8_t i = 0; i < count; i++) {
        if (!pos[i].saved) continue;
        snprintf(buf, sizeof(buf), "%d,%d,%d,%d,%s\n",
            (int)pos[i].slot, (int)pos[i].updown, (int)pos[i].fwdback,
            (int)pos[i].hotkey, pos[i].label);
        f.print(buf);
    }
    f.close();
}

bool SDManager::loadPositions(ChairPosition *pos, uint8_t count) {
    if (!_ready) return false;
    for (uint8_t i = 0; i < count; i++) {
        pos[i].saved = false; pos[i].slot = i+1; pos[i].hotkey = 0; pos[i].label[0] = '\0';
    }
    File f = SD.open("/positions.txt");
    if (!f) return false;
    char line[56];
    while (f.available()) {
        uint8_t len = 0;
        while (f.available() && len < 55) {
            char c = f.read(); if (c == '\n') break; if (c != '\r') line[len++] = c;
        }
        line[len] = '\0'; if (len == 0) continue;
        int slot, ud, fb, hk; char lbl[12] = {};
        int matched = sscanf(line, "%d,%d,%d,%d,%11[^\n]", &slot, &ud, &fb, &hk, lbl);
        if (matched < 4) continue;
        if (slot < 1 || slot > (int)count) continue;
        uint8_t idx = (uint8_t)(slot - 1);
        pos[idx].slot = (uint8_t)slot; pos[idx].updown = (uint16_t)ud;
        pos[idx].fwdback = (uint16_t)fb; pos[idx].hotkey = (uint8_t)hk;
        strncpy(pos[idx].label, lbl, 11); pos[idx].label[11] = '\0';
        pos[idx].saved = true;
    }
    f.close(); return true;
}
