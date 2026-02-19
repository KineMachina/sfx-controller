# Kinemachina SFX Controller Enhancement Plan

**Project**: ESP32-S3 Kinemachina SFX Controller
**Last Updated**: 2026-02-09 (audited against codebase)
**Source**: Cross-checked with actual `src/` implementation

---

## Executive Summary

**Overall Status**: 10 of 56 items complete (~18%), 2 partially done

| Status | Count |
|--------|-------|
| Completed | 10 |
| Partially Implemented | 2 |
| In Progress | 1 (FreeRTOS Optimization) |
| Pending | 43 |

**Codebase Stats**: 10,095 LOC across 16 source files, 7 controllers + 1 utility (`EffectDispatch.h`), 23 strip effects, 31 matrix effects

**Next up**: Quick wins — serial commands (#29, #30), file navigation (#32), then UX improvements

---

## Completed Enhancements

### 1. Settings Persistence (NVS)
- Status: Implemented
- `SettingsController` Preferences read/write
- Settings are automatically saved and restored on boot

### 2. Configuration File on SD Card
- Status: Implemented
- `loadFromConfigFile()`, ArduinoJson parsing of `/config.json`
- Supports audio/LED settings, demo delay, and custom effects
- WiFi and MQTT are set via web UI only (stored in Preferences)

### 6. Audio Visualization Sync
- Status: Implemented
- `av_sync` flag in Effect struct, audio-reactive LED modulation
- Only enabled for effects tagged with `"av_sync": true` in config.json
- Audio level sampling updates LED effect intensity in real-time

### 7. FreeRTOS Queues Implementation
- Status: Implemented
- Command/status queues in Audio, LED, MQTT, and Demo controllers
- Thread-safe command processing with proper blocking behavior

### 8/16a. Battle Arena REST API
- Status: Implemented
- 5 endpoints in `HTTPServerController` (`/api/battle/*`)
- Effect categorization, status reporting, health check
- Full REST API with JSON request/response handling

### 14. MQTT Support
- Status: Implemented
- `MQTTController.h/.cpp` (1,047 lines) — full production-ready implementation
- AsyncMqttClient library with dedicated FreeRTOS task
- Topics: `{baseTopic}/{deviceId}/command/{trigger|stop|volume|brightness}`, `status/{audio|led|health}`, `response`
- LWT at `status/online` for offline detection
- Reconnect with exponential backoff (1s–30s)
- mDNS hostname resolution for broker discovery
- Web UI settings page for broker, port, credentials, device ID, base topic
- QoS levels for reliable command delivery
- HTTP API remains for web interface; MQTT is primary for battle arena control

### 28. Track Volume Level
- Status: Implemented (part of settings persistence)

### 34. Show File Count in Demo Status
- Status: Implemented (part of demo status endpoint)

### EffectDispatch Refactor
- `EffectDispatch.h` — shared LED effect dispatch helper
- Eliminates duplicated dispatch logic between controllers

---

## Partially Implemented

### 31. Show Current LED Effect in Status
- **Done**: HTTP `/status` endpoint returns `ledEffect` field; web UI highlights active effect
- **Missing**: Serial `status` command doesn't include LED effect name

### 8. Effect Duration Control
- **Done**: `effectStartTime` tracked in LED controllers; some effects have implicit durations
- **Missing**: No HTTP endpoint or API parameter for custom duration

---

## In Progress

### 19. FreeRTOS Optimization
- **Done**: Command/status queues for all task-based controllers
- **Remaining**: Event Groups, Software Timers, Task Notifications, Task Suspension/Resume, Watchdog, event-driven main loop
- See FREERTOS_OPTIMIZATION.md for detailed implementation guide

---

## High Priority Quick Wins (not yet implemented)

These are small (2–5 hours each), high-value, and have no dependencies:

| # | Feature | Effort | Notes |
|---|---------|--------|-------|
| 29 | Serial Commands for LED Effects | 2–4h | Add `led:<effect_name>` to serial parser in main.cpp |
| 30 | Serial Commands for Demo Mode | 2–4h | Add `demo:start`, `demo:stop`, `demo:pause`, `demo:resume` |
| 31 | Show LED Effect in Serial Status | 1h | Add LED effect name to serial `status` output (HTTP already done) |
| 32 | Next/Previous File Navigation | 3–5h | Add `/next`, `/prev` endpoints + serial commands |
| 33 | Repeat/Single Track Playback | 3–4h | Playback mode state in AudioController + HTTP endpoints |
| 35 | LED Effect Preview | 2–3h | Preview mode with short duration override |

---

## Audio Enhancements

### 3. Playlists/Sequences
- **Description**: Create playlists of audio files that play in order or shuffle
- **Benefits**: Better organization, sequential playback, shuffle mode
- **Implementation**:
  - HTTP endpoints to create/manage playlists
  - Playlist storage (JSON on SD or Preferences)
  - Playlist playback mode

### 4. Audio Fade In/Out
- **Description**: Smooth transitions between tracks
- **Benefits**: Professional sound transitions, fade on stop/pause
- **Implementation**:
  - Gradual volume changes during transitions
  - Configurable fade time (100ms-2000ms)

### 5. Audio Metadata Display
- **Description**: Show track name, duration, file size in web UI
- **Benefits**: Parse ID3 tags, display in web interface, show in status endpoint

---

## LED Effect Enhancements

### 7. More Themed Effects
- **Description**: Additional effects for other characters
- **Effects to Add**:
  - Mothra Light Beam (green/pink)
  - Rodan Fire Storm (red/orange waves)
  - Mechagodzilla Plasma (purple/blue)
  - King Kong Roar (amber pulses)

### 9. Multi-Zone LED Control
- **Description**: Control multiple LED strips independently
- **Benefits**: Different effects per zone, coordinated sequences
- **Note**: Hardware already supports 2 zones (strip + matrix)

### 10. Effect Intensity/Speed Control
- **Description**: Adjust effect speed and intensity
- **Implementation**: Speed multiplier, intensity/brightness scaling, color palette selection

---

## Demo Mode Enhancements

### 11. Scheduled Demos
- **Description**: Time-based scheduling (e.g., 9 AM - 5 PM)
- **Implementation**: NTP time sync, schedule storage in Preferences, HTTP endpoints

### 12. Demo Playlists
- **Description**: Predefined demo sequences with weighted random selection
- **Depends on**: #3 (Playlists)

### 13. Demo Statistics
- **Description**: Track play counts and history
- **Implementation**: Play count tracking, statistics storage, HTTP endpoint

---

## Network & Remote Control

### 15. WebSocket for Real-Time Updates
- **Description**: Live status updates without polling
- **Note**: ESPAsyncWebServer supports WebSocket, not yet used
- **Implementation**: WebSocket server, push updates on state changes, replace polling in web UI

### 16. REST API Improvements
- **Description**: Full JSON API documentation, batch operations, webhook callbacks
- **Note**: Less critical now that MQTT is implemented

---

## User Interface

### 17. Better Web UI
- **Description**: Enhanced web interface with file browser, drag-and-drop, real-time dashboard, mobile-responsive design

### 18. Audio Waveform Visualization
- **Description**: Canvas-based waveform display with progress bar, time remaining, scrubbing

---

## Display System (ST7789 2" IPS — 13 items, all pending)

Phased approach, none started:

| Phase | Items | Effort |
|-------|-------|--------|
| 1: Basic Display | #35 Status Display, #46 Brightness Control | 3–4 days |
| 2: Visualization | #36 Audio Viz, #44 VU Meters | 5–7 days |
| 3: Interactive | #37 Menu System, #38 Effect Preview | 7–10 days |
| 4: Info & Polish | #39 System Info, #40 Demo Viz, #42 Config, #43 File Browser | 4–6 days |
| 5: Customization | #41 Themed Graphics, #45 Multi-Layout, #47 Touch | 8–12 days |

**Risks**: SPI bus contention with SD card, memory for graphics buffers, CPU load vs FreeRTOS tasks

### Display Items
- **35** Status Display Screen — real-time status on 2" ST7789 IPS
- **36** Audio Visualization Display — spectrum analyzer, waveform, VU meters
- **37** Interactive Menu System — local control via touch or buttons
- **38** Effect Preview & Selection — visual preview of LED effects on screen
- **39** System Information Dashboard — WiFi, memory, SD card, uptime
- **40** Demo Mode Visualization — current mode, file name, statistics
- **41** Themed Graphics — splash screen, themed icons, animations
- **42** Configuration Display — current settings readout
- **43** File Browser Display — browse SD card audio files on screen
- **44** Real-Time Audio Meters — VU meters with peak hold
- **45** Multi-Screen Layouts — switchable display modes
- **46** Display Brightness Control — adjustable with auto/sleep modes
- **47** Touch Screen Support — optional touch input

---

## System & Reliability

### 20. OTA (Over-The-Air) Updates
- **Description**: Update firmware via web interface, rollback capability

### 21. Better Error Handling
- **Description**: Retry logic, graceful degradation, error logging to SD card

### 22. System Health Monitoring
- **Description**: Temperature, memory, SD card health, WiFi signal strength

### 23. Watchdog & Recovery
- **Description**: Automatic restart on crashes, safe mode, diagnostic mode

---

## Advanced Features

### 56. Bass Mono DSP for Tactile Transducer
- **Status**: Implemented
- **Description**: Below a configurable crossover frequency (default 80 Hz), L+R stereo is summed to mono for coherent low-frequency energy on tactile transducers/subwoofers. Stereo imaging preserved above crossover. Uses 2nd-order Butterworth biquad LPF in `BassMonoProcessor.h/.cpp`, wired via `audio_process_extern()` callback. Configurable via config.json (`audio.bass_mono`), HTTP API (`GET/POST /audio/bass-mono`), and MQTT (`command/bass_mono`). Includes 19 native unit tests.

### 24. Audio Equalizer (DSP)
- **Description**: Bass/treble adjustment, preset EQ profiles

### 25. Multi-Language Support
- **Description**: Localized web UI and serial commands

### 26. Access Control / Auth
- **Description**: Password protection, API key authentication, user roles

### 27. Backup & Restore
- **Description**: Export/import configuration, SD card backup utility

---

## Recommended Sprint Order

### Sprint 1: Quick Wins (1 week)
Serial commands (#29, #30), LED status in serial (#31), file navigation (#32), repeat mode (#33), effect preview (#35)

### Sprint 2: UX Improvements (2–3 weeks)
Audio fade (#4), more themed effects (#7), effect intensity/speed (#10), WebSocket (#15)

### Sprint 3: FreeRTOS Completion (1 week)
Finish #19: Event Groups, Software Timers, Task Notifications

### Sprint 4+: Display System, Advanced Features
As needed per phases above

---

## Dependencies

- #31 (LED effect in serial status) → #35 (LED effect preview)
- #3 (Playlists) → #12 (Demo Playlists)
- #19 (FreeRTOS) → improves all features' performance
- Display Phase 1 → Phase 2 → ... → Phase 5

## Feature Groupings

- **Battle Arena**: #14 (done), #29, #30, #31, #32
- **Audio**: #3, #4, #5, #24, #56 (done)
- **LED Effects**: #7, #8, #9, #10, #35
- **Demo Mode**: #11, #12, #13
- **Network**: #15, #16
- **UI**: #17, #18
- **Display**: #35–#47
- **System**: #19, #20, #21, #22, #23
- **Advanced**: #24, #25, #26, #27

---

## Notes

- All enhancements should maintain backward compatibility where possible
- Consider memory constraints when adding features
- Test thoroughly on actual hardware
- Document new features in code and web UI
- **Display Notes**: ST7789 2" screen (240x320) requires SPI communication — shared SPI bus with SD card (different CS pins), consider refresh rate vs performance, memory for graphics buffers, power consumption

## Recent Updates (2026-02-09)

- **Bass Mono DSP (#56)**: Implemented — `BassMonoProcessor.h/.cpp` with 2nd-order Butterworth biquad, `audio_process_extern()` callback, config.json support, HTTP API, MQTT command, 19 native unit tests.
- **MQTT Support (#14)**: Fully implemented — `MQTTController.h/.cpp` with AsyncMqttClient, FreeRTOS task, exponential backoff, LWT, mDNS, web UI config. Moved from pending to completed.
- **EffectDispatch refactor**: Shared LED effect dispatch helper extracted into `EffectDispatch.h`
- **Native unit test framework**: 50 SettingsController tests running in PlatformIO native environment
- **Codebase audit**: Updated enhancement plan to reflect actual implementation status. MQTT (#14) was fully done but marked pending. LED status (#31) and effect duration (#8) are partially implemented.
- **Previous**: FreeRTOS Queues implemented for all controllers, Battle Arena API fully implemented
