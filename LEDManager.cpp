#include "LEDManager.h"
#include "ConfigManager.h"
#include <Arduino.h>

// ===== CONSTRUCTOR/DESTRUCTOR =====
LEDManager::LEDManager() 
    : _mode1GreenState(false), _mode1RedState(false),
      _mode2GreenState(false), _mode2RedState(false),
      _blinkState(false), _blinking(false),
      _lastBlinkTime(0), _blinkInterval(500),
      _ledEnabled(true), _brightness(100) {
}

LEDManager::~LEDManager() {
    turnOffAll();
}

// ===== INITIALIZATION =====
void LEDManager::begin() {
    Serial.println("Initializing LEDs...");
    
    // Configure pins
    pinMode(LED_MODE1_GREEN, OUTPUT);
    pinMode(LED_MODE1_RED, OUTPUT);
    pinMode(LED_MODE2_GREEN, OUTPUT);
    pinMode(LED_MODE2_RED, OUTPUT);
    
    // Load settings
    _ledEnabled = ConfigManager::getInstance().getLEDEnabled();
    _brightness = ConfigManager::getInstance().getLEDBrightness();
    
    // Turn off all LEDs initially
    turnOffAll();
    
    Serial.print("LEDs initialized: ");
    Serial.print(_ledEnabled ? "ENABLED" : "DISABLED");
    Serial.print(", Brightness: ");
    Serial.print(_brightness);
    Serial.println("%");
}

void LEDManager::update() {
    if (!_ledEnabled) return;
    
    unsigned long currentTime = millis();
    
    // Handle blinking
    if (_blinking && (currentTime - _lastBlinkTime >= _blinkInterval)) {
        _blinkState = !_blinkState;
        _lastBlinkTime = currentTime;
        
        // Update LED states based on blinking
        updateLEDOutputs();
    }
    
    // Handle timeout for alert LEDs
    if (_alertTimeout > 0 && currentTime >= _alertTimeout) {
        resetAlertLEDs();
        _alertTimeout = 0;
    }
}

// ===== LED CONTROL =====
void LEDManager::setMode1LEDs(bool green, bool red) {
    _mode1GreenState = green;
    _mode1RedState = red;
    _blinking = false;
    updateLEDOutputs();
}

void LEDManager::setMode2LEDs(bool green, bool red) {
    _mode2GreenState = green;
    _mode2RedState = red;
    _blinking = false;
    updateLEDOutputs();
}

void LEDManager::setAlertLEDs(bool mode1Green, bool mode1Red, 
                              bool mode2Green, bool mode2Red, 
                              uint32_t timeoutMs) {
    _mode1GreenState = mode1Green;
    _mode1RedState = mode1Red;
    _mode2GreenState = mode2Green;
    _mode2RedState = mode2Red;
    _blinking = true;
    
    if (timeoutMs > 0) {
        _alertTimeout = millis() + timeoutMs;
    }
    
    updateLEDOutputs();
}

void LEDManager::blinkLEDs(uint8_t pattern[], uint8_t patternLength, uint8_t repeats) {
    if (!_ledEnabled) return;
    
    for (uint8_t r = 0; r < repeats; r++) {
        for (uint8_t i = 0; i < patternLength; i++) {
            uint8_t ledMask = pattern[i];
            
            // Set LEDs based on pattern
            digitalWrite(LED_MODE1_GREEN, (ledMask & 0x01) ? HIGH : LOW);
            digitalWrite(LED_MODE1_RED, (ledMask & 0x02) ? HIGH : LOW);
            digitalWrite(LED_MODE2_GREEN, (ledMask & 0x04) ? HIGH : LOW);
            digitalWrite(LED_MODE2_RED, (ledMask & 0x08) ? HIGH : LOW);
            
            delay(150); // Standard blink duration
            
            // Turn off
            turnOffAll();
            
            if (i < patternLength - 1) delay(100);
        }
        
        if (r < repeats - 1) delay(300);
    }
}

void LEDManager::testSequence() {
    if (!_ledEnabled) return;
    
    Serial.println("Testing LED sequence...");
    
    // Turn on each LED individually
    digitalWrite(LED_MODE1_GREEN, HIGH);
    delay(300);
    digitalWrite(LED_MODE1_GREEN, LOW);
    
    digitalWrite(LED_MODE1_RED, HIGH);
    delay(300);
    digitalWrite(LED_MODE1_RED, LOW);
    
    digitalWrite(LED_MODE2_GREEN, HIGH);
    delay(300);
    digitalWrite(LED_MODE2_GREEN, LOW);
    
    digitalWrite(LED_MODE2_RED, HIGH);
    delay(300);
    digitalWrite(LED_MODE2_RED, LOW);
    
    // All on
    turnOnAll();
    delay(500);
    turnOffAll();
    
    // Pattern
    uint8_t testPattern[] = {0x01, 0x02, 0x04, 0x08, 0x0F};
    blinkLEDs(testPattern, 5, 2);
    
    Serial.println("LED test complete");
}

void LEDManager::turnOffAll() {
    digitalWrite(LED_MODE1_GREEN, LOW);
    digitalWrite(LED_MODE1_RED, LOW);
    digitalWrite(LED_MODE2_GREEN, LOW);
    digitalWrite(LED_MODE2_RED, LOW);
}

void LEDManager::turnOnAll() {
    digitalWrite(LED_MODE1_GREEN, HIGH);
    digitalWrite(LED_MODE1_RED, HIGH);
    digitalWrite(LED_MODE2_GREEN, HIGH);
    digitalWrite(LED_MODE2_RED, HIGH);
}

void LEDManager::resetAlertLEDs() {
    _mode1GreenState = false;
    _mode1RedState = false;
    _mode2GreenState = false;
    _mode2RedState = false;
    _blinking = false;
    updateLEDOutputs();
}

// ===== SETTINGS CONTROL =====
void LEDManager::setBrightness(uint8_t brightness) {
    _brightness = constrain(brightness, 0, 100);
    ConfigManager::getInstance().setLEDBrightness(_brightness);
    
    Serial.print("LED brightness set to: ");
    Serial.print(_brightness);
    Serial.println("%");
}

void LEDManager::toggleEnabled() {
    _ledEnabled = !_ledEnabled;
    ConfigManager::getInstance().setLEDEnabled(_ledEnabled);
    
    if (!_ledEnabled) {
        turnOffAll();
    }
    
    Serial.print("LEDs ");
    Serial.println(_ledEnabled ? "enabled" : "disabled");
}

void LEDManager::setBlinkInterval(uint32_t intervalMs) {
    _blinkInterval = intervalMs;
    Serial.print("LED blink interval set to: ");
    Serial.print(intervalMs);
    Serial.println("ms");
}

// ===== PRIVATE METHODS =====
void LEDManager::updateLEDOutputs() {
    if (!_ledEnabled) {
        turnOffAll();
        return;
    }
    
    uint8_t greenValue = map(_brightness, 0, 100, 0, 255);
    
    if (_blinking) {
        // Blinking mode
        uint8_t state = _blinkState ? greenValue : 0;
        
        analogWrite(LED_MODE1_GREEN, _mode1GreenState ? state : 0);
        analogWrite(LED_MODE1_RED, _mode1RedState ? state : 0);
        analogWrite(LED_MODE2_GREEN, _mode2GreenState ? state : 0);
        analogWrite(LED_MODE2_RED, _mode2RedState ? state : 0);
    } else {
        // Solid mode
        analogWrite(LED_MODE1_GREEN, _mode1GreenState ? greenValue : 0);
        analogWrite(LED_MODE1_RED, _mode1RedState ? greenValue : 0);
        analogWrite(LED_MODE2_GREEN, _mode2GreenState ? greenValue : 0);
        analogWrite(LED_MODE2_RED, _mode2RedState ? greenValue : 0);
    }
}

// ===== WEB INTERFACE HANDLERS =====
void LEDManager::handleWebControl(const String& command, const String& params) {
    if (command == "test") {
        testSequence();
    } else if (command == "on") {
        turnOnAll();
    } else if (command == "off") {
        turnOffAll();
    } else if (command == "toggle") {
        toggleEnabled();
    } else if (command == "brightness") {
        setBrightness(params.toInt());
    } else if (command == "alert") {
        // params format: "mode1_green,mode1_red,mode2_green,mode2_red,timeout"
        int paramsArr[5] = {0};
        parseParams(params, paramsArr, 5);
        setAlertLEDs(paramsArr[0], paramsArr[1], paramsArr[2], paramsArr[3], paramsArr[4]);
    } else if (command == "pattern") {
        // Handle pattern command
        handlePatternCommand(params);
    }
}

String LEDManager::getStatusJSON() {
    String json = "{";
    json += "\"enabled\":" + String(_ledEnabled ? "true" : "false") + ",";
    json += "\"brightness\":" + String(_brightness) + ",";
    json += "\"blinking\":" + String(_blinking ? "true" : "false") + ",";
    json += "\"mode1_green\":" + String(_mode1GreenState ? "true" : "false") + ",";
    json += "\"mode1_red\":" + String(_mode1RedState ? "true" : "false") + ",";
    json += "\"mode2_green\":" + String(_mode2GreenState ? "true" : "false") + ",";
    json += "\"mode2_red\":" + String(_mode2RedState ? "true" : "false") + ",";
    json += "\"alert_timeout\":" + String(_alertTimeout);
    json += "}";
    return json;
}

void LEDManager::parseParams(const String& paramsStr, int* params, uint8_t count) {
    int startIdx = 0;
    int endIdx = 0;
    
    for (uint8_t i = 0; i < count; i++) {
        endIdx = paramsStr.indexOf(',', startIdx);
        if (endIdx == -1) endIdx = paramsStr.length();
        
        String param = paramsStr.substring(startIdx, endIdx);
        params[i] = param.toInt();
        
        startIdx = endIdx + 1;
        if (startIdx >= paramsStr.length()) break;
    }
}

void LEDManager::handlePatternCommand(const String& params) {
    // Parse pattern from params
    // Format: "pattern_length,repeat_count,pattern_bytes..."
    int patternInfo[2] = {0};
    parseParams(params, patternInfo, 2);
    
    uint8_t patternLength = patternInfo[0];
    uint8_t repeatCount = patternInfo[1];
    
    if (patternLength > 0 && patternLength <= 20) {
        uint8_t pattern[20];
        
        // Extract pattern bytes
        int startIdx = params.indexOf(',', params.indexOf(',') + 1) + 1;
        
        for (uint8_t i = 0; i < patternLength; i++) {
            int endIdx = params.indexOf(',', startIdx);
            if (endIdx == -1) endIdx = params.length();
            
            String byteStr = params.substring(startIdx, endIdx);
            pattern[i] = (uint8_t)byteStr.toInt();
            
            startIdx = endIdx + 1;
            if (startIdx >= params.length()) break;
        }
        
        blinkLEDs(pattern, patternLength, repeatCount);
    }
}

// ===== CONFIGURATION =====
void LEDManager::saveConfig() {
    // Configuration is saved automatically in setter methods
    Serial.println("LED configuration saved");
}

void LEDManager::loadConfig() {
    // Configuration is loaded in begin() method
    Serial.println("LED configuration loaded");
}

// ===== RGB LED CONTROL (Optional Integration) =====
void LEDManager::setRGBMode(bool mode1Enabled, bool mode2Enabled) {
    // This would integrate with RGBManager if needed
    Serial.print("RGB Mode - Mode1: ");
    Serial.print(mode1Enabled ? "ON" : "OFF");
    Serial.print(", Mode2: ");
    Serial.println(mode2Enabled ? "ON" : "OFF");
}

// ===== STATE QUERY METHODS =====
bool LEDManager::isAlertActive() const {
    return _mode1GreenState || _mode1RedState || 
           _mode2GreenState || _mode2RedState;
}

String LEDManager::getCurrentState() const {
    String state = "";
    
    if (_mode1GreenState) state += "M1-G ";
    if (_mode1RedState) state += "M1-R ";
    if (_mode2GreenState) state += "M2-G ";
    if (_mode2RedState) state += "M2-R ";
    
    if (_blinking) state += "[Blinking]";
    if (!_ledEnabled) state += "[Disabled]";
    
    return state;
}