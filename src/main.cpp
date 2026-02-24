#include "Arduino.h"
#include "Audio.h"
#include "SPI.h"
#include "SD.h"
#include "AudioController.h"
#include "HTTPServerController.h"
#include "StripLEDController.h"
#include "MatrixLEDController.h"
#include "DemoController.h"
#include "SettingsController.h"
#include "MQTTController.h"
#include "BassMonoProcessor.h"
#include "RuntimeLog.h"

static const char* TAG = "Main";

// I2S Pins (PCM5122)
#define I2S_BCK      7
#define I2S_LRC      15
#define I2S_DOUT     16

// SD SPI Pins
#define SD_CS        10
#define SPI_MOSI     11
#define SPI_SCK      12
#define SPI_MISO     13

// NeoPixel LED Configuration
#define LED_STRIP_PIN    4   // LED strip data pin (GPIO 4)
#define LED_MATRIX_PIN   6   // LED matrix panel data pin (GPIO 6)
#define LED_COUNT        256  // Number of NeoPixels for strip (adjust to your setup)
#define MATRIX_WIDTH     16  // Matrix panel width
#define MATRIX_HEIGHT    16  // Matrix panel height

// WiFi Configuration (loaded from Preferences; set via web UI)
// If not set, WiFi will fail to connect (user must set via web UI after first boot)
#define HTTP_PORT     80

// WiFi credentials (loaded from Preferences; set via web UI or serial)
char wifiSSID[64] = "";
char wifiPassword[64] = "";

// Controller instances
Audio audio;
AudioController audioController(&audio, I2S_BCK, I2S_LRC, I2S_DOUT,
                                SD_CS, SPI_MOSI, SPI_MISO, SPI_SCK);

StripLEDController ledStripController(LED_STRIP_PIN, LED_COUNT);
MatrixLEDController ledMatrixController(LED_MATRIX_PIN, MATRIX_WIDTH, MATRIX_HEIGHT);

SettingsController settingsController;

DemoController demoController(&audioController, &ledStripController, &settingsController, &ledMatrixController);

HTTPServerController httpServer(wifiSSID, wifiPassword, HTTP_PORT);

MQTTController mqttController;

BassMonoProcessor bassMonoProcessor;

// Runtime log level — checked by ESP_LOGx macros via RuntimeLog.h
esp_log_level_t runtimeLogLevel = ESP_LOG_INFO;


/**
 * List files in a single directory (non-recursive).
 * Interactive command output — uses Serial directly.
 * Note: Should be called while holding the SD card mutex.
 */
void listDirectory(const char* dirPath) {
    File dir = SD.open(dirPath);
    if (!dir) {
        Serial.printf("ERROR: Failed to open directory: %s\n", dirPath);
        return;
    }

    if (!dir.isDirectory()) {
        Serial.printf("ERROR: Path is not a directory: %s\n", dirPath);
        dir.close();
        return;
    }

    Serial.printf("Contents of: %s\n", dirPath);
    Serial.println("----------------------------------------");

    int fileCount = 0;
    int dirCount = 0;

    while (true) {
        File entry = dir.openNextFile();
        if (!entry) {
            break;
        }

        if (entry.isDirectory()) {
            Serial.printf("%s/\n", entry.name());
            dirCount++;
        } else {
            Serial.printf("%-32s %u bytes\n", entry.name(), (unsigned)entry.size());
            fileCount++;
        }
        entry.close();
    }

    dir.close();
    Serial.println("----------------------------------------");
    Serial.printf("Total: %d files, %d directories\n", fileCount, dirCount);
}


void setup() {
    Serial.begin(115200);
    delay(1000);

    ESP_LOGI(TAG, "=== ESP32-S3 Audio Player ===");
    ESP_LOGI(TAG, "SD Card -> PCM5122 DAC");

    // 0. Initialize Audio Controller first (needed for SD card access)
    if (!audioController.begin()) {
        ESP_LOGE(TAG, "Audio controller initialization failed. Halting.");
        while (true) {
            delay(1000);
        }
    }

    // 1. Initialize Settings Controller and load from config.json or Preferences
    // Pass SD card mutex for thread-safe access
    SemaphoreHandle_t sdMutex = audioController.getSDMutex();
    bool settingsLoaded = false;
    if (settingsController.begin(sdMutex)) {
        settingsLoaded = true;
        // Load WiFi credentials from Preferences (set via web UI)
        settingsController.loadWiFiSSID(wifiSSID, sizeof(wifiSSID), "");
        settingsController.loadWiFiPassword(wifiPassword, sizeof(wifiPassword), "");

        if (strlen(wifiSSID) == 0) {
            ESP_LOGW(TAG, "No WiFi credentials found!");
            ESP_LOGW(TAG, "Use serial command 'wifi <ssid> <password>' or the web UI to configure.");
        } else {
            ESP_LOGI(TAG, "Loaded WiFi SSID: %s", wifiSSID);
        }

        // Verify effects are loaded
        int effectCount = settingsController.getEffectCount();
        ESP_LOGI(TAG, "Effects loaded: %d", effectCount);
    } else {
        ESP_LOGE(TAG, "Settings controller initialization failed");
    }

    // Load and apply saved volume
    int savedVolume = settingsController.loadVolume(21);
    audioController.setVolume(savedVolume);

    // Load and apply bass mono settings
    bool bassMonoEnabled = settingsController.loadBassMonoEnabled(false);
    uint16_t bassMonoCrossover = settingsController.loadBassMonoCrossover(80);
    bassMonoProcessor.setEnabled(bassMonoEnabled);
    bassMonoProcessor.setCrossoverHz(bassMonoCrossover);
    ESP_LOGI(TAG, "BassMono enabled: %s, crossover: %u Hz",
             bassMonoEnabled ? "yes" : "no", bassMonoCrossover);

    // 2. Initialize LED Controllers
    if (!ledStripController.begin()) {
        ESP_LOGW(TAG, "LED strip controller initialization failed. Continuing without strip LEDs.");
    }

    if (!ledMatrixController.begin()) {
        ESP_LOGW(TAG, "LED matrix controller initialization failed. Continuing without matrix LEDs.");
    }

    // Load and apply saved brightness (shared between both controllers)
    uint8_t savedBrightness = settingsController.loadBrightness(128);
    ledStripController.setBrightness(savedBrightness);
    ledMatrixController.setBrightness(savedBrightness);

    // 3. Initialize Demo Controller
    if (!demoController.begin()) {
        ESP_LOGW(TAG, "Demo controller initialization failed. Continuing without demo mode.");
    }

    // Load saved demo settings (will be used when demo starts)
    unsigned long savedDemoDelay = settingsController.loadDemoDelay(5000);
    int savedDemoMode = settingsController.loadDemoMode(0);
    int savedDemoOrder = settingsController.loadDemoOrder(0);

    // Note: Demo mode and order are loaded but not automatically applied
    // They can be set via HTTP API or will use defaults when starting demo

    // 4. Initialize HTTP Server (includes WiFi)
    httpServer.setBassMonoProcessor(&bassMonoProcessor);
    if (httpServer.begin(&audioController, &ledStripController, &demoController, &settingsController, &ledMatrixController)) {
        ESP_LOGI(TAG, "Web interface available at: http://%s", httpServer.getIPAddress().c_str());
        httpServer.printEndpoints();
    } else {
        ESP_LOGW(TAG, "HTTP server initialization failed - continuing without network control");
        ESP_LOGI(TAG, "Serial commands still available");
    }

    // 5. Initialize MQTT Controller (if enabled in config)
    // Reuse sdMutex from earlier in setup()
    mqttController.setBassMonoProcessor(&bassMonoProcessor);
    if (mqttController.begin(&audioController, &ledStripController, &ledMatrixController, &demoController, &settingsController, sdMutex)) {
        ESP_LOGI(TAG, "MQTT controller initialized");
    } else {
        ESP_LOGI(TAG, "MQTT controller disabled or initialization failed");
    }

    // Print serial command help (always shown via Serial for interactive use)
    Serial.println("\nSerial Commands:");
    Serial.println("  play:<filename>  - Play a file (e.g., play:/sounds/test.mp3)");
    Serial.println("  stop             - Stop playback");
    Serial.println("  volume:<0-21>    - Set volume (0-21)");
    Serial.println("  status           - Show current status");
    Serial.println("  dir[:<path>]     - List directory (e.g., dir or dir:/sounds)");
    Serial.println("  wifi             - Show WiFi config");
    Serial.println("  wifi <ssid> <pw> - Set WiFi credentials");
    Serial.println("  mqtt             - Show MQTT config");
    Serial.println("  mqtt enable/disable/broker/id/name");
    Serial.println("  log [off|error|warn|info|debug]");
    Serial.println("  reboot           - Restart device");
    Serial.println();
    Serial.println("Tip: Use 'log error' or 'log off' to silence periodic messages.");
    if (httpServer.isConnected()) {
        httpServer.printEndpoints();
    }
    Serial.println();

    // Execute startup effect if configured
    // Only attempt if settings were successfully loaded
    if (settingsLoaded) {
        // Verify effects are loaded before executing startup effect
        int effectCount = settingsController.getEffectCount();
        ESP_LOGI(TAG, "Startup: %d effects loaded", effectCount);

        ESP_LOGI(TAG, "Checking for startup effect in config.json...");
        if (effectCount > 0) {
            // Small delay to ensure all systems are ready
            delay(100);

            if (httpServer.executeEffectByName("startup")) {
                ESP_LOGI(TAG, "Startup effect executed successfully");
            } else {
                ESP_LOGI(TAG, "No startup effect found (this is OK if not configured)");
            }
        } else {
            ESP_LOGI(TAG, "No effects loaded, skipping startup effect");
        }
    } else {
        ESP_LOGW(TAG, "Settings not loaded, skipping startup effect");
    }
}

void loop() {
    // Update HTTP server (handles WiFi reconnection if needed)
    // Note: ESPAsyncWebServer handles requests automatically, no polling needed
    httpServer.update();

    // Update effect loops (restart if needed)
    httpServer.updateLoops();

    // Update MQTT controller (handles connection and status publishing)
    mqttController.update();

    // Update audio controller
    audioController.update();

    // Update LED controllers (effects run in separate tasks)
    ledStripController.update();
    ledMatrixController.update();

    // Update demo controller (runs in separate task)
    demoController.update();

    // Process serial commands (non-blocking, handles \r and \n line endings)
    // Serial command responses use Serial.print directly (interactive output).
    static String serialBuffer = "";
    bool serialLineReady = false;
    while (Serial.available()) {
        char c = (char)Serial.read();
        if (c == '\n' || c == '\r') {
            if (serialBuffer.length() > 0) {
                serialLineReady = true;
            }
            break;  // Stop reading; remaining chars (e.g. \n after \r) consumed next loop
        }
        serialBuffer += c;
    }
    if (serialLineReady) {
        String cmd = serialBuffer;
        serialBuffer = "";
        cmd.trim();
        String originalCmd = cmd;  // Preserve original case for config values
        cmd.toLowerCase();

        if (cmd.startsWith("play:")) {
            // Extract filename from "play:/path/to/file.mp3"
            String newFile = cmd.substring(5);
            newFile.trim();

            if (newFile.length() > 0) {
                if (audioController.playFile(newFile)) {
                    Serial.printf("File change queued: %s\n", newFile.c_str());
                } else {
                    Serial.println("Invalid file path");
                }
            }
        }
        else if (cmd == "stop") {
            audioController.stop();
            Serial.println("Playback stopped.");
        }
        else if (cmd.startsWith("volume:")) {
            int vol = cmd.substring(7).toInt();
            if (vol >= 0 && vol <= 21) {
                audioController.setVolume(vol);
                Serial.printf("Volume set to: %d\n", vol);
            } else {
                Serial.println("Volume must be 0-21");
            }
        }
        else if (cmd == "status") {
            Serial.printf("Current file: %s\n", audioController.getCurrentFile().c_str());
            Serial.printf("Free heap: %u bytes\n", ESP.getFreeHeap());
            Serial.printf("Audio running: %s\n", audioController.isPlaying() ? "Yes" : "No");
        }
        else if (cmd.startsWith("dir")) {
            // Extract directory path from "dir" or "dir:/path/to/dir"
            String dirPath = "/";
            if (cmd.length() > 3) {
                if (cmd.charAt(3) == ':') {
                    dirPath = cmd.substring(4);
                    dirPath.trim();
                } else {
                    dirPath = cmd.substring(3);
                    dirPath.trim();
                }
            }

            if (dirPath.length() == 0) {
                dirPath = "/";
            } else if (!dirPath.startsWith("/")) {
                dirPath = "/" + dirPath;
            }

            // Take mutex before accessing SD card
            SemaphoreHandle_t mutex = audioController.getSDMutex();
            if (mutex != NULL && xSemaphoreTake(mutex, pdMS_TO_TICKS(2000)) == pdTRUE) {
                listDirectory(dirPath.c_str());
                xSemaphoreGive(mutex);
            } else {
                Serial.println("ERROR: Timeout waiting for SD card access!");
            }
        }
        // --- WiFi configuration ---
        else if (cmd == "wifi") {
            // Show current WiFi config
            if (strlen(wifiSSID) > 0) {
                Serial.printf("WiFi SSID: %s\n", wifiSSID);
                Serial.printf("WiFi connected: %s\n", httpServer.isConnected() ? "Yes" : "No");
            } else {
                Serial.println("No WiFi configured. Use: wifi <ssid> <password>");
            }
        }
        else if (cmd.startsWith("wifi ")) {
            // Parse: wifi <ssid> <password> (using original case)
            String args = originalCmd.substring(5);
            args.trim();
            int spaceIdx = args.indexOf(' ');
            if (spaceIdx <= 0) {
                Serial.println("Usage: wifi <ssid> <password>");
            } else {
                String ssid = args.substring(0, spaceIdx);
                String password = args.substring(spaceIdx + 1);
                password.trim();
                if (ssid.length() == 0 || password.length() == 0) {
                    Serial.println("Usage: wifi <ssid> <password>");
                } else {
                    settingsController.saveWiFiSSID(ssid.c_str());
                    settingsController.saveWiFiPassword(password.c_str());
                    strncpy(wifiSSID, ssid.c_str(), sizeof(wifiSSID) - 1);
                    wifiSSID[sizeof(wifiSSID) - 1] = '\0';
                    strncpy(wifiPassword, password.c_str(), sizeof(wifiPassword) - 1);
                    wifiPassword[sizeof(wifiPassword) - 1] = '\0';
                    Serial.printf("WiFi credentials saved. SSID: %s\n", wifiSSID);
                    Serial.println("Reboot to connect with new credentials.");
                }
            }
        }
        // --- MQTT configuration ---
        else if (cmd == "mqtt") {
            // Show current MQTT config
            SettingsController::MQTTConfig mqttConfig;
            settingsController.loadMQTTConfig(&mqttConfig);
            Serial.println("MQTT Configuration:");
            Serial.printf("  Enabled:   %s\n", mqttConfig.enabled ? "Yes" : "No");
            Serial.printf("  Broker:    %s\n", mqttConfig.broker);
            Serial.printf("  Port:      %d\n", mqttConfig.port);
            Serial.printf("  Device ID: %s\n", mqttConfig.deviceId);
            Serial.printf("  Name:      %s\n", mqttConfig.deviceName);
            Serial.printf("  Topic:     %s\n", mqttConfig.baseTopic);
        }
        else if (cmd.startsWith("mqtt ")) {
            String sub = cmd.substring(5);
            sub.trim();
            SettingsController::MQTTConfig mqttConfig;
            settingsController.loadMQTTConfig(&mqttConfig);

            if (sub == "enable") {
                mqttConfig.enabled = true;
                settingsController.saveMQTTConfig(&mqttConfig);
                Serial.println("MQTT enabled. Reboot to apply.");
            }
            else if (sub == "disable") {
                mqttConfig.enabled = false;
                settingsController.saveMQTTConfig(&mqttConfig);
                Serial.println("MQTT disabled. Reboot to apply.");
            }
            else if (sub.startsWith("broker ")) {
                String value = originalCmd.substring(12);  // "mqtt broker "
                value.trim();
                strncpy(mqttConfig.broker, value.c_str(), sizeof(mqttConfig.broker) - 1);
                mqttConfig.broker[sizeof(mqttConfig.broker) - 1] = '\0';
                settingsController.saveMQTTConfig(&mqttConfig);
                Serial.printf("MQTT broker set to: %s\n", mqttConfig.broker);
            }
            else if (sub.startsWith("id ")) {
                String value = originalCmd.substring(8);  // "mqtt id "
                value.trim();
                strncpy(mqttConfig.deviceId, value.c_str(), sizeof(mqttConfig.deviceId) - 1);
                mqttConfig.deviceId[sizeof(mqttConfig.deviceId) - 1] = '\0';
                settingsController.saveMQTTConfig(&mqttConfig);
                Serial.printf("MQTT device ID set to: %s\n", mqttConfig.deviceId);
            }
            else if (sub.startsWith("name ")) {
                String value = originalCmd.substring(10);  // "mqtt name "
                value.trim();
                strncpy(mqttConfig.deviceName, value.c_str(), sizeof(mqttConfig.deviceName) - 1);
                mqttConfig.deviceName[sizeof(mqttConfig.deviceName) - 1] = '\0';
                settingsController.saveMQTTConfig(&mqttConfig);
                Serial.printf("MQTT device name set to: %s\n", mqttConfig.deviceName);
            }
            else {
                Serial.println("MQTT commands:");
                Serial.println("  mqtt                - Show MQTT config");
                Serial.println("  mqtt enable         - Enable MQTT");
                Serial.println("  mqtt disable        - Disable MQTT");
                Serial.println("  mqtt broker <host>  - Set broker hostname");
                Serial.println("  mqtt id <device_id> - Set device ID");
                Serial.println("  mqtt name <name>    - Set device name");
            }
        }
        // --- Logging level control ---
        else if (cmd == "log off" || cmd == "log none") {
            runtimeLogLevel = ESP_LOG_NONE;
            Serial.println("Logging disabled.");
        }
        else if (cmd == "log error") {
            runtimeLogLevel = ESP_LOG_ERROR;
            Serial.println("Log level: ERROR");
        }
        else if (cmd == "log warn") {
            runtimeLogLevel = ESP_LOG_WARN;
            Serial.println("Log level: WARN");
        }
        else if (cmd == "log info" || cmd == "log on") {
            runtimeLogLevel = ESP_LOG_INFO;
            Serial.println("Log level: INFO");
        }
        else if (cmd == "log debug") {
            runtimeLogLevel = ESP_LOG_DEBUG;
            Serial.println("Log level: DEBUG");
        }
        else if (cmd == "log") {
            const char* levelStr = "UNKNOWN";
            switch (runtimeLogLevel) {
                case ESP_LOG_NONE:    levelStr = "OFF"; break;
                case ESP_LOG_ERROR:   levelStr = "ERROR"; break;
                case ESP_LOG_WARN:    levelStr = "WARN"; break;
                case ESP_LOG_INFO:    levelStr = "INFO"; break;
                case ESP_LOG_DEBUG:   levelStr = "DEBUG"; break;
                case ESP_LOG_VERBOSE: levelStr = "VERBOSE"; break;
            }
            Serial.printf("Log level: %s\n", levelStr);
        }
        else if (cmd == "reboot") {
            Serial.println("Rebooting...");
            Serial.flush();
            delay(100);
            ESP.restart();
        }
        else {
            Serial.println("Unknown command. Available commands:");
            Serial.println("  play:<filename>  - Play a file");
            Serial.println("  stop             - Stop playback");
            Serial.println("  volume:<0-21>    - Set volume");
            Serial.println("  status           - Show status");
            Serial.println("  dir[:<path>]     - List directory");
            Serial.println("  wifi             - Show WiFi config");
            Serial.println("  wifi <ssid> <pw> - Set WiFi credentials");
            Serial.println("  mqtt             - Show MQTT config");
            Serial.println("  mqtt enable/disable/broker/id/name");
            Serial.println("  log [off|error|warn|info|debug]");
            Serial.println("  reboot           - Restart device");
        }
    }

    // Heartbeat status every 5 seconds
    static unsigned long lastUpdate = 0;
    if (millis() - lastUpdate > 5000) {
        ESP_LOGD(TAG, "Heap Free: %u bytes | Audio: %s",
                 ESP.getFreeHeap(),
                 audioController.isPlaying() ? "Playing" : "Stopped");
        lastUpdate = millis();
    }

    // Small delay to prevent watchdog issues, but keep it minimal
    // for better HTTP server responsiveness
    delay(1);
}

// Audio DSP callback — called after decode, before volume/I2S output
void audio_process_extern(int16_t* buff, uint16_t len, bool *continueI2S) {
    bassMonoProcessor.process(buff, len);
    *continueI2S = true;
}

// Audio event callbacks
void audio_info(const char *info) {
    ESP_LOGD(TAG, "audio_info: %s", info);

    // Parse sample rate from info strings like "SampleRate=44100"
    if (strncmp(info, "SampleRate=", 11) == 0) {
        uint32_t sr = (uint32_t)atol(info + 11);
        if (sr > 0) {
            bassMonoProcessor.setSampleRate(sr);
            ESP_LOGD(TAG, "BassMono sample rate set to %lu", (unsigned long)sr);
        }
    }
}

void audio_eof_mp3(const char *info) {
    ESP_LOGD(TAG, "audio_eof_mp3: %s - Playback finished", info);

    // Check if we need to restart audio loop
    // The updateLoops() method will handle the restart in the main loop
}
