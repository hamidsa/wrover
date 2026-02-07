#include "BatteryManager.h"
#include "ConfigManager.h"
#include <Preferences.h>

// ===== CONSTANTS =====
#define BATTERY_PIN 34
#define VOLTAGE_DIVIDER_RATIO 2.0
#define REFERENCE_VOLTAGE 3.3
#define ADC_MAX 4095
#define SAMPLE_COUNT 10
#define LOW_BATTERY_THRESHOLD 20
#define CRITICAL_BATTERY_THRESHOLD 10
#define FULL_CHARGE_VOLTAGE 4.2
#define EMPTY_VOLTAGE 3.0
#define CHARGE_DETECTION_THRESHOLD 4.5

// ===== STATIC VARIABLES =====
BatteryManager* BatteryManager::_instance = nullptr;
Preferences BatteryManager::_prefs;

// ===== CONSTRUCTOR/DESTRUCTOR =====
BatteryManager::BatteryManager()
    : _initialized(false),
      _voltage(0.0),
      _percentage(100),
      _charging(false),
      _health(100),
      _lastUpdateTime(0),
      _updateInterval(10000), // 10 seconds
      _lowBatteryAlertSent(false),
      _criticalBatteryAlertSent(false),
      _calibrationOffset(0.0) {
}

BatteryManager::~BatteryManager() {
    if (_instance == this) {
        _instance = nullptr;
    }
}

// ===== INITIALIZATION =====
bool BatteryManager::begin() {
    Serial.println("Initializing Battery Manager...");
    
    // Configure battery pin
    pinMode(BATTERY_PIN, INPUT);
    
    // Load calibration data
    loadCalibration();
    
    // Initial reading
    update();
    
    _initialized = true;
    Serial.println("Battery Manager initialized");
    printStatus();
    
    return true;
}

void BatteryManager::update() {
    if (!_initialized) return;
    
    unsigned long currentTime = millis();
    if (currentTime - _lastUpdateTime < _updateInterval) return;
    
    _lastUpdateTime = currentTime;
    
    // Read voltage
    float newVoltage = readVoltage();
    
    // Apply low-pass filter
    _voltage = (_voltage * 0.7) + (newVoltage * 0.3);
    
    // Calculate percentage
    _percentage = calculatePercentage(_voltage);
    
    // Detect charging status
    _charging = detectCharging();
    
    // Calculate battery health
    _health = calculateHealth();
    
    // Check for alerts
    checkBatteryAlerts();
    
    // Save statistics periodically
    static unsigned long lastSaveTime = 0;
    if (currentTime - lastSaveTime > 60000) { // Every minute
        saveStatistics();
        lastSaveTime = currentTime;
    }
}

// ===== VOLTAGE READING =====
float BatteryManager::readVoltage() {
    // Take multiple samples for accuracy
    uint32_t sum = 0;
    for (int i = 0; i < SAMPLE_COUNT; i++) {
        sum += analogRead(BATTERY_PIN);
        delay(2);
    }
    
    float average = sum / (float)SAMPLE_COUNT;
    
    // Convert to voltage
    float voltage = (average / ADC_MAX) * REFERENCE_VOLTAGE * VOLTAGE_DIVIDER_RATIO;
    
    // Apply calibration offset
    voltage += _calibrationOffset;
    
    return voltage;
}

float BatteryManager::readRawVoltage() {
    return readVoltage() - _calibrationOffset;
}

uint8_t BatteryManager::calculatePercentage(float voltage) {
    // Handle charging case (voltage might be higher)
    if (_charging && voltage > FULL_CHARGE_VOLTAGE) {
        return 100;
    }
    
    // Constrain voltage to valid range
    voltage = constrain(voltage, EMPTY_VOLTAGE, FULL_CHARGE_VOLTAGE);
    
    // Calculate percentage (non-linear approximation for Li-ion)
    if (voltage >= 4.15) return 100;
    if (voltage >= 4.10) return 95;
    if (voltage >= 4.05) return 90;
    if (voltage >= 4.00) return 85;
    if (voltage >= 3.95) return 75;
    if (voltage >= 3.90) return 65;
    if (voltage >= 3.85) return 55;
    if (voltage >= 3.80) return 45;
    if (voltage >= 3.75) return 35;
    if (voltage >= 3.70) return 25;
    if (voltage >= 3.65) return 15;
    if (voltage >= 3.60) return 10;
    if (voltage >= 3.55) return 5;
    return 0;
}

// ===== CHARGING DETECTION =====
bool BatteryManager::detectCharging() {
    // Simple detection based on voltage
    // When charging, voltage is typically higher
    float currentVoltage = readRawVoltage();
    
    // If voltage is above typical full charge, likely charging
    if (currentVoltage > FULL_CHARGE_VOLTAGE + 0.1) {
        return true;
    }
    
    // Check for USB connection
    // Note: ESP32 doesn't have built-in USB detection
    // This is a simplified implementation
    
    return false;
}

// ===== BATTERY HEALTH =====
uint8_t BatteryManager::calculateHealth() {
    // Simple health calculation based on maximum observed voltage
    static float maxObservedVoltage = 0;
    
    float currentVoltage = readRawVoltage();
    if (currentVoltage > maxObservedVoltage) {
        maxObservedVoltage = currentVoltage;
    }
    
    // Health as percentage of original capacity (assuming 4.2V is 100%)
    if (maxObservedVoltage <= EMPTY_VOLTAGE) return 0;
    
    float health = ((maxObservedVoltage - EMPTY_VOLTAGE) / 
                   (FULL_CHARGE_VOLTAGE - EMPTY_VOLTAGE)) * 100;
    
    return constrain((uint8_t)health, 0, 100);
}

// ===== ALERTS =====
void BatteryManager::checkBatteryAlerts() {
    if (_percentage <= CRITICAL_BATTERY_THRESHOLD && !_criticalBatteryAlertSent) {
        triggerCriticalBatteryAlert();
        _criticalBatteryAlertSent = true;
        _lowBatteryAlertSent = true; // Also counts as low battery
    }
    else if (_percentage <= LOW_BATTERY_THRESHOLD && !_lowBatteryAlertSent) {
        triggerLowBatteryAlert();
        _lowBatteryAlertSent = true;
    }
    else if (_percentage > LOW_BATTERY_THRESHOLD) {
        // Reset alerts if battery recovered
        _lowBatteryAlertSent = false;
        if (_percentage > CRITICAL_BATTERY_THRESHOLD) {
            _criticalBatteryAlertSent = false;
        }
    }
}

void BatteryManager::triggerLowBatteryAlert() {
    Serial.println("⚠️ LOW BATTERY ALERT!");
    Serial.print("Battery: ");
    Serial.print(_percentage);
    Serial.println("%");
    
    // Notify other managers
    // DisplayManager::getInstance().showBatteryAlert(false);
    // BuzzerManager::getInstance().playBatteryAlert(false);
    // LEDManager::getInstance().setBatteryAlert(false);
}

void BatteryManager::triggerCriticalBatteryAlert() {
    Serial.println("🚨 CRITICAL BATTERY ALERT!");
    Serial.print("Battery: ");
    Serial.print(_percentage);
    Serial.println("%");
    
    // Notify other managers
    // DisplayManager::getInstance().showBatteryAlert(true);
    // BuzzerManager::getInstance().playBatteryAlert(true);
    // LEDManager::getInstance().setBatteryAlert(true);
    
    // Save unsaved data
    saveCriticalData();
}

void BatteryManager::saveCriticalData() {
    Serial.println("Saving critical data due to low battery...");
    
    // Save current state
    _prefs.begin("battery", false);
    _prefs.putFloat("last_voltage", _voltage);
    _prefs.putUChar("last_percentage", _percentage);
    _prefs.putULong("last_update", millis());
    _prefs.end();
    
    // Save other critical data
    ConfigManager::getInstance().save();
    
    Serial.println("Critical data saved");
}

// ===== CALIBRATION =====
void BatteryManager::calibrate(float knownVoltage) {
    Serial.println("Starting battery calibration...");
    
    float measuredVoltage = readRawVoltage();
    _calibrationOffset = knownVoltage - measuredVoltage;
    
    // Save calibration
    _prefs.begin("battery_cal", false);
    _prefs.putFloat("offset", _calibrationOffset);
    _prefs.putFloat("known_voltage", knownVoltage);
    _prefs.putFloat("measured_voltage", measuredVoltage);
    _prefs.putULong("calibration_time", millis());
    _prefs.end();
    
    Serial.println("Battery calibration complete:");
    Serial.print("  Known Voltage: ");
    Serial.println(knownVoltage);
    Serial.print("  Measured Voltage: ");
    Serial.println(measuredVoltage);
    Serial.print("  Calibration Offset: ");
    Serial.println(_calibrationOffset);
}

void BatteryManager::resetCalibration() {
    _calibrationOffset = 0.0;
    
    _prefs.begin("battery_cal", false);
    _prefs.clear();
    _prefs.end();
    
    Serial.println("Battery calibration reset");
}

void BatteryManager::loadCalibration() {
    _prefs.begin("battery_cal", true);
    _calibrationOffset = _prefs.getFloat("offset", 0.0);
    _prefs.end();
    
    if (_calibrationOffset != 0.0) {
        Serial.print("Loaded calibration offset: ");
        Serial.println(_calibrationOffset);
    }
}

// ===== STATISTICS =====
void BatteryManager::saveStatistics() {
    _prefs.begin("battery_stats", false);
    
    // Increment cycle count
    uint32_t cycles = _prefs.getULong("cycles", 0);
    if (_charging) {
        cycles++;
        _prefs.putULong("cycles", cycles);
    }
    
    // Save voltage history (rolling window)
    String voltageHistory = _prefs.getString("voltage_history", "");
    voltageHistory += String(_voltage, 2) + ",";
    
    // Keep only last 100 readings
    int commaCount = 0;
    for (int i = 0; i < voltageHistory.length(); i++) {
        if (voltageHistory[i] == ',') commaCount++;
        if (commaCount > 100) {
            voltageHistory = voltageHistory.substring(i + 1);
            break;
        }
    }
    
    _prefs.putString("voltage_history", voltageHistory);
    
    // Save min/max voltages
    float minVoltage = _prefs.getFloat("min_voltage", 100.0);
    float maxVoltage = _prefs.getFloat("max_voltage", 0.0);
    
    if (_voltage < minVoltage) _prefs.putFloat("min_voltage", _voltage);
    if (_voltage > maxVoltage) _prefs.putFloat("max_voltage", _voltage);
    
    _prefs.end();
}

BatteryStats BatteryManager::getStatistics() {
    BatteryStats stats;
    
    _prefs.begin("battery_stats", true);
    
    stats.cycles = _prefs.getULong("cycles", 0);
    stats.minVoltage = _prefs.getFloat("min_voltage", 0.0);
    stats.maxVoltage = _prefs.getFloat("max_voltage", 0.0);
    stats.firstUse = _prefs.getULong("first_use", 0);
    
    // Calculate average voltage from history
    String voltageHistory = _prefs.getString("voltage_history", "");
    float sum = 0;
    int count = 0;
    
    int startPos = 0;
    int commaPos = voltageHistory.indexOf(',');
    while (commaPos != -1) {
        String voltageStr = voltageHistory.substring(startPos, commaPos);
        sum += voltageStr.toFloat();
        count++;
        startPos = commaPos + 1;
        commaPos = voltageHistory.indexOf(',', startPos);
    }
    
    stats.averageVoltage = (count > 0) ? (sum / count) : 0.0;
    
    _prefs.end();
    
    return stats;
}

// ===== WEB INTERFACE HANDLERS =====
String BatteryManager::getStatusJSON() {
    DynamicJsonDocument doc(512);
    
    doc["voltage"] = _voltage;
    doc["percentage"] = _percentage;
    doc["charging"] = _charging;
    doc["health"] = _health;
    doc["status"] = getStatusString();
    doc["raw_voltage"] = readRawVoltage();
    doc["calibration_offset"] = _calibrationOffset;
    doc["last_update"] = _lastUpdateTime;
    
    BatteryStats stats = getStatistics();
    JsonObject statsObj = doc.createNestedObject("statistics");
    statsObj["cycles"] = stats.cycles;
    statsObj["min_voltage"] = stats.minVoltage;
    statsObj["max_voltage"] = stats.maxVoltage;
    statsObj["average_voltage"] = stats.averageVoltage;
    
    String json;
    serializeJson(doc, json);
    return json;
}

void BatteryManager::handleWebRequest(const String& action, const String& params) {
    if (action == "status") {
        // Status is returned via getStatusJSON()
    }
    else if (action == "calibrate") {
        float knownVoltage = params.toFloat();
        if (knownVoltage > 0) {
            calibrate(knownVoltage);
        }
    }
    else if (action == "reset_calibration") {
        resetCalibration();
    }
    else if (action == "stats") {
        // Statistics are included in getStatusJSON()
    }
}

// ===== UTILITY FUNCTIONS =====
String BatteryManager::getStatusString() {
    if (_charging) {
        return "charging";
    } else if (_percentage <= CRITICAL_BATTERY_THRESHOLD) {
        return "critical";
    } else if (_percentage <= LOW_BATTERY_THRESHOLD) {
        return "low";
    } else if (_percentage >= 90) {
        return "full";
    } else {
        return "normal";
    }
}

void BatteryManager::printStatus() {
    Serial.println("\n=== Battery Status ===");
    Serial.print("Voltage: ");
    Serial.print(_voltage, 2);
    Serial.println("V");
    
    Serial.print("Percentage: ");
    Serial.print(_percentage);
    Serial.println("%");
    
    Serial.print("Status: ");
    Serial.println(getStatusString());
    
    Serial.print("Charging: ");
    Serial.println(_charging ? "Yes" : "No");
    
    Serial.print("Health: ");
    Serial.print(_health);
    Serial.println("%");
    
    Serial.print("Calibration Offset: ");
    Serial.print(_calibrationOffset, 3);
    Serial.println("V");
    
    BatteryStats stats = getStatistics();
    Serial.print("Charge Cycles: ");
    Serial.println(stats.cycles);
    
    Serial.println("=====================\n");
}

// ===== POWER MANAGEMENT =====
void BatteryManager::enablePowerSave() {
    // Reduce update frequency
    _updateInterval = 30000; // 30 seconds
    
    // Disable non-essential features
    Serial.println("Battery power save mode enabled");
}

void BatteryManager::disablePowerSave() {
    // Restore normal update frequency
    _updateInterval = 10000; // 10 seconds
    
    Serial.println("Battery power save mode disabled");
}

// ===== GETTERS =====
float BatteryManager::getVoltage() const { return _voltage; }
uint8_t BatteryManager::getPercentage() const { return _percentage; }
bool BatteryManager::isCharging() const { return _charging; }
uint8_t BatteryManager::getHealth() const { return _health; }
bool BatteryManager::isLow() const { return _percentage <= LOW_BATTERY_THRESHOLD; }
bool BatteryManager::isCritical() const { return _percentage <= CRITICAL_BATTERY_THRESHOLD; }
bool BatteryManager::isInitialized() const { return _initialized; }

// ===== STATIC ACCESS =====
BatteryManager& BatteryManager::getInstance() {
    if (!_instance) {
        _instance = new BatteryManager();
    }
    return *_instance;
}