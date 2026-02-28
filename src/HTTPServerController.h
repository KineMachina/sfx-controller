#ifndef HTTP_SERVER_CONTROLLER_H
#define HTTP_SERVER_CONTROLLER_H

#include "Arduino.h"
#include <ESPAsyncWebServer.h>
#include "WiFi.h"
#include "AudioController.h"
#include "StripLEDController.h"
#include "MatrixLEDController.h"
#include "DemoController.h"
#include "SettingsController.h"
#include "BassMonoProcessor.h"
#include "SD.h"
#include <FreeRTOS.h>
#include <semphr.h>

/**
 * HTTPServerController - Manages WiFi connection and HTTP server
 * Provides REST API endpoints for controlling audio and LED effects
 * Uses ESPAsyncWebServer for non-blocking, reliable request handling
 */
class HTTPServerController {
private:
    // WiFi configuration (pointers to external buffers)
    char* wifiSSID;
    char* wifiPassword;
    int httpPort;
    char deviceId[32];
    
    // Async Web server
    AsyncWebServer* server;
    
    // Controller references
    AudioController* audioController;
    StripLEDController* ledStripController;   // LED strip controller (GPIO 5)
    MatrixLEDController* ledMatrixController; // LED matrix controller (GPIO 6)
    DemoController* demoController;
    SettingsController* settingsController;
    BassMonoProcessor* bassMonoProcessor;
    
    // Loop tracking
    String currentLoopingEffect;
    bool isLoopingAudio;
    bool isLoopingLED;
    String loopingAudioFile;
    char loopingLEDEffectName[64];
    bool loopingAvSync;  // Track if current loop has av_sync enabled
    bool audioPlaybackFailed;  // Track if audio playback failed (file not found, etc.)
    unsigned long audioLoopStartTime;  // Track when audio loop restart was attempted
    static const unsigned long AUDIO_START_TIMEOUT = 3000;  // 3 seconds timeout for audio to start
    
    // Internal methods
    bool initWiFi();
    void setupRoutes();
    void startEffectLoop(const SettingsController::Effect& effect);
    void stopEffectLoop();
    String getStripLEDEffectName(StripLEDController::EffectType effect) const;
    String getMatrixLEDEffectName(MatrixLEDController::EffectType effect) const;
    
    // HTTP handler methods
    void handleRoot(AsyncWebServerRequest *request);
    void handlePlay(AsyncWebServerRequest *request);
    void handleStop(AsyncWebServerRequest *request);
    void handleVolume(AsyncWebServerRequest *request);
    void handleStatus(AsyncWebServerRequest *request);
    void handleDir(AsyncWebServerRequest *request);
    void handleLEDEffect(AsyncWebServerRequest *request);
    void handleLEDBrightness(AsyncWebServerRequest *request);
    void handleLEDStop(AsyncWebServerRequest *request);
    void handleLEDEffectsList(AsyncWebServerRequest *request);
    void handleDemoStart(AsyncWebServerRequest *request);
    void handleDemoStop(AsyncWebServerRequest *request);
    void handleDemoPause(AsyncWebServerRequest *request);
    void handleDemoResume(AsyncWebServerRequest *request);
    void handleDemoStatus(AsyncWebServerRequest *request);
    void handleSettingsClear(AsyncWebServerRequest *request);
    void handleWiFiConfig(AsyncWebServerRequest *request);
    void handleMQTTConfig(AsyncWebServerRequest *request);
    void handleConfigReload(AsyncWebServerRequest *request);
    void handleEffectsList(AsyncWebServerRequest *request);
    void handleEffectExecute(AsyncWebServerRequest *request);
    void handleEffectStopLoop(AsyncWebServerRequest *request);
    // Bass Mono DSP endpoint
    void handleBassMonoGet(AsyncWebServerRequest *request);
    void handleBassMonoPost(AsyncWebServerRequest *request);

    // Battle Arena API endpoints
    void handleBattleTrigger(AsyncWebServerRequest *request);
    void handleBattleEffects(AsyncWebServerRequest *request);
    void handleBattleStatus(AsyncWebServerRequest *request);
    void handleBattleHealth(AsyncWebServerRequest *request);
    void handleBattleStop(AsyncWebServerRequest *request);
    void handleNotFound(AsyncWebServerRequest *request);
    
    // Static wrapper functions for AsyncWebServer callbacks
    static void handleRootWrapper(AsyncWebServerRequest *request);
    static void handlePlayWrapper(AsyncWebServerRequest *request);
    static void handleStopWrapper(AsyncWebServerRequest *request);
    static void handleVolumeWrapper(AsyncWebServerRequest *request);
    static void handleStatusWrapper(AsyncWebServerRequest *request);
    static void handleDirWrapper(AsyncWebServerRequest *request);
    static void handleLEDEffectWrapper(AsyncWebServerRequest *request);
    static void handleLEDBrightnessWrapper(AsyncWebServerRequest *request);
    static void handleLEDStopWrapper(AsyncWebServerRequest *request);
    static void handleLEDEffectsListWrapper(AsyncWebServerRequest *request);
    static void handleDemoStartWrapper(AsyncWebServerRequest *request);
    static void handleDemoStopWrapper(AsyncWebServerRequest *request);
    static void handleDemoPauseWrapper(AsyncWebServerRequest *request);
    static void handleDemoResumeWrapper(AsyncWebServerRequest *request);
    static void handleDemoStatusWrapper(AsyncWebServerRequest *request);
    static void handleSettingsClearWrapper(AsyncWebServerRequest *request);
    static void handleWiFiConfigWrapper(AsyncWebServerRequest *request);
    static void handleMQTTConfigWrapper(AsyncWebServerRequest *request);
    static void handleConfigReloadWrapper(AsyncWebServerRequest *request);
    static void handleEffectsListWrapper(AsyncWebServerRequest *request);
    static void handleEffectExecuteWrapper(AsyncWebServerRequest *request);
    static void handleEffectStopLoopWrapper(AsyncWebServerRequest *request);
    // Bass Mono DSP wrapper functions
    static void handleBassMonoGetWrapper(AsyncWebServerRequest *request);
    static void handleBassMonoPostWrapper(AsyncWebServerRequest *request);
    // Battle Arena API wrapper functions
    static void handleBattleTriggerWrapper(AsyncWebServerRequest *request);
    static void handleBattleEffectsWrapper(AsyncWebServerRequest *request);
    static void handleBattleStatusWrapper(AsyncWebServerRequest *request);
    static void handleBattleHealthWrapper(AsyncWebServerRequest *request);
    static void handleBattleStopWrapper(AsyncWebServerRequest *request);
    static void handleNotFoundWrapper(AsyncWebServerRequest *request);
    
    // Static instance pointer for callbacks
    static HTTPServerController* instance;
    
public:
    /**
     * Constructor
     * @param ssid WiFi SSID buffer (will be modified)
     * @param password WiFi password buffer (will be modified)
     * @param port HTTP server port (default: 80)
     */
    HTTPServerController(char* ssid, char* password, int port = 80);

    /**
     * Destructor
     */
    ~HTTPServerController();

    /**
     * Set device ID for mDNS hostname and WiFi identification.
     * Must be called before begin().
     */
    void setDeviceId(const char* id);
    
    /**
     * Initialize WiFi and HTTP server
     * @param audioCtrl Reference to AudioController
     * @param ledStripCtrl Reference to LED strip controller (GPIO 5)
     * @param demoCtrl Reference to DemoController
     * @param settingsCtrl Reference to SettingsController
     * @param ledMatrixCtrl Optional reference to LED matrix controller (GPIO 6), can be nullptr
     * @return true if successful, false otherwise
     */
    bool begin(AudioController* audioCtrl, StripLEDController* ledStripCtrl, DemoController* demoCtrl, SettingsController* settingsCtrl, MatrixLEDController* ledMatrixCtrl = nullptr);
    
    /**
     * Update server - checks WiFi connection and handles reconnection
     * Note: ESPAsyncWebServer handles requests automatically, no need to poll
     */
    void update();
    
    /**
     * Check if WiFi is connected
     * @return true if connected, false otherwise
     */
    bool isConnected() const;
    
    /**
     * Get WiFi IP address
     * @return IP address as String, empty if not connected
     */
    String getIPAddress() const;
    
    /**
     * Print API endpoint information to Serial
     */
    void printEndpoints() const;
    
    /**
     * Check and restart loops if needed
     * Call this from main loop to handle loop restarts
     */
    void updateLoops();
    
    /**
     * Get current looping effect name
     * @return Effect name if looping, empty string otherwise
     */
    String getCurrentLoopingEffect() const { return currentLoopingEffect; }
    
    /**
     * Set reference to BassMonoProcessor
     * @param processor Pointer to BassMonoProcessor instance
     */
    void setBassMonoProcessor(BassMonoProcessor* processor) { bassMonoProcessor = processor; }

    /**
     * Execute an effect by name (for internal use, e.g., startup effects)
     * @param effectName Name of the effect to execute
     * @return true if effect was found and executed, false otherwise
     */
    bool executeEffectByName(const char* effectName);
};

#endif // HTTP_SERVER_CONTROLLER_H
