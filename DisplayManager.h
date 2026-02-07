#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <TFT_eSPI.h>
#include <SPI.h>
#include <Arduino.h>
#include "SystemConfig.h"

// Forward declarations
class CryptoData;
struct SystemState;

class DisplayManager {
private:
    TFT_eSPI tft;
    bool initialized;
    bool backlightOn;
    unsigned long lastInteraction;
    unsigned long lastBlinkTime;
    bool blinkState;
    
    // Display settings
    int brightness;
    int timeout;
    uint8_t rotation;
    bool showDetails;
    bool invertColors;
    
    // Current state
    DisplayMode currentMode;
    unsigned long modeStartTime;
    String currentTitle;
    String currentMessage;
    
    // Page management
    int currentPage;
    int totalPages;
    bool pageChanged;
    
    // Font pointers (if using custom fonts)
    uint8_t* fontSmall;
    uint8_t* fontMedium;
    uint8_t* fontLarge;
    
    // Color scheme
    struct ColorScheme {
        uint16_t background;
        uint16_t text;
        uint16_t accent;
        uint16_t positive;
        uint16_t negative;
        uint16_t warning;
        uint16_t info;
        uint16_t header;
        uint16_t border;
    } colors;
    
public:
    DisplayManager();
    ~DisplayManager();
    
    // ===== INITIALIZATION =====
    bool init(int brightness = 100, uint8_t rotation = 0);
    bool isInitialized() const;
    void setBrightness(int level);
    void setRotation(uint8_t rot);
    void setInvertColors(bool invert);
    void setShowDetails(bool show);
    void setTimeout(int milliseconds);
    
    // ===== BASIC OPERATIONS =====
    void clear();
    void clearArea(int x, int y, int width, int height);
    void setBacklight(bool on);
    void toggleBacklight();
    void setColorScheme(uint16_t bg, uint16_t text, uint16_t accent);
    void setDefaultColorScheme();
    void setDarkColorScheme();
    void setLightColorScheme();
    
    // ===== TEXT RENDERING =====
    void setTextFont(uint8_t size);
    void printText(int x, int y, const String& text, 
                   uint16_t color = TFT_WHITE, uint16_t bgColor = TFT_BLACK);
    void printCentered(int y, const String& text, 
                       uint16_t color = TFT_WHITE, uint16_t bgColor = TFT_BLACK);
    void printRightAligned(int x, int y, const String& text,
                           uint16_t color = TFT_WHITE, uint16_t bgColor = TFT_BLACK);
    void printWithShadow(int x, int y, const String& text, 
                        uint16_t color, uint16_t shadowColor = TFT_BLACK);
    
    // ===== GRAPHICS DRAWING =====
    void drawRect(int x, int y, int width, int height, 
                  uint16_t color, bool filled = false, int thickness = 1);
    void drawRoundRect(int x, int y, int width, int height, int radius,
                       uint16_t color, bool filled = false);
    void drawCircle(int x, int y, int radius, 
                   uint16_t color, bool filled = false);
    void drawLine(int x1, int y1, int x2, int y2, uint16_t color);
    void drawPixel(int x, int y, uint16_t color);
    
    // ===== UI COMPONENTS =====
    void drawProgressBar(int x, int y, int width, int height, 
                        float percentage, uint16_t color, uint16_t bgColor = TFT_DARKGREY);
    void drawBatteryIcon(int x, int y, int percent, bool charging = false);
    void drawWiFiIcon(int x, int y, int signalStrength, bool connected);
    void drawSignalBars(int x, int y, int bars, uint16_t color);
    void drawAlertIcon(int x, int y, bool severe = false);
    void drawClock(int x, int y, const String& time);
    void drawSeparator(int y, int margin = 10);
    void drawHeader(const String& title, bool showIcons = true);
    void drawFooter(const String& status);
    
    // ===== SCREEN MANAGEMENT =====
    void showSplashScreen();
    void showMainScreen(const SystemState& state, const CryptoData& data);
    void showAlertScreen(const String& title, const String& symbol, 
                        const String& message, float price, 
                        bool isSevere, byte mode);
    void showConnectionScreen(const String& ssid, const String& status, 
                             int progress = -1);
    void showErrorScreen(const String& title, const String& message);
    void showSetupScreen();
    void showMessage(const String& line1, const String& line2 = "", 
                    const String& line3 = "", const String& line4 = "");
    void showWarning(const String& title, const String& message);
    void showSuccess(const String& title, const String& message);
    
    // ===== UPDATE FUNCTIONS =====
    void update();
    void updateMainScreen(const SystemState& state, const CryptoData& data);
    void updateAlertScreen(unsigned long displayStartTime);
    void updateConnectionScreen(const String& status, int progress);
    
    // ===== DATA DISPLAY FUNCTIONS =====
    void displayPortfolioSummary(const PortfolioSummary& summary, 
                                int startY, uint16_t color);
    void displayPositionList(const CryptoPosition* positions, int count, 
                            int startY, int itemsPerPage);
    void displaySinglePosition(const CryptoPosition& position, 
                              int x, int y, int width);
    void displayAlertHistory(const AlertHistory* history, int count, 
                            int startY, int itemsPerPage);
    void displaySystemInfo(const SystemState& state, int startY);
    
    // ===== PAGE MANAGEMENT =====
    void nextPage();
    void previousPage();
    void setPage(int page);
    int getCurrentPage() const;
    int getTotalPages() const;
    void calculateTotalPages(int itemCount, int itemsPerPage);
    
    // ===== INTERACTION MANAGEMENT =====
    void recordInteraction();
    bool shouldDim() const;
    bool shouldTurnOff() const;
    void handleAutoDim();
    
    // ===== MODE MANAGEMENT =====
    void setMode(DisplayMode mode);
    DisplayMode getMode() const;
    bool isModeExpired() const;
    void resetModeTimer();
    
    // ===== UTILITY FUNCTIONS =====
    String formatNumber(float number, int decimals = 2) const;
    String formatPercent(float percent) const;
    String formatPrice(float price) const;
    String truncateText(const String& text, int maxLength) const;
    String getShortSymbol(const String& symbol) const;
    
    // ===== ANIMATION EFFECTS =====
    void fadeIn();
    void fadeOut();
    void blinkText(int x, int y, const String& text, uint16_t color);
    void drawLoadingAnimation(int x, int y, int size);
    void drawProgressDots(int x, int y, int count, int activeIndex);
    
    // ===== DEBUG FUNCTIONS =====
    void testPattern();
    void showTestScreen();
    void drawColorBars();
    void printMemoryUsage();
    
private:
    // Internal helper functions
    void initColors();
    void initFonts();
    void applyRotation();
    
    // Main screen components
    void drawMainHeader(const SystemState& state);
    void drawMainContent(const SystemState& state, const CryptoData& data);
    void drawMainFooter(const SystemState& state);
    
    void drawEntrySection(int y, const PortfolioSummary& summary, 
                         const SystemState& state);
    void drawExitSection(int y, const PortfolioSummary& summary, 
                        const SystemState& state);
    void drawTotalSection(int y, const PortfolioSummary& entry, 
                         const PortfolioSummary& exit, const SystemState& state);
    void drawStatusBar(const SystemState& state);
    
    // Alert screen components
    void drawAlertHeader(const String& title, bool isSevere);
    void drawAlertContent(const String& symbol, const String& message, 
                         float price, byte mode);
    void drawAlertFooter(unsigned long displayStartTime);
    
    // Connection screen components
    void drawConnectionHeader();
    void drawConnectionContent(const String& ssid, const String& status, 
                              int progress);
    
    // Utility drawing functions
    void drawGradient(int x, int y, int width, int height, 
                     uint16_t startColor, uint16_t endColor, bool vertical = true);
    void drawGlow(int x, int y, int radius, uint16_t color, int intensity = 10);
    void drawShadow(int x, int y, int width, int height, int offset = 2);
    
    // Text measurement
    int getTextWidth(const String& text, uint8_t font = 1) const;
    int getTextHeight(const String& text, uint8_t font = 1) const;
    void wrapText(const String& text, int maxWidth, 
                  std::vector<String>& lines) const;
    
    // Color manipulation
    uint16_t blendColors(uint16_t color1, uint16_t color2, float ratio) const;
    uint16_t darkenColor(uint16_t color, float factor) const;
    uint16_t lightenColor(uint16_t color, float factor) const;
    uint16_t alphaBlend(uint16_t foreground, uint16_t background, float alpha) const;
    
    // Animation helpers
    void updateBlinkState();
    float getFadeAlpha() const;
    
    // Memory management
    void* allocateFontMemory(size_t size);
    void freeFontMemory();
};

// Color definitions for easy access
namespace DisplayColors {
    const uint16_t BLACK        = TFT_BLACK;
    const uint16_t WHITE        = TFT_WHITE;
    const uint16_t RED          = TFT_RED;
    const uint16_t GREEN        = TFT_GREEN;
    const uint16_t BLUE         = TFT_BLUE;
    const uint16_t CYAN         = TFT_CYAN;
    const uint16_t MAGENTA      = TFT_MAGENTA;
    const uint16_t YELLOW       = TFT_YELLOW;
    const uint16_t ORANGE       = TFT_ORANGE;
    const uint16_t PURPLE       = TFT_PURPLE;
    const uint16_t PINK         = TFT_PINK;
    const uint16_t BROWN        = TFT_BROWN;
    const uint16_t GOLD         = TFT_GOLD;
    const uint16_t SILVER       = TFT_SILVER;
    
    // Custom colors
    const uint16_t DARK_GREY    = tft.color565(64, 64, 64);
    const uint16_t MEDIUM_GREY  = tft.color565(128, 128, 128);
    const uint16_t LIGHT_GREY   = tft.color565(192, 192, 192);
    const uint16_t DARK_BLUE    = tft.color565(0, 0, 128);
    const uint16_t DARK_GREEN   = tft.color565(0, 128, 0);
    const uint16_t DARK_RED     = tft.color565(128, 0, 0);
    const uint16_t LIGHT_BLUE   = tft.color565(173, 216, 230);
    const uint16_t LIGHT_GREEN  = tft.color565(144, 238, 144);
    const uint16_t LIGHT_RED    = tft.color565(255, 182, 193);
    const uint16_t TEAL         = tft.color565(0, 128, 128);
    const uint16_t NAVY         = tft.color565(0, 0, 128);
    const uint16_t MAROON       = tft.color565(128, 0, 0);
    const uint16_t OLIVE        = tft.color565(128, 128, 0);
    const uint16_t LIME         = tft.color565(0, 255, 0);
    const uint16_t AQUA         = tft.color565(0, 255, 255);
    const uint16_t FUCHSIA      = tft.color565(255, 0, 255);
    
    // Alert colors
    const uint16_t ALERT_RED    = tft.color565(255, 50, 50);
    const uint16_t ALERT_GREEN  = tft.color565(50, 255, 50);
    const uint16_t ALERT_YELLOW = tft.color565(255, 255, 50);
    const uint16_t ALERT_ORANGE = tft.color565(255, 165, 0);
    
    // Portfolio colors
    const uint16_t PROFIT_GREEN = tft.color565(0, 200, 0);
    const uint16_t LOSS_RED     = tft.color565(200, 0, 0);
    const uint16_t NEUTRAL_BLUE = tft.color565(0, 120, 255);
    
    // Status colors
    const uint16_t CONNECTED    = tft.color565(0, 200, 0);
    const uint16_t DISCONNECTED = tft.color565(200, 0, 0);
    const uint16_t CONNECTING   = tft.color565(255, 165, 0);
    const uint16_t AP_MODE      = tft.color565(255, 255, 0);
    
    // Battery colors
    const uint16_t BATTERY_FULL    = GREEN;
    const uint16_t BATTERY_MEDIUM  = YELLOW;
    const uint16_t BATTERY_LOW     = ORANGE;
    const uint16_t BATTERY_CRITICAL = RED;
    const uint16_t BATTERY_CHARGING = CYAN;
}

#endif // DISPLAY_MANAGER_H