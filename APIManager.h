#ifndef API_MANAGER_H
#define API_MANAGER_H

#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "SystemConfig.h"

// Forward declarations
struct SystemSettings;

class APIManager {
private:
    HTTPClient http;
    WiFiClientSecure client;
    
    // Configuration
    SystemSettings* settings;
    String apiServer;
    String apiUsername;
    String apiPassword;
    String entryPortfolio;
    String exitPortfolio;
    
    // Connection settings
    int timeout;
    int retryCount;
    bool useHTTPS;
    bool verifySSL;
    
    // Request state
    bool requestInProgress;
    unsigned long requestStartTime;
    String currentEndpoint;
    
    // Statistics
    struct APIStatistics {
        int totalRequests;
        int successfulRequests;
        int failedRequests;
        int timeoutErrors;
        int connectionErrors;
        int httpErrors;
        unsigned long totalResponseTime;
        float averageResponseTime;
        size_t totalBytesReceived;
        
        APIStatistics() : totalRequests(0), successfulRequests(0),
                         failedRequests(0), timeoutErrors(0),
                         connectionErrors(0), httpErrors(0),
                         totalResponseTime(0), averageResponseTime(0),
                         totalBytesReceived(0) {}
    } stats;
    
    // Cache management
    struct APICache {
        String data;
        unsigned long timestamp;
        unsigned long ttl; // Time to live in ms
        
        APICache() : timestamp(0), ttl(30000) {} // 30 seconds default
    } cacheEntryMode1, cacheEntryMode2;
    
    // Error handling
    int lastErrorCode;
    String lastErrorMessage;
    
public:
    APIManager();
    ~APIManager();
    
    // ===== INITIALIZATION =====
    void init(const SystemSettings& settings);
    bool isInitialized() const;
    void setServer(const String& server);
    void setCredentials(const String& username, const String& password);
    void setPortfolios(const String& entry, const String& exit);
    
    // ===== REQUEST CONFIGURATION =====
    void setTimeout(int milliseconds);
    void setRetryCount(int count);
    void setUseHTTPS(bool use);
    void setVerifySSL(bool verify);
    
    // ===== API REQUESTS =====
    String fetchPortfolioData(byte mode);
    String fetchPortfolioData(const String& portfolioName);
    String fetchMarketData(const String& symbol);
    String fetchMultipleSymbols(const std::vector<String>& symbols);
    String fetchHistoricalData(const String& symbol, const String& interval, 
                              int limit = 100);
    
    // ===== REQUEST METHODS =====
    String GET(const String& endpoint, const std::vector<String>& headers = {});
    String POST(const String& endpoint, const String& body, 
               const std::vector<String>& headers = {});
    String PUT(const String& endpoint, const String& body,
              const std::vector<String>& headers = {});
    String DELETE(const String& endpoint, const std::vector<String>& headers = {});
    
    // ===== CACHE MANAGEMENT =====
    void setCacheTTL(unsigned long ttl, byte mode = 0);
    unsigned long getCacheTTL(byte mode) const;
    bool isCacheValid(byte mode) const;
    String getCachedData(byte mode) const;
    void updateCache(byte mode, const String& data);
    void clearCache(byte mode = 255); // 255 = clear all
    
    // ===== AUTHENTICATION =====
    String generateAuthHeader();
    String generateBearerToken();
    bool validateToken(const String& token);
    bool refreshToken();
    
    // ===== ERROR HANDLING =====
    int getLastErrorCode() const;
    String getLastErrorMessage() const;
    String getErrorString(int code) const;
    void clearError();
    
    // ===== STATISTICS =====
    APIStatistics getStatistics() const;
    void resetStatistics();
    float getSuccessRate() const;
    float getAverageResponseTime() const;
    size_t getTotalBytesReceived() const;
    
    // ===== REQUEST MONITORING =====
    bool isRequestInProgress() const;
    unsigned long getRequestDuration() const;
    String getCurrentEndpoint() const;
    
    // ===== UTILITY FUNCTIONS =====
    String buildURL(const String& endpoint);
    String buildPortfolioURL(const String& portfolioName);
    std::vector<String> getDefaultHeaders();
    
    String encodeURLParameters(const std::map<String, String>& params);
    String generateRequestId();
    
    // ===== DEBUG FUNCTIONS =====
    void printStatistics() const;
    void testConnection();
    void benchmark(int requests = 10);
    
    // ===== WEBHOOK SUPPORT =====
    bool sendWebhook(const String& url, const String& payload);
    bool sendAlertWebhook(const String& symbol, float pnlPercent, 
                         const String& message);
    bool sendStatusWebhook(const String& status);
    
private:
    // Internal helper functions
    bool initializeClient();
    void updateStats(bool success, unsigned long responseTime, size_t bytes);
    
    // Request execution
    String executeRequest(const String& method, const String& url, 
                         const String& body = "", 
                         const std::vector<String>& headers = {});
    bool prepareRequest(const String& url);
    bool addHeaders(const std::vector<String>& headers);
    
    // Response handling
    String handleResponse(int httpCode);
    bool validateResponse(const String& response);
    bool handleErrorResponse(int httpCode);
    
    // Cache operations
    APICache* getCache(byte mode);
    const APICache* getCache(byte mode) const;
    bool shouldUseCache(byte mode) const;
    
    // Authentication
    String getAuthToken();
    bool isTokenExpired() const;
    
    // SSL/TLS
    bool configureSSL();
    void setRootCA();
    
    // Error handling
    void setError(int code, const String& message = "");
    void logError(const String& context, int code, const String& message);
    
    // Debug logging
    void logRequest(const String& method, const String& url, bool success);
    void logResponse(int code, unsigned long duration, size_t size);
};

// Inline functions
inline bool APIManager::isInitialized() const {
    return settings != nullptr && apiServer.length() > 0;
}

inline bool APIManager::isRequestInProgress() const {
    return requestInProgress;
}

inline String APIManager::getCurrentEndpoint() const {
    return currentEndpoint;
}

inline unsigned long APIManager::getRequestDuration() const {
    if (!requestInProgress) return 0;
    return millis() - requestStartTime;
}

inline int APIManager::getLastErrorCode() const {
    return lastErrorCode;
}

inline String APIManager::getLastErrorMessage() const {
    return lastErrorMessage;
}

inline APIManager::APIStatistics APIManager::getStatistics() const {
    return stats;
}

inline float APIManager::getSuccessRate() const {
    if (stats.totalRequests == 0) return 0.0;
    return (stats.successfulRequests * 100.0) / stats.totalRequests;
}

inline float APIManager::getAverageResponseTime() const {
    if (stats.totalRequests == 0) return 0.0;
    return stats.averageResponseTime;
}

inline size_t APIManager::getTotalBytesReceived() const {
    return stats.totalBytesReceived;
}

inline unsigned long APIManager::getCacheTTL(byte mode) const {
    const APICache* cache = getCache(mode);
    return cache ? cache->ttl : 0;
}

inline bool APIManager::isCacheValid(byte mode) const {
    const APICache* cache = getCache(mode);
    if (!cache || cache->data.length() == 0) return false;
    
    unsigned long age = millis() - cache->timestamp;
    return age < cache->ttl;
}

inline String APIManager::getCachedData(byte mode) const {
    const APICache* cache = getCache(mode);
    return cache ? cache->data : "";
}

// Default configuration
namespace APIConfig {
    const int DEFAULT_TIMEOUT = 10000; // 10 seconds
    const int DEFAULT_RETRY_COUNT = 3;
    const unsigned long DEFAULT_CACHE_TTL = 30000; // 30 seconds
    const bool DEFAULT_USE_HTTPS = true;
    const bool DEFAULT_VERIFY_SSL = true;
}

#endif // API_MANAGER_H