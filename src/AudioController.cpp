#include "AudioController.h"
#include "SD.h"
#include "SPI.h"
#include "RuntimeLog.h"

static const char* TAG = "Audio";

AudioController::AudioController(Audio* audioObj,
                                int bck, int lrc, int dout,
                                int cs, int mosi, int miso, int sck,
                                int i2cSda, int i2cScl)
    : audio(audioObj),
      i2sBckPin(bck), i2sLrcPin(lrc), i2sDoutPin(dout),
      i2cSdaPin(i2cSda), i2cSclPin(i2cScl),
      sdCsPin(cs), spiMosiPin(mosi), spiMisoPin(miso), spiSckPin(sck),
      currentFile("/sounds/roar.mp3"),
      fileChanged(false), isSwitchingFile(false),
      currentVolume(21),  // Default volume
      sdCardMutex(NULL),
      audioTaskHandle(NULL),
      audioCommandQueue(NULL),
      audioStatusQueue(NULL) {
}

bool AudioController::initSDCard() {
    ESP_LOGI(TAG, "Initializing SD card...");

    // Give SD card time to stabilize power
    delay(200);
    
    // Initialize CS pin
    pinMode(sdCsPin, OUTPUT);
    digitalWrite(sdCsPin, HIGH);
    delay(50);
    
    // Initialize SPI bus
    SPI.begin(spiSckPin, spiMisoPin, spiMosiPin);
    
    // Configure SPI settings for SD card initialization
    SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
    delay(10);
    SPI.endTransaction();
    delay(100);
    
    // Retry logic: attempt to mount SD card multiple times
    const int maxRetries = 5;
    const int retryDelay = 300;
    
    for (int attempt = 1; attempt <= maxRetries; attempt++) {
        ESP_LOGI(TAG, "Mount attempt %d/%d...", attempt, maxRetries);
        
        if (SD.begin(sdCsPin)) {
            // Verify mount by trying to open root directory
            bool mountVerified = false;
            File root = SD.open("/");
            if (root && root.isDirectory()) {
                root.close();
                mountVerified = true;
            } else {
                if (root) root.close();
            }
            
            if (mountVerified) {
                ESP_LOGI(TAG, "SD card initialized successfully.");
                
                // Create mutex to protect SD card SPI access
                sdCardMutex = xSemaphoreCreateMutex();
                if (sdCardMutex == NULL) {
                    ESP_LOGE(TAG, "Failed to create SD card mutex!");
                    return false;
                }
                
                return true;
            } else {
                ESP_LOGW(TAG, "Mount succeeded but root not accessible");
                SD.end();
                delay(100);
            }
        } else {
            ESP_LOGW(TAG, "Mount attempt failed");
        }
        
        if (attempt < maxRetries) {
            ESP_LOGI(TAG, "Waiting %dms before retry...", retryDelay);
            
            SD.end();
            delay(100);
            
            SPI.end();
            delay(50);
            digitalWrite(sdCsPin, HIGH);
            delay(50);
            
            SPI.begin(spiSckPin, spiMisoPin, spiMosiPin);
            delay(50);
            
            delay(retryDelay - 200);
        }
    }
    
    ESP_LOGE(TAG, "SD Card Mount Failed after all retries!");
    return false;
}

void AudioController::initI2S() {
    ESP_LOGI(TAG, "Initializing I2S for PCM5122...");

    audio->setPinout(i2sBckPin, i2sLrcPin, i2sDoutPin);
    audio->setVolume(21);

    ESP_LOGI(TAG, "I2S initialized.");
}

void AudioController::initDac() {
    if (i2cSdaPin < 0 || i2cSclPin < 0) {
        ESP_LOGI(TAG, "I2C pins not configured, skipping DAC init");
        return;
    }

    if (!dac.begin(i2cSdaPin, i2cSclPin)) {
        ESP_LOGW(TAG, "PCM5122 DAC not found — audio will work without anti-pop");
    }
}

void AudioController::audioTaskWrapper(void* parameter) {
    AudioController* controller = static_cast<AudioController*>(parameter);
    controller->audioTask();
}

void AudioController::audioTask() {
    ESP_LOGI(TAG, "Audio task started on Core 0");
    
    AudioCommand cmd;
    AudioStatus status;
    TickType_t lastStatusUpdate = 0;
    const TickType_t statusUpdateInterval = pdMS_TO_TICKS(1000);  // Update status every 1 second
    
    while (true) {
        // Check for commands from queue (truly non-blocking - 0 timeout)
        // Audio processing must happen frequently, so we can't block here
        if (audioCommandQueue != NULL && 
            xQueueReceive(audioCommandQueue, &cmd, 0) == pdTRUE) {
            
            switch (cmd.type) {
                case AudioCommand::CMD_PLAY: {
                    // Handle play command
                    String filename = String(cmd.filename);
                    filename.trim();
                    
                    if (filename.length() > 0) {
                        // Normalize path
                        if (!filename.startsWith("/")) {
                            filename = "/" + filename;
                        }
                        
                        // Check if file exists
                        bool fileExists = false;
                        if (sdCardMutex != NULL && 
                            xSemaphoreTake(sdCardMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
                            fileExists = SD.exists(filename);
                            
                            if (fileExists) {
                                // Mute DAC before stopping to prevent pop
                                dac.mute();

                                // Stop current playback (if any) and ensure cleanup
                                if (audio->isRunning()) {
                                    audio->stopSong();

                                    // Wait for audio to fully stop
                                    int timeout = 0;
                                    while (audio->isRunning() && timeout < 50) {
                                        audio->loop();
                                        vTaskDelay(pdMS_TO_TICKS(10));
                                        timeout++;
                                    }

                                    vTaskDelay(pdMS_TO_TICKS(50));
                                }

                                // Start playback
                                audio->connecttoFS(SD, filename.c_str());
                                currentFile = filename;
                                ESP_LOGI(TAG, "Playing: %s", filename.c_str());

                                // Let I2S clocks stabilize, then unmute
                                vTaskDelay(pdMS_TO_TICKS(20));
                                dac.unmute();
                            } else {
                                ESP_LOGW(TAG, "File not found: %s", filename.c_str());
                            }
                            
                            xSemaphoreGive(sdCardMutex);
                        }
                    }
                    break;
                }
                
                case AudioCommand::CMD_STOP: {
                    // Mute DAC before stopping to prevent pop
                    dac.mute();
                    vTaskDelay(pdMS_TO_TICKS(20));
                    audio->stopSong();
                    ESP_LOGI(TAG, "Playback stopped");
                    break;
                }
                
                case AudioCommand::CMD_SET_VOLUME: {
                    // Handle volume command
                    if (cmd.volume >= 0 && cmd.volume <= 21) {
                        currentVolume = cmd.volume;
                        audio->setVolume(cmd.volume);
                        ESP_LOGI(TAG, "Volume set to: %d", cmd.volume);
                    }
                    break;
                }
                
                case AudioCommand::CMD_GET_STATUS: {
                    // Status will be sent via status queue below
                    break;
                }
            }
        }
        
        // Process audio loop
        audio->loop();
        
        // Update status periodically and send to queue
        TickType_t currentTime = xTaskGetTickCount();
        if (currentTime - lastStatusUpdate >= statusUpdateInterval) {
            lastStatusUpdate = currentTime;
            
            // Build status
            status.isPlaying = audio->isRunning();
            strncpy(status.currentFile, currentFile.c_str(), sizeof(status.currentFile) - 1);
            status.currentFile[sizeof(status.currentFile) - 1] = '\0';
            status.volume = currentVolume;
            status.playbackTime = millis();
            
            // Send status (non-blocking, drop if queue full)
            if (audioStatusQueue != NULL) {
                // Remove old status if queue is full
                if (uxQueueMessagesWaiting(audioStatusQueue) >= 4) {
                    AudioStatus oldStatus;
                    xQueueReceive(audioStatusQueue, &oldStatus, 0);
                }
                xQueueSend(audioStatusQueue, &status, 0);
            }
        }
        
        // Yield to prevent watchdog trigger
        // Audio task priority is 3, so we need to actually delay to allow IDLE (priority 0) to run
        // When playing: use minimal delay (1 tick = ~10ms) - audio library buffers handle this
        // When not playing: use longer delay (10 ticks = ~100ms) to save CPU
        if (audio->isRunning()) {
            vTaskDelay(1);  // 1 tick delay during playback (minimal jitter, allows IDLE to run)
        } else {
            vTaskDelay(10); // 10 tick delay when idle (saves CPU, allows IDLE to run)
        }
    }
}

bool AudioController::begin() {
    if (!initSDCard()) {
        return false;
    }
    
    initI2S();
    initDac();

    // Create queues for inter-task communication
    audioCommandQueue = xQueueCreate(10, sizeof(AudioCommand));
    audioStatusQueue = xQueueCreate(5, sizeof(AudioStatus));
    
    if (audioCommandQueue == NULL || audioStatusQueue == NULL) {
        ESP_LOGE(TAG, "Failed to create queues!");
        return false;
    }

    ESP_LOGI(TAG, "Queues created successfully");
    
    // Start the Audio Task on Core 0
    xTaskCreatePinnedToCore(
        audioTaskWrapper,
        "AudioTask",
        10000,
        this,
        3,
        &audioTaskHandle,
        0
    );
    
    // Start playback of default file via queue
    playFile(currentFile);
    
    return true;
}

bool AudioController::playFile(const String& filename) {
    if (audioCommandQueue == NULL) {
        ESP_LOGE(TAG, "Command queue not initialized");
        return false;
    }

    String newFile = filename;
    newFile.trim();
    
    if (newFile.length() == 0) {
        return false;
    }
    
    // Normalize path
    if (!newFile.startsWith("/")) {
        newFile = "/" + newFile;
    }
    
    // Create command
    AudioCommand cmd;
    cmd.type = AudioCommand::CMD_PLAY;
    strncpy(cmd.filename, newFile.c_str(), sizeof(cmd.filename) - 1);
    cmd.filename[sizeof(cmd.filename) - 1] = '\0';
    
    // Send command to queue (blocking with timeout)
    if (xQueueSend(audioCommandQueue, &cmd, pdMS_TO_TICKS(100)) == pdTRUE) {
        currentFile = newFile;  // Update local copy for status queries
        return true;
    } else {
        ESP_LOGE(TAG, "Failed to send play command (queue full)");
        return false;
    }
}

void AudioController::stop() {
    if (audioCommandQueue == NULL) {
        ESP_LOGE(TAG, "Command queue not initialized");
        return;
    }

    // Create stop command
    AudioCommand cmd;
    cmd.type = AudioCommand::CMD_STOP;
    
    // Send command to queue (non-blocking)
    if (xQueueSend(audioCommandQueue, &cmd, pdMS_TO_TICKS(100)) != pdTRUE) {
        ESP_LOGW(TAG, "Failed to send stop command (queue full)");
        // Fallback: try direct stop
        audio->stopSong();
    }
}

void AudioController::setVolume(int volume) {
    if (volume < 0 || volume > 21) {
        return;
    }
    
    if (audioCommandQueue == NULL) {
        ESP_LOGE(TAG, "Command queue not initialized");
        return;
    }

    // Create volume command
    AudioCommand cmd;
    cmd.type = AudioCommand::CMD_SET_VOLUME;
    cmd.volume = volume;
    
    // Send command to queue (non-blocking)
    if (xQueueSend(audioCommandQueue, &cmd, pdMS_TO_TICKS(100)) != pdTRUE) {
        ESP_LOGW(TAG, "Failed to send volume command (queue full)");
        // Fallback: set directly
        currentVolume = volume;
        audio->setVolume(volume);
    }
}

int AudioController::getVolume() const {
    // Try to get latest status from queue (non-blocking)
    if (audioStatusQueue != NULL) {
        AudioStatus status;
        AudioStatus latestStatus;
        bool hasStatus = false;
        
        // Get most recent status (discard old ones)
        while (xQueueReceive(audioStatusQueue, &status, 0) == pdTRUE) {
            latestStatus = status;
            hasStatus = true;
        }
        
        if (hasStatus) {
            // Put the latest status back at the front
            xQueueSendToFront(audioStatusQueue, &latestStatus, 0);
            return latestStatus.volume;
        }
    }
    
    // Fallback to cached value
    return currentVolume;
}

bool AudioController::isPlaying() const {
    // Try to get latest status from queue (non-blocking)
    if (audioStatusQueue != NULL) {
        AudioStatus status;
        AudioStatus latestStatus;
        bool hasStatus = false;
        
        // Get most recent status (discard old ones)
        while (xQueueReceive(audioStatusQueue, &status, 0) == pdTRUE) {
            latestStatus = status;
            hasStatus = true;
        }
        
        if (hasStatus) {
            // Put the latest status back at the front
            xQueueSendToFront(audioStatusQueue, &latestStatus, 0);
            return latestStatus.isPlaying;
        }
    }
    
    // Fallback to direct check if queue not available
    return audio->isRunning();
}

String AudioController::getCurrentFile() const {
    // Try to get latest status from queue (non-blocking)
    if (audioStatusQueue != NULL) {
        AudioStatus status;
        AudioStatus latestStatus;
        bool hasStatus = false;
        
        // Get most recent status (discard old ones)
        while (xQueueReceive(audioStatusQueue, &status, 0) == pdTRUE) {
            latestStatus = status;
            hasStatus = true;
        }
        
        if (hasStatus) {
            // Put the latest status back at the front
            xQueueSendToFront(audioStatusQueue, &latestStatus, 0);
            return String(latestStatus.currentFile);
        }
    }
    
    // Fallback to cached value
    return currentFile;
}

bool AudioController::fileExists(const String& filename) {
    bool exists = false;
    if (sdCardMutex != NULL && xSemaphoreTake(sdCardMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        exists = SD.exists(filename);
        xSemaphoreGive(sdCardMutex);
    }
    return exists;
}

void AudioController::update() {
    // File changes are handled in the audio task
    // This method is here for future use or external updates
}
