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

// WiFi credentials (loaded from Preferences; set via web UI)
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


/**
 * List files in a single directory (non-recursive)
 * Note: Should be called while holding the SD card mutex
 */
void listDirectory(const char* dirPath) {
    File dir = SD.open(dirPath);
    if (!dir) {
        Serial.print("ERROR: Failed to open directory: ");
        Serial.println(dirPath);
        return;
    }
    
    if (!dir.isDirectory()) {
        Serial.print("ERROR: Path is not a directory: ");
        Serial.println(dirPath);
        dir.close();
        return;
    }
    
    Serial.print("Contents of: ");
    Serial.println(dirPath);
    Serial.println("----------------------------------------");
    
    int fileCount = 0;
    int dirCount = 0;
    
    while (true) {
        File entry = dir.openNextFile();
        if (!entry) {
            break;
        }
        
        String name = entry.name();
        Serial.print(name);
        
        if (entry.isDirectory()) {
            Serial.println("/");
            dirCount++;
        } else {
            Serial.print("\t\t");
            Serial.print(entry.size(), DEC);
            Serial.println(" bytes");
            fileCount++;
        }
        entry.close();
    }
    
    dir.close();
    Serial.println("----------------------------------------");
    Serial.print("Total: ");
    Serial.print(fileCount);
    Serial.print(" files, ");
    Serial.print(dirCount);
    Serial.println(" directories");
}


void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n\n=== ESP32-S3 Audio Player ===");
    Serial.println("SD Card -> PCM5122 DAC");
    Serial.println();
    
    // 0. Initialize Audio Controller first (needed for SD card access)
    if (!audioController.begin()) {
        Serial.println("Audio controller initialization failed. Halting.");
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
            Serial.println("[Settings] WARNING: No WiFi credentials found!");
            Serial.println("[Settings] Connect to the device AP and set WiFi via the web UI (Settings).");
        } else {
            Serial.print("[Settings] Loaded WiFi SSID: ");
            Serial.println(wifiSSID);
        }
        
        // Verify effects are loaded
        int effectCount = settingsController.getEffectCount();
        Serial.print("[Settings] Effects loaded: ");
        Serial.println(effectCount);
    } else {
        Serial.println("[Settings] Settings controller initialization failed");
    }
    
    // Load and apply saved volume
    int savedVolume = settingsController.loadVolume(21);
    audioController.setVolume(savedVolume);

    // Load and apply bass mono settings
    bool bassMonoEnabled = settingsController.loadBassMonoEnabled(false);
    uint16_t bassMonoCrossover = settingsController.loadBassMonoCrossover(80);
    bassMonoProcessor.setEnabled(bassMonoEnabled);
    bassMonoProcessor.setCrossoverHz(bassMonoCrossover);
    Serial.print("[BassMono] Enabled: ");
    Serial.print(bassMonoEnabled ? "yes" : "no");
    Serial.print(", Crossover: ");
    Serial.print(bassMonoCrossover);
    Serial.println(" Hz");

    // 2. Initialize LED Controllers
    if (!ledStripController.begin()) {
        Serial.println("LED strip controller initialization failed. Continuing without strip LEDs.");
    }
    
    if (!ledMatrixController.begin()) {
        Serial.println("LED matrix controller initialization failed. Continuing without matrix LEDs.");
    }
    
    // Load and apply saved brightness (shared between both controllers)
    uint8_t savedBrightness = settingsController.loadBrightness(128);
    ledStripController.setBrightness(savedBrightness);
    ledMatrixController.setBrightness(savedBrightness);
    
    // 3. Initialize Demo Controller
    if (!demoController.begin()) {
        Serial.println("Demo controller initialization failed. Continuing without demo mode.");
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
        Serial.print("Web interface available at: http://");
        Serial.println(httpServer.getIPAddress());
        httpServer.printEndpoints();
    } else {
        Serial.println("HTTP server initialization failed - continuing without network control");
        Serial.println("Serial commands still available");
    }
    
    // 5. Initialize MQTT Controller (if enabled in config)
    // Reuse sdMutex from earlier in setup()
    mqttController.setBassMonoProcessor(&bassMonoProcessor);
    if (mqttController.begin(&audioController, &ledStripController, &ledMatrixController, &demoController, &settingsController, sdMutex)) {
        Serial.println("MQTT controller initialized");
    } else {
        Serial.println("MQTT controller disabled or initialization failed");
    }
    
    Serial.println("\nSerial Commands:");
    Serial.println("  play:<filename>  - Play a file (e.g., play:/sounds/test.mp3)");
    Serial.println("  stop             - Stop playback");
    Serial.println("  volume:<0-21>    - Set volume (0-21)");
    Serial.println("  status           - Show current status");
    Serial.println("  dir[:<path>]     - List directory (e.g., dir or dir:/sounds)");
    if (httpServer.isConnected()) {
        httpServer.printEndpoints();
    }
    Serial.println();
    
    // Execute startup effect if configured
    // Only attempt if settings were successfully loaded
    if (settingsLoaded) {
        // Verify effects are loaded before executing startup effect
        int effectCount = settingsController.getEffectCount();
        Serial.print("[Startup] Effects loaded: ");
        Serial.println(effectCount);
        
        Serial.println("[Startup] Checking for startup effect in config.json...");
        if (effectCount > 0) {
            // Small delay to ensure all systems are ready
            delay(100);
            
            if (httpServer.executeEffectByName("startup")) {
                Serial.println("[Startup] Startup effect executed successfully");
            } else {
                Serial.println("[Startup] No startup effect found (this is OK if not configured)");
            }
        } else {
            Serial.println("[Startup] No effects loaded, skipping startup effect");
            Serial.println("[Startup] Add effects to config.json to enable startup effect");
        }
    } else {
        Serial.println("[Startup] Settings not loaded, skipping startup effect");
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
    
    // Process serial commands (non-blocking)
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        cmd.toLowerCase();
        
        if (cmd.startsWith("play:")) {
            // Extract filename from "play:/path/to/file.mp3"
            String newFile = cmd.substring(5);
            newFile.trim();
            
            if (newFile.length() > 0) {
                if (audioController.playFile(newFile)) {
                    Serial.print("File change queued: ");
                    Serial.println(newFile);
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
                Serial.print("Volume set to: ");
                Serial.println(vol);
            } else {
                Serial.println("Volume must be 0-21");
            }
        }
        else if (cmd == "status") {
            Serial.print("Current file: ");
            Serial.println(audioController.getCurrentFile());
            Serial.print("Free heap: ");
            Serial.print(ESP.getFreeHeap());
            Serial.println(" bytes");
            Serial.print("Audio running: ");
            Serial.println(audioController.isPlaying() ? "Yes" : "No");
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
        else {
            Serial.println("Unknown command. Available commands:");
            Serial.println("  play:<filename>  - Play a file");
            Serial.println("  stop             - Stop playback");
            Serial.println("  volume:<0-21>    - Set volume");
            Serial.println("  status           - Show status");
            Serial.println("  dir[:<path>]     - List directory");
        }
    }
    
    // Heartbeat status every 5 seconds
    static unsigned long lastUpdate = 0;
    if (millis() - lastUpdate > 5000) {
        Serial.printf("[Status] Heap Free: %u bytes | Audio: %s\n", 
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
    Serial.print("audio_info: ");
    Serial.println(info);

    // Parse sample rate from info strings like "SampleRate=44100"
    if (strncmp(info, "SampleRate=", 11) == 0) {
        uint32_t sr = (uint32_t)atol(info + 11);
        if (sr > 0) {
            bassMonoProcessor.setSampleRate(sr);
            Serial.print("[BassMono] Sample rate set to ");
            Serial.println(sr);
        }
    }
}

void audio_eof_mp3(const char *info) {
    Serial.print("audio_eof_mp3: ");
    Serial.println(info);
    Serial.println("Playback finished.");
    
    // Check if we need to restart audio loop
    // The updateLoops() method will handle the restart in the main loop
}
