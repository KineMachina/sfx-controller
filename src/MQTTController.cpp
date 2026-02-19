#include "MQTTController.h"
#include "EffectDispatch.h"
#include <FreeRTOS.h>
#include <semphr.h>
#include <string.h>
#include <ESPmDNS.h>

// Static instance pointer for callbacks
MQTTController* MQTTController::instance = nullptr;

/** Return human-readable string for disconnect reason (for diagnostics). */
static const char* getDisconnectReasonString(AsyncMqttClientDisconnectReason reason) {
    switch (reason) {
        case AsyncMqttClientDisconnectReason::TCP_DISCONNECTED:
            return "TCP_DISCONNECTED (network/connection lost)";
        case AsyncMqttClientDisconnectReason::MQTT_UNACCEPTABLE_PROTOCOL_VERSION:
            return "MQTT_UNACCEPTABLE_PROTOCOL_VERSION (broker rejected protocol)";
        case AsyncMqttClientDisconnectReason::MQTT_IDENTIFIER_REJECTED:
            return "MQTT_IDENTIFIER_REJECTED (client ID rejected)";
        case AsyncMqttClientDisconnectReason::MQTT_SERVER_UNAVAILABLE:
            return "MQTT_SERVER_UNAVAILABLE (broker overloaded or unavailable)";
        case AsyncMqttClientDisconnectReason::MQTT_MALFORMED_CREDENTIALS:
            return "MQTT_MALFORMED_CREDENTIALS (bad username/password format)";
        case AsyncMqttClientDisconnectReason::MQTT_NOT_AUTHORIZED:
            return "MQTT_NOT_AUTHORIZED (auth failed - check username/password)";
        case AsyncMqttClientDisconnectReason::ESP8266_NOT_ENOUGH_SPACE:
            return "ESP8266_NOT_ENOUGH_SPACE (memory)";
        case AsyncMqttClientDisconnectReason::TLS_BAD_FINGERPRINT:
            return "TLS_BAD_FINGERPRINT (TLS cert mismatch)";
        default:
            return "UNKNOWN";
    }
}

MQTTController::MQTTController() 
    : configLoaded(false)
    , connected(false)
    , connectionInProgress(false)
    , lastReconnectAttempt(0)
    , reconnectDelay(RECONNECT_DELAY_MIN)
    , reconnectAttemptCount(0)
    , lastWiFiWaitLog(0)
    , audioController(nullptr)
    , ledStripController(nullptr)
    , ledMatrixController(nullptr)
    , demoController(nullptr)
    , settingsController(nullptr)
    , bassMonoProcessor(nullptr)
    , lastAudioPlaying(false)
    , lastVolume(0)
    , lastBrightness(0)
    , lastLEDEffect(-1)
    , lastLEDRunning(false)
    , lastHealthPublish(0)
    , mqttTaskHandle(NULL)
    , mqttCommandQueue(NULL)
    , currentTotalLen(0)
    , brokerIPResolved(false)
    , messageBufferLen(0)
{
    // Initialize buffers
    messageBuffer[0] = '\0';
    currentTopic[0] = '\0';
    // Initialize topic strings
    topicCommand[0] = '\0';
    topicStatus[0] = '\0';
    topicStatusAudio[0] = '\0';
    topicStatusLED[0] = '\0';
    topicStatusHealth[0] = '\0';
    topicResponse[0] = '\0';
    topicLWT[0] = '\0';
}

MQTTController::~MQTTController() {
    if (connected) {
        mqttClient.disconnect(true);
    }
    
    // Delete task and queue
    if (mqttTaskHandle != NULL) {
        vTaskDelete(mqttTaskHandle);
        mqttTaskHandle = NULL;
    }
    
    if (mqttCommandQueue != NULL) {
        vQueueDelete(mqttCommandQueue);
        mqttCommandQueue = NULL;
    }
}

bool MQTTController::begin(AudioController* audioCtrl, 
                           StripLEDController* ledStripCtrl,
                           MatrixLEDController* ledMatrixCtrl,
                           DemoController* demoCtrl,
                           SettingsController* settingsCtrl,
                           SemaphoreHandle_t sdMutex) {
    audioController = audioCtrl;
    ledStripController = ledStripCtrl;
    ledMatrixController = ledMatrixCtrl;
    demoController = demoCtrl;
    settingsController = settingsCtrl;
    
    // Set static instance for callbacks
    instance = this;
    
    // Load MQTT configuration
    if (!settingsController) {
        Serial.println("[MQTT] Settings controller not available");
        return false;
    }
    
    configLoaded = settingsController->loadMQTTConfig(&config, sdMutex);
    
    if (!configLoaded || !config.enabled) {
        Serial.println("[MQTT] MQTT disabled or not configured");
        return false;
    }
    
    if (strlen(config.broker) == 0) {
        Serial.println("[MQTT] MQTT broker not configured");
        return false;
    }
    
    // Build topic strings
    buildTopics();
    
    // Resolve broker hostname to IP address (supports mDNS .local domains)
    brokerIP = resolveHostname(config.broker);
    if (brokerIP == IPAddress(0, 0, 0, 0)) {
        Serial.print("[MQTT] ERROR: Failed to resolve broker hostname: ");
        Serial.println(config.broker);
        Serial.println("[MQTT] Make sure broker is on the network and mDNS is working");
        return false;
    }
    
    brokerIPResolved = true;
    Serial.print("[MQTT] Resolved broker hostname '");
    Serial.print(config.broker);
    Serial.print("' to IP: ");
    Serial.println(brokerIP);
    
    // Setup MQTT client callbacks
    mqttClient.onConnect(onMqttConnectWrapper);
    mqttClient.onDisconnect(onMqttDisconnectWrapper);
    mqttClient.onMessage(onMqttMessageWrapper);
    
    // Set server using resolved IP address
    mqttClient.setServer(brokerIP, config.port);
    if (strlen(config.username) > 0) {
        mqttClient.setCredentials(config.username, config.password);
    }
    
    // Set keepalive
    mqttClient.setKeepAlive(config.keepalive);
    
    // Set client ID (use device ID if available, otherwise use MAC address)
    String clientId = "kinemachina_";
    if (strlen(config.deviceId) > 0) {
        clientId += config.deviceId;
    } else {
        clientId += WiFi.macAddress();
        clientId.replace(":", "");
    }
    mqttClient.setClientId(clientId.c_str());
    
    // Set Last Will and Testament (LWT)
    mqttClient.setWill(topicLWT, 0, true, "offline");
    
    // Create command queue (10 items should be enough)
    mqttCommandQueue = xQueueCreate(10, sizeof(MQTTCommand));
    if (mqttCommandQueue == NULL) {
        Serial.println("[MQTT] ERROR: Failed to create command queue!");
        return false;
    }
    
    // Create MQTT processing task
    // Priority 2 (same as LED task), stack 16KB, pinned to Core 1
    // Increased stack size to handle JSON parsing and String operations
    BaseType_t taskResult = xTaskCreatePinnedToCore(
        mqttTaskWrapper,
        "MQTTTask",
        16384,  // 16KB stack (increased from 8KB to prevent overflow)
        this,
        2,     // Priority 2
        &mqttTaskHandle,
        1      // Core 1
    );
    
    if (taskResult != pdPASS) {
        Serial.println("[MQTT] ERROR: Failed to create MQTT task!");
        vQueueDelete(mqttCommandQueue);
        mqttCommandQueue = NULL;
        return false;
    }
    
    Serial.println("[MQTT] -------- Config --------");
    Serial.print("[MQTT] Broker: ");
    Serial.print(config.broker);
    Serial.print(" -> ");
    Serial.print(brokerIP);
    Serial.print(":");
    Serial.println(config.port);
    Serial.print("[MQTT] Device ID: ");
    Serial.println(strlen(config.deviceId) > 0 ? config.deviceId : "(from MAC)");
    Serial.print("[MQTT] Base topic: ");
    Serial.println(strlen(config.baseTopic) > 0 ? config.baseTopic : "kinemachina/sfx");
    Serial.print("[MQTT] Auth: ");
    Serial.println(strlen(config.username) > 0 ? "yes" : "no");
    Serial.println("[MQTT] Command queue and task created");
    Serial.println("[MQTT] ------------------------");
    Serial.println("[MQTT] Attempting initial connection...");

    // Attempt initial connection
    reconnect();

    return true;
}

IPAddress MQTTController::resolveHostname(const char* hostname) {
    // Check if it's already an IP address
    IPAddress ip;
    if (ip.fromString(hostname)) {
        Serial.print("[MQTT] Broker is already an IP address: ");
        Serial.println(ip);
        return ip;
    }
    
    Serial.print("[MQTT] Resolving hostname: ");
    Serial.println(hostname);
    
    // Try mDNS resolution first (for .local domains)
    if (strstr(hostname, ".local") != NULL) {
        Serial.println("[MQTT] Attempting mDNS resolution...");
        
        // Wait a bit for mDNS to be ready (if WiFi just connected)
        delay(1000);
        
        // Query mDNS
        IPAddress mdnsIP = MDNS.queryHost(hostname);
        if (mdnsIP != IPAddress(0, 0, 0, 0)) {
            Serial.print("[MQTT] mDNS resolution successful: ");
            Serial.println(mdnsIP);
            return mdnsIP;
        } else {
            Serial.println("[MQTT] mDNS resolution failed (is broker advertising .local?), trying DNS...");
        }
    }
    
    // Fallback to regular DNS resolution
    Serial.println("[MQTT] Attempting DNS resolution...");
    IPAddress dnsIP;
    int result = WiFi.hostByName(hostname, dnsIP);
    
    if (result == 1) {
        Serial.print("[MQTT] DNS resolution successful: ");
        Serial.println(dnsIP);
        return dnsIP;
    } else {
        Serial.println("[MQTT] ERROR: DNS resolution failed");
        Serial.println("[MQTT] Check: broker hostname correct, network has DNS, broker is reachable");
        return IPAddress(0, 0, 0, 0);
    }
}

void MQTTController::buildTopics() {
    String base = String(config.baseTopic);
    if (base.length() == 0) {
        base = "kinemachina/sfx";
    }
    
    String deviceId = String(config.deviceId);
    if (deviceId.length() == 0) {
        deviceId = "kinemachina_001";
    }
    
    // Command topic (single topic, JSON dispatch)
    snprintf(topicCommand, sizeof(topicCommand), "%s/%s/command", base.c_str(), deviceId.c_str());

    // Status topics
    snprintf(topicStatus, sizeof(topicStatus), "%s/%s/status", base.c_str(), deviceId.c_str());
    snprintf(topicStatusAudio, sizeof(topicStatusAudio), "%s/%s/status/audio", base.c_str(), deviceId.c_str());
    snprintf(topicStatusLED, sizeof(topicStatusLED), "%s/%s/status/led", base.c_str(), deviceId.c_str());
    snprintf(topicStatusHealth, sizeof(topicStatusHealth), "%s/%s/status/health", base.c_str(), deviceId.c_str());
    
    // Response topic
    snprintf(topicResponse, sizeof(topicResponse), "%s/%s/response", base.c_str(), deviceId.c_str());
    
    // LWT topic
    snprintf(topicLWT, sizeof(topicLWT), "%s/%s/status/online", base.c_str(), deviceId.c_str());
}

void MQTTController::update() {
    if (!configLoaded || !config.enabled) {
        return;
    }

    unsigned long now = millis();

    // Connection timeout: if we called connect() but got no response, treat as failure and allow retry
    if (connectionInProgress && (now - lastReconnectAttempt >= CONNECT_TIMEOUT_MS)) {
        Serial.println("[MQTT] -------- Connection timeout --------");
        Serial.print("[MQTT] No response from broker after ");
        Serial.print(CONNECT_TIMEOUT_MS / 1000);
        Serial.println("s - will retry with backoff");
        connectionInProgress = false;
        unsigned long prev = reconnectDelay;
        reconnectDelay = (reconnectDelay * 2 > RECONNECT_DELAY_MAX) ? RECONNECT_DELAY_MAX : (reconnectDelay * 2);
        Serial.print("[MQTT] Next retry in ");
        Serial.print(reconnectDelay / 1000);
        Serial.println("s");
        Serial.println("[MQTT] -------------------------------------");
        lastReconnectAttempt = now;  // So next attempt waits reconnectDelay
    }

    // When WiFi is down, log occasionally so user knows MQTT is waiting
    if (!connected && WiFi.status() != WL_CONNECTED) {
        if (now - lastWiFiWaitLog >= WIFI_WAIT_LOG_INTERVAL) {
            lastWiFiWaitLog = now;
            Serial.println("[MQTT] Disconnected: waiting for WiFi before reconnecting");
        }
        return;
    }

    // Handle reconnection (only when WiFi is up and not already connecting)
    if (!connected && !connectionInProgress && WiFi.status() == WL_CONNECTED) {
        if (now - lastReconnectAttempt >= reconnectDelay) {
            lastReconnectAttempt = now;
            reconnect();
        }
    }

    // Check for state changes and publish status
    if (connected) {
        publishStatus(false);

        // Publish health check periodically
        if (now - lastHealthPublish >= HEALTH_PUBLISH_INTERVAL) {
            lastHealthPublish = now;
            publishHealth();
        }
    }
}

void MQTTController::reconnect() {
    if (connected || connectionInProgress) {
        return;
    }

    reconnectAttemptCount++;
    connectionInProgress = true;

    // Re-resolve hostname if needed: first time, or IP invalid, or every N failed attempts (broker IP may have changed)
    bool needResolve = (!brokerIPResolved || brokerIP == IPAddress(0, 0, 0, 0)) ||
                       (reconnectAttemptCount > 1 && (reconnectAttemptCount % RESOLVE_RETRY_INTERVAL) == 0);
    if (needResolve) {
        if (reconnectAttemptCount > 1) {
            Serial.print("[MQTT] Re-resolving hostname (attempt #");
            Serial.print(reconnectAttemptCount);
            Serial.println(")");
        }
        brokerIP = resolveHostname(config.broker);
        if (brokerIP == IPAddress(0, 0, 0, 0)) {
            Serial.println("[MQTT] ERROR: Broker hostname resolution failed - will retry after backoff");
            connectionInProgress = false;
            lastReconnectAttempt = millis();  // Wait reconnectDelay before next attempt
            return;
        }
        brokerIPResolved = true;
        mqttClient.setServer(brokerIP, config.port);
    }

    Serial.println("[MQTT] -------- Reconnect --------");
    Serial.print("[MQTT] Attempt #");
    Serial.print(reconnectAttemptCount);
    Serial.print(", backoff ");
    Serial.print(reconnectDelay / 1000);
    Serial.println("s");
    Serial.print("[MQTT] Target: ");
    Serial.print(config.broker);
    Serial.print(" -> ");
    Serial.print(brokerIP);
    Serial.print(":");
    Serial.println(config.port);
    if (strlen(config.username) > 0) {
        Serial.println("[MQTT] Auth: username set");
    } else {
        Serial.println("[MQTT] Auth: none");
    }
    Serial.println("[MQTT] --------------------------");

    if (mqttClient.connected()) {
        connected = true;
        connectionInProgress = false;
        return;
    }

    // AsyncMqttClient.connect() returns void; result comes in onMqttConnect or onMqttDisconnect
    mqttClient.connect();
}

void MQTTController::onMqttConnect(bool sessionPresent) {
    connectionInProgress = false;
    connected = true;
    reconnectDelay = RECONNECT_DELAY_MIN;  // Reset backoff on success
    reconnectAttemptCount = 0;             // Reset attempt count

    Serial.println("[MQTT] -------- Connected --------");
    Serial.print("[MQTT] Broker: ");
    Serial.print(config.broker);
    Serial.print(":");
    Serial.println(config.port);
    Serial.print("[MQTT] Session present: ");
    Serial.println(sessionPresent ? "yes" : "no");
    Serial.println("[MQTT] ---------------------------");

    // Publish online status
    mqttClient.publish(topicLWT, 0, true, "online");
    Serial.println("[MQTT] Published LWT: online");

    // Subscribe to command topics
    subscribeToCommands();

    // Publish initial status
    publishStatus(true);
    publishHealth();
}

void MQTTController::onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
    connectionInProgress = false;
    connected = false;

    Serial.println("[MQTT] -------- Disconnect --------");
    Serial.print("[MQTT] Reason code: ");
    Serial.println((int)reason);
    Serial.print("[MQTT] Reason: ");
    Serial.println(getDisconnectReasonString(reason));

    // Exponential backoff: increase delay for next attempt (cap at RECONNECT_DELAY_MAX)
    unsigned long previousDelay = reconnectDelay;
    reconnectDelay = (reconnectDelay * 2 > RECONNECT_DELAY_MAX) ? RECONNECT_DELAY_MAX : (reconnectDelay * 2);
    if (reconnectDelay != previousDelay) {
        Serial.print("[MQTT] Backoff: next retry in ");
        Serial.print(reconnectDelay / 1000);
        Serial.println("s");
    }
    Serial.println("[MQTT] ---------------------------");
}

void MQTTController::subscribeToCommands() {
    uint16_t packetId = mqttClient.subscribe(topicCommand, config.qosCommands);
    Serial.print("[MQTT] Subscribed to commands (packet ID: ");
    Serial.print(packetId);
    Serial.println(")");
}

void MQTTController::onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
    // Handle multi-part messages
    // Use char arrays instead of String for thread safety
    if (index == 0) {
        // First chunk - initialize buffer
        messageBuffer[0] = '\0';
        messageBufferLen = 0;
        strncpy(currentTopic, topic, sizeof(currentTopic) - 1);
        currentTopic[sizeof(currentTopic) - 1] = '\0';
        currentTotalLen = total;
    }
    
    // Append this chunk to buffer (check bounds)
    size_t remainingSpace = sizeof(messageBuffer) - messageBufferLen - 1;
    size_t copyLen = (len < remainingSpace) ? len : remainingSpace;
    
    if (copyLen > 0) {
        memcpy(messageBuffer + messageBufferLen, payload, copyLen);
        messageBufferLen += copyLen;
        messageBuffer[messageBufferLen] = '\0';
    }
    
    // If this is the last chunk, enqueue the complete message
    if (index + len >= total) {
        // Log every received message
        Serial.print("[MQTT] Message received topic=");
        Serial.print(currentTopic);
        Serial.print(" len=");
        Serial.println(total);
        if (messageBufferLen > 0 && messageBufferLen <= 120) {
            Serial.print("[MQTT] Payload: ");
            for (size_t i = 0; i < messageBufferLen && i < 120; i++) {
                char c = messageBuffer[i];
                Serial.print((c >= 32 && c < 127) ? c : '.');
            }
            Serial.println();
        }
        if (strcmp(currentTopic, topicCommand) != 0) {
            Serial.print("[MQTT] Unknown topic: ");
            Serial.println(currentTopic);
            messageBuffer[0] = '\0';
            messageBufferLen = 0;
            currentTopic[0] = '\0';
            return;
        }

        // Create command structure
        MQTTCommand cmd;

        // Copy payload (limit to buffer size)
        size_t payloadSize = messageBufferLen;
        if (payloadSize > sizeof(cmd.payload) - 1) {
            payloadSize = sizeof(cmd.payload) - 1;
            Serial.println("[MQTT] WARNING: Payload truncated!");
        }
        strncpy(cmd.payload, messageBuffer, payloadSize);
        cmd.payload[payloadSize] = '\0';
        cmd.payloadLen = payloadSize;
        
        // Enqueue command (non-blocking, timeout 0)
        if (mqttCommandQueue != NULL) {
            BaseType_t result = xQueueSend(mqttCommandQueue, &cmd, 0);
            if (result != pdTRUE) {
                Serial.println("[MQTT] WARNING: Command queue full, dropping message!");
            }
        }
        
        // Clear buffer
        messageBuffer[0] = '\0';
        messageBufferLen = 0;
        currentTopic[0] = '\0';
        currentTotalLen = 0;
    }
}

// Task wrapper function
void MQTTController::mqttTaskWrapper(void* parameter) {
    MQTTController* controller = static_cast<MQTTController*>(parameter);
    controller->mqttTask();
}

void MQTTController::mqttTask() {
    Serial.println("[MQTT] MQTT task started on Core 1");
    
    MQTTCommand cmd;
    
    while (true) {
        // Wait for command from queue (blocking)
        if (mqttCommandQueue != NULL && 
            xQueueReceive(mqttCommandQueue, &cmd, portMAX_DELAY) == pdTRUE) {
            
            // Process the command
            processCommand(cmd);
        }
        
        // Small delay to prevent watchdog issues
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void MQTTController::processCommand(const MQTTCommand& cmd) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, cmd.payload, cmd.payloadLen);
    if (error) {
        Serial.print("[MQTT] JSON parse error: ");
        Serial.println(error.c_str());
        publishResponse("unknown", false, "Invalid JSON");
        return;
    }

    const char* command = doc["command"] | "";

    if (strcasecmp(command, "trigger") == 0) {
        handleCommandTrigger(cmd.payload, cmd.payloadLen);
    } else if (strcasecmp(command, "stop") == 0) {
        handleCommandStop(cmd.payload, cmd.payloadLen);
    } else if (strcasecmp(command, "volume") == 0) {
        handleCommandVolume(cmd.payload, cmd.payloadLen);
    } else if (strcasecmp(command, "brightness") == 0) {
        handleCommandBrightness(cmd.payload, cmd.payloadLen);
    } else if (strcasecmp(command, "bass_mono") == 0 || strcasecmp(command, "bassMono") == 0) {
        handleCommandBassMono(cmd.payload, cmd.payloadLen);
    } else {
        String msg = "Unknown command: ";
        msg += command;
        publishResponse(command, false, msg.c_str(),
                        doc["request_id"].is<const char*>() ? doc["request_id"].as<const char*>() : nullptr);
    }
}

// Static wrapper functions
void MQTTController::onMqttConnectWrapper(bool sessionPresent) {
    if (instance) {
        instance->onMqttConnect(sessionPresent);
    }
}

void MQTTController::onMqttDisconnectWrapper(AsyncMqttClientDisconnectReason reason) {
    if (instance) {
        instance->onMqttDisconnect(reason);
    }
}

void MQTTController::onMqttMessageWrapper(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
    if (instance) {
        instance->onMqttMessage(topic, payload, properties, len, index, total);
    }
}

void MQTTController::handleCommandTrigger(const char* payload, size_t len) {
    Serial.println("[MQTT] Trigger command received");
    
    if (!audioController || (!ledStripController && !ledMatrixController) || !settingsController) {
        publishResponse("trigger", false, "Controller not available");
        return;
    }
    
    // Parse JSON
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload, len);
    
    if (error) {
        Serial.print("[MQTT] JSON parse error: ");
        Serial.println(error.c_str());
        publishResponse("trigger", false, "Invalid JSON", nullptr);
        return;
    }
    
    // Extract parameters
    String category = doc["category"].is<String>() ? doc["category"].as<String>() : "";
    String effectName = "";
    if (doc["effect"].is<String>() && !doc["effect"].isNull()) {
        effectName = doc["effect"].as<String>();
    }
    String priority = doc["priority"].is<String>() ? doc["priority"].as<String>() : "";
    int duration = doc["duration"].is<int>() ? doc["duration"].as<int>() : 0;
    int audioVolume = doc["audio_volume"].is<int>() ? doc["audio_volume"].as<int>() : -1;
    int ledBrightness = doc["led_brightness"].is<int>() ? doc["led_brightness"].as<int>() : -1;
    String requestId = doc["request_id"].is<String>() ? doc["request_id"].as<String>() : "";

    category.toLowerCase();
    if (effectName.length() > 0) {
        effectName.toLowerCase();
    }
    priority.toLowerCase();

    // Find effect by category or name
    SettingsController::Effect effect;
    bool effectFound = false;
    
    // Only search by name if effectName is not empty and not "null"
    if (effectName.length() > 0 && effectName != "null") {
        effectFound = settingsController->getEffectByName(effectName.c_str(), &effect);
        // Fallback: treat as LED-only effect (same names as HTTP /api/led/effect)
        if (!effectFound) {
            bool isStrip = false;
            int enumVal = 0;
            if (settingsController->getLEDEffectFromName(effectName.c_str(), &isStrip, &enumVal)) {
                effectFound = true;
                strncpy(effect.name, effectName.c_str(), sizeof(effect.name) - 1);
                effect.name[sizeof(effect.name) - 1] = '\0';
                strncpy(effect.ledEffectName, effectName.c_str(), sizeof(effect.ledEffectName) - 1);
                effect.ledEffectName[sizeof(effect.ledEffectName) - 1] = '\0';
                effect.hasAudio = false;
                effect.hasLED = true;
                effect.audioFile[0] = '\0';
                strncpy(effect.category, "led", sizeof(effect.category) - 1);
                effect.category[sizeof(effect.category) - 1] = '\0';
            }
        }
    } else if (category.length() > 0) {
        SettingsController::Effect categoryEffects[32];
        int count = settingsController->getEffectsByCategory(category.c_str(), categoryEffects, 32);
        
        if (count > 0) {
            int index = random(count);
            effect = categoryEffects[index];
            effectFound = true;
        }
    } else {
        publishResponse("trigger", false, "Must specify category or effect", requestId.length() > 0 ? requestId.c_str() : nullptr);
        return;
    }
    
    if (!effectFound) {
        publishResponse("trigger", false, "Effect not found", requestId.length() > 0 ? requestId.c_str() : nullptr);
        return;
    }
    
    // Apply optional parameters
    bool audioPlayed = false;
    bool ledStarted = false;
    
    if (audioVolume >= 0 && audioVolume <= 21) {
        audioController->setVolume(audioVolume);
        settingsController->saveVolume(audioVolume);
    }
    
    if (ledBrightness >= 0 && ledBrightness <= 255) {
        if (ledStripController) {
            ledStripController->setBrightness((uint8_t)ledBrightness);
        }
        if (ledMatrixController) {
            ledMatrixController->setBrightness((uint8_t)ledBrightness);
        }
        settingsController->saveBrightness((uint8_t)ledBrightness);
    }
    
    // Execute effect
    if (effect.hasAudio) {
        if (audioController->playFile(effect.audioFile)) {
            audioPlayed = true;
        }
    }
    
    if (effect.hasLED) {
        if (dispatchLEDEffect(settingsController, effect.ledEffectName, ledStripController, ledMatrixController)) {
            ledStarted = true;
        }
    }
    
    // Build and publish response
    JsonDocument responseDoc;
    responseDoc["status"] = "success";
    responseDoc["command"] = "trigger";
    responseDoc["effect"] = effect.name;
    responseDoc["category"] = effect.category;
    responseDoc["executed"] = true;
    responseDoc["audio_played"] = audioPlayed;
    responseDoc["led_started"] = ledStarted;
    responseDoc["message"] = "Effect triggered successfully";
    responseDoc["timestamp"] = millis();
    if (requestId.length() > 0) {
        responseDoc["request_id"] = requestId;
    }
    responseDoc["error"] = nullptr;
    
    String response;
    serializeJson(responseDoc, response);
    mqttClient.publish(topicResponse, config.qosCommands, false, response.c_str());
    Serial.print("[MQTT] Trigger completed: effect=");
    Serial.print(effect.name);
    Serial.print(" category=");
    Serial.println(effect.category);
    
    // Publish updated status
    publishStatus(true);
}

void MQTTController::handleCommandStop(const char* payload, size_t len) {
    Serial.println("[MQTT] Stop command received");
    
    // Stop all audio and LED effects
    if (audioController) {
        audioController->stop();
    }
    
    if (ledStripController) {
        ledStripController->stopEffect();
    }
    if (ledMatrixController) {
        ledMatrixController->stopEffect();
    }
    
    // Parse request ID if present
    String requestId = "";
    if (len > 0) {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, payload, len);
        if (!error && doc["request_id"].is<String>()) {
            requestId = doc["request_id"].as<String>();
        }
    }
    
    // Build and publish response
    JsonDocument responseDoc;
    responseDoc["status"] = "success";
    responseDoc["command"] = "stop";
    responseDoc["message"] = "All effects stopped";
    responseDoc["timestamp"] = millis();
    if (requestId.length() > 0) {
        responseDoc["request_id"] = requestId;
    }
    
    String response;
    serializeJson(responseDoc, response);
    mqttClient.publish(topicResponse, config.qosCommands, false, response.c_str());
    Serial.println("[MQTT] Stop completed");
    
    // Publish updated status
    publishStatus(true);
}

void MQTTController::handleCommandVolume(const char* payload, size_t len) {
    Serial.println("[MQTT] Volume command received");
    
    if (!audioController) {
        publishResponse("volume", false, "Audio controller not available");
        return;
    }
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload, len);
    
    if (error) {
        publishResponse("volume", false, "Invalid JSON");
        return;
    }
    
    int volume = doc["volume"].is<int>() ? doc["volume"].as<int>() : -1;
    String requestId = doc["request_id"].is<String>() ? doc["request_id"].as<String>() : "";
    
    if (volume < 0 || volume > 21) {
        publishResponse("volume", false, "Volume must be 0-21", requestId.length() > 0 ? requestId.c_str() : nullptr);
        return;
    }
    
    audioController->setVolume(volume);
    settingsController->saveVolume(volume);
    
    Serial.print("[MQTT] Volume set to ");
    Serial.println(volume);
    publishResponse("volume", true, "Volume set successfully", requestId.length() > 0 ? requestId.c_str() : nullptr);
    publishStatus(true);
}

void MQTTController::handleCommandBrightness(const char* payload, size_t len) {
    Serial.println("[MQTT] Brightness command received");
    
    if (!ledStripController && !ledMatrixController) {
        publishResponse("brightness", false, "LED controllers not available");
        return;
    }
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload, len);
    
    if (error) {
        publishResponse("brightness", false, "Invalid JSON");
        return;
    }
    
    int brightness = doc["brightness"].is<int>() ? doc["brightness"].as<int>() : -1;
    String requestId = doc["request_id"].is<String>() ? doc["request_id"].as<String>() : "";
    
    if (brightness < 0 || brightness > 255) {
        publishResponse("brightness", false, "Brightness must be 0-255", requestId.length() > 0 ? requestId.c_str() : nullptr);
        return;
    }
    
    if (ledStripController) {
        ledStripController->setBrightness((uint8_t)brightness);
    }
    if (ledMatrixController) {
        ledMatrixController->setBrightness((uint8_t)brightness);
    }
    settingsController->saveBrightness((uint8_t)brightness);
    
    Serial.print("[MQTT] Brightness set to ");
    Serial.println(brightness);
    publishResponse("brightness", true, "Brightness set successfully", requestId.length() > 0 ? requestId.c_str() : nullptr);
    publishStatus(true);
}

void MQTTController::handleCommandBassMono(const char* payload, size_t len) {
    Serial.println("[MQTT] Bass mono command received");

    if (!bassMonoProcessor) {
        publishResponse("bass_mono", false, "Bass mono processor not available");
        return;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload, len);

    if (error) {
        publishResponse("bass_mono", false, "Invalid JSON");
        return;
    }

    String requestId = doc["request_id"].is<String>() ? doc["request_id"].as<String>() : "";

    if (doc["enabled"].is<bool>()) {
        bool enabled = doc["enabled"].as<bool>();
        bassMonoProcessor->setEnabled(enabled);
        if (settingsController) {
            settingsController->saveBassMonoEnabled(enabled);
        }
    }

    if (doc["crossover_hz"].is<int>()) {
        int hz = doc["crossover_hz"].as<int>();
        if (hz >= 20 && hz <= 500) {
            bassMonoProcessor->setCrossoverHz((uint16_t)hz);
            if (settingsController) {
                settingsController->saveBassMonoCrossover((uint16_t)hz);
            }
        } else {
            publishResponse("bass_mono", false, "crossover_hz must be 20-500", requestId.length() > 0 ? requestId.c_str() : nullptr);
            return;
        }
    }

    Serial.print("[MQTT] Bass mono: enabled=");
    Serial.print(bassMonoProcessor->isEnabled() ? "true" : "false");
    Serial.print(", crossover=");
    Serial.println(bassMonoProcessor->getCrossoverHz());
    publishResponse("bass_mono", true, "Bass mono settings updated", requestId.length() > 0 ? requestId.c_str() : nullptr);
    publishStatus(true);
}

void MQTTController::publishResponse(const char* command, bool success, const char* message, const char* requestId) {
    JsonDocument doc;
    doc["status"] = success ? "success" : "error";
    doc["command"] = command;
    doc["executed"] = success;
    doc["message"] = message;
    doc["timestamp"] = millis();
    if (requestId) {
        doc["request_id"] = requestId;
    }
    if (!success) {
        doc["error"] = message;
    }
    
    String response;
    serializeJson(doc, response);
    mqttClient.publish(topicResponse, config.qosCommands, false, response.c_str());
    Serial.print("[MQTT] Published response command=");
    Serial.print(command);
    Serial.print(" success=");
    Serial.println(success ? "true" : "false");
}

void MQTTController::publishStatus(bool force) {
    if (!connected) {
        return;
    }
    
    // Get current state
    bool audioPlaying = audioController ? audioController->isPlaying() : false;
    int volume = audioController ? audioController->getVolume() : 0;
    uint8_t brightness = ledStripController ? ledStripController->getBrightness() : (ledMatrixController ? ledMatrixController->getBrightness() : 0);
    bool ledRunning = (ledStripController && ledStripController->isEffectRunning()) || (ledMatrixController && ledMatrixController->isEffectRunning());
    int ledEffect = ledStripController ? (int)ledStripController->getCurrentEffect() : (ledMatrixController ? (int)ledMatrixController->getCurrentEffect() : -1);
    
    // Check for state changes (before updating last state)
    bool audioChanged = (audioPlaying != lastAudioPlaying);
    bool volumeChanged = (volume != lastVolume);
    bool brightnessChanged = (brightness != lastBrightness);
    bool ledChanged = (ledRunning != lastLEDRunning) || (ledEffect != lastLEDEffect);
    
    bool stateChanged = force || audioChanged || volumeChanged || brightnessChanged || ledChanged;
    
    if (!stateChanged) {
        return; // No changes, don't publish
    }
    
    Serial.print("[MQTT] Publishing status");
    if (force) Serial.print(" (forced)");
    Serial.println();
    // Build full status JSON (same format as HTTP API)
    JsonDocument doc;
    doc["status"] = "success";
    
    JsonObject audio = doc["audio"].to<JsonObject>();
    audio["playing"] = audioPlaying;
    audio["volume"] = volume;
    if (audioController) {
        audio["current_file"] = audioController->getCurrentFile();
    }
    
    JsonObject led = doc["led"].to<JsonObject>();
    led["effect_running"] = ledRunning;
    led["current_effect"] = ledEffect;
    led["brightness"] = brightness;
    
    if (bassMonoProcessor) {
        JsonObject bassMono = doc["bass_mono"].to<JsonObject>();
        bassMono["enabled"] = bassMonoProcessor->isEnabled();
        bassMono["crossover_hz"] = bassMonoProcessor->getCrossoverHz();
        bassMono["sample_rate"] = bassMonoProcessor->getSampleRate();
    }

    JsonObject wifi = doc["wifi"].to<JsonObject>();
    wifi["connected"] = WiFi.status() == WL_CONNECTED;
    wifi["ip"] = WiFi.localIP().toString();

    doc["timestamp"] = millis();
    doc["free_heap"] = ESP.getFreeHeap();
    
    String statusJson;
    serializeJson(doc, statusJson);
    
    // Publish to main status topic (retained)
    mqttClient.publish(topicStatus, config.qosStatus, true, statusJson.c_str());
    Serial.println("[MQTT] Published status");
    
    // Publish to specific status topics if changed
    if (audioChanged || volumeChanged || force) {
        JsonDocument audioDoc;
        audioDoc["playing"] = audioPlaying;
        audioDoc["volume"] = volume;
        audioDoc["timestamp"] = millis();
        String audioJson;
        serializeJson(audioDoc, audioJson);
        mqttClient.publish(topicStatusAudio, config.qosStatus, false, audioJson.c_str());
        Serial.println("[MQTT] Published status/audio");
    }
    
    if (ledChanged || brightnessChanged || force) {
        JsonDocument ledDoc;
        ledDoc["effect_running"] = ledRunning;
        ledDoc["current_effect"] = ledEffect;
        ledDoc["brightness"] = brightness;
        ledDoc["timestamp"] = millis();
        String ledJson;
        serializeJson(ledDoc, ledJson);
        mqttClient.publish(topicStatusLED, config.qosStatus, false, ledJson.c_str());
        Serial.println("[MQTT] Published status/led");
    }
    
    // Update last state (after publishing)
    lastAudioPlaying = audioPlaying;
    lastVolume = volume;
    lastBrightness = brightness;
    lastLEDRunning = ledRunning;
    lastLEDEffect = ledEffect;
}

void MQTTController::publishHealth() {
    if (!connected) {
        return;
    }
    
    bool healthy = true;
    String issues = "";
    
    if (!audioController) {
        healthy = false;
        issues += "audio_controller_missing;";
    }
    
    if (!ledStripController && !ledMatrixController) {
        healthy = false;
        issues += "led_controllers_missing;";
    }
    
    if (!settingsController) {
        healthy = false;
        issues += "settings_controller_missing;";
    }
    
    if (WiFi.status() != WL_CONNECTED) {
        healthy = false;
        issues += "wifi_disconnected;";
    }
    
    JsonDocument doc;
    doc["status"] = healthy ? "healthy" : "unhealthy";
    doc["healthy"] = healthy;
    doc["issues"] = issues;
    doc["uptime"] = millis();
    doc["free_heap"] = ESP.getFreeHeap();
    doc["timestamp"] = millis();
    
    String healthJson;
    serializeJson(doc, healthJson);
    mqttClient.publish(topicStatusHealth, config.qosStatus, false, healthJson.c_str());
    Serial.print("[MQTT] Published health status=");
    Serial.println(healthy ? "healthy" : "unhealthy");
}
