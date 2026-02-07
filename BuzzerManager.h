#ifndef BUZZER_MANAGER_H
#define BUZZER_MANAGER_H

#include <Arduino.h>
#include "SystemConfig.h"

class BuzzerManager {
private:
    // Hardware configuration
    int pin;
    int frequency;
    int volume;
    bool enabled;
    
    // Volume control
    int minVolume;
    int maxVolume;
    int volumeStep;
    
    // Tone sequences
    struct ToneSequence {
        int frequency;
        int duration;
        int pause;
    };
    
    // Current state
    bool isPlaying;
    unsigned long toneStartTime;
    int currentToneIndex;
    ToneSequence* currentSequence;
    int sequenceLength;
    
    // Statistics
    unsigned long totalPlayTime;
    int totalTonesPlayed;
    
public:
    BuzzerManager(int buzzerPin = BUZZER_PIN);
    ~BuzzerManager();
    
    // ===== INITIALIZATION =====
    void init(int initialVolume = DEFAULT_VOLUME, bool enable = true);
    void enable(bool enable = true);
    bool isEnabled() const;
    
    // ===== VOLUME CONTROL =====
    void setVolume(int level);
    int getVolume() const;
    void increaseVolume(int step = 10);
    void decreaseVolume(int step = 10);
    void mute();
    void unmute();
    bool isMuted() const;
    
    // ===== BASIC TONE PLAYBACK =====
    void playTone(int freq, int durationMs);
    void playToneVolume(int freq, int durationMs, int volumeLevel);
    void stopTone();
    bool isPlayingTone() const;
    
    // ===== ALERT TONES =====
    void playLongAlert(bool isSevere = false);
    void playShortAlert(bool isSevere = false);
    void playPortfolioAlert();
    void playExitAlert(bool isProfit = true);
    void playResetAlert();
    void playSuccessAlert();
    void playErrorAlert();
    void playConnectionLostAlert();
    void playStartupAlert();
    void playWarningAlert();
    void playCriticalAlert();
    
    // ===== SEQUENCES =====
    void playTestSequence();
    void playVolumeTest();
    void playConnectionSequence();
    void playBootSequence();
    void playShutdownSequence();
    void playMelody(const int* notes, const int* durations, int length);
    
    // ===== CUSTOM SEQUENCES =====
    void playSequence(const ToneSequence* sequence, int length);
    void stopSequence();
    void updateSequence(); // Call in loop to play sequences
    
    // ===== PRESET SEQUENCES =====
    void playSiren(int cycles = 3);
    void playBeepBeep(int count = 2);
    void playAscendingTones(int startFreq = 200, int endFreq = 1000, int step = 100);
    void playDescendingTones(int startFreq = 1000, int endFreq = 200, int step = 100);
    
    // ===== VOLUME EFFECTS =====
    void playVolumeFeedback();
    void playVolumeChanged();
    void playVolumeMax();
    void playVolumeMin();
    
    // ===== SYSTEM TONES =====
    void playSystemBeep(int type = 0);
    void playNotification(int priority = 0);
    void playConfirmation();
    void playDenial();
    void playAttention();
    
    // ===== STATISTICS =====
    unsigned long getTotalPlayTime() const;
    int getTotalTonesPlayed() const;
    void resetStatistics();
    
    // ===== DEBUG FUNCTIONS =====
    void testAllTones();
    void testFrequencyRange(int start = 100, int end = 5000, int step = 100);
    void testVolumeRange();
    void printStatus() const;
    
    // ===== UTILITY FUNCTIONS =====
    int frequencyToNote(int freq) const;
    int noteToFrequency(int note) const;
    String frequencyToString(int freq) const;
    
private:
    // Internal helper functions
    void setupPWM();
    void setPWM(int freq, int duty);
    void stopPWM();
    
    int calculateDutyCycle(int volumePercent) const;
    int calculateActualDuration(int durationMs) const;
    
    // Sequence management
    void startSequence(const ToneSequence* seq, int len);
    void nextToneInSequence();
    void endSequence();
    
    // Tone generation methods
    void playPulseTone(int freq, int duration, int pulseCount = 3);
    void playFadeTone(int freq, int duration, bool fadeIn = true);
    void playChord(int* frequencies, int count, int duration);
    
    // Alert tone generators
    void generateLongAlertTones(bool isSevere);
    void generateShortAlertTones(bool isSevere);
    void generatePortfolioAlertTones();
    void generateExitAlertTones(bool isProfit);
    
    // Preset sequences
    static const ToneSequence testSequence[];
    static const ToneSequence startupSequence[];
    static const ToneSequence successSequence[];
    static const ToneSequence errorSequence[];
    static const ToneSequence connectionSequence[];
    static const ToneSequence sirenSequence[];
    static const ToneSequence melody[];
    
    // Volume management
    int validateVolume(int vol) const;
    void applyVolume();
    
    // Safety
    bool validateFrequency(int freq) const;
    bool validateDuration(int duration) const;
    
    // Debug
    void logTonePlay(int freq, int duration, int volume);
};

// Inline functions
inline void BuzzerManager::enable(bool enable) {
    this->enabled = enable;
    if (!enable) stopTone();
}

inline bool BuzzerManager::isEnabled() const {
    return enabled;
}

inline int BuzzerManager::getVolume() const {
    return volume;
}

inline bool BuzzerManager::isMuted() const {
    return volume == 0 || !enabled;
}

inline bool BuzzerManager::isPlayingTone() const {
    return isPlaying;
}

inline void BuzzerManager::mute() {
    setVolume(0);
}

inline void BuzzerManager::unmute() {
    if (volume == 0) setVolume(DEFAULT_VOLUME);
}

inline unsigned long BuzzerManager::getTotalPlayTime() const {
    return totalPlayTime;
}

inline int BuzzerManager::getTotalTonesPlayed() const {
    return totalTonesPlayed;
}

// Tone sequences (examples)
const BuzzerManager::ToneSequence BuzzerManager::testSequence[] = {
    {523, 200, 100},  // C5
    {659, 200, 100},  // E5
    {784, 200, 100},  // G5
    {1047, 400, 200}  // C6
};

const BuzzerManager::ToneSequence BuzzerManager::startupSequence[] = {
    {600, 100, 50},
    {800, 150, 50},
    {1000, 200, 100},
    {1200, 150, 50},
    {1000, 100, 0}
};

#endif // BUZZER_MANAGER_H