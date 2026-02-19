#ifndef SPI_H_MOCK
#define SPI_H_MOCK

#include <cstdint>

#define MSBFIRST 1
#define SPI_MODE0 0

struct SPISettings {
    SPISettings(uint32_t clock, uint8_t bitOrder, uint8_t dataMode) {
        (void)clock; (void)bitOrder; (void)dataMode;
    }
};

class SPIClass {
public:
    void begin(int sck, int miso, int mosi) {
        (void)sck; (void)miso; (void)mosi;
    }
    void beginTransaction(SPISettings settings) { (void)settings; }
    void endTransaction() {}
    void end() {}
};

inline SPIClass SPI;

#endif // SPI_H_MOCK
