#include "DemoController.h"
#include "SettingsController.h"
#include "EffectDispatch.h"

DemoController::DemoController(AudioController* audioCtrl, StripLEDController* ledStripCtrl, SettingsController* settingsCtrl, MatrixLEDController* ledMatrixCtrl)
    : audioController(audioCtrl), ledStripController(ledStripCtrl), ledMatrixController(ledMatrixCtrl), settingsController(settingsCtrl),
      demoRunning(false), demoPaused(false),
      lastDemoAction(0), demoDelay(5000),
      demoMode(DEMO_AUDIO_LED), playbackOrder(ORDER_RANDOM),
      currentAudioIndex(0), currentLEDIndex(0), currentEffectIndex(0),
      audioFileCount(0),
      filesScanned(false),
      demoTaskHandle(NULL) {
}

DemoController::~DemoController() {
    stopDemo();
}

bool DemoController::begin() {
    Serial.println("Initializing Demo Controller...");
    
    // Scan for audio files
    rescanFiles();
    
    // Start demo task on Core 1
    xTaskCreatePinnedToCore(
        demoTaskWrapper,
        "DemoTask",
        8192,  // Larger stack for file scanning
        this,
        1,  // Lower priority than audio/LED
        &demoTaskHandle,
        1   // Core 1
    );
    
    Serial.print("Demo Controller initialized. Found ");
    Serial.print(audioFileCount);
    Serial.println(" audio files.");
    
    return true;
}

void DemoController::demoTaskWrapper(void* parameter) {
    DemoController* controller = static_cast<DemoController*>(parameter);
    controller->demoTask();
}

void DemoController::demoTask() {
    Serial.println("Demo task started on Core 1");
    
    while (true) {
        if (demoRunning && !demoPaused) {
            unsigned long currentTime = millis();
            
            // Check if it's time for next demo action
            // For audio modes, wait for audio to finish. For LED-only, use delay.
            bool shouldTrigger = false;
            
            if (demoMode == DEMO_LED_ONLY) {
                // LED-only mode: trigger when effect finishes or delay expires
                // Check both controllers
                bool stripRunning = ledStripController && ledStripController->isEffectRunning();
                bool matrixRunning = ledMatrixController && ledMatrixController->isEffectRunning();
                shouldTrigger = (!stripRunning && !matrixRunning && (currentTime - lastDemoAction >= demoDelay));
            } else if (demoMode == DEMO_EFFECTS) {
                // Effects mode: check if current effect finished (audio stopped)
                shouldTrigger = (!audioController->isPlaying() && (currentTime - lastDemoAction >= demoDelay));
            } else {
                // Audio modes: wait for audio to finish
                shouldTrigger = (!audioController->isPlaying() && (currentTime - lastDemoAction >= demoDelay));
            }
            
            if (shouldTrigger) {
                // Time to start a new demo sequence
                bool success = false;
                
                switch (demoMode) {
                    case DEMO_AUDIO_LED: {
                        // Original behavior: audio + LED
                        String audioFile = (playbackOrder == ORDER_RANDOM) ? getRandomAudioFile() : getNextAudioFile();
                        if (audioFile.length() > 0) {
                            String effectName = (playbackOrder == ORDER_RANDOM) ?
                                getEffectForFile(audioFile) : getNextEffect();
                            Serial.print("[Demo] Playing: ");
                            Serial.print(audioFile);
                            Serial.print(" with effect: ");
                            Serial.println(effectName);
                            dispatchLEDEffect(settingsController, effectName.c_str(), ledStripController, ledMatrixController);
                            audioController->playFile(audioFile);
                            success = true;
                        }
                        break;
                    }
                    
                    case DEMO_AUDIO_ONLY: {
                        // Audio files only
                        String audioFile = (playbackOrder == ORDER_RANDOM) ? getRandomAudioFile() : getNextAudioFile();
                        if (audioFile.length() > 0) {
                            Serial.print("[Demo] Playing audio: ");
                            Serial.println(audioFile);
                            audioController->playFile(audioFile);
                            success = true;
                        }
                        break;
                    }
                    
                    case DEMO_LED_ONLY: {
                        // LED effects only
                        String effectName = (playbackOrder == ORDER_RANDOM) ? getRandomEffect() : getNextEffect();
                        Serial.print("[Demo] Playing LED effect: ");
                        Serial.println(effectName);
                        if (dispatchLEDEffect(settingsController, effectName.c_str(), ledStripController, ledMatrixController)) {
                            success = true;
                        }
                        break;
                    }
                    
                    case DEMO_EFFECTS: {
                        // Configured effects from config.json
                        executeNextEffect();
                        success = true;
                        break;
                    }
                }
                
                if (success) {
                    lastDemoAction = currentTime;
                } else {
                    Serial.println("[Demo] No content available, rescanning...");
                    if (demoMode == DEMO_AUDIO_ONLY || demoMode == DEMO_AUDIO_LED) {
                        rescanFiles();
                    }
                    lastDemoAction = currentTime + 2000; // Wait 2 seconds before retry
                }
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(100)); // Check every 100ms
    }
}

void DemoController::scanAudioFiles(const char* directory) {
    audioFileCount = 0;
    
    SemaphoreHandle_t mutex = audioController->getSDMutex();
    if (mutex == NULL) {
        Serial.println("[Demo] ERROR: SD card mutex not available");
        return;
    }
    
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(2000)) == pdTRUE) {
        File dir = SD.open(directory);
        if (dir && dir.isDirectory()) {
            while (audioFileCount < MAX_AUDIO_FILES) {
                File entry = dir.openNextFile();
                if (!entry) break;
                
                String name = entry.name();
                
                // Check if it's an audio file (common extensions)
                name.toLowerCase();
                if (!entry.isDirectory() && 
                    (name.endsWith(".mp3") || name.endsWith(".wav") || 
                     name.endsWith(".aac") || name.endsWith(".m4a") ||
                     name.endsWith(".flac") || name.endsWith(".ogg"))) {
                    // Get full path
                    String fullPath = String(directory);
                    if (!fullPath.endsWith("/")) {
                        fullPath += "/";
                    }
                    fullPath += entry.name();
                    
                    audioFiles[audioFileCount] = fullPath;
                    audioFileCount++;
                }
                
                entry.close();
                
                // Yield periodically
                if (audioFileCount % 10 == 0) {
                    vTaskDelay(1);
                }
            }
            dir.close();
        } else {
            if (dir) dir.close();
        }
        
        xSemaphoreGive(mutex);
    } else {
        Serial.println("[Demo] ERROR: Timeout waiting for SD card access");
    }
    
    filesScanned = true;
    Serial.print("[Demo] Scanned ");
    Serial.print(audioFileCount);
    Serial.print(" audio files from ");
    Serial.println(directory);
}

void DemoController::rescanFiles() {
    filesScanned = false;
    // Scan only the /sounds directory
    scanAudioFiles("/sounds");
}

String DemoController::getRandomAudioFile() {
    if (audioFileCount == 0) {
        return String("");
    }
    
    // Use simple pseudo-random based on millis
    int index = (millis() / 17) % audioFileCount; // 17 is a prime number for better distribution
    return audioFiles[index];
}

String DemoController::getNextAudioFile() {
    if (audioFileCount == 0) {
        return String("");
    }
    
    // Sequential playback
    String file = audioFiles[currentAudioIndex];
    currentAudioIndex = (currentAudioIndex + 1) % audioFileCount;
    return file;
}

static const char* DEMO_STRIP_EFFECT_NAMES[] = {
    "atomic_breath", "gravity_beam", "fire_breath", "electric_attack", "battle_damage", "victory"
};
static const int DEMO_STRIP_EFFECT_COUNT = 6;

String DemoController::getRandomEffect() {
    int index = (millis() / 23) % DEMO_STRIP_EFFECT_COUNT;
    return String(DEMO_STRIP_EFFECT_NAMES[index]);
}

String DemoController::getNextEffect() {
    String name = String(DEMO_STRIP_EFFECT_NAMES[currentLEDIndex]);
    currentLEDIndex = (currentLEDIndex + 1) % DEMO_STRIP_EFFECT_COUNT;
    return name;
}

String DemoController::getEffectForFile(const String& filename) {
    String lowerName = filename;
    lowerName.toLowerCase();
    if (lowerName.indexOf("atomic") >= 0 || lowerName.indexOf("godzilla") >= 0) {
        return String("atomic_breath");
    } else if (lowerName.indexOf("gravity") >= 0 || lowerName.indexOf("ghidorah") >= 0) {
        return String("gravity_beam");
    } else if (lowerName.indexOf("fire") >= 0 || lowerName.indexOf("flame") >= 0) {
        return String("fire_breath");
    } else if (lowerName.indexOf("electric") >= 0 || lowerName.indexOf("lightning") >= 0) {
        return String("electric_attack");
    } else if (lowerName.indexOf("damage") >= 0 || lowerName.indexOf("hurt") >= 0) {
        return String("battle_damage");
    } else if (lowerName.indexOf("victory") >= 0 || lowerName.indexOf("win") >= 0) {
        return String("victory");
    }
    return getRandomEffect();
}

void DemoController::startDemo(unsigned long delayMs, DemoMode mode, PlaybackOrder order) {
    if (demoRunning) {
        return; // Already running
    }
    
    demoDelay = delayMs;
    demoMode = mode;
    playbackOrder = order;
    demoRunning = true;
    demoPaused = false;
    lastDemoAction = millis();
    
    // Reset sequential indices
    currentAudioIndex = 0;
    currentLEDIndex = 0;
    currentEffectIndex = 0;
    
    // Rescan files if needed for audio modes
    if ((mode == DEMO_AUDIO_ONLY || mode == DEMO_AUDIO_LED) && (!filesScanned || audioFileCount == 0)) {
        rescanFiles();
    }
    
    Serial.print("[Demo] Demo mode started: mode=");
    Serial.print(mode);
    Serial.print(", order=");
    Serial.print(order);
    Serial.print(", delay=");
    Serial.println(delayMs);
}

void DemoController::setDemoMode(DemoMode mode, PlaybackOrder order) {
    demoMode = mode;
    playbackOrder = order;
    
    // Reset sequential indices when mode changes
    currentAudioIndex = 0;
    currentLEDIndex = 0;
    currentEffectIndex = 0;
    
    Serial.print("[Demo] Mode changed: mode=");
    Serial.print(mode);
    Serial.print(", order=");
    Serial.println(order);
}

void DemoController::executeNextEffect() {
    if (!settingsController) {
        Serial.println("[Demo] ERROR: SettingsController not available for effects mode");
        return;
    }
    
    int effectCount = settingsController->getEffectCount();
    if (effectCount == 0) {
        Serial.println("[Demo] No effects configured");
        return;
    }
    
    SettingsController::Effect effect;
    if (playbackOrder == ORDER_RANDOM) {
        // Random effect
        int index = (millis() / 31) % effectCount; // Prime for distribution
        if (!settingsController->getEffect(index, &effect)) {
            return;
        }
    } else {
        // Sequential effect
        if (!settingsController->getEffect(currentEffectIndex, &effect)) {
            currentEffectIndex = 0; // Wrap around
            if (!settingsController->getEffect(currentEffectIndex, &effect)) {
                return;
            }
        }
        currentEffectIndex = (currentEffectIndex + 1) % effectCount;
    }
    
    Serial.print("[Demo] Executing effect: ");
    Serial.println(effect.name);
    
    // Execute the effect (audio, LED, or both)
    if (effect.hasAudio && audioController) {
        audioController->playFile(effect.audioFile);
    }
    
    if (effect.hasLED) {
        dispatchLEDEffect(settingsController, effect.ledEffectName, ledStripController, ledMatrixController);
    }
}

void DemoController::stopDemo() {
    demoRunning = false;
    demoPaused = false;
    
    // Stop current audio and LED effects
    audioController->stop();
    // Stop both controllers
    if (ledStripController) {
        ledStripController->stopEffect();
    }
    if (ledMatrixController) {
        ledMatrixController->stopEffect();
    }
    
    Serial.println("[Demo] Demo mode stopped");
}

void DemoController::pauseDemo() {
    if (demoRunning && !demoPaused) {
        demoPaused = true;
        Serial.println("[Demo] Demo mode paused");
    }
}

void DemoController::resumeDemo() {
    if (demoRunning && demoPaused) {
        demoPaused = false;
        lastDemoAction = millis();
        Serial.println("[Demo] Demo mode resumed");
    }
}

void DemoController::update() {
    // Demo runs in separate task, but this can be used for synchronization
}
