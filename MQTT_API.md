# SFX Controller MQTT API — KRP v1.0

MQTT interface for the KineMachina SFX Controller, compliant with KRP (KineMachina Robot Protocol) v1.0.

## Topic Structure

All topics use the KRP prefix `krp/{deviceId}/...`. The `deviceId` defaults to `sfx-001` and is configured via the web UI.

```
krp/{deviceId}/
├── $state              (retained) Device lifecycle: online → ready → offline
├── $name               (retained) Human-readable name
├── $capabilities       (retained) JSON capabilities manifest
├── command             (subscribe) Single topic, JSON dispatch
├── response            (publish) Command responses
└── status/
    ├── (main)          (retained) Full system status
    ├── audio           Audio subsystem changes
    ├── led             LED subsystem changes
    └── health          Periodic health (30s)
```

## Birth Sequence

On MQTT connect, the controller executes the KRP birth sequence:

1. **LWT** set to `krp/{deviceId}/$state` → `"offline"` (QoS 1, retained)
2. Publish `$state` → `"online"` (QoS 1, retained)
3. Publish `$name` → device name (QoS 1, retained)
4. Publish `$capabilities` → JSON manifest (QoS 1, retained)
5. Subscribe to `krp/{deviceId}/command`
6. Publish initial status + health
7. Publish `$state` → `"ready"` (QoS 1, retained)

On unexpected disconnect, the broker publishes `$state` → `"offline"` via LWT.

## Capabilities Manifest

Published to `$capabilities` as retained JSON. Uses the KRP v1.0 generic capabilities array format:

```json
{
  "device_id": "sfx-001",
  "device_type": "sfx",
  "device_class": "sfx",
  "name": "SFX Controller A",
  "platform": "esp32s3",
  "protocol_version": "1.0",
  "capabilities": [
    {
      "type": "audio",
      "items": [
        {"id": "godzilla_roar", "category": "attack"},
        {"id": "stomp", "category": "idle"}
      ],
      "params": {"volume": {"min": 0, "max": 21}}
    },
    {
      "type": "lighting",
      "items": [
        {"id": "atomic_breath", "target": "strip"},
        {"id": "beam_attack", "target": "matrix"}
      ],
      "params": {"brightness": {"min": 0, "max": 255}}
    }
  ]
}
```

Audio items are built dynamically from effects in `config.json`. Lighting items are the built-in strip (23) and matrix (31) effect names.

## Commands

**Topic:** `krp/{deviceId}/command`

All commands are JSON with a `"command"` field. Command matching is case-insensitive. All commands accept an optional `request_id` for response correlation.

### trigger

Execute an effect by name or category.

```json
{
  "command": "trigger",
  "effect": "godzilla_roar",
  "category": "attack",
  "audio_volume": 18,
  "led_brightness": 128,
  "request_id": "req_001"
}
```

| Field | Required | Description |
|-------|----------|-------------|
| `effect` | One of effect/category | Effect name (config.json or LED effect name) |
| `category` | One of effect/category | Random effect from category (attack, growl, damage, death, victory, idle) |
| `audio_volume` | No | Volume override (0-21) |
| `led_brightness` | No | Brightness override (0-255) |

If both `effect` and `category` are specified, `effect` takes precedence. LED-only effects (same names as HTTP API) are also available.

### stop

Stop all audio and LED effects.

```json
{"command": "stop", "request_id": "req_002"}
```

### volume

Set audio volume.

```json
{"command": "volume", "volume": 18, "request_id": "req_003"}
```

### brightness

Set LED brightness.

```json
{"command": "brightness", "brightness": 200, "request_id": "req_004"}
```

### bass_mono

Configure bass mono processing.

```json
{"command": "bass_mono", "enabled": true, "crossover_hz": 80, "request_id": "req_005"}
```

## Responses

**Topic:** `krp/{deviceId}/response`

### Success

```json
{
  "status": "ok",
  "command": "trigger",
  "timestamp": 1234567890,
  "request_id": "req_001"
}
```

### Error

```json
{
  "status": "error",
  "command": "trigger",
  "message": "Effect not found",
  "timestamp": 1234567890,
  "request_id": "req_001"
}
```

## Status Topics

### Main Status

**Topic:** `krp/{deviceId}/status` (retained)

```json
{
  "status": "ok",
  "audio": {"playing": true, "volume": 21, "current_file": "/sounds/roar.mp3"},
  "led": {"effect_running": true, "current_effect": 0, "brightness": 128, "audio_reactive": false},
  "wifi": {"connected": true, "ip": "192.168.1.100"},
  "timestamp": 1234567890,
  "free_heap": 245678
}
```

### Audio Status

**Topic:** `krp/{deviceId}/status/audio`

```json
{"playing": true, "volume": 21, "timestamp": 1234567890}
```

### LED Status

**Topic:** `krp/{deviceId}/status/led`

```json
{"effect_running": true, "current_effect": 0, "brightness": 128, "timestamp": 1234567890}
```

### Health

**Topic:** `krp/{deviceId}/status/health` (every 30s)

```json
{"status": "healthy", "healthy": true, "issues": "", "uptime": 1234567890, "free_heap": 245678, "timestamp": 1234567890}
```

## Configuration

Configured via web UI (Settings → MQTT), stored in ESP32 NVS.

| Parameter | Default | Description |
|-----------|---------|-------------|
| `enabled` | `false` | Enable MQTT |
| `broker` | — | Broker hostname/IP (supports `.local` mDNS) |
| `port` | `1883` | Broker port |
| `username` | `""` | MQTT username |
| `password` | `""` | MQTT password |
| `device_id` | `"sfx-001"` | Device ID used in topic paths |
| `device_name` | `"SFX Controller A"` | Human-readable name for `$name` |
| `qos_commands` | `1` | QoS for commands |
| `qos_status` | `0` | QoS for status |
| `keepalive` | `60` | Keepalive interval (seconds) |
| `clean_session` | `false` | Clean session flag |

## Examples

```bash
# Subscribe to all KRP topics for this device
mosquitto_sub -h rpi-5.local -t "krp/sfx-001/#" -v

# Trigger attack effect
mosquitto_pub -h rpi-5.local -t "krp/sfx-001/command" \
  -m '{"command":"trigger","category":"attack","request_id":"cmd_001"}'

# Trigger specific effect
mosquitto_pub -h rpi-5.local -t "krp/sfx-001/command" \
  -m '{"command":"trigger","effect":"godzilla_roar","request_id":"cmd_002"}'

# Stop all effects
mosquitto_pub -h rpi-5.local -t "krp/sfx-001/command" \
  -m '{"command":"stop","request_id":"cmd_003"}'

# Set volume
mosquitto_pub -h rpi-5.local -t "krp/sfx-001/command" \
  -m '{"command":"volume","volume":18,"request_id":"cmd_004"}'

# Watch device state
mosquitto_sub -h rpi-5.local -t "krp/sfx-001/$state" -v
```

## Connection Behavior

- **Reconnection:** Automatic with exponential backoff (1s → 30s max)
- **mDNS:** Supports `.local` hostnames, cached after resolution
- **Hostname re-resolution:** Every 5 failed reconnect attempts
- **Connect timeout:** 30s before treating as failure
