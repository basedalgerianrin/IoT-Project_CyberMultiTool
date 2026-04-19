#include "WebMenuServer.h"

WebMenuServer::WebMenuServer(const char* ssid, const char* password, MenuDisplay& display, WiFiSniffer& sniffer, GPSReader& gps)
    : _ssid(ssid), _password(password), _display(display), _sniffer(sniffer), _gps(gps), _jam(nullptr), _server(80) {}

void WebMenuServer::setJam(StrawberryJam* jam) { _jam = jam; }

void WebMenuServer::begin() {
    Serial.print(F("Connecting to WiFi: "));
    Serial.println(_ssid);

    WiFi.mode(WIFI_AP_STA);  // AP_STA needed for esp_wifi_80211_tx (deauth)

    if (WiFi.softAP("ESP32_CyberTool", NULL, 1, 1)) {
        Serial.println(F("[WiFi] Hidden AP started — WIFI_IF_AP ready for raw TX"));
    } else {
        Serial.println(F("[WiFi] softAP FAILED — deauth will not work!"));
    }

    WiFi.begin(_ssid, _password);

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
        delay(500);
        Serial.print(F("."));
    }

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println(F("\nWiFi FAILED — running without web server."));
        return;
    }

    Serial.println();
    Serial.print(F("Connected to "));
    Serial.println(_ssid);
    Serial.print(F("IP address: "));
    Serial.println(WiFi.localIP());

    if (MDNS.begin("esp32")) {
        Serial.println(F("mDNS started: http://esp32.local"));
    }

    // Root redirects to sniffer as default landing page
    _server.on("/",        [this]() { handleSniffer(); });
    _server.on("/sniffer", [this]() { handleSniffer(); });
    _server.on("/sensor",  [this]() { handleSensor();  });
    _server.on("/gps",     [this]() { handleGPS();     });
    _server.on("/jam",     [this]() { handleJam();     });
    _server.onNotFound(    [this]() { handleNotFound(); });

    _server.begin();
    Serial.println(F("HTTP server started."));
}

void WebMenuServer::handleClient() {
    if (WiFi.status() == WL_CONNECTED) {
        _server.handleClient();
    }
}

// ─── Shared CSS ───────────────────────────────────────────────────────────────

static const char* CSS =
    "<style>"
    "body{font-family:Arial,sans-serif;background:#1a1a2e;color:#eee;margin:0;padding:20px;}"
    "h2{color:#00d4ff;margin-top:0;border-bottom:1px solid #00d4ff;padding-bottom:8px;}"
    "h3{color:#aaa;font-size:14px;margin:20px 0 8px;}"
    "table{width:100%;border-collapse:collapse;}"
    "tr{border-bottom:1px solid #2a2a4a;}"
    "td,th{padding:8px 6px;font-size:13px;text-align:left;}"
    "th{color:#00d4ff;border-bottom:2px solid #00d4ff;}"
    ".stats td:first-child{color:#aaa;width:45%;}"
    ".stats td:last-child{color:#00ff88;font-weight:bold;text-align:right;}"
    ".log td{color:#ccc;font-family:monospace;font-size:12px;}"
    ".log tr:hover{background:#16213e;}"
    ".deauth{color:#ff4444;font-weight:bold;}"
    ".beacon{color:#44ff44;}"
    ".probe{color:#ffaa44;}"
    ".data{color:#4488ff;}"
    "p.note{color:#666;font-size:12px;margin-top:16px;}"
    "nav{margin-top:20px;}"
    "nav a{margin-right:12px;color:#00d4ff;text-decoration:none;font-size:14px;}"
    "nav a:hover{text-decoration:underline;}"
    "</style>";

// ─── Pages ────────────────────────────────────────────────────────────────────

void WebMenuServer::handleSniffer() {
    String html;
    html.reserve(4096);
    html = "<!DOCTYPE html><html><head>"
        "<meta charset='UTF-8'>"
        "<meta name='viewport' content='width=device-width,initial-scale=1'>"
        "<meta http-equiv='refresh' content='3'>"
        "<title>Packet Sniffer</title>" + String(CSS) + "</head><body>"

        // ── Summary Dashboard ──
        "<h2>Packet Sniffer</h2>"
        "<table class='stats'>"
        "<tr><td>Total Packets</td><td>" + String(_sniffer.getTotal()) + "</td></tr>"
        "<tr><td>Beacons</td><td>"       + String(_sniffer.getBeacons()) + "</td></tr>"
        "<tr><td>Deauths</td><td>"        + String(_sniffer.getDeauths()) + "</td></tr>"
        "<tr><td>Probe Requests</td><td>" + String(_sniffer.getProbeReqs()) + "</td></tr>"
        "<tr><td>Probe Responses</td><td>"+ String(_sniffer.getProbeResps()) + "</td></tr>"
        "<tr><td>Data Frames</td><td>"    + String(_sniffer.getDataPkts()) + "</td></tr>"
        "<tr><td>Unique MACs</td><td>"    + String(_sniffer.getUniqueMacs()) + "</td></tr>"
        "</table>";

    // ── Packet Log Table (like Wireshark) ──
    html += "<h3>Packet Log (last " + String(_sniffer.getLogCount()) + " packets)</h3>"
        "<table class='log'>"
        "<tr><th>#</th><th>Time (s)</th><th>Source MAC</th><th>Type</th><th>RSSI</th></tr>";

    // Show packets from newest to oldest
    int count = _sniffer.getLogCount();
    for (int i = count - 1; i >= 0; i--) {
        PacketEntry e = _sniffer.getLogEntry(i);

        // Colour code by packet type
        const char* cls = "";
        switch (e.type) {
            case PKT_DEAUTH:     cls = "deauth"; break;
            case PKT_BEACON:     cls = "beacon"; break;
            case PKT_PROBE_REQ:
            case PKT_PROBE_RESP: cls = "probe";  break;
            case PKT_DATA:       cls = "data";   break;
            default: break;
        }

        html += "<tr><td>" + String(count - i) + "</td>"
            "<td>" + String(e.timestamp / 1000.0, 1) + "</td>"
            "<td>" + WiFiSniffer::macToString(e.srcMac) + "</td>"
            "<td class='" + String(cls) + "'>" + String(WiFiSniffer::typeToString(e.type)) + "</td>"
            "<td>" + String(e.rssi) + "</td></tr>";
    }

    html += "</table>"
        "<p class='note'>Auto-refreshes every 3s | Colour: "
        "<span class='beacon'>Beacon</span> "
        "<span class='deauth'>Deauth</span> "
        "<span class='probe'>Probe</span> "
        "<span class='data'>Data</span></p>"
        "<nav><a href='/sniffer'>Sniffer</a><a href='/jam'>Jam</a><a href='/sensor'>Sensor</a><a href='/gps'>GPS</a></nav>"
        "</body></html>";

    _server.send(200, "text/html", html);
}

void WebMenuServer::handleSensor() {
    // ── Find the strongest suspicious device for the hot/cold indicator ──
    int targetIdx = _sniffer.getStrongestSuspicious();

    String html;
    html.reserve(4096);
    html = "<!DOCTYPE html><html><head>"
        "<meta charset='UTF-8'>"
        "<meta name='viewport' content='width=device-width,initial-scale=1'>"
        "<meta http-equiv='refresh' content='2'>"
        "<title>Wireless Sensor</title>" + String(CSS) +
        "<style>"
        ".hotcold{text-align:center;padding:20px;border-radius:12px;margin:12px 0;}"
        ".hot{background:linear-gradient(135deg,#ff0000,#ff4400);color:#fff;}"
        ".warm{background:linear-gradient(135deg,#ff8800,#ffaa00);color:#000;}"
        ".cool{background:linear-gradient(135deg,#00aaff,#0066ff);color:#fff;}"
        ".cold{background:linear-gradient(135deg,#0033aa,#001166);color:#aaf;}"
        ".none{background:#2a2a4a;color:#888;}"
        ".temp-label{font-size:28px;font-weight:bold;}"
        ".temp-rssi{font-size:14px;margin-top:4px;}"
        ".temp-vendor{font-size:12px;margin-top:2px;opacity:0.8;}"
        ".suspect{color:#ff4444;font-weight:bold;}"
        ".cam-oui{color:#ff8800;}"
        ".normal{color:#888;}"
        ".reason{font-size:11px;color:#aaa;}"
        ".ch-info{color:#00d4ff;font-size:13px;margin:8px 0;}"
        "</style>"
        "</head><body>"

        "<h2>Wireless Sensor</h2>"
        "<p class='ch-info'>Channel: " + String(_sniffer.getCurrentChannel())
        + " | Mode: " + String(_sniffer.isHopping() ? "Scanning (1,6,11)" : "Locked") + "</p>";

    // ── Hot / Cold Indicator ──
    if (targetIdx >= 0) {
        DeviceStats target = _sniffer.getDevice(targetIdx);
        int8_t rssi = target.rssi;

        const char* cls;
        const char* label;
        if      (rssi > -45) { cls = "hot";  label = "HOT — Very Close!"; }
        else if (rssi > -60) { cls = "warm"; label = "WARM — Getting Closer"; }
        else if (rssi > -75) { cls = "cool"; label = "COOL — Moderate Distance"; }
        else                 { cls = "cold"; label = "COLD — Far Away"; }

        // Check if we know the vendor from OUI
        const char* vendor = WiFiSniffer::getOUIVendor(target.mac);
        String vendorStr = vendor ? String(vendor) : "Unknown vendor";

        html += "<div class='hotcold " + String(cls) + "'>"
            "<div class='temp-label'>" + String(label) + "</div>"
            "<div class='temp-rssi'>" + WiFiSniffer::macToString(target.mac)
            + " &nbsp;|&nbsp; RSSI: " + String(rssi) + " dBm</div>"
            "<div class='temp-vendor'>" + vendorStr
            + " &nbsp;|&nbsp; Ch " + String(target.channel)
            + " &nbsp;|&nbsp; Data: " + String(target.dataPkts) + "/" + String(target.totalPkts)
            + " &nbsp;|&nbsp; Large: " + String(target.largePkts) + " Small: " + String(target.smallPkts)
            + "</div></div>";
    } else {
        html += "<div class='hotcold none'>"
            "<div class='temp-label'>Scanning...</div>"
            "<div class='temp-rssi'>No suspicious transmissions detected yet.<br>"
            "Walk around — devices streaming video will be flagged.</div>"
            "</div>";
    }

    // ── Detected Devices Table ──
    int devCount = _sniffer.getDeviceCount();
    int susCount = _sniffer.getSuspiciousCount();

    html += "<h3>Detected Devices (" + String(devCount) + " total, "
        + String(susCount) + " suspicious)</h3>"
        "<table class='log'>"
        "<tr><th>#</th><th>MAC / Vendor</th><th>RSSI</th>"
        "<th>Pkts</th><th>Data%</th><th>Lg/Sm</th><th>Status</th></tr>";

    // Two passes: suspicious first, then normal
    for (int pass = 0; pass < 2; pass++) {
        for (int i = 0; i < devCount; i++) {
            bool sus = _sniffer.isSuspicious(i);
            if ((pass == 0 && !sus) || (pass == 1 && sus)) continue;

            DeviceStats d = _sniffer.getDevice(i);
            int dataPercent = (d.totalPkts > 0) ? (d.dataPkts * 100 / d.totalPkts) : 0;

            // Build MAC cell with vendor info if known
            const char* vendor = WiFiSniffer::getOUIVendor(d.mac);
            String macCell = WiFiSniffer::macToString(d.mac);
            if (vendor) {
                macCell += "<br><span class='cam-oui'>" + String(vendor) + "</span>";
            }

            // Build status cell with reason
            String statusCell;
            if (sus) {
                statusCell = "<span class='suspect'>SUSPECT</span><br><span class='reason'>";
                if (d.knownCamOUI && d.dataPkts > 5) statusCell += "Camera OUI ";
                if (d.totalPkts >= SUSPECT_MIN_PACKETS && (d.dataPkts * 100 / d.totalPkts) >= SUSPECT_DATA_RATIO)
                    statusCell += "High data ";
                if (d.largePkts >= SUSPECT_BIMODAL_MIN && d.smallPkts >= SUSPECT_BIMODAL_MIN)
                    statusCell += "Video sig ";
                statusCell += "</span>";
            } else {
                statusCell = "<span class='normal'>Normal</span>";
            }

            html += "<tr><td>" + String(i + 1) + "</td>"
                "<td>" + macCell + "</td>"
                "<td>" + String(d.rssi) + "</td>"
                "<td>" + String(d.totalPkts) + "</td>"
                "<td>" + String(dataPercent) + "%</td>"
                "<td>" + String(d.largePkts) + "/" + String(d.smallPkts) + "</td>"
                "<td>" + statusCell + "</td></tr>";
        }
    }

    html += "</table>"
        "<p class='note'>Auto-refreshes every 2s<br>"
        "<b>Detection methods:</b> High data ratio (&gt;" + String(SUSPECT_DATA_RATIO) + "%), "
        "Video frame signature (bimodal packet sizes), "
        "Known camera OUI (Hikvision, Dahua, Espressif, Wyze, Tuya)<br>"
        "Walk toward the HOT signal to locate the transmitting device.</p>"
        "<nav><a href='/sniffer'>Sniffer</a><a href='/jam'>Jam</a><a href='/sensor'>Sensor</a><a href='/gps'>GPS</a></nav>"
        "</body></html>";

    _server.send(200, "text/html", html);
}

void WebMenuServer::handleGPS() {
    String html;
    html.reserve(1024);
    html = "<!DOCTYPE html><html><head>"
        "<meta charset='UTF-8'>"
        "<meta name='viewport' content='width=device-width,initial-scale=1'>"
        "<meta http-equiv='refresh' content='5'>"
        "<title>GPS</title>" + String(CSS) + "</head><body>"
        "<h2>GPS</h2>"
        "<table>"
        "<tr><td>Latitude</td><td>" + String(_gps.getLatitude(), 6) + "</td></tr>"
        "<tr><td>Longitude</td><td>" + String(_gps.getLongitude(), 6) + "</td></tr>"
        "<tr><td>Altitude</td><td>" + String(_gps.getAltitude(), 1) + " m</td></tr>"
        "<tr><td>Speed</td><td>" + String(_gps.getSpeed(), 1) + " km/h</td></tr>"
        "<tr><td>Date / Time (UTC)</td><td>" + _gps.getDateTime() + "</td></tr>"
        "</table>"
        "<p class='note'>Auto-refreshes every 5s</p>"
        "<nav><a href='/sniffer'>Sniffer</a><a href='/jam'>Jam</a><a href='/sensor'>Sensor</a><a href='/gps'>GPS</a></nav>"
        "</body></html>";
    _server.send(200, "text/html", html);
}

void WebMenuServer::handleJam() {
    String html;
    html.reserve(3072);
    html = "<!DOCTYPE html><html><head>"
        "<meta charset='UTF-8'>"
        "<meta name='viewport' content='width=device-width,initial-scale=1'>"
        "<meta http-equiv='refresh' content='2'>"
        "<title>Strawberry Jam</title>" + String(CSS) +
        "<style>"
        ".jam-status{text-align:center;padding:20px;border-radius:12px;margin:12px 0;}"
        ".jam-active{background:linear-gradient(135deg,#ff0000,#cc0000);color:#fff;}"
        ".jam-idle{background:#2a2a4a;color:#888;}"
        ".jam-label{font-size:24px;font-weight:bold;}"
        ".jam-count{font-size:40px;font-weight:bold;color:#ff4444;margin:8px 0;}"
        ".warn{color:#ff8800;font-size:12px;padding:8px;border:1px solid #ff8800;border-radius:4px;margin:8px 0;}"
        "</style>"
        "</head><body>"
        "<h2 style='color:#ff4444;'>Strawberry Jam</h2>"
        "<p class='warn'>FOR AUTHORIZED TESTING ONLY. Use only on your own network.</p>";

    if (_jam && _jam->isRunning()) {
        html += "<div class='jam-status jam-active'>"
            "<div class='jam-label'>ACTIVE</div>"
            "<div class='jam-count'>" + String(_jam->getFramesSent()) + "</div>"
            "<div>deauth frames sent</div></div>";
    } else {
        html += "<div class='jam-status jam-idle'>"
            "<div class='jam-label'>IDLE</div>"
            "<div style='margin-top:8px;'>Select Strawberry Jam on the TFT to start.</div></div>";
    }

    // Show device list as potential targets
    int devCount = _sniffer.getDeviceCount();
    html += "<h3>Detected Devices (" + String(devCount) + ")</h3>"
        "<table class='log'>"
        "<tr><th>#</th><th>MAC Address</th><th>RSSI</th><th>Pkts</th><th>Ch</th></tr>";

    for (int i = 0; i < devCount; i++) {
        DeviceStats d = _sniffer.getDevice(i);
        html += "<tr><td>" + String(i + 1) + "</td>"
            "<td>" + WiFiSniffer::macToString(d.mac) + "</td>"
            "<td>" + String(d.rssi) + "</td>"
            "<td>" + String(d.totalPkts) + "</td>"
            "<td>" + String(d.channel) + "</td></tr>";
    }

    html += "</table>"
        "<p class='note'>Auto-refreshes every 2s | Attack auto-targets strongest signal device</p>"
        "<nav><a href='/sniffer'>Sniffer</a><a href='/jam'>Jam</a><a href='/sensor'>Sensor</a><a href='/gps'>GPS</a></nav>"
        "</body></html>";

    _server.send(200, "text/html", html);
}

void WebMenuServer::handleNotFound() {
    _server.send(404, "text/plain", "404: Not Found — URI: " + _server.uri());
}
