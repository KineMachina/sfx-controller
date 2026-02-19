# Contributing

Thanks for your interest in contributing to the Kinemachina SFX Controller. This guide assumes familiarity with PlatformIO, ESP32, and FreeRTOS.

## Getting Started

**Prerequisites:**

- [PlatformIO CLI](https://docs.platformio.org/en/latest/core/installation.html) or the VS Code extension
- Optional: ESP32-S3 dev board for hardware testing

Clone the repo and run `pio run` to verify the toolchain works.

## Build & Test

```bash
pio run                                    # Build for ESP32-S3
pio test -e native -v                      # Run native unit tests (no hardware needed)
pio run -t upload && pio device monitor    # Flash + serial monitor (115200 baud)
```

All PRs must pass `pio test -e native` before merge.

## Project Structure

All source lives in `src/`. Each controller is a `.h`/`.cpp` pair:

| Controller | Responsibility |
|---|---|
| **AudioController** | I2S DAC playback, SD card init, SD mutex owner |
| **HTTPServerController** | WiFi management, REST API, web UI |
| **StripLEDController** | 23 effects for linear LED strips |
| **MatrixLEDController** | 31 effects for 16x16 LED matrix |
| **MQTTController** | MQTT pub/sub with JSON command dispatch |
| **SettingsController** | config.json parsing, Preferences read/write |
| **DemoController** | Autonomous playback modes |

See [`CLAUDE.md`](CLAUDE.md) for detailed architecture (init order, FreeRTOS task/queue pattern, pin map). See [`MQTT_API.md`](MQTT_API.md) for the full MQTT protocol reference.

## Making Changes

1. Fork the repo and create a branch from `main`.
2. One feature or fix per PR.
3. Write tests for new logic. Test files go in `test/test_<name>/test_<name>.cpp` (each in its own subdirectory).
4. Follow existing code patterns -- look at a similar controller for reference.

## Code Style

- **PascalCase** for classes, **camelCase** for methods and variables, **UPPER_SNAKE_CASE** for constants and pin defines.
- Effects are string-identified (e.g., `"atomic_breath"`, `"beam_attack"`) and dispatched via if/else chains in the LED controllers.
- No dynamic allocation in hot paths (FreeRTOS tasks). Use queues and static buffers.

## Pull Request Process

1. Give your PR a descriptive title and fill in the PR template.
2. Ensure `pio test -e native` passes locally.
3. CI must be green (native tests + build check).
4. PRs are reviewed by maintainers before merge.

## Issues

Issues tagged **`good first issue`** are a good starting point for new contributors. Check the issue tracker if you're looking for something to work on.
