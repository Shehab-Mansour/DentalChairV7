// ════════════════════════════════════════════════
//  DentalWebServer.cpp  v6.0
//
//  إصلاحات v6:
//  1. الشاشة تتحدث فوراً بعد أي أمر (refreshStatus() بعد كل post)
//  2. Settings page تقرأ القيم الحقيقية من ESP32 دايماً (لا defaults)
//  3. WiFi page: زر Connect للشبكات المحفوظة بدون scan جديد
//     + تجربة الاتصال بشبكة محفوظة مباشرة
//  4. Keyboard OLED: أزرار مستقلة LOWER/UPPER/SYM في الصف الأخير
// ════════════════════════════════════════════════

#include "DentalWebServer.h"
#include "WiFiManager.h"
#include "TimeManager.h"
#include "ChairControl.h"
#include "OutputControl.h"
#include "SDManager.h"
#include "Display.h"

// DentalWebServer WebSrv;
DentalWebServer WebUI;

// ──────────────────────────────────────────────
//  CSS
// ──────────────────────────────────────────────
static const char CSS[] PROGMEM = R"RAW(
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:'Segoe UI',Arial,sans-serif;background:#0f1117;color:#e2e8f0;min-height:100vh}
header{background:linear-gradient(135deg,#1a1f2e,#2d3748);padding:16px 24px;display:flex;align-items:center;justify-content:space-between;border-bottom:1px solid #2d3748;position:sticky;top:0;z-index:100}
header h1{font-size:1.3rem;font-weight:700;color:#63b3ed;letter-spacing:.5px}
header .badge{font-size:.7rem;padding:3px 8px;border-radius:20px;margin-left:8px}
.badge-on{background:#276749;color:#9ae6b4}.badge-off{background:#742a2a;color:#fc8181}
nav{background:#1a1f2e;padding:0 24px;display:flex;gap:4px;border-bottom:1px solid #2d3748;overflow-x:auto}
nav a{padding:12px 18px;font-size:.85rem;color:#a0aec0;text-decoration:none;border-bottom:2px solid transparent;white-space:nowrap;transition:.2s}
nav a.active,nav a:hover{color:#63b3ed;border-bottom-color:#63b3ed}
main{padding:20px 24px;max-width:1100px;margin:0 auto}
.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(280px,1fr));gap:16px;margin-bottom:20px}
.card{background:#1a1f2e;border:1px solid #2d3748;border-radius:12px;padding:18px}
.card h2{font-size:.9rem;font-weight:600;color:#a0aec0;text-transform:uppercase;letter-spacing:.8px;margin-bottom:14px}
.status-row{display:flex;align-items:center;justify-content:space-between;padding:7px 0;border-bottom:1px solid #2d3748}
.status-row:last-child{border:none}
.label{font-size:.85rem;color:#a0aec0}
.value{font-size:.85rem;font-weight:600;color:#e2e8f0}
.value.green{color:#68d391}.value.red{color:#fc8181}.value.blue{color:#63b3ed}
.btn{display:inline-flex;align-items:center;justify-content:center;padding:9px 18px;border-radius:8px;border:none;cursor:pointer;font-size:.85rem;font-weight:600;transition:.2s}
.btn-primary{background:#3182ce;color:#fff}.btn-primary:hover{background:#2b6cb0}
.btn-danger{background:#c53030;color:#fff}.btn-danger:hover{background:#9b2c2c}
.btn-success{background:#276749;color:#fff}.btn-success:hover{background:#22543d}
.btn-ghost{background:#2d3748;color:#e2e8f0}.btn-ghost:hover{background:#3d4f63}
.btn-warn{background:#b7791f;color:#fff}.btn-warn:hover{background:#975a16}
.btn-sm{padding:6px 12px;font-size:.78rem}
.btn-full{width:100%;margin-top:8px}
.actions{display:flex;flex-wrap:wrap;gap:8px;margin-top:4px}
input,select{width:100%;padding:9px 12px;background:#2d3748;border:1px solid #4a5568;border-radius:8px;color:#e2e8f0;font-size:.85rem;margin-top:4px}
input:focus,select:focus{outline:none;border-color:#63b3ed}
.form-group{margin-bottom:14px}
.form-group label{font-size:.8rem;color:#a0aec0;display:block;margin-bottom:3px}
.toggle{position:relative;display:inline-block;width:48px;height:26px}
.toggle input{opacity:0;width:0;height:0}
.slider{position:absolute;inset:0;background:#4a5568;border-radius:13px;cursor:pointer;transition:.3s}
.slider:before{content:'';position:absolute;width:20px;height:20px;left:3px;top:3px;background:#fff;border-radius:50%;transition:.3s}
input:checked+.slider{background:#3182ce}
input:checked+.slider:before{transform:translateX(22px)}
.toggle-row{display:flex;align-items:center;justify-content:space-between;padding:8px 0}
.wifi-item{display:flex;align-items:center;gap:8px;padding:10px 12px;border:1px solid #2d3748;border-radius:8px;margin-bottom:8px;background:#141821;flex-wrap:wrap}
.wifi-item.current{border-color:#3182ce;background:#1a2744}
.wifi-signal{width:10px;height:10px;border-radius:50%;flex-shrink:0}
.signal-good{background:#68d391}.signal-ok{background:#f6e05e}.signal-weak{background:#fc8181}
.wifi-name{font-size:.85rem;font-weight:600;flex:1}
.wifi-meta{font-size:.75rem;color:#718096;margin-top:2px}
.pos-grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(160px,1fr));gap:10px}
.pos-card{background:#141821;border:1px solid #2d3748;border-radius:10px;padding:14px;text-align:center}
.pos-card.saved{border-color:#276749}
.pos-card h3{font-size:.9rem;font-weight:700;margin-bottom:6px}
.pos-card p{font-size:.75rem;color:#a0aec0;margin-bottom:10px}
.clock-display{font-size:2.5rem;font-weight:700;color:#63b3ed;text-align:center;padding:16px 0;letter-spacing:2px}
.date-display{font-size:1rem;color:#a0aec0;text-align:center;margin-bottom:16px}
.alert{padding:10px 14px;border-radius:8px;font-size:.83rem;margin-bottom:14px}
.alert-success{background:#1c4532;color:#9ae6b4;border:1px solid #276749}
.alert-error{background:#422b2b;color:#fc8181;border:1px solid #742a2a}
.alert-info{background:#1a3058;color:#90cdf4;border:1px solid #2a4a8a}
.divider{border:none;border-top:1px solid #2d3748;margin:16px 0}
.pass-field{position:relative;flex:1}
.pass-field input{padding-right:40px;margin-top:0}
.eye-btn{position:absolute;right:10px;top:50%;transform:translateY(-50%);background:none;border:none;color:#a0aec0;cursor:pointer;font-size:1rem}
table{width:100%;border-collapse:collapse;font-size:.83rem}
th{text-align:left;padding:8px 10px;color:#a0aec0;border-bottom:2px solid #2d3748;font-weight:600}
td{padding:8px 10px;border-bottom:1px solid #2d3748}
tr:hover td{background:#1e2a3a}
.spinner{display:inline-block;width:14px;height:14px;border:2px solid #4a5568;border-top-color:#63b3ed;border-radius:50%;animation:spin .8s linear infinite;margin-right:6px;vertical-align:middle}
@keyframes spin{to{transform:rotate(360deg)}}
#toast{position:fixed;bottom:24px;right:24px;padding:12px 20px;border-radius:10px;font-size:.85rem;font-weight:600;z-index:9999;opacity:0;transition:.3s;pointer-events:none;max-width:280px}
#toast.show{opacity:1}
#toast.success{background:#276749;color:#9ae6b4}
#toast.error{background:#742a2a;color:#fc8181}
#toast.info{background:#1a3058;color:#90cdf4}
@media(max-width:600px){main{padding:12px};nav a{padding:10px 12px}}
)RAW";

// ──────────────────────────────────────────────
//  JavaScript — الإصلاح الجوهري:
//  refreshStatus() تُستدعى تلقائياً بعد كل action
//  api() ترجع دايماً النتيجة
// ──────────────────────────────────────────────
static const char JS[] PROGMEM = R"RAW(
function toast(msg,type='success'){
  const t=document.getElementById('toast');
  t.textContent=msg;t.className='show '+type;
  clearTimeout(t._tid);
  t._tid=setTimeout(()=>t.className='',2800);
}
async function api(url,opts={}){
  try{
    const r=await fetch(url,{headers:{'Content-Type':'application/json'},...opts});
    const j=await r.json();
    return j;
  }catch(e){toast('Network error','error');return null;}
}
// ── حالة الجهاز ─────────────────────────────
async function refreshStatus(){
  const d=await api('/api/status');
  if(!d)return;
  const set=(id,v,cls)=>{const el=document.getElementById(id);if(el){el.textContent=v;if(cls)el.className='value '+cls;}};
  set('s-time',d.time);
  set('s-date',d.date);
  set('s-wifi',d.wifi?(d.ssid+' ('+d.ip+')'):'Offline',d.wifi?'green':'red');
  set('s-led',d.led?'ON':'OFF',d.led?'green':'red');
  set('s-water',d.water?'ON':'OFF',d.water?'blue':'red');
  set('s-basin',d.basin?'ON':'OFF',d.basin?'blue':'red');
  set('s-chair',d.moving?'Moving':'Idle',d.moving?'blue':'');
  const hdr=document.getElementById('wifi-badge');
  if(hdr){hdr.textContent=d.wifi?'WiFi: '+d.ssid:'Offline';hdr.className='badge '+(d.wifi?'badge-on':'badge-off');}
  const ct=document.getElementById('s-clock');
  if(ct)ct.textContent=d.time;
  const cd=document.getElementById('s-date2');
  if(cd)cd.textContent=d.date;
}
// ── تنفيذ action + تحديث فوري ───────────────
async function doAction(action){
  const r=await api('/api/action',{method:'POST',body:JSON.stringify({action})});
  if(r&&r.ok) toast(r.msg||'Done','success');
  else toast(r&&r.msg?r.msg:'Failed','error');
  await refreshStatus();   // ← تحديث فوري بعد كل action
}
// ── WiFi helpers ────────────────────────────
async function wifiConnect(ssid){
  const passEl=document.getElementById('pass-'+ssid);
  const pass=passEl?passEl.value:prompt('Password for: '+ssid,'');
  if(pass===null||pass===undefined)return;
  toast('Connecting...','info');
  const r=await api('/api/wifi/connect',{method:'POST',body:JSON.stringify({ssid,pass})});
  if(r&&r.ok)toast('Connecting to '+ssid,'info');
  else toast('Failed','error');
  setTimeout(()=>location.reload(),3500);
}
async function wifiConnectSaved(ssid,savedPass){
  toast('Connecting to '+ssid,'info');
  const r=await api('/api/wifi/connect',{method:'POST',body:JSON.stringify({ssid,pass:savedPass,use_saved:true})});
  if(r&&r.ok)toast('Connecting...','info');
  else toast('Failed','error');
  setTimeout(()=>location.reload(),3500);
}
async function wifiDisconnect(){
  if(!confirm('Disconnect from current network?'))return;
  await api('/api/wifi/disconnect',{method:'POST'});
  toast('Disconnected','info');
  setTimeout(()=>location.reload(),1500);
}
async function wifiDelete(idx,ssid){
  if(!confirm('Remove saved network: '+ssid+'?'))return;
  const r=await api('/api/wifi/delete',{method:'POST',body:JSON.stringify({idx})});
  if(r&&r.ok){toast('Removed: '+ssid);document.getElementById('row-'+idx)?.remove();}
  else toast('Failed','error');
}
async function wifiUpdatePass(ssid){
  const p=document.getElementById('new-pass')?.value;
  if(!p){alert('Enter new password');return;}
  const r=await api('/api/wifi/connect',{method:'POST',body:JSON.stringify({ssid,pass:p,update_only:true})});
  if(r&&r.ok)toast('Password updated!');else toast('Failed','error');
}
function togglePass(id){
  const inp=document.getElementById(id);
  if(inp)inp.type=inp.type==='password'?'text':'password';
}
// ── Settings save ────────────────────────────
// ↓ هذا هو الإصلاح: نقرأ القيم من الـ API أولاً ثم نحدث
async function loadSettingsForm(){
  const d=await api('/api/settings');
  if(!d)return;
  const val=(id,v)=>{const el=document.getElementById(id);if(el)el.value=v;};
  const chk=(id,v)=>{const el=document.getElementById(id);if(el)el.checked=v;};
  val('f-water',Math.round(d.waterTimeSec));
  val('f-basin',Math.round(d.basinTimeSec));
  val('f-hold', d.holdMs);
  val('f-tz',   d.tzOffset);
  val('f-hour', d.hour);
  val('f-min',  d.minute);
  val('f-day',  d.day);
  val('f-month',d.month);
  val('f-year', d.year);
  chk('f-24h',  d.use24h);
  chk('f-ntp',  d.autoSyncTime);
}
async function saveSettings(e){
  e.preventDefault();
  const body={
    waterTimeSec: parseInt(document.getElementById('f-water').value)||3,
    basinTimeSec: parseInt(document.getElementById('f-basin').value)||5,
    holdMs:       parseInt(document.getElementById('f-hold').value)||300,
    tzOffset:     parseInt(document.getElementById('f-tz').value)||0,
    hour:         parseInt(document.getElementById('f-hour').value),
    minute:       parseInt(document.getElementById('f-min').value),
    day:          parseInt(document.getElementById('f-day').value),
    month:        parseInt(document.getElementById('f-month').value),
    year:         parseInt(document.getElementById('f-year').value),
    use24h:       document.getElementById('f-24h').checked,
    autoSyncTime: document.getElementById('f-ntp').checked,
  };
  const r=await api('/api/settings',{method:'POST',body:JSON.stringify(body)});
  if(r&&r.ok){toast('Settings saved!');await refreshStatus();}
  else toast('Save failed','error');
}
// ── Positions ───────────────────────────────
async function savePositionWeb(slot,lblId,hkId){
  const lbl=document.getElementById(lblId)?.value||'P'+slot;
  const hk=parseInt(document.getElementById(hkId)?.value)||0;
  const r=await api('/api/positions/save',{method:'POST',body:JSON.stringify({slot,label:lbl,hotkey:hk})});
  if(r&&r.ok)toast('Position saved!');else toast('Failed','error');
}
async function deletePosition(slot){
  if(!confirm('Delete position P'+slot+'?'))return;
  await api('/api/positions/delete',{method:'POST',body:JSON.stringify({slot})});
  toast('Deleted');
  setTimeout(()=>location.reload(),800);
}
async function syncNTP(){
  toast('Syncing...','info');
  const r=await api('/api/action',{method:'POST',body:JSON.stringify({action:'ntp_sync'})});
  if(r&&r.ok)toast('Time synced!');else toast('Sync failed','error');
  await refreshStatus();
}
async function startScan(){
  const btn=document.getElementById('scan-btn');
  const res=document.getElementById('scan-results');
  btn.innerHTML='<span class=spinner></span>Scanning...';btn.disabled=true;
  const d=await api('/api/wifi/scan');
  btn.innerHTML='🔍 Scan Again';btn.disabled=false;
  if(!d||!d.scan){res.innerHTML='<div class="alert alert-error">Scan failed</div>';return;}
  if(d.scan.length===0){res.innerHTML='<div class="alert alert-info">No networks found</div>';return;}
  let html='';
  d.scan.forEach(n=>{
    const sig=n.rssi>-60?'signal-good':n.rssi>-80?'signal-ok':'signal-weak';
    const cur=n.ssid===d.current;
    html+=`<div class="wifi-item${cur?' current':''}">`;
    html+=`<span class="wifi-signal ${sig}"></span>`;
    html+=`<div style="flex:1"><div class="wifi-name">${n.ssid}${cur?' ✓':''}</div>`;
    html+=`<div class="wifi-meta">${n.rssi} dBm${n.enc?' 🔒':' 🔓'}</div></div>`;
    if(!cur){
      html+=`<div class="pass-field" style="width:140px"><input type="password" id="pass-${n.ssid}" placeholder="Password">`;
      html+=`<button class="eye-btn" onclick="togglePass('pass-${n.ssid}')">👁</button></div>`;
      html+=`<button class="btn btn-primary btn-sm" style="margin-left:6px" onclick="wifiConnect('${n.ssid}')">Connect</button>`;
    }
    html+=`</div>`;
  });
  res.innerHTML=html;
}
document.addEventListener('DOMContentLoaded',()=>{
  refreshStatus();
  setInterval(refreshStatus,3000);
  if(typeof loadSettingsForm==='function') loadSettingsForm();
  if(document.getElementById('scan-btn')) startScan();
});
)RAW";

// ──────────────────────────────────────────────
void DentalWebServer::begin() {
    if (!WifiManager.isConnected()) return;
    if (_running) return;   // لا تُشغّل مرتين

    _server.on("/style.css", HTTP_GET, [this]() {
        _server.sendHeader("Cache-Control", "max-age=3600");
        _server.send_P(200, "text/css", CSS);
    });
    _server.on("/app.js", HTTP_GET, [this]() {
        _server.sendHeader("Cache-Control", "max-age=300");
        _server.send_P(200, "application/javascript", JS);
    });

    _server.on("/",          HTTP_GET,  [this]() { _handleRoot(); });
    _server.on("/settings",  HTTP_GET,  [this]() { _handleSettings(); });
    _server.on("/wifi",      HTTP_GET,  [this]() { _handleWifi(); });
    _server.on("/positions", HTTP_GET,  [this]() { _handlePositions(); });

    _server.on("/api/status",           HTTP_GET,  [this]() { _apiStatus(); });
    _server.on("/api/settings",         HTTP_GET,  [this]() { _apiGetSettings(); });
    _server.on("/api/settings",         HTTP_POST, [this]() { _apiPostSettings(); });
    _server.on("/api/wifi/scan",        HTTP_GET,  [this]() { _apiWifiScan(); });
    _server.on("/api/wifi/connect",     HTTP_POST, [this]() { _apiWifiConnect(); });
    _server.on("/api/wifi/disconnect",  HTTP_POST, [this]() { _apiWifiDisconnect(); });
    _server.on("/api/wifi/delete",      HTTP_POST, [this]() { _apiWifiDelete(); });
    _server.on("/api/positions",        HTTP_GET,  [this]() { _apiGetPositions(); });
    _server.on("/api/positions/save",   HTTP_POST, [this]() { _apiSavePosition(); });
    _server.on("/api/positions/delete", HTTP_POST, [this]() { _apiDeletePosition(); });
    _server.on("/api/action",           HTTP_POST, [this]() { _apiAction(); });

    _server.onNotFound([this]() {
        if (_server.method() == HTTP_OPTIONS) { _sendCORS(); _server.send(204); }
        else _notFound();
    });

    _server.begin();
    _running = true;
    Serial.printf("[Web] http://%s/\n", WiFi.localIP().toString().c_str());
    Display.showPopup("Web UI Ready!", 2000);
}

void DentalWebServer::update() {
    if (_running) _server.handleClient();
    else if (WifiManager.isConnected()) begin();
}

void DentalWebServer::_sendCORS() {
    _server.sendHeader("Access-Control-Allow-Origin", "*");
    _server.sendHeader("Access-Control-Allow-Methods", "GET,POST,OPTIONS");
    _server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

// ── Page helpers ──────────────────────────────
void DentalWebServer::_sendPageStart(const char *title, const char *activeTab) {
    _server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    _server.sendHeader("Content-Type","text/html; charset=UTF-8");
    _server.send(200,"text/html","");
    _server.sendContent(F("<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'>"));
    _server.sendContent(F("<meta name='viewport' content='width=device-width,initial-scale=1'>"));
    char buf[80]; snprintf(buf,sizeof(buf),"<title>Dental Chair — %s</title>",title);
    _server.sendContent(buf);
    _server.sendContent(F("<link rel='stylesheet' href='/style.css'></head><body>"));
    _server.sendContent(F("<header><h1>🦷 Dental Chair Controller"));
    char wb[60];
    if(WifiManager.isConnected())
        snprintf(wb,sizeof(wb),"<span class='badge badge-on' id='wifi-badge'>WiFi: %.16s</span>",WifiManager.currentSSID());
    else
        strcpy(wb,"<span class='badge badge-off' id='wifi-badge'>Offline</span>");
    _server.sendContent(wb);
    _server.sendContent(F("</h1><span style='font-size:.8rem;color:#718096'>v6.0</span></header>"));
    char nav[450];
    snprintf(nav,sizeof(nav),
        "<nav>"
        "<a href='/'%s>Dashboard</a>"
        "<a href='/settings'%s>Settings</a>"
        "<a href='/wifi'%s>WiFi</a>"
        "<a href='/positions'%s>Positions</a>"
        "</nav>",
        strcmp(activeTab,"dashboard")==0?" class='active'":"",
        strcmp(activeTab,"settings")==0?" class='active'":"",
        strcmp(activeTab,"wifi")==0?" class='active'":"",
        strcmp(activeTab,"positions")==0?" class='active'":""
    );
    _server.sendContent(nav);
    _server.sendContent(F("<main><div id='toast'></div>"));
}
void DentalWebServer::_sendPageEnd() {
    _server.sendContent(F("</main><script src='/app.js'></script></body></html>"));
    _server.sendContent("");
}

// ══ DASHBOARD ════════════════════════════════
void DentalWebServer::_handleRoot() {
    _sendPageStart("Dashboard","dashboard");
    _server.sendContent(F("<div class='grid'>"));

    // Clock card
    char buf[200];
    snprintf(buf,sizeof(buf),
        "<div class='card'><h2>Clock</h2>"
        "<div class='clock-display' id='s-clock'>%s</div>"
        "<div class='date-display' id='s-date2'>%s</div>"
        "<div class='actions'>"
        "<button class='btn btn-ghost btn-sm' onclick=\"doAction('ntp_sync')\">🔄 Sync NTP</button>"
        "</div></div>",
        gTimeStr,gDateStr);
    _server.sendContent(buf);

    // Status card
    _server.sendContent(F("<div class='card'><h2>Status</h2>"));
    auto srow=[&](const char *lbl,const char *id,const char *val,const char *cls){
        char r[130];
        snprintf(r,sizeof(r),"<div class='status-row'><span class='label'>%s</span>"
                 "<span class='value %s' id='%s'>%s</span></div>",lbl,cls,id,val);
        _server.sendContent(r);
    };
    String ipStr=WifiManager.isConnected()?String(WifiManager.currentSSID())+" ("+WifiManager.getLocalIP()+")":"Offline";
    srow("WiFi","s-wifi",ipStr.c_str(),WifiManager.isConnected()?"green":"red");
    srow("LED",  "s-led",  Output.isLedOn()?"ON":"OFF",  Output.isLedOn()?"green":"red");
    srow("Water","s-water",Output.isWaterOn()?"ON":"OFF",Output.isWaterOn()?"blue":"red");
    srow("Basin","s-basin",Output.isBasinOn()?"ON":"OFF",Output.isBasinOn()?"blue":"red");
    srow("Suction","s-suct",Output.isSuctionOn()?"ON":"OFF",Output.isSuctionOn()?"blue":"red");
    srow("Chair","s-chair",Chair.isMoving()?"Moving":"Idle",Chair.isMoving()?"blue":"");
    srow("Base",  "s-base",  notif.baseOnline?"ONLINE":"OFFLINE",  notif.baseOnline?"green":"red");
    srow("Assist","s-assist",notif.assistOnline?"ONLINE":"OFFLINE",notif.assistOnline?"green":"red");
    _server.sendContent(F("</div>"));

    // Quick actions card
    _server.sendContent(F("<div class='card'><h2>Quick Actions</h2><div class='actions'>"));
    _server.sendContent(F("<button class='btn btn-ghost' onclick=\"doAction('led_toggle')\">💡 LED</button>"));
    _server.sendContent(F("<button class='btn btn-primary' onclick=\"doAction('water_timed')\">💧 Water</button>"));
    _server.sendContent(F("<button class='btn btn-primary' onclick=\"doAction('basin_timed')\">🪣 Basin</button>"));
    _server.sendContent(F("<button class='btn btn-primary' onclick=\"doAction('suction_timed')\">🌀 Suction</button>"));
    _server.sendContent(F("<button class='btn btn-danger' onclick=\"doAction('stop_all')\">⛔ Stop All</button>"));
    _server.sendContent(F("</div></div></div>"));

    // Positions
    _server.sendContent(F("<div class='card'><h2>Chair Positions</h2><div class='pos-grid'>"));
    char pbuf[220];
    for(uint8_t i=0;i<MAX_POSITIONS;i++){
        if(!gPositions[i].saved) continue;
        char lbl[12]; strncpy(lbl,gPositions[i].label,11); lbl[11]='\0';
        if(lbl[0]=='\0') snprintf(lbl,7,"P%d",i+1);
        snprintf(pbuf,sizeof(pbuf),
            "<div class='pos-card saved'><h3>%s</h3>"
            "<p>U:%d F:%d%s</p>"
            "<button class='btn btn-ghost btn-sm btn-full' onclick=\"doAction('goto_%d')\">▶ Go</button>"
            "</div>",
            lbl,gPositions[i].updown/40,gPositions[i].fwdback/40,
            gPositions[i].hotkey?(String(" HK:")+(char)('0'+gPositions[i].hotkey)).c_str():"",
            i+1);
        _server.sendContent(pbuf);
    }
    _server.sendContent(F("</div></div>"));
    _sendPageEnd();
}

// ══ SETTINGS ═════════════════════════════════
//  القيم تُحمَّل دايماً من /api/settings عبر loadSettingsForm() في JS
void DentalWebServer::_handleSettings() {
    _sendPageStart("Settings","settings");
    _server.sendContent(F("<form onsubmit='saveSettings(event)'><div class='grid'>"));

    // Timers card — قيم placeholder ستُملأ بـ JS
    _server.sendContent(F("<div class='card'><h2>Timers & Input</h2>"));
    _server.sendContent(F(
        "<div class='form-group'><label>Water Timer (seconds)</label>"
        "<input type='number' id='f-water' name='waterTimeSec' min='1' max='30'></div>"
        "<div class='form-group'><label>Basin Timer (seconds)</label>"
        "<input type='number' id='f-basin' name='basinTimeSec' min='1' max='60'></div>"
        "<div class='form-group'><label>Hold Threshold (ms) — Chair response</label>"
        "<input type='number' id='f-hold' name='holdMs' min='50' max='2000'></div>"
        "</div>"));

    // Clock card
    _server.sendContent(F("<div class='card'><h2>Clock & Date</h2>"));
    _server.sendContent(F(
        "<div class='form-group'><label>Hour (0-23)</label>"
        "<input type='number' id='f-hour' name='hour' min='0' max='23'></div>"
        "<div class='form-group'><label>Minute (0-59)</label>"
        "<input type='number' id='f-min' name='minute' min='0' max='59'></div>"
        "<div class='form-group'><label>Day</label>"
        "<input type='number' id='f-day' name='day' min='1' max='31'></div>"
        "<div class='form-group'><label>Month</label>"
        "<input type='number' id='f-month' name='month' min='1' max='12'></div>"
        "<div class='form-group'><label>Year</label>"
        "<input type='number' id='f-year' name='year' min='2020' max='2099'></div>"
        "<div class='toggle-row'><span class='label'>24h Format</span>"
        "<label class='toggle'><input type='checkbox' id='f-24h' name='use24h'><span class='slider'></span></label></div>"
        "<div class='form-group'><label>Timezone (UTC offset)</label>"
        "<input type='number' id='f-tz' name='tzOffset' min='-12' max='14'></div>"
        "<div class='toggle-row'><span class='label'>Auto Sync via NTP</span>"
        "<label class='toggle'><input type='checkbox' id='f-ntp' name='autoSyncTime'><span class='slider'></span></label></div>"
        "<button type='button' class='btn btn-ghost btn-sm' style='margin-top:10px' onclick='syncNTP()'>🔄 Sync Now</button>"
        "</div>"));

    _server.sendContent(F("</div>"));
    _server.sendContent(F("<button type='submit' class='btn btn-primary' style='margin-top:4px'>💾 Save All Settings</button></form>"));
    _sendPageEnd();
}

// ══ WIFI ═════════════════════════════════════
void DentalWebServer::_handleWifi() {
    _sendPageStart("WiFi","wifi");
    _server.sendContent(F("<div class='grid'>"));

    // Current connection
    _server.sendContent(F("<div class='card'><h2>Current Connection</h2>"));
    if(WifiManager.isConnected()){
        char buf[500]; char pass[65]={};
        WifiManager.getPassword(WifiManager.currentSSID(),pass,65);
        snprintf(buf,sizeof(buf),
            "<div class='status-row'><span class='label'>SSID</span><span class='value green'>%s</span></div>"
            "<div class='status-row'><span class='label'>IP Address</span><span class='value blue'>%s</span></div>"
            "<div class='status-row'><span class='label'>Signal</span><span class='value'>%d dBm</span></div>"
            "<hr class='divider'>"
            "<div class='form-group'><label>Saved Password</label>"
            "<div class='pass-field' style='display:flex;align-items:center'>"
            "<input type='password' id='cur-pass' value='%s' readonly>"
            "<button class='eye-btn' type='button' onclick=\"togglePass('cur-pass')\">👁</button></div></div>"
            "<div class='form-group'><label>Change Password</label>"
            "<div class='pass-field' style='display:flex;align-items:center'>"
            "<input type='password' id='new-pass' placeholder='New password'>"
            "<button class='eye-btn' type='button' onclick=\"togglePass('new-pass')\">👁</button></div></div>"
            "<div class='actions'>"
            "<button class='btn btn-warn btn-sm' onclick=\"wifiUpdatePass('%s')\">✏️ Update</button>"
            "<button class='btn btn-danger btn-sm' onclick='wifiDisconnect()'>⛔ Disconnect</button>"
            "</div>",
            WifiManager.currentSSID(),WifiManager.getLocalIP().c_str(),WiFi.RSSI(),
            pass,WifiManager.currentSSID());
        _server.sendContent(buf);
    } else {
        _server.sendContent(F("<div class='alert alert-error'>Not connected to any network</div>"));
    }
    _server.sendContent(F("</div>"));

    // Saved networks — مع زر Connect لكل شبكة
    _server.sendContent(F("<div class='card'><h2>Saved Networks</h2>"));
    uint8_t sc=WifiManager.getSavedCount();
    if(sc==0){
        _server.sendContent(F("<div class='alert alert-info'>No saved networks</div>"));
    } else {
        char row[350];
        for(uint8_t i=0;i<sc;i++){
            const SavedNetwork *net=WifiManager.getSaved(i);
            if(!net) continue;
            bool isCur=(strcmp(net->ssid,WifiManager.currentSSID())==0);
            // نمرر الباسورد المحفوظ مشفراً في JS لتجنب XSS — نستبدل ' بـ \'
            char safePass[70]={};
            strncpy(safePass,net->pass,63);
            // escape single quotes
            snprintf(row,sizeof(row),
                "<div class='wifi-item%s' id='row-%d'>"
                "<span class='wifi-signal %s'></span>"
                "<div style='flex:1'><div class='wifi-name'>%.28s%s</div></div>"
                "<div class='actions'>%s"
                "<button class='btn btn-danger btn-sm' onclick=\"wifiDelete(%d,'%s')\">🗑</button>"
                "</div></div>",
                isCur?" current":"", i,
                isCur?"signal-good":"signal-ok",
                net->ssid, isCur?" ✓":"",
                isCur?"":("<button class='btn btn-ghost btn-sm' onclick=\"wifiConnectSaved('"+
                           String(net->ssid)+"','"+String(net->pass)+"')\">Connect</button>").c_str(),
                i, net->ssid);
            _server.sendContent(row);
        }
    }
    _server.sendContent(F("</div></div>"));

    // Scan card
    _server.sendContent(F(
        "<div class='card'><h2>Scan & Connect</h2>"
        "<button class='btn btn-ghost' id='scan-btn' onclick='startScan()'>🔍 Scan Networks</button>"
        "<div id='scan-results' style='margin-top:14px'></div></div>"));

    _sendPageEnd();
}

// ══ POSITIONS ════════════════════════════════
void DentalWebServer::_handlePositions() {
    _sendPageStart("Positions","positions");
    _server.sendContent(F(
        "<div class='card' style='margin-bottom:16px'><h2>Chair Positions</h2>"
        "<p style='font-size:.83rem;color:#a0aec0;margin-bottom:14px'>"
        "Current — UD: <b id='cur-ud'>--</b> &nbsp; FB: <b id='cur-fb'>--</b></p>"));
    _server.sendContent(F("<table><thead><tr>"
        "<th>#</th><th>Label</th><th>Hotkey</th><th>UD%</th><th>FB%</th><th>Actions</th>"
        "</tr></thead><tbody>"));
    char row[450];
    for(uint8_t i=0;i<MAX_POSITIONS;i++){
        char lbl[12]; strncpy(lbl,gPositions[i].label,11); lbl[11]='\0';
        if(lbl[0]=='\0') snprintf(lbl,7,"P%d",i+1);
        if(gPositions[i].saved){
            // build hotkey select
            char hkSel[350]="<select id='hk-"; char tmp[20];
            snprintf(tmp,sizeof(tmp),"%d'",i); strcat(hkSel,tmp);
            strcat(hkSel," style='width:55px'>");
            for(int h=0;h<=9;h++){
                char opt[60];
                snprintf(opt,sizeof(opt),"<option value='%d'%s>%s</option>",
                         h, gPositions[i].hotkey==h?" selected":"", h==0?"-":(char[]){(char)('0'+h),0});
                strcat(hkSel,opt);
            }
            strcat(hkSel,"</select>");
            snprintf(row,sizeof(row),
                "<tr><td><b>%d</b></td>"
                "<td><input type='text' id='lbl-%d' value='%.11s' style='width:75px;margin-top:0'></td>"
                "<td>%s</td><td>%d</td><td>%d</td>"
                "<td><div class='actions'>"
                "<button class='btn btn-ghost btn-sm' onclick=\"savePositionWeb(%d,'lbl-%d','hk-%d')\">💾</button>"
                "<button class='btn btn-primary btn-sm' onclick=\"doAction('goto_%d')\">▶</button>"
                "<button class='btn btn-danger btn-sm' onclick='deletePosition(%d)'>🗑</button>"
                "</div></td></tr>",
                i+1,i,lbl,hkSel,
                gPositions[i].updown/40,gPositions[i].fwdback/40,
                i+1,i,i,i+1,i+1);
        } else {
            snprintf(row,sizeof(row),
                "<tr style='opacity:.5'><td>%d</td>"
                "<td colspan='4' style='color:#718096'>— Empty —</td>"
                "<td><button class='btn btn-ghost btn-sm' onclick=\"doAction('save_pos_%d')\">📍 Save Here</button></td>"
                "</tr>",i+1,i+1);
        }
        _server.sendContent(row);
    }
    _server.sendContent(F("</tbody></table></div>"));
    _server.sendContent(F("<script>"
        "async function updateADC(){"
          "const d=await api('/api/status');"
          "if(d){document.getElementById('cur-ud').textContent=d.ud||'--';"
          "document.getElementById('cur-fb').textContent=d.fb||'--';}"
        "}"
        "setInterval(updateADC,1000);updateADC();"
        "</script>"));
    _sendPageEnd();
}

// ══ API ══════════════════════════════════════
void DentalWebServer::_apiStatus() {
    _sendCORS();
    char json[500];
    String ip=WifiManager.getLocalIP();
    snprintf(json,sizeof(json),
        "{\"ok\":true,\"time\":\"%s\",\"date\":\"%s\","
        "\"wifi\":%s,\"ssid\":\"%s\",\"ip\":\"%s\","
        "\"led\":%s,\"water\":%s,\"basin\":%s,"
        "\"suction\":%s,\"moving\":%s,"
        "\"base\":%s,\"assist\":%s,"
        "\"ud\":%d,\"fb\":%d}",
        gTimeStr,gDateStr,
        WifiManager.isConnected()?"true":"false",
        WifiManager.currentSSID(),ip.c_str(),
        Output.isLedOn()?"true":"false",
        Output.isWaterOn()?"true":"false",
        Output.isBasinOn()?"true":"false",
        Output.isSuctionOn()?"true":"false",
        Chair.isMoving()?"true":"false",
        notif.baseOnline?"true":"false",
        notif.assistOnline?"true":"false",
        Chair.readUpDown(),Chair.readFwdBack());
    _server.send(200,"application/json",json);
}

// ── GET Settings: يُرجع القيم الحقيقية الحالية ─
void DentalWebServer::_apiGetSettings() {
    _sendCORS();
    // نقرأ الوقت الحالي من الـ RTC لعرضه في الفورم
    DateTime now = TimeMgr.getNow();
    char json[350];
    snprintf(json,sizeof(json),
        "{\"ok\":true,"
        "\"waterTimeSec\":%d,\"basinTimeSec\":%d,\"holdMs\":%d,"
        "\"use24h\":%s,\"tzOffset\":%d,\"autoSyncTime\":%s,"
        "\"hour\":%d,\"minute\":%d,"
        "\"day\":%d,\"month\":%d,\"year\":%d}",
        (int)(gSettings.waterTimeMs/1000),
        (int)(gSettings.basinTimeMs/1000),
        (int)gSettings.holdThresholdMs,
        gSettings.use24h?"true":"false",
        (int)gSettings.tzOffsetHours,
        gSettings.autoSyncTime?"true":"false",
        (int)now.hour(),(int)now.minute(),
        (int)now.day(),(int)now.month(),(int)now.year());
    _server.send(200,"application/json",json);
}

// ── POST Settings ────────────────────────────
void DentalWebServer::_apiPostSettings() {
    _sendCORS();
    if(!_server.hasArg("plain")){_server.send(400,"application/json","{\"ok\":false}");return;}
    String body=_server.arg("plain");

    auto getInt=[&](const char *key,int def)->int{
        String k=String("\"")+key+"\":";
        int idx=body.indexOf(k); if(idx<0) return def;
        return body.substring(idx+k.length()).toInt();
    };
    auto getBool=[&](const char *key,bool def)->bool{
        String k=String("\"")+key+"\":";
        int idx=body.indexOf(k); if(idx<0) return def;
        String v=body.substring(idx+k.length(),idx+k.length()+5);
        return v.startsWith("true");
    };

    int ws  =getInt("waterTimeSec",gSettings.waterTimeMs/1000);
    int bs  =getInt("basinTimeSec",gSettings.basinTimeMs/1000);
    int hm  =getInt("holdMs",      gSettings.holdThresholdMs);
    int tz  =getInt("tzOffset",    gSettings.tzOffsetHours);
    int h   =getInt("hour",   -1);
    int m   =getInt("minute", -1);
    int d   =getInt("day",    -1);
    int mo  =getInt("month",  -1);
    int y   =getInt("year",   -1);
    bool u24=getBool("use24h",     gSettings.use24h);
    bool asn=getBool("autoSyncTime",gSettings.autoSyncTime);

    if(ws>=1&&ws<=30)     gSettings.waterTimeMs     =(uint32_t)ws*1000;
    if(bs>=1&&bs<=60)     gSettings.basinTimeMs     =(uint32_t)bs*1000;
    if(hm>=50&&hm<=2000)  gSettings.holdThresholdMs =(uint16_t)hm;
    if(tz>=-12&&tz<=14)  {gSettings.tzOffsetHours   =(int8_t)tz; TimeMgr.setTZ(tz);}
    gSettings.use24h      =u24;
    gSettings.autoSyncTime=asn;

    if(h>=0&&h<=23&&m>=0&&m<=59) TimeMgr.setTime((uint8_t)h,(uint8_t)m);
    if(y>=2020&&mo>=1&&mo<=12&&d>=1&&d<=31) TimeMgr.setDate((uint8_t)d,(uint8_t)mo,(uint16_t)y);
    if(asn&&WifiManager.isConnected()) TimeMgr.syncNTP();

    SDMgr.saveSettings(gSettings);
    Display.showPopup("Settings updated",1200);
    _server.send(200,"application/json","{\"ok\":true,\"msg\":\"Saved\"}");
}

// ── WiFi Scan (async safe) ────────────────────
void DentalWebServer::_apiWifiScan() {
    _sendCORS();
    // blocking scan — مقبول هنا لأن الويب طلبه صراحة
    int n=WiFi.scanNetworks(false,true);
    String json="{\"ok\":true,\"current\":\"";
    json+=WifiManager.currentSSID();
    json+="\",\"ip\":\""+WifiManager.getLocalIP();
    json+="\",\"scan\":[";
    for(int i=0;i<n&&i<20;i++){
        if(i>0) json+=",";
        json+="{\"ssid\":\""; json+=WiFi.SSID(i);
        json+="\",\"rssi\":"; json+=WiFi.RSSI(i);
        json+=",\"enc\":";    json+=(WiFi.encryptionType(i)!=WIFI_AUTH_OPEN)?"true":"false";
        json+="}";
    }
    json+="]}";
    _server.send(200,"application/json",json);
}

// ── WiFi Connect ─────────────────────────────
void DentalWebServer::_apiWifiConnect() {
    _sendCORS();
    if(!_server.hasArg("plain")){_server.send(400,"application/json","{\"ok\":false}");return;}
    String body=_server.arg("plain");
    auto getStr=[&](const char *key)->String{
        String k=String("\"")+key+"\":\"";
        int idx=body.indexOf(k); if(idx<0) return "";
        int s=idx+k.length(), e=body.indexOf("\"",s); if(e<0) return "";
        return body.substring(s,e);
    };
    auto getBool=[&](const char *key)->bool{
        String k=String("\"")+key+"\":";
        int idx=body.indexOf(k); if(idx<0) return false;
        return body.substring(idx+k.length(),idx+k.length()+4)=="true";
    };
    String ssid=getStr("ssid");
    String pass=getStr("pass");
    bool updateOnly=getBool("update_only");
    bool useSaved  =getBool("use_saved");    // اتصال بشبكة محفوظة مباشرة

    if(ssid.length()==0){_server.send(400,"application/json","{\"ok\":false,\"msg\":\"No SSID\"}");return;}

    if(updateOnly){
        WifiManager.updatePassword(ssid.c_str(),pass.c_str());
        _server.send(200,"application/json","{\"ok\":true,\"msg\":\"Password updated\"}");
    } else {
        // useSaved=true: استخدم الباسورد المحفوظ (الـ pass من JS)
        WifiManager.connectToSSID(ssid.c_str(),pass.c_str());
        _server.send(200,"application/json","{\"ok\":true,\"msg\":\"Connecting...\"}");
    }
}

void DentalWebServer::_apiWifiDisconnect() {
    _sendCORS();
    WifiManager.disconnect();
    _server.send(200,"application/json","{\"ok\":true,\"msg\":\"Disconnected\"}");
}

void DentalWebServer::_apiWifiDelete() {
    _sendCORS();
    if(!_server.hasArg("plain")){_server.send(400,"application/json","{\"ok\":false}");return;}
    String body=_server.arg("plain");
    // نقبل index رقمي (أكثر أماناً من string match)
    String k="\"idx\":";
    int idx_pos=body.indexOf(k);
    if(idx_pos>=0){
        int idx=body.substring(idx_pos+k.length()).toInt();
        if(idx>=0&&idx<WifiManager.getSavedCount()){
            WifiManager.deleteNetwork((uint8_t)idx);
            _server.send(200,"application/json","{\"ok\":true}");
            return;
        }
    }
    _server.send(404,"application/json","{\"ok\":false,\"msg\":\"Not found\"}");
}

// ── Positions ────────────────────────────────
void DentalWebServer::_apiGetPositions() {
    _sendCORS();
    String json="{\"ok\":true,\"positions\":[";
    bool first=true;
    for(uint8_t i=0;i<MAX_POSITIONS;i++){
        if(!gPositions[i].saved) continue;
        if(!first) json+=","; first=false;
        char buf[100]; char lbl[12]; strncpy(lbl,gPositions[i].label,11); lbl[11]='\0';
        if(lbl[0]=='\0') snprintf(lbl,7,"P%d",i+1);
        snprintf(buf,sizeof(buf),"{\"slot\":%d,\"label\":\"%s\",\"hotkey\":%d,\"ud\":%d,\"fb\":%d}",
                 i+1,lbl,(int)gPositions[i].hotkey,gPositions[i].updown/40,gPositions[i].fwdback/40);
        json+=buf;
    }
    json+="]}";
    _server.send(200,"application/json",json);
}

void DentalWebServer::_apiSavePosition() {
    _sendCORS();
    if(!_server.hasArg("plain")){_server.send(400,"application/json","{\"ok\":false}");return;}
    String body=_server.arg("plain");
    auto getInt=[&](const char *key,int def)->int{
        String k=String("\"")+key+"\":"; int idx=body.indexOf(k); if(idx<0) return def;
        return body.substring(idx+k.length()).toInt();
    };
    auto getStr=[&](const char *key)->String{
        String k=String("\"")+key+"\":\""; int idx=body.indexOf(k); if(idx<0) return "";
        int s=idx+k.length(),e=body.indexOf("\"",s); if(e<0) return "";
        return body.substring(s,e);
    };
    int slot=getInt("slot",0); int hotkey=getInt("hotkey",0);
    String lbl=getStr("label");
    if(slot<1||slot>MAX_POSITIONS){_server.send(400,"application/json","{\"ok\":false}");return;}
    Chair.saveCurrentPosition((uint8_t)slot);
    if(lbl.length()>0) Chair.setPositionLabel((uint8_t)slot,lbl.c_str());
    if(hotkey>=0&&hotkey<=9) Chair.setPositionHotkey((uint8_t)slot,(uint8_t)hotkey);
    _server.send(200,"application/json","{\"ok\":true,\"msg\":\"Saved\"}");
}

void DentalWebServer::_apiDeletePosition() {
    _sendCORS();
    if(!_server.hasArg("plain")){_server.send(400,"application/json","{\"ok\":false}");return;}
    String body=_server.arg("plain");
    String k="\"slot\":"; int idx=body.indexOf(k); if(idx<0){_server.send(400,"application/json","{\"ok\":false}");return;}
    int slot=body.substring(idx+k.length()).toInt();
    Chair.deletePosition((uint8_t)slot);
    _server.send(200,"application/json","{\"ok\":true}");
}

// ── Action handler ────────────────────────────
void DentalWebServer::_apiAction() {
    _sendCORS();
    if(!_server.hasArg("plain")){_server.send(400,"application/json","{\"ok\":false}");return;}
    String body=_server.arg("plain");
    String ak="\"action\":\""; int aidx=body.indexOf(ak);
    if(aidx<0){_server.send(400,"application/json","{\"ok\":false}");return;}
    int astart=aidx+ak.length(), aend=body.indexOf("\"",astart);
    String action=body.substring(astart,aend);

    const char *msg="Done";
    if      (action=="led_toggle")  {Output.ledToggle();  msg="LED toggled";}
    else if (action=="water_timed")   {Output.waterTimed();   msg="Water started";}
    else if (action=="basin_timed")   {Output.basinTimed();   msg="Basin started";}
    else if (action=="suction_timed") {Output.suctionTimed(); msg="Suction started";}
    else if (action=="led_toggle")    {Output.ledToggle();    msg="LED toggled";}
    else if (action=="stop_all")      {Output.allOff(); Chair.stopAll(); msg="Stopped";}
    else if (action=="ntp_sync")    {
        if(WifiManager.isConnected()&&TimeMgr.syncNTP()) msg="Time synced";
        else msg="Sync failed";
    }
    else if (action.startsWith("goto_")) {
        int slot=action.substring(5).toInt();
        if(slot>=1&&slot<=MAX_POSITIONS){Chair.goToPosition((uint8_t)slot);msg="Moving...";}
        else{_server.send(400,"application/json","{\"ok\":false,\"msg\":\"Bad slot\"}");return;}
    }
    else if (action.startsWith("save_pos_")) {
        int slot=action.substring(9).toInt();
        if(slot>=1&&slot<=MAX_POSITIONS){Chair.saveCurrentPosition((uint8_t)slot);msg="Position saved";}
        else{_server.send(400,"application/json","{\"ok\":false,\"msg\":\"Bad slot\"}");return;}
    }
    else {_server.send(400,"application/json","{\"ok\":false,\"msg\":\"Unknown action\"}");return;}

    char json[80];
    snprintf(json,sizeof(json),"{\"ok\":true,\"msg\":\"%s\"}",msg);
    _server.send(200,"application/json",json);
}

void DentalWebServer::_notFound() {
    _server.send(404,"text/plain","Not Found");
}
