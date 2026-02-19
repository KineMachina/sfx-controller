# MQTT API Documentation

Complete reference for the Kinemachina SFX Controller MQTT interface.

## Table of Contents

- [Overview](#overview)
- [Configuration](#configuration)
- [Topic Structure](#topic-structure)
- [Command Topics](#command-topics)
- [Status Topics](#status-topics)
- [Response Topic](#response-topic)
- [Message Formats](#message-formats)
- [QoS Levels](#qos-levels)
- [Connection Behavior](#connection-behavior)
- [Usage Examples](#usage-examples)
- [Error Handling](#error-handling)
- [Best Practices](#best-practices)
- [Troubleshooting](#troubleshooting)

## Overview

The Kinemachina SFX Controller supports MQTT (Message Queuing Telemetry Transport) for event-driven control, ideal for:

- **Battle Arena Systems:** Low-latency event-driven control for automated battle arena systems
- **Home Automation:** Integration with Home Assistant, Node-RED, OpenHAB, and other automation platforms
- **Multi-Controller Systems:** Control multiple controllers via topic-based addressing
- **Real-Time Monitoring:** Automatic status publishing for dashboards and monitoring systems

### Key Features

- **Event-Driven Control:** Pub/sub messaging model for low-latency command processing
- **Automatic Status Publishing:** State changes trigger automatic status updates
- **Health Monitoring:** Periodic health checks every 30 seconds
- **Last Will and Testament:** Automatic offline detection via LWT
- **mDNS Support:** Automatic broker discovery using `.local` hostnames
- **Thread-Safe Processing:** FreeRTOS queue-based command handling
- **Request/Response Correlation:** Track commands via `request_id` field
- **Configurable QoS:** Different quality-of-service levels for commands vs. status

## Configuration

MQTT is configured via the web UI (Settings → MQTT). Settings are stored in ESP32 Preferences (NVS). All fields are optional except broker when MQTT is enabled.

### Configuration Parameters

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `enabled` | boolean | `false` | Enable/disable MQTT functionality |
| `broker` | string | - | MQTT broker hostname or IP address (supports `.local` mDNS) |
| `port` | integer | `1883` | MQTT broker port (typically 1883 for unencrypted, 8883 for TLS) |
| `username` | string | `""` | Optional MQTT username (leave empty for no authentication) |
| `password` | string | `""` | Optional MQTT password (leave empty for no authentication) |
| `device_id` | string | `"kinemachina_001"` | Unique device identifier (used in topic paths) |
| `base_topic` | string | `"kinemachina/sfx"` | Base topic prefix for all topics |
| `qos_commands` | integer | `1` | QoS level for command topics (0, 1, or 2) |
| `qos_status` | integer | `0` | QoS level for status topics (0, 1, or 2) |
| `keepalive` | integer | `60` | Keepalive interval in seconds |
| `clean_session` | boolean | `false` | Clean session flag (false = persistent session) |

### Broker Hostname Resolution

The controller supports multiple broker hostname formats:

- **IP Address:** `192.168.1.100` - Direct IP address (no resolution needed)
- **DNS Hostname:** `mqtt.example.com` - Standard DNS resolution
- **mDNS Hostname:** `mqtt.broker.local` - mDNS/Bonjour resolution (recommended for local networks)

The controller automatically resolves hostnames using:
1. mDNS query (for `.local` domains)
2. Standard DNS resolution (fallback)

**Note:** mDNS resolution may take 1-2 seconds on first connection. The resolved IP is cached for subsequent reconnections.

## Topic Structure

All topics follow a hierarchical structure based on the `base_topic` and `device_id` configuration:

```
{base_topic}/{device_id}/{category}/{subcategory}
```

### Default Topic Structure

With default configuration (`base_topic: "kinemachina/sfx"`, `device_id: "kinemachina_001"`):

```
kinemachina/sfx/kinemachina_001/
├── command            (subscribe, single topic with JSON dispatch)
├── status/
│   ├── (main)         (publish, retained)
│   ├── audio          (publish)
│   ├── led            (publish)
│   ├── health         (publish)
│   └── online         (publish, LWT, retained)
└── response           (publish)
```

### Topic Naming Conventions

- **Commands:** `{base_topic}/{device_id}/command` (single topic, `"command"` field in JSON selects the action)
- **Status:** `{base_topic}/{device_id}/status[/{subsystem}]`
- **Response:** `{base_topic}/{device_id}/response`
- **LWT:** `{base_topic}/{device_id}/status/online`

## Command Topic

**Topic:** `{base_topic}/{device_id}/command`

The controller subscribes to a single command topic. All commands are JSON objects with a `"command"` field that selects the action. Command matching is **case-insensitive**. All commands support an optional `request_id` field for request/response correlation.

### Command Reference

| Command | Required Fields | Example Payload |
|---------|----------------|-----------------|
| `trigger` | `effect` or `category` | `{"command":"trigger","effect":"godzilla_roar"}` |
| `stop` | — | `{"command":"stop"}` |
| `volume` | `volume` (0-21) | `{"command":"volume","volume":18}` |
| `brightness` | `brightness` (0-255) | `{"command":"brightness","brightness":200}` |
| `bass_mono` | `enabled` and/or `crossover_hz` | `{"command":"bass_mono","enabled":true,"crossover_hz":80}` |

### Command: trigger

Execute an effect by category or name. Supports optional parameter overrides.

#### Request Format

```json
{
  "command": "trigger",
  "category": "attack",
  "effect": "godzilla_roar",
  "priority": "high",
  "duration": 3000,
  "audio_volume": 21,
  "led_brightness": 128,
  "request_id": "req_12345"
}
```

#### Parameters

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `command` | string | Yes | Must be `"trigger"` |
| `category` | string | Conditional | Effect category (attack, growl, damage, death, victory, idle). Required if `effect` not specified. |
| `effect` | string | Conditional | Effect name: either a configured effect from config.json, or any LED effect name (same as HTTP API). Required if `category` not specified. |
| `priority` | string | No | Priority level (currently unused, reserved for future use) |
| `duration` | integer | No | Duration in milliseconds (currently unused, reserved for future use) |
| `audio_volume` | integer | No | Override audio volume (0-21). Applied before effect execution. |
| `led_brightness` | integer | No | Override LED brightness (0-255). Applied before effect execution. |
| `request_id` | string | No | Optional request identifier for response correlation |

#### Behavior

- If `category` is specified: Randomly selects an effect from that category
- If `effect` is specified: Resolved by name. First looks up in config.json; if not found, resolves as an LED-only effect using the same names as the HTTP LED API (e.g. `transporter_tos`, `beam_attack`, `atomic_breath`). So all HTTP LED effects are available via MQTT.
- If both are specified: `effect` takes precedence
- Parameter overrides (`audio_volume`, `led_brightness`) are applied before effect execution
- Settings are persisted to EEPROM if overrides are used

#### Response

See [Response Topic](#response-topic) section for response format.

#### Example

```bash
# Trigger random attack effect
mosquitto_pub -h mqtt.broker.local -t "kinemachina/sfx/kinemachina_001/command" \
  -m '{"command":"trigger","category":"attack","request_id":"attack_001"}'

# Trigger specific effect with volume override
mosquitto_pub -h mqtt.broker.local -t "kinemachina/sfx/kinemachina_001/command" \
  -m '{"command":"trigger","effect":"godzilla_roar","audio_volume":18,"request_id":"roar_001"}'

# Trigger LED-only effect (same effect names as HTTP /api/led/effect)
mosquitto_pub -h mqtt.broker.local -t "kinemachina/sfx/kinemachina_001/command" \
  -m '{"command":"trigger","effect":"transporter_tos","request_id":"led_001"}'
```

### Command: stop

Stop all audio playback and LED effects.

#### Request Format

```json
{
  "command": "stop",
  "request_id": "req_12345"
}
```

#### Parameters

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `command` | string | Yes | Must be `"stop"` |
| `request_id` | string | No | Optional request identifier for response correlation |

#### Behavior

- Stops all audio playback
- Stops all LED effects (both strip and matrix)
- Publishes updated status

#### Response

See [Response Topic](#response-topic) section for response format.

#### Example

```bash
mosquitto_pub -h mqtt.broker.local -t "kinemachina/sfx/kinemachina_001/command" \
  -m '{"command":"stop","request_id":"stop_001"}'
```

### Command: volume

Set audio volume level.

#### Request Format

```json
{
  "command": "volume",
  "volume": 21,
  "request_id": "req_12345"
}
```

#### Parameters

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `command` | string | Yes | Must be `"volume"` |
| `volume` | integer | Yes | Volume level (0-21, where 21 is maximum) |
| `request_id` | string | No | Optional request identifier for response correlation |

#### Behavior

- Sets audio volume immediately
- Persists volume setting to EEPROM
- Publishes updated status

#### Response

See [Response Topic](#response-topic) section for response format.

#### Example

```bash
mosquitto_pub -h mqtt.broker.local -t "kinemachina/sfx/kinemachina_001/command" \
  -m '{"command":"volume","volume":18,"request_id":"vol_001"}'
```

### Command: brightness

Set LED brightness level.

#### Request Format

```json
{
  "command": "brightness",
  "brightness": 128,
  "request_id": "req_12345"
}
```

#### Parameters

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `command` | string | Yes | Must be `"brightness"` |
| `brightness` | integer | Yes | Brightness level (0-255, where 255 is maximum) |
| `request_id` | string | No | Optional request identifier for response correlation |

#### Behavior

- Sets brightness for both strip and matrix LED controllers
- Persists brightness setting to EEPROM
- Publishes updated status

#### Response

See [Response Topic](#response-topic) section for response format.

#### Example

```bash
mosquitto_pub -h mqtt.broker.local -t "kinemachina/sfx/kinemachina_001/command" \
  -m '{"command":"brightness","brightness":200,"request_id":"bright_001"}'
```

### Command: bass_mono

Configure bass mono processing settings.

#### Request Format

```json
{
  "command": "bass_mono",
  "enabled": true,
  "crossover_hz": 80,
  "request_id": "req_12345"
}
```

#### Parameters

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `command` | string | Yes | Must be `"bass_mono"` (or `"bassMono"`) |
| `enabled` | boolean | No | Enable/disable bass mono processing |
| `crossover_hz` | integer | No | Crossover frequency in Hz (20-500) |
| `request_id` | string | No | Optional request identifier for response correlation |

#### Behavior

- Sets bass mono enabled state and/or crossover frequency
- Persists settings to EEPROM
- Publishes updated status

#### Response

See [Response Topic](#response-topic) section for response format.

#### Example

```bash
mosquitto_pub -h mqtt.broker.local -t "kinemachina/sfx/kinemachina_001/command" \
  -m '{"command":"bass_mono","enabled":true,"crossover_hz":80,"request_id":"bass_001"}'
```

## Status Topics

The controller automatically publishes status updates to multiple topics. Status is published:

- **On State Changes:** Audio playback, LED effects, volume, brightness changes
- **Periodically:** Health checks every 30 seconds
- **On Connection:** Initial status published when MQTT connects
- **On Demand:** Can be forced via status change detection

### Status: Main Status

**Topic:** `{base_topic}/{device_id}/status`

Complete system status (retained message).

#### Message Format

```json
{
  "status": "success",
  "audio": {
    "playing": true,
    "volume": 21,
    "current_file": "/sounds/godzilla_roar.mp3"
  },
  "led": {
    "effect_running": true,
    "current_effect": 0,
    "brightness": 128,
    "audio_reactive": false
  },
  "wifi": {
    "connected": true,
    "ip": "192.168.1.100"
  },
  "timestamp": 1234567890,
  "free_heap": 245678
}
```

#### Fields

| Field | Type | Description |
|-------|------|-------------|
| `status` | string | Status indicator (`"success"`) |
| `audio.playing` | boolean | Whether audio is currently playing |
| `audio.volume` | integer | Current volume level (0-21) |
| `audio.current_file` | string | Currently playing audio file path (if playing) |
| `led.effect_running` | boolean | Whether an LED effect is currently running |
| `led.current_effect` | integer | Current LED effect ID (-1 if none) |
| `led.brightness` | integer | Current brightness level (0-255) |
| `wifi.connected` | boolean | WiFi connection status |
| `wifi.ip` | string | WiFi IP address |
| `timestamp` | integer | Timestamp in milliseconds (from `millis()`) |
| `free_heap` | integer | Free heap memory in bytes |

**Note:** This topic uses **retained messages**, so subscribers receive the last known state immediately upon subscription.

### Status: Audio

**Topic:** `{base_topic}/{device_id}/status/audio`

Audio subsystem status (published on audio state changes).

#### Message Format

```json
{
  "playing": true,
  "volume": 21,
  "timestamp": 1234567890
}
```

#### Fields

| Field | Type | Description |
|-------|------|-------------|
| `playing` | boolean | Whether audio is currently playing |
| `volume` | integer | Current volume level (0-21) |
| `timestamp` | integer | Timestamp in milliseconds |

### Status: LED

**Topic:** `{base_topic}/{device_id}/status/led`

LED subsystem status (published on LED state changes).

#### Message Format

```json
{
  "effect_running": true,
  "current_effect": 0,
  "brightness": 128,
  "timestamp": 1234567890
}
```

#### Fields

| Field | Type | Description |
|-------|------|-------------|
| `effect_running` | boolean | Whether an LED effect is currently running |
| `current_effect` | integer | Current LED effect ID (-1 if none) |
| `brightness` | integer | Current brightness level (0-255) |
| `timestamp` | integer | Timestamp in milliseconds |

### Status: Health

**Topic:** `{base_topic}/{device_id}/status/health`

System health check (published every 30 seconds).

#### Message Format

```json
{
  "status": "healthy",
  "healthy": true,
  "issues": "",
  "uptime": 1234567890,
  "free_heap": 245678,
  "timestamp": 1234567890
}
```

#### Fields

| Field | Type | Description |
|-------|------|-------------|
| `status` | string | Health status (`"healthy"` or `"unhealthy"`) |
| `healthy` | boolean | Overall health indicator |
| `issues` | string | Semicolon-separated list of issues (empty if healthy) |
| `uptime` | integer | System uptime in milliseconds |
| `free_heap` | integer | Free heap memory in bytes |
| `timestamp` | integer | Timestamp in milliseconds |

#### Health Checks

The controller checks for:
- Audio controller availability
- LED controller availability (strip or matrix)
- Settings controller availability
- WiFi connection status

If any check fails, `healthy` is set to `false` and issues are listed in `issues`.

### Status: Online (LWT)

**Topic:** `{base_topic}/{device_id}/status/online`

Last Will and Testament topic for online/offline status (retained message).

#### Message Values

- `"online"` - Published when controller connects to broker
- `"offline"` - Published automatically by broker when connection is lost

**Note:** This topic uses **retained messages** and is managed by the MQTT broker's Last Will and Testament feature. The controller publishes `"online"` on connection, and the broker automatically publishes `"offline"` if the connection is lost unexpectedly.

## Response Topic

**Topic:** `{base_topic}/{device_id}/response`

Command execution results and errors.

### Success Response Format

```json
{
  "status": "success",
  "command": "trigger",
  "effect": "godzilla_roar",
  "category": "growl",
  "executed": true,
  "audio_played": true,
  "led_started": true,
  "message": "Effect triggered successfully",
  "timestamp": 1234567890,
  "request_id": "req_12345",
  "error": null
}
```

### Error Response Format

```json
{
  "status": "error",
  "command": "trigger",
  "executed": false,
  "message": "Effect not found",
  "timestamp": 1234567890,
  "request_id": "req_12345",
  "error": "Effect not found"
}
```

### Response Fields

| Field | Type | Description |
|-------|------|-------------|
| `status` | string | `"success"` or `"error"` |
| `command` | string | Command that was executed |
| `executed` | boolean | Whether command was executed successfully |
| `message` | string | Human-readable message |
| `timestamp` | integer | Timestamp in milliseconds |
| `request_id` | string | Request ID from command (if provided) |
| `error` | string\|null | Error message (null if success) |

### Command-Specific Fields

#### trigger Command Response

Additional fields:
- `effect` (string) - Effect name that was executed
- `category` (string) - Effect category
- `audio_played` (boolean) - Whether audio was played
- `led_started` (boolean) - Whether LED effect was started

#### stop Command Response

Standard response format with `command: "stop"`.

#### volume Command Response

Standard response format with `command: "volume"`.

#### brightness Command Response

Standard response format with `command: "brightness"`.

## Message Formats

All MQTT messages use JSON format with UTF-8 encoding.

### JSON Schema Validation

Commands must be valid JSON. The controller uses ArduinoJson for parsing, which is lenient but will reject:
- Invalid JSON syntax
- Missing required fields
- Invalid data types
- Out-of-range values

### Character Encoding

- All strings use UTF-8 encoding
- Topic names use ASCII characters only (MQTT specification)
- JSON payloads support UTF-8 strings

## QoS Levels

Quality of Service (QoS) levels determine message delivery guarantees.

### QoS 0: At Most Once

- **Use:** Status topics (default)
- **Guarantee:** Best effort delivery, no acknowledgment
- **Use Case:** Non-critical status updates where occasional loss is acceptable
- **Performance:** Lowest overhead, fastest delivery

### QoS 1: At Least Once

- **Use:** Command topics (default)
- **Guarantee:** Message delivered at least once (may be duplicated)
- **Use Case:** Commands where delivery is important but duplicates are acceptable
- **Performance:** Moderate overhead, reliable delivery

### QoS 2: Exactly Once

- **Use:** Not currently used (reserved for future)
- **Guarantee:** Message delivered exactly once
- **Use Case:** Critical commands where duplicates are unacceptable
- **Performance:** Highest overhead, guaranteed delivery

### Configuration

QoS levels are configured in the web UI (Settings → MQTT):

- `qos_commands`: QoS for command topics (default: 1)
- `qos_status`: QoS for status topics (default: 0)

## Connection Behavior

### Initial Connection

1. Controller loads MQTT configuration from Preferences (set via web UI)
2. Resolves broker hostname (mDNS or DNS)
3. Connects to broker with client ID: `kinemachina_{device_id}` or `kinemachina_{MAC}`
4. Publishes LWT: `"online"` to `{base_topic}/{device_id}/status/online`
5. Subscribes to command topic
6. Publishes initial status and health check

### Reconnection

The controller automatically reconnects if the connection is lost:

1. **Exponential Backoff:** Reconnect delay starts at 1 second, increases to 30 seconds maximum
2. **WiFi Check:** Only attempts reconnection if WiFi is connected
3. **Hostname Re-resolution:** Re-resolves broker hostname if needed (in case IP changed)
4. **Session Persistence:** Uses persistent session (`clean_session: false`) to receive queued messages

### Disconnection

When disconnecting gracefully:
- Controller publishes `"offline"` to LWT topic
- Broker retains `"offline"` message for subscribers

When disconnecting unexpectedly:
- Broker automatically publishes `"offline"` via LWT
- Subscribers receive offline status immediately

### Keepalive

The keepalive interval (default: 60 seconds) ensures the connection stays alive:
- Controller sends PING packets if no messages sent within keepalive period
- Broker disconnects if no PING received within 1.5x keepalive period

## Usage Examples

### Command Line (mosquitto)

#### Subscribe to Status

```bash
# Subscribe to all status topics
mosquitto_sub -h mqtt.broker.local -t "kinemachina/sfx/kinemachina_001/status/#"

# Subscribe to responses only
mosquitto_sub -h mqtt.broker.local -t "kinemachina/sfx/kinemachina_001/response"

# Subscribe to specific status topic
mosquitto_sub -h mqtt.broker.local -t "kinemachina/sfx/kinemachina_001/status/audio"
```

#### Publish Commands

```bash
# Trigger effect by category
mosquitto_pub -h mqtt.broker.local -t "kinemachina/sfx/kinemachina_001/command" \
  -m '{"command":"trigger","category":"attack","request_id":"cmd_001"}'

# Trigger specific effect
mosquitto_pub -h mqtt.broker.local -t "kinemachina/sfx/kinemachina_001/command" \
  -m '{"command":"trigger","effect":"godzilla_roar","request_id":"cmd_002"}'

# Stop all effects
mosquitto_pub -h mqtt.broker.local -t "kinemachina/sfx/kinemachina_001/command" \
  -m '{"command":"stop","request_id":"cmd_003"}'

# Set volume
mosquitto_pub -h mqtt.broker.local -t "kinemachina/sfx/kinemachina_001/command" \
  -m '{"command":"volume","volume":18,"request_id":"cmd_004"}'

# Set brightness
mosquitto_pub -h mqtt.broker.local -t "kinemachina/sfx/kinemachina_001/command" \
  -m '{"command":"brightness","brightness":200,"request_id":"cmd_005"}'
```

### Python

```python
import paho.mqtt.client as mqtt
import json

# Configuration
BROKER = "mqtt.broker.local"
PORT = 1883
BASE_TOPIC = "kinemachina/sfx"
DEVICE_ID = "kinemachina_001"

# Create client
client = mqtt.Client()
client.connect(BROKER, PORT, 60)

# Trigger effect
def trigger_effect(category=None, effect=None, request_id=None):
    topic = f"{BASE_TOPIC}/{DEVICE_ID}/command"
    payload = {"command": "trigger"}

    if category:
        payload["category"] = category
    if effect:
        payload["effect"] = effect
    if request_id:
        payload["request_id"] = request_id

    client.publish(topic, json.dumps(payload))

# Subscribe to responses
def on_message(client, userdata, msg):
    response = json.loads(msg.payload.decode())
    print(f"Response: {response}")

client.on_message = on_message
client.subscribe(f"{BASE_TOPIC}/{DEVICE_ID}/response")
client.loop_start()

# Example: Trigger attack effect
trigger_effect(category="attack", request_id="python_001")
```

### Node-RED

#### Flow Example

```json
[
  {
    "id": "trigger_attack",
    "type": "inject",
    "name": "Trigger Attack",
    "props": [{"p": "payload"}],
    "repeat": "",
    "crontab": "",
    "once": false,
    "onceDelay": 0.1,
    "topic": "",
    "payload": "{\"command\":\"trigger\",\"category\":\"attack\",\"request_id\":\"node_001\"}",
    "payloadType": "json",
    "x": 100,
    "y": 100,
    "wires": [["mqtt_out"]]
  },
  {
    "id": "mqtt_out",
    "type": "mqtt out",
    "name": "MQTT Command",
    "topic": "kinemachina/sfx/kinemachina_001/command",
    "qos": "1",
    "retain": "false",
    "broker": "mqtt_broker",
    "x": 300,
    "y": 100,
    "wires": []
  },
  {
    "id": "mqtt_in",
    "type": "mqtt in",
    "name": "MQTT Status",
    "topic": "kinemachina/sfx/kinemachina_001/status",
    "qos": "0",
    "broker": "mqtt_broker",
    "x": 100,
    "y": 200,
    "wires": [["debug"]]
  },
  {
    "id": "debug",
    "type": "debug",
    "name": "Status",
    "active": true,
    "tosidebar": true,
    "console": false,
    "tostatus": false,
    "complete": "payload",
    "targetType": "full",
    "statusVal": "",
    "statusType": "auto",
    "x": 300,
    "y": 200,
    "wires": []
  },
  {
    "id": "mqtt_broker",
    "type": "mqtt-broker",
    "name": "MQTT Broker",
    "broker": "mqtt.broker.local",
    "port": "1883",
    "clientid": "",
    "autoConnect": true,
    "usetls": false,
    "protocolVersion": "4",
    "keepalive": "60",
    "cleansession": true,
    "birthTopic": "",
    "birthQos": "0",
    "birthPayload": "",
    "birthMsg": {},
    "closeTopic": "",
    "closeQos": "0",
    "closePayload": "",
    "closeMsg": {},
    "willTopic": "",
    "willQos": "0",
    "willPayload": "",
    "willMsg": {},
    "userProps": "",
    "sessionExpiry": ""
  }
]
```

### Home Assistant

#### Configuration (configuration.yaml)

```yaml
mqtt:
  broker: mqtt.broker.local
  port: 1883

# MQTT Sensor for Status
sensor:
  - platform: mqtt
    name: "Kinemachina Status"
    state_topic: "kinemachina/sfx/kinemachina_001/status"
    value_template: "{{ value_json.audio.playing }}"
    json_attributes_topic: "kinemachina/sfx/kinemachina_001/status"
    json_attributes_template: "{{ value_json | tojson }}"

# MQTT Switch for Control
switch:
  - platform: mqtt
    name: "Kinemachina Attack"
    command_topic: "kinemachina/sfx/kinemachina_001/command"
    payload_on: '{"command":"trigger","category":"attack"}'
    payload_off: '{"command":"stop"}'
    state_topic: "kinemachina/sfx/kinemachina_001/status"
    state_on: "true"
    state_off: "false"
    value_template: "{{ value_json.audio.playing }}"
```

## Error Handling

### Command Errors

Commands may fail for various reasons:

1. **Invalid JSON:** Malformed JSON payload
2. **Missing Fields:** Required fields not provided
3. **Invalid Values:** Out-of-range or invalid parameter values
4. **Effect Not Found:** Category or effect name doesn't exist
5. **Controller Unavailable:** Audio or LED controller not initialized

All errors are reported via the response topic with `status: "error"` and an `error` field containing the error message.

### Connection Errors

Connection errors are handled automatically:

- **Broker Unreachable:** Controller retries with exponential backoff
- **Authentication Failed:** Error logged, reconnection attempted
- **Network Issues:** Controller waits for WiFi reconnection before retrying

### Error Response Format

```json
{
  "status": "error",
  "command": "trigger",
  "executed": false,
  "message": "Effect not found",
  "timestamp": 1234567890,
  "request_id": "req_12345",
  "error": "Effect not found"
}
```

## Best Practices

### Topic Naming

- Use descriptive `device_id` values (e.g., `kinemachina_001`, `arena_left`, `display_main`)
- Keep `base_topic` consistent across all controllers in a system
- Use topic wildcards for monitoring multiple devices: `kinemachina/sfx/+/status`

### Request/Response Correlation

- Always include `request_id` in commands for tracking
- Subscribe to response topic to receive command confirmations
- Use unique `request_id` values (UUIDs, timestamps, or sequential IDs)

### Status Monitoring

- Subscribe to main status topic (`.../status`) for complete state
- Subscribe to subsystem topics (`.../status/audio`, `.../status/led`) for specific updates
- Use retained messages to get last known state on subscription
- Monitor health topic (`.../status/health`) for system diagnostics

### Performance

- Use QoS 0 for status (frequent updates, loss acceptable)
- Use QoS 1 for commands (delivery important, duplicates acceptable)
- Avoid publishing commands faster than controller can process (queue limit: 10 commands)
- Use `request_id` to detect duplicate responses if needed

### Security

- Use MQTT authentication (`username`/`password`) in production
- Consider TLS/SSL for encrypted connections (port 8883)
- Use unique `device_id` values to prevent topic collisions
- Restrict broker access via firewall rules

### Multi-Controller Systems

- Use consistent `base_topic` across all controllers
- Use unique `device_id` for each controller
- Subscribe to wildcard topics for system-wide monitoring: `kinemachina/sfx/+/status`
- Use topic structure to organize controllers: `kinemachina/sfx/{location}/{device_id}`

## Troubleshooting

### Controller Not Connecting

**Symptoms:** No status messages, commands not working

**Solutions:**
1. Check MQTT is enabled in the web UI (Settings → MQTT)
2. Verify broker hostname/IP is correct
3. Check broker is accessible from controller's network
4. Verify broker port is correct (default: 1883)
5. Check WiFi connection (controller must be connected to WiFi)
6. Review serial output for connection errors

### Commands Not Executing

**Symptoms:** Commands published but no response

**Solutions:**
1. Verify topic path matches configuration (`{base_topic}/{device_id}/command`)
2. Check JSON format is valid and includes a `"command"` field
3. Verify required fields are present
4. Check response topic for error messages
5. Ensure controller is connected (check `.../status/online` topic)

### Status Not Updating

**Symptoms:** Status topics not publishing updates

**Solutions:**
1. Verify controller is connected to broker
2. Check QoS settings (status uses QoS 0 by default)
3. Subscribe to status topics to verify publishing
4. Check serial output for MQTT errors
5. Verify state changes are occurring (audio playing, LED effects running)

### mDNS Resolution Issues

**Symptoms:** Connection fails with `.local` hostname

**Solutions:**
1. Verify mDNS is working on network (try `ping mqtt.broker.local`)
2. Use IP address instead of hostname as temporary workaround
3. Check broker supports mDNS/Bonjour
4. Wait 1-2 seconds after WiFi connection for mDNS to initialize

### High Latency

**Symptoms:** Commands take too long to execute

**Solutions:**
1. Check network latency to broker
2. Verify WiFi signal strength
3. Reduce QoS level if using QoS 2
4. Check command queue isn't full (limit: 10 commands)
5. Monitor serial output for processing delays

### Duplicate Messages

**Symptoms:** Same command executed multiple times

**Solutions:**
1. This is expected with QoS 1 (at least once delivery)
2. Use `request_id` to detect and ignore duplicates
3. Consider implementing idempotency in your application
4. Check broker configuration for duplicate prevention

---

## Additional Resources

- [MQTT Specification](https://mqtt.org/mqtt-specification/)
- [ArduinoJson Documentation](https://arduinojson.org/)
- [ESP32 MQTT Client Library](https://github.com/marvinroger/async-mqtt-client)
- [Home Assistant MQTT Integration](https://www.home-assistant.io/integrations/mqtt/)
- [Node-RED MQTT Nodes](https://nodered.org/docs/user-guide/nodes/mqtt)

---

**Last Updated:** 2026-01-28  
**Controller Version:** 1.0  
**MQTT API Version:** 1.0
