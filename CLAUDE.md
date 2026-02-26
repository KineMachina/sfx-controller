# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

ESP32-S3 embedded controller for themed audio playback and LED effects. Built with Arduino framework on PlatformIO. Features dual independent LED outputs (strip + 16x16 matrix), I2S DAC audio from SD card, web UI, MQTT control, and FreeRTOS-based concurrency.

## Build Commands

```bash
pio run                          # Build
pio run -t upload                # Build and flash to ESP32-S3
pio device monitor               # Serial monitor (115200 baud)
pio run -t upload && pio device monitor  # Flash and monitor
```

Environments: `esp32-s3-devkitc-1` (target hardware) and `native` (host-side unit tests).

```bash
pio test -e native               # Run all native unit tests
pio test -e native -v             # Verbose test output
```

## Architecture

### Controller Pattern

All functionality is in controller classes instantiated in `main.cpp`. Controllers initialize sequentially in `setup()` (order matters for dependencies), then `loop()` calls each controller's `update()`.

**Initialization order and why it matters:**
1. **AudioController** — initializes SD card, creates the SD card mutex that other controllers need
2. **SettingsController** — loads `/config.json` from SD card (needs SD mutex from AudioController)
3. **StripLEDController** / **MatrixLEDController** — independent LED hardware
4. **DemoController** — references audio, LED, and settings controllers
5. **HTTPServerController** — starts WiFi and web server, receives references to all other controllers
6. **MQTTController** — connects to broker, receives references to other controllers

### FreeRTOS Task + Queue Pattern

Audio, LED (both), MQTT, and Demo controllers each run a dedicated FreeRTOS task. Communication between the main loop and tasks uses `xQueueSend`/`xQueueReceive` command queues. The SD card is protected by a shared `SemaphoreHandle_t` mutex created by AudioController.

```
Main loop (loop())  --[queue]--> AudioTask (priority 5)
                    --[queue]--> StripLEDTask (priority 3)
                    --[queue]--> MatrixLEDTask (priority 3)
                    --[queue]--> MQTTTask (priority 3)
                    --[queue]--> DemoTask (priority 2)
```

### Static Singleton Pattern for Callbacks

HTTPServerController and MQTTController use `static ControllerClass* instance` with static wrapper functions to bridge AsyncWebServer/AsyncMqttClient C-style callbacks to instance methods.

### Configuration System

Two sources, used by different subsystems:
- **`/config.json` on SD card** — audio/LED defaults, effects definitions, MQTT settings. Parsed with ArduinoJson.
- **ESP32 Preferences (NVS)** — WiFi credentials, MQTT config, volume, brightness, demo state. Set via web UI at runtime.

Effects are defined in config.json and can combine an audio file, LED effect name, loop flag, and battle category.

### Effect Execution Flow

`HTTPServerController::executeEffectByName()` or `MQTTController` resolves an effect from SettingsController → plays audio if defined → starts LED effect on strip or matrix → tracks looping state. Only one effect loops at a time; `updateLoops()` in the main loop handles restart.

## Pin Map (main.cpp)

| Function | Pin |
|----------|-----|
| I2S BCK | 7 |
| I2S LRC | 15 |
| I2S DOUT | 16 |
| I2C SDA | 8 |
| I2C SCL | 9 |
| SD CS | 10 |
| SPI MOSI | 11 |
| SPI SCK | 12 |
| SPI MISO | 13 |
| LED Strip | GPIO 4 |
| LED Matrix | GPIO 6 |

## Key Libraries

- `ESP32-audioI2S` — I2S audio playback
- `ESPAsyncWebServer` + `AsyncTCP` — non-blocking HTTP server
- `Adafruit NeoPixel` + `NeoMatrix` — LED control
- `ArduinoJson v7` — JSON parsing
- `AsyncMqttClient` — MQTT pub/sub

## Code Layout

All source in `src/`. Each controller is a `.h`/`.cpp` pair:

- **main.cpp** — entry point, pin defines, controller instantiation, `setup()`/`loop()`
- **AudioController** — I2S playback, SD card init, volume control, SD mutex owner, PCM5122 DAC mute/unmute
- **Pcm5122** — I2C driver for PCM5122 DAC anti-pop control (mute during playback transitions)
- **HTTPServerController** — WiFi management, 40+ REST API endpoints, web UI, effect loop tracking (largest file ~2700 LOC)
- **MatrixLEDController** — 31 effects for 16x16 grid (2nd largest ~2200 LOC)
- **StripLEDController** — 23 effects for linear LED arrays
- **MQTTController** — broker connection with mDNS resolution, exponential backoff reconnect, single command topic with JSON dispatch
- **SettingsController** — config.json parsing, effect definitions (up to 32), Preferences read/write
- **DemoController** — autonomous playback modes (audio+LED, audio-only, LED-only, effects)

## MQTT Topic Structure

Single command topic `{base_topic}/{device_id}/command` with JSON dispatch (`"command"` field selects the action). Status sub-topics (`status/audio`, `status/led`, `status/health`) are published separately. See `MQTT_API.md` for full protocol reference.

## Logging

**All diagnostic/debug output MUST use ESP-IDF structured logging.** Never use raw `Serial.print` for diagnostics.

```cpp
#include "RuntimeLog.h"
static const char* TAG = "MyController";

ESP_LOGE(TAG, "Critical failure: %s", error);     // Errors
ESP_LOGW(TAG, "Non-fatal issue: %s", warning);     // Warnings
ESP_LOGI(TAG, "Connected to %s:%d", host, port);   // Normal events
ESP_LOGD(TAG, "Heap: %u bytes", freeHeap);          // Periodic/verbose
```

### Log Levels

| Level | Use for | Examples |
|-------|---------|---------|
| ERROR | Critical failures requiring attention | Init failures, hardware errors, queue creation failures |
| WARN | Degraded operation, non-fatal issues | Missing optional components, using defaults, config warnings |
| INFO | Normal operational events | Startup, connections, commands received, config loaded |
| DEBUG | Periodic status, detailed operations | Heartbeat, reconnect attempts, status publishing, loop restarts |

### Rules

- **`Serial.print` is ONLY for interactive serial command responses** in `main.cpp` (direct replies to user-typed commands like `status`, `wifi`, `dir`). Everything else uses `ESP_LOGx`.
- Each `.cpp` file defines `static const char* TAG = "Name";` at file scope.
- **Include `"RuntimeLog.h"` (not `<esp_log.h>`)** in all project `.cpp` files. This header adds runtime level checking that Arduino-ESP32's default macros lack. The `USE_ESP_IDF_LOG` flag can't be used globally because it breaks third-party libraries.
- Compile-time max level is set via `-DCORE_DEBUG_LEVEL=4` (DEBUG) in `platformio.ini`.
- Runtime level is controlled by the global `runtimeLogLevel` variable (set via `log` serial command). The `RuntimeLog.h` macros check this before printing.
- Consolidate multi-line output into single `ESP_LOGx` calls with printf format strings.
- Use `.c_str()` for Arduino `String`, `.toString().c_str()` for `IPAddress`.

### TAG Names

| File | TAG |
|------|-----|
| main.cpp | `"Main"` |
| AudioController.cpp | `"Audio"` |
| HTTPServerController.cpp | `"HTTP"` |
| MQTTController.cpp | `"MQTT"` |
| StripLEDController.cpp | `"LEDStrip"` |
| MatrixLEDController.cpp | `"LEDMatrix"` |
| SettingsController.cpp | `"Settings"` |
| DemoController.cpp | `"Demo"` |
| Pcm5122.cpp | `"PCM5122"` |

## Serial Commands

Interactive serial console at 115200 baud. Commands: `play`, `stop`, `volume`, `status`, `dir`, `wifi`, `mqtt`, `log`, `reboot`. WiFi/MQTT can be configured via serial (for bootstrap when no WiFi is available yet).

The `log` command controls runtime log level: `log off`, `log error`, `log warn`, `log info` (default), `log debug`.

## Conventions

- LED effects are string-identified (e.g., `"atomic_breath"`, `"beam_attack"`) and dispatched via if/else chains in the LED controllers
- Audio files are paths on the SD card (e.g., `/sounds/roar.mp3`)
- Volume range: 0–21; brightness range: 0–255
- WiFi and MQTT are configured through the web UI or serial commands (stored in Preferences), not config.json
