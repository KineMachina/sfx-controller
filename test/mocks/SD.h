#ifndef SD_H_MOCK
#define SD_H_MOCK

#include "Arduino.h"
#include <cstring>

// File mock that can optionally hold readable content.
// Default-constructed File behaves as before (empty, bool false).
class File {
public:
    File() : _data(nullptr), _pos(0), _len(0) {}
    File(const char* data) : _data(data), _pos(0), _len(data ? strlen(data) : 0) {}

    operator bool() const { return _data != nullptr && _len > 0; }
    void close() { _data = nullptr; _pos = 0; _len = 0; }
    bool isDirectory() { return true; }
    int read() {
        if (!_data || _pos >= _len) return -1;
        return (unsigned char)_data[_pos++];
    }
    size_t readBytes(char* buffer, size_t length) {
        size_t count = 0;
        while (count < length && _data && _pos < _len) {
            buffer[count++] = _data[_pos++];
        }
        return count;
    }
    size_t size() { return _len; }

private:
    const char* _data;
    size_t _pos;
    size_t _len;
};

// SD card mock — by default all operations fail gracefully.
// Call setFileContent() to make exists() return true and open() return
// a File that yields the given content (used by effects-loading tests).
// Call setBeginResult(true) to make begin() succeed (used by AudioController tests).
class SDClass {
public:
    bool begin(uint8_t csPin = 0) { (void)csPin; return _beginResult; }
    void end() {}

    bool exists(const char* path) {
        (void)path;
        return _fileContent != nullptr;
    }
    bool exists(const String& path) { return exists(path.c_str()); }

    File open(const char* path, uint8_t mode = FILE_READ) {
        (void)path; (void)mode;
        if (_fileContent) return File(_fileContent);
        return File();
    }

    // Test helper: set content returned by the next open() call.
    // Pass nullptr to revert to default (everything fails).
    void setFileContent(const char* content) { _fileContent = content; }

    // Test helper: control whether begin() succeeds or fails.
    void setBeginResult(bool result) { _beginResult = result; }

private:
    const char* _fileContent = nullptr;
    bool _beginResult = false;
};

inline SDClass SD;

#endif // SD_H_MOCK
