#ifndef WEB_INTERFACE_H
#define WEB_INTERFACE_H

#include <WebServer.h>
#include <ArduinoJson.h>
#include "SystemConfig.h"

// Forward declarations
class WiFiManager;
class DisplayManager;
class BuzzerManager;
class CryptoData;
struct SystemState;
struct SystemSettings;

class WebInterface {
private:
    WebServer server;
    
    // References to other managers
    SystemSettings* settings;
    WiFiManager* wifiMgr;
    DisplayManager* displayMgr;
    BuzzerManager* buzzerMgr;
    CryptoData* cryptoData;
    SystemState* systemState;
    
    // Server state
    bool initialized;
    unsigned long startTime;
    int clientCount;
    
    // Authentication
    bool authEnabled;
    String authUsername;
    String authPassword;
    
    // Rate limiting
    struct RateLimit {
        unsigned long lastRequest;
        int requestCount;
        unsigned long windowStart;
    };
    std::vector<RateLimit> rateLimits;
    
    // Statistics
    struct ServerStats {
        unsigned long totalRequests;
        unsigned long totalBytesSent;
        unsigned long totalBytesReceived;
        int successfulRequests;
        int failedRequests;
        int authFailures;
        
        ServerStats() : totalRequests(0), totalBytesSent(0),
                       totalBytesReceived(0), successfulRequests(0),
                       failedRequests(0), authFailures(0) {}
    } stats;
    
    // Session management
    struct ClientSession {
        String clientIP;
        unsigned long lastActivity;
        String sessionId;
        bool authenticated;
        
        ClientSession() : lastActivity(0), authenticated(false) {}
    };
    std::vector<ClientSession> sessions;
    
    // File system for SPIFFS
    bool spiffsMounted;
    
public:
    WebInterface();
    ~WebInterface();
    
    // ===== INITIALIZATION =====
    void init(SystemSettings& settings, WiFiManager& wifiMgr,
              DisplayManager& displayMgr, BuzzerManager& buzzerMgr,
              CryptoData& cryptoData, SystemState& systemState,
              int port = 80);
    bool isInitialized() const;
    void enableAuthentication(const String& username, const String& password);
    void disableAuthentication();
    
    // ===== SERVER CONTROL =====
    void start();
    void stop();
    void handleClient();
    void restart();
    
    // ===== REQUEST HANDLING =====
    void handleRoot();
    void handleDashboard();
    void handleSetup();
    void handleAPI();
    void handleSettings();
    void handleWiFi();
    void handleSystem();
    void handleDebug();
    void handleFiles();
    void handleUpload();
    void handleDownload();
    void handleNotFound();
    
    // ===== API ENDPOINTS =====
    void handleAPIGet(const String& endpoint);
    void handleAPIPost(const String& endpoint);
    void handleAPIPut(const String& endpoint);
    void handleAPIDelete(const String& endpoint);
    
    // ===== SETTINGS ENDPOINTS =====
    void handleSaveWiFi();
    void handleSaveAPI();
    void handleSaveAlerts();
    void handleSaveDisplay();
    void handleSaveSystem();
    void handleSaveLED();
    void handleSaveBuzzer();
    void handleSaveAll();
    
    // ===== ACTION ENDPOINTS =====
    void handleActionConnect();
    void handleActionDisconnect();
    void handleActionScan();
    void handleActionTest();
    void handleActionReset();
    void handleActionRestart();
    void handleActionFactoryReset();
    void handleActionUpdate();
    
    // ===== DATA ENDPOINTS =====
    void handleGetPositions();
    void handleGetPortfolio();
    void handleGetAlerts();
    void handleGetHistory();
    void handleGetStatistics();
    void handleGetSystemInfo();
    void handleGetNetworkInfo();
    
    // ===== CONTROL ENDPOINTS =====
    void handleControlBuzzer();
    void handleControlLED();
    void handleControlDisplay();
    void handleControlRGB();
    void handleControlVolume();
    void handleControlTest();
    
    // ===== FILE MANAGEMENT =====
    void handleFileList();
    void handleFileUpload();
    void handleFileDelete();
    void handleFileDownload();
    void handleSPIFFSInfo();
    
    // ===== WEBSOCKET SUPPORT =====
    void handleWebSocket();
    void broadcastData(const String& data);
    void sendToClient(int clientId, const String& data);
    
    // ===== SSE (SERVER SENT EVENTS) =====
    void handleSSE();
    void sendEvent(const String& event, const String& data);
    
    // ===== HTML GENERATION =====
    String generateDashboardHTML();
    String generateSetupHTML();
    String generateWiFiHTML();
    String generateAlertsHTML();
    String generateDisplayHTML();
    String generateSystemHTML();
    String generateAPIHTML();
    String generateAboutHTML();
    
    // ===== JSON RESPONSES =====
    String generateJSONResponse(bool success, const String& message = "", 
                               const JsonObject& data = JsonObject());
    String generateErrorJSON(int code, const String& message);
    String generateSuccessJSON(const String& message = "", 
                              const JsonObject& data = JsonObject());
    
    // ===== AUTHENTICATION =====
    bool checkAuthentication();
    bool checkRateLimit(const String& clientIP);
    String generateSessionId();
    bool validateSession(const String& sessionId);
    
    // ===== UTILITY FUNCTIONS =====
    String getContentType(const String& filename);
    bool handleFileRead(const String& path);
    void redirectTo(const String& url);
    
    // ===== STATISTICS =====
    ServerStats getStatistics() const;
    void resetStatistics();
    void printStatistics() const;
    
    // ===== DEBUG FUNCTIONS =====
    void printRoutes() const;
    void testAllEndpoints();
    void stressTest(int requests = 100);
    
private:
    // Internal helper functions
    void setupRoutes();
    void setupStaticFiles();
    void setupAPI();
    void setupWebSocket();
    void setupSSE();
    
    // Request processing
    bool preprocessRequest();
    void postprocessRequest();
    void logRequest(const String& method, const String& uri, 
                   int code, size_t bytes);
    
    // Authentication helpers
    bool basicAuth();
    bool sessionAuth();
    bool tokenAuth();
    String generateAuthToken();
    
    // Rate limiting
    void updateRateLimit(const String& clientIP);
    bool isRateLimited(const String& clientIP);
    void cleanupRateLimits();
    
    // Session management
    ClientSession* getSession(const String& clientIP);
    ClientSession* createSession(const String& clientIP);
    void cleanupSessions();
    void updateSessionActivity(ClientSession* session);
    
    // HTML generation helpers
    String generateNavigation();
    String generateHeader(const String& title);
    String generateFooter();
    String generateSidebar();
    String generateAlertBox(const String& type, const String& message);
    String generateModal(const String& id, const String& title, 
                        const String& content);
    
    // Form generation
    String generateWiFiForm();
    String generateAPIForm();
    String generateAlertForm();
    String generateDisplayForm();
    String generateLEDForm();
    String generateBuzzerForm();
    String generateSystemForm();
    
    // Data tables
    String generatePositionTable(byte mode);
    String generateAlertTable(byte mode);
    String generateNetworkTable();
    String generateFileTable();
    
    // JSON data generation
    JsonObject generateSystemJSON();
    JsonObject generatePortfolioJSON(byte mode);
    JsonObject generatePositionJSON(const CryptoPosition& position);
    JsonObject generateAlertJSON(const AlertHistory& alert);
    JsonObject generateNetworkJSON(const WiFiNetwork& network);
    JsonObject generateStatsJSON();
    
    // File system operations
    bool initSPIFFS();
    String listFiles();
    bool uploadFile(const String& filename, const uint8_t* data, size_t len);
    bool deleteFile(const String& filename);
    String readFile(const String& filename);
    
    // WebSocket management
    void handleWebSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, 
                             size_t length);
    void processWebSocketMessage(uint8_t num, const String& message);
    
    // SSE management
    void addSSEClient(WiFiClient client);
    void removeSSEClient(WiFiClient client);
    void sendToSSEClients(const String& data);
    
    // Security
    String sanitizeInput(const String& input);
    bool validateInput(const String& input, const String& pattern);
    String escapeHTML(const String& input);
    String escapeJSON(const String& input);
    
    // Response helpers
    void sendJSON(int code, const JsonDocument& doc);
    void sendHTML(int code, const String& html);
    void sendText(int code, const String& text);
    void sendFile(int code, const String& path, const String& contentType);
    
    // Error handling
    void handleError(int code, const String& message);
    void sendErrorPage(int code, const String& title, const String& message);
    
    // Debug logging
    void debugRequest(const String& method, const String& uri);
    void debugResponse(int code, size_t bytes);
};

// Inline functions
inline bool WebInterface::isInitialized() const {
    return initialized;
}

inline void WebInterface::handleClient() {
    if (initialized) {
        server.handleClient();
    }
}

inline void WebInterface::enableAuthentication(const String& username, 
                                              const String& password) {
    authEnabled = true;
    authUsername = username;
    authPassword = password;
}

inline void WebInterface::disableAuthentication() {
    authEnabled = false;
}

// HTML templates
namespace WebTemplates {
    const char* HEADER = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>%TITLE% | Portfolio Monitor</title>
    <link rel="stylesheet" href="/css/style.css">
    <script src="/js/main.js"></script>
</head>
<body>
    )rawliteral";
    
    const char* NAVIGATION = R"rawliteral(
<nav class="navbar">
    <div class="nav-brand">Portfolio Monitor v4.5</div>
    <div class="nav-links">
        <a href="/">Dashboard</a>
        <a href="/setup">Setup</a>
        <a href="/wifi">WiFi</a>
        <a href="/system">System</a>
        <a href="/api">API</a>
        <a href="/about">About</a>
    </div>
</nav>
    )rawliteral";
    
    const char* FOOTER = R"rawliteral(
<footer class="footer">
    <div class="footer-content">
        <p>ESP32 Portfolio Monitor &copy; 2024</p>
        <p>Uptime: %UPTIME% | Memory: %MEMORY% | Version: 4.5.3</p>
    </div>
</footer>
</body>
</html>
    )rawliteral";
}

#endif // WEB_INTERFACE_H