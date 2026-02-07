#ifndef DATA_PROCESSOR_H
#define DATA_PROCESSOR_H

#include <ArduinoJson.h>
#include <Arduino.h>
#include "SystemConfig.h"

// Forward declarations
class CryptoData;

class DataProcessor {
private:
    // JSON processing
    DynamicJsonDocument* jsonDoc;
    int jsonCapacity;
    
    // Parsing state
    bool parseError;
    String lastError;
    int parseCount;
    int errorCount;
    
    // Data validation
    struct ValidationRules {
        float minPrice;
        float maxPrice;
        float minQuantity;
        float maxQuantity;
        float minPercent;
        float maxPercent;
        
        ValidationRules() : minPrice(0.000001), maxPrice(1000000),
                           minQuantity(0.000001), maxQuantity(1000000),
                           minPercent(-100), maxPercent(1000) {}
    } rules;
    
    // Statistics
    struct ProcessingStats {
        int totalParsed;
        int successfulParses;
        int failedParses;
        int bytesProcessed;
        unsigned long totalTime;
        float averageTime;
        
        ProcessingStats() : totalParsed(0), successfulParses(0),
                           failedParses(0), bytesProcessed(0),
                           totalTime(0), averageTime(0) {}
    } stats;
    
    // Caching
    String lastProcessedData;
    unsigned long lastProcessTime;
    int cacheHits;
    int cacheMisses;
    
    // Transformation rules
    struct TransformRules {
        bool normalizeSymbols;
        bool convertToUSD;
        bool adjustPrecision;
        bool filterInvalid;
        
        TransformRules() : normalizeSymbols(true), convertToUSD(false),
                          adjustPrecision(true), filterInvalid(true) {}
    } transforms;
    
public:
    DataProcessor(int capacity = JSON_BUFFER_SIZE);
    ~DataProcessor();
    
    // ===== INITIALIZATION =====
    bool init(int capacity = JSON_BUFFER_SIZE);
    bool isInitialized() const;
    void setValidationRules(float minPrice, float maxPrice,
                           float minQuantity, float maxQuantity,
                           float minPercent, float maxPercent);
    void setTransforms(bool normalize, bool convert, bool adjust, bool filter);
    
    // ===== DATA PARSING =====
    bool parseData(const String& jsonData, byte mode);
    bool parsePortfolioData(const String& jsonData, byte mode);
    bool parsePositionData(const JsonObject& obj, CryptoPosition& position);
    bool parseSummaryData(const JsonObject& obj, PortfolioSummary& summary);
    
    // ===== DATA VALIDATION =====
    bool validateData(const String& jsonData);
    bool validatePosition(const JsonObject& obj);
    bool validateSummary(const JsonObject& obj);
    bool validateSymbol(const String& symbol);
    bool validatePrice(float price);
    bool validateQuantity(float quantity);
    bool validatePercent(float percent);
    
    // ===== DATA TRANSFORMATION =====
    String normalizeSymbol(const String& symbol);
    float convertCurrency(float amount, const String& from, const String& to = "USD");
    float adjustPrecision(float value, int decimals);
    String filterInvalidCharacters(const String& input);
    
    // ===== DATA PROCESSING =====
    bool processPortfolio(const JsonArray& portfolio, byte mode);
    int processPositions(const JsonArray& positions, byte mode, int maxPositions);
    bool updateCryptoData(CryptoData& data, byte mode);
    
    // ===== JSON OPERATIONS =====
    JsonDocument* parseJSON(const String& jsonData);
    String generateJSON(const CryptoData& data, byte mode, bool includeHistory = false);
    String generatePositionJSON(const CryptoPosition& position);
    String generateSummaryJSON(const PortfolioSummary& summary);
    String generateAlertJSON(const AlertHistory& alert);
    
    // ===== DATA EXTRACTION =====
    bool extractPortfolioData(const String& jsonData, JsonArray& portfolio, 
                             JsonObject& summary);
    bool extractPositionData(const JsonObject& obj, CryptoPosition& position);
    bool extractSummaryData(const JsonObject& obj, PortfolioSummary& summary);
    
    // ===== CACHING =====
    bool isCached(const String& jsonData) const;
    void cacheData(const String& jsonData, byte mode);
    void clearCache();
    int getCacheSize() const;
    
    // ===== ERROR HANDLING =====
    bool hasError() const;
    String getLastError() const;
    void clearError();
    int getErrorCount() const;
    
    // ===== STATISTICS =====
    ProcessingStats getStatistics() const;
    void resetStatistics();
    float getSuccessRate() const;
    float getAverageProcessingTime() const;
    
    // ===== UTILITY FUNCTIONS =====
    String formatForDisplay(float value, int type = 0); // 0=number, 1=percent, 2=price
    String truncateString(const String& str, int maxLength);
    String generateUniqueId(const String& base);
    
    // ===== DEBUG FUNCTIONS =====
    void printParsedData(byte mode) const;
    void printStatistics() const;
    void testParser(const String& testData);
    void benchmark(int iterations = 100);
    
    // ===== DATA CONVERSION =====
    static String base64Encode(const String& data);
    static String base64Decode(const String& data);
    static String urlEncode(const String& data);
    static String urlDecode(const String& data);
    static String hexEncode(const uint8_t* data, size_t length);
    static size_t hexDecode(const String& hex, uint8_t* output, size_t maxLength);
    
private:
    // Internal helper functions
    bool allocateJSONMemory();
    void freeJSONMemory();
    void updateStats(bool success, unsigned long time, int bytes);
    
    // Parsing helpers
    bool parseJSONInternal(const String& jsonData);
    bool validateJSONStructure(const JsonDocument& doc);
    bool processJSONData(const JsonDocument& doc, byte mode);
    
    // Data processing helpers
    bool processSinglePosition(const JsonObject& obj, CryptoPosition& position);
    void calculateMissingFields(CryptoPosition& position);
    void applyTransforms(CryptoPosition& position);
    
    // Validation helpers
    bool validateJSON(const String& jsonData);
    bool validateField(const JsonObject& obj, const char* field, 
                      JsonVariantType type);
    bool validateNumericRange(float value, float min, float max);
    
    // Transformation helpers
    void normalizePositionData(CryptoPosition& position);
    void convertPositionCurrency(CryptoPosition& position);
    void adjustPositionPrecision(CryptoPosition& position);
    
    // Cache management
    String generateCacheKey(const String& jsonData, byte mode);
    bool checkCache(const String& key);
    void updateCache(const String& key, const String& data);
    
    // Error handling
    void setError(const String& error);
    void logError(const String& context, const String& error);
    
    // Debug
    void dumpJSON(const JsonDocument& doc);
    void logProcessing(const String& operation, bool success);
};

// Inline functions
inline bool DataProcessor::isInitialized() const {
    return jsonDoc != nullptr;
}

inline bool DataProcessor::hasError() const {
    return parseError;
}

inline String DataProcessor::getLastError() const {
    return lastError;
}

inline int DataProcessor::getErrorCount() const {
    return errorCount;
}

inline DataProcessor::ProcessingStats DataProcessor::getStatistics() const {
    return stats;
}

inline float DataProcessor::getSuccessRate() const {
    if (stats.totalParsed == 0) return 0.0;
    return (stats.successfulParses * 100.0) / stats.totalParsed;
}

inline float DataProcessor::getAverageProcessingTime() const {
    if (stats.totalParsed == 0) return 0.0;
    return stats.totalTime / (float)stats.totalParsed;
}

inline int DataProcessor::getCacheSize() const {
    return cacheHits + cacheMisses;
}

inline void DataProcessor::clearError() {
    parseError = false;
    lastError = "";
}

// JSON field names
namespace JSONFields {
    const char* PORTFOLIO = "portfolio";
    const char* SUMMARY = "summary";
    const char* SYMBOL = "symbol";
    const char* QUANTITY = "quantity";
    const char* ENTRY_PRICE = "entry_price";
    const char* CURRENT_PRICE = "current_price";
    const char* PNL = "pnl";
    const char* PNL_PERCENT = "pnl_percent";
    const char* SIDE = "side";
    const char* POSITION_SIDE = "position_side";
    const char* MARGIN_TYPE = "margin_type";
    const char* TOTAL_INVESTMENT = "total_investment";
    const char* TOTAL_CURRENT_VALUE = "total_current_value";
    const char* TOTAL_PNL = "total_pnl";
    const char* TOTAL_POSITIONS = "total_positions";
    const char* LONG_POSITIONS = "long_positions";
    const char* SHORT_POSITIONS = "short_positions";
    const char* WINNING_POSITIONS = "winning_positions";
    const char* LOSING_POSITIONS = "losing_positions";
    const char* MAX_DRAWDOWN = "max_drawdown";
    const char* SHARPE_RATIO = "sharpe_ratio";
}

#endif // DATA_PROCESSOR_H