#ifndef CRYPTO_DATA_H
#define CRYPTO_DATA_H

#include <Arduino.h>
#include <vector>
#include "SystemConfig.h"

class CryptoData {
private:
    // Entry Mode data
    CryptoPosition positionsMode1[MAX_POSITIONS_PER_MODE];
    PortfolioSummary summaryMode1;
    AlertHistory alertHistoryMode1[MAX_ALERT_HISTORY];
    int countMode1;
    int alertHistoryCountMode1;
    
    // Exit Mode data
    CryptoPosition positionsMode2[MAX_POSITIONS_PER_MODE];
    PortfolioSummary summaryMode2;
    AlertHistory alertHistoryMode2[MAX_ALERT_HISTORY];
    int countMode2;
    int alertHistoryCountMode2;
    
public:
    CryptoData();
    
    // ===== POSITION MANAGEMENT =====
    
    // Add new position
    bool addPosition(byte mode, const char* symbol, float changePercent, 
                     float pnlValue, float quantity, float entryPrice, 
                     float currentPrice, bool isLong);
    
    // Update existing position
    bool updatePosition(byte mode, int index, float currentPrice, 
                       float changePercent, float pnlValue);
    
    // Get position by index
    CryptoPosition* getPosition(byte mode, int index);
    
    // Get position by symbol
    CryptoPosition* getPositionBySymbol(byte mode, const char* symbol);
    
    // Remove position
    bool removePosition(byte mode, int index);
    
    // Clear all positions
    void clearPositions(byte mode);
    
    // ===== DATA ACCESS =====
    
    // Get counts
    int getCount(byte mode) const;
    int getLongCount(byte mode) const;
    int getShortCount(byte mode) const;
    int getWinningCount(byte mode) const;
    int getLosingCount(byte mode) const;
    
    // Get portfolio summary
    PortfolioSummary* getSummary(byte mode);
    const PortfolioSummary* getSummary(byte mode) const;
    
    // Get alert history
    AlertHistory* getAlertHistory(byte mode, int index);
    int getAlertHistoryCount(byte mode) const;
    
    // ===== CALCULATIONS =====
    
    // Calculate portfolio summary
    void calculatePortfolioSummary(byte mode);
    
    // Sort positions by loss (worst first)
    void sortPositionsByLoss(byte mode);
    
    // Sort positions by profit (best first)
    void sortPositionsByProfit(byte mode);
    
    // Find worst performing position
    CryptoPosition* getWorstPosition(byte mode);
    
    // Find best performing position
    CryptoPosition* getBestPosition(byte mode);
    
    // ===== ALERT MANAGEMENT =====
    
    // Add alert to history
    bool addAlertHistory(byte mode, const char* symbol, float pnlPercent, 
                        float price, bool isLong, bool isSevere, 
                        bool isProfit, byte alertType, byte alertMode);
    
    // Acknowledge alert
    bool acknowledgeAlert(byte mode, int index);
    
    // Clear alert history
    void clearAlertHistory(byte mode);
    
    // Get unacknowledged alerts count
    int getUnacknowledgedAlertsCount(byte mode) const;
    
    // ===== STATISTICS =====
    
    // Get total values
    float getTotalInvestment(byte mode) const;
    float getTotalCurrentValue(byte mode) const;
    float getTotalPnL(byte mode) const;
    float getTotalPnLPercent(byte mode) const;
    
    // Get averages
    float getAveragePnLPercent(byte mode) const;
    float getAveragePositionSize(byte mode) const;
    
    // Get min/max
    float getMaxProfit(byte mode) const;
    float getMaxLoss(byte mode) const;
    
    // ===== UTILITY FUNCTIONS =====
    
    // Check if symbol exists
    bool symbolExists(byte mode, const char* symbol) const;
    
    // Find position index by symbol
    int findPositionIndex(byte mode, const char* symbol) const;
    
    // Reset all alerts for positions
    void resetAllAlerts(byte mode);
    
    // Clear exit alerts
    void clearExitAlerts(byte mode);
    
    // Update exit alert prices
    void updateExitAlertPrices(byte mode);
    
    // ===== DEBUG FUNCTIONS =====
    
    // Print all positions
    void printPositions(byte mode) const;
    
    // Print portfolio summary
    void printSummary(byte mode) const;
    
    // Print alert history
    void printAlertHistory(byte mode) const;
    
private:
    // Internal helper functions
    void initializePosition(CryptoPosition* pos, const char* symbol, 
                          float changePercent, float pnlValue, 
                          float quantity, float entryPrice, 
                          float currentPrice, bool isLong);
    
    void calculateSinglePositionPnL(CryptoPosition* pos);
    
    void bubbleSortPositions(CryptoPosition* positions, int count, 
                           bool (*compare)(const CryptoPosition&, const CryptoPosition&));
    
    void shiftAlertHistory(AlertHistory* history, int* count, int startIndex);
    
    bool addAlertToHistory(AlertHistory* history, int* count, 
                          const AlertHistory& newAlert);
};

// Comparison functions for sorting
bool compareByLoss(const CryptoPosition& a, const CryptoPosition& b);
bool compareByProfit(const CryptoPosition& a, const CryptoPosition& b);
bool compareBySymbol(const CryptoPosition& a, const CryptoPosition& b);

// ===== INLINE FUNCTIONS =====

inline int CryptoData::getCount(byte mode) const {
    return (mode == 0) ? countMode1 : countMode2;
}

inline PortfolioSummary* CryptoData::getSummary(byte mode) {
    return (mode == 0) ? &summaryMode1 : &summaryMode2;
}

inline const PortfolioSummary* CryptoData::getSummary(byte mode) const {
    return (mode == 0) ? &summaryMode1 : &summaryMode2;
}

inline AlertHistory* CryptoData::getAlertHistory(byte mode, int index) {
    if (mode == 0) {
        if (index >= 0 && index < alertHistoryCountMode1) {
            return &alertHistoryMode1[index];
        }
    } else {
        if (index >= 0 && index < alertHistoryCountMode2) {
            return &alertHistoryMode2[index];
        }
    }
    return nullptr;
}

inline int CryptoData::getAlertHistoryCount(byte mode) const {
    return (mode == 0) ? alertHistoryCountMode1 : alertHistoryCountMode2;
}

inline float CryptoData::getTotalInvestment(byte mode) const {
    const PortfolioSummary* s = getSummary(mode);
    return s->totalInvestment;
}

inline float CryptoData::getTotalCurrentValue(byte mode) const {
    const PortfolioSummary* s = getSummary(mode);
    return s->totalCurrentValue;
}

inline float CryptoData::getTotalPnL(byte mode) const {
    const PortfolioSummary* s = getSummary(mode);
    return s->totalPnl;
}

inline float CryptoData::getTotalPnLPercent(byte mode) const {
    const PortfolioSummary* s = getSummary(mode);
    return s->totalPnlPercent;
}

#endif // CRYPTO_DATA_H