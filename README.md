# Kinemachina SFX Controller

A feature-rich audio playback and LED effects controller for ESP32-S3, designed for themed displays and sound effects. Features web-based control, configurable effects, looping support, and demo mode.

## Features

- **Audio Playback**: Play audio files from SD card with volume control and pop-free transitions
- **LED Effects**: 49 LED effects total - 23 strip effects and 28 matrix effects
- **Dual LED Outputs**: Independent control of LED strips (GPIO 6) and LED matrix panels (GPIO 5)
- **16x16 Panel Support**: Dedicated matrix controller for WS2812B LED panels with 2D coordinate system
- **Battle Arena Effects**: 28 specialized matrix effects for fight behaviors (attacks, damage, blocks), dance behaviors, and panel effects (alien control panel, etc.)
- **Effects System**: Configure custom effects combining audio, LED, or both with optional looping
- **Web Interface**: Full-featured web UI for remote control
- **MQTT Support**: Event-driven control via MQTT for battle arena and automation systems
- **Demo Mode**: Automatic random playback with matching LED effects
- **Settings Persistence**: All settings saved to EEPROM/Preferences
- **Configuration File**: JSON-based configuration on SD card
- **Error Handling**: Smart loop management prevents infinite retries on errors
- **FreeRTOS Queues**: Thread-safe inter-task communication for reliable operation

## Hardware Requirements

- ESP32-S3 Development Board
- PCM5122 I2S DAC
- SD Card (FAT32 formatted)
- NeoPixel LED Strips and/or Panels (WS2812B or compatible)
  - LED Strip: Any length (1-1000+ pixels) connected to GPIO 6
  - LED Matrix Panel: 16x16 (256 pixels) connected to GPIO 5
- Audio amplifier and speakers
- Power supply (5V recommended for NeoPixels, 3A+ for panels)

### Pin Configuration

**I2S Audio (PCM5122):**
- BCK: Pin 7
- LRC: Pin 15
- DOUT: Pin 16

**I2C DAC Control (PCM5122):**
- SDA: Pin 8
- SCL: Pin 9

**SD Card (SPI):**
- CS: Pin 10
- MOSI: Pin 11
- MISO: Pin 13
- SCK: Pin 12

**NeoPixel LEDs:**
- LED Strip Data: GPIO 6 (configurable length in `main.cpp`, default 256 pixels)
- LED Matrix Data: GPIO 5 (16x16 panel, 256 pixels)
- Both controllers operate independently and can be used simultaneously

## Installation

1. Clone this repository
2. Install PlatformIO (VS Code extension or CLI)
3. Connect your hardware according to pin configuration
4. Insert SD card with audio files
5. Build and upload:
   ```bash
   pio run -t upload
   ```
6. Monitor serial output:
   ```bash
   pio device monitor
   ```

## Configuration

### SD Card Setup

1. Format SD card as FAT32
2. Create a `/sounds` directory for audio files
3. Copy your audio files to the SD card
4. Create `/config.json` on the SD card root (see `config.json.example`)

### config.json

The configuration file defines default audio/led/demo settings and custom effects. WiFi and MQTT are configured only via the web UI (stored in Preferences).

```json
{
  "audio": {
    "volume": 21
  },
  "led": {
    "brightness": 128
  },
  "demo": {
    "delay": 5000,
    "mode": "audio_led",
    "order": "random"
  },
  "effects": {
    "effect_name": {
      "audio": "/sounds/file.mp3",
      "led": "atomic_breath",
      "loop": true,
      "category": "attack"
    }
  }
}
```

**Configuration:**
- **config.json**: audio, led, demo, effects (loaded on boot from SD card).
- **WiFi and MQTT**: set via web UI only; stored in Preferences.

## Audio Formats Supported

The system supports the following audio formats:

- **MP3** (`.mp3`) - Most common format
- **WAV** (`.wav`) - Uncompressed audio
- **AAC** (`.aac`) - Advanced Audio Coding
- **M4A** (`.m4a`) - MPEG-4 Audio
- **FLAC** (`.flac`) - Free Lossless Audio Codec
- **OGG** (`.ogg`) - Ogg Vorbis

**Recommended:** MP3 files at 44.1kHz sample rate for best compatibility and performance.

## Effects System

The effects system allows you to create custom combinations of audio and LED effects that can be executed together.

### Effect Structure

Each effect can contain:
- **Audio** (optional): Path to audio file on SD card
- **LED** (optional): LED effect type name
- **Loop** (optional): Set to `true` to loop continuously
- **av_sync** (optional): Reserved for future use (sync options)
- **Category** (optional): Effect category for organization (e.g., "attack", "victory", "idle")

### LED Effect Types

**Strip Effects** (23 effects - for linear LED strips on GPIO 6):

*Themed Effects:*
- `atomic_breath` - Blue/cyan pulsing stream (Godzilla)
- `gravity_beam` - Gold/yellow lightning (Ghidorah)
- `fire_breath` - Red/orange fire effect
- `electric` - White/blue rapid flashes
- `battle_damage` - Red flashing/strobing
- `victory` - Green/gold celebration pattern
- `idle` - Subtle blue pulsing animation

*Popular General Effects:*
- `rainbow` - Smooth rainbow color cycle
- `rainbow_chase` - Rainbow colors chasing around strip
- `color_wipe` - Color wipe across strip
- `theater_chase` - Theater marquee chase effect
- `pulse` - Smooth pulsing color
- `breathing` - Breathing effect (slow pulse)
- `meteor` - Meteor/trail effect
- `twinkle` - Random twinkling stars
- `water` - Water/ripple effect
- `strobe` - Strobe/flash effect

*Edge-Lit Disc Effects:*
- `radial_out` - Radial pattern from center outward
- `radial_in` - Radial pattern from edge inward
- `spiral` - Spiral pattern rotating
- `rotating_rainbow` - Rotating rainbow around disc
- `circular_chase` - Circular chase around disc
- `radial_gradient` - Radial color gradient

**Matrix Effects** (28 effects - for 16x16 LED panels on GPIO 5):

*Battle Arena - Fight Behaviors:*
- `beam_attack` - Horizontal/vertical beam sweeping across grid
- `explosion` - Expanding circular explosion from center/point
- `impact_wave` - Circular shockwave from impact point
- `damage_flash` - Grid-wide damage flash with pattern
- `block_shield` - Defensive shield effect
- `dodge_trail` - Motion trail for dodge/evade
- `charge_up` - Energy charging effect
- `finisher_beam` - Massive beam attack (full grid)
- `gravity_beam_attack` - King Ghidorah triple gravity beam (gold lightning)
- `electric_attack_matrix` - White/blue electric lightning (5 second duration)

*Battle Arena - Dance Behaviors:*
- `victory_dance` - Celebratory dance pattern
- `taunt_pattern` - Taunting animation
- `pose_strike` - Dramatic pose with flash
- `celebration_wave` - Wave of celebration across grid
- `confetti` - Confetti/particle celebration
- `heart_eyes` - Heart eyes pattern (cute victory)
- `power_up_aura` - Aura effect for power-up
- `transition_fade` - Smooth transition between effects

*Panel Effects:*
- `game_over_chiron` - Game over text display
- `cylon_eye` - Scanning eye effect
- `perlin_inferno` - Perlin noise fire effect
- `emp_lightning` - EMP lightning pattern
- `game_of_life` - Conway's Game of Life
- `plasma_clouds` - Plasma cloud animation
- `digital_fireflies` - Digital firefly swarm
- `matrix_rain` - Matrix-style falling characters
- `dance_floor` - Dance floor pattern
- `alien_control_panel` - Alien spaceship pulsing control panel (green/cyan/teal)

### Example Effects

```json
"effects": {
  "godzilla_roar": {
    "audio": "/sounds/godzilla_roar.mp3",
    "led": "atomic_breath"
  },
  "background_music": {
    "audio": "/sounds/music.mp3",
    "loop": true
  },
  "ambient_glow": {
    "led": "idle",
    "loop": true
  },
  "fire_breath": {
    "led": "fire_breath"
  }
}
```

### Loop Behavior

- Effects with `"loop": true` will restart automatically when finished
- Audio loops restart when playback completes
- LED loops restart when effect duration ends
- **Error Protection**: If audio file is missing or playback fails, looping stops automatically to prevent infinite retries
- Only one effect can loop at a time (starting a new loop stops the previous one)

## Web Interface

Access the web interface at `http://<ESP32_IP_ADDRESS>` after WiFi connection.

### Features

- **Audio Control**: Play files, stop playback, adjust volume
- **Effects List**: View and execute all configured effects
- **Loop Status**: See current looping effect and stop button
- **LED Effects**: Direct control of LED effects, grouped by Strip Effects (GPIO 6) and Matrix Effects (GPIO 5)
- **Demo Mode**: Start/stop/pause demo mode
- **Settings**: Configure WiFi, volume, brightness, demo delay
- **Directory Browser**: Browse SD card contents

### Effects Display

- Effects with `loop: true` show a "LOOP" badge
- Current looping effect is displayed at the top
- "Stop Loop" button appears when an effect is looping
- Effects list auto-refreshes every 5 seconds

## API Endpoints

### HTTP REST API

#### Audio

- `POST /play?file=/path/to/file.mp3` - Play audio file
- `POST /stop` - Stop playback
- `POST /volume?volume=21` - Set volume (0-21)
- `GET /status` - Get system status
- `GET /dir?path=/` - List directory contents

#### Effects

- `GET /effects/list` - Get list of all configured effects
- `POST /effects/execute?name=effect_name` - Execute an effect
- `POST /effects/stop-loop` - Stop current looping effect

#### LED

- `POST /led/effect?effect=atomic_breath` - Start LED effect
- `POST /led/brightness?brightness=128` - Set brightness (0-255)
- `POST /led/stop` - Stop LED effect

#### Demo Mode

- `POST /demo/start?delay=5000` - Start demo mode
- `POST /demo/stop` - Stop demo mode
- `POST /demo/pause` - Pause demo mode
- `POST /demo/resume` - Resume demo mode
- `GET /demo/status` - Get demo mode status

#### Settings

- `POST /settings/clear` - Clear all settings (factory reset)

#### Battle Arena API

- `POST /api/battle/trigger` - Trigger effect by category or name
- `GET /api/battle/effects` - List all effects with categories
- `GET /api/battle/effects?category=<category>` - List effects by category
- `GET /api/battle/status` - Current system status
- `GET /api/battle/health` - Health check
- `POST /api/battle/stop` - Stop all effects

### MQTT API

The controller supports MQTT for event-driven control, ideal for battle arena systems and home automation. See [MQTT_API.md](MQTT_API.md) for complete documentation.

**Features:**
- Event-driven pub/sub messaging for low-latency control
- Automatic status publishing on state changes
- Health checks every 30 seconds
- Last Will and Testament (LWT) for online/offline status
- mDNS broker resolution (supports `.local` hostnames)
- FreeRTOS queue-based command processing for thread-safe operation
- Configurable QoS levels for commands and status messages
- Automatic reconnection with exponential backoff

**Quick Start:**
- Configure MQTT in the web UI (Settings → MQTT) with broker and topic settings
- Command topic: `{base_topic}/{device_id}/command` (single topic, JSON `"command"` field selects action)
- Status topics: `{base_topic}/{device_id}/status` (and sub-topics)
- Response topic: `{base_topic}/{device_id}/response`
- All messages use JSON format

**Example Commands:**
```bash
# Trigger effect by category
mosquitto_pub -h mqtt.broker.local -t "kinemachina/sfx/kinemachina_001/command" \
  -m '{"command":"trigger","category":"attack","request_id":"test_001"}'

# Trigger specific effect with optional parameters
mosquitto_pub -h mqtt.broker.local -t "kinemachina/sfx/kinemachina_001/command" \
  -m '{"command":"trigger","effect":"godzilla_roar","audio_volume":21,"led_brightness":128,"request_id":"test_002"}'

# Stop all effects
mosquitto_pub -h mqtt.broker.local -t "kinemachina/sfx/kinemachina_001/command" \
  -m '{"command":"stop","request_id":"test_003"}'

# Set volume
mosquitto_pub -h mqtt.broker.local -t "kinemachina/sfx/kinemachina_001/command" \
  -m '{"command":"volume","volume":18,"request_id":"test_004"}'
```

**Subscribe to Status:**
```bash
# Subscribe to all status updates
mosquitto_sub -h mqtt.broker.local -t "kinemachina/sfx/kinemachina_001/status/#"

# Subscribe to responses
mosquitto_sub -h mqtt.broker.local -t "kinemachina/sfx/kinemachina_001/response"
```

## Demo Mode

Demo mode automatically plays random audio files with matching LED effects. Perfect for store displays or unattended operation.

**Features:**
- Randomly selects audio files from `/sounds` directory
- Automatically matches LED effects to audio filenames
- Configurable delay between tracks
- Pause/resume support
- Shows file count in status

**LED Effect Matching:**
- Files with "atomic" or "godzilla" → Atomic Breath
- Files with "gravity" or "ghidorah" → Gravity Beam
- Files with "fire" or "flame" → Fire Breath
- Files with "electric" or "lightning" → Electric (strip effect)
- Files with "damage" or "hurt" → Battle Damage
- Files with "victory" or "win" → Victory
- Others → Random effect

## Serial Commands

When connected via USB serial (115200 baud):

- `play:/path/to/file.mp3` - Play audio file
- `stop` - Stop playback
- `volume:21` - Set volume (0-21)
- `status` - Show system status
- `dir` or `dir:/path` - List directory contents

## Settings Persistence

All settings are automatically saved to ESP32 Preferences (EEPROM):

- Volume level
- LED brightness
- Demo delay
- WiFi credentials
- MQTT settings

Audio, LED, and demo defaults are loaded from `config.json` on boot. WiFi and MQTT are set via the web UI and stored in Preferences.

## Troubleshooting

### Audio Not Playing

- Check SD card is properly formatted (FAT32)
- Verify audio file exists and path is correct
- Check file format is supported (MP3, WAV, AAC, M4A, FLAC, OGG)
- Ensure SD card is inserted before power-on
- Check serial output for error messages

### WiFi Not Connecting

- Verify WiFi credentials are set in the web UI (stored in Preferences)
- Check signal strength (RSSI shown in serial output)
- Ensure 2.4GHz network (ESP32 doesn't support 5GHz)
- Check for special characters in SSID/password

### Effects Not Looping

- Verify `"loop": true` is set in config.json
- Check serial output for error messages
- Ensure audio file exists (loops stop on file errors)
- Check that only one effect is looping at a time

### LED Effects Not Working

- Verify NeoPixel pins in `main.cpp` (GPIO 6 for strips, GPIO 5 for matrix)
- Check power supply (NeoPixels need adequate current)
- Ensure data lines are connected correctly to the appropriate GPIO pins
- Check brightness setting (may be too low to see)
- Strip effects work on GPIO 6 (linear strips)
- Matrix effects work on GPIO 5 (16x16 panels)
- Both controllers operate independently

### SD Card Issues

- Format as FAT32 (not exFAT or NTFS)
- Use smaller SD cards for better compatibility (≤32GB)
- Ensure proper power supply (SD cards need stable power)
- Check SPI connections and CS pin

## Dual LED Controller Architecture

The system uses two independent LED controllers for maximum flexibility:

### StripLEDController (GPIO 6)
- Controls linear LED strips of any length (1-1000+ pixels)
- 23 strip-specific effects optimized for linear displays
- Configurable pixel count in `main.cpp` via `LED_COUNT`
- Perfect for edge-lit discs, linear displays, and strip-based installations

### MatrixLEDController (GPIO 5)
- Controls 16x16 (256 pixel) LED matrix panels
- 25 matrix-specific effects designed for 2D grid displays
- Built-in 2D coordinate system with serpentine wiring support
- Drawing primitives for lines, circles, rectangles
- Ideal for battle arena displays, grid art, and panel installations

### Configuration

Both controllers are initialized in `main.cpp`:
```cpp
StripLEDController ledStripController(LED_STRIP_PIN, LED_COUNT);
MatrixLEDController ledMatrixController(LED_MATRIX_PIN, MATRIX_WIDTH, MATRIX_HEIGHT);
```

The controllers operate independently - you can run strip effects and matrix effects simultaneously, or use just one depending on your hardware setup.

## Libraries Used

- **ESP32-audioI2S** (v2.3.0) - Audio playback
- **ESPAsyncWebServer** (v3.0.0) - Web server
- **AsyncTCP** (v1.1.1) - Async TCP support
- **AsyncMqttClient** (v0.9.0) - MQTT client for event-driven control
- **Adafruit NeoPixel** (v1.12.9) - LED control
- **ArduinoJson** (v7.0.0) - JSON parsing
- **FreeRTOS** - Real-time task management with queues and tasks

## Development

### Project Structure

```
kinemachina-sfx-controller/
├── src/
│   ├── main.cpp              # Main application
│   ├── AudioController.*     # Audio playback management
│   ├── StripLEDController.*  # LED strip effects (GPIO 6)
│   ├── MatrixLEDController.* # LED matrix effects (GPIO 5)
│   ├── HTTPServerController.* # Web server and HTTP API
│   ├── MQTTController.*      # MQTT client and event-driven control
│   ├── Pcm5122.*             # PCM5122 I2C DAC control (anti-pop mute)
│   ├── SettingsController.*  # Settings persistence
│   └── DemoController.*      # Demo mode
├── config.json.example       # Configuration template
├── MQTT_API.md               # MQTT API documentation
├── platformio.ini            # PlatformIO configuration
└── README.md                 # This file
```

### Building

```bash
# Build project
pio run

# Upload to ESP32
pio run -t upload

# Monitor serial output
pio device monitor
```

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Contributing

[Add contribution guidelines here]

## Acknowledgments

- ESP32-audioI2S library for audio playback support
- Adafruit for NeoPixel library
- All creative builders and enthusiasts!
