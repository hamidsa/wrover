#ifndef SYSTEM_CONFIG_H
#define SYSTEM_CONFIG_H

#include <Arduino.h>
#include <vector>

// ===== HARDWARE PIN DEFINITIONS =====
// RGB LEDs (Common Cathode)
#define RGB1_RED         32
#define RGB1_GREEN       33
#define RGB1_BLUE        25
#define RGB2_RED         26
#define RGB2_GREEN       14
#define RGB2_BLUE        12

// Single Color LEDs
#define LED_MODE1_GREEN  27  // Entry Mode LONG/PROFIT
#define LED_MODE1_RED    13  // Entry Mode SHORT/LOSS
#define LED_MODE2_GREEN  21  // Exit Mode PROFIT
#define LED_MODE2_RED    19  // Exit Mode LOSS

// Other Pins
#define BUZZER_PIN       22
#define RESET_BUTTON_PIN 0
#define TFT_BL_PIN       5
#define BATTERY_PIN      34

// ===== TIMING CONSTANTS =====
#define DATA_UPDATE_INTERVAL      15000      // 15 seconds
#define DISPLAY_UPDATE_INTERVAL   2000       // 2 seconds
#define ALERT_DISPLAY_TIME        10000      // 10 seconds
#define WIFI_CONNECT_TIMEOUT      20000      // 20 seconds
#define RECONNECT_INTERVAL        30000      // 30 seconds
#define BATTERY_CHECK_INTERVAL    60000      // 1 minute
#define SCAN_INTERVAL             60000      // 1 minute
#define DEBOUNCE_DELAY            50
#define BUTTON_HOLD_TIME          10000
#define ALERT_AUTO_RETURN_TIME    8000       // 8 seconds

// ===== ALERT THRESHOLDS =====
#define DEFAULT_ALERT_THRESHOLD      -5.0
#define DEFAULT_SEVERE_THRESHOLD    -10.0
#define PORTFOLIO_ALERT_THRESHOLD    -7.0
#define DEFAULT_EXIT_ALERT_PERCENT    3.0

// ===== BUZZER SETTINGS =====
#define DEFAULT_VOLUME              50
#define VOLUME_MIN                  0
#define VOLUME_MAX                  100
#define VOLUME_OFF                  0
#define DEFAULT_LED_BRIGHTNESS      100

// ===== TONE FREQUENCIES =====
#define LONG_NORMAL_TONE            523   // C5
#define LONG_SEVERE_TONE            440   // A4
#define SHORT_NORMAL_TONE           659   // E5
#define SHORT_SEVERE_TONE           784   // G5
#define PORTFOLIO_ALERT_TONE        587   // D5
#define RESET_TONE_1                262   // C4
#define RESET_TONE_2                294   // D4
#define RESET_TONE_3                330   // E4
#define SUCCESS_TONE_1              523   // C5
#define SUCCESS_TONE_2              659   // E5
#define ERROR_TONE_1                349   // F4
#define ERROR_TONE_2                294   // D4
#define CONNECTION_LOST_TONE        392   // G4

// ===== BATTERY SETTINGS =====
#define BATTERY_FULL                8.4
#define BATTERY_EMPTY               6.6
#define BATTERY_WARNING             20    // درصد هشدار

// ===== DISPLAY SETTINGS =====
#define DISPLAY_WIDTH               240
#define DISPLAY_HEIGHT              240
#define DISPLAY_CRYPTO_COUNT        8

// ===== MEMORY SETTINGS =====
#define MAX_POSITIONS_PER_MODE      100
#define MAX_ALERT_HISTORY           50
#define MAX_WIFI_NETWORKS           5
#define EEPROM_SIZE                 4096
#define JSON_BUFFER_SIZE            8192

// ===== NTP SETTINGS =====
const char* NTP_SERVER = "pool.ntp.org";
const long GMT_OFFSET = 12600;     // 3.5 hours for Iran
const int DAYLIGHT_OFFSET = 0;

// ===== ENUMERATIONS =====
enum PowerSource {
    POWER_SOURCE_USB,
    POWER_SOURCE_BATTERY,
    POWER_SOURCE_EXTERNAL
};

enum NetworkState {
    NET_OFFLINE,
    NET_CONNECTING,
    NET_ONLINE,
    NET_AP_MODE
};

enum AlertType {
    ALERT_NONE,
    ALERT_NORMAL,
    ALERT_SEVERE,
    ALERT_PROFIT,
    ALERT_LOSS,
    ALERT_PORTFOLIO,
    ALERT_EXIT_PROFIT,
    ALERT_EXIT_LOSS
};

enum DisplayMode {
    DISPLAY_MODE_MAIN,
    DISPLAY_MODE_ALERT,
    DISPLAY_MODE_CONNECTION,
    DISPLAY_MODE_ERROR,
    DISPLAY_MODE_SPLASH,
    DISPLAY_MODE_SETUP
};

enum WiFiConnectionResult {
    WIFI_CONNECT_SUCCESS,
    WIFI_CONNECT_FAILED,
    WIFI_CONNECT_TIMEOUT,
    WIFI_CONNECT_WRONG_PASSWORD,
    WIFI_CONNECT_NETWORK_NOT_FOUND
};

// ===== DATA STRUCTURES =====
struct WiFiNetwork {
    char ssid[32];
    char password[64];
    bool configured;
    unsigned long lastConnected;
    int connectionAttempts;
    byte priority;           // 1-10 (higher = higher priority)
    int rssi;                // Last signal strength
    bool autoConnect;        // Auto connect
    
    WiFiNetwork() {
        memset(ssid, 0, sizeof(ssid));
        memset(password, 0, sizeof(password));
        configured = false;
        lastConnected = 0;
        connectionAttempts = 0;
        priority = 5;
        rssi = 0;
        autoConnect = true;
    }
    
    WiFiNetwork(const char* s, const char* p, byte prio = 5) {
        strncpy(ssid, s, sizeof(ssid)-1);
        strncpy(password, p, sizeof(password)-1);
        configured = true;
        lastConnected = 0;
        connectionAttempts = 0;
        priority = prio;
        rssi = 0;
        autoConnect = true;
    }
};

struct CryptoPosition {
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
    char positionSide[12];
    char marginType[12];
    
    bool exitAlerted;
    float exitAlertLastPrice;
    unsigned long exitAlertTime;
    bool hasAlerted;
    float lastAlertPercent;
    
    CryptoPosition() {
        memset(symbol, 0, sizeof(symbol));
        changePercent = 0.0;
        pnlValue = 0.0;
        quantity = 0.0;
        entryPrice = 0.0;
        currentPrice = 0.0;
        isLong = true;
        alerted = false;
        severeAlerted = false;
        lastAlertTime = 0;
        lastAlertPrice = 0.0;
        alertThreshold = DEFAULT_ALERT_THRESHOLD;
        severeThreshold = DEFAULT_SEVERE_THRESHOLD;
        memset(positionSide, 0, sizeof(positionSide));
        memset(marginType, 0, sizeof(marginType));
        exitAlerted = false;
        exitAlertLastPrice = 0.0;
        exitAlertTime = 0;
        hasAlerted = false;
        lastAlertPercent = 0.0;
    }
};

struct AlertHistory {
    char symbol[16];
    unsigned long alertTime;
    float pnlPercent;
    float alertPrice;
    bool isLong;
    bool isSevere;
    bool isProfit;
    byte alertType;
    char message[64];
    bool acknowledged;
    char timeString[20];
    byte alertMode;
    
    AlertHistory() {
        memset(symbol, 0, sizeof(symbol));
        alertTime = 0;
        pnlPercent = 0.0;
        alertPrice = 0.0;
        isLong = true;
        isSevere = false;
        isProfit = false;
        alertType = ALERT_NONE;
        memset(message, 0, sizeof(message));
        acknowledged = false;
        memset(timeString, 0, sizeof(timeString));
        alertMode = 0;
    }
};

struct PortfolioSummary {
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
    
    PortfolioSummary() {
        totalInvestment = 0.0;
        totalCurrentValue = 0.0;
        totalPnl = 0.0;
        totalPnlPercent = 0.0;
        totalPositions = 0;
        longPositions = 0;
        shortPositions = 0;
        winningPositions = 0;
        losingPositions = 0;
        maxDrawdown = 0.0;
        sharpeRatio = 0.0;
        avgPositionSize = 0.0;
        riskExposure = 0.0;
    }
};

struct SystemSettings {
    // WiFi Settings
    WiFiNetwork networks[MAX_WIFI_NETWORKS];
    int networkCount;
    int lastConnectedIndex;
    
    // API Settings
    char server[128];
    char username[32];
    char userpass[64];
    char entryPortfolio[32];
    char exitPortfolio[32];
    
    // Alert Settings
    float alertThreshold;
    float severeAlertThreshold;
    float portfolioAlertThreshold;
    int buzzerVolume;
    bool buzzerEnabled;
    bool separateLongShortAlerts;
    bool autoResetAlerts;
    int alertCooldown;
    
    // Display Settings
    int displayBrightness;
    int displayTimeout;
    bool showDetails;
    bool invertDisplay;
    byte displayRotation;
    
    // Exit Alert Settings
    float exitAlertPercent;
    bool exitAlertEnabled;
    bool exitAlertBlinkEnabled;
    
    // LED Settings
    int ledBrightness;
    bool ledEnabled;
    
    // RGB Settings
    bool rgb1Enabled;
    bool rgb2Enabled;
    int rgb1Brightness;
    int rgb2Brightness;
    int rgb1HistorySpeed;
    int rgb2Sensitivity;
    
    // Battery Settings
    bool showBattery;
    int batteryWarningLevel;
    
    // Connection Settings
    bool autoReconnect;
    int reconnectAttempts;
    
    // System Settings
    byte magicNumber;
    bool configured;
    unsigned long firstBoot;
    int bootCount;
    unsigned long totalUptime;
    
    SystemSettings() {
        networkCount = 0;
        lastConnectedIndex = -1;
        
        memset(server, 0, sizeof(server));
        memset(username, 0, sizeof(username));
        memset(userpass, 0, sizeof(userpass));
        strcpy(entryPortfolio, "Arduino");
        strcpy(exitPortfolio, "MyExit");
        
        alertThreshold = DEFAULT_ALERT_THRESHOLD;
        severeAlertThreshold = DEFAULT_SEVERE_THRESHOLD;
        portfolioAlertThreshold = PORTFOLIO_ALERT_THRESHOLD;
        buzzerVolume = DEFAULT_VOLUME;
        buzzerEnabled = true;
        separateLongShortAlerts = true;
        autoResetAlerts = false;
        alertCooldown = 300000;
        
        displayBrightness = 100;
        displayTimeout = 30000;
        showDetails = true;
        invertDisplay = false;
        displayRotation = 0;
        
        exitAlertPercent = DEFAULT_EXIT_ALERT_PERCENT;
        exitAlertEnabled = true;
        exitAlertBlinkEnabled = true;
        
        ledBrightness = DEFAULT_LED_BRIGHTNESS;
        ledEnabled = true;
        
        rgb1Enabled = true;
        rgb2Enabled = true;
        rgb1Brightness = 80;
        rgb2Brightness = 80;
        rgb1HistorySpeed = 50;
        rgb2Sensitivity = 50;
        
        showBattery = true;
        batteryWarningLevel = BATTERY_WARNING;
        
        autoReconnect = true;
        reconnectAttempts = 5;
        
        magicNumber = 0xAA;
        configured = false;
        firstBoot = 0;
        bootCount = 0;
        totalUptime = 0;
    }
};

struct SystemState {
    // Connection states
    bool isConnectedToWiFi;
    bool apModeActive;
    bool showingAlert;
    bool resetInProgress;
    bool displayInitialized;
    bool timeSynced;
    bool connectionLost;
    
    // Power source
    PowerSource powerSource;
    NetworkState networkState;
    
    // Timing variables
    unsigned long lastDataUpdate;
    unsigned long lastDisplayUpdate;
    unsigned long lastWiFiCheck;
    unsigned long lastAlertCheck;
    unsigned long alertDisplayStart;
    unsigned long systemStartTime;
    unsigned long lastBatteryCheck;
    unsigned long lastReconnectAttempt;
    unsigned long connectionLostTime;
    
    // Current state
    String currentDateTime;
    String alertTitle;
    String alertMessage;
    String alertSymbol;
    float alertPrice;
    bool alertIsLong;
    bool alertIsSevere;
    byte alertMode;
    
    // LED states
    bool mode1GreenActive;
    bool mode1RedActive;
    bool mode2GreenActive;
    bool mode2RedActive;
    bool blinkState;
    unsigned long ledTimeout;
    
    // Alert symbols
    String mode1AlertSymbol;
    String mode2AlertSymbol;
    float mode1AlertPercent;
    float mode2AlertPercent;
    
    // RGB states
    int rgb1HistoryIndex;
    int rgb1ColorIndex;
    bool rgb1Active;
    float rgb2CurrentPercent;
    bool rgb2AlertActive;
    
    // Display
    int currentDisplayPage;
    int totalDisplayPages;
    bool displayNeedsUpdate;
    
    // Battery
    float batteryVoltage;
    int batteryPercent;
    bool batteryLow;
    
    // API Statistics
    int apiSuccessCount;
    int apiErrorCount;
    unsigned long lastApiCallTime;
    float apiAverageResponseTime;
    
    // Connection Statistics
    int connectionLostCount;
    int reconnectSuccessCount;
    unsigned long totalDowntime;
    
    // Current display mode
    DisplayMode currentDisplayMode;
    
    SystemState() {
        isConnectedToWiFi = false;
        apModeActive = false;
        showingAlert = false;
        resetInProgress = false;
        displayInitialized = false;
        timeSynced = false;
        connectionLost = false;
        
        powerSource = POWER_SOURCE_USB;
        networkState = NET_OFFLINE;
        
        lastDataUpdate = 0;
        lastDisplayUpdate = 0;
        lastWiFiCheck = 0;
        lastAlertCheck = 0;
        alertDisplayStart = 0;
        systemStartTime = 0;
        lastBatteryCheck = 0;
        lastReconnectAttempt = 0;
        connectionLostTime = 0;
        
        alertPrice = 0.0;
        alertIsLong = false;
        alertIsSevere = false;
        alertMode = 0;
        
        mode1GreenActive = false;
        mode1RedActive = false;
        mode2GreenActive = false;
        mode2RedActive = false;
        blinkState = false;
        ledTimeout = 0;
        
        mode1AlertPercent = 0.0;
        mode2AlertPercent = 0.0;
        
        rgb1HistoryIndex = 0;
        rgb1ColorIndex = 0;
        rgb1Active = true;
        rgb2CurrentPercent = 0.0;
        rgb2AlertActive = false;
        
        currentDisplayPage = 0;
        totalDisplayPages = 1;
        displayNeedsUpdate = true;
        
        batteryVoltage = 0.0;
        batteryPercent = 100;
        batteryLow = false;
        
        apiSuccessCount = 0;
        apiErrorCount = 0;
        lastApiCallTime = 0;
        apiAverageResponseTime = 0.0;
        
        connectionLostCount = 0;
        reconnectSuccessCount = 0;
        totalDowntime = 0;
        
        currentDisplayMode = DISPLAY_MODE_SPLASH;
    }
};

// ===== FORWARD DECLARATIONS =====
class DisplayManager;
class AlertManager;
class DataProcessor;
class BuzzerManager;
class LEDManager;
class SettingsManager;
class WiFiManager;
class WebInterface;
class BatteryManager;
class TimeManager;
class APIManager;

// ===== UTILITY FUNCTIONS =====
String formatNumber(float number, int decimals = 2);
String formatPercent(float percent);
String formatPrice(float price);
String formatTime(unsigned long timestamp);
String formatDateTime(unsigned long timestamp);
String getShortSymbol(const char* symbol);
String getWiFiQuality(int rssi);
String urlEncode(String str);
String urlDecode(String str);
String base64Encode(String data);

#endif // SYSTEM_CONFIG_H