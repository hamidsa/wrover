#include "DataProcessor.h"
#include "CryptoData.h"
#include <ArduinoJson.h>
#include <algorithm>

DataProcessor::DataProcessor(int capacity) : 
    jsonDoc(nullptr),
    jsonCapacity(capacity),
    parseError(false),
    parseCount(0),
    errorCount(0),
    lastProcessTime(0),
    cacheHits(0),
    cacheMisses(0) {
    
    // Initialize JSON document
    jsonDoc = new DynamicJsonDocument(jsonCapacity);
}

DataProcessor::~DataProcessor() {
    if (jsonDoc) {
        delete jsonDoc;
        jsonDoc = nullptr;
    }
}

bool DataProcessor::init(int capacity) {
    if (jsonDoc) {
        delete jsonDoc;
    }
    
    jsonCapacity = capacity;
    jsonDoc = new DynamicJsonDocument(jsonCapacity);
    
    if (!jsonDoc) {
        setError("Failed to allocate JSON memory");
        return false;
    }
    
    parseError = false;
    lastError = "";
    
    Serial.println("Data Processor initialized");
    Serial.println("  JSON Capacity: " + String(jsonCapacity) + " bytes");
    Serial.println("  Validation: " + String(transforms.filterInvalid ? "Enabled" : "Disabled"));
    
    return true;
}

bool DataProcessor::parseData(const String& jsonData, byte mode) {
    if (!jsonDoc || jsonData.length() == 0) {
        setError("Invalid input data");
        return false;
    }
    
    // Check cache first
    String cacheKey = generateCacheKey(jsonData, mode);
    if (checkCache(cacheKey)) {
        cacheHits++;
        return true;
    }
    cacheMisses++;
    
    unsigned long startTime = millis();
    
    // Clear previous parse state
    parseError = false;
    lastError = "";
    
    // Parse JSON
    DeserializationError error = deserializeJson(*jsonDoc, jsonData);
    if (error) {
        setError("JSON parse error: " + String(error.c_str()));
        updateStats(false, millis() - startTime, jsonData.length());
        return false;
    }
    
    // Validate JSON structure
    if (!validateJSONStructure(*jsonDoc)) {
        setError("Invalid JSON structure");
        updateStats(false, millis() - startTime, jsonData.length());
        return false;
    }
    
    // Process the data
    bool success = processJSONData(*jsonDoc, mode);
    
    // Update cache if successful
    if (success) {
        updateCache(cacheKey, jsonData);
    }
    
    updateStats(success, millis() - startTime, jsonData.length());
    return success;
}

bool DataProcessor::parsePortfolioData(const String& jsonData, byte mode) {
    return parseData(jsonData, mode);
}

bool DataProcessor::parsePositionData(const JsonObject& obj, CryptoPosition& position) {
    if (obj.isNull()) {
        return false;
    }
    
    // Extract required fields
    const char* symbol = obj[JSONFields::SYMBOL] | "UNKNOWN";
    float quantity = obj[JSONFields::QUANTITY] | 0.0;
    float entryPrice = obj[JSONFields::ENTRY_PRICE] | 0.0;
    float currentPrice = obj[JSONFields::CURRENT_PRICE] | 0.0;
    float pnlValue = obj[JSONFields::PNL] | 0.0;
    float pnlPercent = obj[JSONFields::PNL_PERCENT] | 0.0;
    
    // Determine position side
    bool isLong = true;
    if (obj.containsKey(JSONFields::SIDE)) {
        const char* side = obj[JSONFields::SIDE];
        if (strcasecmp(side, "sell") == 0 || strcasecmp(side, "short") == 0) {
            isLong = false;
        }
    } else if (obj.containsKey(JSONFields::POSITION_SIDE)) {
        const char* positionSide = obj[JSONFields::POSITION_SIDE];
        if (strcasecmp(positionSide, "short") == 0) {
            isLong = false;
        }
    }
    
    // Apply validation
    if (!validateSymbol(symbol) || 
        !validatePrice(currentPrice) || 
        !validateQuantity(quantity) ||
        !validatePercent(pnlPercent)) {
        return false;
    }
    
    // Initialize position
    strncpy(position.symbol, symbol, sizeof(position.symbol) - 1);
    position.symbol[sizeof(position.symbol) - 1] = '\0';
    
    position.quantity = quantity;
    position.entryPrice = entryPrice;
    position.currentPrice = currentPrice;
    position.pnlValue = pnlValue;
    position.changePercent = pnlPercent;
    position.isLong = isLong;
    
    // Extract additional fields if available
    if (obj.containsKey(JSONFields::POSITION_SIDE)) {
        const char* positionSide = obj[JSONFields::POSITION_SIDE];
        strncpy(position.positionSide, positionSide, sizeof(position.positionSide) - 1);
        position.positionSide[sizeof(position.positionSide) - 1] = '\0';
    }
    
    if (obj.containsKey(JSONFields::MARGIN_TYPE)) {
        const char* marginType = obj[JSONFields::MARGIN_TYPE];
        strncpy(position.marginType, marginType, sizeof(position.marginType) - 1);
        position.marginType[sizeof(position.marginType) - 1] = '\0';
    }
    
    // Apply transformations
    if (transforms.normalizeSymbols) {
        String normalized = normalizeSymbol(position.symbol);
        strncpy(position.symbol, normalized.c_str(), sizeof(position.symbol) - 1);
    }
    
    if (transforms.adjustPrecision) {
        position.currentPrice = adjustPrecision(position.currentPrice, 6);
        position.entryPrice = adjustPrecision(position.entryPrice, 6);
        position.quantity = adjustPrecision(position.quantity, 6);
        position.pnlValue = adjustPrecision(position.pnlValue, 2);
        position.changePercent = adjustPrecision(position.changePercent, 2);
    }
    
    // Calculate any missing fields
    calculateMissingFields(position);
    
    return true;
}

bool DataProcessor::parseSummaryData(const JsonObject& obj, PortfolioSummary& summary) {
    if (obj.isNull()) {
        return false;
    }
    
    // Extract summary fields
    summary.totalInvestment = obj[JSONFields::TOTAL_INVESTMENT] | 0.0;
    summary.totalCurrentValue = obj[JSONFields::TOTAL_CURRENT_VALUE] | 0.0;
    summary.totalPnl = obj[JSONFields::TOTAL_PNL] | 0.0;
    
    // Calculate PNL percentage
    if (summary.totalInvestment > 0) {
        summary.totalPnlPercent = ((summary.totalCurrentValue - summary.totalInvestment) / 
                                  summary.totalInvestment) * 100;
    } else {
        summary.totalPnlPercent = 0.0;
    }
    
    // Extract additional statistics
    summary.totalPositions = obj[JSONFields::TOTAL_POSITIONS] | 0;
    summary.longPositions = obj[JSONFields::LONG_POSITIONS] | 0;
    summary.shortPositions = obj[JSONFields::SHORT_POSITIONS] | 0;
    summary.winningPositions = obj[JSONFields::WINNING_POSITIONS] | 0;
    summary.losingPositions = obj[JSONFields::LOSING_POSITIONS] | 0;
    summary.maxDrawdown = obj[JSONFields::MAX_DRAWDOWN] | 0.0;
    summary.sharpeRatio = obj[JSONFields::SHARPE_RATIO] | 0.0;
    
    return true;
}

bool DataProcessor::validateData(const String& jsonData) {
    if (jsonData.length() == 0) {
        return false;
    }
    
    // Basic JSON validation
    DeserializationError error = deserializeJson(*jsonDoc, jsonData);
    if (error) {
        setError("JSON validation failed: " + String(error.c_str()));
        return false;
    }
    
    // Check for required structure
    if (!jsonDoc->containsKey(JSONFields::PORTFOLIO)) {
        setError("Missing 'portfolio' field");
        return false;
    }
    
    JsonArray portfolio = (*jsonDoc)[JSONFields::PORTFOLIO];
    if (!portfolio.is<JsonArray>()) {
        setError("'portfolio' is not an array");
        return false;
    }
    
    // Validate each position
    for (JsonObject position : portfolio) {
        if (!validatePosition(position)) {
            return false;
        }
    }
    
    // Validate summary if present
    if (jsonDoc->containsKey(JSONFields::SUMMARY)) {
        JsonObject summary = (*jsonDoc)[JSONFields::SUMMARY];
        if (!validateSummary(summary)) {
            return false;
        }
    }
    
    return true;
}

bool DataProcessor::validatePosition(const JsonObject& obj) {
    if (obj.isNull()) {
        return false;
    }
    
    // Check required fields
    if (!obj.containsKey(JSONFields::SYMBOL) ||
        !obj.containsKey(JSONFields::CURRENT_PRICE)) {
        return false;
    }
    
    // Validate field types
    if (!validateField(obj, JSONFields::SYMBOL, JsonVariantType::String) ||
        !validateField(obj, JSONFields::CURRENT_PRICE, JsonVariantType::Float) ||
        !validateField(obj, JSONFields::QUANTITY, JsonVariantType::Float) ||
        !validateField(obj, JSONFields::PNL_PERCENT, JsonVariantType::Float)) {
        return false;
    }
    
    // Validate values
    const char* symbol = obj[JSONFields::SYMBOL];
    float price = obj[JSONFields::CURRENT_PRICE];
    float quantity = obj.containsKey(JSONFields::QUANTITY) ? 
                    obj[JSONFields::QUANTITY] : 0.0;
    float percent = obj.containsKey(JSONFields::PNL_PERCENT) ? 
                   obj[JSONFields::PNL_PERCENT] : 0.0;
    
    return validateSymbol(symbol) &&
           validatePrice(price) &&
           validateQuantity(quantity) &&
           validatePercent(percent);
}

bool DataProcessor::validateSymbol(const String& symbol) {
    if (symbol.length() == 0 || symbol.length() > 15) {
        return false;
    }
    
    // Check for invalid characters
    for (unsigned int i = 0; i < symbol.length(); i++) {
        char c = symbol.charAt(i);
        if (!isalnum(c) && c != '_' && c != '-') {
            return false;
        }
    }
    
    return true;
}

bool DataProcessor::validatePrice(float price) {
    return validateNumericRange(price, rules.minPrice, rules.maxPrice);
}

bool DataProcessor::validateQuantity(float quantity) {
    return validateNumericRange(quantity, rules.minQuantity, rules.maxQuantity);
}

bool DataProcessor::validatePercent(float percent) {
    return validateNumericRange(percent, rules.minPercent, rules.maxPercent);
}

String DataProcessor::normalizeSymbol(const String& symbol) {
    String normalized = symbol;
    
    // Remove common suffixes
    if (normalized.endsWith("USDT")) {
        normalized = normalized.substring(0, normalized.length() - 4);
    } else if (normalized.endsWith("_USDT")) {
        normalized = normalized.substring(0, normalized.length() - 5);
    } else if (normalized.endsWith("PERP")) {
        normalized = normalized.substring(0, normalized.length() - 4);
    }
    
    // Convert to uppercase
    normalized.toUpperCase();
    
    return normalized;
}

float DataProcessor::adjustPrecision(float value, int decimals) {
    if (value == 0.0) return 0.0;
    
    float multiplier = pow(10.0, decimals);
    return round(value * multiplier) / multiplier;
}

String DataProcessor::filterInvalidCharacters(const String& input) {
    String filtered;
    
    for (unsigned int i = 0; i < input.length(); i++) {
        char c = input.charAt(i);
        if (isprint(c) && c != '<' && c != '>' && c != '&' && c != '"' && c != '\'') {
            filtered += c;
        }
    }
    
    return filtered;
}

bool DataProcessor::processPortfolio(const JsonArray& portfolio, byte mode, int maxPositions) {
    if (portfolio.isNull()) {
        return false;
    }
    
    int processed = 0;
    
    for (JsonObject positionObj : portfolio) {
        if (processed >= maxPositions) {
            break;
        }
        
        CryptoPosition position;
        if (parsePositionData(positionObj, position)) {
            // Here you would add the position to your data structure
            // For example: cryptoData->addPosition(mode, position);
            processed++;
        }
    }
    
    return processed > 0;
}

String DataProcessor::generateJSON(const CryptoData& data, byte mode, bool includeHistory) {
    DynamicJsonDocument doc(jsonCapacity);
    
    // Create portfolio array
    JsonArray portfolio = doc.createNestedArray(JSONFields::PORTFOLIO);
    
    int positionCount = data.getCount(mode);
    for (int i = 0; i < positionCount; i++) {
        CryptoPosition* position = data.getPosition(mode, i);
        if (position) {
            JsonObject posObj = portfolio.createNestedObject();
            posObj[JSONFields::SYMBOL] = position->symbol;
            posObj[JSONFields::QUANTITY] = position->quantity;
            posObj[JSONFields::ENTRY_PRICE] = position->entryPrice;
            posObj[JSONFields::CURRENT_PRICE] = position->currentPrice;
            posObj[JSONFields::PNL] = position->pnlValue;
            posObj[JSONFields::PNL_PERCENT] = position->changePercent;
            posObj["side"] = position->isLong ? "long" : "short";
        }
    }
    
    // Add summary
    const PortfolioSummary* summary = data.getSummary(mode);
    if (summary) {
        JsonObject summaryObj = doc.createNestedObject(JSONFields::SUMMARY);
        summaryObj[JSONFields::TOTAL_INVESTMENT] = summary->totalInvestment;
        summaryObj[JSONFields::TOTAL_CURRENT_VALUE] = summary->totalCurrentValue;
        summaryObj[JSONFields::TOTAL_PNL] = summary->totalPnl;
        summaryObj[JSONFields::TOTAL_PNL_PERCENT] = summary->totalPnlPercent;
        summaryObj[JSONFields::TOTAL_POSITIONS] = summary->totalPositions;
        summaryObj[JSONFields::LONG_POSITIONS] = summary->longPositions;
        summaryObj[JSONFields::SHORT_POSITIONS] = summary->shortPositions;
        summaryObj[JSONFields::WINNING_POSITIONS] = summary->winningPositions;
        summaryObj[JSONFields::LOSING_POSITIONS] = summary->losingPositions;
    }
    
    // Add alert history if requested
    if (includeHistory) {
        JsonArray history = doc.createNestedArray("alert_history");
        int alertCount = data.getAlertHistoryCount(mode);
        
        for (int i = 0; i < alertCount; i++) {
            AlertHistory* alert = data.getAlertHistory(mode, i);
            if (alert) {
                JsonObject alertObj = history.createNestedObject();
                alertObj["symbol"] = alert->symbol;
                alertObj["time"] = alert->timeString;
                alertObj["pnl_percent"] = alert->pnlPercent;
                alertObj["price"] = alert->alertPrice;
                alertObj["type"] = alert->alertType;
                alertObj["severity"] = alert->isSevere ? "severe" : "normal";
            }
        }
    }
    
    String output;
    serializeJson(doc, output);
    return output;
}

String DataProcessor::formatForDisplay(float value, int type) {
    switch (type) {
        case 0: // Number
            return formatNumber(value, 2);
        case 1: // Percent
            return formatPercent(value);
        case 2: // Price
            return formatPrice(value);
        default:
            return String(value, 2);
    }
}

String DataProcessor::formatNumber(float number, int decimals) {
    if (number == 0) return "0";
    
    float absNumber = fabs(number);
    
    if (absNumber >= 1000000) {
        return String(number / 1000000, decimals) + "M";
    } else if (absNumber >= 10000) {
        return String(number / 1000, 1) + "K";
    } else if (absNumber >= 1000) {
        return String(number / 1000, 2) + "K";
    } else if (absNumber >= 1) {
        return String(number, decimals);
    } else if (absNumber >= 0.01) {
        return String(number, 4);
    } else if (absNumber >= 0.0001) {
        return String(number, 6);
    } else {
        return String(number, 8);
    }
}

String DataProcessor::formatPercent(float percent) {
    if (percent > 0) {
        return "+" + String(percent, 2) + "%";
    } else if (percent < 0) {
        return String(percent, 2) + "%";
    } else {
        return "0.00%";
    }
}

String DataProcessor::formatPrice(float price) {
    if (price <= 0) return "0.00";
    
    if (price >= 1000) {
        return String(price, 2);
    } else if (price >= 1) {
        return String(price, 4);
    } else if (price >= 0.01) {
        return String(price, 6);
    } else if (price >= 0.0001) {
        return String(price, 8);
    } else {
        return String(price, 10);
    }
}

// Static utility functions
String DataProcessor::base64Encode(const String& data) {
    const char* base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    String encoded;
    int i = 0;
    int len = data.length();
    
    while (i < len) {
        uint32_t octet_a = i < len ? (unsigned char)data[i++] : 0;
        uint32_t octet_b = i < len ? (unsigned char)data[i++] : 0;
        uint32_t octet_c = i < len ? (unsigned char)data[i++] : 0;
        
        uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;
        
        encoded += base64_chars[(triple >> 3 * 6) & 0x3F];
        encoded += base64_chars[(triple >> 2 * 6) & 0x3F];
        encoded += base64_chars[(triple >> 1 * 6) & 0x3F];
        encoded += base64_chars[(triple >> 0 * 6) & 0x3F];
    }
    
    if (len % 3 == 1) {
        encoded[encoded.length() - 1] = '=';
        encoded[encoded.length() - 2] = '=';
    } else if (len % 3 == 2) {
        encoded[encoded.length() - 1] = '=';
    }
    
    return encoded;
}

String DataProcessor::urlEncode(const String& str) {
    String encoded;
    char c;
    char hex[4];
    
    for (unsigned int i = 0; i < str.length(); i++) {
        c = str.charAt(i);
        
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            encoded += c;
        } else if (c == ' ') {
            encoded += '+';
        } else {
            sprintf(hex, "%%%02X", (unsigned char)c);
            encoded += hex;
        }
    }
    return encoded;
}

String DataProcessor::urlDecode(const String& str) {
    String decoded;
    char temp[] = "0x00";
    unsigned int len = str.length();
    
    for (unsigned int i = 0; i < len; i++) {
        char ch = str.charAt(i);
        
        if (ch == '%') {
            if (i + 2 < len) {
                temp[2] = str.charAt(i + 1);
                temp[3] = str.charAt(i + 2);
                decoded += (char)strtol(temp, NULL, 16);
                i += 2;
            }
        } else if (ch == '+') {
            decoded += ' ';
        } else {
            decoded += ch;
        }
    }
    
    return decoded;
}

// Private helper functions
bool DataProcessor::validateJSONStructure(const JsonDocument& doc) {
    return doc.containsKey(JSONFields::PORTFOLIO);
}

bool DataProcessor::validateField(const JsonObject& obj, const char* field, JsonVariantType type) {
    if (!obj.containsKey(field)) {
        return true; // Optional field
    }
    
    JsonVariant value = obj[field];
    
    switch (type) {
        case JsonVariantType::String:
            return value.is<const char*>();
        case JsonVariantType::Float:
            return value.is<float>() || value.is<int>();
        case JsonVariantType::Integer:
            return value.is<int>();
        case JsonVariantType::Boolean:
            return value.is<bool>();
        default:
            return true;
    }
}

bool DataProcessor::validateNumericRange(float value, float min, float max) {
    return !isnan(value) && value >= min && value <= max;
}

void DataProcessor::calculateMissingFields(CryptoPosition& position) {
    // Calculate PNL if missing
    if (position.pnlValue == 0.0 && position.quantity > 0 && position.entryPrice > 0) {
        if (position.isLong) {
            position.pnlValue = (position.currentPrice - position.entryPrice) * position.quantity;
        } else {
            position.pnlValue = (position.entryPrice - position.currentPrice) * position.quantity;
        }
    }
    
    // Calculate percentage if missing
    if (position.changePercent == 0.0 && position.entryPrice > 0) {
        if (position.isLong) {
            position.changePercent = ((position.currentPrice - position.entryPrice) / 
                                     position.entryPrice) * 100;
        } else {
            position.changePercent = ((position.entryPrice - position.currentPrice) / 
                                     position.entryPrice) * 100;
        }
    }
}

void DataProcessor::updateStats(bool success, unsigned long time, int bytes) {
    stats.totalParsed++;
    stats.bytesProcessed += bytes;
    stats.totalTime += time;
    
    if (success) {
        stats.successfulParses++;
    } else {
        stats.failedParses++;
    }
    
    // Update average
    stats.averageTime = (stats.averageTime * 0.9) + (time * 0.1);
}

String DataProcessor::generateCacheKey(const String& jsonData, byte mode) {
    // Simple hash of the data
    uint32_t hash = 0;
    for (unsigned int i = 0; i < jsonData.length(); i++) {
        hash = (hash * 31) + jsonData.charAt(i);
    }
    
    return String(hash, HEX) + "_" + String(mode);
}

bool DataProcessor::checkCache(const String& key) {
    return lastProcessedData == key && 
           (millis() - lastProcessTime) < 30000; // 30 second cache
}

void DataProcessor::updateCache(const String& key, const String& data) {
    lastProcessedData = key;
    lastProcessTime = millis();
}

void DataProcessor::setError(const String& error) {
    parseError = true;
    lastError = error;
    errorCount++;
    
    Serial.println("Data Processor Error: " + error);
}

void DataProcessor::printStatistics() const {
    Serial.println("\n=== DATA PROCESSOR STATISTICS ===");
    Serial.println("Total Parsed: " + String(stats.totalParsed));
    Serial.println("Successful: " + String(stats.successfulParses));
    Serial.println("Failed: " + String(stats.failedParses));
    Serial.println("Success Rate: " + String(getSuccessRate(), 1) + "%");
    Serial.println("Bytes Processed: " + String(stats.bytesProcessed));
    Serial.println("Average Time: " + String(stats.averageTime, 0) + "ms");
    Serial.println("Cache Hits: " + String(cacheHits));
    Serial.println("Cache Misses: " + String(cacheMisses));
    Serial.println("Cache Ratio: " + 
                  String(cacheHits * 100.0 / (cacheHits + cacheMisses), 1) + "%");
}