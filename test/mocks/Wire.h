#ifndef WIRE_H_MOCK
#define WIRE_H_MOCK

#include <cstdint>

class TwoWireMock {
public:
    void begin(int, int) {}
    void beginTransmission(uint8_t) {}
    uint8_t endTransmission(bool = true) { return 0; }
    uint8_t requestFrom(uint8_t, uint8_t) { return 0; }
    void write(uint8_t) {}
    int available() { return 0; }
    uint8_t read() { return 0xFF; }
};

static TwoWireMock Wire;

#endif // WIRE_H_MOCK
