#ifndef VALIDATORS_H
#define VALIDATORS_H

#include <Arduino.h>
#include <regex>

class Validators {
public:
    // Network validation
    static bool isValidSSID(const String& ssid);
    static bool isValidPassword(const String& password);
    static bool isValidIP(const String& ip);
    static bool isValidMAC(const String& mac);
    
    // API validation
    static bool isValidURL(const String& url);
    static bool isValidAPIKey(const String& key);
    static bool isValidPort(int port);
    
    // Data validation
    static bool isValidSymbol(const String& symbol);
    static bool isValidPrice(float price);
    static bool isValidQuantity(float quantity);
    static bool isValidPercentage(float percentage);
    
    // User input validation
    static bool isValidEmail(const String& email);
    static bool isValidUsername(const String& username);
    static bool isStrongPassword(const String& password);
    
    // JSON validation
    static bool isValidJSON(const String& json);
    
    // Range validation
    template<typename T>
    static bool isInRange(T value, T min, T max) {
        return value >= min && value <= max;
    }
    
    // Pattern matching
    static bool matchesPattern(const String& input, const String& pattern);
    
private:
    static std::regex createRegex(const String& pattern);
};

#endif