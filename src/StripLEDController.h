#ifndef STRIP_LED_CONTROLLER_H
#define STRIP_LED_CONTROLLER_H

#include "Arduino.h"
#include <Adafruit_NeoPixel.h>
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

/**
 * StripLEDController - Manages NeoPixel LED strip effects
 * Handles linear/1D effects for configurable-length LED strips
 */
class StripLEDController {
public:
    enum EffectType {
        EFFECT_NONE = 0,
        // Themed effects
        EFFECT_ATOMIC_BREATH,      // Godzilla - Blue/cyan pulsing stream
        EFFECT_GRAVITY_BEAM,       // Ghidorah - Gold/yellow lightning
        EFFECT_FIRE_BREATH,        // Generic fire attack - Red/orange
        EFFECT_ELECTRIC_ATTACK,    // Electric attack - White/blue flashes
        EFFECT_BATTLE_DAMAGE,      // Red flashing/strobing
        EFFECT_VICTORY,            // Green/gold celebration
        EFFECT_IDLE,               // Subtle idle animation
        // Popular general effects
        EFFECT_RAINBOW,             // Smooth rainbow color cycle
        EFFECT_RAINBOW_CHASE,       // Rainbow colors chasing around strip
        EFFECT_COLOR_WIPE,          // Color wipe across strip
        EFFECT_THEATER_CHASE,       // Theater marquee chase effect
        EFFECT_PULSE,               // Smooth pulsing color
        EFFECT_BREATHING,           // Breathing effect (slow pulse)
        EFFECT_METEOR,              // Meteor/trail effect
        EFFECT_TWINKLE,             // Random twinkling stars
        EFFECT_WATER,               // Water/ripple effect
        EFFECT_STROBE,              // Strobe/flash effect
        // Edge-lit acrylic disc effects
        EFFECT_RADIAL_OUT,          // Radial pattern from center outward
        EFFECT_RADIAL_IN,           // Radial pattern from edge inward
        EFFECT_SPIRAL,              // Spiral pattern rotating
        EFFECT_ROTATING_RAINBOW,    // Rotating rainbow around disc
        EFFECT_CIRCULAR_CHASE,      // Circular chase around disc
        EFFECT_RADIAL_GRADIENT      // Radial color gradient
    };

private:
    // Hardware
    Adafruit_NeoPixel* pixels;
    int pin;
    int numPixels;
    
    // Effect state (protected by queues)
    EffectType currentEffect;
    bool effectRunning;
    unsigned long effectStartTime;
    unsigned long lastUpdate;
    uint8_t currentBrightness;  // Track current brightness
    
    // FreeRTOS task and queues
    TaskHandle_t ledTaskHandle;
    QueueHandle_t ledCommandQueue;
    QueueHandle_t ledStatusQueue;
    
    // Command structure for queue
    struct LEDCommand {
        enum Type {
            CMD_START_EFFECT,
            CMD_STOP_EFFECT,
            CMD_SET_BRIGHTNESS
        };
        Type type;
        EffectType effect;        // For START_EFFECT command
        uint8_t brightness;       // For SET_BRIGHTNESS command
    };
    
    // Status structure for queue
    struct LEDStatus {
        EffectType currentEffect;
        bool effectRunning;
        uint8_t brightness;
        unsigned long effectStartTime;
    };
    
    // Effect start callback (called when an effect starts)
    typedef void (*EffectStartCallback)(EffectType effect, void* userData);
    EffectStartCallback effectStartCallback;
    void* effectStartCallbackData;
    
    // Internal methods
    static void ledTaskWrapper(void* parameter);
    void ledTask();
    void updateEffect();
    
    // Effect implementations
    void effectAtomicBreath();
    void effectGravityBeam();
    void effectFireBreath();
    void effectElectricAttack();
    void effectBattleDamage();
    void effectVictory();
    void effectIdle();
    // Popular general effects
    void effectRainbow();
    void effectRainbowChase();
    void effectColorWipe();
    void effectTheaterChase();
    void effectPulse();
    void effectBreathing();
    void effectMeteor();
    void effectTwinkle();
    void effectWater();
    void effectStrobe();
    // Edge-lit acrylic disc effects
    void effectRadialOut();
    void effectRadialIn();
    void effectSpiral();
    void effectRotatingRainbow();
    void effectCircularChase();
    void effectRadialGradient();
    
    // Helper functions
    uint32_t colorWheel(byte wheelPos);
    uint32_t blendColor(uint32_t color1, uint32_t color2, float ratio);
    void setAllPixels(uint32_t color);
    void fadeToBlack(int fadeValue);
    void showPixels();
    void setPixelBrightness(uint8_t brightness);
    
public:
    /**
     * Constructor
     * @param pin NeoPixel data pin
     * @param numPixels Number of NeoPixels in the strip
     */
    StripLEDController(int pin, int numPixels);
    
    /**
     * Destructor
     */
    ~StripLEDController();
    
    /**
     * Initialize LED controller
     * @return true if successful, false otherwise
     */
    bool begin();
    
    /**
     * Start a specific effect
     * @param effect The effect type to play
     */
    void startEffect(EffectType effect);
    
    /**
     * Set callback function to be called when an effect starts
     * @param callback Callback function (nullptr to disable)
     * @param userData User data to pass to callback
     */
    void setEffectStartCallback(EffectStartCallback callback, void* userData = nullptr);
    
    /**
     * Stop current effect and return to idle
     */
    void stopEffect();
    
    /**
     * Get current effect type
     * @return Current effect type
     */
    EffectType getCurrentEffect() const;
    
    /**
     * Check if an effect is currently running
     * @return true if effect is running, false otherwise
     */
    bool isEffectRunning() const;
    
    /**
     * Set brightness (0-255)
     * @param brightness Brightness level
     */
    void setBrightness(uint8_t brightness);
    
    /**
     * Get current brightness
     * @return Current brightness level (0-255)
     */
    uint8_t getBrightness() const;
    
    /**
     * Update loop - call this frequently from main loop
     * (Effects run in a separate task, but this can be used for sync)
     */
    void update();
};

#endif // STRIP_LED_CONTROLLER_H
