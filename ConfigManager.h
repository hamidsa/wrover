#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <Arduino.h>
#include <Preferences.h>

class ConfigManager {
private:
    Preferences _prefs;
    static ConfigManager* _instance;
    
    ConfigManager() {}
    
public:
    static ConfigManager& getInstance();
    
    void begin();
    
    // WiFi settings
    String getWiFiSSID();
    void setWiFiSSID(const String& ssid);
    String getWiFiPassword();
    void setWiFiPassword(const String& password);
    bool getWiFiAutoConnect();
    void setWiFiAutoConnect(bool autoConnect);
    bool getAPEnabled();
    void setAPEnabled(bool enabled);
    
    // API settings
    String getAPIServer();
    void setAPIServer(const String& server);
    String getAPIUsername();
    void setAPIUsername(const String& username);
    String getAPIPassword();
    void setAPIPassword(const String& password);
    String getEntryPortfolio();
    void setEntryPortfolio(const String& portfolio);
    String getExitPortfolio();
    void setExitPortfolio(const String& portfolio);
    
    // Alert settings
    float getAlertThreshold();
    void setAlertThreshold(float threshold);
    float getSevereThreshold();
    void setSevereThreshold(float threshold);
    float getPortfolioThreshold();
    void setPortfolioThreshold(float threshold);
    uint8_t getBuzzerVolume();
    void setBuzzerVolume(uint8_t volume);
    bool getBuzzerEnabled();
    void setBuzzerEnabled(bool enabled);
    
    // General getters/setters
    String getString(const char* key, const String& defaultValue = "");
    void putString(const char* key, const String& value);
    int getInt(const char* key, int defaultValue = 0);
    void putInt(const char* key, int value);
    float getFloat(const char* key, float defaultValue = 0.0);
    void putFloat(const char* key, float value);
    bool getBool(const char* key, bool defaultValue = false);
    void putBool(const char* key, bool value);
    uint8_t getUChar(const char* key, uint8_t defaultValue = 0);
    void putUChar(const char* key, uint8_t value);
    uint32_t getUInt(const char* key, uint32_t defaultValue = 0);
    void putUInt(const char* key, uint32_t value);
    
    // Utility
    void clear();
    void factoryReset();
    String getAllSettingsJSON();
};

#endif