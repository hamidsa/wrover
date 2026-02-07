#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>
#include <vector>
#include <functional>
#include "SystemConfig.h"

// Forward declarations
struct SystemSettings;

// Callback types
typedef std::function<void(bool connected)> ConnectionCallback;
typedef std::function<void(const String& ssid, int rssi)> ScanResultCallback;
typedef std::function<void(int progress)> ConnectionProgressCallback;

class WiFiManager {
private:
    // Configuration
    SystemSettings* settings;
    bool apEnabled;
    String apSSID;
    String apPassword;
    
    // Current state
    NetworkState state;
    String currentSSID;
    String currentIP;
    int currentRSSI;
    int currentChannel;
    
    // Connection management
    unsigned long connectionStartTime;
    unsigned long lastConnectAttempt;
    unsigned long lastScanTime;
    unsigned long lastReconnectAttempt;
    
    // Scanning
    bool isScanning;
    std::vector<WiFiNetwork> scannedNetworks;
    int scannedCount;
    
    // Callbacks
    ConnectionCallback connectionCallback;
    ScanResultCallback scanCallback;
    ConnectionProgressCallback progressCallback;
    
    // Statistics
    struct WiFiStatistics {
        unsigned long totalUptime;
        unsigned long totalDowntime;
        int connectionAttempts;
        int successfulConnections;
        int failedConnections;
        int reconnections;
        int scansPerformed;
        
        WiFiStatistics() : totalUptime(0), totalDowntime(0),
                          connectionAttempts(0), successfulConnections(0),
                          failedConnections(0), reconnections(0),
                          scansPerformed(0) {}
    } stats;
    
    // Connection parameters
    int connectTimeout;
    int reconnectInterval;
    int maxReconnectAttempts;
    int rssiThreshold;
    bool autoReconnect;
    bool prefer5GHz;
    
    // Error tracking
    int lastError;
    String lastErrorMsg;
    
public:
    WiFiManager();
    ~WiFiManager();
    
    // ===== INITIALIZATION =====
    void init(SystemSettings& settings, bool apEnabled = true);
    void setAPCredentials(const String& ssid, const String& password);
    
    // ===== CONNECTION MANAGEMENT =====
    bool connectToWiFi();
    bool connectToBestWiFi();
    bool connectToSSID(const String& ssid, const String& password = "");
    bool reconnect();
    void disconnect();
    void forceDisconnect();
    
    // ===== ACCESS POINT =====
    bool startAP();
    bool startAP(const String& ssid, const String& password);
    void stopAP();
    bool isAPRunning() const;
    String getAPIP() const;
    void enableAP(bool enable = true);
    bool isAPEnabled() const;
    
    // ===== SCANNING =====
    bool scanNetworks(bool async = false, bool showHidden = false);
    bool isScanning() const;
    int getScannedCount() const;
    const std::vector<WiFiNetwork>& getScannedNetworks() const;
    WiFiNetwork* getScannedNetwork(int index);
    
    // ===== NETWORK MANAGEMENT =====
    bool addNetwork(const String& ssid, const String& password, 
                   byte priority = 5, bool autoConnect = true);
    bool removeNetwork(const String& ssid);
    bool updateNetwork(const String& ssid, const String& password = "", 
                      byte priority = -1, bool autoConnect = true);
    bool networkExists(const String& ssid) const;
    int getNetworkCount() const;
    WiFiNetwork* getNetwork(int index);
    WiFiNetwork* getNetworkBySSID(const String& ssid);
    
    // ===== STATUS & INFORMATION =====
    bool isConnected() const;
    bool isConnecting() const;
    NetworkState getState() const;
    String getStateString() const;
    
    String getCurrentSSID() const;
    String getLocalIP() const;
    String getGatewayIP() const;
    String getSubnetMask() const;
    String getDNS() const;
    String getMAC() const;
    int getRSSI() const;
    int getChannel() const;
    String getBSSID() const;
    
    // ===== QUALITY & SIGNAL =====
    int getSignalStrength() const; // 0-100%
    String getSignalQuality() const;
    bool isSignalGood() const;
    bool isSignalFair() const;
    bool isSignalWeak() const;
    
    // ===== CALLBACKS =====
    void setConnectionCallback(ConnectionCallback callback);
    void setScanCallback(ScanResultCallback callback);
    void setProgressCallback(ConnectionProgressCallback callback);
    
    // ===== CONFIGURATION =====
    void setAutoReconnect(bool enable);
    void setReconnectInterval(unsigned long interval);
    void setMaxReconnectAttempts(int attempts);
    void setRSSIThreshold(int threshold);
    void setPrefer5GHz(bool prefer);
    void setConnectTimeout(unsigned long timeout);
    
    // ===== UPDATE LOOP =====
    void update();
    void checkConnection();
    void handleReconnection();
    
    // ===== WIFI MODE MANAGEMENT =====
    bool setMode(wifi_mode_t mode);
    wifi_mode_t getMode() const;
    bool enableSTA(bool enable = true);
    bool enableAPMode(bool enable = true);
    bool enableHybridMode();
    
    // ===== NETWORK DIAGNOSTICS =====
    bool ping(const String& host, int timeout = 1000);
    bool testInternetConnection();
    bool testGatewayConnection();
    bool testDNSServer();
    String performDiagnostics();
    
    // ===== ADVANCED SETTINGS =====
    void setPowerSaveMode(wifi_ps_type_t type);
    void setBandwidth(wifi_bandwidth_t bandwidth);
    void setChannel(int channel);
    void setTxPower(int power);
    
    // ===== SECURITY =====
    bool changeAPPassword(const String& newPassword);
    bool hideAP(bool hide = true);
    bool setAPMaxConnections(int max);
    
    // ===== STATISTICS =====
    WiFiStatistics getStatistics() const;
    void resetStatistics();
    float getUptimePercentage() const;
    float getSuccessRate() const;
    unsigned long getCurrentUptime() const;
    
    // ===== ERROR HANDLING =====
    int getLastError() const;
    String getLastErrorString() const;
    String getErrorString(int errorCode) const;
    void clearError();
    
    // ===== UTILITY FUNCTIONS =====
    String encryptPassword(const String& password);
    String generateAPSSID();
    bool validateSSID(const String& ssid);
    bool validatePassword(const String& password);
    
    // ===== SAVING & LOADING =====
    bool saveNetworks();
    bool loadNetworks();
    bool clearNetworks();
    
    // ===== DEBUG FUNCTIONS =====
    void printNetworks() const;
    void printCurrentStatus() const;
    void printScanResults() const;
    void testAllNetworks();
    
private:
    // Internal helper functions
    void initializeWiFi();
    void setupEventHandlers();
    void removeEventHandlers();
    
    // Connection helpers
    bool attemptConnection(const String& ssid, const String& password);
    bool waitForConnection(unsigned long timeout);
    void handleConnectionResult(bool success);
    
    // Network selection
    WiFiNetwork* selectBestNetwork();
    int calculateNetworkScore(const WiFiNetwork& network, int rssi) const;
    bool isNetworkAvailable(const String& ssid) const;
    
    // Scanning helpers
    static void scanCompleteHandler(int networksFound);
    void processScanResults(int networksFound);
    void sortScannedNetworks();
    
    // AP management
    bool configureAP();
    bool validateAPConfig() const;
    
    // Callback handlers
    void onStationConnected();
    void onStationDisconnected();
    void onAPStarted();
    void onAPStopped();
    void onScanComplete(int networksFound);
    
    // Event handlers
    static void WiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info);
    void handleWiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info);
    
    // Error handling
    void setError(int code, const String& message = "");
    void logError(const String& message);
    
    // Configuration validation
    bool validateConfiguration() const;
    bool validateNetwork(const WiFiNetwork& network) const;
    
    // Power management
    void updatePowerSaveMode();
    
    // Security
    String generateRandomPassword(int length = 12);
    
    // Debug
    void logConnectionAttempt(const String& ssid, bool success);
    void logStateChange(NetworkState oldState, NetworkState newState);
};

// Inline functions
inline bool WiFiManager::isConnected() const {
    return state == NET_ONLINE;
}

inline bool WiFiManager::isConnecting() const {
    return state == NET_CONNECTING;
}

inline bool WiFiManager::isAPRunning() const {
    return WiFi.getMode() & WIFI_MODE_AP;
}

inline bool WiFiManager::isAPEnabled() const {
    return apEnabled;
}

inline bool WiFiManager::isScanning() const {
    return isScanning;
}

inline int WiFiManager::getScannedCount() const {
    return scannedCount;
}

inline NetworkState WiFiManager::getState() const {
    return state;
}

inline String WiFiManager::getCurrentSSID() const {
    return currentSSID;
}

inline String WiFiManager::getLocalIP() const {
    return currentIP;
}

inline int WiFiManager::getRSSI() const {
    return currentRSSI;
}

inline int WiFiManager::getNetworkCount() const {
    return settings ? settings->networkCount : 0;
}

inline void WiFiManager::setAutoReconnect(bool enable) {
    autoReconnect = enable;
}

inline void WiFiManager::enableAP(bool enable) {
    apEnabled = enable;
}

inline int WiFiManager::getLastError() const {
    return lastError;
}

#endif // WIFI_MANAGER_H