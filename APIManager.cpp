#include "APIManager.h"
#include "ConfigManager.h"
#include "WiFiManager.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <base64.h>

// ===== STATIC VARIABLES =====
APIManager* APIManager::_instance = nullptr;
HTTPClient APIManager::_httpClient;

// ===== CONSTANTS =====
#define API_TIMEOUT 10000  // 10 seconds
#define MAX_RETRIES 3
#define API_CACHE_DURATION 30000  // 30 seconds cache

// ===== CONSTRUCTOR/DESTRUCTOR =====
APIManager::APIManager()
    : _initialized(false),
      _lastApiCallTime(0),
      _apiSuccessCount(0),
      _apiErrorCount(0),
      _totalResponseTime(0),
      _averageResponseTime(0),
      _cacheEnabled(true) {
}

APIManager::~APIManager() {
    if (_instance == this) {
        _instance = nullptr;
    }
}

// ===== INITIALIZATION =====
bool APIManager::begin() {
    Serial.println("Initializing API Manager...");
    
    // Load configuration
    _cacheEnabled = ConfigManager::getInstance().getBool("api_cache", true);
    
    _initialized = true;
    Serial.println("API Manager initialized");
    
    return true;
}

// ===== API CALL METHODS =====
bool APIManager::fetchPortfolioData(const String& portfolioName, bool isExitMode, 
                                   String& response, APIResponseInfo* responseInfo) {
    if (!WiFiManager::getInstance().isConnected()) {
        Serial.println("Cannot fetch data: WiFi not connected");
        if (responseInfo) {
            responseInfo->success = false;
            responseInfo->error = "WiFi not connected";
            responseInfo->httpCode = 0;
        }
        return false;
    }
    
    // Check cache first
    String cacheKey = portfolioName + (isExitMode ? "_exit" : "_entry");
    if (_cacheEnabled && isCacheValid(cacheKey)) {
        response = getCachedResponse(cacheKey);
        Serial.println("Using cached response for: " + portfolioName);
        if (responseInfo) {
            responseInfo->success = true;
            responseInfo->fromCache = true;
        }
        return true;
    }
    
    // Build URL
    String server = ConfigManager::getInstance().getAPIServer();
    String username = ConfigManager::getInstance().getAPIUsername();
    
    if (server.isEmpty() || username.isEmpty()) {
        Serial.println("API configuration incomplete");
        if (responseInfo) {
            responseInfo->success = false;
            responseInfo->error = "API configuration incomplete";
        }
        return false;
    }
    
    String url = server + "/api/device/portfolio/" + username + 
                "?portfolio_name=" + portfolioName;
    
    Serial.println("Fetching portfolio data from: " + url);
    
    // Make API call with retries
    for (int attempt = 0; attempt < MAX_RETRIES; attempt++) {
        if (attempt > 0) {
            Serial.print("Retry attempt ");
            Serial.println(attempt + 1);
            delay(1000 * attempt); // Exponential backoff
        }
        
        if (makeAPICall(url, response, responseInfo)) {
            // Cache the response
            if (_cacheEnabled) {
                cacheResponse(cacheKey, response);
            }
            return true;
        }
    }
    
    Serial.println("All API retries failed");
    return false;
}

bool APIManager::makeAPICall(const String& url, String& response, APIResponseInfo* responseInfo) {
    unsigned long startTime = millis();
    
    // Setup HTTP client
    _httpClient.begin(url);
    _httpClient.setTimeout(API_TIMEOUT);
    _httpClient.setReuse(true);
    
    // Add headers
    String authHeader = getAuthHeader();
    _httpClient.addHeader("Authorization", authHeader);
    _httpClient.addHeader("Content-Type", "application/json");
    _httpClient.addHeader("User-Agent", "PortfolioMonitor/4.5.3");
    
    // Make request
    int httpCode = _httpClient.GET();
    unsigned long responseTime = millis() - startTime;
    
    // Update statistics
    updateStatistics(httpCode == 200, responseTime);
    
    // Process response
    if (responseInfo) {
        responseInfo->httpCode = httpCode;
        responseInfo->responseTime = responseTime;
        responseInfo->fromCache = false;
    }
    
    if (httpCode == HTTP_CODE_OK) {
        response = _httpClient.getString();
        
        if (responseInfo) {
            responseInfo->success = true;
            responseInfo->payloadSize = response.length();
        }
        
        Serial.print("API call successful: ");
        Serial.print(response.length());
        Serial.println(" bytes received");
        
        _httpClient.end();
        return true;
    } else {
        String error = "HTTP Error: " + String(httpCode);
        if (httpCode <= 0) {
            error = "Connection failed: " + String(_httpClient.errorToString(httpCode));
        }
        
        if (responseInfo) {
            responseInfo->success = false;
            responseInfo->error = error;
        }
        
        Serial.println("API call failed: " + error);
        
        _httpClient.end();
        return false;
    }
}

// ===== AUTHENTICATION =====
String APIManager::getAuthHeader() {
    String username = ConfigManager::getInstance().getAPIUsername();
    String password = ConfigManager::getInstance().getAPIPassword();
    
    if (username.isEmpty() || password.isEmpty()) {
        return "";
    }
    
    String auth = username + ":" + password;
    String encoded = base64::encode(auth);
    
    return "Basic " + encoded;
}

bool APIManager::testConnection(String& errorMessage) {
    String server = ConfigManager::getInstance().getAPIServer();
    String username = ConfigManager::getInstance().getAPIUsername();
    
    if (server.isEmpty() || username.isEmpty()) {
        errorMessage = "API configuration incomplete";
        return false;
    }
    
    // Build test URL
    String url = server + "/api/device/test";
    
    String response;
    APIResponseInfo responseInfo;
    
    if (makeAPICall(url, response, &responseInfo)) {
        errorMessage = "Connection successful";
        return true;
    } else {
        errorMessage = responseInfo.error;
        return false;
    }
}

// ===== CACHE MANAGEMENT =====
void APIManager::cacheResponse(const String& key, const String& response) {
    APICacheEntry entry;
    entry.timestamp = millis();
    entry.response = response;
    
    _apiCache[key] = entry;
    
    // Limit cache size
    if (_apiCache.size() > 10) {
        // Remove oldest entry
        auto oldest = _apiCache.begin();
        for (auto it = _apiCache.begin(); it != _apiCache.end(); ++it) {
            if (it->second.timestamp < oldest->second.timestamp) {
                oldest = it;
            }
        }
        _apiCache.erase(oldest);
    }
}

String APIManager::getCachedResponse(const String& key) {
    auto it = _apiCache.find(key);
    if (it != _apiCache.end()) {
        return it->second.response;
    }
    return "";
}

bool APIManager::isCacheValid(const String& key) {
    auto it = _apiCache.find(key);
    if (it != _apiCache.end()) {
        return (millis() - it->second.timestamp) < API_CACHE_DURATION;
    }
    return false;
}

void APIManager::clearCache() {
    _apiCache.clear();
    Serial.println("API cache cleared");
}

void APIManager::setCacheDuration(uint32_t duration) {
    _cacheDuration = duration;
    ConfigManager::getInstance().setUInt("api_cache_duration", duration);
}

// ===== STATISTICS =====
void APIManager::updateStatistics(bool success, unsigned long responseTime) {
    _lastApiCallTime = millis();
    
    if (success) {
        _apiSuccessCount++;
    } else {
        _apiErrorCount++;
    }
    
    // Update average response time
    _totalResponseTime += responseTime;
    uint32_t totalCalls = _apiSuccessCount + _apiErrorCount;
    if (totalCalls > 0) {
        _averageResponseTime = _totalResponseTime / totalCalls;
    }
    
    // Save statistics periodically
    static unsigned long lastSaveTime = 0;
    if (millis() - lastSaveTime > 60000) { // Every minute
        saveStatistics();
        lastSaveTime = millis();
    }
}

void APIManager::saveStatistics() {
    // Save to preferences
    _prefs.begin("api_stats", false);
    _prefs.putULong("success_count", _apiSuccessCount);
    _prefs.putULong("error_count", _apiErrorCount);
    _prefs.putULong("total_response_time", _totalResponseTime);
    _prefs.putULong("last_call_time", _lastApiCallTime);
    _prefs.end();
}

void APIManager::loadStatistics() {
    _prefs.begin("api_stats", true);
    _apiSuccessCount = _prefs.getULong("success_count", 0);
    _apiErrorCount = _prefs.getULong("error_count", 0);
    _totalResponseTime = _prefs.getULong("total_response_time", 0);
    _lastApiCallTime = _prefs.getULong("last_call_time", 0);
    _prefs.end();
    
    // Calculate average
    uint32_t totalCalls = _apiSuccessCount + _apiErrorCount;
    if (totalCalls > 0) {
        _averageResponseTime = _totalResponseTime / totalCalls;
    }
}

void APIManager::resetStatistics() {
    _apiSuccessCount = 0;
    _apiErrorCount = 0;
    _totalResponseTime = 0;
    _averageResponseTime = 0;
    
    saveStatistics();
    Serial.println("API statistics reset");
}

// ===== WEB INTERFACE =====
String APIManager::getStatusJSON() {
    DynamicJsonDocument doc(512);
    
    doc["success_count"] = _apiSuccessCount;
    doc["error_count"] = _apiErrorCount;
    doc["success_rate"] = getSuccessRate();
    doc["average_response_time"] = _averageResponseTime;
    doc["last_call_time"] = _lastApiCallTime;
    doc["cache_enabled"] = _cacheEnabled;
    doc["cache_size"] = _apiCache.size();
    doc["cache_duration"] = _cacheDuration;
    
    // Configuration
    doc["config"]["server"] = ConfigManager::getInstance().getAPIServer();
    doc["config"]["username"] = ConfigManager::getInstance().getAPIUsername();
    doc["config"]["entry_portfolio"] = ConfigManager::getInstance().getEntryPortfolio();
    doc["config"]["exit_portfolio"] = ConfigManager::getInstance().getExitPortfolio();
    
    String json;
    serializeJson(doc, json);
    return json;
}

void APIManager::handleWebRequest(const String& action, const String& params) {
    if (action == "test") {
        String errorMessage;
        bool success = testConnection(errorMessage);
        
        DynamicJsonDocument doc(256);
        doc["success"] = success;
        doc["message"] = errorMessage;
        
        String json;
        serializeJson(doc, json);
        // Send response via web server
    }
    else if (action == "clear_cache") {
        clearCache();
    }
    else if (action == "reset_stats") {
        resetStatistics();
    }
    else if (action == "cache_duration") {
        setCacheDuration(params.toInt());
    }
    else if (action == "toggle_cache") {
        _cacheEnabled = !_cacheEnabled;
        ConfigManager::getInstance().setBool("api_cache", _cacheEnabled);
    }
}

// ===== UTILITY FUNCTIONS =====
float APIManager::getSuccessRate() {
    uint32_t totalCalls = _apiSuccessCount + _apiErrorCount;
    if (totalCalls == 0) return 0.0;
    
    return (_apiSuccessCount * 100.0) / totalCalls;
}

String APIManager::formatResponseTime(unsigned long ms) {
    if (ms < 1000) {
        return String(ms) + "ms";
    } else {
        return String(ms / 1000.0, 1) + "s";
    }
}

void APIManager::printStatistics() {
    Serial.println("\n=== API Statistics ===");
    Serial.print("Success Count: ");
    Serial.println(_apiSuccessCount);
    Serial.print("Error Count: ");
    Serial.println(_apiErrorCount);
    Serial.print("Success Rate: ");
    Serial.print(getSuccessRate(), 1);
    Serial.println("%");
    Serial.print("Average Response Time: ");
    Serial.print(formatResponseTime(_averageResponseTime));
    Serial.println();
    Serial.print("Last Call: ");
    if (_lastApiCallTime > 0) {
        unsigned long secondsAgo = (millis() - _lastApiCallTime) / 1000;
        Serial.print(secondsAgo);
        Serial.println(" seconds ago");
    } else {
        Serial.println("Never");
    }
    Serial.print("Cache Size: ");
    Serial.println(_apiCache.size());
    Serial.print("Cache Enabled: ");
    Serial.println(_cacheEnabled ? "Yes" : "No");
    Serial.println("====================\n");
}

// ===== BATCH OPERATIONS =====
bool APIManager::fetchAllData(String& entryResponse, String& exitResponse) {
    String entryPortfolio = ConfigManager::getInstance().getEntryPortfolio();
    String exitPortfolio = ConfigManager::getInstance().getExitPortfolio();
    
    bool entrySuccess = false;
    bool exitSuccess = false;
    
    // Fetch entry data
    if (!entryPortfolio.isEmpty()) {
        entrySuccess = fetchPortfolioData(entryPortfolio, false, entryResponse);
    }
    
    // Fetch exit data
    if (!exitPortfolio.isEmpty()) {
        exitSuccess = fetchPortfolioData(exitPortfolio, true, exitResponse);
    }
    
    return entrySuccess || exitSuccess;
}

// ===== ERROR HANDLING =====
String APIManager::getErrorMessage(int httpCode) {
    switch (httpCode) {
        case 200: return "Success";
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 429: return "Too Many Requests";
        case 500: return "Internal Server Error";
        case 502: return "Bad Gateway";
        case 503: return "Service Unavailable";
        case 504: return "Gateway Timeout";
        case -1: return "Connection Failed";
        case -2: return "Send Header Failed";
        case -3: return "Send Payload Failed";
        case -4: return "Not Connected";
        case -5: return "Connection Lost";
        case -6: return "No Stream";
        case -7: return "No HTTP Server";
        case -8: return "Too Less RAM";
        case -9: return "Encoding Failed";
        case -10: return "Stream Write Failed";
        case -11: return "Read Timeout";
        default: return "Unknown Error: " + String(httpCode);
    }
}

// ===== GETTERS =====
uint32_t APIManager::getSuccessCount() const { return _apiSuccessCount; }
uint32_t APIManager::getErrorCount() const { return _apiErrorCount; }
uint32_t APIManager::getAverageResponseTime() const { return _averageResponseTime; }
bool APIManager::isCacheEnabled() const { return _cacheEnabled; }
size_t APIManager::getCacheSize() const { return _apiCache.size(); }
bool APIManager::isInitialized() const { return _initialized; }

// ===== STATIC ACCESS =====
APIManager& APIManager::getInstance() {
    if (!_instance) {
        _instance = new APIManager();
    }
    return *_instance;
}