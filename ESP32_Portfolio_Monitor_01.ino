/* ============================================================================
   PORTFOLIO MONITOR - ESP32-WROVER-E - Modular Version
   Version: 4.5.3 - Modular Architecture
   Author: AI Assistant
   ============================================================================ */

#include <Arduino.h>
#include "SystemConfig.h"
#include "CryptoData.h"
#include "DisplayManager.h"
#include "AlertManager.h"
#include "DataProcessor.h"
#include "BuzzerManager.h"
#include "LEDManager.h"
#include "SettingsManager.h"
#include "WiFiManager.h"
#include "WebInterface.h"
#include "BatteryManager.h"
#include "TimeManager.h"
#include "APIManager.h"

// ===== GLOBAL OBJECTS =====
DisplayManager displayMgr;
AlertManager alertMgr;
DataProcessor dataProcessor;
BuzzerManager buzzerMgr;
LEDManager ledMgr;
SettingsManager settingsMgr;
WiFiManager wifiMgr;
WebInterface webInterface;
BatteryManager batteryMgr;
TimeManager timeMgr;
APIManager apiMgr;

// ===== GLOBAL VARIABLES =====
SystemSettings settings;
CryptoData cryptoData;
SystemState systemState;
unsigned long systemStartTime = 0;
bool apEnabled = true;

// ===== FUNCTION PROTOTYPES =====
void initializeSystem();
void updateSystem();
void handleSystemEvents();
void checkResetButton();
void manageWiFiMode();

// ===== SETUP =====
void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n\n========================================");
    Serial.println("PORTFOLIO MONITOR v4.5.3 - MODULAR");
    Serial.println("ESP32-WROVER-E with Enhanced WiFi");
    Serial.println("========================================");
    
    systemStartTime = millis();
    
    // Initialize all managers
    initializeSystem();
    
    // Display splash screen
    displayMgr.showSplashScreen();
    
    // Play startup tone
    buzzerMgr.playStartupTone();
    
    Serial.println("\n✅ System initialized successfully!");
    Serial.println("Free Heap: " + String(ESP.getFreeHeap()) + " bytes");
    Serial.println("========================================");
}

// ===== LOOP =====
void loop() {
    unsigned long currentTime = millis();
    
    // 1. Handle web server
    webInterface.handleClient();
    
    // 2. Update WiFi
    wifiMgr.update();
    
    // 3. Manage WiFi mode
    manageWiFiMode();
    
    // 4. Check battery
    if (currentTime - systemState.lastBatteryCheck > BATTERY_CHECK_INTERVAL) {
        batteryMgr.checkBattery();
        systemState.lastBatteryCheck = currentTime;
    }
    
    // 5. Update time if connected
    if (wifiMgr.isConnected()) {
        timeMgr.update();
    }
    
    // 6. Update data from API
    if (wifiMgr.isConnected() && 
        (currentTime - systemState.lastDataUpdate > DATA_UPDATE_INTERVAL)) {
        
        // Entry Mode data
        if (strlen(settings.entryPortfolio) > 0) {
            String data = apiMgr.fetchPortfolioData(0);
            if (data != "{}") {
                dataProcessor.parseData(data, 0);
                cryptoData.calculatePortfolioSummary(0);
            }
        }
        
        // Exit Mode data
        if (strlen(settings.exitPortfolio) > 0) {
            String data = apiMgr.fetchPortfolioData(1);
            if (data != "{}") {
                dataProcessor.parseData(data, 1);
                cryptoData.calculatePortfolioSummary(1);
            }
        }
        
        systemState.lastDataUpdate = currentTime;
    }
    
    // 7. Check alerts
    if (currentTime - systemState.lastAlertCheck > 5000) {
        if (cryptoData.getCount(0) > 0) alertMgr.checkAlerts(0);
        if (cryptoData.getCount(1) > 0) alertMgr.checkAlerts(1);
        systemState.lastAlertCheck = currentTime;
    }
    
    // 8. Update display
    if (currentTime - systemState.lastDisplayUpdate > DISPLAY_UPDATE_INTERVAL) {
        displayMgr.updateMainScreen(systemState, cryptoData);
        systemState.lastDisplayUpdate = currentTime;
    }
    
    // 9. Update LEDs
    ledMgr.update(systemState);
    
    // 10. Check reset button
    checkResetButton();
    
    // 11. Handle system events
    handleSystemEvents();
    
    delay(2);
}

// ===== SYSTEM INITIALIZATION =====
void initializeSystem() {
    Serial.println("\n🔧 Initializing System Components...");
    
    // 1. Load settings
    Serial.print("  Loading settings... ");
    settingsMgr.loadSettings(settings);
    Serial.println("✅");
    
    // 2. Initialize display
    Serial.print("  Initializing display... ");
    displayMgr.init(settings.displayBrightness, settings.displayRotation);
    Serial.println("✅");
    
    // 3. Initialize buzzer
    Serial.print("  Initializing buzzer... ");
    buzzerMgr.init(settings.buzzerVolume, settings.buzzerEnabled);
    Serial.println("✅");
    
    // 4. Initialize LEDs
    Serial.print("  Initializing LEDs... ");
    ledMgr.init(settings.ledBrightness, settings.ledEnabled,
                settings.rgb1Brightness, settings.rgb2Brightness,
                settings.rgb1Enabled, settings.rgb2Enabled);
    Serial.println("✅");
    
    // 5. Initialize WiFi
    Serial.print("  Initializing WiFi... ");
    wifiMgr.init(settings, apEnabled);
    Serial.println("✅");
    
    // 6. Initialize web interface
    Serial.print("  Initializing web interface... ");
    webInterface.init(settings, wifiMgr, displayMgr, buzzerMgr, 
                      cryptoData, systemState);
    Serial.println("✅");
    
    // 7. Initialize battery manager
    Serial.print("  Initializing battery manager... ");
    batteryMgr.init();
    Serial.println("✅");
    
    // 8. Initialize time manager
    Serial.print("  Initializing time manager... ");
    timeMgr.init();
    Serial.println("✅");
    
    // 9. Initialize API manager
    Serial.print("  Initializing API manager... ");
    apiMgr.init(settings);
    Serial.println("✅");
    
    // 10. Initialize alert manager
    Serial.print("  Initializing alert manager... ");
    alertMgr.init(settings, buzzerMgr, cryptoData, displayMgr);
    Serial.println("✅");
    
    // Initialize system state
    systemState.lastDataUpdate = millis() - DATA_UPDATE_INTERVAL;
    systemState.lastAlertCheck = millis();
    systemState.lastDisplayUpdate = millis();
    systemState.lastBatteryCheck = millis();
    systemState.connectionLost = false;
    systemState.isConnectedToWiFi = false;
    systemState.apModeActive = false;
    systemState.powerSource = POWER_SOURCE_USB;
    
    Serial.println("🎯 System initialization complete!");
}

// ===== SYSTEM EVENT HANDLER =====
void handleSystemEvents() {
    static bool lastConnectedState = false;
    
    bool currentConnectedState = wifiMgr.isConnected();
    
    if (currentConnectedState != lastConnectedState) {
        if (currentConnectedState) {
            Serial.println("\n✅ WiFi Connection Established");
            Serial.println("   SSID: " + wifiMgr.getCurrentSSID());
            Serial.println("   IP: " + wifiMgr.getLocalIP());
            buzzerMgr.playSuccessTone();
            
            // Sync time after connection
            timeMgr.syncTime();
        } else {
            Serial.println("\n⚠️ WiFi Connection Lost");
            buzzerMgr.playConnectionLostTone();
            systemState.connectionLost = true;
            systemState.connectionLostTime = millis();
        }
        lastConnectedState = currentConnectedState;
    }
    
    // Update WiFi state in system state
    systemState.isConnectedToWiFi = currentConnectedState;
    systemState.apModeActive = wifiMgr.isAPActive();
    
    // Check battery warning
    if (batteryMgr.isLow() && !systemState.batteryLow) {
        systemState.batteryLow = true;
        displayMgr.showWarning("BATTERY LOW", 
                              String(batteryMgr.getPercent()) + "%");
        Serial.println("⚠️ Battery Low: " + String(batteryMgr.getPercent()) + "%");
    }
}

// ===== RESET BUTTON HANDLER =====
void checkResetButton() {
    static unsigned long pressStart = 0;
    static bool buttonPressed = false;
    
    if (digitalRead(RESET_BUTTON_PIN) == LOW) {
        if (!buttonPressed) {
            buttonPressed = true;
            pressStart = millis();
        }
        
        unsigned long holdTime = millis() - pressStart;
        
        if (holdTime > 10000 && !systemState.resetInProgress) {
            systemState.resetInProgress = true;
            Serial.println("\n🚨 FACTORY RESET TRIGGERED (10s hold)");
            displayMgr.showMessage("FACTORY RESET", "In Progress", "Please wait...");
            settingsMgr.factoryReset();
            displayMgr.showMessage("FACTORY RESET", "Complete", "Restarting...");
            delay(2000);
            ESP.restart();
        } else if (holdTime > 3000) {
            // Show countdown
            int secondsLeft = 10 - (holdTime / 1000);
            if (secondsLeft >= 0 && secondsLeft <= 7) {
                displayMgr.showMessage("HOLD FOR RESET", 
                                      "Release to cancel", 
                                      String(secondsLeft) + " seconds...");
            }
        }
    } else {
        if (buttonPressed) {
            buttonPressed = false;
            unsigned long holdTime = millis() - pressStart;
            
            if (holdTime > 500 && holdTime < 3000) {
                Serial.println("\n🔄 Short press - Resetting alerts");
                alertMgr.resetAll();
                displayMgr.showMessage("ALERTS RESET", "All alerts cleared");
                delay(1000);
            }
        }
    }
}

// ===== WIFI MODE MANAGEMENT =====
void manageWiFiMode() {
    static unsigned long lastCheck = 0;
    unsigned long now = millis();
    
    if (now - lastCheck < 10000) return;
    lastCheck = now;
    
    bool connectedNow = wifiMgr.isConnected();
    bool apActiveNow = wifiMgr.isAPActive();
    
    if (apEnabled) {
        if (connectedNow && !apActiveNow) {
            Serial.println("📡 Switching to AP+STA mode");
            wifiMgr.enableAPMode(true);
        } else if (!connectedNow && !apActiveNow) {
            Serial.println("📡 Starting AP mode");
            wifiMgr.enableAPMode(false);
        }
    } else {
        if (apActiveNow) {
            Serial.println("📡 Disabling AP mode");
            wifiMgr.disableAPMode();
        }
    }
}

// ===== UTILITY FUNCTIONS =====
String getUptimeString() {
    unsigned long uptime = millis() - systemStartTime;
    unsigned long seconds = uptime / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;
    unsigned long days = hours / 24;
    
    String result = "";
    if (days > 0) result += String(days) + "d ";
    if (hours % 24 > 0) result += String(hours % 24) + "h ";
    if (minutes % 60 > 0) result += String(minutes % 60) + "m ";
    result += String(seconds % 60) + "s";
    return result;
}

void printSystemStatus() {
    Serial.println("\n📊 SYSTEM STATUS:");
    Serial.println("  Uptime: " + getUptimeString());
    Serial.println("  Free Heap: " + String(ESP.getFreeHeap()) + " bytes");
    Serial.println("  WiFi: " + String(systemState.isConnectedToWiFi ? "Connected" : "Disconnected"));
    Serial.println("  AP Mode: " + String(systemState.apModeActive ? "Active" : "Inactive"));
    Serial.println("  Entry Positions: " + String(cryptoData.getCount(0)));
    Serial.println("  Exit Positions: " + String(cryptoData.getCount(1)));
    Serial.println("  Battery: " + String(batteryMgr.getPercent()) + "%");
    Serial.println("  Volume: " + String(settings.buzzerVolume) + "%");
}