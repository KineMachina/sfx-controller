#ifndef MATRIX_LED_CONTROLLER_H
#define MATRIX_LED_CONTROLLER_H

#include "Arduino.h"
#include <Adafruit_NeoMatrix.h>
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

/**
 * MatrixLEDController - Manages NeoPixel LED matrix panel effects
 * Handles 2D grid effects for LED matrix panels
 */
class MatrixLEDController {
public:
    enum EffectType {
        EFFECT_NONE = 0,
        // Battle Arena - Fight Behaviors
        EFFECT_BEAM_ATTACK,          // Horizontal/vertical beam across grid
        EFFECT_EXPLOSION,            // Expanding explosion from center/point
        EFFECT_IMPACT_WAVE,          // Circular shockwave from impact point
        EFFECT_DAMAGE_FLASH,         // Grid-wide damage flash with pattern
        EFFECT_BLOCK_SHIELD,         // Defensive shield effect
        EFFECT_DODGE_TRAIL,          // Motion trail for dodge/evade
        EFFECT_CHARGE_UP,            // Energy charging effect
        EFFECT_FINISHER_BEAM,        // Massive beam attack (full grid)
        EFFECT_GRAVITY_BEAM,         // King Ghidorah triple gravity beam (gold lightning)
        EFFECT_ELECTRIC_ATTACK_MATRIX,  // White/blue electric attack, 5 second duration
        // Battle Arena - Dance Behaviors  
        EFFECT_VICTORY_DANCE,        // Celebratory dance pattern
        EFFECT_TAUNT_PATTERN,        // Taunting animation
        EFFECT_POSE_STRIKE,          // Dramatic pose with flash
        EFFECT_CELEBRATION_WAVE,     // Wave of celebration across grid
        EFFECT_CONFETTI,             // Confetti/particle celebration
        EFFECT_HEART_EYES,           // Heart eyes pattern (cute victory)
        EFFECT_POWER_UP_AURA,        // Aura effect for power-up
        EFFECT_TRANSITION_FADE,      // Smooth transition between effects
        // Panel Effects
        EFFECT_GAME_OVER_CHIRON,     // Scrolling "GAME OVER" text
        EFFECT_CYLON_EYE,            // Red scanning eye animation
        EFFECT_PERLIN_INFERNO,       // Perlin noise fire effect
        EFFECT_EMP_LIGHTNING,        // Lightning storm effect
        EFFECT_GAME_OF_LIFE,         // Conway's Game of Life
        EFFECT_PLASMA_CLOUDS,        // Plasma cloud effect
        EFFECT_DIGITAL_FIREFLIES,    // Digital fireflies effect
        EFFECT_MATRIX_RAIN,          // Matrix-style falling characters
        EFFECT_DANCE_FLOOR,          // Disco dance floor effect
        EFFECT_ALIEN_CONTROL_PANEL,  // Alien spaceship pulsing control panel
        EFFECT_ATOMIC_BREATH_MINUS_ONE,  // Godzilla Minus One - purple/magenta atomic breath
        EFFECT_ATOMIC_BREATH_MUSHROOM,   // Finishing move - atomic breath mushroom cloud
        EFFECT_TRANSPORTER_TOS           // Star Trek TOS transporter - 6s shimmer/sparkle
    };

private:
    // Hardware
    Adafruit_NeoMatrix* matrix;
    int pin;
    int gridWidth;
    int gridHeight;
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
    // Battle Arena - Fight Behaviors
    void effectBeamAttack();
    void effectExplosion();
    void effectImpactWave();
    void effectDamageFlash();
    void effectBlockShield();
    void effectDodgeTrail();
    void effectChargeUp();
    void effectFinisherBeam();
    void effectGravityBeam();
    void effectElectricAttackMatrix();
    // Battle Arena - Dance Behaviors
    void effectVictoryDance();
    void effectTauntPattern();
    void effectPoseStrike();
    void effectCelebrationWave();
    void effectConfetti();
    void effectHeartEyes();
    void effectPowerUpAura();
    void effectTransitionFade();
    // Panel Effects
    void effectGameOverChiron();
    void effectCylonEye();
    void effectPerlinInferno();
    void effectEmpLightning();
    void effectGameOfLife();
    void effectPlasmaClouds();
    void effectDigitalFireflies();
    void effectMatrixRain();
    void effectDanceFloor();
    void effectAlienControlPanel();
    void effectAtomicBreathMinusOne();
    void effectAtomicBreathMushroom();
    void effectTransporterTOS();
    
    // Helper functions for matrix effects
    float perlinNoise2D(float x, float y, float time);
    void drawChar(int x, int y, char c, uint32_t color);
    uint8_t getCharBitmap(char c, int row);
    void drawLightningBolt(int x1, int y1, int y2, uint32_t color, int thickness);
    
    // Helper functions
    uint32_t colorWheel(byte wheelPos);
    uint32_t blendColor(uint32_t color1, uint32_t color2, float ratio);
    void setAllPixels(uint32_t color);
    void fadeToBlack(int fadeValue);
    void showPixels();
    void setPixelBrightness(uint8_t brightness);
    
    // Grid helper functions
    int xyToIndex(int x, int y) const;
    void indexToXY(int index, int& x, int& y) const;
    void setPixelXY(int x, int y, uint32_t color);
    uint32_t getPixelXY(int x, int y) const;
    float distance2D(int x1, int y1, int x2, int y2) const;
    void drawLine(int x1, int y1, int x2, int y2, uint32_t color);
    void drawCircle(int cx, int cy, int radius, uint32_t color, bool filled = false);
    void drawRectangle(int x1, int y1, int x2, int y2, uint32_t color, bool filled = false);
    
public:
    /**
     * Constructor
     * @param pin NeoPixel data pin
     * @param gridWidth Grid width
     * @param gridHeight Grid height
     */
    MatrixLEDController(int pin, int gridWidth, int gridHeight);
    
    /**
     * Destructor
     */
    ~MatrixLEDController();
    
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
    
    /**
     * Get grid width
     * @return Grid width
     */
    int getGridWidth() const { return gridWidth; }
    
    /**
     * Get grid height
     * @return Grid height
     */
    int getGridHeight() const { return gridHeight; }
};

#endif // MATRIX_LED_CONTROLLER_H
