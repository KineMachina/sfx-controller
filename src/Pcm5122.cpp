#include "Pcm5122.h"
#include <Wire.h>
#include "RuntimeLog.h"

static const char* TAG = "PCM5122";

Pcm5122::Pcm5122(uint8_t i2cAddress)
    : _addr(i2cAddress), _available(false) {
}

bool Pcm5122::begin(int sdaPin, int sclPin) {
    Wire.begin(sdaPin, sclPin);

    // Probe device
    Wire.beginTransmission(_addr);
    if (Wire.endTransmission() != 0) {
        ESP_LOGW(TAG, "DAC not found on I2C address 0x%02X", _addr);
        _available = false;
        return false;
    }

    _available = true;

    // Mute both channels before any audio starts
    writeRegister(0, 0x03, 0x11);

    // Clear standby — ensure DAC is active
    writeRegister(0, 0x02, 0x00);

    ESP_LOGI(TAG, "DAC initialized on I2C address 0x%02X", _addr);
    return true;
}

void Pcm5122::mute() {
    if (!_available) return;
    writeRegister(0, 0x03, 0x11);
    ESP_LOGD(TAG, "Muted");
}

void Pcm5122::unmute() {
    if (!_available) return;
    writeRegister(0, 0x03, 0x00);
    ESP_LOGD(TAG, "Unmuted");
}

bool Pcm5122::isConnected() {
    Wire.beginTransmission(_addr);
    return Wire.endTransmission() == 0;
}

void Pcm5122::selectPage(uint8_t page) {
    Wire.beginTransmission(_addr);
    Wire.write(0x00);  // Page select register
    Wire.write(page);
    Wire.endTransmission();
}

bool Pcm5122::writeRegister(uint8_t page, uint8_t reg, uint8_t val) {
    selectPage(page);
    Wire.beginTransmission(_addr);
    Wire.write(reg);
    Wire.write(val);
    uint8_t err = Wire.endTransmission();
    if (err != 0) {
        ESP_LOGW(TAG, "I2C write failed: page=%u reg=0x%02X err=%u", page, reg, err);
        return false;
    }
    return true;
}

uint8_t Pcm5122::readRegister(uint8_t page, uint8_t reg) {
    selectPage(page);
    Wire.beginTransmission(_addr);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom(_addr, (uint8_t)1);
    if (Wire.available()) {
        return Wire.read();
    }
    return 0xFF;
}
