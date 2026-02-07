#ifndef BATTERY_MANAGER_H
#define BATTERY_MANAGER_H

#include <Arduino.h>
#include "SystemConfig.h"

class BatteryManager {
private:
    // Hardware configuration
    int batteryPin;
    float voltageDividerRatio;
    float referenceVoltage;
    int adcResolution;
    
    // Calibration
    float calibrationOffset;
    float calibrationFactor;
    
    // Battery parameters
    float fullVoltage;
    float emptyVoltage;
    float chargingVoltage;
    
    // Current state
    float voltage;
    float smoothedVoltage;
    int percentage;
    PowerSource powerSource;
    bool isCharging;
    bool isLow;
    bool isCritical;
    
    // Measurement
    unsigned long lastMeasurement;
    int measurementInterval;
    std::vector<float> voltageHistory;
    int historySize;
    
    // Statistics
    struct BatteryStats {
        unsigned long totalUptime;
        unsigned long batteryUptime;
        unsigned long usbUptime;
        int chargeCycles;
        float minVoltage;
        float maxVoltage;
        float avgVoltage;
        
        BatteryStats() : totalUptime(0), batteryUptime(0), usbUptime(0),
                        chargeCycles(0), minVoltage(999.0), maxVoltage(0),
                        avgVoltage(0) {}
    } stats;
    
    // Callbacks
    typedef std::function<void(int percent)> BatteryCallback;
    BatteryCallback lowCallback;
    BatteryCallback criticalCallback;
    BatteryCallback chargedCallback;
    
public:
    BatteryManager(int pin = BATTERY_PIN);
    
    // ===== INITIALIZATION =====
    void init();
    void calibrate(float knownVoltage);
    void setCalibration(float offset, float factor);
    
    // ===== CONFIGURATION =====
    void setVoltageRange(float full, float empty);
    void setChargingVoltage(float voltage);
    void setMeasurementInterval(int interval);
    void setHistorySize(int size);
    
    // ===== MEASUREMENT =====
    float readVoltage();
    int readPercentage();
    PowerSource detectPowerSource();
    bool isBatteryConnected() const;
    
    void update();
    void checkBattery();
    
    // ===== STATE INFORMATION =====
    float getVoltage() const;
    int getPercentage() const;
    PowerSource getPowerSource() const;
    bool isCharging() const;
    bool isLowBattery() const;
    bool isCriticalBattery() const;
    
    String getPowerSourceString() const;
    String getBatteryStatus() const;
    
    // ===== HISTORY & TRENDS =====
    float getAverageVoltage() const;
    float getMinVoltage() const;
    float getMaxVoltage() const;
    float getVoltageTrend() const; // volts per hour
    int getRemainingTime() const; // minutes
    
    std::vector<float> getVoltageHistory() const;
    void clearHistory();
    
    // ===== CALLBACKS =====
    void setLowCallback(BatteryCallback callback);
    void setCriticalCallback(BatteryCallback callback);
    void setChargedCallback(BatteryCallback callback);
    
    // ===== POWER MANAGEMENT =====
    void enablePowerSave(bool enable = true);
    void setLowPowerMode(bool enable);
    bool shouldShutdown() const;
    
    // ===== STATISTICS =====
    BatteryStats getStatistics() const;
    void resetStatistics();
    float getBatteryHealth() const; // 0-100%
    
    // ===== DEBUG FUNCTIONS =====
    void printStatus() const;
    void printStatistics() const;
    void testMeasurement(int samples = 100);
    
    // ===== UTILITY FUNCTIONS =====
    float voltageToPercentage(float volts) const;
    float percentageToVoltage(int percent) const;
    String voltageToString(float volts) const;
    
private:
    // Internal helper functions
    void setupADC();
    int readADC();
    float rawToVoltage(int raw);
    
    // Smoothing and filtering
    void applySmoothing();
    float exponentialSmoothing(float newValue, float oldValue, float alpha);
    
    // State detection
    void updatePowerSource();
    void updateBatteryStatus();
    void checkThresholds();
    
    // History management
    void addToHistory(float voltage);
    void updateStatistics(float voltage);
    
    // Callback triggers
    void triggerLowBattery();
    void triggerCriticalBattery();
    void triggerCharged();
    
    // Calibration
    void autoCalibrate();
    bool needsCalibration() const;
    
    // Safety checks
    bool validateVoltage(float volts) const;
    bool validatePercentage(int percent) const;
    
    // Debug
    void logMeasurement(float voltage, int percentage);
};

// Inline functions
inline float BatteryManager::getVoltage() const {
    return smoothedVoltage;
}

inline int BatteryManager::getPercentage() const {
    return percentage;
}

inline PowerSource BatteryManager::getPowerSource() const {
    return powerSource;
}

inline bool BatteryManager::isCharging() const {
    return isCharging;
}

inline bool BatteryManager::isLowBattery() const {
    return isLow;
}

inline bool BatteryManager::isCriticalBattery() const {
    return isCritical;
}

inline bool BatteryManager::isBatteryConnected() const {
    return powerSource == POWER_SOURCE_BATTERY || 
           powerSource == POWER_SOURCE_EXTERNAL;
}

inline BatteryManager::BatteryStats BatteryManager::getStatistics() const {
    return stats;
}

inline void BatteryManager::setLowCallback(BatteryCallback callback) {
    lowCallback = callback;
}

inline void BatteryManager::setCriticalCallback(BatteryCallback callback) {
    criticalCallback = callback;
}

inline void BatteryManager::setChargedCallback(BatteryCallback callback) {
    chargedCallback = callback;
}

// Default values
namespace BatteryConfig {
    const float DEFAULT_FULL_VOLTAGE = 4.2;
    const float DEFAULT_EMPTY_VOLTAGE = 3.0;
    const float DEFAULT_CHARGING_VOLTAGE = 5.0;
    const float VOLTAGE_DIVIDER_RATIO = 2.0;
    const float REFERENCE_VOLTAGE = 3.3;
    const int ADC_RESOLUTION = 4095;
    const int MEASUREMENT_INTERVAL = 60000; // 1 minute
    const int HISTORY_SIZE = 60; // 1 hour of data
    const int LOW_THRESHOLD = 20; // percentage
    const int CRITICAL_THRESHOLD = 10; // percentage
}

#endif // BATTERY_MANAGER_H