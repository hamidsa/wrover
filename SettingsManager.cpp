#include "SettingsManager.h"
#include "ConfigManager.h"
#include <Preferences.h>
#include <ArduinoJson.h>

// ===== CONSTRUCTOR/DESTRUCTOR =====
SettingsManager::SettingsManager() 
    : _initialized(false),
      _dirty(false),
      _lastSaveTime(0),
      _autoSaveInterval(60000) { // Auto-save every minute
}

SettingsManager::~SettingsManager() {
    if (_dirty) {
        save();
    }
}

// ===== INITIALIZATION =====
bool SettingsManager::begin() {
    Serial.println("Initializing Settings Manager...");
    
    // Initialize preferences
    if (!ConfigManager::getInstance().begin()) {
        Serial.println("Failed to initialize preferences!");
        return false;
    }
    
    // Load all settings
    if (!load()) {
        Serial.println("Failed to load settings, using defaults");
        setDefaults();
        save();
    }
    
    _initialized = true;
    Serial.println("Settings Manager initialized successfully");
    printCurrentSettings();
    
    return true;
}

void SettingsManager::update() {
    if (!_initialized) return;
    
    // Auto-save if dirty and enough time has passed
    if (_dirty && (millis() - _lastSaveTime >= _autoSaveInterval)) {
        save();
    }
}

// ===== LOAD/SAVE SETTINGS =====
bool SettingsManager::load() {
    Serial.println("Loading settings from storage...");
    
    // Load WiFi settings
    _settings.wifiSettings.ssid = ConfigManager::getInstance().getString("wifi_ssid", "");
    _settings.wifiSettings.password = ConfigManager::getInstance().getString("wifi_pass", "");
    _settings.wifiSettings.autoConnect = ConfigManager::getInstance().getBool("wifi_auto", true);
    _settings.wifiSettings.apEnabled = ConfigManager::getInstance().getBool("ap_enabled", true);
    
    // Load API settings
    _settings.apiSettings.server = ConfigManager::getInstance().getString("api_server", "");
    _settings.apiSettings.username = ConfigManager::getInstance().getString("api_user", "");
    _settings.apiSettings.password = ConfigManager::getInstance().getString("api_pass", "");
    _settings.apiSettings.entryPortfolio = ConfigManager::getInstance().getString("port_entry", "Arduino");
    _settings.apiSettings.exitPortfolio = ConfigManager::getInstance().getString("port_exit", "MyExit");
    
    // Load Alert settings
    _settings.alertSettings.alertThreshold = ConfigManager::getInstance().getFloat("alert_thresh", -5.0);
    _settings.alertSettings.severeThreshold = ConfigManager::getInstance().getFloat("sev_thresh", -10.0);
    _settings.alertSettings.portfolioThreshold = ConfigManager::getInstance().getFloat("port_thresh", -7.0);
    _settings.alertSettings.buzzerVolume = ConfigManager::getInstance().getUChar("buzzer_vol", 50);
    _settings.alertSettings.buzzerEnabled = ConfigManager::getInstance().getBool("buzzer_en", true);
    
    // Load Display settings
    _settings.displaySettings.brightness = ConfigManager::getInstance().getUChar("disp_bright", 100);
    _settings.displaySettings.timeout = ConfigManager::getInstance().getUInt("disp_timeout", 30000);
    _settings.displaySettings.rotation = ConfigManager::getInstance().getUChar("disp_rot", 0);
    _settings.displaySettings.showDetails = ConfigManager::getInstance().getBool("disp_details", true);
    
    // Load LED/RGB settings
    _settings.ledSettings.ledEnabled = ConfigManager::getInstance().getBool("led_en", true);
    _settings.ledSettings.ledBrightness = ConfigManager::getInstance().getUChar("led_bright", 100);
    _settings.ledSettings.rgb1Enabled = ConfigManager::getInstance().getBool("rgb1_en", true);
    _settings.ledSettings.rgb2Enabled = ConfigManager::getInstance().getBool("rgb2_en", true);
    _settings.ledSettings.rgb1Brightness = ConfigManager::getInstance().getUChar("rgb1_bright", 80);
    _settings.ledSettings.rgb2Brightness = ConfigManager::getInstance().getUChar("rgb2_bright", 80);
    
    // Load System settings
    _settings.systemSettings.autoReconnect = ConfigManager::getInstance().getBool("auto_recon", true);
    _settings.systemSettings.reconnectAttempts = ConfigManager::getInstance().getUChar("recon_att", 5);
    _settings.systemSettings.showBattery = ConfigManager::getInstance().getBool("show_batt", true);
    _settings.systemSettings.batteryWarning = ConfigManager::getInstance().getUChar("batt_warn", 20);
    
    // Load Exit Alert settings
    _settings.exitAlertSettings.enabled = ConfigManager::getInstance().getBool("exit_en", true);
    _settings.exitAlertSettings.percent = ConfigManager::getInstance().getFloat("exit_percent", 3.0);
    _settings.exitAlertSettings.blinkEnabled = ConfigManager::getInstance().getBool("exit_blink", true);
    
    Serial.println("Settings loaded successfully");
    return true;
}

bool SettingsManager::save() {
    if (!_initialized) return false;
    
    Serial.println("Saving settings to storage...");
    
    // Save WiFi settings
    ConfigManager::getInstance().putString("wifi_ssid", _settings.wifiSettings.ssid.c_str());
    ConfigManager::getInstance().putString("wifi_pass", _settings.wifiSettings.password.c_str());
    ConfigManager::getInstance().putBool("wifi_auto", _settings.wifiSettings.autoConnect);
    ConfigManager::getInstance().putBool("ap_enabled", _settings.wifiSettings.apEnabled);
    
    // Save API settings
    ConfigManager::getInstance().putString("api_server", _settings.apiSettings.server.c_str());
    ConfigManager::getInstance().putString("api_user", _settings.apiSettings.username.c_str());
    ConfigManager::getInstance().putString("api_pass", _settings.apiSettings.password.c_str());
    ConfigManager::getInstance().putString("port_entry", _settings.apiSettings.entryPortfolio.c_str());
    ConfigManager::getInstance().putString("port_exit", _settings.apiSettings.exitPortfolio.c_str());
    
    // Save Alert settings
    ConfigManager::getInstance().putFloat("alert_thresh", _settings.alertSettings.alertThreshold);
    ConfigManager::getInstance().putFloat("sev_thresh", _settings.alertSettings.severeThreshold);
    ConfigManager::getInstance().putFloat("port_thresh", _settings.alertSettings.portfolioThreshold);
    ConfigManager::getInstance().putUChar("buzzer_vol", _settings.alertSettings.buzzerVolume);
    ConfigManager::getInstance().putBool("buzzer_en", _settings.alertSettings.buzzerEnabled);
    
    // Save Display settings
    ConfigManager::getInstance().putUChar("disp_bright", _settings.displaySettings.brightness);
    ConfigManager::getInstance().putUInt("disp_timeout", _settings.displaySettings.timeout);
    ConfigManager::getInstance().putUChar("disp_rot", _settings.displaySettings.rotation);
    ConfigManager::getInstance().putBool("disp_details", _settings.displaySettings.showDetails);
    
    // Save LED/RGB settings
    ConfigManager::getInstance().putBool("led_en", _settings.ledSettings.ledEnabled);
    ConfigManager::getInstance().putUChar("led_bright", _settings.ledSettings.ledBrightness);
    ConfigManager::getInstance().putBool("rgb1_en", _settings.ledSettings.rgb1Enabled);
    ConfigManager::getInstance().putBool("rgb2_en", _settings.ledSettings.rgb2Enabled);
    ConfigManager::getInstance().putUChar("rgb1_bright", _settings.ledSettings.rgb1Brightness);
    ConfigManager::getInstance().putUChar("rgb2_bright", _settings.ledSettings.rgb2Brightness);
    
    // Save System settings
    ConfigManager::getInstance().putBool("auto_recon", _settings.systemSettings.autoReconnect);
    ConfigManager::getInstance().putUChar("recon_att", _settings.systemSettings.reconnectAttempts);
    ConfigManager::getInstance().putBool("show_batt", _settings.systemSettings.showBattery);
    ConfigManager::getInstance().putUChar("batt_warn", _settings.systemSettings.batteryWarning);
    
    // Save Exit Alert settings
    ConfigManager::getInstance().putBool("exit_en", _settings.exitAlertSettings.enabled);
    ConfigManager::getInstance().putFloat("exit_percent", _settings.exitAlertSettings.percent);
    ConfigManager::getInstance().putBool("exit_blink", _settings.exitAlertSettings.blinkEnabled);
    
    _dirty = false;
    _lastSaveTime = millis();
    
    Serial.println("Settings saved successfully");
    return true;
}

// ===== DEFAULT SETTINGS =====
void SettingsManager::setDefaults() {
    Serial.println("Setting default configuration...");
    
    // WiFi defaults
    _settings.wifiSettings.ssid = "";
    _settings.wifiSettings.password = "";
    _settings.wifiSettings.autoConnect = true;
    _settings.wifiSettings.apEnabled = true;
    
    // API defaults
    _settings.apiSettings.server = "";
    _settings.apiSettings.username = "";
    _settings.apiSettings.password = "";
    _settings.apiSettings.entryPortfolio = "Arduino";
    _settings.apiSettings.exitPortfolio = "MyExit";
    
    // Alert defaults
    _settings.alertSettings.alertThreshold = -5.0;
    _settings.alertSettings.severeThreshold = -10.0;
    _settings.alertSettings.portfolioThreshold = -7.0;
    _settings.alertSettings.buzzerVolume = 50;
    _settings.alertSettings.buzzerEnabled = true;
    
    // Display defaults
    _settings.displaySettings.brightness = 100;
    _settings.displaySettings.timeout = 30000;
    _settings.displaySettings.rotation = 0;
    _settings.displaySettings.showDetails = true;
    
    // LED/RGB defaults
    _settings.ledSettings.ledEnabled = true;
    _settings.ledSettings.ledBrightness = 100;
    _settings.ledSettings.rgb1Enabled = true;
    _settings.ledSettings.rgb2Enabled = true;
    _settings.ledSettings.rgb1Brightness = 80;
    _settings.ledSettings.rgb2Brightness = 80;
    
    // System defaults
    _settings.systemSettings.autoReconnect = true;
    _settings.systemSettings.reconnectAttempts = 5;
    _settings.systemSettings.showBattery = true;
    _settings.systemSettings.batteryWarning = 20;
    
    // Exit Alert defaults
    _settings.exitAlertSettings.enabled = true;
    _settings.exitAlertSettings.percent = 3.0;
    _settings.exitAlertSettings.blinkEnabled = true;
    
    _dirty = true;
    Serial.println("Default settings applied");
}

// ===== SETTER METHODS =====
// WiFi Settings
void SettingsManager::setWiFiSSID(const String& ssid) {
    _settings.wifiSettings.ssid = ssid.c_str();
    _dirty = true;
}

void SettingsManager::setWiFiPassword(const String& password) {
    _settings.wifiSettings.password = password.c_str();
    _dirty = true;
}

void SettingsManager::setWiFiAutoConnect(bool autoConnect) {
    _settings.wifiSettings.autoConnect = autoConnect;
    _dirty = true;
}

void SettingsManager::setAPEnabled(bool enabled) {
    _settings.wifiSettings.apEnabled = enabled;
    _dirty = true;
}

// API Settings
void SettingsManager::setAPIServer(const String& server) {
    _settings.apiSettings.server = server.c_str();
    _dirty = true;
}

void SettingsManager::setAPIUsername(const String& username) {
    _settings.apiSettings.username = username.c_str();
    _dirty = true;
}

void SettingsManager::setAPIPassword(const String& password) {
    _settings.apiSettings.password = password.c_str();
    _dirty = true;
}

void SettingsManager::setEntryPortfolio(const String& portfolio) {
    _settings.apiSettings.entryPortfolio = portfolio.c_str();
    _dirty = true;
}

void SettingsManager::setExitPortfolio(const String& portfolio) {
    _settings.apiSettings.exitPortfolio = portfolio.c_str();
    _dirty = true;
}

// Alert Settings
void SettingsManager::setAlertThreshold(float threshold) {
    _settings.alertSettings.alertThreshold = threshold;
    _dirty = true;
}

void SettingsManager::setSevereThreshold(float threshold) {
    _settings.alertSettings.severeThreshold = threshold;
    _dirty = true;
}

void SettingsManager::setPortfolioThreshold(float threshold) {
    _settings.alertSettings.portfolioThreshold = threshold;
    _dirty = true;
}

void SettingsManager::setBuzzerVolume(uint8_t volume) {
    _settings.alertSettings.buzzerVolume = constrain(volume, 0, 100);
    _dirty = true;
}

void SettingsManager::setBuzzerEnabled(bool enabled) {
    _settings.alertSettings.buzzerEnabled = enabled;
    _dirty = true;
}

// Display Settings
void SettingsManager::setDisplayBrightness(uint8_t brightness) {
    _settings.displaySettings.brightness = constrain(brightness, 0, 100);
    _dirty = true;
}

void SettingsManager::setDisplayTimeout(uint32_t timeout) {
    _settings.displaySettings.timeout = timeout;
    _dirty = true;
}

void SettingsManager::setDisplayRotation(uint8_t rotation) {
    _settings.displaySettings.rotation = rotation % 4;
    _dirty = true;
}

void SettingsManager::setShowDetails(bool show) {
    _settings.displaySettings.showDetails = show;
    _dirty = true;
}

// LED Settings
void SettingsManager::setLEDEnabled(bool enabled) {
    _settings.ledSettings.ledEnabled = enabled;
    _dirty = true;
}

void SettingsManager::setLEDBrightness(uint8_t brightness) {
    _settings.ledSettings.ledBrightness = constrain(brightness, 0, 100);
    _dirty = true;
}

void SettingsManager::setRGB1Enabled(bool enabled) {
    _settings.ledSettings.rgb1Enabled = enabled;
    _dirty = true;
}

void SettingsManager::setRGB2Enabled(bool enabled) {
    _settings.ledSettings.rgb2Enabled = enabled;
    _dirty = true;
}

void SettingsManager::setRGB1Brightness(uint8_t brightness) {
    _settings.ledSettings.rgb1Brightness = constrain(brightness, 0, 100);
    _dirty = true;
}

void SettingsManager::setRGB2Brightness(uint8_t brightness) {
    _settings.ledSettings.rgb2Brightness = constrain(brightness, 0, 100);
    _dirty = true;
}

// System Settings
void SettingsManager::setAutoReconnect(bool autoReconnect) {
    _settings.systemSettings.autoReconnect = autoReconnect;
    _dirty = true;
}

void SettingsManager::setReconnectAttempts(uint8_t attempts) {
    _settings.systemSettings.reconnectAttempts = attempts;
    _dirty = true;
}

void SettingsManager::setShowBattery(bool show) {
    _settings.systemSettings.showBattery = show;
    _dirty = true;
}

void SettingsManager::setBatteryWarning(uint8_t percent) {
    _settings.systemSettings.batteryWarning = constrain(percent, 5, 50);
    _dirty = true;
}

// Exit Alert Settings
void SettingsManager::setExitAlertEnabled(bool enabled) {
    _settings.exitAlertSettings.enabled = enabled;
    _dirty = true;
}

void SettingsManager::setExitAlertPercent(float percent) {
    _settings.exitAlertSettings.percent = percent;
    _dirty = true;
}

void SettingsManager::setExitAlertBlink(bool blink) {
    _settings.exitAlertSettings.blinkEnabled = blink;
    _dirty = true;
}

// ===== GETTER METHODS =====
// WiFi Settings
String SettingsManager::getWiFiSSID() const { return _settings.wifiSettings.ssid.c_str(); }
String SettingsManager::getWiFiPassword() const { return _settings.wifiSettings.password.c_str(); }
bool SettingsManager::getWiFiAutoConnect() const { return _settings.wifiSettings.autoConnect; }
bool SettingsManager::getAPEnabled() const { return _settings.wifiSettings.apEnabled; }

// API Settings
String SettingsManager::getAPIServer() const { return _settings.apiSettings.server.c_str(); }
String SettingsManager::getAPIUsername() const { return _settings.apiSettings.username.c_str(); }
String SettingsManager::getAPIPassword() const { return _settings.apiSettings.password.c_str(); }
String SettingsManager::getEntryPortfolio() const { return _settings.apiSettings.entryPortfolio.c_str(); }
String SettingsManager::getExitPortfolio() const { return _settings.apiSettings.exitPortfolio.c_str(); }

// Alert Settings
float SettingsManager::getAlertThreshold() const { return _settings.alertSettings.alertThreshold; }
float SettingsManager::getSevereThreshold() const { return _settings.alertSettings.severeThreshold; }
float SettingsManager::getPortfolioThreshold() const { return _settings.alertSettings.portfolioThreshold; }
uint8_t SettingsManager::getBuzzerVolume() const { return _settings.alertSettings.buzzerVolume; }
bool SettingsManager::getBuzzerEnabled() const { return _settings.alertSettings.buzzerEnabled; }

// Display Settings
uint8_t SettingsManager::getDisplayBrightness() const { return _settings.displaySettings.brightness; }
uint32_t SettingsManager::getDisplayTimeout() const { return _settings.displaySettings.timeout; }
uint8_t SettingsManager::getDisplayRotation() const { return _settings.displaySettings.rotation; }
bool SettingsManager::getShowDetails() const { return _settings.displaySettings.showDetails; }

// LED Settings
bool SettingsManager::getLEDEnabled() const { return _settings.ledSettings.ledEnabled; }
uint8_t SettingsManager::getLEDBrightness() const { return _settings.ledSettings.ledBrightness; }
bool SettingsManager::getRGB1Enabled() const { return _settings.ledSettings.rgb1Enabled; }
bool SettingsManager::getRGB2Enabled() const { return _settings.ledSettings.rgb2Enabled; }
uint8_t SettingsManager::getRGB1Brightness() const { return _settings.ledSettings.rgb1Brightness; }
uint8_t SettingsManager::getRGB2Brightness() const { return _settings.ledSettings.rgb2Brightness; }

// System Settings
bool SettingsManager::getAutoReconnect() const { return _settings.systemSettings.autoReconnect; }
uint8_t SettingsManager::getReconnectAttempts() const { return _settings.systemSettings.reconnectAttempts; }
bool SettingsManager::getShowBattery() const { return _settings.systemSettings.showBattery; }
uint8_t SettingsManager::getBatteryWarning() const { return _settings.systemSettings.batteryWarning; }

// Exit Alert Settings
bool SettingsManager::getExitAlertEnabled() const { return _settings.exitAlertSettings.enabled; }
float SettingsManager::getExitAlertPercent() const { return _settings.exitAlertSettings.percent; }
bool SettingsManager::getExitAlertBlink() const { return _settings.exitAlertSettings.blinkEnabled; }

// ===== JSON HANDLING =====
String SettingsManager::toJSON() const {
    DynamicJsonDocument doc(2048);
    
    // WiFi settings
    doc["wifi"]["ssid"] = _settings.wifiSettings.ssid;
    doc["wifi"]["autoConnect"] = _settings.wifiSettings.autoConnect;
    doc["wifi"]["apEnabled"] = _settings.wifiSettings.apEnabled;
    
    // API settings
    doc["api"]["server"] = _settings.apiSettings.server;
    doc["api"]["username"] = _settings.apiSettings.username;
    doc["api"]["entryPortfolio"] = _settings.apiSettings.entryPortfolio;
    doc["api"]["exitPortfolio"] = _settings.apiSettings.exitPortfolio;
    
    // Alert settings
    doc["alert"]["threshold"] = _settings.alertSettings.alertThreshold;
    doc["alert"]["severeThreshold"] = _settings.alertSettings.severeThreshold;
    doc["alert"]["portfolioThreshold"] = _settings.alertSettings.portfolioThreshold;
    doc["alert"]["buzzerVolume"] = _settings.alertSettings.buzzerVolume;
    doc["alert"]["buzzerEnabled"] = _settings.alertSettings.buzzerEnabled;
    
    // Display settings
    doc["display"]["brightness"] = _settings.displaySettings.brightness;
    doc["display"]["timeout"] = _settings.displaySettings.timeout;
    doc["display"]["rotation"] = _settings.displaySettings.rotation;
    doc["display"]["showDetails"] = _settings.displaySettings.showDetails;
    
    // LED settings
    doc["led"]["enabled"] = _settings.ledSettings.ledEnabled;
    doc["led"]["brightness"] = _settings.ledSettings.ledBrightness;
    doc["led"]["rgb1Enabled"] = _settings.ledSettings.rgb1Enabled;
    doc["led"]["rgb2Enabled"] = _settings.ledSettings.rgb2Enabled;
    doc["led"]["rgb1Brightness"] = _settings.ledSettings.rgb1Brightness;
    doc["led"]["rgb2Brightness"] = _settings.ledSettings.rgb2Brightness;
    
    // System settings
    doc["system"]["autoReconnect"] = _settings.systemSettings.autoReconnect;
    doc["system"]["reconnectAttempts"] = _settings.systemSettings.reconnectAttempts;
    doc["system"]["showBattery"] = _settings.systemSettings.showBattery;
    doc["system"]["batteryWarning"] = _settings.systemSettings.batteryWarning;
    
    // Exit alert settings
    doc["exitAlert"]["enabled"] = _settings.exitAlertSettings.enabled;
    doc["exitAlert"]["percent"] = _settings.exitAlertSettings.percent;
    doc["exitAlert"]["blinkEnabled"] = _settings.exitAlertSettings.blinkEnabled;
    
    String json;
    serializeJson(doc, json);
    return json;
}

bool SettingsManager::fromJSON(const String& json) {
    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, json);
    
    if (error) {
        Serial.print("JSON parsing error: ");
        Serial.println(error.c_str());
        return false;
    }
    
    // WiFi settings
    if (doc.containsKey("wifi")) {
        JsonObject wifi = doc["wifi"];
        if (wifi.containsKey("ssid")) _settings.wifiSettings.ssid = wifi["ssid"].as<String>().c_str();
        if (wifi.containsKey("password")) _settings.wifiSettings.password = wifi["password"].as<String>().c_str();
        if (wifi.containsKey("autoConnect")) _settings.wifiSettings.autoConnect = wifi["autoConnect"];
        if (wifi.containsKey("apEnabled")) _settings.wifiSettings.apEnabled = wifi["apEnabled"];
    }
    
    // API settings
    if (doc.containsKey("api")) {
        JsonObject api = doc["api"];
        if (api.containsKey("server")) _settings.apiSettings.server = api["server"].as<String>().c_str();
        if (api.containsKey("username")) _settings.apiSettings.username = api["username"].as<String>().c_str();
        if (api.containsKey("password")) _settings.apiSettings.password = api["password"].as<String>().c_str();
        if (api.containsKey("entryPortfolio")) _settings.apiSettings.entryPortfolio = api["entryPortfolio"].as<String>().c_str();
        if (api.containsKey("exitPortfolio")) _settings.apiSettings.exitPortfolio = api["exitPortfolio"].as<String>().c_str();
    }
    
    // Update other settings similarly...
    
    _dirty = true;
    return true;
}

// ===== UTILITY METHODS =====
void SettingsManager::printCurrentSettings() {
    Serial.println("\n=== Current Settings ===");
    Serial.print("WiFi SSID: "); Serial.println(_settings.wifiSettings.ssid.c_str());
    Serial.print("AP Enabled: "); Serial.println(_settings.wifiSettings.apEnabled ? "Yes" : "No");
    Serial.print("API Server: "); Serial.println(_settings.apiSettings.server.c_str());
    Serial.print("Entry Portfolio: "); Serial.println(_settings.apiSettings.entryPortfolio.c_str());
    Serial.print("Exit Portfolio: "); Serial.println(_settings.apiSettings.exitPortfolio.c_str());
    Serial.print("Buzzer Volume: "); Serial.print(_settings.alertSettings.buzzerVolume); Serial.println("%");
    Serial.print("Display Brightness: "); Serial.print(_settings.displaySettings.brightness); Serial.println("%");
    Serial.println("======================\n");
}

void SettingsManager::factoryReset() {
    Serial.println("Performing factory reset...");
    ConfigManager::getInstance().clear();
    setDefaults();
    save();
    Serial.println("Factory reset complete");
}

bool SettingsManager::validateSettings() {
    // Check for critical settings
    if (_settings.apiSettings.server.empty()) {
        Serial.println("Warning: API server not configured");
        return false;
    }
    
    if (_settings.apiSettings.username.empty()) {
        Serial.println("Warning: API username not configured");
        return false;
    }
    
    // Validate ranges
    if (_settings.alertSettings.buzzerVolume > 100) {
        _settings.alertSettings.buzzerVolume = 100;
        _dirty = true;
    }
    
    if (_settings.displaySettings.brightness > 100) {
        _settings.displaySettings.brightness = 100;
        _dirty = true;
    }
    
    return true;
}

// ===== WEB INTERFACE HANDLERS =====
void SettingsManager::handleWebRequest(const String& section, const String& action, const String& params) {
    if (section == "save") {
        // Parse and save settings from params (JSON format)
        fromJSON(params);
        save();
    } else if (section == "load") {
        // Return current settings as JSON
        // (This would be handled by the web server)
    } else if (section == "reset") {
        if (action == "factory") {
            factoryReset();
        } else if (action == "section") {
            // Reset specific section
            resetSection(params);
        }
    }
}

void SettingsManager::resetSection(const String& section) {
    if (section == "wifi") {
        _settings.wifiSettings.ssid = "";
        _settings.wifiSettings.password = "";
    } else if (section == "api") {
        _settings.apiSettings.server = "";
        _settings.apiSettings.username = "";
        _settings.apiSettings.password = "";
    } else if (section == "alert") {
        _settings.alertSettings.alertThreshold = -5.0;
        _settings.alertSettings.severeThreshold = -10.0;
        _settings.alertSettings.portfolioThreshold = -7.0;
        _settings.alertSettings.buzzerVolume = 50;
    } else if (section == "display") {
        _settings.displaySettings.brightness = 100;
        _settings.displaySettings.timeout = 30000;
        _settings.displaySettings.rotation = 0;
    }
    
    _dirty = true;
    Serial.print("Reset section: ");
    Serial.println(section);
}