#include "StripLEDController.h"
#include <math.h>

StripLEDController::StripLEDController(int pin, int numPixels)
    : pin(pin), numPixels(numPixels),
      currentEffect(EFFECT_IDLE), effectRunning(false),
      effectStartTime(0), lastUpdate(0),
      currentBrightness(128),  // Default brightness
      ledTaskHandle(NULL),
      ledCommandQueue(NULL),
      ledStatusQueue(NULL),
      effectStartCallback(nullptr), effectStartCallbackData(nullptr) {
    pixels = new Adafruit_NeoPixel(numPixels, pin, NEO_GRB + NEO_KHZ800);
}

StripLEDController::~StripLEDController() {
    if (pixels) {
        delete pixels;
    }
}

bool StripLEDController::begin() {
    Serial.println("Initializing NeoPixel LED strip...");
    
    pixels->begin();
    setPixelBrightness(currentBrightness);
    showPixels(); // Initialize all pixels to 'off'
    
    // Create queues for inter-task communication
    ledCommandQueue = xQueueCreate(10, sizeof(LEDCommand));
    ledStatusQueue = xQueueCreate(5, sizeof(LEDStatus));
    
    if (ledCommandQueue == NULL || ledStatusQueue == NULL) {
        Serial.println("[StripLED] ERROR: Failed to create queues!");
        return false;
    }
    
    Serial.println("[StripLED] Queues created successfully");
    
    // Start the LED task on Core 1
    xTaskCreatePinnedToCore(
        ledTaskWrapper,
        "StripLEDTask",
        4096,
        this,
        2,  // Lower priority than audio
        &ledTaskHandle,
        1   // Core 1
    );
    
    Serial.print("NeoPixel LED strip initialized: ");
    Serial.print(numPixels);
    Serial.println(" pixels");
    
    return true;
}

void StripLEDController::ledTaskWrapper(void* parameter) {
    StripLEDController* controller = static_cast<StripLEDController*>(parameter);
    controller->ledTask();
}

void StripLEDController::ledTask() {
    Serial.println("[StripLED] LED task started on Core 1");
    
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
                    
                    Serial.print("[StripLED] Effect started: ");
                    Serial.print(cmd.effect);
                    Serial.print(", brightness: ");
                    Serial.print(currentBrightness);
                    Serial.print(", numPixels: ");
                    Serial.println(numPixels);
                    break;
                }
                
                case LEDCommand::CMD_STOP_EFFECT: {
                    // Handle stop effect command
                    effectRunning = false;
                    currentEffect = EFFECT_IDLE;
                    effectStartTime = millis();
                    
                    // Turn off all LEDs immediately
                    setAllPixels(0);
                    showPixels();
                    
                    Serial.println("[StripLED] Effect stopped - all LEDs off");
                    break;
                }
                
                case LEDCommand::CMD_SET_BRIGHTNESS: {
                    // Handle brightness command
                    currentBrightness = cmd.brightness;
                    setPixelBrightness(cmd.brightness);
                    Serial.print("[StripLED] Brightness set to: ");
                    Serial.println(cmd.brightness);
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

void StripLEDController::startEffect(EffectType effect) {
    if (ledCommandQueue == NULL) {
        Serial.println("[StripLED] ERROR: Command queue not initialized");
        return;
    }
    
    // Create command
    LEDCommand cmd;
    cmd.type = LEDCommand::CMD_START_EFFECT;
    cmd.effect = effect;
    
    // Send command to queue (non-blocking with timeout)
    if (xQueueSend(ledCommandQueue, &cmd, pdMS_TO_TICKS(100)) != pdTRUE) {
        Serial.println("[StripLED] WARNING: Failed to send start effect command (queue full)");
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

void StripLEDController::setEffectStartCallback(EffectStartCallback callback, void* userData) {
    effectStartCallback = callback;
    effectStartCallbackData = userData;
}

void StripLEDController::stopEffect() {
    if (ledCommandQueue == NULL) {
        Serial.println("[StripLED] ERROR: Command queue not initialized");
        return;
    }
    
    // Create stop command
    LEDCommand cmd;
    cmd.type = LEDCommand::CMD_STOP_EFFECT;
    
    // Send command to queue (non-blocking)
    if (xQueueSend(ledCommandQueue, &cmd, pdMS_TO_TICKS(100)) != pdTRUE) {
        Serial.println("[StripLED] WARNING: Failed to send stop effect command (queue full)");
        // Fallback: stop directly and turn off LEDs
        effectRunning = false;
        currentEffect = EFFECT_IDLE;
        effectStartTime = millis();
        
        // Turn off all LEDs immediately
        setAllPixels(0);
        showPixels();
    }
}

void StripLEDController::updateEffect() {
    // Only render effects if effectRunning is true
    // If not running, turn off all LEDs (no effects)
    if (!effectRunning) {
        // Turn off all pixels - default state is no effects
        setAllPixels(0);
        showPixels();
        
        // Reset currentEffect if it wasn't already IDLE
        if (currentEffect != EFFECT_IDLE) {
            currentEffect = EFFECT_IDLE;
        }
        return;
    }
    
    // effectRunning is true - render the current effect
    switch (currentEffect) {
        case EFFECT_ATOMIC_BREATH:
            effectAtomicBreath();
            break;
        case EFFECT_GRAVITY_BEAM:
            effectGravityBeam();
            break;
        case EFFECT_FIRE_BREATH:
            effectFireBreath();
            break;
        case EFFECT_ELECTRIC_ATTACK:
            effectElectricAttack();
            break;
        case EFFECT_BATTLE_DAMAGE:
            effectBattleDamage();
            break;
        case EFFECT_VICTORY:
            effectVictory();
            break;
        case EFFECT_IDLE:
            // IDLE should not be rendered when effectRunning is true
            // If we reach here, turn off LEDs (shouldn't happen in normal flow)
            setAllPixels(0);
            showPixels();
            break;
        // Popular general effects
        case EFFECT_RAINBOW:
            effectRainbow();
            break;
        case EFFECT_RAINBOW_CHASE:
            effectRainbowChase();
            break;
        case EFFECT_COLOR_WIPE:
            effectColorWipe();
            break;
        case EFFECT_THEATER_CHASE:
            effectTheaterChase();
            break;
        case EFFECT_PULSE:
            effectPulse();
            break;
        case EFFECT_BREATHING:
            effectBreathing();
            break;
        case EFFECT_METEOR:
            effectMeteor();
            break;
        case EFFECT_TWINKLE:
            effectTwinkle();
            break;
        case EFFECT_WATER:
            effectWater();
            break;
        case EFFECT_STROBE:
            effectStrobe();
            break;
        // Edge-lit acrylic disc effects
        case EFFECT_RADIAL_OUT:
            effectRadialOut();
            break;
        case EFFECT_RADIAL_IN:
            effectRadialIn();
            break;
        case EFFECT_SPIRAL:
            effectSpiral();
            break;
        case EFFECT_ROTATING_RAINBOW:
            effectRotatingRainbow();
            break;
        case EFFECT_CIRCULAR_CHASE:
            effectCircularChase();
            break;
        case EFFECT_RADIAL_GRADIENT:
            effectRadialGradient();
            break;
        default:
            effectIdle();
            break;
    }
}

// Godzilla Atomic Breath - Blue/cyan pulsing stream effect
void StripLEDController::effectAtomicBreath() {
    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - effectStartTime;
    
    // Effect duration: 3 seconds
    if (elapsed > 3000) {
        // Stop effect directly (we're in task context)
        effectRunning = false;
        currentEffect = EFFECT_IDLE;
        effectStartTime = millis();
        return;
    }
    
    // Create a pulsing blue/cyan stream that moves forward
    float phase = (elapsed % 500) / 500.0f; // 500ms cycle
    float intensity = sin(phase * PI * 2) * 0.5f + 0.5f; // 0-1 range
    
    for (int i = 0; i < numPixels; i++) {
        // Create wave effect moving from start to end
        float position = (float)i / numPixels;
        float wave = sin((position * 4.0f + phase * 2.0f) * PI) * 0.5f + 0.5f;
        
        // Blue/cyan color (more cyan in center, blue on edges)
        uint8_t r = 0;
        uint8_t g = (uint8_t)(wave * intensity * 255);
        uint8_t b = (uint8_t)(intensity * 255);
        
        // Add white core for brightness
        if (wave > 0.7f) {
            uint8_t white = (uint8_t)((wave - 0.7f) * 3.33f * 255 * intensity);
            r = white;
            g = min(255, g + white / 2);
            b = min(255, b + white);
        }
        
        pixels->setPixelColor(i, pixels->Color(r, g, b));
    }
    
    showPixels();
}

// Ghidorah Gravity Beam - Gold/yellow lightning effect
void StripLEDController::effectGravityBeam() {
    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - effectStartTime;
    
    // Effect duration: 2.5 seconds
    if (elapsed > 2500) {
        effectRunning = false;
        currentEffect = EFFECT_IDLE;
        effectStartTime = millis();
        return;
    }
    
    // Create lightning-like flashes with gold/yellow
    float phase = (elapsed % 200) / 200.0f; // 200ms flash cycle
    
    for (int i = 0; i < numPixels; i++) {
        // Random lightning branches
        float randomPhase = (sin(i * 7.3f) + 1.0f) / 2.0f;
        float flash = sin((phase + randomPhase) * PI * 2);
        flash = max(0.0f, flash); // Only positive values
        
        // Gold/yellow color with white highlights
        uint8_t r = (uint8_t)(flash * 255);
        uint8_t g = (uint8_t)(flash * 200);
        uint8_t b = (uint8_t)(flash * 50);
        
        // Add bright white core
        if (flash > 0.8f) {
            uint8_t white = (uint8_t)((flash - 0.8f) * 5.0f * 255);
            r = min(255, r + white);
            g = min(255, g + white);
            b = min(255, b + white);
        }
        
        pixels->setPixelColor(i, pixels->Color(r, g, b));
    }
    
    showPixels();
}

// Fire Breath - Red/orange fire effect
void StripLEDController::effectFireBreath() {
    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - effectStartTime;
    
    // Effect duration: 3 seconds
    if (elapsed > 3000) {
        effectRunning = false;
        currentEffect = EFFECT_IDLE;
        effectStartTime = millis();
        return;
    }
    
    // Create fire-like flickering
    float phase = (elapsed % 300) / 300.0f;
    
    for (int i = 0; i < numPixels; i++) {
        // Fire intensity varies by position and time
        float position = (float)i / numPixels;
        float flicker = sin((position * 8.0f + phase * 4.0f) * PI) * 0.5f + 0.5f;
        flicker += sin((position * 13.0f + phase * 6.0f) * PI) * 0.3f;
        flicker = constrain(flicker, 0.0f, 1.0f);
        
        // Red/orange gradient (more red at base, more orange/yellow at top)
        uint8_t r = (uint8_t)(flicker * 255);
        uint8_t g = (uint8_t)(flicker * position * 180);
        uint8_t b = (uint8_t)(flicker * position * 50);
        
        pixels->setPixelColor(i, pixels->Color(r, g, b));
    }
    
    showPixels();
}

// Electric Attack - White/blue electric flashes
void StripLEDController::effectElectricAttack() {
    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - effectStartTime;
    
    // Effect duration: 1.5 seconds
    if (elapsed > 1500) {
        effectRunning = false;
        currentEffect = EFFECT_IDLE;
        effectStartTime = millis();
        return;
    }
    
    // Rapid white/blue flashes
    float phase = (elapsed % 100) / 100.0f; // 100ms flash cycle
    float flash = sin(phase * PI * 2);
    flash = max(0.0f, flash); // Only positive values
    
    // White/blue color
    uint8_t r = (uint8_t)(flash * 255);
    uint8_t g = (uint8_t)(flash * 200);
    uint8_t b = (uint8_t)(flash * 255);
    
    for (int i = 0; i < numPixels; i++) {
        pixels->setPixelColor(i, pixels->Color(r, g, b));
    }
    
    showPixels();
}

// Battle Damage - Red flashing/strobing
void StripLEDController::effectBattleDamage() {
    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - effectStartTime;
    
    // Effect duration: 2 seconds
    if (elapsed > 2000) {
        effectRunning = false;
        currentEffect = EFFECT_IDLE;
        effectStartTime = millis();
        return;
    }
    
    // Rapid red flashes
    float phase = (elapsed % 150) / 150.0f;
    float flash = sin(phase * PI * 2);
    flash = max(0.0f, flash);
    
    uint8_t r = (uint8_t)(flash * 255);
    uint8_t g = 0;
    uint8_t b = 0;
    
    for (int i = 0; i < numPixels; i++) {
        pixels->setPixelColor(i, pixels->Color(r, g, b));
    }
    
    showPixels();
}

// Victory - Green/gold celebration
void StripLEDController::effectVictory() {
    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - effectStartTime;
    
    // Effect duration: 4 seconds
    if (elapsed > 4000) {
        effectRunning = false;
        currentEffect = EFFECT_IDLE;
        effectStartTime = millis();
        return;
    }
    
    // Rotating green/gold pattern
    float phase = (elapsed % 800) / 800.0f;
    
    for (int i = 0; i < numPixels; i++) {
        float position = ((float)i / numPixels) + phase;
        position = position - floor(position); // Wrap around
        
        // Green/gold gradient
        uint8_t r, g, b;
        if (position < 0.5f) {
            // Green
            r = 0;
            g = 255;
            b = 0;
        } else {
            // Gold
            r = 255;
            g = 200;
            b = 0;
        }
        
        pixels->setPixelColor(i, pixels->Color(r, g, b));
    }
    
    showPixels();
}

// Idle - Subtle pulsing animation
void StripLEDController::effectIdle() {
    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - effectStartTime;
    
    // Slow breathing pulse
    float phase = (elapsed % 3000) / 3000.0f; // 3 second cycle
    float intensity = sin(phase * PI * 2) * 0.1f + 0.1f; // 0.0 to 0.2 range
    
    // Subtle blue/cyan color
    uint8_t r = 0;
    uint8_t g = (uint8_t)(intensity * 100);
    uint8_t b = (uint8_t)(intensity * 150);
    
    for (int i = 0; i < numPixels; i++) {
        pixels->setPixelColor(i, pixels->Color(r, g, b));
    }
    
    showPixels();
}

// Popular general effects
void StripLEDController::effectRainbow() {
    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - effectStartTime;
    
    // Smooth rainbow color cycle
    uint16_t hue = (uint16_t)((uint32_t)(elapsed * 2) % 65536);
    
    for (int i = 0; i < numPixels; i++) {
        uint16_t pixelHue = (hue + (i * 65536L / numPixels)) % 65536;
        pixels->setPixelColor(i, colorWheel((byte)(pixelHue >> 8)));
    }
    
    showPixels();
}

void StripLEDController::effectRainbowChase() {
    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - effectStartTime;
    float speed = 0.5f;
    
    for (int i = 0; i < numPixels; i++) {
        uint16_t hue = (uint16_t)(((uint32_t)(elapsed * speed * 256) + (i * 65536L / 8)) % 65536);
        pixels->setPixelColor(i, colorWheel((byte)(hue >> 8)));
    }
    
    showPixels();
}

void StripLEDController::effectColorWipe() {
    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - effectStartTime;
    float speed = 50.0f; // pixels per second
    
    int pixelIndex = (int)((elapsed / 1000.0f) * speed) % (numPixels * 2);
    
    for (int i = 0; i < numPixels; i++) {
        if (i < pixelIndex && i >= (pixelIndex - numPixels)) {
            pixels->setPixelColor(i, pixels->Color(0, 255, 0));
        } else {
            pixels->setPixelColor(i, 0);
        }
    }
    
    showPixels();
}

void StripLEDController::effectTheaterChase() {
    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - effectStartTime;
    
    int spacing = 3;
    int position = (elapsed / 100) % (spacing * 2);
    
    for (int i = 0; i < numPixels; i++) {
        if ((i + position) % (spacing * 2) < spacing) {
            pixels->setPixelColor(i, pixels->Color(255, 0, 0));
        } else {
            pixels->setPixelColor(i, 0);
        }
    }
    
    showPixels();
}

void StripLEDController::effectPulse() {
    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - effectStartTime;
    
    float phase = (elapsed % 2000) / 2000.0f;
    float intensity = sin(phase * PI * 2) * 0.5f + 0.5f;
    
    uint8_t r = (uint8_t)(intensity * 255);
    uint8_t g = (uint8_t)(intensity * 100);
    uint8_t b = (uint8_t)(intensity * 150);
    
    setAllPixels(pixels->Color(r, g, b));
    showPixels();
}

void StripLEDController::effectBreathing() {
    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - effectStartTime;
    
    float phase = (elapsed % 4000) / 4000.0f;
    float intensity = sin(phase * PI * 2) * 0.3f + 0.3f;
    
    uint8_t r = (uint8_t)(intensity * 255);
    uint8_t g = (uint8_t)(intensity * 50);
    uint8_t b = (uint8_t)(intensity * 100);
    
    setAllPixels(pixels->Color(r, g, b));
    showPixels();
}

void StripLEDController::effectMeteor() {
    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - effectStartTime;
    float speed = 0.5f;
    
    fadeToBlack(20);
    
    int meteorPos = (int)((elapsed / 1000.0f) * speed * numPixels) % (numPixels + 20);
    
    for (int i = 0; i < 10; i++) {
        int pos = meteorPos - i;
        if (pos >= 0 && pos < numPixels) {
            float intensity = 1.0f - ((float)i / 10.0f);
            uint8_t r = (uint8_t)(intensity * 255);
            uint8_t g = (uint8_t)(intensity * 200);
            uint8_t b = (uint8_t)(intensity * 100);
            pixels->setPixelColor(pos, pixels->Color(r, g, b));
        }
    }
    
    showPixels();
}

void StripLEDController::effectTwinkle() {
    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - effectStartTime;
    
    fadeToBlack(30);
    
    // Random twinkles
    for (int i = 0; i < numPixels / 10; i++) {
        int pos = (elapsed * 7 + i * 7919) % numPixels;
        float intensity = sin((elapsed * 0.01f + i) * PI * 2) * 0.5f + 0.5f;
        uint8_t r = (uint8_t)(intensity * 255);
        uint8_t g = (uint8_t)(intensity * 255);
        uint8_t b = (uint8_t)(intensity * 255);
        pixels->setPixelColor(pos, pixels->Color(r, g, b));
    }
    
    showPixels();
}

void StripLEDController::effectWater() {
    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - effectStartTime;
    
    float phase = (elapsed % 2000) / 2000.0f;
    
    for (int i = 0; i < numPixels; i++) {
        float position = (float)i / numPixels;
        float wave = sin((position * 4.0f + phase * 2.0f) * PI) * 0.5f + 0.5f;
        uint8_t r = 0;
        uint8_t g = (uint8_t)(wave * 100);
        uint8_t b = (uint8_t)(wave * 255);
        pixels->setPixelColor(i, pixels->Color(r, g, b));
    }
    
    showPixels();
}

void StripLEDController::effectStrobe() {
    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - effectStartTime;
    
    float phase = (elapsed % 200) / 200.0f;
    bool flash = phase < 0.1f;
    
    uint32_t color = flash ? pixels->Color(255, 255, 255) : 0;
    setAllPixels(color);
    showPixels();
}

// Edge-lit acrylic disc effects
void StripLEDController::effectRadialOut() {
    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - effectStartTime;
    float speed = 0.3f;
    
    int center = numPixels / 2;
    float maxDistance = numPixels / 2.0f;
    float wave = (elapsed / 1000.0f) * speed;
    
    for (int i = 0; i < numPixels; i++) {
        float distance = abs(i - center) / maxDistance;
        float intensity = sin((wave - distance) * PI * 2);
        intensity = max(0.0f, intensity);
        
        uint16_t hue = (uint16_t)((uint32_t)(elapsed * 0.1f + distance * 256) % 65536);
        uint32_t color = colorWheel((byte)(hue >> 8));
        uint8_t r = (uint8_t)((color >> 16) & 0xFF);
        uint8_t g = (uint8_t)((color >> 8) & 0xFF);
        uint8_t b = (uint8_t)(color & 0xFF);
        
        r = (uint8_t)(r * intensity);
        g = (uint8_t)(g * intensity);
        b = (uint8_t)(b * intensity);
        
        pixels->setPixelColor(i, pixels->Color(r, g, b));
    }
    
    showPixels();
}

void StripLEDController::effectRadialIn() {
    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - effectStartTime;
    float speed = 0.3f;
    
    int center = numPixels / 2;
    float maxDistance = numPixels / 2.0f;
    float wave = (elapsed / 1000.0f) * speed;
    
    for (int i = 0; i < numPixels; i++) {
        float distance = abs(i - center) / maxDistance;
        float intensity = sin((wave + distance) * PI * 2);
        intensity = max(0.0f, intensity);
        
        uint16_t hue = (uint16_t)((uint32_t)(elapsed * 0.1f + distance * 256) % 65536);
        uint32_t color = colorWheel((byte)(hue >> 8));
        uint8_t r = (uint8_t)((color >> 16) & 0xFF);
        uint8_t g = (uint8_t)((color >> 8) & 0xFF);
        uint8_t b = (uint8_t)(color & 0xFF);
        
        r = (uint8_t)(r * intensity);
        g = (uint8_t)(g * intensity);
        b = (uint8_t)(b * intensity);
        
        pixels->setPixelColor(i, pixels->Color(r, g, b));
    }
    
    showPixels();
}

void StripLEDController::effectSpiral() {
    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - effectStartTime;
    float speed = 0.05f;
    
    for (int i = 0; i < numPixels; i++) {
        float position = (float)i / numPixels;
        float phase = (elapsed / 1000.0f) * speed + position;
        float intensity = sin(phase * PI * 2) * 0.5f + 0.5f;
        
        uint16_t hue = (uint16_t)((uint32_t)(elapsed * 0.05f + position * 256) % 65536);
        uint32_t color = colorWheel((byte)(hue >> 8));
        uint8_t r = (uint8_t)((color >> 16) & 0xFF);
        uint8_t g = (uint8_t)((color >> 8) & 0xFF);
        uint8_t b = (uint8_t)(color & 0xFF);
        
        r = (uint8_t)(r * intensity);
        g = (uint8_t)(g * intensity);
        b = (uint8_t)(b * intensity);
        
        pixels->setPixelColor(i, pixels->Color(r, g, b));
    }
    
    showPixels();
}

void StripLEDController::effectRotatingRainbow() {
    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - effectStartTime;
    
    int center = numPixels / 2;
    float rotation = (elapsed / 1000.0f) * 0.5f;
    
    for (int i = 0; i < numPixels; i++) {
        float position = (float)(i - center) / numPixels;
        uint16_t hue = (uint16_t)((uint32_t)((position + rotation) * 256) % 65536);
        pixels->setPixelColor(i, colorWheel((byte)(hue >> 8)));
    }
    
    showPixels();
}

void StripLEDController::effectCircularChase() {
    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - effectStartTime;
    float speed = 0.5f;
    
    int chasePos = (int)((elapsed / 1000.0f) * speed * numPixels) % numPixels;
    
    for (int i = 0; i < numPixels; i++) {
        int distance = abs(i - chasePos);
        if (distance > numPixels / 2) {
            distance = numPixels - distance;
        }
        
        float intensity = 1.0f - ((float)distance / (numPixels / 2.0f));
        intensity = max(0.0f, intensity);
        
        pixels->setPixelColor(i, pixels->Color(0, (uint8_t)(intensity * 255), 0));
    }
    
    showPixels();
}

void StripLEDController::effectRadialGradient() {
    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - effectStartTime;
    
    int center = numPixels / 2;
    float maxDistance = numPixels / 2.0f;
    float hueOffset = (elapsed / 1000.0f) * 50.0f;
    
    for (int i = 0; i < numPixels; i++) {
        float distance = abs(i - center) / maxDistance;
        uint16_t hue = (uint16_t)((uint32_t)(hueOffset + distance * 256) % 65536);
        pixels->setPixelColor(i, colorWheel((byte)(hue >> 8)));
    }
    
    showPixels();
}

// Helper functions
uint32_t StripLEDController::colorWheel(byte wheelPos) {
    wheelPos = 255 - wheelPos;
    if (wheelPos < 85) {
        return pixels->Color(255 - wheelPos * 3, 0, wheelPos * 3);
    } else if (wheelPos < 170) {
        wheelPos -= 85;
        return pixels->Color(0, wheelPos * 3, 255 - wheelPos * 3);
    } else {
        wheelPos -= 170;
        return pixels->Color(wheelPos * 3, 255 - wheelPos * 3, 0);
    }
}

uint32_t StripLEDController::blendColor(uint32_t color1, uint32_t color2, float ratio) {
    ratio = constrain(ratio, 0.0f, 1.0f);
    uint8_t r1 = (color1 >> 16) & 0xFF;
    uint8_t g1 = (color1 >> 8) & 0xFF;
    uint8_t b1 = color1 & 0xFF;
    uint8_t r2 = (color2 >> 16) & 0xFF;
    uint8_t g2 = (color2 >> 8) & 0xFF;
    uint8_t b2 = color2 & 0xFF;
    
    uint8_t r = (uint8_t)(r1 * (1.0f - ratio) + r2 * ratio);
    uint8_t g = (uint8_t)(g1 * (1.0f - ratio) + g2 * ratio);
    uint8_t b = (uint8_t)(b1 * (1.0f - ratio) + b2 * ratio);
    
    return pixels->Color(r, g, b);
}

void StripLEDController::setAllPixels(uint32_t color) {
    for (int i = 0; i < numPixels; i++) {
        pixels->setPixelColor(i, color);
    }
}

void StripLEDController::fadeToBlack(int fadeValue) {
    for (int i = 0; i < numPixels; i++) {
        uint32_t color = pixels->getPixelColor(i);
        uint8_t r = (color >> 16) & 0xFF;
        uint8_t g = (color >> 8) & 0xFF;
        uint8_t b = color & 0xFF;
        
        r = (r * fadeValue) >> 8;
        g = (g * fadeValue) >> 8;
        b = (b * fadeValue) >> 8;
        
        pixels->setPixelColor(i, pixels->Color(r, g, b));
    }
}

void StripLEDController::showPixels() {
    pixels->show();
}

void StripLEDController::setPixelBrightness(uint8_t brightness) {
    currentBrightness = brightness;
    pixels->setBrightness(brightness);
}

// Public methods
void StripLEDController::setBrightness(uint8_t brightness) {
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

uint8_t StripLEDController::getBrightness() const {
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

StripLEDController::EffectType StripLEDController::getCurrentEffect() const {
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

bool StripLEDController::isEffectRunning() const {
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

void StripLEDController::update() {
    // Effects run in a separate task, so this is mostly a placeholder
    // Can be used for synchronization if needed
}
