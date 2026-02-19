#ifndef MQTT_CONTROLLER_H
#define MQTT_CONTROLLER_H

#include "Arduino.h"
#include <AsyncMqttClient.h>
#include "WiFi.h"
#include "AudioController.h"
#include "StripLEDController.h"
#include "MatrixLEDController.h"
#include "DemoController.h"
#include "SettingsController.h"
#include "BassMonoProcessor.h"
#include <ArduinoJson.h>
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

/**
 * MQTTController - Manages MQTT connection for battle arena control
 * Provides event-driven command processing and automatic status publishing
 */
class MQTTController {
private:
    // MQTT client
    AsyncMqttClient mqttClient;
    
    // Configuration
    SettingsController::MQTTConfig config;
    bool configLoaded;
    
    // Controller references
    AudioController* audioController;
    StripLEDController* ledStripController;
    MatrixLEDController* ledMatrixController;
    DemoController* demoController;
    SettingsController* settingsController;
    BassMonoProcessor* bassMonoProcessor;
    
    // Connection state
    bool connected;
    bool connectionInProgress;       // True after connect() until onConnect/onDisconnect
    unsigned long lastReconnectAttempt;
    unsigned long reconnectDelay;
    uint32_t reconnectAttemptCount;  // Total reconnect attempts (for logging)
    unsigned long lastWiFiWaitLog;   // Throttle "waiting for WiFi" messages
    static const unsigned long RECONNECT_DELAY_MIN = 1000;   // 1 second
    static const unsigned long RECONNECT_DELAY_MAX = 30000;  // 30 seconds
    static const unsigned long CONNECT_TIMEOUT_MS = 30000;   // Treat no response as failure after 30s
    static const unsigned long WIFI_WAIT_LOG_INTERVAL = 60000;  // Log "waiting for WiFi" at most every 60s
    static const int RESOLVE_RETRY_INTERVAL = 5;  // Re-resolve hostname every N failed attempts
    
    // Topic strings (built from config)
    char topicCommand[256];
    char topicStatus[256];
    char topicStatusAudio[256];
    char topicStatusLED[256];
    char topicStatusHealth[256];
    char topicResponse[256];
    char topicLWT[256];
    
    // State tracking for change detection
    bool lastAudioPlaying;
    int lastVolume;
    uint8_t lastBrightness;
    int lastLEDEffect;
    bool lastLEDRunning;
    unsigned long lastHealthPublish;
    static const unsigned long HEALTH_PUBLISH_INTERVAL = 30000; // 30 seconds
    
    // FreeRTOS task and queue
    TaskHandle_t mqttTaskHandle;
    QueueHandle_t mqttCommandQueue;
    
    // Command structure for queue
    struct MQTTCommand {
        char payload[512];  // Enough for JSON commands
        size_t payloadLen;
    };
    
    // Message buffer for multi-part messages (use char arrays for thread safety)
    char messageBuffer[512];  // Fixed-size buffer instead of String
    char currentTopic[256];    // Fixed-size buffer instead of String
    size_t messageBufferLen;   // Current length of message buffer
    size_t currentTotalLen;
    
    // Resolved broker IP (cached after first resolution)
    IPAddress brokerIP;
    bool brokerIPResolved;
    
    // Internal methods
    static void mqttTaskWrapper(void* parameter);
    void mqttTask();
    void buildTopics();
    IPAddress resolveHostname(const char* hostname);
    void onMqttConnect(bool sessionPresent);
    void onMqttDisconnect(AsyncMqttClientDisconnectReason reason);
    void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total);
    void subscribeToCommands();
    void processCommand(const MQTTCommand& cmd);
    void handleCommandTrigger(const char* payload, size_t len);
    void handleCommandStop(const char* payload, size_t len);
    void handleCommandVolume(const char* payload, size_t len);
    void handleCommandBrightness(const char* payload, size_t len);
    void handleCommandBassMono(const char* payload, size_t len);
    void publishResponse(const char* command, bool success, const char* message, const char* requestId = nullptr);
    void publishStatus(bool force = false);
    void publishHealth();
    void reconnect();
    
    // Static wrapper functions for AsyncMqttClient callbacks
    static void onMqttConnectWrapper(bool sessionPresent);
    static void onMqttDisconnectWrapper(AsyncMqttClientDisconnectReason reason);
    static void onMqttMessageWrapper(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total);
    
    // Static instance pointer for callbacks
    static MQTTController* instance;
    
public:
    /**
     * Constructor
     */
    MQTTController();
    
    /**
     * Destructor
     */
    ~MQTTController();
    
    /**
     * Initialize MQTT client
     * @param audioCtrl Reference to AudioController
     * @param ledStripCtrl Reference to LED strip controller
     * @param ledMatrixCtrl Reference to LED matrix controller
     * @param demoCtrl Reference to DemoController
     * @param settingsCtrl Reference to SettingsController
     * @param sdMutex Optional SD card mutex for thread-safe access
     * @return true if successful, false otherwise
     */
    bool begin(AudioController* audioCtrl, 
               StripLEDController* ledStripCtrl,
               MatrixLEDController* ledMatrixCtrl,
               DemoController* demoCtrl,
               SettingsController* settingsCtrl,
               SemaphoreHandle_t sdMutex = nullptr);
    
    /**
     * Update MQTT controller - call from main loop
     * Handles reconnection and status publishing
     */
    void update();
    
    /**
     * Check if MQTT is connected
     * @return true if connected, false otherwise
     */
    bool isConnected() const { return connected; }
    
    /**
     * Check if MQTT is enabled in config
     * @return true if enabled, false otherwise
     */
    bool isEnabled() const { return configLoaded && config.enabled; }
    
    /**
     * Set reference to BassMonoProcessor
     * @param processor Pointer to BassMonoProcessor instance
     */
    void setBassMonoProcessor(BassMonoProcessor* processor) { bassMonoProcessor = processor; }

    /**
     * Manually publish status (for testing or forced updates)
     */
    void publishStatusNow() { publishStatus(true); }
};

#endif // MQTT_CONTROLLER_H
