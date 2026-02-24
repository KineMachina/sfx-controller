#include "MatrixLEDController.h"
#include "MatrixMath.h"
#include "LEDMath.h"
#include <math.h>
#include "RuntimeLog.h"

static const char* TAG = "LEDMatrix";

MatrixLEDController::MatrixLEDController(int pin, int gridWidth, int gridHeight)
    : pin(pin), gridWidth(gridWidth), gridHeight(gridHeight),
      numPixels(gridWidth * gridHeight),
      currentEffect(EFFECT_NONE), effectRunning(false),
      effectStartTime(0), lastUpdate(0),
      currentBrightness(128),  // Default brightness
      ledTaskHandle(NULL),
      ledCommandQueue(NULL),
      ledStatusQueue(NULL),
      effectStartCallback(nullptr), effectStartCallbackData(nullptr) {
    // Initialize NeoMatrix for grid mode (serpentine/zigzag wiring)
    // Matrix configuration: TOP + LEFT + ROWS + ZIGZAG
    // This matches the xyToIndex implementation:
    // - Even rows go left-to-right (top to bottom)
    // - Odd rows go right-to-left (zigzag pattern)
    matrix = new Adafruit_NeoMatrix(
        gridWidth, gridHeight, pin,
        NEO_MATRIX_TOP + NEO_MATRIX_LEFT + NEO_MATRIX_ROWS + NEO_MATRIX_ZIGZAG,
        NEO_GRB + NEO_KHZ800
    );
    ESP_LOGI(TAG, "NeoMatrix initialized for grid mode");
}

MatrixLEDController::~MatrixLEDController() {
    if (matrix) {
        delete matrix;
    }
}

bool MatrixLEDController::begin() {
    ESP_LOGI(TAG, "Initializing NeoPixel LED matrix...");

    matrix->begin();
    matrix->setBrightness(currentBrightness);
    matrix->fillScreen(0);
    matrix->show();
    
    // Create queues for inter-task communication
    ledCommandQueue = xQueueCreate(10, sizeof(LEDCommand));
    ledStatusQueue = xQueueCreate(5, sizeof(LEDStatus));
    
    if (ledCommandQueue == NULL || ledStatusQueue == NULL) {
        ESP_LOGE(TAG, "Failed to create queues!");
        return false;
    }

    ESP_LOGI(TAG, "Queues created successfully");
    
    // Start the LED task on Core 1
    xTaskCreatePinnedToCore(
        ledTaskWrapper,
        "MatrixLEDTask",
        4096,
        this,
        2,  // Lower priority than audio
        &ledTaskHandle,
        1   // Core 1
    );
    
    ESP_LOGI(TAG, "NeoMatrix initialized: %dx%d grid", gridWidth, gridHeight);
    
    return true;
}

void MatrixLEDController::ledTaskWrapper(void* parameter) {
    MatrixLEDController* controller = static_cast<MatrixLEDController*>(parameter);
    controller->ledTask();
}

void MatrixLEDController::ledTask() {
    ESP_LOGI(TAG, "LED task started on Core 1");
    
    LEDCommand cmd;
    LEDStatus status;
    TickType_t lastStatusUpdate = 0;
    const TickType_t statusUpdateInterval = pdMS_TO_TICKS(500);  // Update status every 500ms
    
    while (true) {
        // Check for commands from queue (non-blocking - 0 timeout)
        if (ledCommandQueue != NULL && 
            xQueueReceive(ledCommandQueue, &cmd, 0) == pdTRUE) {
            
            switch (cmd.type) {
                case LEDCommand::CMD_START_EFFECT: {
                    // Handle start effect command
                    currentEffect = cmd.effect;
                    effectRunning = true;
                    effectStartTime = millis();
                    lastUpdate = 0;
                    
                    // Call callback if set
                    if (effectStartCallback != nullptr) {
                        effectStartCallback(cmd.effect, effectStartCallbackData);
                    }
                    
                    ESP_LOGI(TAG, "Effect started: %d, brightness: %u, grid: %dx%d", cmd.effect, currentBrightness, gridWidth, gridHeight);
                    break;
                }
                
                case LEDCommand::CMD_STOP_EFFECT: {
                    // Handle stop effect command
                    effectRunning = false;
                    currentEffect = EFFECT_NONE;
                    effectStartTime = millis();
                    
                    // Turn off all LEDs immediately
                    setAllPixels(0);
                    showPixels();
                    
                    ESP_LOGI(TAG, "Effect stopped - all LEDs off");
                    break;
                }
                
                case LEDCommand::CMD_SET_BRIGHTNESS: {
                    // Handle brightness command
                    currentBrightness = cmd.brightness;
                    setPixelBrightness(cmd.brightness);
                    ESP_LOGI(TAG, "Brightness set to: %u", cmd.brightness);
                    break;
                }
            }
        }
        
        // Update LED effect
        updateEffect();
        
        // Update status periodically and send to queue
        TickType_t currentTime = xTaskGetTickCount();
        if (currentTime - lastStatusUpdate >= statusUpdateInterval) {
            lastStatusUpdate = currentTime;
            
            // Build status
            status.currentEffect = currentEffect;
            status.effectRunning = effectRunning;
            status.brightness = currentBrightness;
            status.effectStartTime = effectStartTime;
            
            // Send status (non-blocking, drop if queue full)
            if (ledStatusQueue != NULL) {
                // Remove old status if queue is full
                if (uxQueueMessagesWaiting(ledStatusQueue) >= 4) {
                    LEDStatus oldStatus;
                    xQueueReceive(ledStatusQueue, &oldStatus, 0);
                }
                xQueueSend(ledStatusQueue, &status, 0);
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(20)); // 50 FPS update rate
    }
}

void MatrixLEDController::startEffect(EffectType effect) {
    if (ledCommandQueue == NULL) {
        ESP_LOGE(TAG, "Command queue not initialized");
        return;
    }

    // Create command
    LEDCommand cmd;
    cmd.type = LEDCommand::CMD_START_EFFECT;
    cmd.effect = effect;

    // Send command to queue (non-blocking with timeout)
    if (xQueueSend(ledCommandQueue, &cmd, pdMS_TO_TICKS(100)) != pdTRUE) {
        ESP_LOGW(TAG, "Failed to send start effect command (queue full)");
        // Fallback: set directly (for backward compatibility)
        currentEffect = effect;
        effectRunning = true;
        effectStartTime = millis();
        lastUpdate = 0;
        if (effectStartCallback != nullptr) {
            effectStartCallback(effect, effectStartCallbackData);
        }
    }
}

void MatrixLEDController::setEffectStartCallback(EffectStartCallback callback, void* userData) {
    effectStartCallback = callback;
    effectStartCallbackData = userData;
}

void MatrixLEDController::stopEffect() {
    if (ledCommandQueue == NULL) {
        ESP_LOGE(TAG, "Command queue not initialized");
        return;
    }

    // Create stop command
    LEDCommand cmd;
    cmd.type = LEDCommand::CMD_STOP_EFFECT;

    // Send command to queue (non-blocking)
    if (xQueueSend(ledCommandQueue, &cmd, pdMS_TO_TICKS(100)) != pdTRUE) {
        ESP_LOGW(TAG, "Failed to send stop effect command (queue full)");
        // Fallback: stop directly and turn off LEDs
        effectRunning = false;
        currentEffect = EFFECT_NONE;
        effectStartTime = millis();
        
        // Turn off all LEDs immediately
        setAllPixels(0);
        showPixels();
    }
}

void MatrixLEDController::updateEffect() {
    // Only render effects if effectRunning is true
    // If not running, turn off all LEDs (no effects)
    if (!effectRunning) {
        // Turn off all pixels - default state is no effects
        setAllPixels(0);
        showPixels();
        
        // Reset currentEffect if it wasn't already NONE
        if (currentEffect != EFFECT_NONE) {
            currentEffect = EFFECT_NONE;
        }
        return;
    }
    
    // effectRunning is true - render the current effect
    switch (currentEffect) {
        // Battle Arena - Fight Behaviors
        case EFFECT_BEAM_ATTACK:
            effectBeamAttack();
            break;
        case EFFECT_EXPLOSION:
            effectExplosion();
            break;
        case EFFECT_IMPACT_WAVE:
            effectImpactWave();
            break;
        case EFFECT_DAMAGE_FLASH:
            effectDamageFlash();
            break;
        case EFFECT_BLOCK_SHIELD:
            effectBlockShield();
            break;
        case EFFECT_DODGE_TRAIL:
            effectDodgeTrail();
            break;
        case EFFECT_CHARGE_UP:
            effectChargeUp();
            break;
        case EFFECT_FINISHER_BEAM:
            effectFinisherBeam();
            break;
        case EFFECT_GRAVITY_BEAM:
            effectGravityBeam();
            break;
        case EFFECT_ELECTRIC_ATTACK_MATRIX:
            effectElectricAttackMatrix();
            break;
        // Battle Arena - Dance Behaviors
        case EFFECT_VICTORY_DANCE:
            effectVictoryDance();
            break;
        case EFFECT_TAUNT_PATTERN:
            effectTauntPattern();
            break;
        case EFFECT_POSE_STRIKE:
            effectPoseStrike();
            break;
        case EFFECT_CELEBRATION_WAVE:
            effectCelebrationWave();
            break;
        case EFFECT_CONFETTI:
            effectConfetti();
            break;
        case EFFECT_HEART_EYES:
            effectHeartEyes();
            break;
        case EFFECT_POWER_UP_AURA:
            effectPowerUpAura();
            break;
        case EFFECT_TRANSITION_FADE:
            effectTransitionFade();
            break;
        // Panel Effects
        case EFFECT_GAME_OVER_CHIRON:
            effectGameOverChiron();
            break;
        case EFFECT_CYLON_EYE:
            effectCylonEye();
            break;
        case EFFECT_PERLIN_INFERNO:
            effectPerlinInferno();
            break;
        case EFFECT_EMP_LIGHTNING:
            effectEmpLightning();
            break;
        case EFFECT_GAME_OF_LIFE:
            effectGameOfLife();
            break;
        case EFFECT_PLASMA_CLOUDS:
            effectPlasmaClouds();
            break;
        case EFFECT_DIGITAL_FIREFLIES:
            effectDigitalFireflies();
            break;
        case EFFECT_MATRIX_RAIN:
            effectMatrixRain();
            break;
        case EFFECT_DANCE_FLOOR:
            effectDanceFloor();
            break;
        case EFFECT_ALIEN_CONTROL_PANEL:
            effectAlienControlPanel();
            break;
        case EFFECT_ATOMIC_BREATH_MINUS_ONE:
            effectAtomicBreathMinusOne();
            break;
        case EFFECT_ATOMIC_BREATH_MUSHROOM:
            effectAtomicBreathMushroom();
            break;
        case EFFECT_TRANSPORTER_TOS:
            effectTransporterTOS();
            break;
        default:
            setAllPixels(0);
            showPixels();
            break;
    }
}

// Battle Arena - Fight Behaviors
void MatrixLEDController::effectBeamAttack() {
    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - effectStartTime;
    
    // Effect duration: 1.5 seconds
    if (elapsed > 1500) {
        effectRunning = false;
        currentEffect = EFFECT_NONE;
        effectStartTime = millis();
        return;
    }
    
    // Clear grid
    setAllPixels(0);
    
    // Determine direction (alternate or use elapsed time)
    bool horizontal = (elapsed / 200) % 2 == 0;
    
    float intensity = 1.0f;
    
    uint32_t beamColor = matrix->Color(
        (uint8_t)(0 * intensity),
        (uint8_t)(150 * intensity),
        (uint8_t)(255 * intensity)
    ); // Blue beam
    
    if (horizontal) {
        // Horizontal beam
        int beamY = (int)((elapsed % 750) * gridHeight / 750.0f);
        for (int x = 0; x < gridWidth; x++) {
            setPixelXY(x, beamY, beamColor);
            if (beamY > 0) setPixelXY(x, beamY - 1, blendColor(beamColor, 0, 0.5f));
            if (beamY < gridHeight - 1) setPixelXY(x, beamY + 1, blendColor(beamColor, 0, 0.5f));
        }
    } else {
        // Vertical beam
        int beamX = (int)((elapsed % 750) * gridWidth / 750.0f);
        for (int y = 0; y < gridHeight; y++) {
            setPixelXY(beamX, y, beamColor);
            if (beamX > 0) setPixelXY(beamX - 1, y, blendColor(beamColor, 0, 0.5f));
            if (beamX < gridWidth - 1) setPixelXY(beamX + 1, y, blendColor(beamColor, 0, 0.5f));
        }
    }
    
    showPixels();
}

void MatrixLEDController::effectExplosion() {
    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - effectStartTime;
    
    // Effect duration: 1.5 seconds
    if (elapsed > 1500) {
        effectRunning = false;
        currentEffect = EFFECT_NONE;
        effectStartTime = millis();
        return;
    }
    
    // Clear grid
    setAllPixels(0);
    
    // Explosion center (can be parameterized later)
    int cx = gridWidth / 2;
    int cy = gridHeight / 2;
    
    float intensity = 1.0f;
    
    // Expanding radius
    float maxRadius = sqrt(gridWidth * gridWidth + gridHeight * gridHeight) / 2.0f;
    float radius = (elapsed / 1500.0f) * maxRadius;
    
    // Draw explosion rings
    for (int r = (int)radius; r >= 0 && r >= (int)radius - 3; r--) {
        float ringIntensity = 1.0f - ((radius - r) / 3.0f);
        if (ringIntensity < 0) ringIntensity = 0;
        
        uint32_t color;
        if (r == (int)radius) {
            // Outer ring: red
            color = matrix->Color(
                (uint8_t)(255 * ringIntensity * intensity),
                (uint8_t)(100 * ringIntensity * intensity),
                0
            );
        } else if (r == (int)radius - 1) {
            // Middle ring: yellow
            color = matrix->Color(
                (uint8_t)(255 * ringIntensity * intensity),
                (uint8_t)(255 * ringIntensity * intensity),
                0
            );
        } else {
            // Inner ring: white
            color = matrix->Color(
                (uint8_t)(255 * ringIntensity * intensity),
                (uint8_t)(255 * ringIntensity * intensity),
                (uint8_t)(255 * ringIntensity * intensity)
            );
        }
        
        drawCircle(cx, cy, r, color, false);
    }
    
    showPixels();
}

void MatrixLEDController::effectImpactWave() {
    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - effectStartTime;
    
    // Effect duration: 2 seconds
    if (elapsed > 2000) {
        effectRunning = false;
        currentEffect = EFFECT_NONE;
        effectStartTime = millis();
        return;
    }
    
    // Clear grid
    setAllPixels(0);
    
    // Impact point (can be parameterized later)
    int cx = gridWidth / 2;
    int cy = gridHeight / 2;
    
    float intensity = 1.0f;
    
    // Multiple expanding rings
    float maxRadius = sqrt(gridWidth * gridWidth + gridHeight * gridHeight) / 2.0f;
    float speed = 0.5f;
    
    for (int ring = 0; ring < 3; ring++) {
        float ringRadius = fmod((elapsed * speed * 50.0f / maxRadius) + (ring * maxRadius / 3.0f), maxRadius);
        float ringIntensity = 1.0f - (ringRadius / maxRadius);
        if (ringIntensity < 0) ringIntensity = 0;
        
        uint32_t color = matrix->Color(
            (uint8_t)(0 * ringIntensity * intensity),
            (uint8_t)(200 * ringIntensity * intensity),
            (uint8_t)(255 * ringIntensity * intensity)
        ); // Cyan wave
        
        drawCircle(cx, cy, (int)ringRadius, color, false);
    }
    
    showPixels();
}

void MatrixLEDController::effectDamageFlash() {
    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - effectStartTime;
    
    // Effect duration: 1 second
    if (elapsed > 1000) {
        effectRunning = false;
        currentEffect = EFFECT_NONE;
        effectStartTime = millis();
        return;
    }
    
    float intensity = 1.0f;
    
    // Rapid red/white strobing with grid pattern
    float phase = (elapsed % 100) / 100.0f; // 100ms strobe cycle
    bool flashOn = phase < 0.3f;
    
    uint32_t flashColor;
    if (flashOn) {
        // Red flash
        flashColor = matrix->Color(
            (uint8_t)(255 * intensity),
            (uint8_t)(50 * intensity),
            0
        );
    } else {
        // White flash
        flashColor = matrix->Color(
            (uint8_t)(255 * intensity * 0.3f),
            (uint8_t)(255 * intensity * 0.3f),
            (uint8_t)(255 * intensity * 0.3f)
        );
    }
    
    // Grid pattern overlay
    for (int y = 0; y < gridHeight; y++) {
        for (int x = 0; x < gridWidth; x++) {
            if ((x + y) % 2 == 0) {
                setPixelXY(x, y, flashColor);
            } else {
                setPixelXY(x, y, blendColor(flashColor, 0, 0.5f));
            }
        }
    }
    
    showPixels();
}

void MatrixLEDController::effectBlockShield() {
    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - effectStartTime;
    
    // Effect duration: 3 seconds (or until stopped)
    if (elapsed > 3000 && !effectRunning) {
        effectRunning = false;
        currentEffect = EFFECT_NONE;
        effectStartTime = millis();
        return;
    }
    
    // Clear grid
    setAllPixels(0);
    
    // Shield center
    int cx = gridWidth / 2;
    int cy = gridHeight / 2;
    
    float intensity = 1.0f;
    
    // Pulsing shield radius
    float baseRadius = min(gridWidth, gridHeight) / 3.0f;
    float pulsePhase = (elapsed % 1000) / 1000.0f;
    float radius = baseRadius + sin(pulsePhase * PI * 2) * (baseRadius * 0.2f);
    
    // Draw shield (multiple rings)
    for (int r = (int)radius; r >= 0 && r >= (int)radius - 2; r--) {
        float ringIntensity = 1.0f - ((radius - r) / 2.0f);
        if (ringIntensity < 0) ringIntensity = 0;
        
        uint32_t color = matrix->Color(
            0,
            (uint8_t)(200 * ringIntensity * intensity),
            (uint8_t)(255 * ringIntensity * intensity)
        ); // Cyan shield
        
        drawCircle(cx, cy, r, color, false);
    }
    
    // Energy particles
    for (int i = 0; i < 5; i++) {
        float angle = (elapsed * 0.001f + i * 0.4f) * PI * 2;
        int px = (int)(cx + cos(angle) * radius);
        int py = (int)(cy + sin(angle) * radius);
        if (px >= 0 && px < gridWidth && py >= 0 && py < gridHeight) {
            setPixelXY(px, py, matrix->Color(255, 255, 255));
        }
    }
    
    showPixels();
}

void MatrixLEDController::effectDodgeTrail() {
    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - effectStartTime;
    
    // Effect duration: 1 second
    if (elapsed > 1000) {
        effectRunning = false;
        currentEffect = EFFECT_NONE;
        effectStartTime = millis();
        return;
    }
    
    // Clear grid with fade
    fadeToBlack(240);
    
    float intensity = 1.0f;
    
    // Diagonal dodge path
    float progress = elapsed / 1000.0f;
    int startX = 0;
    int startY = 0;
    int endX = gridWidth - 1;
    int endY = gridHeight - 1;
    
    int currentX = (int)(startX + (endX - startX) * progress);
    int currentY = (int)(startY + (endY - startY) * progress);
    
    // Draw trail (fade out behind)
    for (int i = 0; i < 8; i++) {
        float trailProgress = progress - (i * 0.1f);
        if (trailProgress < 0) continue;
        
        int trailX = (int)(startX + (endX - startX) * trailProgress);
        int trailY = (int)(startY + (endY - startY) * trailProgress);
        
        if (trailX >= 0 && trailX < gridWidth && trailY >= 0 && trailY < gridHeight) {
            float trailIntensity = (1.0f - i * 0.15f) * intensity;
            if (trailIntensity < 0) trailIntensity = 0;
            
            uint32_t color = matrix->Color(
                (uint8_t)(255 * trailIntensity),
                (uint8_t)(255 * trailIntensity),
                (uint8_t)(255 * trailIntensity)
            ); // White trail
            
            setPixelXY(trailX, trailY, color);
        }
    }
    
    showPixels();
}

void MatrixLEDController::effectChargeUp() {
    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - effectStartTime;
    
    // Effect duration: 3 seconds
    if (elapsed > 3000) {
        effectRunning = false;
        currentEffect = EFFECT_NONE;
        effectStartTime = millis();
        return;
    }
    
    // Clear grid
    setAllPixels(0);
    
    int cx = gridWidth / 2;
    int cy = gridHeight / 2;
    
    float intensity = 1.0f;
    float speed = 1.0f;
    
    // Spiral charging effect
    float chargeProgress = elapsed / 3000.0f;
    float maxRadius = min(gridWidth, gridHeight) / 2.0f;
    
    // Draw spiral
    for (int i = 0; i < 360; i += 5) {
        float angle = (i + elapsed * speed * 50.0f) * PI / 180.0f;
        float radius = (chargeProgress * maxRadius) * (1.0f - (i / 360.0f) * 0.5f);
        
        int px = (int)(cx + cos(angle) * radius);
        int py = (int)(cy + sin(angle) * radius);
        
        if (px >= 0 && px < gridWidth && py >= 0 && py < gridHeight) {
            float pixelIntensity = (1.0f - chargeProgress * 0.3f) * intensity;
            uint32_t color = matrix->Color(
                (uint8_t)(255 * pixelIntensity),
                (uint8_t)(100 * pixelIntensity),
                0
            ); // Orange/yellow charge
            
            setPixelXY(px, py, color);
        }
    }
    
    // Pulsing center
    float pulsePhase = (elapsed % 500) / 500.0f;
    float centerIntensity = sin(pulsePhase * PI * 2) * 0.5f + 0.5f;
    uint32_t centerColor = matrix->Color(
        (uint8_t)(255 * centerIntensity * intensity),
        (uint8_t)(255 * centerIntensity * intensity),
        (uint8_t)(255 * centerIntensity * intensity)
    );
    setPixelXY(cx, cy, centerColor);
    
    showPixels();
}

void MatrixLEDController::effectFinisherBeam() {
    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - effectStartTime;
    
    // Effect duration: 2 seconds
    if (elapsed > 2000) {
        effectRunning = false;
        currentEffect = EFFECT_NONE;
        effectStartTime = millis();
        return;
    }
    
    float intensity = 1.0f;
    
    // Multiple beam layers (horizontal + vertical)
    float phase = elapsed / 2000.0f;
    
    // Horizontal beam
    for (int y = 0; y < gridHeight; y++) {
        float beamIntensity = sin((phase + y * 0.1f) * PI * 2) * 0.5f + 0.5f;
        uint32_t color = matrix->Color(
            (uint8_t)(0 * beamIntensity * intensity),
            (uint8_t)(150 * beamIntensity * intensity),
            (uint8_t)(255 * beamIntensity * intensity)
        );
        for (int x = 0; x < gridWidth; x++) {
            setPixelXY(x, y, blendColor(getPixelXY(x, y), color, 0.7f));
        }
    }
    
    // Vertical beam
    for (int x = 0; x < gridWidth; x++) {
        float beamIntensity = sin((phase + x * 0.1f) * PI * 2) * 0.5f + 0.5f;
        uint32_t color = matrix->Color(
            (uint8_t)(255 * beamIntensity * intensity),
            (uint8_t)(100 * beamIntensity * intensity),
            0
        );
        for (int y = 0; y < gridHeight; y++) {
            setPixelXY(x, y, blendColor(getPixelXY(x, y), color, 0.7f));
        }
    }
    
    showPixels();
}

void MatrixLEDController::effectGravityBeam() {
    // King Ghidorah's gravity beam attack - three gold/yellow lightning beams from three heads
    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - effectStartTime;
    
    // Effect duration: 2 seconds
    if (elapsed > 2000) {
        effectRunning = false;
        currentEffect = EFFECT_NONE;
        effectStartTime = millis();
        return;
    }
    
    // Fade previous frame for trail effect
    fadeToBlack(220);
    
    float intensity = 1.0f;
    
    // Pulsing intensity (build-up and sustain)
    float pulsePhase = (elapsed % 400) / 400.0f;
    float pulse = (elapsed < 200) ? (elapsed / 200.0f) : (0.85f + 0.15f * sin(pulsePhase * PI * 2));
    intensity *= pulse;
    
    // Gold/yellow gravity beam color (King Ghidorah's signature)
    uint32_t coreColor = matrix->Color(
        (uint8_t)(255 * intensity),
        (uint8_t)(200 * intensity),
        (uint8_t)(50 * intensity)
    );
    uint32_t glowColor = matrix->Color(
        (uint8_t)(180 * intensity),
        (uint8_t)(140 * intensity),
        (uint8_t)(30 * intensity)
    );
    
    // Three beam origins (three heads) - left, center, right at top of grid
    int beamX[] = { 2, gridWidth / 2, gridWidth - 3 };
    // Stagger the beams slightly (phase offset) so they don't all fire in sync
    float phaseOffsets[] = { 0.0f, 0.15f, 0.3f };
    
    for (int b = 0; b < 3; b++) {
        float beamPhase = fmod((elapsed * 0.008f) + phaseOffsets[b] * 10.0f, 1.0f);
        // Only draw this beam when "firing" in its phase window
        if (beamPhase < 0.85f) {
            int startX = beamX[b];
            int startY = 0;
            int endY = gridHeight - 1;
            drawLightningBolt(startX, startY, endY, coreColor, 2);
            // Draw softer glow alongside (offset) for each beam
            int glowX = constrain(startX + (b == 0 ? 1 : (b == 1 ? -1 : -1)), 0, gridWidth - 1);
            if (glowX >= 0 && glowX < gridWidth) {
                drawLightningBolt(glowX, startY, endY, glowColor, 1);
            }
        }
    }
    
    showPixels();
}

// Battle Arena - Dance Behaviors
void MatrixLEDController::effectVictoryDance() {
    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - effectStartTime;
    
    // Effect duration: 5 seconds (looping)
    if (elapsed > 5000) {
        effectStartTime = currentTime;
        elapsed = 0;
    }
    
    float speed = 1.0f;
    
    // Animated wave pattern
    for (int y = 0; y < gridHeight; y++) {
        for (int x = 0; x < gridWidth; x++) {
            float wave = sin((x * 0.5f + y * 0.5f + elapsed * speed * 0.01f) * PI * 2) * 0.5f + 0.5f;
            uint16_t hue = (uint16_t)((uint32_t)(elapsed * speed * 10 + x * 16 + y * 16) % 65536);
            uint32_t baseColor = colorWheel((byte)(hue / 256));
            
            uint8_t r = (uint8_t)((baseColor >> 16) * wave);
            uint8_t g = (uint8_t)((baseColor >> 8) * wave);
            uint8_t b = (uint8_t)(baseColor * wave);
            
            setPixelXY(x, y, matrix->Color(r, g, b));
        }
    }
    
    showPixels();
}

void MatrixLEDController::effectTauntPattern() {
    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - effectStartTime;
    
    // Effect duration: 2 seconds
    if (elapsed > 2000) {
        effectRunning = false;
        currentEffect = EFFECT_NONE;
        effectStartTime = millis();
        return;
    }
    
    float intensity = 1.0f;
    
    // Aggressive flashing pattern
    float phase = (elapsed % 200) / 200.0f;
    bool flashOn = phase < 0.5f;
    
    uint32_t color;
    if (flashOn) {
        // Red/orange flash
        color = matrix->Color(
            (uint8_t)(255 * intensity),
            (uint8_t)(100 * intensity),
            0
        );
    } else {
        color = 0;
    }
    
    // Diagonal pattern
    for (int y = 0; y < gridHeight; y++) {
        for (int x = 0; x < gridWidth; x++) {
            if ((x + y + (elapsed / 100)) % 3 == 0) {
                setPixelXY(x, y, color);
            } else {
                setPixelXY(x, y, 0);
            }
        }
    }
    
    showPixels();
}

void MatrixLEDController::effectPoseStrike() {
    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - effectStartTime;
    
    // Effect duration: 1.5 seconds
    if (elapsed > 1500) {
        effectRunning = false;
        currentEffect = EFFECT_NONE;
        effectStartTime = millis();
        return;
    }
    
    float intensity = 1.0f;
    
    if (elapsed < 200) {
        // Initial flash
        uint32_t flashColor = matrix->Color(
            (uint8_t)(255 * intensity),
            (uint8_t)(255 * intensity),
            (uint8_t)(255 * intensity)
        );
        setAllPixels(flashColor);
    } else {
        // Pose outline (stylized silhouette)
        setAllPixels(0);
        
        // Simple pose pattern (can be enhanced)
        int cx = gridWidth / 2;
        int cy = gridHeight / 2;
        
        uint32_t poseColor = matrix->Color(
            (uint8_t)(255 * intensity),
            (uint8_t)(200 * intensity),
            0
        ); // Gold outline
        
        // Draw pose outline (simplified)
        drawCircle(cx, cy, 5, poseColor, false);
        drawLine(cx, cy - 5, cx, cy + 5, poseColor); // Body
        drawLine(cx - 3, cy - 3, cx - 5, cy - 6, poseColor); // Left arm
        drawLine(cx + 3, cy - 3, cx + 5, cy - 6, poseColor); // Right arm
    }
    
    showPixels();
}

void MatrixLEDController::effectCelebrationWave() {
    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - effectStartTime;
    
    // Effect duration: 4 seconds (looping)
    if (elapsed > 4000) {
        effectStartTime = currentTime;
        elapsed = 0;
    }
    
    float speed = 1.0f;
    
    // Multiple waves
    for (int y = 0; y < gridHeight; y++) {
        for (int x = 0; x < gridWidth; x++) {
            float wave1 = sin((x * 0.3f + elapsed * speed * 0.02f) * PI * 2) * 0.5f + 0.5f;
            float wave2 = sin((y * 0.3f + elapsed * speed * 0.02f + 1.0f) * PI * 2) * 0.5f + 0.5f;
            float wave = (wave1 + wave2) / 2.0f;
            
            uint16_t hue = (uint16_t)((uint32_t)(elapsed * speed * 20 + x * 8 + y * 8) % 65536);
            uint32_t baseColor = colorWheel((byte)(hue / 256));
            
            uint8_t r = (uint8_t)((baseColor >> 16) * wave);
            uint8_t g = (uint8_t)((baseColor >> 8) * wave);
            uint8_t b = (uint8_t)(baseColor * wave);
            
            setPixelXY(x, y, matrix->Color(r, g, b));
        }
    }
    
    showPixels();
}

void MatrixLEDController::effectConfetti() {
    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - effectStartTime;
    
    // Effect duration: 3 seconds
    if (elapsed > 3000) {
        effectRunning = false;
        currentEffect = EFFECT_NONE;
        effectStartTime = millis();
        return;
    }
    
    // Fade all pixels
    fadeToBlack(240);
    
    float density = 1.0f;
    
    // Random colored pixels falling (simplified - using time-based pattern)
    int numParticles = (int)(20 * density);
    for (int i = 0; i < numParticles; i++) {
        // Use elapsed time + i to create pseudo-random positions
        int x = ((elapsed * 3 + i * 17) % (gridWidth * 100)) / 100;
        int y = ((elapsed * 5 + i * 23) % (gridHeight * 100)) / 100;
        
        if (x >= 0 && x < gridWidth && y >= 0 && y < gridHeight) {
            uint16_t hue = (uint16_t)((uint32_t)(elapsed * 50 + i * 100) % 65536);
            uint32_t color = colorWheel((byte)(hue / 256));
            setPixelXY(x, y, color);
        }
    }
    
    showPixels();
}

void MatrixLEDController::effectHeartEyes() {
    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - effectStartTime;
    
    // Effect duration: 3 seconds (looping)
    if (elapsed > 3000) {
        effectStartTime = currentTime;
        elapsed = 0;
    }
    
    setAllPixels(0);
    
    float intensity = 1.0f;
    
    // Heart pulse
    float pulsePhase = (elapsed % 1000) / 1000.0f;
    float pulse = sin(pulsePhase * PI * 2) * 0.3f + 0.7f;
    
    // Draw two hearts (eyes)
    int heartSize = 3;
    int leftHeartX = gridWidth / 2 - 4;
    int rightHeartX = gridWidth / 2 + 4;
    int heartY = gridHeight / 2;
    
    uint32_t heartColor = matrix->Color(
        (uint8_t)(255 * pulse * intensity),
        (uint8_t)(50 * pulse * intensity),
        (uint8_t)(100 * pulse * intensity)
    ); // Pink
    
    // Simple heart shape (can be enhanced)
    for (int y = -heartSize; y <= heartSize; y++) {
        for (int x = -heartSize; x <= heartSize; x++) {
            float dist = sqrt(x * x + y * y);
            if (dist <= heartSize && (y > 0 || abs(x) < heartSize * 0.7f)) {
                setPixelXY(leftHeartX + x, heartY + y, heartColor);
                setPixelXY(rightHeartX + x, heartY + y, heartColor);
            }
        }
    }
    
    showPixels();
}

void MatrixLEDController::effectPowerUpAura() {
    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - effectStartTime;
    
    // Effect duration: 3 seconds
    if (elapsed > 3000) {
        effectRunning = false;
        currentEffect = EFFECT_NONE;
        effectStartTime = millis();
        return;
    }
    
    setAllPixels(0);
    
    int cx = gridWidth / 2;
    int cy = gridHeight / 2;
    
    float intensity = 1.0f;
    
    // Expanding/contracting aura rings
    float pulsePhase = (elapsed % 1000) / 1000.0f;
    float baseRadius = min(gridWidth, gridHeight) / 3.0f;
    float radius1 = baseRadius + sin(pulsePhase * PI * 2) * (baseRadius * 0.3f);
    float radius2 = baseRadius + sin((pulsePhase + 0.5f) * PI * 2) * (baseRadius * 0.3f);
    
    // Outer ring
    uint32_t outerColor = matrix->Color(
        0,
        (uint8_t)(200 * intensity),
        (uint8_t)(255 * intensity)
    ); // Cyan
    drawCircle(cx, cy, (int)radius1, outerColor, false);
    
    // Inner ring
    uint32_t innerColor = matrix->Color(
        (uint8_t)(255 * intensity),
        (uint8_t)(255 * intensity),
        (uint8_t)(255 * intensity)
    ); // White
    drawCircle(cx, cy, (int)radius2, innerColor, false);
    
    showPixels();
}

void MatrixLEDController::effectTransitionFade() {
    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - effectStartTime;
    
    // Effect duration: 1 second
    if (elapsed > 1000) {
        effectRunning = false;
        currentEffect = EFFECT_NONE;
        effectStartTime = millis();
        return;
    }
    
    // Fade out all pixels
    float fadeProgress = elapsed / 1000.0f;
    int fadeValue = (int)(256 * (1.0f - fadeProgress));
    if (fadeValue < 0) fadeValue = 0;
    
    fadeToBlack(fadeValue);
    showPixels();
}

// Panel Effects
void MatrixLEDController::effectGameOverChiron() {
    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - effectStartTime;
    
    // Clear grid
    setAllPixels(0);
    
    // Scroll speed (doubled from 0.5 to 1.0 pixels per second)
    float speed = 1.0f; // pixels per second
    
    // Calculate scroll position
    int scrollPos = (int)((elapsed / 1000.0f) * speed) % (gridWidth + 50);
    
    // Text to display
    const char* text = "GAME OVER";
    int textLength = 9;
    int charWidth = 5;
    int charHeight = 7;
    int charSpacing = 1;
    
    // Red/orange color scheme
    uint32_t textColor = matrix->Color(255, 50, 0);
    
    // Draw each character (scroll from left to right)
    int textStartX = -charWidth + scrollPos;
    for (int i = 0; i < textLength; i++) {
        int charX = textStartX + i * (charWidth + charSpacing);
        if (charX > -charWidth && charX < gridWidth) {
            char c = text[i];
            drawChar(charX, (gridHeight - charHeight) / 2, c, textColor);
        }
    }
    
    showPixels();
}

void MatrixLEDController::effectCylonEye() {
    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - effectStartTime;
    
    // Clear all pixels
    setAllPixels(0);
    
    float speed = 0.002f; // Speed of sweep
    
    int eyeWidth = 5;
    int maxPos = gridWidth - eyeWidth;
    
    // Calculate position using sine wave for smooth back-and-forth
    float phase = fmod(elapsed * speed, 2.0f * PI);
    int eyePos = (int)((sin(phase) + 1.0f) * 0.5f * maxPos);
    
    // Draw eye with fade
    for (int i = 0; i < eyeWidth; i++) {
        float intensity = 1.0f - abs(i - eyeWidth / 2.0f) / (eyeWidth / 2.0f);
        intensity = max(0.0f, intensity);
        
        // Trailing fade
        for (int trail = 0; trail < 3; trail++) {
            int trailPos = eyePos + i - trail;
            if (trailPos >= 0 && trailPos < gridWidth) {
                float trailIntensity = intensity * (1.0f - trail * 0.3f);
                uint8_t r = (uint8_t)(255 * trailIntensity);
                for (int y = 0; y < gridHeight; y++) {
                    setPixelXY(trailPos, y, matrix->Color(r, 0, 0));
                }
            }
        }
    }
    
    showPixels();
}

void MatrixLEDController::effectPerlinInferno() {
    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - effectStartTime;
    
    float speed = 0.01f;
    
    float timeOffset = elapsed * speed;
    
    for (int y = 0; y < gridHeight; y++) {
        for (int x = 0; x < gridWidth; x++) {
            // Normalize coordinates
            float nx = (float)x / gridWidth;
            float ny = (float)y / gridHeight;
            
            // Get noise value (0-1 range)
            float noise = perlinNoise2D(nx * 3.0f, ny * 3.0f - timeOffset, timeOffset);
            
            // Map noise to fire colors (bottom = hot, top = cool)
            float heat = noise * (1.0f - ny * 0.5f); // Cooler at top
            
            uint8_t r, g, b;
            if (heat < 0.2f) {
                // Black
                r = g = b = 0;
            } else if (heat < 0.4f) {
                // Dark red
                r = (uint8_t)(80 * (heat - 0.2f) / 0.2f);
                g = b = 0;
            } else if (heat < 0.6f) {
                // Red
                r = 255;
                g = (uint8_t)(100 * (heat - 0.4f) / 0.2f);
                b = 0;
            } else if (heat < 0.8f) {
                // Orange
                r = 255;
                g = (uint8_t)(100 + 100 * (heat - 0.6f) / 0.2f);
                b = 0;
            } else {
                // Yellow to white
                float white = (heat - 0.8f) / 0.2f;
                r = 255;
                g = (uint8_t)(200 + 55 * white);
                b = (uint8_t)(200 * white);
            }
            
            setPixelXY(x, y, matrix->Color(r, g, b));
        }
    }
    
    showPixels();
}

void MatrixLEDController::effectEmpLightning() {
    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - effectStartTime;
    
    // Fade background
    fadeToBlack(245);
    
    // Lightning strike frequency
    float strikeChance = 0.02f; // 2% chance per frame
    
    // Use time-based pseudo-random for strikes
    static unsigned long lastStrikeTime = 0;
    static int lastStrikeX = 0;
    static int lastStrikeY = 0;
    
    // Check for new strike
    if ((random(0, 1000) / 1000.0f) < strikeChance || 
        (elapsed - lastStrikeTime) > 2000) {
        lastStrikeTime = elapsed;
        lastStrikeX = random(0, gridWidth);
        lastStrikeY = random(0, gridHeight / 2); // Start from top half
        
        // Draw main lightning bolt
        drawLightningBolt(lastStrikeX, lastStrikeY, gridHeight - 1, 
                         matrix->Color(255, 255, 255), 3);
    }
    
    // Draw fading branches
    if (lastStrikeTime > 0 && (elapsed - lastStrikeTime) < 500) {
        float fade = 1.0f - ((elapsed - lastStrikeTime) / 500.0f);
        uint32_t color = matrix->Color(
            (uint8_t)(255 * fade),
            (uint8_t)(255 * fade),
            (uint8_t)(255 * fade)
        );
        
        // Draw some branches
        for (int i = 0; i < 3; i++) {
            int branchX = lastStrikeX + random(-5, 6);
            int branchY = lastStrikeY + random(2, 8);
            if (branchX >= 0 && branchX < gridWidth && 
                branchY >= 0 && branchY < gridHeight) {
                setPixelXY(branchX, branchY, color);
            }
        }
    }
    
    showPixels();
}

void MatrixLEDController::effectElectricAttackMatrix() {
    // Electric attack - white/blue lightning on matrix, 5 second duration
    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - effectStartTime;

    // Effect duration: 5 seconds
    if (elapsed > 5000) {
        effectRunning = false;
        currentEffect = EFFECT_NONE;
        effectStartTime = millis();
        return;
    }

    // Fade background
    fadeToBlack(235);

    float intensity = 1.0f;

    // Lightning strike frequency (white/blue electric)
    float strikeChance = 0.025f;

    static unsigned long lastStrikeTime = 0;
    static int lastStrikeX = 0;
    static int lastStrikeY = 0;

    if ((random(0, 1000) / 1000.0f) < strikeChance ||
        (elapsed - lastStrikeTime) > 1800) {
        lastStrikeTime = elapsed;
        lastStrikeX = random(0, gridWidth);
        lastStrikeY = random(0, gridHeight / 2);
        // White/blue electric bolt
        uint32_t boltColor = matrix->Color(
            (uint8_t)(255 * intensity),
            (uint8_t)(220 * intensity),
            (uint8_t)(255 * intensity)
        );
        drawLightningBolt(lastStrikeX, lastStrikeY, gridHeight - 1, boltColor, 2);
    }

    // Fading branches (white/blue)
    if (lastStrikeTime > 0 && (elapsed - lastStrikeTime) < 400) {
        float fade = 1.0f - ((elapsed - lastStrikeTime) / 400.0f);
        uint32_t color = matrix->Color(
            (uint8_t)(200 * fade * intensity),
            (uint8_t)(180 * fade * intensity),
            (uint8_t)(255 * fade * intensity)
        );
        for (int i = 0; i < 3; i++) {
            int branchX = lastStrikeX + random(-4, 5);
            int branchY = lastStrikeY + random(2, 8);
            if (branchX >= 0 && branchX < gridWidth &&
                branchY >= 0 && branchY < gridHeight) {
                setPixelXY(branchX, branchY, color);
            }
        }
    }

    showPixels();
}

void MatrixLEDController::effectGameOfLife() {
    static uint8_t grid[16][16] = {0};
    static uint8_t nextGrid[16][16] = {0};
    static bool initialized = false;
    static unsigned long lastUpdate = 0;
    static unsigned long lastEffectStartTime = 0;
    
    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - effectStartTime;
    
    // Reset if effect was restarted
    if (effectStartTime != lastEffectStartTime) {
        initialized = false;
        lastEffectStartTime = effectStartTime;
        for (int y = 0; y < 16; y++) {
            for (int x = 0; x < 16; x++) {
                grid[y][x] = 0;
                nextGrid[y][x] = 0;
            }
        }
    }
    
    // Initialize with random pattern
    if (!initialized) {
        for (int y = 0; y < gridHeight; y++) {
            for (int x = 0; x < gridWidth; x++) {
                grid[y][x] = (random(0, 100) < 30) ? 1 : 0; // 30% alive
            }
        }
        initialized = true;
        lastUpdate = currentTime;
    }
    
    // Update every 300ms
    if (currentTime - lastUpdate > 300) {
        lastUpdate = currentTime;
        
        // Calculate next generation
        for (int y = 0; y < gridHeight; y++) {
            for (int x = 0; x < gridWidth; x++) {
                // Count neighbors (8-directional)
                int neighbors = 0;
                for (int dy = -1; dy <= 1; dy++) {
                    for (int dx = -1; dx <= 1; dx++) {
                        if (dx == 0 && dy == 0) continue;
                        int nx = x + dx;
                        int ny = y + dy;
                        if (nx >= 0 && nx < gridWidth && ny >= 0 && ny < gridHeight) {
                            neighbors += grid[ny][nx];
                        }
                    }
                }
                
                // Apply rules
                if (grid[y][x] == 1) {
                    // Live cell
                    nextGrid[y][x] = (neighbors == 2 || neighbors == 3) ? 1 : 0;
                } else {
                    // Dead cell
                    nextGrid[y][x] = (neighbors == 3) ? 1 : 0;
                }
            }
        }
        
        // Copy next to current
        for (int y = 0; y < gridHeight; y++) {
            for (int x = 0; x < gridWidth; x++) {
                grid[y][x] = nextGrid[y][x];
            }
        }
    }
    
    // Render
    for (int y = 0; y < gridHeight; y++) {
        for (int x = 0; x < gridWidth; x++) {
            if (grid[y][x] == 1) {
                setPixelXY(x, y, matrix->Color(0, 255, 0)); // Green
            } else {
                setPixelXY(x, y, 0); // Black
            }
        }
    }
    
    showPixels();
}

void MatrixLEDController::effectPlasmaClouds() {
    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - effectStartTime;
    
    float speed = 0.001f;
    
    float time = elapsed * speed;
    
    for (int y = 0; y < gridHeight; y++) {
        for (int x = 0; x < gridWidth; x++) {
            // Normalize coordinates
            float nx = (float)x / gridWidth;
            float ny = (float)y / gridHeight;
            
            // Combine multiple sine waves
            float value = sin(nx * 3.14159f * 2 + time);
            value += sin(ny * 3.14159f * 2 + time * 1.3f);
            value += sin((nx + ny) * 3.14159f * 2 + time * 0.7f);
            value += sin(sqrt(nx*nx + ny*ny) * 3.14159f * 4 + time);
            
            // Normalize to 0-1
            value = (value + 4.0f) / 8.0f;
            value = constrain(value, 0.0f, 1.0f);
            
            // Map to color wheel
            uint16_t hue = (uint16_t)((uint32_t)(value * 65536 + elapsed * 10) % 65536);
            uint32_t color = colorWheel((byte)(hue >> 8));
            
            setPixelXY(x, y, color);
        }
    }
    
    showPixels();
}

void MatrixLEDController::effectDigitalFireflies() {
    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - effectStartTime;
    
    // Fade background
    fadeToBlack(250);
    
    // Number of fireflies
    int numFireflies = 15;
    
    // Create fireflies using time-based pseudo-random
    for (int i = 0; i < numFireflies; i++) {
        // Use i and elapsed to create pseudo-random but consistent positions
        int seed = (elapsed * 7 + i * 7919) % 10000;
        int x = (seed * 13) % gridWidth;
        int y = (seed * 17) % gridHeight;
        
        // Calculate lifecycle phase
        float phase = fmod((elapsed * 0.001f + i * 0.1f), 4.0f);
        float intensity = 0.0f;
        
        if (phase < 1.0f) {
            // Fade in
            intensity = phase;
        } else if (phase < 2.0f) {
            // Full brightness
            intensity = 1.0f;
        } else if (phase < 3.0f) {
            // Hold
            intensity = 1.0f;
        } else {
            // Fade out
            intensity = 1.0f - (phase - 3.0f);
        }
        
        // Warm colors (yellow, orange, white)
        uint8_t r = (uint8_t)(255 * intensity);
        uint8_t g = (uint8_t)(200 * intensity);
        uint8_t b = (uint8_t)(100 * intensity);
        
        setPixelXY(x, y, matrix->Color(r, g, b));
    }
    
    showPixels();
}

void MatrixLEDController::effectMatrixRain() {
    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - effectStartTime;
    
    // Fade background
    fadeToBlack(240);
    
    float baseSpeed = 0.05f;
    
    // Each column has independent properties
    for (int x = 0; x < gridWidth; x++) {
        // Use x as seed for consistent column behavior
        int seed = x * 7919;
        float speed = baseSpeed * (0.5f + (seed % 100) / 100.0f);
        int length = 3 + (seed % 8);
        
        // Calculate head position
        float headPos = fmod((elapsed * speed * 50.0f) + (seed % gridHeight), gridHeight + length);
        int headY = (int)headPos;
        
        // Draw column
        for (int i = 0; i < length; i++) {
            int y = headY - i;
            if (y >= 0 && y < gridHeight) {
                // Brightness fades from head to tail
                float brightness = 1.0f - ((float)i / length);
                brightness = max(0.0f, brightness);
                
                // Head is bright green, tail fades to dark green
                uint8_t g = (uint8_t)(255 * brightness);
                uint8_t r = (uint8_t)(g * 0.2f); // Slight red tint for head
                
                setPixelXY(x, y, matrix->Color(r, g, 0));
            }
        }
    }
    
    showPixels();
}

void MatrixLEDController::effectDanceFloor() {
    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - effectStartTime;
    
    float speed = 0.001f;
    
    // Divide into 4x4 squares
    int squareSize = 4;
    
    for (int y = 0; y < gridHeight; y++) {
        for (int x = 0; x < gridWidth; x++) {
            int squareX = x / squareSize;
            int squareY = y / squareSize;
            
            // Calculate color based on square position and time
            float hue = fmod((squareX + squareY) * 30.0f + elapsed * speed * 1000.0f, 256.0f);
            uint32_t baseColor = colorWheel((byte)hue);
            
            // Pulse effect
            float pulsePhase = (elapsed * 0.002f + (squareX + squareY) * 0.1f);
            float pulse = sin(pulsePhase * PI * 2) * 0.3f + 0.7f;
            
            // Extract RGB and apply pulse
            uint8_t r = (uint8_t)(((baseColor >> 16) & 0xFF) * pulse);
            uint8_t g = (uint8_t)(((baseColor >> 8) & 0xFF) * pulse);
            uint8_t b = (uint8_t)(baseColor & 0xFF) * pulse;
            
            setPixelXY(x, y, matrix->Color(r, g, b));
        }
    }
    
    showPixels();
}

void MatrixLEDController::effectAlienControlPanel() {
    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - effectStartTime;
    
    // Alien control panel: grid of pulsing indicators (green/cyan/blue sci-fi palette)
    // Each "panel" is 2x2 pixels; panels pulse at different phases for a living dashboard look
    const int panelW = 2;
    const int panelH = 2;
    float timeBase = elapsed * 0.002f;
    
    for (int y = 0; y < gridHeight; y++) {
        for (int x = 0; x < gridWidth; x++) {
            int panelX = x / panelW;
            int panelY = y / panelH;
            // Phase offset per panel so they pulse out of sync
            float phase = timeBase + (panelX * 0.7f + panelY * 0.5f);
            float pulse = sin(phase * PI * 2) * 0.45f + 0.55f;  // 0.1 to 1.0
            
            // Alternate alien colors: green, cyan, teal by panel position
            int panelIdx = (panelX + panelY) % 3;
            uint8_t r, g, b;
            if (panelIdx == 0) {
                r = 0;   g = (uint8_t)(200 * pulse); b = (uint8_t)(80 * pulse);   // green
            } else if (panelIdx == 1) {
                r = 0;   g = (uint8_t)(255 * pulse); b = (uint8_t)(200 * pulse);  // cyan
            } else {
                r = 0;   g = (uint8_t)(180 * pulse); b = (uint8_t)(220 * pulse);  // teal
            }
            setPixelXY(x, y, matrix->Color(r, g, b));
        }
    }
    
    showPixels();
}

void MatrixLEDController::effectAtomicBreathMinusOne() {
    // Godzilla Minus One atomic breath - purple/magenta beam with bright core
    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - effectStartTime;
    
    // Effect duration: 2 seconds
    if (elapsed > 2000) {
        effectRunning = false;
        currentEffect = EFFECT_NONE;
        effectStartTime = millis();
        return;
    }
    
    setAllPixels(0);
    float intensity = 1.0f;
    float phase = elapsed / 2000.0f;
    
    // Purple/magenta outer beam (Minus One style)
    for (int y = 0; y < gridHeight; y++) {
        float beamIntensity = sin((phase + y * 0.08f) * PI * 2) * 0.5f + 0.5f;
        beamIntensity = constrain(beamIntensity, 0.2f, 1.0f);
        uint32_t color = matrix->Color(
            (uint8_t)(220 * beamIntensity * intensity),
            (uint8_t)(40 * beamIntensity * intensity),
            (uint8_t)(255 * beamIntensity * intensity)
        );
        for (int x = 0; x < gridWidth; x++) {
            setPixelXY(x, y, color);
        }
    }
    
    // Bright white core down center (breath spine)
    int cy = gridHeight / 2;
    float corePulse = sin((elapsed % 400) / 400.0f * PI * 2) * 0.3f + 0.7f;
    uint32_t coreColor = matrix->Color(
        (uint8_t)(255 * corePulse * intensity),
        (uint8_t)(240 * corePulse * intensity),
        (uint8_t)(255 * corePulse * intensity)
    );
    uint32_t coreFringe = matrix->Color(200, 180, 255);
    for (int x = 0; x < gridWidth; x++) {
        setPixelXY(x, cy, coreColor);
        if (cy > 0) setPixelXY(x, cy - 1, coreFringe);
        if (cy < gridHeight - 1) setPixelXY(x, cy + 1, coreFringe);
    }
    
    showPixels();
}

void MatrixLEDController::effectAtomicBreathMushroom() {
    // Atomic breath mushroom cloud finishing move - stem rises, cap expands
    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - effectStartTime;
    
    const unsigned long durationMs = 2800;
    if (elapsed > durationMs) {
        effectRunning = false;
        currentEffect = EFFECT_NONE;
        effectStartTime = millis();
        return;
    }
    
    setAllPixels(0);
    int cx = gridWidth / 2;
    int cy = gridHeight / 2;
    float t = (float)elapsed / (float)durationMs;
    
    // Phase 1 (0-0.25): Initial flash / impact
    // Phase 2 (0.25-0.5): Stem rises from bottom center
    // Phase 3 (0.5-1.0): Mushroom cap expands at top
    
    if (t < 0.12f) {
        // Quick white flash
        float flash = 1.0f - (t / 0.12f);
        uint32_t c = matrix->Color(
            (uint8_t)(255 * flash),
            (uint8_t)(255 * flash),
            (uint8_t)(255 * flash)
        );
        setAllPixels(c);
        showPixels();
        return;
    }
    
    float stemPhase = (t - 0.12f) / 0.38f;  // 0..1 over next ~38% of time
    if (stemPhase > 1.0f) stemPhase = 1.0f;
    
    // Stem: rises from bottom (y = gridHeight-1) up to middle-top
    int stemTop = gridHeight - 1 - (int)(stemPhase * (gridHeight * 3 / 4));
    if (stemTop < 0) stemTop = 0;
    
    for (int y = gridHeight - 1; y >= stemTop; y--) {
        float dist = (float)(gridHeight - 1 - y) / (float)(gridHeight - 1 - stemTop + 1);
        uint8_t r = (uint8_t)(255);
        uint8_t g = (uint8_t)(80 + (1.0f - dist) * 120);
        uint8_t b = (uint8_t)(20);
        uint32_t color = matrix->Color(r, g, b);
        setPixelXY(cx, y, color);
        if (cx > 0) setPixelXY(cx - 1, y, matrix->Color(r / 2, g / 2, b / 2));
        if (cx < gridWidth - 1) setPixelXY(cx + 1, y, matrix->Color(r / 2, g / 2, b / 2));
    }
    
    // Cap: expands from top of stem (phase 0.5 onward)
    float capPhase = (t - 0.5f) / 0.5f;
    if (capPhase < 0.0f) capPhase = 0.0f;
    if (capPhase > 1.0f) capPhase = 1.0f;
    
    float capRadius = capPhase * (sqrt(gridWidth * gridWidth + gridHeight * gridHeight) / 2.0f);
    int capCenterY = stemTop;
    if (capCenterY < 2) capCenterY = 2;
    
    for (int py = 0; py < gridHeight; py++) {
        for (int px = 0; px < gridWidth; px++) {
            float d = distance2D(px, py, cx, capCenterY);
            if (d <= capRadius && py <= capCenterY + 1) {
                float falloff = 1.0f - (d / (capRadius + 0.01f)) * 0.6f;
                if (falloff < 0) falloff = 0;
                // Mushroom cap: orange/red center, gray-ish edge
                uint8_t r = (uint8_t)(200 * falloff + 80 * (1.0f - falloff));
                uint8_t g = (uint8_t)(80 * falloff + 70 * (1.0f - falloff));
                uint8_t b = (uint8_t)(30 * falloff + 60 * (1.0f - falloff));
                setPixelXY(px, py, matrix->Color(r, g, b));
            }
        }
    }
    
    showPixels();
}

void MatrixLEDController::effectTransporterTOS() {
    // Star Trek TOS transporter - 6 second shimmer: sweeping bands + sparkles (white/cyan)
    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - effectStartTime;
    const unsigned long durationMs = 6000;

    if (elapsed > durationMs) {
        effectRunning = false;
        currentEffect = EFFECT_NONE;
        effectStartTime = millis();
        return;
    }

    float t = (float)elapsed / (float)durationMs;  // 0..1 over 6 seconds
    // Intensity envelope: ramp up 0–1s, sustain 1–5s, ramp down 5–6s
    float envelope;
    if (t < (1.0f / 6.0f)) {
        envelope = t * 6.0f;  // 0..1 in first second
    } else if (t < (5.0f / 6.0f)) {
        envelope = 1.0f;
    } else {
        envelope = (1.0f - t) * 6.0f;  // 1..0 in last second
    }

    // Fade previous frame slightly for trail
    fadeToBlack(248);

    // Sweeping "transporter" bands moving upward (multiple bands, ~3 rows each)
    float sweepPhase = (elapsed % 1200) / 1200.0f;  // 1.2s per full sweep cycle
    int bandCenter = (int)(sweepPhase * (gridHeight + 4)) - 2;
    for (int band = -1; band <= 1; band++) {
        int center = bandCenter + band * (gridHeight / 2 + 2);
        for (int y = 0; y < gridHeight; y++) {
            int dist = abs(y - center);
            if (dist <= 2) {
                float bandBright = 1.0f - (dist / 3.0f);
                bandBright *= envelope;
                uint8_t r = (uint8_t)(180 * bandBright);
                uint8_t g = (uint8_t)(220 * bandBright);
                uint8_t blue = (uint8_t)(255 * bandBright);
                for (int x = 0; x < gridWidth; x++) {
                    setPixelXY(x, y, matrix->Color(r, g, blue));
                }
            }
        }
    }

    // Random sparkles (TOS glitter)
    for (int i = 0; i < 25; i++) {
        int sx = (elapsed * 7 + i * 31) % (gridWidth * 17);
        int sy = (elapsed * 11 + i * 47) % (gridHeight * 17);
        sx = sx % gridWidth;
        sy = sy % gridHeight;
        float twinkle = 0.5f + 0.5f * sinf((elapsed + i * 100) * 0.012f);
        twinkle *= envelope;
        uint8_t r = (uint8_t)(200 * twinkle);
        uint8_t g = (uint8_t)(230 * twinkle);
        uint8_t b = (uint8_t)(255 * twinkle);
        setPixelXY(sx, sy, matrix->Color(r, g, b));
    }

    showPixels();
}

// Helper functions for matrix effects
float MatrixLEDController::perlinNoise2D(float x, float y, float time) {
    return MatrixMath::perlinNoise2D(x, y, time);
}

void MatrixLEDController::drawChar(int x, int y, char c, uint32_t color) {
    // Simple 5x7 font - only implement basic characters
    for (int row = 0; row < 7; row++) {
        uint8_t bitmap = getCharBitmap(c, row);
        for (int col = 0; col < 5; col++) {
            if (bitmap & (1 << (4 - col))) {
                int px = x + col;
                int py = y + row;
                if (px >= 0 && px < gridWidth && py >= 0 && py < gridHeight) {
                    setPixelXY(px, py, color);
                }
            }
        }
    }
}

uint8_t MatrixLEDController::getCharBitmap(char c, int row) {
    return MatrixMath::getCharBitmap(c, row);
}

void MatrixLEDController::drawLightningBolt(int x1, int y1, int y2, uint32_t color, int thickness) {
    // Simple lightning bolt - jagged line from top to bottom
    int currentY = y1;
    int currentX = x1;
    
    while (currentY < y2) {
        // Draw current segment
        for (int t = 0; t < thickness; t++) {
            int px = currentX + t - thickness/2;
            if (px >= 0 && px < gridWidth && currentY >= 0 && currentY < gridHeight) {
                setPixelXY(px, currentY, color);
            }
        }
        
        // Move down and add random horizontal jitter
        currentY++;
        if (currentY % 2 == 0) {
            currentX += random(-1, 2);
            currentX = constrain(currentX, 0, gridWidth - 1);
        }
    }
}

// Helper functions
uint32_t MatrixLEDController::colorWheel(byte wheelPos) {
    return LEDMath::colorWheel(wheelPos);
}

uint32_t MatrixLEDController::blendColor(uint32_t color1, uint32_t color2, float ratio) {
    return LEDMath::blendColor(color1, color2, ratio);
}

void MatrixLEDController::setAllPixels(uint32_t color) {
    matrix->fillScreen(color);
}

void MatrixLEDController::fadeToBlack(int fadeValue) {
    // For matrix, fade each pixel individually
    for (int y = 0; y < gridHeight; y++) {
        for (int x = 0; x < gridWidth; x++) {
            int index = xyToIndex(x, y);
            if (index >= 0 && index < numPixels) {
                uint32_t c = matrix->getPixelColor(index);
                uint8_t r = LEDMath::fadeChannel(LEDMath::unpackR(c), fadeValue);
                uint8_t g = LEDMath::fadeChannel(LEDMath::unpackG(c), fadeValue);
                uint8_t b = LEDMath::fadeChannel(LEDMath::unpackB(c), fadeValue);
                matrix->drawPixel(x, y, matrix->Color(r, g, b));
            }
        }
    }
}

void MatrixLEDController::showPixels() {
    matrix->show();
}

void MatrixLEDController::setPixelBrightness(uint8_t brightness) {
    currentBrightness = brightness;
    matrix->setBrightness(brightness);
}

// Grid helper functions
int MatrixLEDController::xyToIndex(int x, int y) const {
    return MatrixMath::xyToIndex(x, y, gridWidth, gridHeight);
}

void MatrixLEDController::indexToXY(int index, int& x, int& y) const {
    MatrixMath::indexToXY(index, x, y, gridWidth, numPixels);
}

void MatrixLEDController::setPixelXY(int x, int y, uint32_t color) {
    // Use NeoMatrix's built-in coordinate system
    matrix->drawPixel(x, y, color);
}

uint32_t MatrixLEDController::getPixelXY(int x, int y) const {
    // NeoMatrix inherits from NeoPixel, so convert x,y to index
    // Since matrix is configured with the same zigzag pattern as xyToIndex(),
    // we can use xyToIndex() to get the correct pixel index
    int index = xyToIndex(x, y);
    if (index >= 0 && index < numPixels) {
        return matrix->getPixelColor(index);
    }
    return 0;
}

float MatrixLEDController::distance2D(int x1, int y1, int x2, int y2) const {
    return MatrixMath::distance2D(x1, y1, x2, y2);
}

void MatrixLEDController::drawLine(int x1, int y1, int x2, int y2, uint32_t color) {
    // Bresenham's line algorithm
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;
    
    int x = x1;
    int y = y1;
    
    while (true) {
        setPixelXY(x, y, color);
        
        if (x == x2 && y == y2) break;
        
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x += sx;
        }
        if (e2 < dx) {
            err += dx;
            y += sy;
        }
    }
}

void MatrixLEDController::drawCircle(int cx, int cy, int radius, uint32_t color, bool filled) {
    if (radius <= 0) {
        setPixelXY(cx, cy, color);
        return;
    }
    
    int x = 0;
    int y = radius;
    int d = 1 - radius;
    
    // Midpoint circle algorithm
    while (x <= y) {
        if (filled) {
            // Draw filled circle
            for (int i = -x; i <= x; i++) {
                setPixelXY(cx + i, cy + y, color);
                setPixelXY(cx + i, cy - y, color);
            }
            for (int i = -y; i <= y; i++) {
                setPixelXY(cx + i, cy + x, color);
                setPixelXY(cx + i, cy - x, color);
            }
        } else {
            // Draw circle outline
            setPixelXY(cx + x, cy + y, color);
            setPixelXY(cx - x, cy + y, color);
            setPixelXY(cx + x, cy - y, color);
            setPixelXY(cx - x, cy - y, color);
            setPixelXY(cx + y, cy + x, color);
            setPixelXY(cx - y, cy + x, color);
            setPixelXY(cx + y, cy - x, color);
            setPixelXY(cx - y, cy - x, color);
        }
        
        if (d < 0) {
            d += 2 * x + 3;
        } else {
            d += 2 * (x - y) + 5;
            y--;
        }
        x++;
    }
}

void MatrixLEDController::drawRectangle(int x1, int y1, int x2, int y2, uint32_t color, bool filled) {
    int minX = min(x1, x2);
    int maxX = max(x1, x2);
    int minY = min(y1, y2);
    int maxY = max(y1, y2);
    
    if (filled) {
        for (int y = minY; y <= maxY; y++) {
            for (int x = minX; x <= maxX; x++) {
                setPixelXY(x, y, color);
            }
        }
    } else {
        // Draw outline
        for (int x = minX; x <= maxX; x++) {
            setPixelXY(x, minY, color);
            setPixelXY(x, maxY, color);
        }
        for (int y = minY; y <= maxY; y++) {
            setPixelXY(minX, y, color);
            setPixelXY(maxX, y, color);
        }
    }
}

// Public methods
void MatrixLEDController::setBrightness(uint8_t brightness) {
    if (ledCommandQueue == NULL) {
        currentBrightness = brightness;
        setPixelBrightness(brightness);
        return;
    }
    
    LEDCommand cmd;
    cmd.type = LEDCommand::CMD_SET_BRIGHTNESS;
    cmd.brightness = brightness;
    
    if (xQueueSend(ledCommandQueue, &cmd, pdMS_TO_TICKS(100)) != pdTRUE) {
        // Fallback: set directly
        currentBrightness = brightness;
        setPixelBrightness(brightness);
    }
}

uint8_t MatrixLEDController::getBrightness() const {
    if (ledStatusQueue == NULL) {
        return currentBrightness;
    }
    
    // Get latest status from queue
    LEDStatus status;
    while (xQueueReceive(ledStatusQueue, &status, 0) == pdTRUE) {
        // Keep reading until we get the latest
    }
    
    // Put it back at the front
    if (xQueueSendToFront(ledStatusQueue, &status, 0) == pdTRUE) {
        return status.brightness;
    }
    
    return currentBrightness;
}

MatrixLEDController::EffectType MatrixLEDController::getCurrentEffect() const {
    if (ledStatusQueue == NULL) {
        return currentEffect;
    }
    
    // Get latest status from queue
    LEDStatus status;
    while (xQueueReceive(ledStatusQueue, &status, 0) == pdTRUE) {
        // Keep reading until we get the latest
    }
    
    // Put it back at the front
    if (xQueueSendToFront(ledStatusQueue, &status, 0) == pdTRUE) {
        return status.currentEffect;
    }
    
    return currentEffect;
}

bool MatrixLEDController::isEffectRunning() const {
    if (ledStatusQueue == NULL) {
        return effectRunning;
    }
    
    // Get latest status from queue
    LEDStatus status;
    while (xQueueReceive(ledStatusQueue, &status, 0) == pdTRUE) {
        // Keep reading until we get the latest
    }
    
    // Put it back at the front
    if (xQueueSendToFront(ledStatusQueue, &status, 0) == pdTRUE) {
        return status.effectRunning;
    }
    
    return effectRunning;
}

void MatrixLEDController::update() {
    // Effects run in a separate task, so this is mostly a placeholder
    // Can be used for synchronization if needed
}
