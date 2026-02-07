#ifndef FORMATTERS_H
#define FORMATTERS_H

#include <Arduino.h>

class Formatters {
public:
    // Number formatters
    static String formatDecimal(float value, int precision = 2);
    static String formatInteger(int value);
    static String formatLargeInteger(long value);
    static String formatScientific(float value, int precision = 2);
    
    // Financial formatters
    static String formatMoney(float amount, const String& currency = "USD");
    static String formatPercentage(float percent, bool includeSign = true);
    static String formatChange(float change, bool isPercent = false);
    
    // Crypto specific
    static String formatCryptoPrice(float price);
    static String formatCryptoAmount(float amount, const String& symbol);
    static String formatMarketCap(float cap);
    static String formatVolume(float volume);
    
    // Date/Time formatters
    static String formatDateTime(time_t timestamp, const String& format = "%Y-%m-%d %H:%M:%S");
    static String formatDate(time_t timestamp);
    static String formatTime(time_t timestamp);
    static String formatRelativeTime(time_t timestamp);
    
    // Data size formatters
    static String formatBytes(size_t bytes);
    static String formatKilobytes(size_t kilobytes);
    static String formatMegabytes(size_t megabytes);
    
    // Color formatters
    static String formatHexColor(uint32_t color);
    static String formatRGBColor(uint8_t r, uint8_t g, uint8_t b);
    
    // Specialized portfolio formatters
    static String formatPNL(float pnl, bool includeCurrency = true);
    static String formatPositionSize(float size);
    static String formatRisk(float risk);
    static String formatLeverage(float leverage);
    
    // UI formatters
    static String formatProgress(float progress, int width = 20);
    static String formatBarChart(const std::vector<float>& values, int height = 10);
    
private:
    static String padLeft(const String& str, int length, char padChar = ' ');
    static String padRight(const String& str, int length, char padChar = ' ');
    static String truncate(const String& str, int maxLength, const String& suffix = "...");
};

#endif