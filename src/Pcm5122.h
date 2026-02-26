#ifndef PCM5122_H
#define PCM5122_H

#include <stdint.h>

class Pcm5122 {
public:
    Pcm5122(uint8_t i2cAddress = 0x4C);

    bool begin(int sdaPin, int sclPin);
    void mute();
    void unmute();
    bool isConnected();

private:
    uint8_t _addr;
    bool _available;

    bool writeRegister(uint8_t page, uint8_t reg, uint8_t val);
    uint8_t readRegister(uint8_t page, uint8_t reg);
    void selectPage(uint8_t page);
};

#endif // PCM5122_H
