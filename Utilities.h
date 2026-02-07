#ifndef UTILITIES_H
#define UTILITIES_H

#include <Arduino.h>
#include <vector>
#include <string>

class Utilities {
public:
    // String utilities
    static String trim(const String& str);
    static String toLowerCase(const String& str);
    static String toUpperCase(const String& str);
    static bool startsWith(const String& str, const String& prefix);
    static bool endsWith(const String& str, const String& suffix);
    static String replace(const String& str, const String& from, const String& to);
    
    // Number formatting
    static String formatFloat(float value, int decimals = 2);
    static String formatCurrency(float value, const String& symbol = "$");
    static String formatPercent(float value, bool includeSign = true);
    static String formatLargeNumber(float value);
    
    // Time utilities
    static String formatDuration(unsigned long milliseconds);
    static String formatTimestamp(unsigned long timestamp);
    static String getTimeAgo(unsigned long timestamp);
    
    // Data utilities
    static String bytesToHumanReadable(size_t bytes);
    static float mapFloat(float x, float in_min, float in_max, float out_min, float out_max);
    
    // Validation
    static bool isValidNumber(const String& str);
    static bool isValidFloat(const String& str);
    static bool isInRange(float value, float min, float max);
    
    // Array utilities
    template<typename T>
    static T clamp(T value, T min, T max) {
        if (value < min) return min;
        if (value > max) return max;
        return value;
    }
    
    template<typename T>
    static float average(const std::vector<T>& values) {
        if (values.empty()) return 0;
        T sum = 0;
        for (const auto& val : values) sum += val;
        return static_cast<float>(sum) / values.size();
    }
    
    // Crypto/Finance specific
    static String getShortSymbol(const String& symbol);
    static float calculatePNL(float entryPrice, float currentPrice, float quantity, bool isLong);
    static float calculatePNLPercent(float entryPrice, float currentPrice, bool isLong);
    static float calculateRiskRewardRatio(float entryPrice, float stopLoss, float takeProfit);
};

#endif