#include "WiFiManager.h"
#include "ConfigManager.h"
#include "ArduinoJson.h"
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <vector>
#include <algorithm>

// ===== CONSTANTS =====
#define WIFI_SCAN_INTERVAL 60000        // 1 minute
#define CONNECTION_TIMEOUT 20000        // 20 seconds
#define RECONNECT_INTERVAL 30000        // 30 seconds
#define MAX_RECONNECT_ATTEMPTS 5
#define DNS_PORT 53
#define AP_CHANNEL 1
#define AP_MAX_CONNECTIONS 4

// ===== STATIC VARIABLES =====
DNSServer WiFiManager::_dnsServer;
WebServer WiFiManager::_webServer(80);
std::vector<WiFiNetwork> WiFiManager::_savedNetworks;
std::vector<WiFiNetwork> WiFiManager::_scannedNetworks;
WiFiManagerState WiFiManager::_state;
String WiFiManager::_apSSID;
String WiFiManager::_apPassword;
bool WiFiManager::_apEnabled = true;
bool WiFiManager::_initialized = false;
unsigned long WiFiManager::_lastScanTime = 0;
unsigned long WiFiManager::_lastConnectionAttempt = 0;
unsigned long WiFiManager::_connectionStartTime = 0;
uint8_t WiFiManager::_reconnectAttempts = 0;
String WiFiManager::_currentSSID = "";
int WiFiManager::_currentRSSI = 0;
IPAddress WiFiManager::_apIP(192, 168, 4, 1);
IPAddress WiFiManager::_apGateway(192, 168, 4, 1);
IPAddress WiFiManager::_apSubnet(255, 255, 255, 0);

// ===== INITIALIZATION =====
bool WiFiManager::begin(bool enableCaptivePortal) {
    Serial.println("Initializing WiFi Manager...");
    
    // Generate unique AP SSID
    uint32_t chipId = 0;
    for(int i=0; i<17; i=i+8) {
        chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
    }
    _apSSID = "PortfolioMonitor_" + String(chipId, HEX);
    _apPassword = "12345678";
    
    // Load saved networks
    loadNetworks();
    
    // Set initial state
    _state = WIFI_STATE_DISCONNECTED;
    _apEnabled = ConfigManager::getInstance().getBool("ap_enabled", true);
    
    // Start in AP mode initially for configuration
    if (_apEnabled) {
        startAPMode();
    }
    
    // Try to connect to saved networks if any
    if (_savedNetworks.size() > 0) {
        connectToBestNetwork();
    }
    
    // Setup web server
    setupWebServer();
    
    // Setup mDNS if connected
    if (WiFi.status() == WL_CONNECTED) {
        setupMDNS();
    }
    
    // Setup captive portal if enabled
    if (enableCaptivePortal && _apEnabled) {
        setupCaptivePortal();
    }
    
    _initialized = true;
    Serial.println("WiFi Manager initialized");
    printStatus();
    
    return true;
}

void WiFiManager::update() {
    if (!_initialized) return;
    
    unsigned long currentTime = millis();
    
    // Handle DNS for captive portal
    if (_state == WIFI_STATE_AP_MODE || _state == WIFI_STATE_AP_STA_MODE) {
        _dnsServer.processNextRequest();
    }
    
    // Handle web server
    _webServer.handleClient();
    
    // Check WiFi status
    checkConnectionStatus();
    
    // Auto-reconnect logic
    if (_state == WIFI_STATE_DISCONNECTED && 
        _apEnabled && 
        _savedNetworks.size() > 0 &&
        (currentTime - _lastConnectionAttempt > RECONNECT_INTERVAL)) {
        
        if (_reconnectAttempts < MAX_RECONNECT_ATTEMPTS) {
            _reconnectAttempts++;
            Serial.print("Auto-reconnect attempt ");
            Serial.println(_reconnectAttempts);
            connectToBestNetwork();
        } else {
            // Max attempts reached, ensure AP is running
            if (!_apEnabled) {
                startAPMode();
            }
        }
    }
    
    // Periodic network scan
    if (currentTime - _lastScanTime > WIFI_SCAN_INTERVAL) {
        scanNetworks(false); // Non-blocking scan
    }
}

// ===== NETWORK MANAGEMENT =====
bool WiFiManager::addNetwork(const String& ssid, const String& password, 
                            uint8_t priority, bool autoConnect) {
    // Check if already exists
    for (auto& net : _savedNetworks) {
        if (net.ssid == ssid) {
            // Update existing
            net.password = password;
            net.priority = priority;
            net.autoConnect = autoConnect;
            Serial.print("Updated network: ");
            Serial.println(ssid);
            saveNetworks();
            return true;
        }
    }
    
    // Check max networks
    if (_savedNetworks.size() >= MAX_WIFI_NETWORKS) {
        // Remove lowest priority network
        auto it = std::min_element(_savedNetworks.begin(), _savedNetworks.end(),
            [](const WiFiNetwork& a, const WiFiNetwork& b) {
                return a.priority < b.priority;
            });
        
        Serial.print("Removing low priority network: ");
        Serial.println(it->ssid);
        _savedNetworks.erase(it);
    }
    
    // Add new network
    WiFiNetwork newNet;
    newNet.ssid = ssid;
    newNet.password = password;
    newNet.priority = priority;
    newNet.autoConnect = autoConnect;
    newNet.lastConnected = 0;
    newNet.connectionAttempts = 0;
    newNet.rssi = 0;
    
    _savedNetworks.push_back(newNet);
    
    // Sort by priority
    std::sort(_savedNetworks.begin(), _savedNetworks.end(),
        [](const WiFiNetwork& a, const WiFiNetwork& b) {
            return a.priority > b.priority;
        });
    
    saveNetworks();
    
    Serial.print("Added network: ");
    Serial.print(ssid);
    Serial.print(" (Priority: ");
    Serial.print(priority);
    Serial.println(")");
    
    return true;
}

bool WiFiManager::removeNetwork(const String& ssid) {
    auto it = std::remove_if(_savedNetworks.begin(), _savedNetworks.end(),
        [&ssid](const WiFiNetwork& net) {
            return net.ssid == ssid;
        });
    
    if (it != _savedNetworks.end()) {
        _savedNetworks.erase(it, _savedNetworks.end());
        saveNetworks();
        
        Serial.print("Removed network: ");
        Serial.println(ssid);
        return true;
    }
    
    return false;
}

bool WiFiManager::connectToNetwork(const String& ssid) {
    for (auto& net : _savedNetworks) {
        if (net.ssid == ssid) {
            return connectToWiFi(net);
        }
    }
    return false;
}

bool WiFiManager::connectToBestNetwork() {
    if (_savedNetworks.empty()) {
        Serial.println("No saved networks to connect to");
        return false;
    }
    
    // Scan for available networks first
    scanNetworks(true);
    
    // Find best available network
    WiFiNetwork* bestNet = nullptr;
    int bestScore = -1000;
    
    for (auto& savedNet : _savedNetworks) {
        if (!savedNet.autoConnect) continue;
        
        // Check if this network is in scan results
        for (auto& scannedNet : _scannedNetworks) {
            if (scannedNet.ssid == savedNet.ssid) {
                // Calculate score: priority * 100 + RSSI
                int score = (savedNet.priority * 100) + scannedNet.rssi;
                
                if (score > bestScore) {
                    bestScore = score;
                    bestNet = &savedNet;
                }
                break;
            }
        }
    }
    
    if (bestNet) {
        Serial.print("Best network selected: ");
        Serial.print(bestNet->ssid);
        Serial.print(" (Priority: ");
        Serial.print(bestNet->priority);
        Serial.print(", RSSI: ");
        Serial.print(getRSSIForNetwork(bestNet->ssid));
        Serial.println(" dBm)");
        
        return connectToWiFi(*bestNet);
    }
    
    Serial.println("No suitable networks found");
    return false;
}

// ===== AP MODE =====
bool WiFiManager::startAPMode() {
    Serial.println("Starting Access Point mode...");
    
    // Stop STA if connected
    if (WiFi.status() == WL_CONNECTED) {
        WiFi.disconnect(false);
        delay(100);
    }
    
    // Configure AP
    WiFi.mode(WIFI_AP);
    
    // Set AP configuration
    if (!WiFi.softAPConfig(_apIP, _apGateway, _apSubnet)) {
        Serial.println("Failed to configure AP");
        return false;
    }
    
    // Start AP
    if (!WiFi.softAP(_apSSID.c_str(), _apPassword.c_str(), AP_CHANNEL, 0, AP_MAX_CONNECTIONS)) {
        Serial.println("Failed to start AP");
        return false;
    }
    
    // Update state
    _state = WIFI_STATE_AP_MODE;
    _apEnabled = true;
    
    Serial.println("AP Started Successfully:");
    Serial.print("  SSID: ");
    Serial.println(_apSSID);
    Serial.print("  Password: ");
    Serial.println(_apPassword);
    Serial.print("  IP: ");
    Serial.println(WiFi.softAPIP().toString());
    Serial.print("  MAC: ");
    Serial.println(WiFi.softAPmacAddress());
    
    // Start DNS server for captive portal
    _dnsServer.start(DNS_PORT, "*", _apIP);
    
    return true;
}

void WiFiManager::stopAPMode() {
    Serial.println("Stopping AP mode...");
    
    _dnsServer.stop();
    WiFi.softAPdisconnect(true);
    _apEnabled = false;
    
    ConfigManager::getInstance().putBool("ap_enabled", false);
    
    Serial.println("AP mode stopped");
}

bool WiFiManager::startAPSTA() {
    Serial.println("Starting AP+STA mode...");
    
    // Configure both AP and STA
    WiFi.mode(WIFI_AP_STA);
    
    // Start AP
    if (!WiFi.softAP(_apSSID.c_str(), _apPassword.c_str())) {
        Serial.println("Failed to start AP in hybrid mode");
        return false;
    }
    
    _state = WIFI_STATE_AP_STA_MODE;
    _apEnabled = true;
    
    Serial.println("AP+STA mode started");
    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP().toString());
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.print("STA IP: ");
        Serial.println(WiFi.localIP().toString());
    }
    
    return true;
}

// ===== SCANNING =====
void WiFiManager::scanNetworks(bool blocking) {
    if (blocking) {
        performScan();
    } else {
        // Start async scan
        WiFi.scanNetworks(true);
        _lastScanTime = millis();
    }
}

void WiFiManager::performScan() {
    Serial.println("Scanning for WiFi networks...");
    
    // Disconnect during scan for better results
    bool wasConnected = (WiFi.status() == WL_CONNECTED);
    if (wasConnected) {
        WiFi.disconnect(false);
        delay(100);
    }
    
    // Perform scan
    int numNetworks = WiFi.scanNetworks(false, true);
    
    // Reconnect if we were connected
    if (wasConnected) {
        WiFi.reconnect();
    }
    
    _scannedNetworks.clear();
    
    if (numNetworks == 0) {
        Serial.println("No networks found");
        return;
    }
    
    Serial.print("Found ");
    Serial.print(numNetworks);
    Serial.println(" networks:");
    
    for (int i = 0; i < numNetworks; i++) {
        WiFiNetwork net;
        net.ssid = WiFi.SSID(i);
        net.rssi = WiFi.RSSI(i);
        net.encrypted = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
        
        // Check if this network is saved
        net.saved = false;
        net.autoConnect = false;
        for (const auto& saved : _savedNetworks) {
            if (saved.ssid == net.ssid) {
                net.saved = true;
                net.autoConnect = saved.autoConnect;
                break;
            }
        }
        
        _scannedNetworks.push_back(net);
        
        Serial.print("  ");
        Serial.print(i + 1);
        Serial.print(": ");
        Serial.print(net.ssid);
        Serial.print(" (");
        Serial.print(net.rssi);
        Serial.print(" dBm) ");
        Serial.print(net.encrypted ? "[Secured]" : "[Open]");
        Serial.println(net.saved ? " [Saved]" : "");
    }
    
    // Sort by RSSI (strongest first)
    std::sort(_scannedNetworks.begin(), _scannedNetworks.end(),
        [](const WiFiNetwork& a, const WiFiNetwork& b) {
            return a.rssi > b.rssi;
        });
    
    WiFi.scanDelete();
    _lastScanTime = millis();
}

// ===== CONNECTION MANAGEMENT =====
bool WiFiManager::connectToWiFi(WiFiNetwork& network) {
    Serial.println("\n========================================");
    Serial.print("Connecting to: ");
    Serial.println(network.ssid);
    Serial.println("========================================");
    
    // Disconnect first
    WiFi.disconnect(true);
    delay(500);
    
    // Set mode
    if (_apEnabled) {
        WiFi.mode(WIFI_AP_STA);
    } else {
        WiFi.mode(WIFI_STA);
    }
    
    // Configure WiFi
    WiFi.setAutoReconnect(true);
    WiFi.persistent(false);
    WiFi.setSleep(false);
    
    // Begin connection
    WiFi.begin(network.ssid.c_str(), network.password.c_str());
    
    _connectionStartTime = millis();
    _lastConnectionAttempt = millis();
    _state = WIFI_STATE_CONNECTING;
    
    // Wait for connection
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 40) {
        delay(500);
        attempts++;
        
        Serial.print(".");
        
        // Timeout check
        if (millis() - _connectionStartTime > CONNECTION_TIMEOUT) {
            Serial.println("\nConnection timeout");
            network.connectionAttempts++;
            _state = WIFI_STATE_DISCONNECTED;
            return false;
        }
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        // Success
        _state = WIFI_STATE_CONNECTED;
        _currentSSID = network.ssid;
        _currentRSSI = WiFi.RSSI();
        network.lastConnected = millis();
        network.connectionAttempts++;
        network.rssi = _currentRSSI;
        
        // Update AP mode if needed
        if (_apEnabled) {
            startAPSTA();
        }
        
        // Setup mDNS
        setupMDNS();
        
        Serial.println("\n✅ CONNECTED!");
        Serial.print("  IP Address: ");
        Serial.println(WiFi.localIP().toString());
        Serial.print("  Gateway: ");
        Serial.println(WiFi.gatewayIP().toString());
        Serial.print("  RSSI: ");
        Serial.print(_currentRSSI);
        Serial.println(" dBm");
        Serial.print("  Channel: ");
        Serial.println(WiFi.channel());
        
        saveNetworks();
        _reconnectAttempts = 0;
        
        return true;
    } else {
        // Failed
        Serial.println("\n❌ CONNECTION FAILED");
        network.connectionAttempts++;
        _state = WIFI_STATE_DISCONNECTED;
        
        // Ensure AP is running if enabled
        if (_apEnabled && _state != WIFI_STATE_AP_MODE) {
            startAPMode();
        }
        
        return false;
    }
}

void WiFiManager::disconnect() {
    Serial.println("Disconnecting from WiFi...");
    WiFi.disconnect(true);
    _state = WIFI_STATE_DISCONNECTED;
    _currentSSID = "";
    _currentRSSI = 0;
    
    if (_apEnabled) {
        startAPMode();
    }
}

// ===== STATUS CHECKS =====
void WiFiManager::checkConnectionStatus() {
    static wl_status_t lastStatus = WL_IDLE_STATUS;
    wl_status_t currentStatus = WiFi.status();
    
    if (currentStatus != lastStatus) {
        lastStatus = currentStatus;
        
        switch (currentStatus) {
            case WL_CONNECTED:
                if (_state != WIFI_STATE_CONNECTED) {
                    _state = WIFI_STATE_CONNECTED;
                    _currentRSSI = WiFi.RSSI();
                    Serial.println("WiFi connection established");
                }
                break;
                
            case WL_DISCONNECTED:
                if (_state == WIFI_STATE_CONNECTED) {
                    _state = WIFI_STATE_DISCONNECTED;
                    Serial.println("WiFi connection lost");
                    
                    // Start AP if enabled
                    if (_apEnabled && _state != WIFI_STATE_AP_MODE) {
                        startAPMode();
                    }
                }
                break;
                
            default:
                break;
        }
    }
    
    // Update RSSI periodically when connected
    if (_state == WIFI_STATE_CONNECTED && millis() % 5000 < 100) {
        _currentRSSI = WiFi.RSSI();
    }
}

// ===== WEB SERVER =====
void WiFiManager::setupWebServer() {
    // WiFi management endpoints
    _webServer.on("/api/wifi/scan", HTTP_GET, handleScanRequest);
    _webServer.on("/api/wifi/connect", HTTP_POST, handleConnectRequest);
    _webServer.on("/api/wifi/disconnect", HTTP_POST, handleDisconnectRequest);
    _webServer.on("/api/wifi/networks", HTTP_GET, handleNetworksRequest);
    _webServer.on("/api/wifi/status", HTTP_GET, handleStatusRequest);
    _webServer.on("/api/wifi/config", HTTP_POST, handleConfigRequest);
    _webServer.on("/api/wifi/remove", HTTP_POST, handleRemoveRequest);
    _webServer.on("/api/wifi/test", HTTP_POST, handleTestRequest);
    
    // AP control endpoints
    _webServer.on("/api/wifi/ap/start", HTTP_POST, handleAPStartRequest);
    _webServer.on("/api/wifi/ap/stop", HTTP_POST, handleAPStopRequest);
    _webServer.on("/api/wifi/ap/toggle", HTTP_POST, handleAPToggleRequest);
    
    Serial.println("WiFi web endpoints registered");
}

void WiFiManager::setupCaptivePortal() {
    // Redirect all captive portal requests
    _webServer.onNotFound([]() {
        if (isCaptivePortalRequest()) {
            _webServer.sendHeader("Location", "http://" + _apIP.toString(), true);
            _webServer.send(302, "text/plain", "");
        } else {
            _webServer.send(404, "text/plain", "Not found");
        }
    });
    
    Serial.println("Captive portal enabled");
}

bool WiFiManager::isCaptivePortalRequest() {
    String host = _webServer.hostHeader();
    return host != _apIP.toString() && host != ("http://" + _apIP.toString());
}

// ===== WEB HANDLERS =====
void WiFiManager::handleScanRequest() {
    performScan();
    
    DynamicJsonDocument doc(4096);
    JsonArray networks = doc.createNestedArray("networks");
    
    for (const auto& net : _scannedNetworks) {
        JsonObject network = networks.createNestedObject();
        network["ssid"] = net.ssid;
        network["rssi"] = net.rssi;
        network["secured"] = net.encrypted;
        network["saved"] = net.saved;
        network["autoConnect"] = net.autoConnect;
    }
    
    doc["count"] = _scannedNetworks.size();
    
    String response;
    serializeJson(doc, response);
    _webServer.send(200, "application/json", response);
}

void WiFiManager::handleConnectRequest() {
    if (!_webServer.hasArg("plain")) {
        _webServer.send(400, "application/json", "{\"error\":\"No data\"}");
        return;
    }
    
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, _webServer.arg("plain"));
    
    if (error) {
        _webServer.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
    }
    
    String ssid = doc["ssid"] | "";
    String password = doc["password"] | "";
    uint8_t priority = doc["priority"] | 5;
    bool autoConnect = doc["autoConnect"] | true;
    
    if (ssid.isEmpty()) {
        _webServer.send(400, "application/json", "{\"error\":\"SSID required\"}");
        return;
    }
    
    bool success = addNetwork(ssid, password, priority, autoConnect);
    
    if (success && doc["connectNow"] | false) {
        success = connectToNetwork(ssid);
    }
    
    if (success) {
        _webServer.send(200, "application/json", "{\"success\":true}");
    } else {
        _webServer.send(500, "application/json", "{\"error\":\"Failed to add network\"}");
    }
}

void WiFiManager::handleStatusRequest() {
    DynamicJsonDocument doc(512);
    
    doc["state"] = getStateString();
    doc["connected"] = (_state == WIFI_STATE_CONNECTED || _state == WIFI_STATE_AP_STA_MODE);
    doc["ssid"] = _currentSSID;
    doc["rssi"] = _currentRSSI;
    doc["ip"] = WiFi.localIP().toString();
    doc["apEnabled"] = _apEnabled;
    doc["apIP"] = WiFi.softAPIP().toString();
    doc["apSSID"] = _apSSID;
    doc["mac"] = WiFi.macAddress();
    
    String response;
    serializeJson(doc, response);
    _webServer.send(200, "application/json", response);
}

// Additional handlers would follow similar pattern...

// ===== MDNS =====
void WiFiManager::setupMDNS() {
    if (!MDNS.begin("portfoliomonitor")) {
        Serial.println("Error setting up MDNS responder!");
        return;
    }
    
    MDNS.addService("http", "tcp", 80);
    Serial.println("mDNS responder started: portfoliomonitor.local");
}

// ===== SAVE/LOAD =====
void WiFiManager::saveNetworks() {
    DynamicJsonDocument doc(4096);
    JsonArray networks = doc.createNestedArray("networks");
    
    for (const auto& net : _savedNetworks) {
        JsonObject network = networks.createNestedObject();
        network["ssid"] = net.ssid;
        network["password"] = net.password;
        network["priority"] = net.priority;
        network["autoConnect"] = net.autoConnect;
        network["lastConnected"] = net.lastConnected;
        network["connectionAttempts"] = net.connectionAttempts;
        network["rssi"] = net.rssi;
    }
    
    String json;
    serializeJson(doc, json);
    ConfigManager::getInstance().putString("wifi_networks", json.c_str());
    
    Serial.println("WiFi networks saved");
}

void WiFiManager::loadNetworks() {
    String json = ConfigManager::getInstance().getString("wifi_networks", "{}");
    
    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, json);
    
    if (error || !doc.containsKey("networks")) {
        Serial.println("No saved networks found");
        return;
    }
    
    JsonArray networks = doc["networks"];
    _savedNetworks.clear();
    
    for (JsonObject net : networks) {
        WiFiNetwork network;
        network.ssid = net["ssid"].as<String>();
        network.password = net["password"].as<String>();
        network.priority = net["priority"] | 5;
        network.autoConnect = net["autoConnect"] | true;
        network.lastConnected = net["lastConnected"] | 0;
        network.connectionAttempts = net["connectionAttempts"] | 0;
        network.rssi = net["rssi"] | 0;
        
        _savedNetworks.push_back(network);
    }
    
    // Sort by priority
    std::sort(_savedNetworks.begin(), _savedNetworks.end(),
        [](const WiFiNetwork& a, const WiFiNetwork& b) {
            return a.priority > b.priority;
        });
    
    Serial.print("Loaded ");
    Serial.print(_savedNetworks.size());
    Serial.println(" saved networks");
}

// ===== UTILITIES =====
String WiFiManager::getStateString() {
    switch (_state) {
        case WIFI_STATE_DISCONNECTED: return "disconnected";
        case WIFI_STATE_CONNECTING: return "connecting";
        case WIFI_STATE_CONNECTED: return "connected";
        case WIFI_STATE_AP_MODE: return "ap_mode";
        case WIFI_STATE_AP_STA_MODE: return "ap_sta_mode";
        default: return "unknown";
    }
}

String WiFiManager::getEncryptionTypeString(wifi_auth_mode_t type) {
    switch (type) {
        case WIFI_AUTH_OPEN: return "Open";
        case WIFI_AUTH_WEP: return "WEP";
        case WIFI_AUTH_WPA_PSK: return "WPA";
        case WIFI_AUTH_WPA2_PSK: return "WPA2";
        case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/WPA2";
        case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2 Enterprise";
        case WIFI_AUTH_WPA3_PSK: return "WPA3";
        case WIFI_AUTH_WPA2_WPA3_PSK: return "WPA2/WPA3";
        default: return "Unknown";
    }
}

int WiFiManager::getRSSIForNetwork(const String& ssid) {
    for (const auto& net : _scannedNetworks) {
        if (net.ssid == ssid) {
            return net.rssi;
        }
    }
    return -100; // Not found
}

String WiFiManager::getSignalQuality(int rssi) {
    if (rssi >= -50) return "Excellent";
    if (rssi >= -60) return "Good";
    if (rssi >= -70) return "Fair";
    if (rssi >= -80) return "Weak";
    return "Poor";
}

void WiFiManager::printStatus() {
    Serial.println("\n=== WiFi Status ===");
    Serial.print("State: ");
    Serial.println(getStateString());
    
    if (_state == WIFI_STATE_CONNECTED || _state == WIFI_STATE_AP_STA_MODE) {
        Serial.print("Connected to: ");
        Serial.println(_currentSSID);
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP().toString());
        Serial.print("RSSI: ");
        Serial.print(_currentRSSI);
        Serial.print(" dBm (");
        Serial.print(getSignalQuality(_currentRSSI));
        Serial.println(")");
    }
    
    if (_apEnabled) {
        Serial.print("AP Mode: ");
        Serial.println(_state == WIFI_STATE_AP_MODE ? "Active" : "Inactive");
        Serial.print("AP SSID: ");
        Serial.println(_apSSID);
        Serial.print("AP IP: ");
        Serial.println(WiFi.softAPIP().toString());
    }
    
    Serial.print("Saved Networks: ");
    Serial.println(_savedNetworks.size());
    Serial.print("Scanned Networks: ");
    Serial.println(_scannedNetworks.size());
    Serial.println("===================\n");
}

// ===== GETTERS =====
bool WiFiManager::isConnected() { 
    return _state == WIFI_STATE_CONNECTED || _state == WIFI_STATE_AP_STA_MODE; 
}

bool WiFiManager::isAPMode() { 
    return _state == WIFI_STATE_AP_MODE || _state == WIFI_STATE_AP_STA_MODE; 
}

String WiFiManager::getCurrentSSID() { return _currentSSID; }
int WiFiManager::getCurrentRSSI() { return _currentRSSI; }
IPAddress WiFiManager::getLocalIP() { return WiFi.localIP(); }
IPAddress WiFiManager::getAPIP() { return WiFi.softAPIP(); }
String WiFiManager::getAPSSID() { return _apSSID; }
std::vector<WiFiNetwork> WiFiManager::getSavedNetworks() { return _savedNetworks; }
std::vector<WiFiNetwork> WiFiManager::getScannedNetworks() { return _scannedNetworks; }

// ===== STATIC ACCESS =====
WiFiManager& WiFiManager::getInstance() {
    static WiFiManager instance;
    return instance;
}