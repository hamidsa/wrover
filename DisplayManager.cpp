#include "DisplayManager.h"
#include "SystemConfig.h"
#include "CryptoData.h"
#include <SPI.h>

DisplayManager::DisplayManager() : 
    initialized(false), 
    backlightOn(true), 
    lastInteraction(millis()),
    lastBlinkTime(0),
    blinkState(false),
    brightness(100),
    timeout(30000),
    rotation(0),
    showDetails(true),
    invertColors(false),
    currentMode(DISPLAY_MODE_SPLASH),
    modeStartTime(millis()),
    currentPage(0),
    totalPages(1),
    pageChanged(false),
    fontSmall(nullptr),
    fontMedium(nullptr),
    fontLarge(nullptr) {
    
    initColors();
}

DisplayManager::~DisplayManager() {
    freeFontMemory();
}

bool DisplayManager::init(int brightnessLevel, uint8_t rot) {
    if (initialized) return true;
    
    Serial.println("Initializing ST7789 240x240 IPS Display...");
    
    // Initialize backlight pin
    pinMode(TFT_BL_PIN, OUTPUT);
    digitalWrite(TFT_BL_PIN, HIGH);
    delay(100);
    
    // Initialize TFT
    tft.init();
    tft.setRotation(rot);
    rotation = rot;
    
    // Set brightness
    setBrightness(brightnessLevel);
    
    // Set default colors
    setDefaultColorScheme();
    
    // Clear screen
    clear();
    
    // Initialize fonts
    initFonts();
    
    initialized = true;
    
    Serial.println("Display initialized successfully");
    Serial.println("  Rotation: " + String(rotation));
    Serial.println("  Brightness: " + String(brightness) + "%");
    Serial.println("  Timeout: " + String(timeout) + "ms");
    
    return true;
}

bool DisplayManager::isInitialized() const {
    return initialized;
}

void DisplayManager::setBrightness(int level) {
    brightness = constrain(level, 0, 100);
    // Note: TFT backlight control would go here
    // For now, we just track the value
}

void DisplayManager::setRotation(uint8_t rot) {
    rotation = rot % 4;
    if (initialized) {
        tft.setRotation(rotation);
    }
}

void DisplayManager::clear() {
    if (!initialized) return;
    
    tft.fillScreen(colors.background);
}

void DisplayManager::setBacklight(bool on) {
    backlightOn = on;
    digitalWrite(TFT_BL_PIN, on ? HIGH : LOW);
}

void DisplayManager::setDefaultColorScheme() {
    colors.background = DisplayColors::BLACK;
    colors.text = DisplayColors::WHITE;
    colors.accent = DisplayColors::CYAN;
    colors.positive = DisplayColors::PROFIT_GREEN;
    colors.negative = DisplayColors::LOSS_RED;
    colors.warning = DisplayColors::ALERT_ORANGE;
    colors.info = DisplayColors::LIGHT_BLUE;
    colors.header = DisplayColors::DARK_BLUE;
    colors.border = DisplayColors::MEDIUM_GREY;
    
    if (initialized) {
        tft.setTextColor(colors.text, colors.background);
    }
}

void DisplayManager::printText(int x, int y, const String& text, 
                              uint16_t color, uint16_t bgColor) {
    if (!initialized || text.length() == 0) return;
    
    tft.setTextColor(color, bgColor);
    tft.setCursor(x, y);
    tft.print(text);
}

void DisplayManager::printCentered(int y, const String& text, 
                                  uint16_t color, uint16_t bgColor) {
    if (!initialized) return;
    
    int textWidth = getTextWidth(text);
    int x = (DISPLAY_WIDTH - textWidth) / 2;
    
    printText(x, y, text, color, bgColor);
}

void DisplayManager::drawProgressBar(int x, int y, int width, int height, 
                                    float percentage, uint16_t color, uint16_t bgColor) {
    if (!initialized) return;
    
    percentage = constrain(percentage, 0.0, 100.0);
    
    // Draw background
    tft.fillRect(x, y, width, height, bgColor);
    tft.drawRect(x, y, width, height, colors.border);
    
    // Calculate fill width
    int fillWidth = (width - 2) * (percentage / 100.0);
    fillWidth = constrain(fillWidth, 0, width - 2);
    
    // Draw fill
    if (fillWidth > 0) {
        tft.fillRect(x + 1, y + 1, fillWidth, height - 2, color);
    }
}

void DisplayManager::drawBatteryIcon(int x, int y, int percent, bool charging) {
    if (!initialized) return;
    
    percent = constrain(percent, 0, 100);
    
    // Battery body
    tft.drawRect(x, y, 30, 15, colors.text);
    tft.drawRect(x + 30, y + 4, 3, 7, colors.text);
    
    // Calculate fill
    int fillWidth = (28 * percent) / 100;
    fillWidth = constrain(fillWidth, 0, 28);
    
    // Choose color based on percentage
    uint16_t fillColor;
    if (charging) {
        fillColor = DisplayColors::BATTERY_CHARGING;
    } else if (percent > 50) {
        fillColor = DisplayColors::BATTERY_FULL;
    } else if (percent > 20) {
        fillColor = DisplayColors::BATTERY_MEDIUM;
    } else {
        fillColor = DisplayColors::BATTERY_CRITICAL;
    }
    
    // Draw fill
    if (fillWidth > 0) {
        tft.fillRect(x + 1, y + 1, fillWidth, 13, fillColor);
    }
    
    // Draw percentage text
    if (showDetails) {
        tft.setTextColor(colors.text, colors.background);
        tft.setCursor(x + 35, y + 4);
        tft.print(String(percent) + "%");
    }
}

void DisplayManager::showSplashScreen() {
    if (!initialized) return;
    
    clear();
    
    // Draw border
    tft.drawRect(0, 0, DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 1, colors.accent);
    tft.drawRect(1, 1, DISPLAY_WIDTH - 3, DISPLAY_HEIGHT - 3, colors.info);
    
    // Title
    tft.setTextColor(colors.accent, colors.background);
    tft.setTextSize(3);
    printCentered(40, "PORTFOLIO");
    printCentered(80, "MONITOR");
    
    // Version
    tft.setTextColor(colors.info, colors.background);
    tft.setTextSize(2);
    printCentered(120, "v4.5.3");
    
    // Hardware info
    tft.setTextSize(1);
    printCentered(150, "ESP32-WROVER-E");
    printCentered(170, "240x240 IPS + RGB LEDs");
    
    // Loading animation
    for (int i = 0; i < DISPLAY_WIDTH; i += 10) {
        tft.drawFastHLine(20, 200, i, colors.accent);
        delay(10);
    }
    
    delay(1500);
    setMode(DISPLAY_MODE_MAIN);
}

void DisplayManager::showMainScreen(const SystemState& state, const CryptoData& data) {
    if (!initialized || currentMode != DISPLAY_MODE_MAIN) return;
    
    clear();
    
    // Draw header
    drawMainHeader(state);
    
    // Draw separator
    drawSeparator(75);
    
    // Get portfolio summaries
    const PortfolioSummary* entrySummary = data.getSummary(0);
    const PortfolioSummary* exitSummary = data.getSummary(1);
    
    // Draw entry section
    drawEntrySection(90, *entrySummary, state);
    
    // Draw separator
    drawSeparator(130);
    
    // Draw exit section
    drawExitSection(140, *exitSummary, state);
    
    // Draw total section
    float totalValue = entrySummary->totalCurrentValue + exitSummary->totalCurrentValue;
    float totalInvestment = entrySummary->totalInvestment + exitSummary->totalInvestment;
    float totalPnLPercent = 0;
    
    if (totalInvestment > 0) {
        totalPnLPercent = ((totalValue - totalInvestment) / totalInvestment) * 100;
    }
    
    tft.setTextColor(colors.accent, colors.background);
    tft.setTextSize(1);
    tft.setCursor(5, 180);
    tft.print("TOTAL:");
    
    tft.setTextColor(colors.text, colors.background);
    tft.setCursor(60, 180);
    tft.print("$" + formatNumber(totalValue));
    
    tft.setTextColor(totalPnLPercent >= 0 ? colors.positive : colors.negative, 
                     colors.background);
    tft.setCursor(150, 180);
    tft.print(formatPercent(totalPnLPercent));
    
    // Draw status bar
    drawStatusBar(state);
    
    // Record interaction for auto-dim
    recordInteraction();
}

void DisplayManager::showAlertScreen(const String& title, const String& symbol, 
                                    const String& message, float price, 
                                    bool isSevere, byte mode) {
    if (!initialized) return;
    
    setMode(DISPLAY_MODE_ALERT);
    clear();
    
    // Set background color based on severity
    uint16_t bgColor = isSevere ? DisplayColors::ALERT_RED : DisplayColors::ALERT_ORANGE;
    tft.fillRect(0, 0, DISPLAY_WIDTH, 50, bgColor);
    
    // Draw title
    tft.setTextColor(DisplayColors::WHITE, bgColor);
    tft.setTextSize(3);
    printCentered(10, title);
    
    // Draw symbol
    tft.setTextColor(DisplayColors::YELLOW, colors.background);
    tft.setTextSize(4);
    printCentered(70, symbol);
    
    // Draw price
    tft.setTextSize(3);
    tft.setCursor(30, 120);
    tft.print("$" + formatPrice(price));
    
    // Draw message
    tft.setTextColor(colors.text, colors.background);
    tft.setTextSize(2);
    printCentered(160, message);
    
    // Draw mode indicator
    tft.setTextColor(mode == 0 ? colors.positive : colors.warning, colors.background);
    tft.setTextSize(1);
    tft.setCursor(5, 220);
    tft.print(mode == 0 ? "ENTRY MODE" : "EXIT MODE");
    
    // Draw auto-return countdown
    tft.setTextColor(colors.info, colors.background);
    tft.setCursor(150, 220);
    tft.print("Auto-close: 8s");
    
    modeStartTime = millis();
}

void DisplayManager::update() {
    if (!initialized) return;
    
    updateBlink();
    handleAutoDim();
    
    // Check if alert screen should auto-return
    if (currentMode == DISPLAY_MODE_ALERT && isModeExpired()) {
        setMode(DISPLAY_MODE_MAIN);
    }
}

void DisplayManager::updateMainScreen(const SystemState& state, const CryptoData& data) {
    if (currentMode == DISPLAY_MODE_MAIN) {
        showMainScreen(state, data);
    }
}

void DisplayManager::drawMainHeader(const SystemState& state) {
    // Title
    tft.setTextColor(colors.accent, colors.background);
    tft.setTextSize(2);
    tft.setCursor(5, 5);
    tft.print("PORTFOLIO");
    
    // WiFi status
    tft.setTextSize(1);
    tft.setCursor(5, 35);
    tft.print("WiFi:");
    
    if (state.isConnectedToWiFi) {
        tft.setTextColor(colors.positive, colors.background);
        String ssid = state.currentSSID.length() > 0 ? state.currentSSID : "Connected";
        if (ssid.length() > 12) {
            ssid = ssid.substring(0, 12) + "...";
        }
        tft.setCursor(35, 35);
        tft.print(ssid);
    } else if (state.apModeActive) {
        tft.setTextColor(colors.warning, colors.background);
        tft.setCursor(35, 35);
        tft.print("AP Mode");
    } else {
        tft.setTextColor(colors.negative, colors.background);
        tft.setCursor(35, 35);
        tft.print("No WiFi");
    }
    
    // Time
    tft.setTextColor(colors.info, colors.background);
    tft.setCursor(5, 55);
    tft.print("Time:");
    
    if (state.currentDateTime.length() > 10) {
        tft.setCursor(35, 55);
        tft.print(state.currentDateTime.substring(11, 19));
    } else {
        tft.setCursor(35, 55);
        tft.print("--:--:--");
    }
}

void DisplayManager::drawEntrySection(int y, const PortfolioSummary& summary, 
                                     const SystemState& state) {
    tft.setTextColor(colors.positive, colors.background);
    tft.setTextSize(1);
    tft.setCursor(5, y);
    tft.print("ENTRY:");
    
    tft.setTextColor(colors.text, colors.background);
    tft.setCursor(60, y);
    tft.print(String(summary.totalPositions) + " pos");
    
    tft.setTextColor(summary.totalPnlPercent >= 0 ? colors.positive : colors.negative, 
                     colors.background);
    tft.setCursor(120, y);
    tft.print(formatPercent(summary.totalPnlPercent));
    
    if (showDetails) {
        tft.setTextColor(colors.info, colors.background);
        tft.setCursor(5, y + 15);
        tft.print("Val: $" + formatNumber(summary.totalCurrentValue));
    }
}

void DisplayManager::drawExitSection(int y, const PortfolioSummary& summary, 
                                    const SystemState& state) {
    tft.setTextColor(colors.warning, colors.background);
    tft.setTextSize(1);
    tft.setCursor(5, y);
    tft.print("EXIT:");
    
    tft.setTextColor(colors.text, colors.background);
    tft.setCursor(60, y);
    tft.print(String(summary.totalPositions) + " pos");
    
    tft.setTextColor(summary.totalPnlPercent >= 0 ? colors.positive : colors.negative, 
                     colors.background);
    tft.setCursor(120, y);
    tft.print(formatPercent(summary.totalPnlPercent));
    
    if (showDetails) {
        tft.setTextColor(colors.info, colors.background);
        tft.setCursor(5, y + 15);
        tft.print("Val: $" + formatNumber(summary.totalCurrentValue));
    }
}

void DisplayManager::drawStatusBar(const SystemState& state) {
    tft.drawFastHLine(0, 200, DISPLAY_WIDTH, colors.border);
    
    // Alert status
    if (state.mode1GreenActive || state.mode1RedActive || 
        state.mode2GreenActive || state.mode2RedActive) {
        tft.setTextColor(colors.warning, colors.background);
        tft.setCursor(5, 210);
        tft.print("ALERT!");
    } else if (state.connectionLost) {
        tft.setTextColor(colors.negative, colors.background);
        tft.setCursor(5, 210);
        tft.print("NO CONN");
    } else {
        tft.setTextColor(colors.positive, colors.background);
        tft.setCursor(5, 210);
        tft.print("READY");
    }
    
    // Power source
    if (state.powerSource == POWER_SOURCE_USB) {
        tft.setTextColor(colors.info, colors.background);
        tft.setCursor(60, 210);
        tft.print("USB");
    } else if (state.showBattery) {
        drawBatteryIcon(60, 210, state.batteryPercent, false);
    }
    
    // Volume
    tft.setTextColor(DisplayColors::MAGENTA, colors.background);
    tft.setCursor(120, 210);
    tft.print("Vol:" + String(state.buzzerVolume) + "%");
    
    // Connection type
    if (state.apModeActive) {
        tft.setTextColor(colors.warning, colors.background);
        tft.setCursor(180, 210);
        tft.print("AP");
    } else if (state.isConnectedToWiFi) {
        tft.setTextColor(colors.positive, colors.background);
        tft.setCursor(180, 210);
        tft.print("WiFi");
    } else {
        tft.setTextColor(colors.negative, colors.background);
        tft.setCursor(180, 210);
        tft.print("OFF");
    }
}

void DisplayManager::drawSeparator(int y) {
    tft.drawFastHLine(0, y, DISPLAY_WIDTH, colors.border);
}

String DisplayManager::formatNumber(float number, int decimals) {
    if (number == 0) return "0";
    
    float absNumber = fabs(number);
    
    if (absNumber >= 1000000) {
        return String(number / 1000000, decimals) + "M";
    } else if (absNumber >= 10000) {
        return String(number / 1000, 1) + "K";
    } else if (absNumber >= 1000) {
        return String(number / 1000, 2) + "K";
    } else if (absNumber >= 1) {
        return String(number, decimals);
    } else if (absNumber >= 0.01) {
        return String(number, 4);
    } else if (absNumber >= 0.0001) {
        return String(number, 6);
    } else {
        return String(number, 8);
    }
}

String DisplayManager::formatPercent(float percent) {
    if (percent > 0) {
        return "+" + String(percent, 2) + "%";
    } else if (percent < 0) {
        return String(percent, 2) + "%";
    } else {
        return "0.00%";
    }
}

String DisplayManager::formatPrice(float price) {
    if (price <= 0) return "0.00";
    
    if (price >= 1000) {
        return String(price, 2);
    } else if (price >= 1) {
        return String(price, 4);
    } else if (price >= 0.01) {
        return String(price, 6);
    } else if (price >= 0.0001) {
        return String(price, 8);
    } else {
        return String(price, 10);
    }
}

void DisplayManager::recordInteraction() {
    lastInteraction = millis();
}

bool DisplayManager::shouldDim() const {
    if (timeout == 0) return false;
    return (millis() - lastInteraction) > (timeout / 2);
}

bool DisplayManager::shouldTurnOff() const {
    if (timeout == 0) return false;
    return (millis() - lastInteraction) > timeout;
}

void DisplayManager::handleAutoDim() {
    if (shouldTurnOff()) {
        setBacklight(false);
    } else if (shouldDim()) {
        // Could implement dimming here
        // For now, just ensure backlight is on
        if (!backlightOn) {
            setBacklight(true);
        }
    }
}

void DisplayManager::setMode(DisplayMode mode) {
    if (currentMode != mode) {
        currentMode = mode;
        modeStartTime = millis();
        pageChanged = true;
    }
}

DisplayMode DisplayManager::getMode() const {
    return currentMode;
}

bool DisplayManager::isModeExpired() const {
    if (currentMode == DISPLAY_MODE_ALERT) {
        return (millis() - modeStartTime) > ALERT_DISPLAY_TIME;
    }
    return false;
}

void DisplayManager::showMessage(const String& line1, const String& line2, 
                                const String& line3, const String& line4) {
    if (!initialized) return;
    
    clear();
    
    tft.setTextColor(colors.text, colors.background);
    tft.setTextSize(2);
    
    if (line1.length() > 0) {
        printCentered(40, line1);
    }
    
    if (line2.length() > 0) {
        printCentered(80, line2);
    }
    
    tft.setTextSize(1);
    
    if (line3.length() > 0) {
        printCentered(130, line3);
    }
    
    if (line4.length() > 0) {
        printCentered(150, line4);
    }
    
    recordInteraction();
}

void DisplayManager::showWarning(const String& title, const String& message) {
    if (!initialized) return;
    
    clear();
    
    // Warning background
    tft.fillRect(0, 0, DISPLAY_WIDTH, 50, DisplayColors::ALERT_ORANGE);
    
    // Title
    tft.setTextColor(DisplayColors::WHITE, DisplayColors::ALERT_ORANGE);
    tft.setTextSize(3);
    printCentered(10, title);
    
    // Message
    tft.setTextColor(colors.text, colors.background);
    tft.setTextSize(2);
    printCentered(100, message);
    
    // Icon
    tft.setTextSize(4);
    printCentered(160, "⚠️");
    
    modeStartTime = millis();
    setMode(DISPLAY_MODE_ALERT);
}

void DisplayManager::initColors() {
    // Already initialized in constructor
}

void DisplayManager::initFonts() {
    // Load custom fonts if available
    // For now, using built-in fonts
}

void DisplayManager::freeFontMemory() {
    // Free any allocated font memory
}

void DisplayManager::updateBlink() {
    unsigned long now = millis();
    if (now - lastBlinkTime > 500) {
        lastBlinkTime = now;
        blinkState = !blinkState;
    }
}

int DisplayManager::getTextWidth(const String& text) const {
    // Simple estimation - in real implementation, use font metrics
    return text.length() * 6 * 1; // 6 pixels per char * text size
}