#ifndef SD_H_MOCK
#define SD_H_MOCK

#include "Arduino.h"

// Minimal File stub
class File {
public:
    operator bool() const { return false; }
    void close() {}
    int read() { return -1; }
    size_t size() { return 0; }
};

// Minimal SD card stub — all operations fail gracefully
class SDClass {
public:
    bool begin(uint8_t csPin = 0) { (void)csPin; return false; }
    bool exists(const char* path) { (void)path; return false; }
    File open(const char* path, uint8_t mode = FILE_READ) { (void)path; (void)mode; return File(); }
};

static SDClass SD;

#endif // SD_H_MOCK
