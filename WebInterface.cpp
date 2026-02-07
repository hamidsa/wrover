#include "WebInterface.h"
#include "ConfigManager.h"
#include "WiFiManager.h"
#include "DataManager.h"
#include "AlertManager.h"
#include "BuzzerManager.h"
#include "LEDManager.h"
#include "DisplayManager.h"
#include "BatteryManager.h"
#include "TimeManager.h"
#include "APIManager.h"
// در ابتدای فایل WebInterface.cpp
#include "DataManager.h"
#include <ArduinoJson.h>
#include <FS.h>
#include <SPIFFS.h>
#include <Update.h>

// ===== STATIC VARIABLES =====
WebServer WebInterface::_server(80);
bool WebInterface::_spiffsInitialized = false;
String WebInterface::_authUsername = "";
String WebInterface::_authPassword = "";
bool WebInterface::_authEnabled = false;

// ===== INITIALIZATION =====
bool WebInterface::begin(bool enableAuth, const String& username, const String& password) {
    Serial.println("Initializing Web Interface...");
    
    // Initialize SPIFFS for web files
    if (!initSPIFFS()) {
        Serial.println("Failed to initialize SPIFFS");
        return false;
    }
    
    // Setup authentication if enabled
    _authEnabled = enableAuth;
    _authUsername = username;
    _authPassword = password;
    
    // Setup all routes
    setupRoutes();
    
    // Start server
    _server.begin();
    
    Serial.println("Web Interface initialized");
    Serial.print("Server started on port 80");
    if (_authEnabled) {
        Serial.print(" (Authentication enabled)");
    }
    Serial.println();
    
    return true;
}

void WebInterface::update() {
    _server.handleClient();
}

// ===== SPIFFS INITIALIZATION =====
bool WebInterface::initSPIFFS() {
    if (_spiffsInitialized) return true;
    
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS Mount Failed");
        return false;
    }
    
    // List files for debugging
    Serial.println("SPIFFS Files:");
    File root = SPIFFS.open("/");
    File file = root.openNextFile();
    while (file) {
        Serial.print("  ");
        Serial.print(file.name());
        Serial.print(" (");
        Serial.print(file.size());
        Serial.println(" bytes)");
        file = root.openNextFile();
    }
    
    _spiffsInitialized = true;
    return true;
}

// ===== ROUTE SETUP =====
void WebInterface::setupRoutes() {
    // API Routes
    setupAPIRoutes();
    
    // Web Page Routes
    setupPageRoutes();
    
    // File Server Routes
    setupFileRoutes();
    
    // System Routes
    setupSystemRoutes();
    
    // 404 Handler
    _server.onNotFound([]() {
        if (!handleFileRead(_server.uri())) {
            handleNotFound();
        }
    });
}

void WebInterface::setupAPIRoutes() {
    // System API
    _server.on("/api/system/status", HTTP_GET, handleSystemStatus);
    _server.on("/api/system/info", HTTP_GET, handleSystemInfo);
    _server.on("/api/system/restart", HTTP_POST, handleSystemRestart);
    _server.on("/api/system/factory-reset", HTTP_POST, handleFactoryReset);
    _server.on("/api/system/update", HTTP_POST, handleSystemUpdate);
    
    // WiFi API
    _server.on("/api/wifi/scan", HTTP_GET, handleWiFiScan);
    _server.on("/api/wifi/connect", HTTP_POST, handleWiFiConnect);
    _server.on("/api/wifi/disconnect", HTTP_POST, handleWiFiDisconnect);
    _server.on("/api/wifi/status", HTTP_GET, handleWiFiStatus);
    _server.on("/api/wifi/networks", HTTP_GET, handleWiFiNetworks);
    _server.on("/api/wifi/ap/toggle", HTTP_POST, handleAPToggle);
    
    // Settings API
    _server.on("/api/settings/get", HTTP_GET, handleSettingsGet);
    _server.on("/api/settings/save", HTTP_POST, handleSettingsSave);
    _server.on("/api/settings/reset", HTTP_POST, handleSettingsReset);
    
    // Data API
    _server.on("/api/data/positions", HTTP_GET, handleDataPositions);
    _server.on("/api/data/summary", HTTP_GET, handleDataSummary);
    _server.on("/api/data/refresh", HTTP_POST, handleDataRefresh);
    _server.on("/api/data/history", HTTP_GET, handleDataHistory);
    
    // Alert API
    _server.on("/api/alerts/status", HTTP_GET, handleAlertsStatus);
    _server.on("/api/alerts/history", HTTP_GET, handleAlertsHistory);
    _server.on("/api/alerts/reset", HTTP_POST, handleAlertsReset);
    _server.on("/api/alerts/test", HTTP_POST, handleAlertsTest);
    
    // Device Control API
    _server.on("/api/device/buzzer", HTTP_POST, handleBuzzerControl);
    _server.on("/api/device/leds", HTTP_POST, handleLEDControl);
    _server.on("/api/device/display", HTTP_POST, handleDisplayControl);
    _server.on("/api/device/rgb", HTTP_POST, handleRGBControl);
    
    // Battery API
    _server.on("/api/battery/status", HTTP_GET, handleBatteryStatus);
    
    // Time API
    _server.on("/api/time/current", HTTP_GET, handleTimeCurrent);
    _server.on("/api/time/sync", HTTP_POST, handleTimeSync);
    
    // Logs API
    _server.on("/api/logs/system", HTTP_GET, handleSystemLogs);
    _server.on("/api/logs/clear", HTTP_POST, handleLogsClear);
}

void WebInterface::setupPageRoutes() {
    // Main pages
    _server.on("/", HTTP_GET, []() {
        if (!handleFileRead("/index.html")) {
            handleNotFound();
        }
    });
    
    _server.on("/dashboard", HTTP_GET, []() {
        if (!handleFileRead("/dashboard.html")) {
            handleNotFound();
        }
    });
    
    _server.on("/setup", HTTP_GET, []() {
        if (!handleFileRead("/setup.html")) {
            handleNotFound();
        }
    });
    
    _server.on("/wifi", HTTP_GET, []() {
        if (!handleFileRead("/wifi.html")) {
            handleNotFound();
        }
    });
    
    _server.on("/alerts", HTTP_GET, []() {
        if (!handleFileRead("/alerts.html")) {
            handleNotFound();
        }
    });
    
    _server.on("/settings", HTTP_GET, []() {
        if (!handleFileRead("/settings.html")) {
            handleNotFound();
        }
    });
    
    _server.on("/system", HTTP_GET, []() {
        if (!handleFileRead("/system.html")) {
            handleNotFound();
        }
    });
    
    _server.on("/logs", HTTP_GET, []() {
        if (!handleFileRead("/logs.html")) {
            handleNotFound();
        }
    });
}

void WebInterface::setupFileRoutes() {
    // Serve static files
    _server.on("/styles.css", HTTP_GET, []() {
        handleFileRead("/styles.css");
    });
    
    _server.on("/script.js", HTTP_GET, []() {
        handleFileRead("/script.js");
    });
    
    _server.on("/favicon.ico", HTTP_GET, []() {
        handleFileRead("/favicon.ico");
    });
    
    // Serve any other static files
    _server.on("/assets/", HTTP_GET, []() {
        handleFileRead(_server.uri());
    });
}

void WebInterface::setupSystemRoutes() {
    // OTA Update
    _server.on("/update", HTTP_GET, []() {
        if (!handleFileRead("/update.html")) {
            handleNotFound();
        }
    });
    
    _server.on("/update", HTTP_POST, []() {
        _server.sendHeader("Connection", "close");
        _server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
        ESP.restart();
    }, []() {
        HTTPUpload& upload = _server.upload();
        if (upload.status == UPLOAD_FILE_START) {
            Serial.printf("Update: %s\n", upload.filename.c_str());
            if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
                Update.printError(Serial);
            }
        } else if (upload.status == UPLOAD_FILE_WRITE) {
            if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
                Update.printError(Serial);
            }
        } else if (upload.status == UPLOAD_FILE_END) {
            if (Update.end(true)) {
                Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
            } else {
                Update.printError(Serial);
            }
        }
    });
}

// ===== AUTHENTICATION =====
bool WebInterface::checkAuth() {
    if (!_authEnabled) return true;
    
    if (!_server.authenticate(_authUsername.c_str(), _authPassword.c_str())) {
        _server.requestAuthentication();
        return false;
    }
    return true;
}

// ===== FILE HANDLING =====
bool WebInterface::handleFileRead(String path) {
    Serial.println("handleFileRead: " + path);
    
    if (path.endsWith("/")) {
        path += "index.html";
    }
    
    String contentType = getContentType(path);
    String pathWithGz = path + ".gz";
    
    if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) {
        if (SPIFFS.exists(pathWithGz)) {
            path += ".gz";
        }
        
        File file = SPIFFS.open(path, "r");
        if (!file) {
            Serial.println("Failed to open file: " + path);
            return false;
        }
        
        size_t sent = _server.streamFile(file, contentType);
        file.close();
        
        Serial.println(String("Sent file: ") + path + " (" + sent + " bytes)");
        return true;
    }
    
    Serial.println("File not found: " + path);
    return false;
}

String WebInterface::getContentType(String filename) {
    if (filename.endsWith(".html")) return "text/html";
    if (filename.endsWith(".css")) return "text/css";
    if (filename.endsWith(".js")) return "application/javascript";
    if (filename.endsWith(".json")) return "application/json";
    if (filename.endsWith(".png")) return "image/png";
    if (filename.endsWith(".jpg")) return "image/jpeg";
    if (filename.endsWith(".ico")) return "image/x-icon";
    if (filename.endsWith(".gz")) return "application/x-gzip";
    return "text/plain";
}

// ===== API HANDLERS =====

// System Status Handler
void WebInterface::handleSystemStatus() {
    if (!checkAuth()) return;
    
    DynamicJsonDocument doc(1024);
    
    doc["status"] = "online";
    doc["uptime"] = millis() / 1000;
    doc["freeHeap"] = ESP.getFreeHeap();
    doc["chipId"] = String((uint32_t)ESP.getEfuseMac(), HEX);
    doc["sdkVersion"] = ESP.getSdkVersion();
    doc["cpuFreq"] = ESP.getCpuFreqMHz();
    doc["flashSize"] = ESP.getFlashChipSize();
    
    String response;
    serializeJson(doc, response);
    _server.send(200, "application/json", response);
}

// System Info Handler
void WebInterface::handleSystemInfo() {
    if (!checkAuth()) return;
    
    DynamicJsonDocument doc(2048);
    
    // Hardware info
    doc["hardware"]["model"] = "ESP32-WROVER-E";
    doc["hardware"]["chipId"] = String((uint32_t)ESP.getEfuseMac(), HEX);
    doc["hardware"]["cpuFreq"] = ESP.getCpuFreqMHz();
    doc["hardware"]["flashSize"] = ESP.getFlashChipSize();
    doc["hardware"]["freeHeap"] = ESP.getFreeHeap();
    doc["hardware"]["minHeap"] = ESP.getMinFreeHeap();
    doc["hardware"]["maxHeap"] = ESP.getMaxAllocHeap();
    
    // Software info
    doc["software"]["version"] = "4.5.3";
    doc["software"]["buildDate"] = __DATE__ " " __TIME__;
    doc["software"]["sdkVersion"] = ESP.getSdkVersion();
    
    // Network info
    doc["network"]["mac"] = WiFi.macAddress();
    doc["network"]["hostname"] = WiFi.getHostname();
    
    if (WiFiManager::getInstance().isConnected()) {
        doc["network"]["connected"] = true;
        doc["network"]["ssid"] = WiFiManager::getInstance().getCurrentSSID();
        doc["network"]["rssi"] = WiFiManager::getInstance().getCurrentRSSI();
        doc["network"]["ip"] = WiFi.localIP().toString();
        doc["network"]["gateway"] = WiFi.gatewayIP().toString();
        doc["network"]["subnet"] = WiFi.subnetMask().toString();
        doc["network"]["dns"] = WiFi.dnsIP().toString();
    } else {
        doc["network"]["connected"] = false;
    }
    
    if (WiFiManager::getInstance().isAPMode()) {
        doc["network"]["apMode"] = true;
        doc["network"]["apSSID"] = WiFiManager::getInstance().getAPSSID();
        doc["network"]["apIP"] = WiFiManager::getInstance().getAPIP().toString();
    } else {
        doc["network"]["apMode"] = false;
    }
    
    // Component status
    doc["components"]["wifi"] = true;
    doc["components"]["spiffs"] = _spiffsInitialized;
    doc["components"]["display"] = DisplayManager::getInstance().isInitialized();
    doc["components"]["buzzer"] = BuzzerManager::getInstance().isEnabled();
    doc["components"]["leds"] = LEDManager::getInstance().isEnabled();
    doc["components"]["battery"] = BatteryManager::getInstance().isInitialized();
    doc["components"]["time"] = TimeManager::getInstance().isSynced();
    
    String response;
    serializeJson(doc, response);
    _server.send(200, "application/json", response);
}

// WiFi Scan Handler
void WebInterface::handleWiFiScan() {
    if (!checkAuth()) return;
    
    WiFiManager::getInstance().scanNetworks(true);
    auto networks = WiFiManager::getInstance().getScannedNetworks();
    
    DynamicJsonDocument doc(4096);
    JsonArray array = doc.to<JsonArray>();
    
    for (const auto& net : networks) {
        JsonObject obj = array.createNestedObject();
        obj["ssid"] = net.ssid;
        obj["rssi"] = net.rssi;
        obj["secured"] = net.encrypted;
        obj["saved"] = net.saved;
        obj["autoConnect"] = net.autoConnect;
    }
    
    String response;
    serializeJson(doc, response);
    _server.send(200, "application/json", response);
}

// WiFi Connect Handler
void WebInterface::handleWiFiConnect() {
    if (!checkAuth()) return;
    
    if (!_server.hasArg("plain")) {
        _server.send(400, "application/json", "{\"error\":\"No data\"}");
        return;
    }
    
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, _server.arg("plain"));
    
    if (error) {
        _server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
    }
    
    String ssid = doc["ssid"] | "";
    String password = doc["password"] | "";
    uint8_t priority = doc["priority"] | 5;
    bool autoConnect = doc["autoConnect"] | true;
    bool connectNow = doc["connectNow"] | true;
    
    if (ssid.isEmpty()) {
        _server.send(400, "application/json", "{\"error\":\"SSID required\"}");
        return;
    }
    
    bool success = WiFiManager::getInstance().addNetwork(ssid, password, priority, autoConnect);
    
    if (success && connectNow) {
        success = WiFiManager::getInstance().connectToNetwork(ssid);
    }
    
    if (success) {
        _server.send(200, "application/json", "{\"success\":true}");
    } else {
        _server.send(500, "application/json", "{\"error\":\"Failed to connect\"}");
    }
}

// Data Positions Handler
void WebInterface::handleDataPositions() {
    if (!checkAuth()) return;
    
    String mode = _server.arg("mode");
    bool exitMode = (mode == "exit");
    
    auto positions = DataManager::getInstance().getPositions(exitMode);
    auto summary = DataManager::getInstance().getSummary(exitMode);
    
    DynamicJsonDocument doc(8192);
    
    // Summary
    JsonObject summaryObj = doc.createNestedObject("summary");
    summaryObj["totalInvestment"] = summary.totalInvestment;
    summaryObj["totalCurrentValue"] = summary.totalCurrentValue;
    summaryObj["totalPnl"] = summary.totalPnl;
    summaryObj["totalPnlPercent"] = summary.totalPnlPercent;
    summaryObj["totalPositions"] = summary.totalPositions;
    summaryObj["longPositions"] = summary.longPositions;
    summaryObj["shortPositions"] = summary.shortPositions;
    summaryObj["winningPositions"] = summary.winningPositions;
    summaryObj["losingPositions"] = summary.losingPositions;
    
    // Positions
    JsonArray positionsArray = doc.createNestedArray("positions");
    
    for (const auto& pos : positions) {
        JsonObject posObj = positionsArray.createNestedObject();
        posObj["symbol"] = pos.symbol;
        posObj["changePercent"] = pos.changePercent;
        posObj["pnlValue"] = pos.pnlValue;
        posObj["quantity"] = pos.quantity;
        posObj["entryPrice"] = pos.entryPrice;
        posObj["currentPrice"] = pos.currentPrice;
        posObj["isLong"] = pos.isLong;
        posObj["alerted"] = pos.alerted;
        posObj["severeAlerted"] = pos.severeAlerted;
        posObj["lastAlertTime"] = pos.lastAlertTime;
    }
    
    String response;
    serializeJson(doc, response);
    _server.send(200, "application/json", response);
}

// Alert Status Handler
void WebInterface::handleAlertsStatus() {
    if (!checkAuth()) return;
    
    DynamicJsonDocument doc(1024);
    
    // Entry alerts
    JsonObject entryAlerts = doc.createNestedObject("entry");
    auto entryAlertHistory = AlertManager::getInstance().getAlertHistory(false);
    
    JsonArray entryArray = entryAlerts.createNestedArray("active");
    for (const auto& alert : entryAlertHistory) {
        if (!alert.acknowledged && (millis() - alert.alertTime < 3600000)) { // Last hour
            JsonObject alertObj = entryArray.createNestedObject();
            alertObj["symbol"] = alert.symbol;
            alertObj["pnlPercent"] = alert.pnlPercent;
            alertObj["alertPrice"] = alert.alertPrice;
            alertObj["isLong"] = alert.isLong;
            alertObj["isSevere"] = alert.isSevere;
            alertObj["alertTime"] = alert.alertTime;
            alertObj["message"] = alert.message;
        }
    }
    entryAlerts["count"] = entryArray.size();
    
    // Exit alerts
    JsonObject exitAlerts = doc.createNestedObject("exit");
    auto exitAlertHistory = AlertManager::getInstance().getAlertHistory(true);
    
    JsonArray exitArray = exitAlerts.createNestedArray("active");
    for (const auto& alert : exitAlertHistory) {
        if (!alert.acknowledged && (millis() - alert.alertTime < 3600000)) {
            JsonObject alertObj = exitArray.createNestedObject();
            alertObj["symbol"] = alert.symbol;
            alertObj["pnlPercent"] = alert.pnlPercent;
            alertObj["alertPrice"] = alert.alertPrice;
            alertObj["isProfit"] = alert.isProfit;
            alertObj["alertTime"] = alert.alertTime;
            alertObj["message"] = alert.message;
        }
    }
    exitAlerts["count"] = exitArray.size();
    
    String response;
    serializeJson(doc, response);
    _server.send(200, "application/json", response);
}

// Buzzer Control Handler
void WebInterface::handleBuzzerControl() {
    if (!checkAuth()) return;
    
    String action = _server.arg("action");
    String value = _server.arg("value");
    
    if (action == "volume") {
        uint8_t volume = value.toInt();
        BuzzerManager::getInstance().setVolume(volume);
        _server.send(200, "application/json", "{\"success\":true, \"volume\":" + String(volume) + "}");
    } 
    else if (action == "toggle") {
        BuzzerManager::getInstance().toggleEnabled();
        _server.send(200, "application/json", "{\"success\":true, \"enabled\":" + 
                    String(BuzzerManager::getInstance().isEnabled() ? "true" : "false") + "}");
    }
    else if (action == "test") {
        BuzzerManager::getInstance().testAllAlerts();
        _server.send(200, "application/json", "{\"success\":true}");
    }
    else if (action == "mute") {
        BuzzerManager::getInstance().mute();
        _server.send(200, "application/json", "{\"success\":true}");
    }
    else if (action == "unmute") {
        BuzzerManager::getInstance().unmute();
        _server.send(200, "application/json", "{\"success\":true}");
    }
    else {
        _server.send(400, "application/json", "{\"error\":\"Invalid action\"}");
    }
}

// Settings Get Handler
void WebInterface::handleSettingsGet() {
    if (!checkAuth()) return;
    
    String section = _server.arg("section");
    
    if (section == "all") {
        String settings = ConfigManager::getInstance().getAllSettingsJSON();
        _server.send(200, "application/json", settings);
    }
    else if (section == "wifi") {
        DynamicJsonDocument doc(1024);
        doc["ssid"] = ConfigManager::getInstance().getWiFiSSID();
        doc["apEnabled"] = ConfigManager::getInstance().getAPEnabled();
        doc["autoConnect"] = ConfigManager::getInstance().getWiFiAutoConnect();
        
        String response;
        serializeJson(doc, response);
        _server.send(200, "application/json", response);
    }
    else if (section == "alerts") {
        DynamicJsonDocument doc(512);
        doc["alertThreshold"] = ConfigManager::getInstance().getAlertThreshold();
        doc["severeThreshold"] = ConfigManager::getInstance().getSevereThreshold();
        doc["portfolioThreshold"] = ConfigManager::getInstance().getPortfolioThreshold();
        doc["buzzerVolume"] = ConfigManager::getInstance().getBuzzerVolume();
        doc["buzzerEnabled"] = ConfigManager::getInstance().getBuzzerEnabled();
        
        String response;
        serializeJson(doc, response);
        _server.send(200, "application/json", response);
    }
    else {
        _server.send(400, "application/json", "{\"error\":\"Invalid section\"}");
    }
}

// Settings Save Handler
void WebInterface::handleSettingsSave() {
    if (!checkAuth()) return;
    
    if (!_server.hasArg("plain")) {
        _server.send(400, "application/json", "{\"error\":\"No data\"}");
        return;
    }
    
    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, _server.arg("plain"));
    
    if (error) {
        _server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
    }
    
    // Save all settings from JSON
    if (doc.containsKey("wifi")) {
        JsonObject wifi = doc["wifi"];
        if (wifi.containsKey("ssid")) ConfigManager::getInstance().setWiFiSSID(wifi["ssid"].as<String>());
        if (wifi.containsKey("password")) ConfigManager::getInstance().setWiFiPassword(wifi["password"].as<String>());
        if (wifi.containsKey("apEnabled")) ConfigManager::getInstance().setAPEnabled(wifi["apEnabled"]);
        if (wifi.containsKey("autoConnect")) ConfigManager::getInstance().setWiFiAutoConnect(wifi["autoConnect"]);
    }
    
    if (doc.containsKey("api")) {
        JsonObject api = doc["api"];
        if (api.containsKey("server")) ConfigManager::getInstance().setAPIServer(api["server"].as<String>());
        if (api.containsKey("username")) ConfigManager::getInstance().setAPIUsername(api["username"].as<String>());
        if (api.containsKey("password")) ConfigManager::getInstance().setAPIPassword(api["password"].as<String>());
        if (api.containsKey("entryPortfolio")) ConfigManager::getInstance().setEntryPortfolio(api["entryPortfolio"].as<String>());
        if (api.containsKey("exitPortfolio")) ConfigManager::getInstance().setExitPortfolio(api["exitPortfolio"].as<String>());
    }
    
    if (doc.containsKey("alerts")) {
        JsonObject alerts = doc["alerts"];
        if (alerts.containsKey("alertThreshold")) ConfigManager::getInstance().setAlertThreshold(alerts["alertThreshold"].as<float>());
        if (alerts.containsKey("severeThreshold")) ConfigManager::getInstance().setSevereThreshold(alerts["severeThreshold"].as<float>());
        if (alerts.containsKey("portfolioThreshold")) ConfigManager::getInstance().setPortfolioThreshold(alerts["portfolioThreshold"].as<float>());
        if (alerts.containsKey("buzzerVolume")) ConfigManager::getInstance().setBuzzerVolume(alerts["buzzerVolume"].as<uint8_t>());
        if (alerts.containsKey("buzzerEnabled")) ConfigManager::getInstance().setBuzzerEnabled(alerts["buzzerEnabled"].as<bool>());
    }
    
    // Save other sections similarly...
    
    ConfigManager::getInstance().save();
    
    _server.send(200, "application/json", "{\"success\":true}");
}

// Battery Status Handler
void WebInterface::handleBatteryStatus() {
    if (!checkAuth()) return;
    
    DynamicJsonDocument doc(512);
    
    doc["voltage"] = BatteryManager::getInstance().getVoltage();
    doc["percentage"] = BatteryManager::getInstance().getPercentage();
    doc["charging"] = BatteryManager::getInstance().isCharging();
    doc["health"] = BatteryManager::getInstance().getHealth();
    doc["status"] = BatteryManager::getInstance().getStatusString();
    
    String response;
    serializeJson(doc, response);
    _server.send(200, "application/json", response);
}

// Time Current Handler
void WebInterface::handleTimeCurrent() {
    if (!checkAuth()) return;
    
    DynamicJsonDocument doc(512);
    
    doc["timestamp"] = TimeManager::getInstance().getTimestamp();
    doc["formatted"] = TimeManager::getInstance().getFormattedTime();
    doc["date"] = TimeManager::getInstance().getFormattedDate();
    doc["synced"] = TimeManager::getInstance().isSynced();
    doc["timezone"] = TimeManager::getInstance().getTimezone();
    
    String response;
    serializeJson(doc, response);
    _server.send(200, "application/json", response);
}

// System Update Handler
void WebInterface::handleSystemUpdate() {
    if (!checkAuth()) return;
    
    if (!_server.hasArg("url")) {
        _server.send(400, "application/json", "{\"error\":\"No URL provided\"}");
        return;
    }
    
    String url = _server.arg("url");
    
    // Start OTA update in background
    _server.send(200, "application/json", "{\"success\":true, \"message\":\"Update started\"}");
    
    // Note: Actual OTA implementation would go here
    Serial.println("OTA Update requested for: " + url);
}

// System Restart Handler
void WebInterface::handleSystemRestart() {
    if (!checkAuth()) return;
    
    _server.send(200, "application/json", "{\"success\":true, \"message\":\"Restarting...\"}");
    delay(1000);
    ESP.restart();
}

// Factory Reset Handler
void WebInterface::handleFactoryReset() {
    if (!checkAuth()) return;
    
    ConfigManager::getInstance().factoryReset();
    
    _server.send(200, "application/json", "{\"success\":true, \"message\":\"Factory reset complete. Restarting...\"}");
    delay(1000);
    ESP.restart();
}

// 404 Handler
void WebInterface::handleNotFound() {
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += _server.uri();
    message += "\nMethod: ";
    message += (_server.method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += _server.args();
    message += "\n";
    
    for (uint8_t i = 0; i < _server.args(); i++) {
        message += " " + _server.argName(i) + ": " + _server.arg(i) + "\n";
    }
    
    _server.send(404, "text/plain", message);
}

// ===== UTILITY FUNCTIONS =====
String WebInterface::generateDashboardHTML() {
    // This function generates dynamic dashboard HTML
    // In practice, you would use template files from SPIFFS
    
    String html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Portfolio Monitor Dashboard</title>
    <link rel="stylesheet" href="/styles.css">
    <script src="/script.js"></script>
</head>
<body>
    <div class="container">
        <header>
            <h1>Portfolio Monitor Dashboard</h1>
            <div class="status-bar">
                <span id="wifi-status">Connecting...</span>
                <span id="battery-status">100%</span>
                <span id="time">00:00:00</span>
            </div>
        </header>
        
        <main>
            <div class="dashboard-grid">
                <div class="card">
                    <h2>Entry Mode</h2>
                    <div class="stats" id="entry-stats">
                        Loading...
                    </div>
                </div>
                
                <div class="card">
                    <h2>Exit Mode</h2>
                    <div class="stats" id="exit-stats">
                        Loading...
                    </div>
                </div>
                
                <div class="card">
                    <h2>Alerts</h2>
                    <div class="alerts" id="alerts-list">
                        No active alerts
                    </div>
                </div>
                
                <div class="card">
                    <h2>Quick Actions</h2>
                    <div class="actions">
                        <button onclick="refreshData()">Refresh Data</button>
                        <button onclick="testAlerts()">Test Alerts</button>
                        <button onclick="openSettings()">Settings</button>
                    </div>
                </div>
            </div>
        </main>
        
        <footer>
            <p>Portfolio Monitor v4.5.3 | ESP32-WROVER-E</p>
        </footer>
    </div>
    
    <script>
        // JavaScript for dynamic updates
        function updateDashboard() {
            fetch('/api/data/summary')
                .then(response => response.json())
                .then(data => {
                    // Update UI with data
                    document.getElementById('entry-stats').innerHTML = 
                        `Positions: ${data.entry.totalPositions}<br>
                         P/L: ${data.entry.totalPnlPercent.toFixed(2)}%<br>
                         Value: $${data.entry.totalCurrentValue.toFixed(2)}`;
                    
                    document.getElementById('exit-stats').innerHTML = 
                        `Positions: ${data.exit.totalPositions}<br>
                         P/L: ${data.exit.totalPnlPercent.toFixed(2)}%<br>
                         Value: $${data.exit.totalCurrentValue.toFixed(2)}`;
                });
            
            fetch('/api/alerts/status')
                .then(response => response.json())
                .then(data => {
                    // Update alerts
                    let alertsHtml = '';
                    if (data.entry.count > 0 || data.exit.count > 0) {
                        alertsHtml = '<ul>';
                        data.entry.active.forEach(alert => {
                            alertsHtml += `<li>${alert.symbol}: ${alert.message}</li>`;
                        });
                        data.exit.active.forEach(alert => {
                            alertsHtml += `<li>${alert.symbol}: ${alert.message}</li>`;
                        });
                        alertsHtml += '</ul>';
                    } else {
                        alertsHtml = 'No active alerts';
                    }
                    document.getElementById('alerts-list').innerHTML = alertsHtml;
                });
            
            fetch('/api/system/status')
                .then(response => response.json())
                .then(data => {
                    // Update status bar
                    document.getElementById('wifi-status').textContent = 
                        data.network.connected ? 'WiFi Connected' : 'WiFi Disconnected';
                });
        }
        
        // Update every 10 seconds
        setInterval(updateDashboard, 10000);
        updateDashboard(); // Initial update
    </script>
</body>
</html>
)rawliteral";
    
    return html;
}

// ===== STATIC ACCESS =====
WebInterface& WebInterface::getInstance() {
    static WebInterface instance;
    return instance;
}

void WebInterface::handleClient() {
    _server.handleClient();
}