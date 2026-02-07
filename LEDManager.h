#ifndef LED_MANAGER_H
#define LED_MANAGER_H

#include <Arduino.h>
#include "SystemConfig.h"

class LEDManager {
private:
    // LED pin assignments
    struct LEDPins {
        int mode1Green;
        int mode1Red;
        int mode2Green;
        int mode2Red;
        int rgb1Red;
        int rgb1Green;
        int rgb1Blue;
        int rgb2Red;
        int rgb2Green;
        int rgb2Blue;
    } pins;
    
    // LED states
    struct LEDState {
        bool mode1Green;
        bool mode1Red;
        bool mode2Green;
        bool mode2Red;
        
        struct RGBState {
            uint8_t red;
            uint8_t green;
            uint8_t blue;
            bool enabled;
            int brightness;
        } rgb1, rgb2;
        
        LEDState() : mode1Green(false), mode1Red(false),
                    mode2Green(false), mode2Red(false) {
            rgb1.red = rgb1.green = rgb1.blue = 0;
            rgb1.enabled = true;
            rgb1.brightness = 100;
            
            rgb2.red = rgb2.green = rgb2.blue = 0;
            rgb2.enabled = true;
            rgb2.brightness = 100;
        }
    } state;
    
    // Blink control
    struct BlinkControl {
        bool enabled;
        unsigned long interval;
        unsigned long lastChange;
        bool state;
        
        BlinkControl() : enabled(false), interval(500),
                        lastChange(0), state(false) {}
    } blink;
    
    // Animation control
    struct Animation {
        bool active;
        byte type;
        unsigned long startTime;
        unsigned long duration;
        int cycles;
        float progress;
        
        Animation() : active(false), type(0), startTime(0),
                     duration(1000), cycles(0), progress(0.0) {}
    } animation;
    
    // Configuration
    bool initialized;
    bool globalEnabled;
    int globalBrightness;
    
    // Statistics
    unsigned long totalUptime;
    unsigned long lastUpdate;
    
public:
    LEDManager();
    
    // ===== INITIALIZATION =====
    void init(int ledBrightness = DEFAULT_LED_BRIGHTNESS, bool ledEnabled = true,
              int rgb1Brightness = 80, int rgb2Brightness = 80,
              bool rgb1Enabled = true, bool rgb2Enabled = true);
    bool isInitialized() const;
    void enable(bool enable = true);
    void disable();
    bool isEnabled() const;
    
    // ===== SINGLE LED CONTROL =====
    void setMode1Green(bool on, bool blink = false);
    void setMode1Red(bool on, bool blink = false);
    void setMode2Green(bool on, bool blink = false);
    void setMode2Red(bool on, bool blink = false);
    
    void toggleMode1Green();
    void toggleMode1Red();
    void toggleMode2Green();
    void toggleMode2Red();
    
    bool getMode1Green() const;
    bool getMode1Red() const;
    bool getMode2Green() const;
    bool getMode2Red() const;
    
    // ===== RGB LED CONTROL =====
    void setRGB1Color(uint8_t red, uint8_t green, uint8_t blue);
    void setRGB2Color(uint8_t red, uint8_t green, uint8_t blue);
    
    void setRGB1Color(uint32_t color);
    void setRGB2Color(uint32_t color);
    
    void setRGB1Brightness(int brightness);
    void setRGB2Brightness(int brightness);
    int getRGB1Brightness() const;
    int getRGB2Brightness() const;
    
    void enableRGB1(bool enable = true);
    void enableRGB2(bool enable = true);
    void disableRGB1();
    void disableRGB2();
    bool isRGB1Enabled() const;
    bool isRGB2Enabled() const;
    
    // ===== COLOR UTILITIES =====
    uint32_t colorFromPercent(float percent) const;
    uint32_t colorFromTemperature(float temp) const;
    uint32_t colorFromMood(byte mood) const;
    
    void calculateRGB2FromPercent(float percent);
    void setStatusColor(uint32_t color, bool rgb1 = true, bool rgb2 = true);
    
    // ===== BLINK CONTROL =====
    void setBlink(bool enable, unsigned long interval = 500);
    void setBlinkMode1Green(bool blink);
    void setBlinkMode1Red(bool blink);
    void setBlinkMode2Green(bool blink);
    void setBlinkMode2Red(bool blink);
    
    void setBlinkInterval(unsigned long interval);
    unsigned long getBlinkInterval() const;
    bool isBlinking() const;
    
    // ===== ANIMATION CONTROL =====
    void startAnimation(byte type, unsigned long duration = 1000, int cycles = 0);
    void stopAnimation();
    bool isAnimating() const;
    
    void startPulse(uint32_t color, unsigned long duration = 1000);
    void startRainbow(unsigned long duration = 5000);
    void startBreathing(uint32_t color, unsigned long duration = 2000);
    void startChase(uint32_t color, unsigned long speed = 100);
    void startColorCycle(unsigned long duration = 3000);
    
    // ===== PATTERNS =====
    void setPattern(byte pattern);
    void showConnectionPattern(bool connected);
    void showAlertPattern(bool severe);
    void showBatteryPattern(int percent);
    void showWiFiPattern(int strength);
    void showBootPattern();
    void showErrorPattern();
    void showSuccessPattern();
    
    // ===== SYSTEM STATES =====
    void update(const SystemState& systemState);
    void showSystemStatus(const SystemState& state);
    void showPortfolioStatus(float percentChange, byte mode = 0);
    void showConnectionStatus(bool connected, bool apMode = false);
    void showAlertStatus(bool active, bool severe = false, byte mode = 0);
    
    // ===== UPDATE LOOP =====
    void update();
    void updateBlink();
    void updateAnimation();
    void updateBrightness();
    
    // ===== PRESET FUNCTIONS =====
    void allOn();
    void allOff();
    void testSequence();
    void colorTest();
    
    void setSuccess();
    void setWarning();
    void setError();
    void setNeutral();
    void setActive();
    void setIdle();
    
    // ===== BRIGHTNESS CONTROL =====
    void setGlobalBrightness(int brightness);
    int getGlobalBrightness() const;
    void fadeToBrightness(int targetBrightness, unsigned long duration = 1000);
    
    // ===== POWER MANAGEMENT =====
    void powerSaveMode(bool enable);
    void setMaxPower(int milliamps);
    
    // ===== STATISTICS =====
    unsigned long getUptime() const;
    void resetStatistics();
    
    // ===== DEBUG FUNCTIONS =====
    void printState() const;
    void testAllLEDs();
    void measureCurrentConsumption();
    
private:
    // Internal helper functions
    void setupPins();
    void setupPWM();
    
    // LED control
    void writeSingleLED(int pin, bool state);
    void writeRGBLED(int redPin, int greenPin, int bluePin, 
                     uint8_t red, uint8_t green, uint8_t blue);
    
    void applyBrightness(uint8_t& red, uint8_t& green, uint8_t& blue, 
                        int brightness) const;
    void applyGammaCorrection(uint8_t& red, uint8_t& green, uint8_t& blue) const;
    
    // Animation helpers
    void updatePulseAnimation();
    void updateRainbowAnimation();
    void updateBreathingAnimation();
    void updateChaseAnimation();
    void updateColorCycleAnimation();
    
    float getAnimationProgress() const;
    uint32_t interpolateColor(uint32_t startColor, uint32_t endColor, 
                             float progress) const;
    
    // Color conversion
    void rgbToHSV(uint8_t r, uint8_t g, uint8_t b, 
                  float& h, float& s, float& v) const;
    void hsvToRGB(float h, float s, float v, 
                  uint8_t& r, uint8_t& g, uint8_t& b) const;
    
    uint32_t rgbToColor(uint8_t r, uint8_t g, uint8_t b) const;
    void colorToRGB(uint32_t color, uint8_t& r, uint8_t& g, uint8_t& b) const;
    
    // Pattern generators
    void generateConnectionPattern(bool connected);
    void generateAlertPattern(bool severe);
    void generateBatteryPattern(int percent);
    void generateWiFiPattern(int strength);
    
    // Safety
    bool validateBrightness(int brightness) const;
    bool validateColor(uint8_t r, uint8_t g, uint8_t b) const;
    void limitCurrent();
    
    // Debug
    void logLEDState(const char* operation) const;
};

// Inline functions
inline bool LEDManager::isInitialized() const {
    return initialized;
}

inline bool LEDManager::isEnabled() const {
    return globalEnabled;
}

inline bool LEDManager::getMode1Green() const {
    return state.mode1Green;
}

inline bool LEDManager::getMode1Red() const {
    return state.mode1Red;
}

inline bool LEDManager::getMode2Green() const {
    return state.mode2Green;
}

inline bool LEDManager::getMode2Red() const {
    return state.mode2Red;
}

inline int LEDManager::getRGB1Brightness() const {
    return state.rgb1.brightness;
}

inline int LEDManager::getRGB2Brightness() const {
    return state.rgb2.brightness;
}

inline bool LEDManager::isRGB1Enabled() const {
    return state.rgb1.enabled;
}

inline bool LEDManager::isRGB2Enabled() const {
    return state.rgb2.enabled;
}

inline bool LEDManager::isBlinking() const {
    return blink.enabled;
}

inline unsigned long LEDManager::getBlinkInterval() const {
    return blink.interval;
}

inline bool LEDManager::isAnimating() const {
    return animation.active;
}

inline int LEDManager::getGlobalBrightness() const {
    return globalBrightness;
}

inline unsigned long LEDManager::getUptime() const {
    return totalUptime;
}

// Color presets
namespace LEDColors {
    const uint32_t OFF       = 0x000000;
    const uint32_t WHITE     = 0xFFFFFF;
    const uint32_t RED       = 0xFF0000;
    const uint32_t GREEN     = 0x00FF00;
    const uint32_t BLUE      = 0x0000FF;
    const uint32_t YELLOW    = 0xFFFF00;
    const uint32_t CYAN      = 0x00FFFF;
    const uint32_t MAGENTA   = 0xFF00FF;
    const uint32_t ORANGE    = 0xFF8800;
    const uint32_t PURPLE    = 0x8800FF;
    const uint32_t PINK      = 0xFF0088;
    const uint32_t LIME      = 0x88FF00;
    const uint32_t TEAL      = 0x008888;
    const uint32_t NAVY      = 0x000088;
    const uint32_t MAROON    = 0x880000;
    const uint32_t OLIVE     = 0x888800;
    const uint32_t GRAY      = 0x888888;
    const uint32_t SILVER    = 0xCCCCCC;
    const uint32_t GOLD      = 0xFFD700;
    const uint32_t VIOLET    = 0x8B00FF;
    const uint32_t INDIGO    = 0x4B0082;
    const uint32_t TURQUOISE = 0x40E0D0;
    const uint32_t CORAL     = 0xFF7F50;
    const uint32_t SALMON    = 0xFA8072;
    const uint32_t CHOCOLATE = 0xD2691E;
    const uint32_t KHAKI     = 0xF0E68C;
}

#endif // LED_MANAGER_H