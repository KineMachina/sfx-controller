#ifndef DEMO_CONTROLLER_H
#define DEMO_CONTROLLER_H

#include "Arduino.h"
#include "AudioController.h"
#include "StripLEDController.h"
#include "MatrixLEDController.h"
#include "SD.h"
#include <FreeRTOS.h>
#include <task.h>

// Forward declaration
class SettingsController;

/**
 * DemoController - Manages demo mode with configurable playback modes
 * Supports audio-only, LED-only, effects, or combined audio+LED modes
 * Can play randomly or sequentially
 */
class DemoController {
public:
    enum DemoMode {
        DEMO_AUDIO_LED,    // Audio + LED (original behavior)
        DEMO_AUDIO_ONLY,   // Audio files only
        DEMO_LED_ONLY,     // LED effects only
        DEMO_EFFECTS       // Configured effects (from config.json)
    };
    
    enum PlaybackOrder {
        ORDER_RANDOM,      // Random selection
        ORDER_SEQUENTIAL   // Sequential (in order)
    };

private:
    // Controller references
    AudioController* audioController;
    StripLEDController* ledStripController;   // LED strip controller (GPIO 5)
    MatrixLEDController* ledMatrixController;  // LED matrix controller (GPIO 6), can be nullptr
    class SettingsController* settingsController;  // Forward declaration
    
    // Demo state
    bool demoRunning;
    bool demoPaused;
    unsigned long lastDemoAction;
    unsigned long demoDelay;
    DemoMode demoMode;
    PlaybackOrder playbackOrder;
    
    // Sequential playback tracking
    int currentAudioIndex;
    int currentLEDIndex;
    int currentEffectIndex;
    
    // Audio file list
    static const int MAX_AUDIO_FILES = 100;
    String audioFiles[MAX_AUDIO_FILES];
    int audioFileCount;
    bool filesScanned;
    
    // FreeRTOS task
    TaskHandle_t demoTaskHandle;
    
    // Internal methods
    static void demoTaskWrapper(void* parameter);
    void demoTask();
    void scanAudioFiles(const char* directory);
    String getRandomAudioFile();
    String getNextAudioFile();  // Sequential audio
    String getRandomEffect();  // Returns effect name (e.g. "atomic_breath")
    String getNextEffect();  // Sequential LED - returns effect name
    String getEffectForFile(const String& filename);  // Returns effect name
    void executeNextEffect();  // For effects mode
    
public:
    /**
     * Constructor
     * @param audioCtrl Reference to AudioController
     * @param ledStripCtrl Reference to LED strip controller (GPIO 5)
     * @param settingsCtrl Reference to SettingsController (optional, for effects mode)
     * @param ledMatrixCtrl Optional reference to LED matrix controller (GPIO 6), can be nullptr
     */
    DemoController(AudioController* audioCtrl, StripLEDController* ledStripCtrl, class SettingsController* settingsCtrl = nullptr, MatrixLEDController* ledMatrixCtrl = nullptr);
    
    /**
     * Destructor
     */
    ~DemoController();
    
    /**
     * Initialize demo controller
     * @return true if successful, false otherwise
     */
    bool begin();
    
    /**
     * Start demo mode
     * @param delayMs Delay between demos in milliseconds (default: 5000)
     * @param mode Demo mode (default: DEMO_AUDIO_LED)
     * @param order Playback order (default: ORDER_RANDOM)
     */
    void startDemo(unsigned long delayMs = 5000, DemoMode mode = DEMO_AUDIO_LED, PlaybackOrder order = ORDER_RANDOM);
    
    /**
     * Set demo mode configuration
     * @param mode Demo mode
     * @param order Playback order
     */
    void setDemoMode(DemoMode mode, PlaybackOrder order);
    
    /**
     * Get current demo mode
     * @return Current demo mode
     */
    DemoMode getDemoMode() const { return demoMode; }
    
    /**
     * Get current playback order
     * @return Current playback order
     */
    PlaybackOrder getPlaybackOrder() const { return playbackOrder; }
    
    /**
     * Stop demo mode
     */
    void stopDemo();
    
    /**
     * Pause/resume demo mode
     */
    void pauseDemo();
    void resumeDemo();
    
    /**
     * Check if demo is running
     * @return true if running, false otherwise
     */
    bool isRunning() const { return demoRunning; }
    
    /**
     * Check if demo is paused
     * @return true if paused, false otherwise
     */
    bool isPaused() const { return demoPaused; }
    
    /**
     * Get number of audio files found
     * @return Number of audio files
     */
    int getAudioFileCount() const { return audioFileCount; }
    
    /**
     * Rescan audio files from SD card
     */
    void rescanFiles();
    
    /**
     * Update loop - call this frequently from main loop
     */
    void update();
};

#endif // DEMO_CONTROLLER_H
