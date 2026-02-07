#ifndef SETTINGS_MANAGER_H
#define SETTINGS_MANAGER_H

#include <EEPROM.h>
#include <Arduino.h>
#include "SystemConfig.h"

class SettingsManager {
private:
    // EEPROM management
    int eepromSize;
    int settingsAddress;
    int checksumAddress;
    
    // Current settings
    SystemSettings currentSettings;
    
    // Backup management
    SystemSettings backupSettings;
    bool backupValid;
    
    // Change tracking
    bool settingsChanged;
    unsigned long lastSaveTime;
    int saveCount;
    
    // Validation
    byte expectedMagic;
    int currentVersion;
    int savedVersion;
    
    // Statistics
    struct SettingsStats {
        int loadCount;
        int saveCount;
        int errorCount;
        int recoverCount;
        int backupCount;
        
        SettingsStats() : loadCount(0), saveCount(0), errorCount(0),
                         recoverCount(0), backupCount(0) {}
    } stats;
    
public:
    SettingsManager();
    ~SettingsManager();
    
    // ===== INITIALIZATION =====
    bool init(int size = EEPROM_SIZE, int address = 0);
    bool isInitialized() const;
    void setMagicNumber(byte magic);
    void setVersion(int version);
    
    // ===== SETTINGS MANAGEMENT =====
    bool loadSettings(SystemSettings& settings);
    bool saveSettings(const SystemSettings& settings);
    bool saveIfChanged();
    
    const SystemSettings& getSettings() const;
    SystemSettings& getSettings();
    
    void updateSettings(const SystemSettings& newSettings);
    void applyChanges();
    void discardChanges();
    
    // ===== DEFAULT SETTINGS =====
    void setDefaults(SystemSettings& settings);
    void restoreDefaults();
    void factoryReset();
    
    // ===== VALIDATION =====
    bool validateSettings(const SystemSettings& settings) const;
    bool verifyChecksum() const;
    uint32_t calculateChecksum(const SystemSettings& settings) const;
    bool compareSettings(const SystemSettings& a, const SystemSettings& b) const;
    
    // ===== BACKUP & RECOVERY =====
    bool createBackup();
    bool restoreBackup();
    bool hasBackup() const;
    void deleteBackup();
    
    bool recoverSettings();
    bool repairSettings();
    bool migrateSettings(int fromVersion, int toVersion);
    
    // ===== INDIVIDUAL SETTINGS =====
    // WiFi settings
    bool addWiFiNetwork(const String& ssid, const String& password,
                       byte priority = 5, bool autoConnect = true);
    bool removeWiFiNetwork(const String& ssid);
    bool updateWiFiNetwork(const String& ssid, const String& password = "",
                          byte priority = -1, bool autoConnect = true);
    WiFiNetwork* findWiFiNetwork(const String& ssid);
    
    // Alert settings
    void setAlertThresholds(float normal, float severe, float portfolio);
    void setBuzzerSettings(int volume, bool enabled);
    void setDisplaySettings(int brightness, int timeout, uint8_t rotation);
    void setLEDSettings(int brightness, bool enabled);
    void setRGBSettings(bool rgb1Enabled, bool rgb2Enabled,
                       int rgb1Brightness, int rgb2Brightness);
    
    // API settings
    void setAPISettings(const String& server, const String& username,
                       const String& password, const String& entryPortfolio,
                       const String& exitPortfolio);
    
    // System settings
    void setSystemSettings(bool autoReconnect, int reconnectAttempts,
                          bool showBattery, int batteryWarning);
    
    // ===== EEPROM MANAGEMENT =====
    bool formatEEPROM();
    bool clearEEPROM();
    bool verifyEEPROM();
    int getEEPROMUsage() const;
    String getEEPROMInfo() const;
    
    // ===== VERSION MANAGEMENT =====
    int getCurrentVersion() const;
    int getSavedVersion() const;
    bool needsMigration() const;
    bool performMigration();
    
    // ===== EXPORT/IMPORT =====
    String exportSettings() const;
    bool importSettings(const String& json);
    String exportToJSON() const;
    bool importFromJSON(const String& json);
    
    bool exportToFile(const String& filename);
    bool importFromFile(const String& filename);
    
    // ===== ENCRYPTION =====
    void enableEncryption(bool enable = true);
    bool isEncryptionEnabled() const;
    String encryptData(const String& data) const;
    String decryptData(const String& data) const;
    
    // ===== STATISTICS =====
    SettingsStats getStatistics() const;
    void resetStatistics();
    void printStatistics() const;
    
    // ===== DEBUG FUNCTIONS =====
    void printSettings() const;
    void printWiFiNetworks() const;
    void testEEPROM();
    void benchmark();
    
    // ===== UTILITY FUNCTIONS =====
    String settingsToString() const;
    bool stringToSettings(const String& str, SystemSettings& settings);
    
    bool validateWiFiNetwork(const WiFiNetwork& network) const;
    bool validateAPISettings(const SystemSettings& settings) const;
    bool validateAlertSettings(const SystemSettings& settings) const;
    
private:
    // Internal helper functions
    bool initEEPROM();
    void setupAddresses();
    
    // Read/Write operations
    bool readFromEEPROM(SystemSettings& settings);
    bool writeToEEPROM(const SystemSettings& settings);
    
    bool readWithChecksum(SystemSettings& settings);
    bool writeWithChecksum(const SystemSettings& settings);
    
    // Memory operations
    void* readBlock(int address, size_t size);
    bool writeBlock(int address, const void* data, size_t size);
    bool eraseBlock(int address, size_t size);
    
    // Validation helpers
    bool validateMagicNumber() const;
    bool validateStructure() const;
    bool validateNetworkCount() const;
    bool validateRange(float value, float min, float max) const;
    bool validateRange(int value, int min, int max) const;
    
    // Backup operations
    bool saveBackup(const SystemSettings& settings);
    bool loadBackup(SystemSettings& settings);
    int getBackupAddress() const;
    
    // Migration functions
    bool migrateV1toV2(SystemSettings& settings);
    bool migrateV2toV3(SystemSettings& settings);
    bool migrateV3toV4(SystemSettings& settings);
    
    // Encryption helpers
    void generateKey();
    bool hasKey() const;
    void rotateKey();
    
    // Error handling
    void logError(const String& operation, int errorCode);
    bool handleError(const String& operation, bool result);
    
    // Debug logging
    void logOperation(const String& operation, bool success);
    void dumpMemory(int address, int length);
};

// Inline functions
inline bool SettingsManager::isInitialized() const {
    return eepromSize > 0;
}

inline const SystemSettings& SettingsManager::getSettings() const {
    return currentSettings;
}

inline SystemSettings& SettingsManager::getSettings() {
    settingsChanged = true;
    return currentSettings;
}

inline bool SettingsManager::hasBackup() const {
    return backupValid;
}

inline int SettingsManager::getCurrentVersion() const {
    return currentVersion;
}

inline int SettingsManager::getSavedVersion() const {
    return savedVersion;
}

inline SettingsManager::SettingsStats SettingsManager::getStatistics() const {
    return stats;
}

// Default values
namespace DefaultSettings {
    const byte MAGIC_NUMBER = 0xAA;
    const int VERSION = 4;
    
    const float ALERT_THRESHOLD = -5.0;
    const float SEVERE_THRESHOLD = -10.0;
    const float PORTFOLIO_THRESHOLD = -7.0;
    
    const int BUZZER_VOLUME = 50;
    const bool BUZZER_ENABLED = true;
    
    const int DISPLAY_BRIGHTNESS = 100;
    const int DISPLAY_TIMEOUT = 30000;
    const uint8_t DISPLAY_ROTATION = 0;
    
    const int LED_BRIGHTNESS = 100;
    const bool LED_ENABLED = true;
    
    const bool RGB1_ENABLED = true;
    const bool RGB2_ENABLED = true;
    const int RGB1_BRIGHTNESS = 80;
    const int RGB2_BRIGHTNESS = 80;
    
    const bool SHOW_BATTERY = true;
    const int BATTERY_WARNING = 20;
    
    const bool AUTO_RECONNECT = true;
    const int RECONNECT_ATTEMPTS = 5;
    
    const float EXIT_ALERT_PERCENT = 3.0;
    const bool EXIT_ALERT_ENABLED = true;
}

#endif // SETTINGS_MANAGER_H