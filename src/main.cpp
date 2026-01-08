#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "LS.h"

// WiFi credentials
const char* WIFI_SSID = "YOUR_SSID";
const char* WIFI_PASS = "YOUR_PASSWORD";

WebServer server(80);

// helper: convert bytes to hex string
String bytesToHex(const uint8_t* data, size_t len) {
    String s;
    s.reserve(len * 2);
    for (size_t i = 0; i < len; ++i) {
        char buf[3];
        snprintf(buf, sizeof(buf), "%02X", data[i]);
        s += buf;
    }
    return s;
}

// Return JSON array of manufacturer data entries (as hex strings)
void handle_state() {
    String out = "[";
    size_t maxEntries = sizeof(manufacturerDataList) / MANUFACTURER_DATA_LENGTH;
    for (size_t i = 0; i < maxEntries; ++i) {
        if (i) out += ",";
        out += "\"" + bytesToHex(manufacturerDataList[i], MANUFACTURER_DATA_LENGTH) + "\"";
    }
    out += "]";
    server.send(200, "application/json", out);
}

// POST /adjust?index=0&pos=9&value=123
void handle_adjust() {
    if (!server.hasArg("index") || !server.hasArg("pos") || !server.hasArg("value")) {
        server.send(400, "text/plain", "missing args");
        return;
    }
    int idx = server.arg("index").toInt();
    int pos = server.arg("pos").toInt();
    int val = server.arg("value").toInt();
    size_t maxEntries = sizeof(manufacturerDataList) / MANUFACTURER_DATA_LENGTH;

    if (idx < 0 || idx >= maxEntries) {
        server.send(400, "text/plain", "invalid index");
        return;
    }

    int vibStart = MANUFACTURER_DATA_LENGTH - 3;
    if (pos < vibStart + 1 || pos > vibStart + 2) {
        server.send(400, "text/plain", "invalid pos");
        return;
    }

    if (val < 0) val = 0;
    if (val > 255) val = 255;

    manufacturerDataList[idx][pos] = (uint8_t)val;

    server.send(200, "application/json", "{\"ok\":true}\n");
}

// POST /send?index=0 -> advertise immediately
void handle_send() {
    if (!server.hasArg("index")) {
        server.send(400, "text/plain", "missing index");
        return;
    }
    int idx = server.arg("index").toInt();
    size_t maxEntries = sizeof(manufacturerDataList) / MANUFACTURER_DATA_LENGTH;
    if (idx < 0 || idx >= maxEntries) {
        server.send(400, "text/plain", "invalid index");
        return;
    }

    set_manufacturer_data(idx); // сразу рекламируем BLE
    server.send(200, "application/json", "{\"ok\":true}\n");
}

// Single-page UI
const char index_html[] PROGMEM = R"rawliteral(
<!doctype html>
<html>
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>BLE HEX Picker</title>
<style>
body{font-family:Arial,Helvetica,sans-serif;background:#071029;color:#e6eef8;margin:0;padding:10px;font-size:13px}
.wrap{max-width:760px;margin:0 auto}
h1{font-size:18px;margin:0 0 8px;text-align:center}
.card{background:#0f1724;padding:8px;border-radius:6px;border:1px solid rgba(255,255,255,0.03);margin:6px 0}
.label{font-weight:700;font-size:14px}
.hex{font-family:monospace;background:#08121a;padding:4px 6px;border-radius:6px;display:inline-block;margin-top:6px;font-size:12px}
.row{display:flex;align-items:center;gap:6px;margin:6px 0}
input[type=range]{width:140px}
.small{color:#94a3b8;font-size:12px;margin-bottom:8px}
button{background:#7c3aed;color:#fff;border:0;padding:6px 8px;border-radius:6px;cursor:pointer;font-size:13px}
button.ghost{background:transparent;border:1px solid rgba(255,255,255,0.06);color:#cfe0ff}
@media(min-width:800px){.cards{display:grid;grid-template-columns:repeat(2,1fr);gap:8px}}
</style>
</head>
<body>
<div class="wrap">
<h1>HEX Picker</h1>
<div id="grid" class="cards"></div>
</div>

<script>
const labels = ['Stop','L1','L2','L3','L4','L5','L6','L7'];

async function refresh(){
    const res = await fetch('/state');
    const arr = await res.json();
    const grid = document.getElementById('grid'); grid.innerHTML = '';

    for (let i=0;i<arr.length;i++){
        const hex = arr[i];
        const bytes = [];
        for (let k=0;k<hex.length;k+=2) bytes.push(parseInt(hex.slice(k,k+2),16));
        const vibStart = bytes.length - 3;

        const card = document.createElement('div'); card.className='card';
        const title = document.createElement('div'); title.className='label'; title.textContent = labels[i] || ('Profile '+i);
        const hexEl = document.createElement('div'); hexEl.className='hex';
        const displayHex = (hex.length > 6) ? ('...' + hex.slice(-6)) : hex;
        hexEl.textContent = displayHex;
        card.appendChild(title); card.appendChild(hexEl);

        for (let idx=vibStart+1; idx<=vibStart+2; ++idx){
            const r = document.createElement('div'); r.className='row';
            const idxEl = document.createElement('div'); idxEl.textContent = '#'+idx; idxEl.style.width='34px';
            const input = document.createElement('input'); input.type='range'; input.min=0; input.max=255; input.value=bytes[idx];
            const num = document.createElement('input'); num.type='number'; num.min=0; num.max=255; num.value=bytes[idx]; num.style.width='56px';

            async function applyTarget(target){
                await fetch('/adjust?index='+i+'&pos='+idx+'&value='+target, {method:'POST'});
                await refresh();
            }

            input.addEventListener('input', ()=>{ num.value = input.value; });
            input.addEventListener('change', ()=> applyTarget(parseInt(input.value)));
            num.addEventListener('change', ()=> applyTarget(parseInt(num.value)));

            r.appendChild(idxEl); r.appendChild(input); r.appendChild(num); card.appendChild(r);
        }

        const actions = document.createElement('div'); actions.className='row';
        const sendBtn = document.createElement('button'); sendBtn.textContent='Send'; sendBtn.onclick = ()=>fetch('/send?index='+i,{method:'POST'});
        actions.appendChild(sendBtn); card.appendChild(actions);

        grid.appendChild(card);
    }
}

window.onload = refresh;
</script>
</body>
</html>
)rawliteral";

void handle_root() {
    server.send_P(200, "text/html", index_html);
}

void setup() {
    Serial.begin(115200);

    // Initialize BLE
    NimBLEDevice::init("BLE HEX Picker");
    muse_init();
    muse_start();

    // WiFi
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    Serial.printf("Connecting to WiFi '%s'...\n", WIFI_SSID);
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 40) {
        delay(500);
        Serial.print('.');
        attempts++;
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("\nWiFi connected, IP: %s\n", WiFi.localIP().toString().c_str());
    } else {
        Serial.println("\nWiFi connection failed, starting anyway.\n");
    }

    // HTTP endpoints
    server.on("/", HTTP_GET, handle_root);
    server.on("/state", HTTP_GET, handle_state);
    server.on("/adjust", HTTP_POST, handle_adjust);
    server.on("/send", HTTP_POST, handle_send);
    server.begin();
    Serial.println("HTTP server started");
}

void loop() {
    server.handleClient();
    delay(10);
}
