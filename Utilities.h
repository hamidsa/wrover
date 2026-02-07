#ifndef UTILITIES_H
#define UTILITIES_H

#include <Arduino.h>
#include <vector>
#include <map>

class Utilities {
public:
    // ===== STRING MANIPULATION =====
    static String trim(const String& str);
    static String toLowerCase(const String& str);
    static String toUpperCase(const String& str);
    static String replace(const String& str, const String& from, const String& to);
    static bool startsWith(const String& str, const String& prefix);
    static bool endsWith(const String& str, const String& suffix);
    static bool contains(const String& str, const String& substring);
    
    static std::vector<String> split(const String& str, char delimiter);
    static String join(const std::vector<String>& parts, const String& delimiter);
    
    static String padLeft(const String& str, int length, char padChar = ' ');
    static String padRight(const String& str, int length, char padChar = ' ');
    static String padCenter(const String& str, int length, char padChar = ' ');
    
    // ===== NUMBER FORMATTING =====
    static String formatFloat(float value, int decimals = 2);
    static String formatDouble(double value, int decimals = 4);
    static String formatInteger(int value, int minDigits = 1);
    static String formatBytes(size_t bytes);
    static String formatPercentage(float percent, int decimals = 1);
    static String formatCurrency(float amount, const String& symbol = "$");
    
    static String scientificNotation(float value, int decimals = 2);
    static String engineeringNotation(float value, int decimals = 2);
    
    // ===== DATE/TIME FORMATTING =====
    static String formatDuration(unsigned long milliseconds);
    static String formatTimestamp(unsigned long timestamp);
    static String formatInterval(unsigned long start, unsigned long end);
    
    // ===== COLOR CONVERSION =====
    static uint32_t rgbToHex(uint8_t r, uint8_t g, uint8_t b);
    static void hexToRgb(uint32_t hex, uint8_t& r, uint8_t& g, uint8_t& b);
    static uint32_t hslToHex(float h, float s, float l);
    static void hexToHsl(uint32_t hex, float& h, float& s, float& l);
    
    static uint32_t interpolateColor(uint32_t color1, uint32_t color2, float ratio);
    static uint32_t darkenColor(uint32_t color, float factor);
    static uint32_t lightenColor(uint32_t color, float factor);
    
    // ===== MATHEMATICAL FUNCTIONS =====
    static float mapFloat(float x, float in_min, float in_max, 
                         float out_min, float out_max);
    static float constrainFloat(float value, float min, float max);
    static float lerp(float a, float b, float t);
    static float smoothStep(float edge0, float edge1, float x);
    
    static float calculateAverage(const std::vector<float>& values);
    static float calculateStdDev(const std::vector<float>& values);
    static float calculateMovingAverage(float newValue, float oldAverage, 
                                       int count, int window = 10);
    
    // ===== DATA VALIDATION =====
    static bool isValidFloat(const String& str);
    static bool isValidInteger(const String& str);
    static bool isValidHex(const String& str);
    static bool isValidIP(const String& ip);
    static bool isValidMAC(const String& mac);
    static bool isValidEmail(const String& email);
    
    static bool isInRange(float value, float min, float max);
    static bool isInRange(int value, int min, int max);
    
    // ===== ENCRYPTION & HASHING =====
    static String md5Hash(const String& data);
    static String sha256Hash(const String& data);
    static String base64Encode(const String& data);
    static String base64Decode(const String& data);
    static String xorEncrypt(const String& data, const String& key);
    static String xorDecrypt(const String& data, const String& key);
    
    // ===== FILE SYSTEM HELPERS =====
    static String getFileExtension(const String& filename);
    static String getFileName(const String& path);
    static String getDirectory(const String& path);
    static String normalizePath(const String& path);
    
    static bool fileExists(const String& path);
    static size_t getFileSize(const String& path);
    static String getFileContent(const String& path);
    
    // ===== NETWORK UTILITIES =====
    static String getLocalIP();
    static String getMACAddress();
    static String getHostname();
    static int getRSSI();
    
    static bool ping(const String& host, int timeout = 1000);
    static String resolveHostname(const String& hostname);
    
    // ===== SYSTEM INFORMATION =====
    static uint32_t getChipId();
    static String getChipModel();
    static int getCpuFrequency();
    static int getFreeHeap();
    static int getTotalHeap();
    static int getMinFreeHeap();
    static int getMaxAllocHeap();
    
    static String getResetReason();
    static String getResetInfo();
    
    // ===== MEMORY MANAGEMENT =====
    static void* allocateMemory(size_t size);
    static void freeMemory(void* ptr);
    static size_t getMemoryUsage();
    
    static String memoryInfo();
    static void checkMemory(const String& tag = "");
    
    // ===== DEBUG & LOGGING =====
    static void debug(const String& message);
    static void info(const String& message);
    static void warning(const String& message);
    static void error(const String& message);
    
    static void hexDump(const uint8_t* data, size_t length, 
                       const String& prefix = "");
    static void printMemoryMap();
    
    // ===== RANDOM GENERATION =====
    static int randomInt(int min, int max);
    static float randomFloat(float min, float max);
    static String randomString(int length, const String& charset = 
                              "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789");
    static String randomHex(int length);
    
    // ===== DATA STRUCTURES =====
    template<typename T>
    static bool vectorContains(const std::vector<T>& vec, const T& value);
    
    template<typename K, typename V>
    static bool mapContains(const std::map<K, V>& map, const K& key);
    
    template<typename T>
    static void removeDuplicates(std::vector<T>& vec);
    
    // ===== TEMPLATE IMPLEMENTATIONS =====
    template<typename T>
    static bool vectorContains(const std::vector<T>& vec, const T& value) {
        return std::find(vec.begin(), vec.end(), value) != vec.end();
    }
    
    template<typename K, typename V>
    static bool mapContains(const std::map<K, V>& map, const K& key) {
        return map.find(key) != map.end();
    }
    
    template<typename T>
    static void removeDuplicates(std::vector<T>& vec) {
        std::sort(vec.begin(), vec.end());
        vec.erase(std::unique(vec.begin(), vec.end()), vec.end());
    }
};

// Macros for common operations
#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))
#define ABS(x) ((x)>0?(x):-(x))
#define CLAMP(x,min,max) ((x)<(min)?(min):((x)>(max)?(max):(x)))
#define LERP(a,b,t) ((a)+(t)*((b)-(a)))

// Debug macros
#ifdef DEBUG_ENABLED
    #define DEBUG_LOG(msg) Serial.println("[DEBUG] " + String(msg))
    #define DEBUG_VAR(name, value) Serial.println("[DEBUG] " + String(name) + " = " + String(value))
    #define DEBUG_HEX(data, len) Utilities::hexDump(data, len, "[DEBUG] ")
#else
    #define DEBUG_LOG(msg)
    #define DEBUG_VAR(name, value)
    #define DEBUG_HEX(data, len)
#endif

// Memory checking
#define CHECK_MEMORY(tag) Utilities::checkMemory(tag)

#endif // UTILITIES_H