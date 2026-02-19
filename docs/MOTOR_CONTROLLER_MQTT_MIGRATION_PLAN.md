# Motor Controller MQTT Migration Plan

Complete migration plan for adding MQTT support to the Motor Controller, following the same pattern as the Audio & LED Controller MQTT implementation.

## Overview

This plan outlines the migration of motor controller functionality to use MQTT for event-driven control, following the successful MQTT migration pattern used for the Audio & LED Controller. The motor controller will control turntable/motorized platforms in the battle arena system.

## Migration Strategy

### Phase 1: Analysis & Design
- Analyze existing motor controller HTTP API endpoints (if any)
- Design MQTT topic structure following established patterns
- Define command and status message formats
- Plan FreeRTOS integration (queues, tasks)

### Phase 2: Implementation
- Create MotorController class (if not exists)
- Add MQTT command handling to MQTTController
- Implement motor-specific command topics
- Add motor status publishing
- Integrate with existing MQTT infrastructure

### Phase 3: Documentation
- Create MOTOR_MQTT_API.md documentation
- Update README.md with motor MQTT section
- Update PRODUCT_SPECIFICATION.md
- Add usage examples

### Phase 4: Testing & Validation
- Test motor commands via MQTT
- Verify status publishing
- Test multi-controller scenarios
- Validate error handling

## Current State Analysis

### Assumed Motor Controller Features

Based on battle arena requirements, the motor controller likely needs:

1. **Position Control:**
   - Rotate to absolute position (0-360 degrees)
   - Rotate relative (clockwise/counterclockwise)
   - Set rotation speed

2. **Movement Commands:**
   - Start rotation
   - Stop rotation
   - Emergency stop
   - Home/calibrate position

3. **Status Reporting:**
   - Current position
   - Target position
   - Movement state (idle, rotating, homing, error)
   - Speed setting
   - Error conditions

4. **Configuration:**
   - Maximum speed
   - Acceleration/deceleration
   - Home position
   - Position limits

### HTTP API Endpoints (Assumed)

If motor controller has HTTP API, it likely includes:

- `POST /motor/rotate?angle=90&speed=50` - Rotate to position
- `POST /motor/rotate-relative?degrees=45&speed=50` - Relative rotation
- `POST /motor/stop` - Stop movement
- `POST /motor/home` - Home/calibrate
- `GET /motor/status` - Get current status
- `POST /motor/speed?speed=50` - Set speed
- `POST /motor/config` - Update configuration

## MQTT Topic Structure

Following the established pattern from Audio & LED Controller:

### Base Topic Structure

```
{base_topic}/{device_id}/motor/{command|status|response}
```

### Command Topics (Subscribe)

- `{base_topic}/{device_id}/motor/command/rotate` - Rotate to absolute position
- `{base_topic}/{device_id}/motor/command/rotate-relative` - Relative rotation
- `{base_topic}/{device_id}/motor/command/stop` - Stop movement
- `{base_topic}/{device_id}/motor/command/home` - Home/calibrate
- `{base_topic}/{device_id}/motor/command/speed` - Set rotation speed
- `{base_topic}/{device_id}/motor/command/config` - Update configuration

### Status Topics (Publish)

- `{base_topic}/{device_id}/motor/status` - Complete motor status (retained)
- `{base_topic}/{device_id}/motor/status/position` - Position updates
- `{base_topic}/{device_id}/motor/status/movement` - Movement state changes
- `{base_topic}/{device_id}/motor/status/health` - Health check

### Response Topic (Publish)

- `{base_topic}/{device_id}/motor/response` - Command execution results

### Example with Default Config

With `base_topic: "kinemachina/sfx"` and `device_id: "motor_001"`:

```
kinemachina/sfx/motor_001/motor/
├── command/
│   ├── rotate           (subscribe)
│   ├── rotate-relative  (subscribe)
│   ├── stop             (subscribe)
│   ├── home             (subscribe)
│   ├── speed            (subscribe)
│   └── config           (subscribe)
├── status/
│   ├── (main)           (publish, retained)
│   ├── position         (publish)
│   ├── movement         (publish)
│   └── health           (publish)
└── response             (publish)
```

## Command Message Formats

### Command: rotate

**Topic:** `{base_topic}/{device_id}/motor/command/rotate`

Rotate to absolute position.

#### Request Format

```json
{
  "command": "rotate",
  "angle": 90,
  "speed": 50,
  "direction": "shortest",
  "request_id": "req_12345"
}
```

#### Parameters

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `command` | string | Yes | Must be `"rotate"` |
| `angle` | float | Yes | Target angle in degrees (0-360) |
| `speed` | integer | No | Rotation speed (0-100, default: current speed) |
| `direction` | string | No | `"shortest"`, `"clockwise"`, or `"counterclockwise"` (default: `"shortest"`) |
| `request_id` | string | No | Optional request identifier |

#### Behavior

- Rotates motor to specified absolute angle
- Uses shortest path by default
- Can override with direction parameter
- Speed can be specified or uses current setting
- Publishes position updates during movement
- Publishes response when movement completes

### Command: rotate-relative

**Topic:** `{base_topic}/{device_id}/motor/command/rotate-relative`

Rotate relative to current position.

#### Request Format

```json
{
  "command": "rotate-relative",
  "degrees": 45,
  "speed": 50,
  "request_id": "req_12346"
}
```

#### Parameters

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `command` | string | Yes | Must be `"rotate-relative"` |
| `degrees` | float | Yes | Degrees to rotate (positive = clockwise, negative = counterclockwise) |
| `speed` | integer | No | Rotation speed (0-100, default: current speed) |
| `request_id` | string | No | Optional request identifier |

### Command: stop

**Topic:** `{base_topic}/{device_id}/motor/command/stop`

Stop all movement immediately.

#### Request Format

```json
{
  "command": "stop",
  "emergency": false,
  "request_id": "req_12347"
}
```

#### Parameters

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `command` | string | Yes | Must be `"stop"` |
| `emergency` | boolean | No | Emergency stop (immediate, no deceleration) |
| `request_id` | string | No | Optional request identifier |

### Command: home

**Topic:** `{base_topic}/{device_id}/motor/command/home`

Home/calibrate motor position.

#### Request Format

```json
{
  "command": "home",
  "direction": "clockwise",
  "request_id": "req_12348"
}
```

#### Parameters

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `command` | string | Yes | Must be `"home"` |
| `direction` | string | No | `"clockwise"` or `"counterclockwise"` (default: `"clockwise"`) |
| `request_id` | string | No | Optional request identifier |

### Command: speed

**Topic:** `{base_topic}/{device_id}/motor/command/speed`

Set rotation speed.

#### Request Format

```json
{
  "command": "speed",
  "speed": 75,
  "request_id": "req_12349"
}
```

#### Parameters

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `command` | string | Yes | Must be `"speed"` |
| `speed` | integer | Yes | Speed level (0-100, where 100 is maximum) |
| `request_id` | string | No | Optional request identifier |

### Command: config

**Topic:** `{base_topic}/{device_id}/motor/command/config`

Update motor configuration.

#### Request Format

```json
{
  "command": "config",
  "max_speed": 100,
  "acceleration": 50,
  "deceleration": 50,
  "home_position": 0,
  "min_angle": 0,
  "max_angle": 360,
  "request_id": "req_12350"
}
```

#### Parameters

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `command` | string | Yes | Must be `"config"` |
| `max_speed` | integer | No | Maximum speed (0-100) |
| `acceleration` | integer | No | Acceleration rate (0-100) |
| `deceleration` | integer | No | Deceleration rate (0-100) |
| `home_position` | float | No | Home position angle (0-360) |
| `min_angle` | float | No | Minimum allowed angle |
| `max_angle` | float | No | Maximum allowed angle |
| `request_id` | string | No | Optional request identifier |

## Status Message Formats

### Status: Main Status

**Topic:** `{base_topic}/{device_id}/motor/status`

Complete motor status (retained message).

#### Message Format

```json
{
  "status": "success",
  "position": {
    "current": 45.5,
    "target": 90.0,
    "home": 0.0
  },
  "movement": {
    "state": "rotating",
    "speed": 50,
    "direction": "clockwise"
  },
  "config": {
    "max_speed": 100,
    "acceleration": 50,
    "deceleration": 50,
    "min_angle": 0,
    "max_angle": 360
  },
  "error": null,
  "timestamp": 1234567890
}
```

#### Fields

| Field | Type | Description |
|-------|------|-------------|
| `status` | string | Status indicator (`"success"` or `"error"`) |
| `position.current` | float | Current angle in degrees (0-360) |
| `position.target` | float | Target angle (null if idle) |
| `position.home` | float | Home position angle |
| `movement.state` | string | Movement state: `"idle"`, `"rotating"`, `"homing"`, `"error"` |
| `movement.speed` | integer | Current speed setting (0-100) |
| `movement.direction` | string | Current direction: `"clockwise"`, `"counterclockwise"`, or `null` |
| `config.max_speed` | integer | Maximum speed setting |
| `config.acceleration` | integer | Acceleration rate |
| `config.deceleration` | integer | Deceleration rate |
| `config.min_angle` | float | Minimum allowed angle |
| `config.max_angle` | float | Maximum allowed angle |
| `error` | string\|null | Error message (null if no error) |
| `timestamp` | integer | Timestamp in milliseconds |

### Status: Position

**Topic:** `{base_topic}/{device_id}/motor/status/position`

Position updates (published during movement).

#### Message Format

```json
{
  "current": 67.3,
  "target": 90.0,
  "progress": 0.75,
  "timestamp": 1234567890
}
```

### Status: Movement

**Topic:** `{base_topic}/{device_id}/motor/status/movement`

Movement state changes.

#### Message Format

```json
{
  "state": "rotating",
  "speed": 50,
  "direction": "clockwise",
  "timestamp": 1234567890
}
```

### Status: Health

**Topic:** `{base_topic}/{device_id}/motor/status/health`

Motor health check.

#### Message Format

```json
{
  "status": "healthy",
  "healthy": true,
  "issues": "",
  "uptime": 1234567890,
  "error_count": 0,
  "timestamp": 1234567890
}
```

## Response Message Format

**Topic:** `{base_topic}/{device_id}/motor/response`

Command execution results.

### Success Response

```json
{
  "status": "success",
  "command": "rotate",
  "executed": true,
  "message": "Rotation started",
  "target_angle": 90.0,
  "estimated_duration": 2000,
  "timestamp": 1234567890,
  "request_id": "req_12345",
  "error": null
}
```

### Error Response

```json
{
  "status": "error",
  "command": "rotate",
  "executed": false,
  "message": "Invalid angle: 450",
  "timestamp": 1234567890,
  "request_id": "req_12345",
  "error": "Angle must be between 0 and 360 degrees"
}
```

## Implementation Steps

### Step 1: Extend MQTTController

Add motor command types to `MQTTController.h`:

```cpp
// In MQTTCommand struct
enum Type {
    CMD_TRIGGER,
    CMD_STOP,
    CMD_VOLUME,
    CMD_BRIGHTNESS,
    // Motor commands
    CMD_MOTOR_ROTATE,
    CMD_MOTOR_ROTATE_RELATIVE,
    CMD_MOTOR_STOP,
    CMD_MOTOR_HOME,
    CMD_MOTOR_SPEED,
    CMD_MOTOR_CONFIG
};
```

### Step 2: Add Motor Command Topics

In `MQTTController::buildTopics()`:

```cpp
// Motor command topics
snprintf(topicMotorRotate, sizeof(topicMotorRotate), 
         "%s/%s/motor/command/rotate", base.c_str(), deviceId.c_str());
snprintf(topicMotorRotateRelative, sizeof(topicMotorRotateRelative), 
         "%s/%s/motor/command/rotate-relative", base.c_str(), deviceId.c_str());
snprintf(topicMotorStop, sizeof(topicMotorStop), 
         "%s/%s/motor/command/stop", base.c_str(), deviceId.c_str());
snprintf(topicMotorHome, sizeof(topicMotorHome), 
         "%s/%s/motor/command/home", base.c_str(), deviceId.c_str());
snprintf(topicMotorSpeed, sizeof(topicMotorSpeed), 
         "%s/%s/motor/command/speed", base.c_str(), deviceId.c_str());
snprintf(topicMotorConfig, sizeof(topicMotorConfig), 
         "%s/%s/motor/command/config", base.c_str(), deviceId.c_str());

// Motor status topics
snprintf(topicMotorStatus, sizeof(topicMotorStatus), 
         "%s/%s/motor/status", base.c_str(), deviceId.c_str());
snprintf(topicMotorStatusPosition, sizeof(topicMotorStatusPosition), 
         "%s/%s/motor/status/position", base.c_str(), deviceId.c_str());
snprintf(topicMotorStatusMovement, sizeof(topicMotorStatusMovement), 
         "%s/%s/motor/status/movement", base.c_str(), deviceId.c_str());
snprintf(topicMotorStatusHealth, sizeof(topicMotorStatusHealth), 
         "%s/%s/motor/status/health", base.c_str(), deviceId.c_str());
```

### Step 3: Subscribe to Motor Commands

In `MQTTController::subscribeToCommands()`:

```cpp
// Motor command subscriptions
packetId = mqttClient.subscribe(topicMotorRotate, config.qosCommands);
packetId = mqttClient.subscribe(topicMotorRotateRelative, config.qosCommands);
packetId = mqttClient.subscribe(topicMotorStop, config.qosCommands);
packetId = mqttClient.subscribe(topicMotorHome, config.qosCommands);
packetId = mqttClient.subscribe(topicMotorSpeed, config.qosCommands);
packetId = mqttClient.subscribe(topicMotorConfig, config.qosCommands);
```

### Step 4: Add Motor Controller Reference

In `MQTTController.h`:

```cpp
class MQTTController {
private:
    // ... existing controller references
    MotorController* motorController;  // Add motor controller reference
    
    // ... rest of class
};
```

### Step 5: Implement Command Handlers

Add handler methods in `MQTTController.cpp`:

```cpp
void MQTTController::handleCommandMotorRotate(const char* payload, size_t len);
void MQTTController::handleCommandMotorRotateRelative(const char* payload, size_t len);
void MQTTController::handleCommandMotorStop(const char* payload, size_t len);
void MQTTController::handleCommandMotorHome(const char* payload, size_t len);
void MQTTController::handleCommandMotorSpeed(const char* payload, size_t len);
void MQTTController::handleCommandMotorConfig(const char* payload, size_t len);
```

### Step 6: Add Status Publishing

In `MQTTController.cpp`:

```cpp
void MQTTController::publishMotorStatus(bool force = false);
void MQTTController::publishMotorPosition();
void MQTTController::publishMotorMovement();
void MQTTController::publishMotorHealth();
```

### Step 7: Update Message Handler

In `MQTTController::onMqttMessage()`, add motor topic handling:

```cpp
else if (strcmp(currentTopic, topicMotorRotate) == 0) {
    cmdType = MQTTCommand::CMD_MOTOR_ROTATE;
} else if (strcmp(currentTopic, topicMotorRotateRelative) == 0) {
    cmdType = MQTTCommand::CMD_MOTOR_ROTATE_RELATIVE;
}
// ... etc for other motor commands
```

### Step 8: Update Command Processor

In `MQTTController::processCommand()`:

```cpp
case MQTTCommand::CMD_MOTOR_ROTATE:
    handleCommandMotorRotate(cmd.payload, cmd.payloadLen);
    break;
case MQTTCommand::CMD_MOTOR_ROTATE_RELATIVE:
    handleCommandMotorRotateRelative(cmd.payload, cmd.payloadLen);
    break;
// ... etc
```

### Step 9: Integrate with MotorController

Ensure `MotorController` class has methods:

```cpp
class MotorController {
public:
    bool rotateTo(float angle, int speed = -1, const char* direction = "shortest");
    bool rotateRelative(float degrees, int speed = -1);
    void stop(bool emergency = false);
    bool home(const char* direction = "clockwise");
    bool setSpeed(int speed);
    bool updateConfig(const MotorConfig& config);
    
    float getCurrentPosition() const;
    float getTargetPosition() const;
    MovementState getMovementState() const;
    int getSpeed() const;
    MotorConfig getConfig() const;
};
```

### Step 10: Add Status Tracking

Track motor state changes for automatic status publishing:

```cpp
// In MQTTController private members
float lastMotorPosition;
MovementState lastMotorState;
int lastMotorSpeed;
unsigned long lastMotorStatusPublish;
```

## FreeRTOS Integration

### Queue-Based Command Processing

Follow the same pattern as Audio & LED commands:

1. **Command Queue:** Use existing `mqttCommandQueue` (shared with audio/LED commands)
2. **Dedicated Task:** Use existing `mqttTask` for processing
3. **Thread Safety:** All motor operations called from MQTT task context

### Status Publishing Task

Consider adding a separate task for motor status updates if high-frequency position updates are needed:

```cpp
// Optional: Dedicated motor status task
TaskHandle_t motorStatusTaskHandle;

void motorStatusTask(void* parameter) {
    while (true) {
        // Publish position updates during movement
        if (motorController->getMovementState() == MOVING) {
            publishMotorPosition();
        }
        vTaskDelay(pdMS_TO_TICKS(100)); // 10 Hz update rate
    }
}
```

## Configuration

### config.json Motor Section

Add motor configuration to `config.json`:

```json
{
  "motor": {
    "enabled": true,
    "max_speed": 100,
    "acceleration": 50,
    "deceleration": 50,
    "home_position": 0,
    "min_angle": 0,
    "max_angle": 360,
    "position_update_rate": 10
  }
}
```

### SettingsController Integration

Add motor config loading to `SettingsController`:

```cpp
struct MotorConfig {
    bool enabled;
    int maxSpeed;
    int acceleration;
    int deceleration;
    float homePosition;
    float minAngle;
    float maxAngle;
    int positionUpdateRate;
};

bool loadMotorConfig(MotorConfig* config, SemaphoreHandle_t sdMutex = nullptr);
bool saveMotorConfig(const MotorConfig& config);
```

## Usage Examples

### Command Line (mosquitto)

```bash
# Rotate to 90 degrees
mosquitto_pub -h mqtt.broker.local -t "kinemachina/sfx/motor_001/motor/command/rotate" \
  -m '{"command":"rotate","angle":90,"speed":50,"request_id":"rotate_001"}'

# Rotate relative 45 degrees clockwise
mosquitto_pub -h mqtt.broker.local -t "kinemachina/sfx/motor_001/motor/command/rotate-relative" \
  -m '{"command":"rotate-relative","degrees":45,"speed":75,"request_id":"rel_001"}'

# Stop movement
mosquitto_pub -h mqtt.broker.local -t "kinemachina/sfx/motor_001/motor/command/stop" \
  -m '{"command":"stop","request_id":"stop_001"}'

# Home motor
mosquitto_pub -h mqtt.broker.local -t "kinemachina/sfx/motor_001/motor/command/home" \
  -m '{"command":"home","direction":"clockwise","request_id":"home_001"}'

# Set speed
mosquitto_pub -h mqtt.broker.local -t "kinemachina/sfx/motor_001/motor/command/speed" \
  -m '{"command":"speed","speed":75,"request_id":"speed_001"}'

# Subscribe to status
mosquitto_sub -h mqtt.broker.local -t "kinemachina/sfx/motor_001/motor/status/#"
```

### Python Example

```python
import paho.mqtt.client as mqtt
import json

BROKER = "mqtt.broker.local"
BASE_TOPIC = "kinemachina/sfx"
DEVICE_ID = "motor_001"

client = mqtt.Client()
client.connect(BROKER, 1883, 60)

# Rotate to position
def rotate_to(angle, speed=50, request_id=None):
    topic = f"{BASE_TOPIC}/{DEVICE_ID}/motor/command/rotate"
    payload = {
        "command": "rotate",
        "angle": angle,
        "speed": speed
    }
    if request_id:
        payload["request_id"] = request_id
    client.publish(topic, json.dumps(payload))

# Subscribe to responses
def on_message(client, userdata, msg):
    response = json.loads(msg.payload.decode())
    print(f"Motor Response: {response}")

client.on_message = on_message
client.subscribe(f"{BASE_TOPIC}/{DEVICE_ID}/motor/response")
client.loop_start()

# Example: Rotate to 90 degrees
rotate_to(90, speed=75, request_id="python_001")
```

### Node-RED Example

```json
[
  {
    "id": "motor_rotate",
    "type": "inject",
    "name": "Rotate to 90°",
    "props": [{"p": "payload"}],
    "repeat": "",
    "crontab": "",
    "once": false,
    "topic": "",
    "payload": "{\"command\":\"rotate\",\"angle\":90,\"speed\":50,\"request_id\":\"node_001\"}",
    "payloadType": "json",
    "x": 100,
    "y": 100,
    "wires": [["motor_mqtt_out"]]
  },
  {
    "id": "motor_mqtt_out",
    "type": "mqtt out",
    "name": "Motor Command",
    "topic": "kinemachina/sfx/motor_001/motor/command/rotate",
    "qos": "1",
    "retain": "false",
    "broker": "mqtt_broker",
    "x": 300,
    "y": 100,
    "wires": []
  },
  {
    "id": "motor_status_in",
    "type": "mqtt in",
    "name": "Motor Status",
    "topic": "kinemachina/sfx/motor_001/motor/status",
    "qos": "0",
    "broker": "mqtt_broker",
    "x": 100,
    "y": 200,
    "wires": [["debug"]]
  }
]
```

## Error Handling

### Common Errors

1. **Invalid Angle:** Angle outside valid range (0-360 or configured min/max)
2. **Motor Busy:** Command received while motor is moving
3. **Home Not Calibrated:** Movement command before homing
4. **Speed Out of Range:** Speed value outside 0-100
5. **Hardware Error:** Motor driver error or communication failure

### Error Response Format

```json
{
  "status": "error",
  "command": "rotate",
  "executed": false,
  "message": "Invalid angle: 450",
  "timestamp": 1234567890,
  "request_id": "req_12345",
  "error": "Angle must be between 0 and 360 degrees",
  "error_code": "INVALID_ANGLE"
}
```

## Testing Checklist

### Unit Tests

- [ ] Command parsing (valid JSON)
- [ ] Parameter validation (angle, speed ranges)
- [ ] Error handling (invalid commands)
- [ ] Response generation

### Integration Tests

- [ ] MQTT connection and subscription
- [ ] Command execution (all command types)
- [ ] Status publishing (all status topics)
- [ ] Response publishing
- [ ] Error response handling

### System Tests

- [ ] Multi-controller scenarios
- [ ] High-frequency commands
- [ ] Network interruption recovery
- [ ] Concurrent commands (queue handling)
- [ ] Status update frequency

## Documentation Updates

### Files to Update

1. **MOTOR_MQTT_API.md** (NEW)
   - Complete API reference following MQTT_API.md pattern
   - All commands, status topics, examples

2. **README.md**
   - Add Motor Controller MQTT section
   - Quick start examples
   - Link to MOTOR_MQTT_API.md

3. **PRODUCT_SPECIFICATION.md**
   - Add motor controller features
   - Update competitive advantages
   - Add motor use cases

4. **config.json.example**
   - Add motor configuration section

## Migration Timeline

### Week 1: Design & Planning
- Finalize topic structure
- Design message formats
- Plan MotorController integration

### Week 2: Implementation
- Extend MQTTController
- Add command handlers
- Implement status publishing

### Week 3: Testing
- Unit tests
- Integration tests
- System tests

### Week 4: Documentation
- Create MOTOR_MQTT_API.md
- Update README.md
- Update PRODUCT_SPECIFICATION.md
- Add examples

## Success Criteria

- [ ] All motor commands work via MQTT
- [ ] Status publishing works correctly
- [ ] Error handling is robust
- [ ] Documentation is complete
- [ ] Examples work as documented
- [ ] Multi-controller scenarios work
- [ ] Performance meets requirements (<50ms latency)

## Notes

- Follow the same patterns as Audio & LED MQTT implementation
- Reuse existing MQTT infrastructure (connection, queues, tasks)
- Maintain backward compatibility with HTTP API (if exists)
- Consider position update frequency (balance between accuracy and network load)
- Use retained messages for status topic (last known state)
- Implement proper error recovery for motor hardware failures

---

**Last Updated:** 2026-01-28  
**Plan Version:** 1.0  
**Based On:** MQTT_API.md migration pattern
