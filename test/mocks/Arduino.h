#ifndef ARDUINO_H_MOCK
#define ARDUINO_H_MOCK

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

// Base Arduino types
typedef uint8_t byte;
typedef bool boolean;

// Arduino constants
#define LOW  0
#define HIGH 1
#define INPUT  0
#define OUTPUT 1

// File mode constant used by SD library
#define FILE_READ 0

// Minimal String class mirroring Arduino String
class String {
public:
    String() : _str() {}
    String(const char* s) : _str(s ? s : "") {}
    String(const String& other) : _str(other._str) {}
    String(int value) : _str(std::to_string(value)) {}
    String(unsigned long value) : _str(std::to_string(value)) {}

    String& operator=(const String& other) { _str = other._str; return *this; }
    String& operator=(const char* s) { _str = s ? s : ""; return *this; }

    bool operator==(const String& other) const { return _str == other._str; }
    bool operator==(const char* s) const { return _str == (s ? s : ""); }
    bool operator!=(const String& other) const { return _str != other._str; }
    bool operator!=(const char* s) const { return _str != (s ? s : ""); }

    String operator+(const String& other) const { return String((_str + other._str).c_str()); }
    String operator+(const char* s) const { return String((_str + (s ? s : "")).c_str()); }

    const char* c_str() const { return _str.c_str(); }
    unsigned int length() const { return (unsigned int)_str.length(); }

    void toLowerCase() {
        std::transform(_str.begin(), _str.end(), _str.begin(),
                       [](unsigned char c) { return std::tolower(c); });
    }

    void trim() {
        // Trim leading whitespace
        size_t start = _str.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) { _str.clear(); return; }
        // Trim trailing whitespace
        size_t end = _str.find_last_not_of(" \t\r\n");
        _str = _str.substr(start, end - start + 1);
    }

    char charAt(unsigned int index) const {
        if (index < _str.length()) return _str[index];
        return 0;
    }

    int indexOf(char c) const {
        auto pos = _str.find(c);
        return pos == std::string::npos ? -1 : (int)pos;
    }

    String substring(unsigned int from) const {
        if (from >= _str.length()) return String();
        return String(_str.substr(from).c_str());
    }

    String substring(unsigned int from, unsigned int to) const {
        if (from >= _str.length()) return String();
        return String(_str.substr(from, to - from).c_str());
    }

    // Append support (used by ArduinoJson's ArduinoStringWriter)
    bool concat(const char* s) {
        if (s) _str.append(s);
        return true;
    }
    bool concat(char c) { _str += c; return true; }

    // Implicit conversion to std::string for ArduinoJson compatibility
    operator std::string() const { return _str; }

    // Construct from std::string
    String(const std::string& s) : _str(s) {}

private:
    std::string _str;
};

// Minimal Serial stub (no-op)
class SerialMock {
public:
    void begin(unsigned long) {}
    void print(const char*) {}
    void print(int) {}
    void print(unsigned long) {}
    void print(const String& s) { (void)s; }
    void println() {}
    void println(const char*) {}
    void println(int) {}
    void println(unsigned long) {}
    void println(const String& s) { (void)s; }
    void printf(const char*, ...) {}
};

static SerialMock Serial;

// Timing stubs
inline unsigned long millis() { return 0; }
inline void delay(unsigned long) {}

#endif // ARDUINO_H_MOCK
