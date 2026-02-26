#ifndef AUDIO_CONTROLLER_H
#define AUDIO_CONTROLLER_H

#include "Arduino.h"
#include "Audio.h"
#include "SPI.h"
#include "SD.h"
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include "Pcm5122.h"

/**
 * AudioController - Manages audio playback from SD card to PCM5122 DAC
 * Handles file playback, volume control, and SD card management
 */
class AudioController {
private:
    // Audio hardware
    Audio* audio;
    int i2sBckPin;
    int i2sLrcPin;
    int i2sDoutPin;

    // I2C DAC control
    Pcm5122 dac;
    int i2cSdaPin;
    int i2cSclPin;

    // SD card configuration
    int sdCsPin;
    int spiMosiPin;
    int spiMisoPin;
    int spiSckPin;
    SemaphoreHandle_t sdCardMutex;
    
    // Playback state (protected by queues)
    String currentFile;
    bool fileChanged;
    bool isSwitchingFile;
    int currentVolume;  // Track current volume level
    
    // FreeRTOS task and queues
    TaskHandle_t audioTaskHandle;
    QueueHandle_t audioCommandQueue;
    QueueHandle_t audioStatusQueue;
    
    // Command structure for queue
    struct AudioCommand {
        enum Type {
            CMD_PLAY,
            CMD_STOP,
            CMD_SET_VOLUME,
            CMD_GET_STATUS
        };
        Type type;
        char filename[128];  // For PLAY command
        int volume;          // For SET_VOLUME command
    };
    
    // Status structure for queue
    struct AudioStatus {
        bool isPlaying;
        char currentFile[128];
        int volume;
        unsigned long playbackTime;
    };
    
    // Internal methods
    bool initSDCard();
    void initI2S();
    void initDac();
    static void audioTaskWrapper(void* parameter);
    void audioTask();
    
public:
    /**
     * Constructor
     * @param audioObj Audio object instance
     * @param bck I2S bit clock pin
     * @param lrc I2S left/right clock pin
     * @param dout I2S data output pin
     * @param cs SD card chip select pin
     * @param mosi SPI MOSI pin
     * @param miso SPI MISO pin
     * @param sck SPI clock pin
     */
    AudioController(Audio* audioObj,
                    int bck, int lrc, int dout,
                    int cs, int mosi, int miso, int sck,
                    int i2cSda = -1, int i2cScl = -1);
    
    /**
     * Initialize audio system (SD card, I2S, start task)
     * @return true if successful, false otherwise
     */
    bool begin();
    
    /**
     * Play a file from SD card
     * @param filename Path to audio file (e.g., "/sounds/test.mp3")
     * @return true if file queued successfully, false otherwise
     */
    bool playFile(const String& filename);
    
    /**
     * Stop current playback
     */
    void stop();
    
    /**
     * Set volume level
     * @param volume Volume level (0-21, where 21 is maximum)
     */
    void setVolume(int volume);
    
    /**
     * Get current volume
     * @return Current volume level (0-21)
     */
    int getVolume() const;
    
    /**
     * Check if audio is currently playing
     * @return true if playing, false otherwise
     */
    bool isPlaying() const;
    
    /**
     * Get current file being played
     * @return Current file path
     */
    String getCurrentFile() const;
    
    /**
     * Check if file exists on SD card
     * @param filename File path to check
     * @return true if file exists, false otherwise
     */
    bool fileExists(const String& filename);
    
    /**
     * Get SD card mutex (for external access to SD card)
     * @return Mutex handle
     */
    SemaphoreHandle_t getSDMutex() const { return sdCardMutex; }
    
    /**
     * Update loop - call this frequently from main loop
     * Handles file changes and audio processing
     */
    void update();
};

#endif // AUDIO_CONTROLLER_H
