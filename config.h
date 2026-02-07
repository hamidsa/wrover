#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ===== PIN DEFINITIONS =====
// RGB LEDs (Common Cathode)
#define RGB1_RED    32
#define RGB1_GREEN  33
#define RGB1_BLUE   25

#define RGB2_RED    26
#define RGB2_GREEN  14
#define RGB2_BLUE   12

// Single Color LEDs
#define LED_MODE1_GREEN   27
#define LED_MODE1_RED     13
#define LED_MODE2_GREEN   21
#define LED_MODE2_RED     19

// Buzzer
#define BUZZER_PIN        22
#define RESET_BUTTON_PIN  0
#define TFT_BL_PIN        5
#define BATTERY_PIN       34

// ===== CONSTANTS =====
#define MAX_POSITIONS_PER_MODE 100
#define MAX_WIFI_NETWORKS 5
#define EEPROM_SIZE 4096
#define JSON_BUFFER_SIZE 8192
#define MAX_ALERT_HISTORY 50

// ===== TIMING CONSTANTS =====
#define DATA_UPDATE_INTERVAL 15000
#define DISPLAY_UPDATE_INTERVAL 2000
#define WIFI_CONNECT_TIMEOUT 20000
#define ALERT_AUTO_RETURN_TIME 8000
#define BATTERY_CHECK_INTERVAL 60000

// ===== ALERT THRESHOLDS =====
#define DEFAULT_ALERT_THRESHOLD -5.0
#define DEFAULT_SEVERE_THRESHOLD -10.0
#define PORTFOLIO_ALERT_THRESHOLD -7.0
#define DEFAULT_EXIT_ALERT_PERCENT 3.0

// ===== POWER SOURCE =====
enum PowerSource {
    POWER_SOURCE_USB = 0,
    POWER_SOURCE_BATTERY = 1,
    POWER_SOURCE_EXTERNAL = 2
};

#endif