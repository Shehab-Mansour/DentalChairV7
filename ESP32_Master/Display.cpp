// ════════════════════════════════════════════════
//  Display.cpp  —  OLED SH1106  UI Engine  v5.0
// ════════════════════════════════════════════════

#include "Display.h"
#include "ChairControl.h"
#include "TimeManager.h"
#include "WiFiManager.h"

DisplayManager Display;

static const uint8_t PROGMEM ICO_WIFI[]  = {0b00111100,0b01000010,0b10011001,0b00100100,0b01000010,0b00011000,0b00000000,0b00011000};
static const uint8_t PROGMEM ICO_LED[]   = {0b00111100,0b01000010,0b10000001,0b10000001,0b01000010,0b00111100,0b00011000,0b00111100};
static const uint8_t PROGMEM ICO_WATER[] = {0b00010000,0b00010000,0b00111000,0b01111100,0b11111110,0b11111110,0b01111100,0b00111000};
static const uint8_t PROGMEM ICO_BASIN[] = {0b11111110,0b10000010,0b10000010,0b11111110,0b00010000,0b00010000,0b00111000,0b01111100};
static const uint8_t PROGMEM ICO_CHAIR[] = {0b11110000,0b10010000,0b10010000,0b11110000,0b00001000,0b00001100,0b00001010,0b00001001};
static const uint8_t PROGMEM ICO_CLOCK[] = {0b00111100,0b01000010,0b10010001,0b10010001,0b10000001,0b01000010,0b00111100,0b00000000};

// ── Shared keyboard tables ──
static const char KB_LOWER[4][10] = {
    {'q','w','e','r','t','y','u','i','o','p'},{'a','s','d','f','g','h','j','k','l',';'},
    {'z','x','c','v','b','n','m',',','.','_'},{' ',' ',' ',' ',' ',' ',' ',' ',' ',' '}
};
static const char KB_UPPER[4][10] = {
    {'Q','W','E','R','T','Y','U','I','O','P'},{'A','S','D','F','G','H','J','K','L',';'},
    {'Z','X','C','V','B','N','M',',','.','_'},{' ',' ',' ',' ',' ',' ',' ',' ',' ',' '}
};
static const char KB_SYM[4][10] = {
    {'1','2','3','4','5','6','7','8','9','0'},{'!','@','#','$','%','^','&','*','(',')'},
    {'-','+','=','[',']','{','}','\\','/','\"'},{'\'','`','~','<','>','?',':',' ',' ',' '}
};
char kbGetChar(bool shift, bool symPage, uint8_t r, uint8_t c) {
    if (r >= 4 || c >= 10) return 0;
    if (symPage) return KB_SYM[r][c];
    return shift ? KB_UPPER[r][c] : KB_LOWER[r][c];
}

// ──────────────────────────────────────────────
void DisplayManager::begin() {
    _oled.begin(OLED_ADDR, true);
    _oled.setContrast(200);
    _oled.clearDisplay();
    _oled.display();
}

void DisplayManager::update() {
    static uint32_t lastRefresh = 0;
    uint32_t now = millis();
    if (_popup.active && now >= _popup.untilMs) { _popup.active = false; _dirty = true; }
    if (!_dirty && (now - lastRefresh) < DISPLAY_REFRESH_MS) return;
    lastRefresh = now; _dirty = false;
    _oled.clearDisplay();
    switch (_screen) {
        case SCREEN_HOME:          _drawHome();         break;
        case SCREEN_MENU:          _drawMenu();         break;
        case SCREEN_WIFI_SCAN:     _drawWifiScan();     break;
        case SCREEN_WIFI_PASS:     _drawWifiPass();     break;
        case SCREEN_WIFI_DETAIL:   _drawWifiDetail();   break;
        case SCREEN_WIFI_SAVED:    _drawWifiSaved();    break;
        case SCREEN_WIFI_SETTINGS: _drawWifiSettings(); break;
        case SCREEN_WATER_TIME:
        case SCREEN_BASIN_TIME:
        case SCREEN_SUCTION_TIME:  // v7
        case SCREEN_HOLD_TIME:     _drawTimeEditor();   break;
        case SCREEN_CLOCK_SET:     _drawClockSet();     break;
        case SCREEN_DATE_SET:      _drawDateSet();      break;
        case SCREEN_POSITIONS:     _drawPositions();    break;
        case SCREEN_LIMITS:        _drawLimits();       break;
        case SCREEN_SLAVE_STATUS:  _drawSlaveStatus();  break;
        default: break;
    }
    if (_popup.active) _drawPopup();
    _oled.display();
}

void DisplayManager::setScreen(Screen s) { _screen = s; _dirty = true; }
void DisplayManager::showPopup(const char *msg, uint16_t durationMs) {
    strncpy(_popup.msg, msg, 21); _popup.msg[21] = '\0';
    _popup.untilMs = millis() + durationMs; _popup.active = true; _dirty = true;
}
void DisplayManager::setProgress(uint8_t pct, bool visible) {
    _progressPct = pct; _progressShow = visible; _dirty = true;
}
void DisplayManager::setKbState(const char *pass, uint8_t passLen,
                                uint8_t row, uint8_t col, bool shift) {
    uint8_t n = min(passLen, PASS_MAXLEN);
    memcpy(_kbPass, pass, n); _kbPass[n] = '\0';
    _kbPassLen = n; _kbRow = row; _kbCol = col; _kbShift = shift; _dirty = true;
}
void DisplayManager::setEditValue(uint32_t ms, const char *title) {
    _editValue = ms; strncpy(_editTitle, title, 19); _editTitle[19] = '\0'; _dirty = true;
}
void DisplayManager::setClockEditState(uint8_t field, int8_t h, uint8_t m,
                                       bool use24h, int8_t tz, bool autoSync) {
    _clkField = field; _clkH = h; _clkM = m;
    _clk24h = use24h; _clkTZ = tz; _clkAutoSync = autoSync; _dirty = true;
}
void DisplayManager::setDateEditState(uint8_t field, uint16_t year, uint8_t month, uint8_t day) {
    _dtField = field; _dtYear = year; _dtMonth = month; _dtDay = day; _dirty = true;
}
void DisplayManager::setWifiDetailState(const char *ssid, const char *ip,
                                        const char *pass, uint8_t selItem, bool passVisible) {
    strncpy(_wdSSID, ssid, 32); _wdSSID[32] = '\0';
    strncpy(_wdIP,   ip,   15); _wdIP[15]   = '\0';
    strncpy(_wdPass, pass, 63); _wdPass[63] = '\0';
    _wdSel = selItem; _wdPassVisible = passVisible; _dirty = true;
}
void DisplayManager::setWifiSavedState(uint8_t selIdx, uint8_t total) {
    _wsSel = selIdx; _wsTotal = total; _dirty = true;
}
void DisplayManager::setWifiSettingsState(uint8_t selItem, bool autoSync) {
    _wstSel = selItem; _wstAutoSync = autoSync; _dirty = true;
}

// ── Utility ──────────────────────────────────
void DisplayManager::_drawProgressBar(uint8_t x,uint8_t y,uint8_t w,uint8_t h,uint8_t pct) {
    _oled.drawRect(x,y,w,h,SH110X_WHITE);
    uint8_t fill=(uint8_t)((uint32_t)pct*(w-2)/100);
    if(fill>0) _oled.fillRect(x+1,y+1,fill,h-2,SH110X_WHITE);
}

// ── Notification bar ─────────────────────────
void DisplayManager::_drawNotifBar() {
    _oled.drawFastHLine(0,10,OLED_W,SH110X_WHITE);
    uint8_t x=1;
    auto icon=[&](const uint8_t *bmp){ _oled.drawBitmap(x,1,bmp,8,8,SH110X_WHITE); x+=10; };
    if(notif.wifiConnected) icon(ICO_WIFI);
    if(notif.ledOn)         icon(ICO_LED);
    if(notif.waterOn)       icon(ICO_WATER);
    if(notif.basinOn)       icon(ICO_BASIN);
    if(notif.suctionOn)     icon(ICO_WATER);  // reuse water icon for suction
    if(notif.chairMoving)   icon(ICO_CHAIR);
    if(notif.timeSynced)    icon(ICO_CLOCK);
    // Base/Assist offline indicator
    if(!notif.baseOnline || !notif.assistOnline) {
        _oled.setTextColor(SH110X_WHITE);
        _oled.setCursor(x, 1);
        _oled.print(!notif.baseOnline ? "!B" : "!A");
        x += 14;
    }
    _oled.setTextSize(1); _oled.setTextColor(SH110X_WHITE);
    if      (gAppState==APP_AUTO_MOVING){_oled.setCursor(96,1);_oled.print("AUTO");}
    else if (gAppState==APP_WIFI_CONN)  {_oled.setCursor(90,1);_oled.print("WIFI..");}
    else if (gAppState==APP_ERROR)      {_oled.setCursor(102,1);_oled.print("ERR");}
}

// ── HOME ─────────────────────────────────────
void DisplayManager::_drawHome() {
    _drawNotifBar();
    _oled.setTextSize(2); _oled.setTextColor(SH110X_WHITE);
    uint8_t tlen=strlen(gTimeStr),tw=tlen*12;
    _oled.setCursor((OLED_W-tw)/2,13); _oled.print(gTimeStr);
    _oled.setTextSize(1);
    uint8_t dlen=strlen(gDateStr),dw=dlen*6;
    _oled.setCursor((OLED_W-dw)/2,33); _oled.print(gDateStr);
    if(_progressShow){
        _drawProgressBar(0,42,OLED_W,6,_progressPct);
        _oled.setCursor(0,50); _oled.print("Moving...");
    } else {
        _oled.setCursor(0,45); _oled.print("2^8v 4<6> A:LED B:H2O");
        _oled.setCursor(0,55); _oled.print("C:BSN D:SCT 1-9 *:MENU");
    }
}

// ── MENU (7 items) ───────────────────────────
static const char * const MENU_ITEMS[] = {
    "1. WiFi Settings","2. Water Timer","3. Basin Timer",
    "4. Suction Timer","5. Hold Threshold","6. Set Time",
    "7. Set Date","8. Chair Positions","9. Safety Limits","0. Slave Status",
};
static constexpr uint8_t MENU_COUNT = 10;

void DisplayManager::_drawMenu() {
    _drawNotifBar();
    _oled.setTextSize(1); _oled.setTextColor(SH110X_WHITE);
    _oled.setCursor(28,12); _oled.print("=== MENU ===");
    uint8_t startI=(_menuIdx>=4)?_menuIdx-3:0;
    for(uint8_t i=0;i<4&&(startI+i)<MENU_COUNT;i++){
        uint8_t idx=startI+i,y=22+i*10;
        if(idx==_menuIdx){_oled.fillRect(0,y-1,OLED_W,10,SH110X_WHITE);_oled.setTextColor(SH110X_BLACK);}
        else _oled.setTextColor(SH110X_WHITE);
        _oled.setCursor(2,y); _oled.print(MENU_ITEMS[idx]);
    }
    if(MENU_COUNT>4){
        uint8_t barH=40*4/MENU_COUNT,barY=22+(40*startI/MENU_COUNT);
        _oled.drawFastVLine(OLED_W-1,22,40,SH110X_WHITE);
        _oled.fillRect(OLED_W-2,barY,2,barH,SH110X_WHITE);
    }
    _oled.setTextColor(SH110X_WHITE);
    _oled.setCursor(0,56); _oled.print("2/8:Nav  0:OK  *:Home");
}

// ── WIFI SCAN ────────────────────────────────
void DisplayManager::_drawWifiScan() {
    _oled.setTextSize(1); _oled.setTextColor(SH110X_WHITE);
    // Header: show current connection if any
    if(WifiManager.isConnected()){
        char hdr[22]; snprintf(hdr,22,"Cur: %.14s",WifiManager.currentSSID());
        _oled.setCursor(0,0); _oled.print(hdr);
    } else {
        _oled.setCursor(0,0); _oled.print("WiFi Networks:");
    }
    _oled.drawFastHLine(0,9,OLED_W,SH110X_WHITE);
    if(gWifiCount==0){
        _oled.setCursor(15,25); _oled.print("Scanning...");
        uint8_t dot=(millis()/400)%4;
        for(uint8_t d=0;d<dot;d++) _oled.fillCircle(50+d*8,38,2,SH110X_WHITE);
        _dirty=true; return;
    }
    uint8_t startIdx=(_listIdx>=4)?_listIdx-3:0;
    for(uint8_t i=0;i<4;i++){
        uint8_t idx=startIdx+i;
        if(idx>=gWifiCount) break;
        uint8_t y=11+i*11;
        bool isCur=(strcmp(gWifiNetworks[idx],WifiManager.currentSSID())==0);
        if(idx==_listIdx){_oled.fillRect(0,y,OLED_W-10,11,SH110X_WHITE);_oled.setTextColor(SH110X_BLACK);}
        else _oled.setTextColor(SH110X_WHITE);
        _oled.setCursor(2,y+2);
        char buf[22]; snprintf(buf,22,"%.16s%s",gWifiNetworks[idx],isCur?" *":"");
        _oled.print(buf);
    }
    _oled.setTextColor(SH110X_WHITE);
    _oled.setCursor(0,56); _oled.print("0:Conn A:Detail *:Back");
}

// ── WIFI PASS ────────────────────────────────
void DisplayManager::_drawWifiPass() {
    _oled.setTextSize(1); _oled.setTextColor(SH110X_WHITE);
    char disp[20]={};
    if(_kbPassLen>15){ disp[0]='>'; memcpy(disp+1,_kbPass+_kbPassLen-14,14); disp[15]='|'; disp[16]='\0'; }
    else { memcpy(disp,_kbPass,_kbPassLen); disp[_kbPassLen]='|'; disp[_kbPassLen+1]='\0'; }
    _oled.setCursor(0,0); _oled.print(disp);
    _oled.drawFastHLine(0,9,OLED_W,SH110X_WHITE);
    bool symPage=(_kbCol>=10); uint8_t col=symPage?_kbCol-10:_kbCol;
    for(uint8_t r=0;r<3;r++) for(uint8_t c=0;c<10;c++){
        uint8_t px=c*12+1,py=11+r*12;
        char ch=kbGetChar(_kbShift,symPage,r,c); if(!ch||ch==' ') continue;
        bool sel=(r==_kbRow&&c==col);
        if(sel){_oled.fillRect(px-1,py-1,11,10,SH110X_WHITE);_oled.setTextColor(SH110X_BLACK);}
        else _oled.setTextColor(SH110X_WHITE);
        _oled.setCursor(px,py); _oled.print(ch);
    }
    _oled.setTextColor(SH110X_WHITE);
    uint8_t py3=11+3*12;
    // 6 أزرار مستقلة: SPC abc ABC 123 DEL OK
    const char *labels[6]={"SPC","abc","ABC","123","DEL","OK"};
    uint8_t xPos[6]={1,22,43,64,85,107},wid[6]={19,19,19,19,19,19};
    for(uint8_t b=0;b<5;b++){
        bool sel=(_kbRow==3&&col==b);
        if(sel){_oled.fillRect(xPos[b],py3-1,wid[b],10,SH110X_WHITE);_oled.setTextColor(SH110X_BLACK);}
        else _oled.setTextColor(SH110X_WHITE);
        _oled.setCursor(xPos[b]+1,py3); _oled.print(labels[b]);
    }
    _oled.setTextColor(SH110X_WHITE);
    _oled.setCursor(0,56); _oled.print("2468:Mv 0:Sel #:Send");
}

// ── WIFI DETAIL (v5) ─────────────────────────
void DisplayManager::_drawWifiDetail() {
    _oled.setTextSize(1); _oled.setTextColor(SH110X_WHITE);
    _oled.setCursor(0,0); _oled.print("WiFi Detail");
    _oled.drawFastHLine(0,9,OLED_W,SH110X_WHITE);
    char ssidLine[22]; snprintf(ssidLine,22,"SSID: %.15s",_wdSSID);
    _oled.setCursor(0,11); _oled.print(ssidLine);
    char ipLine[22];   snprintf(ipLine,22,"IP: %.16s",_wdIP);
    _oled.setCursor(0,20); _oled.print(ipLine);
    // Password row
    {
        bool sel=(_wdSel==0);
        if(sel){_oled.fillRect(0,29,OLED_W,10,SH110X_WHITE);_oled.setTextColor(SH110X_BLACK);}
        else _oled.setTextColor(SH110X_WHITE);
        char passLine[22];
        if(_wdPassVisible){ snprintf(passLine,22,"Pass: %.14s",_wdPass); }
        else {
            char stars[15]={}; uint8_t n=min((uint8_t)strlen(_wdPass),(uint8_t)14);
            for(uint8_t i=0;i<n;i++) stars[i]='*';
            snprintf(passLine,22,"Pass: %s",stars);
        }
        _oled.setCursor(0,30); _oled.print(passLine);
    }
    _oled.setTextColor(SH110X_WHITE);
    const char *actions[]={"Show/Hide Pass","Edit Password","Disconnect"};
    for(uint8_t i=0;i<3;i++){
        uint8_t y=41+i*8;
        bool sel=(_wdSel==(i+1));
        if(sel){_oled.fillRect(0,y,OLED_W,8,SH110X_WHITE);_oled.setTextColor(SH110X_BLACK);}
        else _oled.setTextColor(SH110X_WHITE);
        _oled.setCursor(2,y); _oled.print(actions[i]);
    }
    _oled.setTextColor(SH110X_WHITE);
    _oled.setCursor(0,57); _oled.print("2/8:Nav 0:OK *:Back");
}

// ── WIFI SAVED (v5) ──────────────────────────
void DisplayManager::_drawWifiSaved() {
    _oled.setTextSize(1); _oled.setTextColor(SH110X_WHITE);
    _oled.setCursor(0,0); _oled.print("Saved Networks:");
    _oled.drawFastHLine(0,9,OLED_W,SH110X_WHITE);
    if(_wsTotal==0){
        _oled.setCursor(10,28); _oled.print("No saved networks");
        _oled.setCursor(0,56);  _oled.print("*:Back"); return;
    }
    uint8_t startI=(_wsSel>=4)?_wsSel-3:0;
    for(uint8_t i=0;i<4&&(startI+i)<_wsTotal;i++){
        uint8_t idx=startI+i;
        const SavedNetwork *net=WifiManager.getSaved(idx);
        if(!net) continue;
        uint8_t y=11+i*12;
        bool isCur=(strcmp(net->ssid,WifiManager.currentSSID())==0);
        if(idx==_wsSel){_oled.fillRect(0,y,OLED_W,12,SH110X_WHITE);_oled.setTextColor(SH110X_BLACK);}
        else _oled.setTextColor(SH110X_WHITE);
        _oled.setCursor(2,y+2);
        char buf[24]; snprintf(buf,24,"%.16s%s",net->ssid,isCur?" [ON]":"");
        _oled.print(buf);
    }
    _oled.setTextColor(SH110X_WHITE);
    _oled.setCursor(0,56); _oled.print("2/8:Nav  0:Del  *:Back");
}

// ── WIFI SETTINGS (v5) ───────────────────────
void DisplayManager::_drawWifiSettings() {
    _oled.setTextSize(1); _oled.setTextColor(SH110X_WHITE);
    _oled.setCursor(0,0); _oled.print("WiFi Settings");
    _oled.drawFastHLine(0,9,OLED_W,SH110X_WHITE);
    struct Item { const char *label; bool hasVal; const char *val; };
    char connBuf[22]; char savedBuf[16];
    if(WifiManager.isConnected()) snprintf(connBuf,22,"%.14s [OK]",WifiManager.currentSSID());
    else strncpy(connBuf,"Not Connected",22);
    snprintf(savedBuf,16,"Saved (%d)",WifiManager.getSavedCount());
    const char *labels[4]={
        _wstAutoSync?"AutoSync: ON ":"AutoSync: OFF",
        "Scan Networks", savedBuf, connBuf
    };
    for(uint8_t i=0;i<4;i++){
        uint8_t y=11+i*12;
        bool sel=(_wstSel==i);
        if(sel){_oled.fillRect(0,y,OLED_W,12,SH110X_WHITE);_oled.setTextColor(SH110X_BLACK);}
        else _oled.setTextColor(SH110X_WHITE);
        _oled.setCursor(2,y+2); _oled.print(labels[i]);
    }
    _oled.setTextColor(SH110X_WHITE);
    _oled.setCursor(0,57); _oled.print("2/8:Nav  0:OK  *:Back");
}

// ── TIME EDITOR ──────────────────────────────
void DisplayManager::_drawTimeEditor() {
    _oled.setTextSize(1); _oled.setTextColor(SH110X_WHITE);
    _oled.setCursor(0,0); _oled.print(_editTitle);
    _oled.drawFastHLine(0,9,OLED_W,SH110X_WHITE);
    char buf[14];
    if(_screen==SCREEN_HOLD_TIME) snprintf(buf,sizeof(buf),"%dms",(int)_editValue);
    else                          snprintf(buf,sizeof(buf),"%ds",(int)(_editValue/1000));
    _oled.setTextSize(3);
    uint8_t tw=strlen(buf)*18;
    _oled.setCursor((OLED_W-tw)/2,18); _oled.print(buf);
    _oled.setTextSize(1);
    uint8_t pct=0;
    if(_screen==SCREEN_HOLD_TIME) pct=(uint8_t)(_editValue*100/2000);
    else {
        uint32_t maxMs = (_screen==SCREEN_WATER_TIME) ? 30000 :
                         (_screen==SCREEN_SUCTION_TIME) ? 30000 : 60000;
        pct=(uint8_t)(_editValue*100/maxMs);
    }
    _drawProgressBar(10,44,108,5,pct);
    _oled.setCursor(0,56);
    if(_screen==SCREEN_HOLD_TIME) _oled.print("2:+50ms 8:-50ms 0:Save");
    else                          _oled.print("2:+1s  8:-1s  0:Save");
}

// ── CLOCK SET (v5: 5 fields + autoSync) ──────
void DisplayManager::_drawClockSet() {
    _oled.setTextSize(1); _oled.setTextColor(SH110X_WHITE);
    _oled.setCursor(0,0); _oled.print("Set Time");
    _oled.drawFastHLine(0,9,OLED_W,SH110X_WHITE);
    char hbuf[4],mbuf[4];
    snprintf(hbuf,4,"%02d",(int)(_clk24h?_clkH:(_clkH%12==0?12:_clkH%12)));
    snprintf(mbuf,4,"%02d",(int)_clkM);
    _oled.setTextSize(2);
    if(_clkField==0){_oled.fillRect(0,11,28,18,SH110X_WHITE);_oled.setTextColor(SH110X_BLACK);}
    else _oled.setTextColor(SH110X_WHITE);
    _oled.setCursor(2,12); _oled.print(hbuf);
    _oled.setTextColor(SH110X_WHITE); _oled.setCursor(30,12); _oled.print(":");
    if(_clkField==1){_oled.fillRect(38,11,28,18,SH110X_WHITE);_oled.setTextColor(SH110X_BLACK);}
    else _oled.setTextColor(SH110X_WHITE);
    _oled.setCursor(40,12); _oled.print(mbuf);
    _oled.setTextSize(1); _oled.setTextColor(SH110X_WHITE);
    if(!_clk24h){_oled.setCursor(70,18);_oled.print(_clkH>=12?"PM":"AM");}
    if(_clkField==2){_oled.fillRect(0,31,82,10,SH110X_WHITE);_oled.setTextColor(SH110X_BLACK);}
    else _oled.setTextColor(SH110X_WHITE);
    char tzBuf[14]; snprintf(tzBuf,14,"TZ: UTC%+d",(int)_clkTZ);
    _oled.setCursor(2,32); _oled.print(tzBuf);
    _oled.setTextColor(SH110X_WHITE);
    if(_clkField==3){_oled.fillRect(0,43,70,10,SH110X_WHITE);_oled.setTextColor(SH110X_BLACK);}
    else _oled.setTextColor(SH110X_WHITE);
    _oled.setCursor(2,44); _oled.print(_clk24h?"Format: 24h":"Format: 12h");
    _oled.setTextColor(SH110X_WHITE);
    if(_clkField==4){_oled.fillRect(0,54,OLED_W,10,SH110X_WHITE);_oled.setTextColor(SH110X_BLACK);}
    else _oled.setTextColor(SH110X_WHITE);
    _oled.setCursor(2,55); _oled.print(_clkAutoSync?"AutoSync NTP: ON ":"AutoSync NTP: OFF");
    _oled.setTextColor(SH110X_WHITE);
}

// ── DATE SET (v5 NEW) ────────────────────────
void DisplayManager::_drawDateSet() {
    _oled.setTextSize(1); _oled.setTextColor(SH110X_WHITE);
    _oled.setCursor(0,0); _oled.print("Set Date");
    _oled.drawFastHLine(0,9,OLED_W,SH110X_WHITE);
    _oled.setCursor(4,12);  _oled.print("Day");
    _oled.setCursor(50,12); _oled.print("Month");
    _oled.setCursor(100,12);_oled.print("Year");
    _oled.setTextSize(2);
    char dbuf[3],mbuf[3],ybuf[5];
    snprintf(dbuf,3,"%02d",(int)_dtDay);
    snprintf(mbuf,3,"%02d",(int)_dtMonth);
    snprintf(ybuf,5,"%04d",(int)_dtYear);
    if(_dtField==0){_oled.fillRect(0,22,30,18,SH110X_WHITE);_oled.setTextColor(SH110X_BLACK);}
    else _oled.setTextColor(SH110X_WHITE);
    _oled.setCursor(2,23); _oled.print(dbuf);
    _oled.setTextColor(SH110X_WHITE); _oled.setCursor(32,23); _oled.print("/");
    if(_dtField==1){_oled.fillRect(42,22,30,18,SH110X_WHITE);_oled.setTextColor(SH110X_BLACK);}
    else _oled.setTextColor(SH110X_WHITE);
    _oled.setCursor(44,23); _oled.print(mbuf);
    _oled.setTextColor(SH110X_WHITE); _oled.setCursor(74,23); _oled.print("/");
    if(_dtField==2){_oled.fillRect(82,22,46,18,SH110X_WHITE);_oled.setTextColor(SH110X_BLACK);}
    else _oled.setTextColor(SH110X_WHITE);
    _oled.setCursor(84,23); _oled.print(ybuf);
    _oled.setTextSize(1); _oled.setTextColor(SH110X_WHITE);
    // Day of week preview
    DateTime preview(_dtYear,_dtMonth,_dtDay,12,0,0);
    const char *DOW[]={"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
    char prev[22];
    snprintf(prev,22,"= %s %02d/%02d/%04d",DOW[preview.dayOfTheWeek()],_dtDay,_dtMonth,_dtYear);
    _oled.setCursor(0,44); _oled.print(prev);
    _oled.setCursor(0,54); _oled.print("6:Fld 2/8:+/- 0:Save");
}

// ── POSITIONS ────────────────────────────────
void DisplayManager::_drawPositions() {
    _oled.setTextSize(1); _oled.setTextColor(SH110X_WHITE);
    _oled.setCursor(0,0); _oled.print("Chair Positions:");
    _oled.drawFastHLine(0,9,OLED_W,SH110X_WHITE);
    uint8_t startI=(_posSlot>=4)?_posSlot-3:0;
    for(uint8_t i=0;i<4;i++){
        uint8_t idx=startI+i;
        if(idx>=MAX_POSITIONS) break;
        uint8_t y=11+i*12;
        if(idx==_posSlot){_oled.fillRect(0,y,OLED_W,12,SH110X_WHITE);_oled.setTextColor(SH110X_BLACK);}
        else _oled.setTextColor(SH110X_WHITE);
        _oled.setCursor(2,y+2);
        if(gPositions[idx].saved){
            char buf[28],lbl[7]; char hk=gPositions[idx].hotkey?('0'+gPositions[idx].hotkey):'-';
            strncpy(lbl,gPositions[idx].label,6); lbl[6]='\0';
            if(lbl[0]=='\0') snprintf(lbl,7,"P%d",idx+1);
            snprintf(buf,28,"%s [%c] U%d F%d",lbl,hk,gPositions[idx].updown/40,gPositions[idx].fwdback/40);
            _oled.print(buf);
        } else {
            char buf[18]; snprintf(buf,18,"P%d: -- Empty --",idx+1); _oled.print(buf);
        }
    }
    _oled.setTextColor(SH110X_WHITE);
    _oled.setCursor(0,56);
    if(_posEditing) _oled.print("Enter hotkey (1-9) *=X");
    else            _oled.print("0:Save #:Del A:Key *:Bk");
}

// ── POPUP ────────────────────────────────────
void DisplayManager::_drawPopup() {
    uint8_t msgLen=strlen(_popup.msg),bw=msgLen*6+8;
    if(bw>OLED_W-4) bw=OLED_W-4;
    uint8_t bx=(OLED_W-bw)/2,by=(OLED_H/2)-8;
    _oled.fillRect(bx-1,by-1,bw+2,18,SH110X_BLACK);
    _oled.drawRect(bx-1,by-1,bw+2,18,SH110X_WHITE);
    _oled.setTextSize(1); _oled.setTextColor(SH110X_WHITE);
    _oled.setCursor(bx+3,by+4); _oled.print(_popup.msg);
}

// ══════════════════════════════════════════════
//  LIMITS SCREEN (v7)
//  Items: 0=UP_MAX 1=UP_MIN 2=FWD_MAX 3=BACK_MAX 4=Clear All
// ══════════════════════════════════════════════
void DisplayManager::setLimitsState(uint8_t selItem, uint16_t ud, uint16_t fb) {
    _limSel = selItem; _limUD = ud; _limFB = fb; _dirty = true;
}

void DisplayManager::_drawLimits() {
    const ChairLimits &L = gSettings.limits;
    _oled.setTextSize(1); _oled.setTextColor(SH110X_WHITE);
    _oled.setCursor(0,0); _oled.print("Safety Limits Setup");
    _oled.drawFastHLine(0,9,OLED_W,SH110X_WHITE);

    // Current ADC values
    char cur[22];
    snprintf(cur,22,"UD:%4d  FB:%4d", _limUD, _limFB);
    _oled.setCursor(0,10); _oled.print(cur);
    _oled.drawFastHLine(0,19,OLED_W,SH110X_WHITE);

    struct LimRow { const char *label; bool set; uint16_t val; };
    LimRow rows[5] = {
        {"Up   MAX", L.udMaxSet, L.udMax},
        {"Down MIN", L.udMinSet, L.udMin},
        {"Fwd  MAX", L.fbMaxSet, L.fbMax},
        {"Back MIN", L.fbMinSet, L.fbMin},
        {"Clear ALL",false, 0},
    };

    for (uint8_t i = 0; i < 5; i++) {
        uint8_t y = 21 + i * 8;
        bool sel = (_limSel == i);
        if (sel) { _oled.fillRect(0,y,OLED_W,8,SH110X_WHITE); _oled.setTextColor(SH110X_BLACK); }
        else     _oled.setTextColor(SH110X_WHITE);
        char row[24];
        if (i < 4) {
            if (rows[i].set) snprintf(row,24,"%s: %4d", rows[i].label, rows[i].val);
            else             snprintf(row,24,"%s: ---",  rows[i].label);
        } else {
            snprintf(row,24,"[ Clear All Limits ]");
        }
        _oled.setCursor(2,y); _oled.print(row);
    }
    _oled.setTextColor(SH110X_WHITE);
    _oled.setCursor(0,62); _oled.print("0:Set Here  *:Back");
}

// ══════════════════════════════════════════════
//  SLAVE STATUS SCREEN (v7)
// ══════════════════════════════════════════════
void DisplayManager::setSlaveStatusState(bool baseOk, bool assistOk,
                                          uint16_t ud, uint16_t fb) {
    _slvBaseOk = baseOk; _slvAssOk = assistOk;
    _limUD = ud; _limFB = fb; _dirty = true;
}

void DisplayManager::_drawSlaveStatus() {
    _oled.setTextSize(1); _oled.setTextColor(SH110X_WHITE);
    _oled.setCursor(0,0); _oled.print("Slave Devices Status");
    _oled.drawFastHLine(0,9,OLED_W,SH110X_WHITE);

    // Base
    if (_slvBaseOk) {
        _oled.fillRect(0,11,OLED_W,12,SH110X_WHITE);
        _oled.setTextColor(SH110X_BLACK);
    } else _oled.setTextColor(SH110X_WHITE);
    _oled.setCursor(2,13);
    _oled.print(_slvBaseOk ? "Base:  ONLINE  (Chair)" : "Base:  OFFLINE!");

    _oled.setTextColor(SH110X_WHITE);
    if (_slvAssOk) {
        _oled.fillRect(0,24,OLED_W,12,SH110X_WHITE);
        _oled.setTextColor(SH110X_BLACK);
    } else _oled.setTextColor(SH110X_WHITE);
    _oled.setCursor(2,26);
    _oled.print(_slvAssOk ? "Asst:  ONLINE (Output)" : "Asst:  OFFLINE!");

    _oled.setTextColor(SH110X_WHITE);
    _oled.drawFastHLine(0,37,OLED_W,SH110X_WHITE);

    // ADC from Base
    char buf[22];
    snprintf(buf,22,"UD ADC : %4d / 4095", _limUD);
    _oled.setCursor(0,39); _oled.print(buf);
    snprintf(buf,22,"FB ADC : %4d / 4095", _limFB);
    _oled.setCursor(0,48); _oled.print(buf);

    _oled.setCursor(0,58); _oled.print("*:Back  0:Refresh");
}
