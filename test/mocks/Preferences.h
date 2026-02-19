#ifndef PREFERENCES_H_MOCK
#define PREFERENCES_H_MOCK

#include "Arduino.h"
#include <map>
#include <string>

class Preferences {
public:
    bool begin(const char* name, bool readOnly = false) {
        (void)readOnly;
        _namespace = name ? name : "";
        return true;
    }

    void end() {}

    bool clear() {
        _strings.clear();
        _ints.clear();
        _bools.clear();
        return true;
    }

    // String operations
    size_t putString(const char* key, const char* value) {
        _strings[key] = value ? value : "";
        return _strings[key].length();
    }

    size_t putString(const char* key, const String& value) {
        return putString(key, value.c_str());
    }

    String getString(const char* key, const char* defaultValue = "") {
        auto it = _strings.find(key);
        if (it != _strings.end()) {
            return String(it->second.c_str());
        }
        return String(defaultValue ? defaultValue : "");
    }

    // Int operations
    size_t putInt(const char* key, int32_t value) {
        _ints[key] = value;
        return sizeof(int32_t);
    }

    int32_t getInt(const char* key, int32_t defaultValue = 0) {
        auto it = _ints.find(key);
        if (it != _ints.end()) {
            return it->second;
        }
        return defaultValue;
    }

    // Bool operations
    size_t putBool(const char* key, bool value) {
        _bools[key] = value;
        return sizeof(bool);
    }

    bool getBool(const char* key, bool defaultValue = false) {
        auto it = _bools.find(key);
        if (it != _bools.end()) {
            return it->second;
        }
        return defaultValue;
    }

private:
    std::string _namespace;
    std::map<std::string, std::string> _strings;
    std::map<std::string, int32_t> _ints;
    std::map<std::string, bool> _bools;
};

#endif // PREFERENCES_H_MOCK
