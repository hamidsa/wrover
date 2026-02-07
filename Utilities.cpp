// Utilities.cpp
#include "Utilities.h"

String urlEncode(const String& str) {
    String encoded = "";
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

String urlDecode(const String& str) {
    String decoded = "";
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

String getValueFromQueryString(const String& queryString, const String& key) {
    int startIndex = queryString.indexOf(key + "=");
    if (startIndex == -1) return "";
    
    startIndex += key.length() + 1;
    int endIndex = queryString.indexOf('&', startIndex);
    
    if (endIndex == -1) {
        return queryString.substring(startIndex);
    }
    
    return queryString.substring(startIndex, endIndex);
}

bool isValidNumber(const String& str) {
    if (str.length() == 0) return false;
    
    bool hasDecimal = false;
    for (unsigned int i = 0; i < str.length(); i++) {
        char c = str.charAt(i);
        
        if (i == 0 && (c == '-' || c == '+')) {
            continue;
        }
        
        if (c == '.') {
            if (hasDecimal) return false;
            hasDecimal = true;
            continue;
        }
        
        if (!isdigit(c)) return false;
    }
    
    return true;
}

bool isValidSSID(const String& ssid) {
    if (ssid.length() == 0 || ssid.length() > 32) return false;
    
    // Check for invalid characters
    for (unsigned int i = 0; i < ssid.length(); i++) {
        char c = ssid.charAt(i);
        if (c < 32 || c > 126) return false;
    }
    
    return true;
}

bool isValidPassword(const String& password) {
    // WiFi passwords can be 8-63 characters (WPA2)
    return password.length() >= 8 && password.length() <= 63;
}

String formatBytes(size_t bytes) {
    if (bytes < 1024) {
        return String(bytes) + " B";
    } else if (bytes < 1048576) {
        return String(bytes / 1024.0, 1) + " KB";
    } else if (bytes < 1073741824) {
        return String(bytes / 1048576.0, 1) + " MB";
    } else {
        return String(bytes / 1073741824.0, 1) + " GB";
    }
}

String getDeviceID() {
    uint32_t chipId = (uint32_t)ESP.getEfuseMac();
    char buffer[20];
    snprintf(buffer, sizeof(buffer), "%08X", chipId);
    return String(buffer);
}

String getFirmwareVersion() {
    return "4.5.3";
}

String getDeviceInfo() {
    String info = "ESP32-WROVER-E\n";
    info += "Chip ID: " + getDeviceID() + "\n";
    info += "CPU Freq: " + String(ESP.getCpuFreqMHz()) + " MHz\n";
    info += "Flash Size: " + formatBytes(ESP.getFlashChipSize()) + "\n";
    info += "PSRAM Size: " + formatBytes(ESP.getPsramSize()) + "\n";
    info += "Free Heap: " + formatBytes(ESP.getFreeHeap()) + "\n";
    info += "SDK Version: " + String(ESP.getSdkVersion()) + "\n";
    info += "Firmware: v" + getFirmwareVersion();
    
    return info;
}

void logEvent(const String& event, const String& details) {
    Serial.println("[" + getCurrentTimeString() + "] " + event + ": " + details);
}

String getCurrentTimeString() {
    // This would use TimeManager in a real implementation
    unsigned long ms = millis();
    unsigned long seconds = ms / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;
    
    char buffer[20];
    snprintf(buffer, sizeof(buffer), "%02lu:%02lu:%02lu.%03lu", 
             hours % 24, minutes % 60, seconds % 60, ms % 1000);
    return String(buffer);
}

String generateRandomString(int length) {
    String result = "";
    const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    
    for (int i = 0; i < length; i++) {
        int key = random(0, sizeof(charset) - 1);
        result += charset[key];
    }
    
    return result;
}

bool isWithinRange(float value, float min, float max) {
    return value >= min && value <= max;
}

float clamp(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

int clamp(int value, int min, int max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

String boolToString(bool value) {
    return value ? "true" : "false";
}

String intToHexString(int value, int digits) {
    char buffer[20];
    snprintf(buffer, sizeof(buffer), "%0*X", digits, value);
    return String(buffer);
}

String floatToString(float value, int precision) {
    char buffer[20];
    dtostrf(value, 0, precision, buffer);
    return String(buffer);
}

String escapeHTML(const String& str) {
    String escaped = "";
    
    for (unsigned int i = 0; i < str.length(); i++) {
        char c = str.charAt(i);
        
        switch (c) {
            case '&': escaped += "&amp;"; break;
            case '<': escaped += "&lt;"; break;
            case '>': escaped += "&gt;"; break;
            case '"': escaped += "&quot;"; break;
            case '\'': escaped += "&#39;"; break;
            default: escaped += c;
        }
    }
    
    return escaped;
}

String truncateString(const String& str, int maxLength) {
    if (str.length() <= maxLength) return str;
    return str.substring(0, maxLength - 3) + "...";
}

bool stringToBool(const String& str) {
    String lower = str;
    lower.toLowerCase();
    
    return lower == "true" || lower == "1" || lower == "yes" || lower == "on";
}

String getFileExtension(const String& filename) {
    int dotIndex = filename.lastIndexOf('.');
    if (dotIndex == -1) return "";
    return filename.substring(dotIndex + 1);
}

String getFileName(const String& path) {
    int slashIndex = path.lastIndexOf('/');
    if (slashIndex == -1) return path;
    return path.substring(slashIndex + 1);
}

String getDirectoryPath(const String& path) {
    int slashIndex = path.lastIndexOf('/');
    if (slashIndex == -1) return "";
    return path.substring(0, slashIndex);
}