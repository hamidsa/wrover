#ifndef DATAMANAGER_H
#define DATAMANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>
#include <string>

// ساختار برای موقعیت‌های کریپتو
typedef struct {
    char symbol[16];
    float changePercent;
    float pnlValue;
    float quantity;
    float entryPrice;
    float currentPrice;
    bool isLong;
    bool alerted;
    bool severeAlerted;
    unsigned long lastAlertTime;
    float lastAlertPrice;
    float alertThreshold;
    float severeThreshold;
    
    // فیلدهای اضافی برای Exit Mode
    bool exitAlerted;
    float exitAlertLastPrice;
    unsigned long exitAlertTime;
    bool hasAlerted;
    float lastAlertPercent;
    
    // فیلدهای اضافی از JSON اصلی
    char positionSide[12];
    char marginType[12];
    float leverage;
    float liquidationPrice;
} CryptoPosition;

// ساختار برای خلاصه پرتفولیو
typedef struct {
    float totalInvestment;
    float totalCurrentValue;
    float totalPnl;
    float totalPnlPercent;
    int totalPositions;
    int longPositions;
    int shortPositions;
    int winningPositions;
    int losingPositions;
    float maxDrawdown;
    float sharpeRatio;
    float avgPositionSize;
    float riskExposure;
} PortfolioSummary;

// ساختار برای تاریخچه موقعیت
typedef struct {
    char symbol[16];
    std::vector<float> priceHistory;
    unsigned long lastUpdate;
    float lastPrice;
    float changePercent;
} PositionHistory;

class DataManager {
public:
    static DataManager& getInstance();
    
    // Initialization
    bool begin();
    void update();
    
    // Data parsing
    bool parsePortfolioData(const String& jsonData, bool isExitMode);
    bool fetchData(bool isExitMode);
    bool fetchAllData();
    
    // Data access
    const CryptoPosition* getPositions(bool isExitMode = false) const;
    int getPositionCount(bool isExitMode = false) const;
    const PortfolioSummary& getSummary(bool isExitMode = false) const;
    CryptoPosition* getPosition(const char* symbol, bool isExitMode = false);
    
    // Data analysis
    void sortPositionsByPNL(bool isExitMode, bool ascending = true);
    CryptoPosition* getWorstPosition(bool isExitMode = false);
    CryptoPosition* getBestPosition(bool isExitMode = false);
    
    // Data management
    void clearAllData();
    void clearData(bool isExitMode);
    bool hasData(bool isExitMode = false) const;
    
    // History management
    void updatePositionHistory(bool isExitMode);
    
    // JSON output
    String getDataJSON(bool isExitMode = false);
    
    // Utility
    unsigned long getLastUpdateTime() const;
    void printSummary(bool isExitMode = false);
    bool isInitialized() const;
    
private:
    DataManager();
    ~DataManager();
    
    static DataManager* _instance;
    
    // Data storage
    CryptoPosition _entryPositions[100];  // MAX_POSITIONS_PER_MODE
    CryptoPosition _exitPositions[100];   // MAX_POSITIONS_PER_MODE
    PortfolioSummary _entrySummary;
    PortfolioSummary _exitSummary;
    
    // History
    std::vector<PositionHistory> _entryPositionHistory;
    std::vector<PositionHistory> _exitPositionHistory;
    
    // State
    bool _initialized;
    int _entryPositionCount;
    int _exitPositionCount;
    unsigned long _lastUpdateTime;
    unsigned long _updateInterval;
    
    // Preferences
    Preferences _prefs;
    
    // Helper methods
    bool parsePosition(JsonObject& item, CryptoPosition& position);
    void parseSummary(JsonObject& summary, bool isExitMode);
    void calculateDerivedMetrics(bool isExitMode);
    void saveDataSnapshot(bool isExitMode);
    void loadHistoricalData();
    void saveDetailedDataToFile(bool isExitMode);
};

#endif