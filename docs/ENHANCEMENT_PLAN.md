# Kinemachina SFX Controller Enhancement Plan

**Project**: ESP32-S3 Kinemachina SFX Controller
**Last Updated**: 2026-02-09 (audited against codebase)
**Source**: Cross-checked with actual `src/` implementation

---

## Executive Summary

**Overall Status**: 11 of 58 items complete (~19%), 2 partially done

| Status | Count |
|--------|-------|
| Completed | 11 |
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
- **Note**: Pop/click elimination is already implemented via I2C DAC mute (see #57). This item is for audible crossfade effects.

### 57. PCM5122 I2C Anti-Pop Control
- **Status**: Implemented
- **Description**: I2C control of the PCM5122 DAC to mute during playback transitions, eliminating audible pops/clicks caused by abrupt I2S start/stop. Uses the DAC's built-in soft volume ramp via register 0x03. Gracefully degrades if DAC is not found on I2C bus.
- **Files**: `Pcm5122.h/.cpp`, integrated into `AudioController`
- **I2C Pins**: GPIO8 (SDA), GPIO9 (SCL), address 0x4C

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

## Hardware / Signal Integrity

### 58. LED Matrix Sparkle / Random Pixel Noise
- **Status**: Open — level shifter added (did not fix), further investigation needed
- **Symptom**: Random pixels flash incorrect colors ("sparkle") on the 16×16 WS2812B matrix. More pronounced on USB power alone; improves with dedicated 5V power injection, but does not fully disappear.
- **Priority**: High — visible artifact that degrades every matrix effect

#### Root Cause Analysis

**Ruled out: 3.3V logic level** — 74AHCT125 level shifter added to data line, no noticeable improvement. The signal is now a clean 5V, so the sparkle is not caused by voltage threshold issues.

**Remaining suspects (re-prioritized):**

| Factor | Likelihood | Notes |
|--------|------------|-------|
| **Voltage drop across 256 pixels** | HIGH | With only one power injection point, pixels far from the input see lower VDD. WS2812B regenerates the data signal at each pixel — if VDD is sagging, the regenerated signal degrades progressively down the chain. The correlation with USB vs. dedicated rail strongly points here. |
| **Insufficient bulk capacitance** | HIGH | 256 pixels switching simultaneously draw large transient currents. Without adequate local capacitance, VDD dips momentarily cause data corruption. |
| **Ground bounce / return path** | MEDIUM | High-frequency switching currents from 256 LEDs flow through the ground path. If the ground wire is thin or long, voltage spikes on ground shift the data signal reference, causing bit errors. A separate dedicated ground wire for the data signal (star ground) may help. |
| **Data line reflections** | MEDIUM | At 800 KHz NeoPixel data rate, unterminated long wires can develop reflections. The 470Ω series resistor should help, but if the wire is long (>15cm) or unshielded, reflections can still cause glitches. |
| **RMT peripheral timing** | LOW | ESP32 uses the RMT peripheral for NeoPixel timing. The RMT channel has limited memory (48 entries by default). For 256 pixels (6144 bits), the library must refill the RMT buffer mid-transmission. If an interrupt delays the refill, the data stream glitches. WiFi interrupts on the same core could cause this. |
| **Defective panel / cold solder joints** | LOW | Some cheap 16×16 panels have intermittent connections. A single bad pixel regenerates corrupted data to everything downstream. |

#### Recommended Fixes (updated priority order)

**1. ~~Add level shifter~~ DONE — did not resolve the issue**

**2. Add power injection at matrix midpoint (next to try)**
- Add a second 5V+GND injection point at the opposite edge or center of the panel
- Reduces max voltage drop from ~0.5V to ~0.25V
- This directly addresses the strongest remaining suspect (voltage drop)
- Solder 5V and GND wires to the midpoint or far end of the matrix panel

**3. Add/verify bulk capacitance**
- 1000µF electrolytic across 5V and GND at each power injection point
- Add a 100µF cap at the far end of the matrix if only injecting at one end
- BOM specifies 1000µF but verify it's actually present and close to the panel

**4. Improve ground path**
- Run a dedicated ground wire alongside (or twisted with) the data line
- Ensure ground connections are thick gauge and short
- If using separate power supplies, verify grounds are tied solidly at one point (star ground)

**5. Shorten/shield data line**
- Keep the wire from level shifter output to first pixel as short as possible (<10cm ideal)
- Use twisted pair (data + ground) if the run exceeds 10cm

**6. Test for defective pixels**
- Run a slow single-pixel chase (one pixel lit at a time, walking through all 256)
- If sparkle consistently appears after a specific pixel index, that pixel or its upstream neighbor has a bad data-out connection
- Replace the defective pixel or re-solder its connections

**7. Investigate RMT buffer underrun (if above fixes don't resolve)**
- Try pinning the matrix LED task to a core without WiFi (Core 0 has WiFi on ESP32-S3)
- Matrix task currently runs on Core 1 — verify this is still the case
- Consider reducing NeoPixel data rate if the library supports it (400 KHz mode)

#### Testing Plan

1. ~~Add level shifter → test~~ Done, no improvement
2. Add second power injection point → test with USB power and dedicated rail
3. If still present, add bulk caps → test
4. Run single-pixel chase to check for defective pixels
5. Test at full white (all 256 pixels) to max out current draw
6. Test during WiFi activity + audio playback

#### BOM Impact

| Component | Qty | Part | Notes |
|-----------|-----|------|-------|
| U7 | 1 | 74AHCT125 | Level shifter — already installed |
| C13 | 1 | 100nF | Decoupling cap for U7 — already installed |
| C14 | 1 | 1000µF | Bulk cap at matrix power input (verify present) |
| C15 | 1 | 100–1000µF | Bulk cap at 2nd power injection point |
| Wire | — | 18-20 AWG | For 2nd power injection (5V + GND) |

#### Software Changes

None required — this is a hardware fix. No code changes needed.

---

## Feature Groupings

- **Battle Arena**: #14 (done), #29, #30, #31, #32
- **Audio**: #3, #4, #5, #24, #56 (done), #57 (done)
- **LED Effects**: #7, #8, #9, #10, #35, #58 (hardware)
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

## Recent Updates (2026-02-25)

- **LED Matrix Sparkle Investigation (#58)**: Level shifter (74AHCT125) installed — did not resolve. Re-analysis points to voltage drop across 256 pixels as primary suspect. Next step: add 2nd power injection point at matrix midpoint. Hardware-only fix.
- **PCM5122 I2C Anti-Pop (#57)**: Implemented — `Pcm5122.h/.cpp` I2C driver mutes DAC during playback transitions via register 0x03. Integrated into AudioController CMD_PLAY/CMD_STOP handlers. I2C pins GPIO8 (SDA), GPIO9 (SCL). Graceful degradation if DAC not found.

## Previous Updates (2026-02-09)

- **Bass Mono DSP (#56)**: Implemented — `BassMonoProcessor.h/.cpp` with 2nd-order Butterworth biquad, `audio_process_extern()` callback, config.json support, HTTP API, MQTT command, 19 native unit tests.
- **MQTT Support (#14)**: Fully implemented — `MQTTController.h/.cpp` with AsyncMqttClient, FreeRTOS task, exponential backoff, LWT, mDNS, web UI config. Moved from pending to completed.
- **EffectDispatch refactor**: Shared LED effect dispatch helper extracted into `EffectDispatch.h`
- **Native unit test framework**: 50 SettingsController tests running in PlatformIO native environment
- **Codebase audit**: Updated enhancement plan to reflect actual implementation status. MQTT (#14) was fully done but marked pending. LED status (#31) and effect duration (#8) are partially implemented.
- **Previous**: FreeRTOS Queues implemented for all controllers, Battle Arena API fully implemented
