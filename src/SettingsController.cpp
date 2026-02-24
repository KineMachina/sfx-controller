#include "SettingsController.h"
#include "RuntimeLog.h"

static const char* TAG = "Settings";

const char* SettingsController::NAMESPACE = "kinemachina";
const char* SettingsController::CONFIG_FILE = "/config.json";

SettingsController::SettingsController() : effectCount(0),
    bootVolume(-1), bootBrightness(-1), bootDemoDelay((unsigned long)-1), bootDemoMode(-1), bootDemoOrder(-1),
    bootBassMonoEnabled(-1), bootBassMonoCrossover(-1) {
    // Initialize effects array
    for (int i = 0; i < MAX_EFFECTS; i++) {
        effects[i].name[0] = '\0';
        effects[i].audioFile[0] = '\0';
        effects[i].ledEffectName[0] = '\0';
        effects[i].hasAudio = false;
        effects[i].hasLED = false;
        effects[i].loop = false;
        effects[i].av_sync = false;
        effects[i].category[0] = '\0';
    }
}

bool SettingsController::begin(SemaphoreHandle_t sdMutex) {
    if (!preferences.begin(NAMESPACE, false)) {
        ESP_LOGE(TAG, "Failed to open preferences namespace");
        return false;
    }

    // Try to load from SD card config file first
    bool configLoaded = loadFromConfigFile(sdMutex);
    if (configLoaded) {
        ESP_LOGI(TAG, "Loaded settings from config.json");
    } else {
        ESP_LOGW(TAG, "No config.json found, using Preferences/defaults");
        // Load effects from Preferences if config.json wasn't loaded
        loadEffectsFromPreferences();
    }

    ESP_LOGI(TAG, "Settings controller initialized");
    return true;
}

bool SettingsController::loadFromConfigFile(SemaphoreHandle_t sdMutex) {
    bool mutexTaken = false;
    
    // Take mutex if provided
    if (sdMutex != NULL) {
        if (xSemaphoreTake(sdMutex, pdMS_TO_TICKS(2000)) != pdTRUE) {
            ESP_LOGE(TAG, "Timeout waiting for SD card mutex");
            return false;
        }
        mutexTaken = true;
    }
    
    bool success = false;
    
    // Check if config file exists
    if (SD.exists(CONFIG_FILE)) {
        File configFile = SD.open(CONFIG_FILE, FILE_READ);
        if (configFile) {
            // Parse JSON (using JsonDocument - ArduinoJson v7 API)
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, configFile);
            configFile.close();
            
            if (!error) {
                // Load audio settings (in-memory only; not NVS)
                if (doc["audio"].is<JsonObject>()) {
                    JsonObject audio = doc["audio"];
                    if (audio["volume"].is<int>()) {
                        int volume = audio["volume"];
                        if (volume >= 0 && volume <= 21) {
                            bootVolume = volume;
                        }
                    }
                }
                
                // Load bass mono settings (in-memory only; not NVS)
                if (doc["audio"].is<JsonObject>()) {
                    JsonObject audioObj = doc["audio"];
                    if (audioObj["bass_mono"].is<JsonObject>()) {
                        JsonObject bassMono = audioObj["bass_mono"];
                        if (bassMono["enabled"].is<bool>()) {
                            bootBassMonoEnabled = bassMono["enabled"].as<bool>() ? 1 : 0;
                        }
                        if (bassMono["crossover_hz"].is<int>()) {
                            int hz = bassMono["crossover_hz"].as<int>();
                            if (hz >= 20 && hz <= 500) {
                                bootBassMonoCrossover = hz;
                            }
                        }
                    }
                }

                // Load LED settings (in-memory only; not NVS)
                if (doc["led"].is<JsonObject>()) {
                    JsonObject led = doc["led"];
                    if (led["brightness"].is<int>()) {
                        int brightness = led["brightness"];
                        if (brightness >= 0 && brightness <= 255) {
                            bootBrightness = (int)(uint8_t)brightness;
                        }
                    }
                }
                
                // Load demo settings (in-memory only; not NVS)
                if (doc["demo"].is<JsonObject>()) {
                    JsonObject demo = doc["demo"];
                    if (demo["delay"].is<unsigned long>()) {
                        unsigned long delay = demo["delay"];
                        if (delay >= 1000 && delay <= 60000) {
                            bootDemoDelay = delay;
                        }
                    }
                    if (demo["mode"].is<String>()) {
                        String modeStr = demo["mode"].as<String>();
                        modeStr.toLowerCase();
                        int mode = 0; // Default: audio_led
                        if (modeStr == "audio_only") mode = 1;
                        else if (modeStr == "led_only") mode = 2;
                        else if (modeStr == "effects") mode = 3;
                        bootDemoMode = mode;
                        ESP_LOGI(TAG, "Demo mode: %s", modeStr.c_str());
                    }
                    if (demo["order"].is<String>()) {
                        String orderStr = demo["order"].as<String>();
                        orderStr.toLowerCase();
                        int order = (orderStr == "sequential" || orderStr == "order") ? 1 : 0;
                        bootDemoOrder = order;
                        ESP_LOGI(TAG, "Demo order: %s", orderStr.c_str());
                    }
                }
                
                // Load effects configuration
                if (doc["effects"].is<JsonObject>()) {
                    ESP_LOGI(TAG, "Found 'effects' key in config.json");
                    JsonObject effectsObj = doc["effects"];
                    ESP_LOGD(TAG, "Effects object size: %d", effectsObj.size());
                    loadEffectsFromConfig(effectsObj);
                    ESP_LOGI(TAG, "After loading, effectCount: %d", effectCount);
                } else {
                    ESP_LOGW(TAG, "No 'effects' key found in config.json");
                }
                
                success = true;
                ESP_LOGI(TAG, "Successfully loaded config.json");
            } else {
                ESP_LOGE(TAG, "Failed to parse config.json: %s", error.c_str());
            }
        } else {
            ESP_LOGE(TAG, "Failed to open config.json");
        }
    }
    
    // Release mutex if we took it
    if (mutexTaken && sdMutex != NULL) {
        xSemaphoreGive(sdMutex);
    }
    
    return success;
}

void SettingsController::saveVolume(int volume) {
    bootVolume = volume;
    ESP_LOGI(TAG, "Saved volume (in-memory): %d", volume);
}

int SettingsController::loadVolume(int defaultVolume) {
    if (bootVolume >= 0) {
        return bootVolume;
    }
    return defaultVolume;
}

void SettingsController::saveBrightness(uint8_t brightness) {
    bootBrightness = (int)brightness;
    ESP_LOGI(TAG, "Saved brightness (in-memory): %u", (unsigned)brightness);
}

uint8_t SettingsController::loadBrightness(uint8_t defaultBrightness) {
    if (bootBrightness >= 0) {
        return (uint8_t)bootBrightness;
    }
    return defaultBrightness;
}

void SettingsController::saveDemoDelay(unsigned long delayMs) {
    bootDemoDelay = delayMs;
    ESP_LOGI(TAG, "Saved demo delay (in-memory): %lu", delayMs);
}

unsigned long SettingsController::loadDemoDelay(unsigned long defaultDelay) {
    if (bootDemoDelay != (unsigned long)-1) {
        return bootDemoDelay;
    }
    return defaultDelay;
}

void SettingsController::saveDemoMode(int mode) {
    bootDemoMode = mode;
    ESP_LOGI(TAG, "Saved demo mode (in-memory): %d", mode);
}

int SettingsController::loadDemoMode(int defaultMode) {
    if (bootDemoMode >= 0) {
        return bootDemoMode;
    }
    return defaultMode;
}

void SettingsController::saveDemoOrder(int order) {
    bootDemoOrder = order;
    ESP_LOGI(TAG, "Saved demo order (in-memory): %d", order);
}

int SettingsController::loadDemoOrder(int defaultOrder) {
    if (bootDemoOrder >= 0) {
        return bootDemoOrder;
    }
    return defaultOrder;
}

void SettingsController::saveBassMonoEnabled(bool enabled) {
    bootBassMonoEnabled = enabled ? 1 : 0;
    ESP_LOGI(TAG, "Saved bass mono enabled (in-memory): %s", enabled ? "true" : "false");
}

bool SettingsController::loadBassMonoEnabled(bool defaultEnabled) {
    if (bootBassMonoEnabled >= 0) {
        return bootBassMonoEnabled == 1;
    }
    return defaultEnabled;
}

void SettingsController::saveBassMonoCrossover(uint16_t crossoverHz) {
    if (crossoverHz < 20) crossoverHz = 20;
    if (crossoverHz > 500) crossoverHz = 500;
    bootBassMonoCrossover = (int)crossoverHz;
    ESP_LOGI(TAG, "Saved bass mono crossover (in-memory): %u", (unsigned)crossoverHz);
}

uint16_t SettingsController::loadBassMonoCrossover(uint16_t defaultHz) {
    if (bootBassMonoCrossover >= 0) {
        return (uint16_t)bootBassMonoCrossover;
    }
    return defaultHz;
}

int SettingsController::getEffectsByCategory(const char* category, Effect* effects, int maxEffects) {
    int count = 0;
    String categoryLower = String(category);
    categoryLower.toLowerCase();
    
    for (int i = 0; i < effectCount && count < maxEffects; i++) {
        String effectCategory = String(this->effects[i].category);
        effectCategory.toLowerCase();
        
        if (effectCategory == categoryLower) {
            effects[count] = this->effects[i];
            count++;
        }
    }
    
    return count;
}

void SettingsController::saveWiFiSSID(const char* ssid) {
    preferences.putString("wifiSSID", ssid);
    ESP_LOGI(TAG, "Saved WiFi SSID: %s", ssid);
}

bool SettingsController::loadWiFiSSID(char* buffer, size_t bufferSize, const char* defaultSSID) {
    String ssid = preferences.getString("wifiSSID", defaultSSID);
    if (ssid.length() > 0 && ssid != defaultSSID) {
        strncpy(buffer, ssid.c_str(), bufferSize - 1);
        buffer[bufferSize - 1] = '\0';
        ESP_LOGI(TAG, "Loaded WiFi SSID: %s", buffer);
        return true;
    }
    if (defaultSSID && strlen(defaultSSID) > 0) {
        strncpy(buffer, defaultSSID, bufferSize - 1);
        buffer[bufferSize - 1] = '\0';
    }
    return false;
}

void SettingsController::saveWiFiPassword(const char* password) {
    preferences.putString("wifiPass", password);
    ESP_LOGI(TAG, "Saved WiFi password");
}

bool SettingsController::loadWiFiPassword(char* buffer, size_t bufferSize, const char* defaultPassword) {
    String password = preferences.getString("wifiPass", defaultPassword);
    if (password.length() > 0 && password != defaultPassword) {
        strncpy(buffer, password.c_str(), bufferSize - 1);
        buffer[bufferSize - 1] = '\0';
        ESP_LOGI(TAG, "Loaded WiFi password");
        return true;
    }
    if (defaultPassword && strlen(defaultPassword) > 0) {
        strncpy(buffer, defaultPassword, bufferSize - 1);
        buffer[bufferSize - 1] = '\0';
    }
    return false;
}

void SettingsController::clearAll() {
    preferences.clear();
    ESP_LOGI(TAG, "All settings cleared");
}

bool SettingsController::hasSettings() {
    return preferences.getString("wifiSSID", "").length() > 0 || effectCount > 0;
}

bool SettingsController::loadMQTTConfig(MQTTConfig* config, SemaphoreHandle_t sdMutex) {
    if (!config) {
        return false;
    }
    
    // Initialize with defaults
    config->enabled = false;
    config->port = 1883;
    config->qosCommands = 1;
    config->qosStatus = 0;
    config->keepalive = 60;
    config->cleanSession = false;
    config->broker[0] = '\0';
    config->username[0] = '\0';
    config->password[0] = '\0';
    config->deviceId[0] = '\0';
    config->baseTopic[0] = '\0';
    strncpy(config->deviceName, "SFX Controller A", sizeof(config->deviceName) - 1);
    config->deviceName[sizeof(config->deviceName) - 1] = '\0';
    
    // Try to load from config.json first
    bool configLoaded = loadFromConfigFile(sdMutex);
    
    // Load from Preferences (config.json may have saved them)
    config->enabled = preferences.getBool("mqttEnabled", false);
    config->port = preferences.getInt("mqttPort", 1883);
    config->qosCommands = preferences.getInt("mqttQosCmd", 1);
    config->qosStatus = preferences.getInt("mqttQosStatus", 0);
    config->keepalive = preferences.getInt("mqttKeepalive", 60);
    config->cleanSession = preferences.getBool("mqttCleanSess", false);
    
    String broker = preferences.getString("mqttBroker", "");
    if (broker.length() > 0) {
        strncpy(config->broker, broker.c_str(), sizeof(config->broker) - 1);
        config->broker[sizeof(config->broker) - 1] = '\0';
    }
    
    String username = preferences.getString("mqttUsername", "");
    if (username.length() > 0) {
        strncpy(config->username, username.c_str(), sizeof(config->username) - 1);
        config->username[sizeof(config->username) - 1] = '\0';
    }
    
    String password = preferences.getString("mqttPassword", "");
    if (password.length() > 0) {
        strncpy(config->password, password.c_str(), sizeof(config->password) - 1);
        config->password[sizeof(config->password) - 1] = '\0';
    }
    
    String deviceId = preferences.getString("mqttDeviceId", "");
    if (deviceId.length() > 0) {
        strncpy(config->deviceId, deviceId.c_str(), sizeof(config->deviceId) - 1);
        config->deviceId[sizeof(config->deviceId) - 1] = '\0';
    }
    
    String baseTopic = preferences.getString("mqttBaseTopic", "");
    if (baseTopic.length() > 0) {
        strncpy(config->baseTopic, baseTopic.c_str(), sizeof(config->baseTopic) - 1);
        config->baseTopic[sizeof(config->baseTopic) - 1] = '\0';
    }

    String deviceName = preferences.getString("mqttDevName", "");
    if (deviceName.length() > 0) {
        strncpy(config->deviceName, deviceName.c_str(), sizeof(config->deviceName) - 1);
        config->deviceName[sizeof(config->deviceName) - 1] = '\0';
    }

    return config->enabled && strlen(config->broker) > 0;
}

void SettingsController::saveMQTTConfig(const MQTTConfig* config) {
    if (!config) {
        return;
    }
    
    preferences.putBool("mqttEnabled", config->enabled);
    preferences.putInt("mqttPort", config->port);
    preferences.putInt("mqttQosCmd", config->qosCommands);
    preferences.putInt("mqttQosStatus", config->qosStatus);
    preferences.putInt("mqttKeepalive", config->keepalive);
    preferences.putBool("mqttCleanSess", config->cleanSession);
    preferences.putString("mqttBroker", config->broker);
    preferences.putString("mqttUsername", config->username);
    preferences.putString("mqttPassword", config->password);
    preferences.putString("mqttDeviceId", config->deviceId);
    preferences.putString("mqttBaseTopic", config->baseTopic);
    preferences.putString("mqttDevName", config->deviceName);

    ESP_LOGI(TAG, "Saved MQTT configuration");
}

void SettingsController::loadEffectsFromConfig(JsonObject& effectsObj) {
    ESP_LOGD(TAG, "loadEffectsFromConfig() called");
    effectCount = 0;
    
    // Iterate through all effects in the JSON object
    for (JsonPair pair : effectsObj) {
        ESP_LOGD(TAG, "Processing effect pair, current count: %d", effectCount);

        if (effectCount >= MAX_EFFECTS) {
            ESP_LOGW(TAG, "Maximum number of effects reached!");
            break;
        }
        
        const char* effectName = pair.key().c_str();
        ESP_LOGD(TAG, "Effect name from key: '%s'", effectName);
        
        JsonObject effectData = pair.value().as<JsonObject>();
        ESP_LOGD(TAG, "Effect data object size: %d", effectData.size());
        
        Effect& effect = effects[effectCount];
        strncpy(effect.name, effectName, sizeof(effect.name) - 1);
        effect.name[sizeof(effect.name) - 1] = '\0';
        ESP_LOGD(TAG, "Stored effect name: '%s'", effect.name);
        
        // Parse loop setting (optional, defaults to false)
        effect.loop = false;
        if (effectData["loop"].is<bool>()) {
            effect.loop = effectData["loop"].as<bool>();
            ESP_LOGD(TAG, "Loop setting: %s", effect.loop ? "enabled" : "disabled");
        }
        
        // Parse av_sync setting (optional, defaults to false)
        effect.av_sync = false;
        if (effectData["av_sync"].is<bool>()) {
            effect.av_sync = effectData["av_sync"].as<bool>();
            ESP_LOGD(TAG, "AV sync setting: %s", effect.av_sync ? "enabled" : "disabled");
        }
        
        // Parse category setting (optional, defaults to empty)
        effect.category[0] = '\0';
        if (effectData["category"].is<String>()) {
            String categoryStr = effectData["category"].as<String>();
            strncpy(effect.category, categoryStr.c_str(), sizeof(effect.category) - 1);
            effect.category[sizeof(effect.category) - 1] = '\0';
            ESP_LOGD(TAG, "Category: %s", effect.category);
        }
        
        // Parse audio file (optional)
        effect.hasAudio = false;
        if (effectData["audio"].is<String>()) {
            String audioFile = effectData["audio"].as<String>();
            ESP_LOGD(TAG, "Found audio key, value: '%s'", audioFile.c_str());
            if (audioFile.length() > 0) {
                strncpy(effect.audioFile, audioFile.c_str(), sizeof(effect.audioFile) - 1);
                effect.audioFile[sizeof(effect.audioFile) - 1] = '\0';
                effect.hasAudio = true;
                ESP_LOGD(TAG, "Set audio file: '%s'", effect.audioFile);
            }
        } else {
            ESP_LOGD(TAG, "No audio key found for this effect");
        }
        
        // Parse LED effect (optional) - store name as-is
        effect.hasLED = false;
        effect.ledEffectName[0] = '\0';
        if (effectData["led"].is<String>()) {
            String ledEffectName = effectData["led"].as<String>();
            ledEffectName.trim();
            ESP_LOGD(TAG, "Found led key, value: '%s'", ledEffectName.c_str());
            if (ledEffectName.length() > 0) {
                strncpy(effect.ledEffectName, ledEffectName.c_str(), sizeof(effect.ledEffectName) - 1);
                effect.ledEffectName[sizeof(effect.ledEffectName) - 1] = '\0';
                effect.hasLED = true;
                ESP_LOGD(TAG, "LED effect name: %s", effect.ledEffectName);
            }
        } else {
            ESP_LOGD(TAG, "No led key found for this effect");
        }
        
        // Effects are read-only from config.json; not persisted to NVS
        effectCount++;

        ESP_LOGI(TAG, "Loaded effect #%d: %s%s%s%s%s%s%s",
            effectCount, effect.name,
            effect.hasAudio ? " (audio: " : "",
            effect.hasAudio ? effect.audioFile : "",
            effect.hasAudio ? ")" : "",
            effect.hasLED ? " (LED: " : "",
            effect.hasLED ? effect.ledEffectName : "",
            effect.hasLED ? ")" : "");
    }
    
    ESP_LOGI(TAG, "loadEffectsFromConfig() complete, total effects: %d", effectCount);
}

int SettingsController::getEffectCount() {
    ESP_LOGD(TAG, "getEffectCount() called, returning: %d", effectCount);
    return effectCount;
}

bool SettingsController::getEffect(int index, Effect* effect) {
    ESP_LOGD(TAG, "getEffect(%d) called, effectCount: %d", index, effectCount);

    if (index < 0 || index >= effectCount || effect == nullptr) {
        ESP_LOGW(TAG, "getEffect() - invalid index or null pointer");
        return false;
    }
    *effect = effects[index];
    ESP_LOGD(TAG, "getEffect() - returning effect: %s", effects[index].name);
    return true;
}

bool SettingsController::getEffectByName(const char* name, Effect* effect) {
    ESP_LOGD(TAG, "getEffectByName('%s') called, effectCount: %d", name ? name : "(null)", effectCount);

    if (name == nullptr || effect == nullptr) {
        ESP_LOGW(TAG, "getEffectByName() - null pointer");
        return false;
    }
    
    for (int i = 0; i < effectCount; i++) {
        ESP_LOGD(TAG, "Comparing '%s' with '%s' -> %s", name, effects[i].name,
            strcmp(effects[i].name, name) == 0 ? "MATCH" : "no match");
        
        if (strcmp(effects[i].name, name) == 0) {
            *effect = effects[i];
            ESP_LOGD(TAG, "getEffectByName() - found effect: %s (hasAudio: %s, hasLED: %s)",
                effect->name,
                effect->hasAudio ? "yes" : "no",
                effect->hasLED ? "yes" : "no");
            return true;
        }
    }
    
    ESP_LOGW(TAG, "getEffectByName() - effect not found");
    return false;
}

void SettingsController::loadEffectsFromPreferences() {
    effectCount = 0;
    
    // Scan preferences for effect keys (they start with "eff_")
    // Note: Preferences doesn't have a way to enumerate keys, so we'll
    // try to load effects that were saved. For now, we'll rely on config.json
    // for initial loading. This method can be enhanced later if needed.
    
    // For now, effects are primarily loaded from config.json
    // This method is a placeholder for future enhancement
}

bool SettingsController::getLEDEffectFromName(const char* name, bool* outIsStrip, int* outEnumValue) const {
    if (name == nullptr || outIsStrip == nullptr || outEnumValue == nullptr) {
        return false;
    }
    String lower(name);
    lower.toLowerCase();
    lower.trim();
    if (lower.length() == 0) {
        return false;
    }
    // Strip effects: enum values 1-23 (StripLEDController::EffectType)
    if (lower == "atomic_breath" || lower == "atomic") { *outIsStrip = true; *outEnumValue = 1; return true; }
    if (lower == "gravity_beam" || lower == "gravity") { *outIsStrip = true; *outEnumValue = 2; return true; }
    if (lower == "fire_breath" || lower == "fire") { *outIsStrip = true; *outEnumValue = 3; return true; }
    if (lower == "electric_attack" || lower == "electric") { *outIsStrip = true; *outEnumValue = 4; return true; }
    if (lower == "battle_damage" || lower == "damage") { *outIsStrip = true; *outEnumValue = 5; return true; }
    if (lower == "victory") { *outIsStrip = true; *outEnumValue = 6; return true; }
    if (lower == "idle") { *outIsStrip = true; *outEnumValue = 7; return true; }
    if (lower == "rainbow") { *outIsStrip = true; *outEnumValue = 8; return true; }
    if (lower == "rainbow_chase" || lower == "rainbowchase") { *outIsStrip = true; *outEnumValue = 9; return true; }
    if (lower == "color_wipe" || lower == "colorwipe") { *outIsStrip = true; *outEnumValue = 10; return true; }
    if (lower == "theater_chase" || lower == "theaterchase" || lower == "theater") { *outIsStrip = true; *outEnumValue = 11; return true; }
    if (lower == "pulse") { *outIsStrip = true; *outEnumValue = 12; return true; }
    if (lower == "breathing") { *outIsStrip = true; *outEnumValue = 13; return true; }
    if (lower == "meteor") { *outIsStrip = true; *outEnumValue = 14; return true; }
    if (lower == "twinkle") { *outIsStrip = true; *outEnumValue = 15; return true; }
    if (lower == "water") { *outIsStrip = true; *outEnumValue = 16; return true; }
    if (lower == "strobe") { *outIsStrip = true; *outEnumValue = 17; return true; }
    if (lower == "radial_out" || lower == "radialout") { *outIsStrip = true; *outEnumValue = 18; return true; }
    if (lower == "radial_in" || lower == "radialin") { *outIsStrip = true; *outEnumValue = 19; return true; }
    if (lower == "spiral") { *outIsStrip = true; *outEnumValue = 20; return true; }
    if (lower == "rotating_rainbow" || lower == "rotatingrainbow") { *outIsStrip = true; *outEnumValue = 21; return true; }
    if (lower == "circular_chase" || lower == "circularchase") { *outIsStrip = true; *outEnumValue = 22; return true; }
    if (lower == "radial_gradient" || lower == "radialgradient") { *outIsStrip = true; *outEnumValue = 23; return true; }
    // Matrix effects: enum values 1-28 (MatrixLEDController::EffectType)
    if (lower == "beam_attack" || lower == "beam") { *outIsStrip = false; *outEnumValue = 1; return true; }
    if (lower == "explosion") { *outIsStrip = false; *outEnumValue = 2; return true; }
    if (lower == "impact_wave" || lower == "impact") { *outIsStrip = false; *outEnumValue = 3; return true; }
    if (lower == "damage_flash" || lower == "damageflash") { *outIsStrip = false; *outEnumValue = 4; return true; }
    if (lower == "block_shield" || lower == "block" || lower == "shield") { *outIsStrip = false; *outEnumValue = 5; return true; }
    if (lower == "dodge_trail" || lower == "dodge") { *outIsStrip = false; *outEnumValue = 6; return true; }
    if (lower == "charge_up" || lower == "charge") { *outIsStrip = false; *outEnumValue = 7; return true; }
    if (lower == "finisher_beam" || lower == "finisher") { *outIsStrip = false; *outEnumValue = 8; return true; }
    if (lower == "gravity_beam_attack" || lower == "ghidorah_beam" || lower == "ghidorah_gravity") { *outIsStrip = false; *outEnumValue = 9; return true; }
    if (lower == "electric_attack_matrix" || lower == "electric_matrix") { *outIsStrip = false; *outEnumValue = 10; return true; }
    if (lower == "victory_dance" || lower == "victorydance") { *outIsStrip = false; *outEnumValue = 11; return true; }
    if (lower == "taunt_pattern" || lower == "taunt") { *outIsStrip = false; *outEnumValue = 12; return true; }
    if (lower == "pose_strike" || lower == "pose") { *outIsStrip = false; *outEnumValue = 13; return true; }
    if (lower == "celebration_wave" || lower == "celebration") { *outIsStrip = false; *outEnumValue = 14; return true; }
    if (lower == "confetti") { *outIsStrip = false; *outEnumValue = 15; return true; }
    if (lower == "heart_eyes" || lower == "heart") { *outIsStrip = false; *outEnumValue = 16; return true; }
    if (lower == "power_up_aura" || lower == "powerup" || lower == "aura") { *outIsStrip = false; *outEnumValue = 17; return true; }
    if (lower == "transition_fade" || lower == "transition") { *outIsStrip = false; *outEnumValue = 18; return true; }
    if (lower == "game_over_chiron" || lower == "gameover" || lower == "chiron") { *outIsStrip = false; *outEnumValue = 19; return true; }
    if (lower == "cylon_eye" || lower == "cylon") { *outIsStrip = false; *outEnumValue = 20; return true; }
    if (lower == "perlin_inferno" || lower == "inferno" || lower == "perlin") { *outIsStrip = false; *outEnumValue = 21; return true; }
    if (lower == "emp_lightning" || lower == "lightning" || lower == "emp") { *outIsStrip = false; *outEnumValue = 22; return true; }
    if (lower == "game_of_life" || lower == "gol" || lower == "life") { *outIsStrip = false; *outEnumValue = 23; return true; }
    if (lower == "plasma_clouds" || lower == "plasma") { *outIsStrip = false; *outEnumValue = 24; return true; }
    if (lower == "digital_fireflies" || lower == "fireflies") { *outIsStrip = false; *outEnumValue = 25; return true; }
    if (lower == "matrix_rain" || lower == "matrix") { *outIsStrip = false; *outEnumValue = 26; return true; }
    if (lower == "dance_floor" || lower == "disco") { *outIsStrip = false; *outEnumValue = 27; return true; }
    if (lower == "alien_control_panel" || lower == "alien_panel" || lower == "spaceship_panel") { *outIsStrip = false; *outEnumValue = 28; return true; }
    if (lower == "atomic_breath_minus_one" || lower == "godzilla_minus_one" || lower == "minus_one_breath" || lower == "minus_one") { *outIsStrip = false; *outEnumValue = 29; return true; }
    if (lower == "atomic_breath_mushroom" || lower == "mushroom_cloud" || lower == "finishing_move" || lower == "mushroom") { *outIsStrip = false; *outEnumValue = 30; return true; }
    if (lower == "transporter_tos" || lower == "transporter" || lower == "star_trek_transporter" || lower == "tos_transporter") { *outIsStrip = false; *outEnumValue = 31; return true; }
    return false;
}
