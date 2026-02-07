#include "BuzzerManager.h"
#include "ConfigManager.h"
#include <Arduino.h>

// ===== CONSTRUCTOR/DESTRUCTOR =====
BuzzerManager::BuzzerManager(uint8_t pin) 
    : _pin(pin), 
      _volume(DEFAULT_VOLUME),
      _enabled(true),
      _isPlaying(false),
      _toneEndTime(0),
      _currentFrequency(0),
      _muted(false) {
}

BuzzerManager::~BuzzerManager() {
    if (_isPlaying) {
        noTone(_pin);
    }
}

// ===== INITIALIZATION =====
void BuzzerManager::begin() {
    Serial.print("Initializing Buzzer on pin ");
    Serial.println(_pin);
    
    pinMode(_pin, OUTPUT);
    digitalWrite(_pin, LOW);
    
    // Load settings
    _enabled = ConfigManager::getInstance().getBuzzerEnabled();
    _volume = ConfigManager::getInstance().getBuzzerVolume();
    
    Serial.print("Buzzer initialized: ");
    Serial.print(_enabled ? "ENABLED" : "DISABLED");
    Serial.print(", Volume: ");
    Serial.print(_volume);
    Serial.println("%");
    
    // Play startup tone if enabled
    if (_enabled && _volume > 0) {
        playStartupTone();
    }
}

void BuzzerManager::update() {
    if (_isPlaying && millis() >= _toneEndTime) {
        stopTone();
    }
}

// ===== VOLUME CONTROL =====
void BuzzerManager::setVolume(uint8_t volume) {
    _volume = constrain(volume, VOLUME_MIN, VOLUME_MAX);
    Serial.print("Buzzer volume set to: ");
    Serial.print(_volume);
    Serial.println("%");
    
    // Save to config
    ConfigManager::getInstance().setBuzzerVolume(_volume);
    
    // Provide feedback
    playVolumeFeedback();
}

void BuzzerManager::increaseVolume(uint8_t step) {
    uint8_t newVolume = _volume + step;
    if (newVolume > VOLUME_MAX) newVolume = VOLUME_MAX;
    setVolume(newVolume);
}

void BuzzerManager::decreaseVolume(uint8_t step) {
    uint8_t newVolume = _volume - step;
    if (newVolume < VOLUME_MIN) newVolume = VOLUME_MIN;
    setVolume(newVolume);
}

void BuzzerManager::toggleEnabled() {
    _enabled = !_enabled;
    ConfigManager::getInstance().setBuzzerEnabled(_enabled);
    
    Serial.print("Buzzer ");
    Serial.println(_enabled ? "enabled" : "disabled");
    
    if (_enabled && _volume > 0) {
        playVolumeFeedback();
    }
}

void BuzzerManager::mute() {
    _muted = true;
    if (_isPlaying) {
        stopTone();
    }
    Serial.println("Buzzer muted");
}

void BuzzerManager::unmute() {
    _muted = false;
    Serial.println("Buzzer unmuted");
}

// ===== TONE PLAYBACK =====
void BuzzerManager::playTone(uint16_t frequency, uint32_t duration) {
    if (!_enabled || _muted || _volume == 0 || _isPlaying) {
        return;
    }
    
    // Calculate actual duration based on volume
    uint32_t actualDuration = map(_volume, 0, 100, 0, duration);
    if (actualDuration == 0) return;
    
    // For low volumes, use pulsing
    if (_volume < 30) {
        playPulsedTone(frequency, actualDuration);
    } else {
        tone(_pin, frequency, actualDuration);
        _currentFrequency = frequency;
        _isPlaying = true;
        _toneEndTime = millis() + actualDuration + 10; // Add small buffer
    }
    
    Serial.print("Playing tone: ");
    Serial.print(frequency);
    Serial.print("Hz, ");
    Serial.print(actualDuration);
    Serial.print("ms (Vol: ");
    Serial.print(_volume);
    Serial.println("%)");
}

void BuzzerManager::playMelody(const uint16_t* frequencies, const uint16_t* durations, uint8_t count) {
    if (!_enabled || _muted || _volume == 0) return;
    
    Serial.println("Playing melody...");
    for (uint8_t i = 0; i < count; i++) {
        if (_muted) break; // Allow interruption
        
        playTone(frequencies[i], durations[i]);
        
        // Wait for tone to complete (with small buffer)
        unsigned long noteEnd = millis() + durations[i] + 20;
        while (millis() < noteEnd && !_muted) {
            delay(10);
        }
        
        // Small pause between notes
        if (i < count - 1) delay(30);
    }
}

void BuzzerManager::stopTone() {
    noTone(_pin);
    _isPlaying = false;
    _currentFrequency = 0;
}

// ===== ALERT TONES =====
void BuzzerManager::playAlert(bool isLong, bool isSevere) {
    if (!_enabled || _muted) return;
    
    Serial.print("Playing ");
    Serial.print(isLong ? "LONG" : "SHORT");
    Serial.print(" alert ");
    Serial.println(isSevere ? "(SEVERE)" : "(NORMAL)");
    
    if (isLong) {
        if (isSevere) {
            // Severe long alert: low descending tones
            playTone(440, 200);  // A4
            delay(250);
            playTone(349, 250);  // F4
            delay(300);
            playTone(294, 300);  // D4
        } else {
            // Normal long alert: single tone
            playTone(523, 300);  // C5
        }
    } else {
        if (isSevere) {
            // Severe short alert: rapid beeps
            for (int i = 0; i < 3; i++) {
                playTone(784, 100);  // G5
                delay(120);
            }
        } else {
            // Normal short alert: single tone
            playTone(659, 250);  // E5
        }
    }
}

void BuzzerManager::playExitAlert(bool isProfit) {
    if (!_enabled || _muted) return;
    
    Serial.print("Playing EXIT alert: ");
    Serial.println(isProfit ? "PROFIT" : "LOSS");
    
    if (isProfit) {
        // Profit: ascending tones
        playTone(523, 150);   // C5
        delay(200);
        playTone(659, 150);   // E5
        delay(200);
        playTone(784, 200);   // G5
    } else {
        // Loss: descending tones
        playTone(784, 150);   // G5
        delay(200);
        playTone(659, 150);   // E5
        delay(200);
        playTone(523, 200);   // C5
    }
}

void BuzzerManager::playPortfolioAlert() {
    if (!_enabled || _muted) return;
    
    Serial.println("Playing PORTFOLIO alert");
    
    // Portfolio alert: repeating pattern
    for (int i = 0; i < 3; i++) {
        playTone(587, 200);  // D5
        delay(250);
        playTone(494, 150);  // B4
        delay(200);
    }
}

void BuzzerManager::playConnectionAlert(bool connected) {
    if (!_enabled || _muted) return;
    
    Serial.print("Playing CONNECTION alert: ");
    Serial.println(connected ? "CONNECTED" : "DISCONNECTED");
    
    if (connected) {
        // Connection established: happy tones
        playTone(659, 150);   // E5
        delay(200);
        playTone(784, 150);   // G5
        delay(200);
        playTone(880, 200);   // A5
    } else {
        // Connection lost: sad tones
        playTone(880, 150);   // A5
        delay(200);
        playTone(784, 150);   // G5
        delay(200);
        playTone(659, 200);   // E5
    }
}

void BuzzerManager::playErrorAlert() {
    if (!_enabled || _muted) return;
    
    Serial.println("Playing ERROR alert");
    
    // Error: dissonant tones
    playTone(349, 200);   // F4
    delay(250);
    playTone(415, 200);   // G#4 (dissonant with F4)
    delay(250);
    playTone(349, 300);   // F4
}

void BuzzerManager::playSuccessAlert() {
    if (!_enabled || _muted) return;
    
    Serial.println("Playing SUCCESS alert");
    
    // Success: cheerful tones
    playTone(523, 150);   // C5
    delay(200);
    playTone(659, 150);   // E5
    delay(200);
    playTone(784, 200);   // G5
    delay(250);
    playTone(1047, 300);  // C6
}

void BuzzerManager::playStartupTone() {
    if (!_enabled || _muted) return;
    
    Serial.println("Playing STARTUP tone");
    
    // Startup: ascending arpeggio
    uint16_t startupFreqs[] = {523, 659, 784, 1047};  // C5, E5, G5, C6
    uint16_t startupDurs[] = {100, 100, 100, 200};
    
    for (int i = 0; i < 4; i++) {
        playTone(startupFreqs[i], startupDurs[i]);
        delay(120);
    }
}

void BuzzerManager::playShutdownTone() {
    if (!_enabled || _muted) return;
    
    Serial.println("Playing SHUTDOWN tone");
    
    // Shutdown: descending arpeggio
    uint16_t shutdownFreqs[] = {1047, 784, 659, 523};  // C6, G5, E5, C5
    uint16_t shutdownDurs[] = {100, 100, 100, 200};
    
    for (int i = 0; i < 4; i++) {
        playTone(shutdownFreqs[i], shutdownDurs[i]);
        delay(120);
    }
}

// ===== TEST FUNCTIONS =====
void BuzzerManager::testVolumeRange() {
    if (!_enabled || _muted) return;
    
    Serial.println("Testing volume range...");
    
    uint8_t originalVolume = _volume;
    
    // Test each 10% increment
    for (uint8_t vol = 10; vol <= 100; vol += 10) {
        setVolume(vol);
        Serial.print("Volume ");
        Serial.print(vol);
        Serial.println("%: Testing...");
        
        playTone(440, 200);  // A4
        delay(300);
        playTone(523, 200);  // C5
        delay(300);
        playTone(659, 200);  // E5
        delay(500);
    }
    
    // Restore original volume
    setVolume(originalVolume);
    Serial.println("Volume test complete");
}

void BuzzerManager::testAllAlerts() {
    if (!_enabled || _muted) return;
    
    Serial.println("Testing all alert types...");
    
    // Long normal alert
    Serial.println("1. Long normal alert");
    playAlert(true, false);
    delay(800);
    
    // Long severe alert
    Serial.println("2. Long severe alert");
    playAlert(true, true);
    delay(800);
    
    // Short normal alert
    Serial.println("3. Short normal alert");
    playAlert(false, false);
    delay(800);
    
    // Short severe alert
    Serial.println("4. Short severe alert");
    playAlert(false, true);
    delay(800);
    
    // Exit profit alert
    Serial.println("5. Exit profit alert");
    playExitAlert(true);
    delay(800);
    
    // Exit loss alert
    Serial.println("6. Exit loss alert");
    playExitAlert(false);
    delay(800);
    
    // Portfolio alert
    Serial.println("7. Portfolio alert");
    playPortfolioAlert();
    delay(800);
    
    // Success alert
    Serial.println("8. Success alert");
    playSuccessAlert();
    delay(800);
    
    // Error alert
    Serial.println("9. Error alert");
    playErrorAlert();
    delay(800);
    
    Serial.println("Alert test complete");
}

// ===== PRIVATE METHODS =====
void BuzzerManager::playVolumeFeedback() {
    if (!_enabled || _muted) return;
    
    // Frequency based on volume (higher volume = higher pitch)
    uint16_t freq = map(_volume, 0, 100, 300, 1500);
    
    // Duration based on volume
    uint16_t duration = map(_volume, 0, 100, 50, 200);
    
    playTone(freq, duration);
}

void BuzzerManager::playPulsedTone(uint16_t frequency, uint32_t totalDuration) {
    uint16_t pulseCount = totalDuration / 30;
    uint16_t pulseDuration = 20;
    uint16_t pauseDuration = 30 - pulseDuration;
    
    for (uint16_t i = 0; i < pulseCount; i++) {
        tone(_pin, frequency, pulseDuration);
        delay(pauseDuration);
    }
}

// ===== WEB INTERFACE HANDLERS =====
void BuzzerManager::handleWebVolumeSet(const String& volumeStr) {
    uint8_t volume = volumeStr.toInt();
    setVolume(volume);
}

void BuzzerManager::handleWebToggle() {
    toggleEnabled();
}

String BuzzerManager::getStatusJSON() {
    String json = "{";
    json += "\"enabled\":" + String(_enabled ? "true" : "false") + ",";
    json += "\"volume\":" + String(_volume) + ",";
    json += "\"muted\":" + String(_muted ? "true" : "false") + ",";
    json += "\"playing\":" + String(_isPlaying ? "true" : "false");
    if (_isPlaying) {
        json += ",\"frequency\":" + String(_currentFrequency);
    }
    json += "}";
    return json;
}

// ===== CONFIGURATION =====
void BuzzerManager::saveConfig() {
    // Configuration is saved automatically in setter methods
    Serial.println("Buzzer configuration saved");
}

void BuzzerManager::loadConfig() {
    // Configuration is loaded in begin() method
    Serial.println("Buzzer configuration loaded");
}