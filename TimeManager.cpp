#include "TimeManager.h"
#include "ConfigManager.h"
#include <WiFi.h>

// ===== CONSTANTS =====
#define NTP_SERVER "pool.ntp.org"
#define NTP_TIMEOUT 5000
#define TIME_SYNC_INTERVAL 3600000  // 1 hour
#define TIME_FORMAT_24H true

// ===== STATIC VARIABLES =====
TimeManager* TimeManager::_instance = nullptr;

// ===== CONSTRUCTOR/DESTRUCTOR =====
TimeManager::TimeManager()
    : _initialized(false),
      _synced(false),
      _timezone(3.5),  // Iran timezone (UTC+3:30)
      _daylightSaving(false),
      _lastSyncTime(0),
      _lastUpdateTime(0),
      _updateInterval(1000) {
}

TimeManager::~TimeManager() {
    if (_instance == this) {
        _instance = nullptr;
    }
}

// ===== INITIALIZATION =====
bool TimeManager::begin() {
    Serial.println("Initializing Time Manager...");
    
    // Load configuration
    _timezone = ConfigManager::getInstance().getFloat("timezone", 3.5);
    _daylightSaving = ConfigManager::getInstance().getBool("daylight_saving", false);
    
    // Set timezone
    setTimezone(_timezone, _daylightSaving);
    
    _initialized = true;
    Serial.println("Time Manager initialized");
    
    // Try to sync if WiFi is connected
    if (WiFi.status() == WL_CONNECTED) {
        syncTime();
    }
    
    return true;
}

void TimeManager::update() {
    if (!_initialized) return;
    
    unsigned long currentTime = millis();
    if (currentTime - _lastUpdateTime < _updateInterval) return;
    
    _lastUpdateTime = currentTime;
    
    // Check if we need to sync time
    if (!_synced && WiFi.status() == WL_CONNECTED) {
        if (currentTime - _lastSyncTime > 30000) { // Try every 30 seconds until synced
            syncTime();
        }
    }
    
    // Periodic sync
    if (_synced && currentTime - _lastSyncTime > TIME_SYNC_INTERVAL) {
        syncTime();
    }
}

// ===== TIME SYNC =====
bool TimeManager::syncTime() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Cannot sync time: WiFi not connected");
        return false;
    }
    
    Serial.println("Syncing time with NTP server...");
    
    // Configure NTP
    configTime(_timezone * 3600, _daylightSaving ? 3600 : 0, NTP_SERVER);
    
    // Wait for time to be set
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, NTP_TIMEOUT)) {
        Serial.println("Failed to obtain time from NTP");
        return false;
    }
    
    _synced = true;
    _lastSyncTime = millis();
    
    Serial.print("Time synced: ");
    Serial.println(asctime(&timeinfo));
    
    // Notify other components
    onTimeSynced();
    
    return true;
}

bool TimeManager::syncTime(const char* ntpServer, long gmtOffset, int daylightOffset) {
    Serial.print("Syncing time with custom server: ");
    Serial.println(ntpServer);
    
    configTime(gmtOffset, daylightOffset, ntpServer);
    
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, NTP_TIMEOUT)) {
        Serial.println("Failed to obtain time from custom NTP server");
        return false;
    }
    
    _synced = true;
    _lastSyncTime = millis();
    
    Serial.print("Time synced: ");
    Serial.println(asctime(&timeinfo));
    
    return true;
}

// ===== TIMEZONE MANAGEMENT =====
void TimeManager::setTimezone(float timezone, bool daylightSaving) {
    _timezone = timezone;
    _daylightSaving = daylightSaving;
    
    // Save to config
    ConfigManager::getInstance().setFloat("timezone", timezone);
    ConfigManager::getInstance().setBool("daylight_saving", daylightSaving);
    
    // Update NTP configuration if synced
    if (_synced) {
        configTime(_timezone * 3600, _daylightSaving ? 3600 : 0, NTP_SERVER);
    }
    
    Serial.print("Timezone set to: UTC");
    if (_timezone >= 0) Serial.print("+");
    Serial.print(_timezone, 1);
    if (_daylightSaving) Serial.print(" (DST)");
    Serial.println();
}

void TimeManager::setTimezone(const char* timezoneStr) {
    // Parse timezone string like "UTC+3:30"
    float timezone = 0;
    bool daylightSaving = false;
    
    // Simple parsing (for more robust parsing, use a library)
    if (strstr(timezoneStr, "UTC+3:30") || strstr(timezoneStr, "Iran")) {
        timezone = 3.5;
    } else if (strstr(timezoneStr, "UTC+1") || strstr(timezoneStr, "CET")) {
        timezone = 1.0;
    } else if (strstr(timezoneStr, "UTC+0") || strstr(timezoneStr, "GMT")) {
        timezone = 0.0;
    } else if (strstr(timezoneStr, "UTC-5") || strstr(timezoneStr, "EST")) {
        timezone = -5.0;
    }
    // Add more timezones as needed
    
    if (strstr(timezoneStr, "DST") || strstr(timezoneStr, "daylight")) {
        daylightSaving = true;
    }
    
    setTimezone(timezone, daylightSaving);
}

// ===== TIME GETTERS =====
time_t TimeManager::getTimestamp() {
    time_t now;
    time(&now);
    return now;
}

String TimeManager::getFormattedTime() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        return "00:00:00";
    }
    
    char buffer[20];
    if (TIME_FORMAT_24H) {
        strftime(buffer, sizeof(buffer), "%H:%M:%S", &timeinfo);
    } else {
        strftime(buffer, sizeof(buffer), "%I:%M:%S %p", &timeinfo);
    }
    
    return String(buffer);
}

String TimeManager::getFormattedDate() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        return "1970-01-01";
    }
    
    char buffer[20];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d", &timeinfo);
    
    return String(buffer);
}

String TimeManager::getFormattedDateTime() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        return "1970-01-01 00:00:00";
    }
    
    char buffer[30];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
    
    return String(buffer);
}

String TimeManager::getDayOfWeek() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        return "Unknown";
    }
    
    char buffer[20];
    strftime(buffer, sizeof(buffer), "%A", &timeinfo);
    
    return String(buffer);
}

uint8_t TimeManager::getHour() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        return 0;
    }
    return timeinfo.tm_hour;
}

uint8_t TimeManager::getMinute() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        return 0;
    }
    return timeinfo.tm_min;
}

uint8_t TimeManager::getSecond() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        return 0;
    }
    return timeinfo.tm_sec;
}

uint8_t TimeManager::getDay() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        return 1;
    }
    return timeinfo.tm_mday;
}

uint8_t TimeManager::getMonth() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        return 1;
    }
    return timeinfo.tm_mon + 1; // tm_mon is 0-based
}

uint16_t TimeManager::getYear() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        return 1970;
    }
    return timeinfo.tm_year + 1900; // tm_year is years since 1900
}

// ===== TIME SETTERS =====
bool TimeManager::setTime(uint16_t year, uint8_t month, uint8_t day,
                         uint8_t hour, uint8_t minute, uint8_t second) {
    struct tm timeinfo = {0};
    
    timeinfo.tm_year = year - 1900;
    timeinfo.tm_mon = month - 1;
    timeinfo.tm_mday = day;
    timeinfo.tm_hour = hour;
    timeinfo.tm_min = minute;
    timeinfo.tm_sec = second;
    
    time_t t = mktime(&timeinfo);
    struct timeval now = { .tv_sec = t };
    
    if (settimeofday(&now, NULL) == 0) {
        _synced = true;
        Serial.println("Time set manually");
        return true;
    }
    
    return false;
}

bool TimeManager::setTime(const char* dateTimeStr) {
    // Parse string in format "YYYY-MM-DD HH:MM:SS"
    struct tm timeinfo = {0};
    
    if (sscanf(dateTimeStr, "%d-%d-%d %d:%d:%d",
               &timeinfo.tm_year, &timeinfo.tm_mon, &timeinfo.tm_mday,
               &timeinfo.tm_hour, &timeinfo.tm_min, &timeinfo.tm_sec) == 6) {
        
        timeinfo.tm_year -= 1900;
        timeinfo.tm_mon -= 1;
        
        return setTime(timeinfo.tm_year + 1900, timeinfo.tm_mon + 1,
                      timeinfo.tm_mday, timeinfo.tm_hour,
                      timeinfo.tm_min, timeinfo.tm_sec);
    }
    
    return false;
}

// ===== TIME UTILITIES =====
String TimeManager::formatTimestamp(time_t timestamp) {
    struct tm* timeinfo = localtime(&timestamp);
    
    char buffer[30];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
    
    return String(buffer);
}

String TimeManager::formatTimeDifference(time_t from, time_t to) {
    time_t diff = abs(to - from);
    
    if (diff < 60) {
        return String(diff) + " seconds";
    } else if (diff < 3600) {
        return String(diff / 60) + " minutes";
    } else if (diff < 86400) {
        return String(diff / 3600) + " hours";
    } else {
        return String(diff / 86400) + " days";
    }
}

bool TimeManager::isDaytime() {
    uint8_t hour = getHour();
    return (hour >= 6 && hour < 18);
}

bool TimeManager::isWeekend() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        return false;
    }
    
    // Sunday = 0, Saturday = 6
    return (timeinfo.tm_wday == 0 || timeinfo.tm_wday == 6);
}

// ===== ALARMS & TIMERS =====
void TimeManager::setAlarm(uint8_t hour, uint8_t minute, AlarmCallback callback) {
    Alarm alarm;
    alarm.hour = hour;
    alarm.minute = minute;
    alarm.callback = callback;
    alarm.enabled = true;
    alarm.lastTriggered = 0;
    
    _alarms.push_back(alarm);
    
    Serial.print("Alarm set for ");
    Serial.print(hour);
    Serial.print(":");
    Serial.println(minute);
}

void TimeManager::checkAlarms() {
    if (_alarms.empty()) return;
    
    uint8_t currentHour = getHour();
    uint8_t currentMinute = getMinute();
    time_t currentTime = getTimestamp();
    
    for (auto& alarm : _alarms) {
        if (!alarm.enabled) continue;
        
        if (currentHour == alarm.hour && currentMinute == alarm.minute) {
            // Check if we already triggered this alarm this minute
            if (currentTime - alarm.lastTriggered > 60) {
                alarm.lastTriggered = currentTime;
                
                if (alarm.callback) {
                    alarm.callback();
                }
                
                Serial.print("Alarm triggered at ");
                Serial.print(currentHour);
                Serial.print(":");
                Serial.println(currentMinute);
            }
        }
    }
}

// ===== SCHEDULED TASKS =====
void TimeManager::scheduleTask(uint32_t interval, TaskCallback callback, bool repeat) {
    ScheduledTask task;
    task.interval = interval;
    task.callback = callback;
    task.repeat = repeat;
    task.lastRun = 0;
    task.enabled = true;
    
    _scheduledTasks.push_back(task);
    
    Serial.print("Task scheduled every ");
    Serial.print(interval);
    Serial.println("ms");
}

void TimeManager::checkScheduledTasks() {
    if (_scheduledTasks.empty()) return;
    
    unsigned long currentTime = millis();
    
    for (auto it = _scheduledTasks.begin(); it != _scheduledTasks.end();) {
        if (!it->enabled) {
            ++it;
            continue;
        }
        
        if (currentTime - it->lastRun >= it->interval) {
            it->lastRun = currentTime;
            
            if (it->callback) {
                it->callback();
            }
            
            if (!it->repeat) {
                it = _scheduledTasks.erase(it);
                continue;
            }
        }
        
        ++it;
    }
}

// ===== WEB INTERFACE =====
String TimeManager::getStatusJSON() {
    DynamicJsonDocument doc(512);
    
    doc["synced"] = _synced;
    doc["timestamp"] = getTimestamp();
    doc["formatted_time"] = getFormattedTime();
    doc["formatted_date"] = getFormattedDate();
    doc["formatted_datetime"] = getFormattedDateTime();
    doc["day_of_week"] = getDayOfWeek();
    doc["timezone"] = _timezone;
    doc["daylight_saving"] = _daylightSaving;
    doc["last_sync"] = _lastSyncTime;
    doc["is_daytime"] = isDaytime();
    doc["is_weekend"] = isWeekend();
    
    // Current time components
    JsonObject timeComponents = doc.createNestedObject("components");
    timeComponents["hour"] = getHour();
    timeComponents["minute"] = getMinute();
    timeComponents["second"] = getSecond();
    timeComponents["day"] = getDay();
    timeComponents["month"] = getMonth();
    timeComponents["year"] = getYear();
    
    String json;
    serializeJson(doc, json);
    return json;
}

void TimeManager::handleWebRequest(const String& action, const String& params) {
    if (action == "sync") {
        bool success = syncTime();
        if (success) {
            // Return success
        } else {
            // Return error
        }
    }
    else if (action == "set_timezone") {
        setTimezone(params.toFloat(), _daylightSaving);
    }
    else if (action == "set_time") {
        // params format: "YYYY-MM-DD HH:MM:SS"
        setTime(params.c_str());
    }
    else if (action == "set_dst") {
        _daylightSaving = (params == "true");
        setTimezone(_timezone, _daylightSaving);
    }
}

// ===== EVENT HANDLERS =====
void TimeManager::onTimeSynced() {
    // Notify other components that time has been synced
    Serial.println("Time synchronization complete");
    
    // You can add callbacks here for other components
    // For example:
    // DisplayManager::getInstance().updateTimeDisplay();
    // DataManager::getInstance().scheduleDailyUpdate();
}

// ===== PRINT STATUS =====
void TimeManager::printStatus() {
    Serial.println("\n=== Time Status ===");
    Serial.print("Synced: ");
    Serial.println(_synced ? "Yes" : "No");
    
    if (_synced) {
        Serial.print("Current Time: ");
        Serial.println(getFormattedDateTime());
        Serial.print("Timezone: UTC");
        if (_timezone >= 0) Serial.print("+");
        Serial.print(_timezone, 1);
        if (_daylightSaving) Serial.print(" (DST)");
        Serial.println();
        Serial.print("Day of Week: ");
        Serial.println(getDayOfWeek());
        Serial.print("Is Daytime: ");
        Serial.println(isDaytime() ? "Yes" : "No");
        Serial.print("Is Weekend: ");
        Serial.println(isWeekend() ? "Yes" : "No");
    }
    
    Serial.print("Alarms: ");
    Serial.println(_alarms.size());
    Serial.print("Scheduled Tasks: ");
    Serial.println(_scheduledTasks.size());
    Serial.println("==================\n");
}

// ===== GETTERS =====
bool TimeManager::isSynced() const { return _synced; }
float TimeManager::getTimezone() const { return _timezone; }
bool TimeManager::isDaylightSaving() const { return _daylightSaving; }
bool TimeManager::isInitialized() const { return _initialized; }

// ===== STATIC ACCESS =====
TimeManager& TimeManager::getInstance() {
    if (!_instance) {
        _instance = new TimeManager();
    }
    return *_instance;
}