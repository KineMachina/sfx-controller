#ifndef SETTINGS_CONTROLLER_H
#define SETTINGS_CONTROLLER_H

#include "Arduino.h"
#include <Preferences.h>
#include "SD.h"
#include <ArduinoJson.h>

/**
 * SettingsController - Manages persistent settings using ESP32 Preferences
 * Also supports loading from JSON config file on SD card
 * Priority: SD card config.json > Preferences > Defaults
 */
class SettingsController {
private:
    Preferences preferences;
    static const char* NAMESPACE;
    static const char* CONFIG_FILE;
    
    // In-memory state for volume, brightness, demo (loaded from config.json or set by save* at runtime; not in NVS)
    int bootVolume;              // -1 = not set
    int bootBrightness;          // -1 = not set
    unsigned long bootDemoDelay; // (unsigned long)-1 = not set
    int bootDemoMode;            // -1 = not set
    int bootDemoOrder;           // -1 = not set
    int bootBassMonoEnabled;     // -1 = not set, 0 = disabled, 1 = enabled
    int bootBassMonoCrossover;   // -1 = not set, otherwise Hz (20-500)
    
    // Internal method to load from SD card config file
    bool loadFromConfigFile();
    
    // Internal method to load effects from Preferences (no-op; effects from SD only)
    void loadEffectsFromPreferences();
    
public:
    /**
     * Constructor
     */
    SettingsController();
    
    /**
     * Initialize settings controller
     * @param sdMutex Optional SD card mutex for thread-safe access
     * @return true if successful, false otherwise
     */
    bool begin(SemaphoreHandle_t sdMutex = nullptr);
    
    /**
     * Load settings from SD card config file
     * Called automatically in begin(), but can be called manually to reload
     * @param sdMutex Optional SD card mutex for thread-safe access
     * @return true if config file was found and loaded, false otherwise
     */
    bool loadFromConfigFile(SemaphoreHandle_t sdMutex = nullptr);
    
    /**
     * Save volume setting
     * @param volume Volume level (0-21)
     */
    void saveVolume(int volume);
    
    /**
     * Load volume setting
     * @param defaultVolume Default value if not found
     * @return Volume level
     */
    int loadVolume(int defaultVolume = 21);
    
    /**
     * Save LED brightness setting
     * @param brightness Brightness level (0-255)
     */
    void saveBrightness(uint8_t brightness);
    
    /**
     * Load LED brightness setting
     * @param defaultBrightness Default value if not found
     * @return Brightness level
     */
    uint8_t loadBrightness(uint8_t defaultBrightness = 128);
    
    /**
     * Save demo delay setting
     * @param delayMs Delay in milliseconds
     */
    void saveDemoDelay(unsigned long delayMs);
    
    /**
     * Load demo delay setting
     * @param defaultDelay Default value if not found
     * @return Delay in milliseconds
     */
    unsigned long loadDemoDelay(unsigned long defaultDelay = 5000);
    
    /**
     * Save demo mode setting
     * @param mode Demo mode (0=audio_led, 1=audio_only, 2=led_only, 3=effects)
     */
    void saveDemoMode(int mode);
    
    /**
     * Load demo mode setting
     * @param defaultMode Default value if not found
     * @return Demo mode
     */
    int loadDemoMode(int defaultMode = 0);
    
    /**
     * Save demo playback order setting
     * @param order Playback order (0=random, 1=sequential)
     */
    void saveDemoOrder(int order);

    /**
     * Save bass mono enabled setting (in-memory)
     * @param enabled true to enable bass mono
     */
    void saveBassMonoEnabled(bool enabled);

    /**
     * Load bass mono enabled setting
     * @param defaultEnabled Default value if not set
     * @return true if bass mono is enabled
     */
    bool loadBassMonoEnabled(bool defaultEnabled = false);

    /**
     * Save bass mono crossover frequency (in-memory)
     * @param crossoverHz Crossover frequency in Hz (20-500)
     */
    void saveBassMonoCrossover(uint16_t crossoverHz);

    /**
     * Load bass mono crossover frequency
     * @param defaultHz Default value if not set
     * @return Crossover frequency in Hz
     */
    uint16_t loadBassMonoCrossover(uint16_t defaultHz = 80);
    
    /**
     * Load demo playback order setting
     * @param defaultOrder Default value if not found
     * @return Playback order
     */
    int loadDemoOrder(int defaultOrder = 0);
    
    /**
     * Save WiFi SSID
     * @param ssid WiFi SSID
     */
    void saveWiFiSSID(const char* ssid);
    
    /**
     * Load WiFi SSID
     * @param buffer Buffer to store SSID
     * @param bufferSize Size of buffer
     * @param defaultSSID Default value if not found
     * @return true if loaded, false if using default
     */
    bool loadWiFiSSID(char* buffer, size_t bufferSize, const char* defaultSSID = "");
    
    /**
     * Save WiFi password
     * @param password WiFi password
     */
    void saveWiFiPassword(const char* password);
    
    /**
     * Load WiFi password
     * @param buffer Buffer to store password
     * @param bufferSize Size of buffer
     * @param defaultPassword Default value if not found
     * @return true if loaded, false if using default
     */
    bool loadWiFiPassword(char* buffer, size_t bufferSize, const char* defaultPassword = "");
    
    /**
     * Clear all settings (factory reset)
     */
    void clearAll();
    
    /**
     * Check if settings exist
     * @return true if settings have been saved before
     */
    bool hasSettings();
    
    /**
     * Effect structure - can contain audio, LED effect, or both
     */
    struct Effect {
        char name[64];
        char audioFile[128];
        char ledEffectName[64];  // LED effect name (e.g. "atomic_breath", "beam_attack"), empty if none
        bool hasAudio;
        bool hasLED;
        bool loop;  // Whether to loop this effect continuously
        bool av_sync;  // Whether to sync LED effects to audio amplitude/frequency
        char category[32];  // Battle arena category: "attack", "growl", "death", "damage", "victory", "idle", etc.
    };

    /**
     * Resolve LED effect name to controller type and enum value.
     * @param name Effect name (e.g. "atomic_breath", "beam_attack")
     * @param outIsStrip true if strip effect, false if matrix effect
     * @param outEnumValue StripLEDController::EffectType or MatrixLEDController::EffectType value (1-based for first effect)
     * @return true if name was recognized, false otherwise
     */
    bool getLEDEffectFromName(const char* name, bool* outIsStrip, int* outEnumValue) const;
    
    /**
     * Get effects by category
     * @param category Category name (e.g., "attack", "growl", "death")
     * @param effects Array to store matching effects
     * @param maxEffects Maximum number of effects to return
     * @return Number of effects found
     */
    int getEffectsByCategory(const char* category, Effect* effects, int maxEffects);
    
    /**
     * Get number of configured effects
     * @return Number of effects
     */
    int getEffectCount();
    
    /**
     * Get effect by index
     * @param index Effect index (0-based)
     * @param effect Buffer to store effect data
     * @return true if effect exists, false otherwise
     */
    bool getEffect(int index, Effect* effect);
    
    /**
     * Get effect by name
     * @param name Effect name
     * @param effect Buffer to store effect data
     * @return true if effect exists, false otherwise
     */
    bool getEffectByName(const char* name, Effect* effect);
    
    /**
     * MQTT Configuration structure
     */
    struct MQTTConfig {
        bool enabled;
        char broker[128];
        int port;
        char username[64];
        char password[64];
        char deviceId[32];
        char baseTopic[64];
        int qosCommands;
        int qosStatus;
        int keepalive;
        bool cleanSession;
    };
    
    /**
     * Load MQTT configuration
     * @param config Buffer to store MQTT config
     * @param sdMutex Optional SD card mutex for thread-safe access
     * @return true if config loaded, false otherwise
     */
    bool loadMQTTConfig(MQTTConfig* config, SemaphoreHandle_t sdMutex = nullptr);
    
    /**
     * Save MQTT configuration to Preferences
     * @param config MQTT config to save
     */
    void saveMQTTConfig(const MQTTConfig* config);
    
private:
    // Internal method to load effects from config file
    void loadEffectsFromConfig(JsonObject& effectsObj);
    static const int MAX_EFFECTS = 32;  // Maximum number of effects
    Effect effects[MAX_EFFECTS];
    int effectCount;
};

#endif // SETTINGS_CONTROLLER_H
