#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#include <time.h>
#include "SystemConfig.h"

class TimeManager {
private:
    // NTP configuration
    const char* ntpServer;
    long gmtOffset;
    int daylightOffset;
    
    // Time state
    bool timeSynced;
    unsigned long lastSyncTime;
    unsigned long lastUpdateTime;
    
    // RTC simulation
    unsigned long systemUptime;
    unsigned long lastMillis;
    
    // Timezone support
    struct Timezone {
        String name;
        long offset;
        bool dst;
        String dstRule;
    } currentTimezone;
    
    // Statistics
    struct TimeStats {
        int syncAttempts;
        int successfulSyncs;
        int failedSyncs;
        unsigned long totalSyncTime;
        float averageSyncTime;
        unsigned long uptime;
        
        TimeStats() : syncAttempts(0), successfulSyncs(0), failedSyncs(0),
                     totalSyncTime(0), averageSyncTime(0), uptime(0) {}
    } stats;
    
    // Callbacks
    typedef std::function<void()> TimeSyncCallback;
    TimeSyncCallback syncCallback;
    
public:
    TimeManager();
    
    // ===== INITIALIZATION =====
    void init(const char* server = NTP_SERVER, 
              long offset = GMT_OFFSET, 
              int daylight = DAYLIGHT_OFFSET);
    bool isInitialized() const;
    
    // ===== TIME SYNCHRONIZATION =====
    bool syncTime();
    bool syncTime(const char* server, long offset, int daylight);
    bool isTimeSynced() const;
    unsigned long getLastSyncTime() const;
    
    // ===== TIMEZONE MANAGEMENT =====
    void setTimezone(const String& name, long offset, bool dst = false);
    void setTimezoneOffset(long offset);
    void enableDST(bool enable);
    String getTimezone() const;
    long getTimezoneOffset() const;
    
    // ===== TIME RETRIEVAL =====
    String getCurrentTime();
    String getCurrentDate();
    String getCurrentDateTime();
    unsigned long getEpochTime();
    struct tm getLocalTime();
    
    // ===== TIME FORMATTING =====
    String formatTime(const struct tm& timeinfo, const String& format = "%H:%M:%S");
    String formatDate(const struct tm& timeinfo, const String& format = "%Y-%m-%d");
    String formatDateTime(const struct tm& timeinfo, const String& format = "%Y-%m-%d %H:%M:%S");
    
    String formatTime(unsigned long timestamp, const String& format = "%H:%M:%S");
    String formatDate(unsigned long timestamp, const String& format = "%Y-%m-%d");
    String formatDateTime(unsigned long timestamp, const String& format = "%Y-%m-%d %H:%M:%S");
    
    // ===== TIME CALCULATIONS =====
    unsigned long addSeconds(unsigned long time, int seconds);
    unsigned long addMinutes(unsigned long time, int minutes);
    unsigned long addHours(unsigned long time, int hours);
    unsigned long addDays(unsigned long time, int days);
    
    int getSecondsSince(unsigned long timestamp);
    int getMinutesSince(unsigned long timestamp);
    int getHoursSince(unsigned long timestamp);
    int getDaysSince(unsigned long timestamp);
    
    String getTimeSince(unsigned long timestamp);
    
    // ===== UPDATE LOOP =====
    void update();
    void maintainSync();
    
    // ===== RTC FUNCTIONS =====
    unsigned long getUptime() const;
    String getUptimeString() const;
    void resetUptime();
    
    // ===== CALLBACKS =====
    void setSyncCallback(TimeSyncCallback callback);
    
    // ===== STATISTICS =====
    TimeStats getStatistics() const;
    void resetStatistics();
    float getSyncSuccessRate() const;
    
    // ===== DEBUG FUNCTIONS =====
    void printTime() const;
    void printStatistics() const;
    void testTimeFunctions();
    
    // ===== UTILITY FUNCTIONS =====
    bool isDSTActive() const;
    int getDayOfWeek() const;
    int getDayOfYear() const;
    bool isLeapYear(int year) const;
    
    String getMonthName(int month);
    String getDayName(int day);
    
private:
    // Internal helper functions
    bool configureNTP();
    bool waitForSync(unsigned long timeout = 5000);
    
    // Timezone calculations
    long calculateTimezoneOffset() const;
    bool isDST(const struct tm& timeinfo) const;
    void applyDST(struct tm& timeinfo);
    
    // Statistics updating
    void updateStats(bool success, unsigned long syncTime);
    
    // RTC maintenance
    void updateUptime();
    
    // Error handling
    bool handleSyncError(int error);
    String getSyncErrorString(int error) const;
    
    // Validation
    bool validateTime(const struct tm& timeinfo) const;
    bool validateDate(const struct tm& timeinfo) const;
    
    // Debug logging
    void logSyncAttempt(bool success, unsigned long duration);
};

// Inline functions
inline bool TimeManager::isInitialized() const {
    return ntpServer != nullptr;
}

inline bool TimeManager::isTimeSynced() const {
    return timeSynced;
}

inline unsigned long TimeManager::getLastSyncTime() const {
    return lastSyncTime;
}

inline String TimeManager::getTimezone() const {
    return currentTimezone.name;
}

inline long TimeManager::getTimezoneOffset() const {
    return currentTimezone.offset + (currentTimezone.dst && isDSTActive() ? 3600 : 0);
}

inline TimeManager::TimeStats TimeManager::getStatistics() const {
    return stats;
}

inline float TimeManager::getSyncSuccessRate() const {
    if (stats.syncAttempts == 0) return 0.0;
    return (stats.successfulSyncs * 100.0) / stats.syncAttempts;
}

inline unsigned long TimeManager::getUptime() const {
    return stats.uptime;
}

inline void TimeManager::setSyncCallback(TimeSyncCallback callback) {
    syncCallback = callback;
}

// Time format constants
namespace TimeFormats {
    const char* TIME_12H = "%I:%M:%S %p";
    const char* TIME_24H = "%H:%M:%S";
    const char* DATE_SHORT = "%m/%d/%Y";
    const char* DATE_LONG = "%B %d, %Y";
    const char* DATETIME_SHORT = "%m/%d/%Y %H:%M";
    const char* DATETIME_LONG = "%A, %B %d, %Y %H:%M:%S";
    const char* ISO8601 = "%Y-%m-%dT%H:%M:%SZ";
    const char* LOG_FORMAT = "%Y-%m-%d %H:%M:%S";
}

#endif // TIME_MANAGER_H