#ifndef ALERT_MANAGER_H
#define ALERT_MANAGER_H

#include <Arduino.h>
#include "SystemConfig.h"

// Forward declarations
class BuzzerManager;
class CryptoData;
class DisplayManager;
struct SystemSettings;

class AlertManager {
private:
    // References to other managers
    BuzzerManager* buzzerMgr;
    CryptoData* cryptoData;
    DisplayManager* displayMgr;
    const SystemSettings* settings;
    
    // Alert state
    struct AlertState {
        bool active;
        byte mode;
        String symbol;
        String title;
        String message;
        float price;
        bool isLong;
        bool isSevere;
        unsigned long startTime;
        bool acknowledged;
        
        AlertState() : active(false), mode(0), price(0.0), 
                      isLong(false), isSevere(false), 
                      startTime(0), acknowledged(false) {}
    } currentAlert;
    
    // Alert cooldowns
    unsigned long lastPortfolioAlertTime;
    unsigned long lastPositionAlertTime[MAX_POSITIONS_PER_MODE];
    unsigned long lastExitAlertTime[MAX_POSITIONS_PER_MODE];
    
    // Statistics
    struct AlertStatistics {
        int totalAlerts;
        int portfolioAlerts;
        int positionAlerts;
        int exitAlerts;
        int severeAlerts;
        int acknowledgedAlerts;
        
        AlertStatistics() : totalAlerts(0), portfolioAlerts(0), 
                          positionAlerts(0), exitAlerts(0), 
                          severeAlerts(0), acknowledgedAlerts(0) {}
    } stats;
    
    // Configuration
    bool enabled;
    bool soundEnabled;
    bool visualEnabled;
    int cooldownPeriod;
    
public:
    AlertManager();
    
    // ===== INITIALIZATION =====
    void init(const SystemSettings& settings, BuzzerManager& buzzer, 
              CryptoData& data, DisplayManager& display);
    void enable(bool enable = true);
    void enableSound(bool enable = true);
    void enableVisual(bool enable = true);
    void setCooldown(int milliseconds);
    
    // ===== ALERT CHECKING =====
    void checkAlerts(byte mode);
    void checkPortfolioAlerts(byte mode);
    void checkPositionAlerts(byte mode);
    void checkExitAlerts(byte mode);
    void checkAllAlerts();
    
    // ===== ALERT TRIGGERING =====
    void triggerPortfolioAlert(byte mode, float pnlPercent, bool isSevere);
    void triggerPositionAlert(byte mode, int positionIndex, bool isSevere);
    void triggerExitAlert(byte mode, int positionIndex, bool isProfit, 
                         float changePercent);
    void triggerCustomAlert(const String& title, const String& symbol, 
                          const String& message, float price, 
                          bool isSevere, byte mode);
    
    // ===== ALERT MANAGEMENT =====
    bool isAlertActive() const;
    const AlertState& getCurrentAlert() const;
    void acknowledgeAlert();
    void clearAlert();
    void resetAll();
    void resetPortfolioAlerts(byte mode);
    void resetPositionAlerts(byte mode);
    void resetExitAlerts(byte mode);
    
    // ===== ALERT PROCESSING =====
    void processAlertQueue();
    void handleAlertTimeout();
    void updateAlertDisplay();
    
    // ===== COOLDOWN MANAGEMENT =====
    bool isCooldownActive(byte alertType, byte mode, int index = -1) const;
    void updateCooldown(byte alertType, byte mode, int index = -1);
    void resetCooldowns();
    
    // ===== NOTIFICATION METHODS =====
    void playAlertSound(bool isLong, bool isSevere, bool isExit = false, 
                       bool isProfit = false);
    void flashLEDs(byte mode, bool isLong, bool isSevere);
    void updateRGBForAlert(byte mode, float percentChange);
    
    // ===== STATISTICS =====
    AlertStatistics getStatistics() const;
    void resetStatistics();
    int getTotalAlerts() const;
    int getUnacknowledgedAlerts() const;
    float getAlertRate() const; // Alerts per hour
    
    // ===== CONFIGURATION =====
    void updateSettings(const SystemSettings& newSettings);
    void setThresholds(float normal, float severe, float portfolio);
    void setExitAlertPercent(float percent);
    
    // ===== DEBUG FUNCTIONS =====
    void printAlertState() const;
    void printStatistics() const;
    void testAllAlertTypes();
    
    // ===== UTILITY FUNCTIONS =====
    String getAlertTypeString(byte alertType) const;
    String getAlertSeverityString(bool isSevere) const;
    String generateAlertMessage(byte alertType, float value, 
                               const String& symbol = "") const;
    bool shouldAutoReset(float currentValue, float alertValue) const;
    
private:
    // Internal helper functions
    void initializeCooldowns();
    void playPortfolioAlertSound(bool isSevere);
    void playPositionAlertSound(bool isLong, bool isSevere);
    void playExitAlertSound(bool isProfit);
    
    bool checkPortfolioThreshold(const PortfolioSummary& summary) const;
    bool checkPositionThreshold(const CryptoPosition& position) const;
    bool checkExitThreshold(const CryptoPosition& position) const;
    
    void addToHistory(byte mode, const String& symbol, float pnlPercent, 
                     float price, bool isLong, bool isSevere, 
                     bool isProfit, byte alertType);
    
    void updateAlertHistoryCounters();
    void cleanupOldAlerts();
    
    // Cooldown helpers
    unsigned long getCooldownTime(byte alertType) const;
    bool isInCooldownPeriod(unsigned long lastTime) const;
    
    // Validation
    bool validateAlertParameters(byte mode, int positionIndex) const;
    bool validatePortfolioData(byte mode) const;
    
    // Message generation
    String generatePortfolioAlertMessage(float pnlPercent, bool isSevere) const;
    String generatePositionAlertMessage(const CryptoPosition& position, 
                                       bool isSevere) const;
    String generateExitAlertMessage(const CryptoPosition& position, 
                                   bool isProfit, float changePercent) const;
};

// Inline functions
inline bool AlertManager::isAlertActive() const {
    return currentAlert.active;
}

inline const AlertManager::AlertState& AlertManager::getCurrentAlert() const {
    return currentAlert;
}

inline int AlertManager::getTotalAlerts() const {
    return stats.totalAlerts;
}

inline void AlertManager::enable(bool enable) {
    this->enabled = enable;
}

inline void AlertManager::enableSound(bool enable) {
    this->soundEnabled = enable;
}

inline void AlertManager::enableVisual(bool enable) {
    this->visualEnabled = enable;
}

#endif // ALERT_MANAGER_H