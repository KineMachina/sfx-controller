#include <unity.h>
#include "freertos_mock.h"
#include "SettingsController.h"

static SettingsController* settings;

void setUp(void) {
    settings = new SettingsController();
}

void tearDown(void) {
    delete settings;
    settings = nullptr;
}

// ---------------------------------------------------------------------------
// getLEDEffectFromName — strip effects
// ---------------------------------------------------------------------------

void test_led_effect_atomic_breath(void) {
    bool isStrip = false;
    int enumVal = 0;
    TEST_ASSERT_TRUE(settings->getLEDEffectFromName("atomic_breath", &isStrip, &enumVal));
    TEST_ASSERT_TRUE(isStrip);
    TEST_ASSERT_EQUAL_INT(1, enumVal);
}

void test_led_effect_atomic_alias(void) {
    bool isStrip = false;
    int enumVal = 0;
    TEST_ASSERT_TRUE(settings->getLEDEffectFromName("atomic", &isStrip, &enumVal));
    TEST_ASSERT_TRUE(isStrip);
    TEST_ASSERT_EQUAL_INT(1, enumVal);
}

void test_led_effect_fire_breath(void) {
    bool isStrip = false;
    int enumVal = 0;
    TEST_ASSERT_TRUE(settings->getLEDEffectFromName("fire_breath", &isStrip, &enumVal));
    TEST_ASSERT_TRUE(isStrip);
    TEST_ASSERT_EQUAL_INT(3, enumVal);
}

void test_led_effect_rainbow(void) {
    bool isStrip = false;
    int enumVal = 0;
    TEST_ASSERT_TRUE(settings->getLEDEffectFromName("rainbow", &isStrip, &enumVal));
    TEST_ASSERT_TRUE(isStrip);
    TEST_ASSERT_EQUAL_INT(8, enumVal);
}

void test_led_effect_radial_gradient(void) {
    bool isStrip = false;
    int enumVal = 0;
    TEST_ASSERT_TRUE(settings->getLEDEffectFromName("radial_gradient", &isStrip, &enumVal));
    TEST_ASSERT_TRUE(isStrip);
    TEST_ASSERT_EQUAL_INT(23, enumVal);
}

// ---------------------------------------------------------------------------
// getLEDEffectFromName — matrix effects
// ---------------------------------------------------------------------------

void test_led_effect_beam_attack(void) {
    bool isStrip = true;
    int enumVal = 0;
    TEST_ASSERT_TRUE(settings->getLEDEffectFromName("beam_attack", &isStrip, &enumVal));
    TEST_ASSERT_FALSE(isStrip);
    TEST_ASSERT_EQUAL_INT(1, enumVal);
}

void test_led_effect_beam_alias(void) {
    bool isStrip = true;
    int enumVal = 0;
    TEST_ASSERT_TRUE(settings->getLEDEffectFromName("beam", &isStrip, &enumVal));
    TEST_ASSERT_FALSE(isStrip);
    TEST_ASSERT_EQUAL_INT(1, enumVal);
}

void test_led_effect_explosion(void) {
    bool isStrip = true;
    int enumVal = 0;
    TEST_ASSERT_TRUE(settings->getLEDEffectFromName("explosion", &isStrip, &enumVal));
    TEST_ASSERT_FALSE(isStrip);
    TEST_ASSERT_EQUAL_INT(2, enumVal);
}

void test_led_effect_transporter_tos(void) {
    bool isStrip = true;
    int enumVal = 0;
    TEST_ASSERT_TRUE(settings->getLEDEffectFromName("transporter_tos", &isStrip, &enumVal));
    TEST_ASSERT_FALSE(isStrip);
    TEST_ASSERT_EQUAL_INT(31, enumVal);
}

void test_led_effect_transporter_alias(void) {
    bool isStrip = true;
    int enumVal = 0;
    TEST_ASSERT_TRUE(settings->getLEDEffectFromName("transporter", &isStrip, &enumVal));
    TEST_ASSERT_FALSE(isStrip);
    TEST_ASSERT_EQUAL_INT(31, enumVal);
}

// ---------------------------------------------------------------------------
// getLEDEffectFromName — case insensitivity
// ---------------------------------------------------------------------------

void test_led_effect_case_insensitive(void) {
    bool isStrip = false;
    int enumVal = 0;
    TEST_ASSERT_TRUE(settings->getLEDEffectFromName("ATOMIC_BREATH", &isStrip, &enumVal));
    TEST_ASSERT_TRUE(isStrip);
    TEST_ASSERT_EQUAL_INT(1, enumVal);
}

void test_led_effect_mixed_case(void) {
    bool isStrip = false;
    int enumVal = 0;
    TEST_ASSERT_TRUE(settings->getLEDEffectFromName("Beam_Attack", &isStrip, &enumVal));
    TEST_ASSERT_FALSE(isStrip);
    TEST_ASSERT_EQUAL_INT(1, enumVal);
}

// ---------------------------------------------------------------------------
// getLEDEffectFromName — invalid inputs
// ---------------------------------------------------------------------------

void test_led_effect_unknown_name(void) {
    bool isStrip = false;
    int enumVal = 0;
    TEST_ASSERT_FALSE(settings->getLEDEffectFromName("nonexistent_effect", &isStrip, &enumVal));
}

void test_led_effect_empty_string(void) {
    bool isStrip = false;
    int enumVal = 0;
    TEST_ASSERT_FALSE(settings->getLEDEffectFromName("", &isStrip, &enumVal));
}

void test_led_effect_null_name(void) {
    bool isStrip = false;
    int enumVal = 0;
    TEST_ASSERT_FALSE(settings->getLEDEffectFromName(nullptr, &isStrip, &enumVal));
}

void test_led_effect_null_out_params(void) {
    int enumVal = 0;
    TEST_ASSERT_FALSE(settings->getLEDEffectFromName("atomic_breath", nullptr, &enumVal));
    bool isStrip = false;
    TEST_ASSERT_FALSE(settings->getLEDEffectFromName("atomic_breath", &isStrip, nullptr));
}

void test_led_effect_whitespace_trimmed(void) {
    bool isStrip = false;
    int enumVal = 0;
    TEST_ASSERT_TRUE(settings->getLEDEffectFromName("  rainbow  ", &isStrip, &enumVal));
    TEST_ASSERT_TRUE(isStrip);
    TEST_ASSERT_EQUAL_INT(8, enumVal);
}

// ---------------------------------------------------------------------------
// In-memory state: volume
// ---------------------------------------------------------------------------

void test_volume_default(void) {
    TEST_ASSERT_EQUAL_INT(21, settings->loadVolume(21));
    TEST_ASSERT_EQUAL_INT(10, settings->loadVolume(10));
}

void test_volume_save_load(void) {
    settings->saveVolume(15);
    TEST_ASSERT_EQUAL_INT(15, settings->loadVolume(21));
}

void test_volume_save_zero(void) {
    settings->saveVolume(0);
    // 0 is >= 0 so should be returned, not the default
    TEST_ASSERT_EQUAL_INT(0, settings->loadVolume(21));
}

// ---------------------------------------------------------------------------
// In-memory state: brightness
// ---------------------------------------------------------------------------

void test_brightness_default(void) {
    TEST_ASSERT_EQUAL_UINT8(128, settings->loadBrightness(128));
    TEST_ASSERT_EQUAL_UINT8(64, settings->loadBrightness(64));
}

void test_brightness_save_load(void) {
    settings->saveBrightness(200);
    TEST_ASSERT_EQUAL_UINT8(200, settings->loadBrightness(128));
}

void test_brightness_save_zero(void) {
    settings->saveBrightness(0);
    TEST_ASSERT_EQUAL_UINT8(0, settings->loadBrightness(128));
}

void test_brightness_save_max(void) {
    settings->saveBrightness(255);
    TEST_ASSERT_EQUAL_UINT8(255, settings->loadBrightness(128));
}

// ---------------------------------------------------------------------------
// In-memory state: demo delay
// ---------------------------------------------------------------------------

void test_demo_delay_default(void) {
    TEST_ASSERT_EQUAL_UINT32(5000, settings->loadDemoDelay(5000));
}

void test_demo_delay_save_load(void) {
    settings->saveDemoDelay(10000);
    TEST_ASSERT_EQUAL_UINT32(10000, settings->loadDemoDelay(5000));
}

// ---------------------------------------------------------------------------
// In-memory state: demo mode
// ---------------------------------------------------------------------------

void test_demo_mode_default(void) {
    TEST_ASSERT_EQUAL_INT(0, settings->loadDemoMode(0));
}

void test_demo_mode_save_load(void) {
    settings->saveDemoMode(2);
    TEST_ASSERT_EQUAL_INT(2, settings->loadDemoMode(0));
}

// ---------------------------------------------------------------------------
// In-memory state: demo order
// ---------------------------------------------------------------------------

void test_demo_order_default(void) {
    TEST_ASSERT_EQUAL_INT(0, settings->loadDemoOrder(0));
}

void test_demo_order_save_load(void) {
    settings->saveDemoOrder(1);
    TEST_ASSERT_EQUAL_INT(1, settings->loadDemoOrder(0));
}

// ---------------------------------------------------------------------------
// Effects: initial state
// ---------------------------------------------------------------------------

void test_effect_count_initial(void) {
    TEST_ASSERT_EQUAL_INT(0, settings->getEffectCount());
}

void test_get_effect_empty(void) {
    SettingsController::Effect effect;
    TEST_ASSERT_FALSE(settings->getEffect(0, &effect));
}

void test_get_effect_negative_index(void) {
    SettingsController::Effect effect;
    TEST_ASSERT_FALSE(settings->getEffect(-1, &effect));
}

void test_get_effect_null_pointer(void) {
    TEST_ASSERT_FALSE(settings->getEffect(0, nullptr));
}

// ---------------------------------------------------------------------------
// Effects: getEffectByName edge cases
// ---------------------------------------------------------------------------

void test_get_effect_by_name_null_name(void) {
    SettingsController::Effect effect;
    TEST_ASSERT_FALSE(settings->getEffectByName(nullptr, &effect));
}

void test_get_effect_by_name_null_effect(void) {
    TEST_ASSERT_FALSE(settings->getEffectByName("test", nullptr));
}

void test_get_effect_by_name_not_found(void) {
    SettingsController::Effect effect;
    TEST_ASSERT_FALSE(settings->getEffectByName("nonexistent", &effect));
}

// ---------------------------------------------------------------------------
// Effects: load from JSON and query
// ---------------------------------------------------------------------------

// Helper: directly call begin() which will fail to load SD (mock returns false)
// Then manually parse a JSON config to populate effects
static void loadTestEffects(SettingsController* sc) {
    // begin() will open preferences (mock succeeds) and try SD (mock fails).
    // Effects will remain at 0 after begin().
    sc->begin(nullptr);

    // Now simulate what loadFromConfigFile would do by calling
    // loadEffectsFromConfig via a JSON document
    const char* json = R"({
        "roar": {
            "audio": "/sounds/roar.mp3",
            "led": "atomic_breath",
            "loop": false,
            "category": "attack"
        },
        "idle_glow": {
            "led": "idle",
            "loop": true,
            "category": "idle"
        },
        "battle_cry": {
            "audio": "/sounds/battle.mp3",
            "led": "beam_attack",
            "loop": false,
            "av_sync": true,
            "category": "Attack"
        }
    })";

    JsonDocument doc;
    deserializeJson(doc, json);
    JsonObject obj = doc.as<JsonObject>();

    // loadEffectsFromConfig is private, but we can test via the public API
    // by using a workaround: we'll use the fact that SettingsController has
    // the method in the private section. Instead, we test through begin()
    // which calls it. Since SD mock fails, we need another approach.
    //
    // Alternative: make the test a friend, or test through the public API only.
    // For now, we'll test what we can through the public API.
    // The effects are loaded through loadFromConfigFile which needs SD.
    // We'll skip direct loadEffectsFromConfig tests and test only public APIs.
}

// Since loadEffectsFromConfig is private and SD mock always returns false,
// we test the public query API with the empty state — and verify the
// getLEDEffectFromName lookup works independently (it doesn't need loaded effects).

void test_get_effects_by_category_empty(void) {
    SettingsController::Effect results[5];
    int count = settings->getEffectsByCategory("attack", results, 5);
    TEST_ASSERT_EQUAL_INT(0, count);
}

void test_get_effects_by_category_null_safe(void) {
    SettingsController::Effect results[5];
    // Category string is used to build a String — should handle edge cases
    int count = settings->getEffectsByCategory("", results, 5);
    TEST_ASSERT_EQUAL_INT(0, count);
}

// ---------------------------------------------------------------------------
// begin() — verifies initialization doesn't crash with SD unavailable
// ---------------------------------------------------------------------------

void test_begin_without_sd(void) {
    // Should succeed (preferences mock works, SD mock gracefully fails)
    TEST_ASSERT_TRUE(settings->begin(nullptr));
    // No effects loaded since SD is unavailable
    TEST_ASSERT_EQUAL_INT(0, settings->getEffectCount());
}

void test_begin_with_mutex(void) {
    SemaphoreHandle_t fakeMutex = (SemaphoreHandle_t)1;
    TEST_ASSERT_TRUE(settings->begin(fakeMutex));
}

// ---------------------------------------------------------------------------
// Preferences-backed operations (WiFi, MQTT)
// ---------------------------------------------------------------------------

void test_wifi_ssid_save_load(void) {
    settings->begin(nullptr);
    settings->saveWiFiSSID("TestNetwork");
    char buffer[64];
    bool loaded = settings->loadWiFiSSID(buffer, sizeof(buffer));
    TEST_ASSERT_TRUE(loaded);
    TEST_ASSERT_EQUAL_STRING("TestNetwork", buffer);
}

void test_wifi_ssid_default(void) {
    settings->begin(nullptr);
    char buffer[64];
    bool loaded = settings->loadWiFiSSID(buffer, sizeof(buffer), "default_ssid");
    TEST_ASSERT_FALSE(loaded);
    TEST_ASSERT_EQUAL_STRING("default_ssid", buffer);
}

void test_wifi_password_save_load(void) {
    settings->begin(nullptr);
    settings->saveWiFiPassword("secret123");
    char buffer[64];
    bool loaded = settings->loadWiFiPassword(buffer, sizeof(buffer));
    TEST_ASSERT_TRUE(loaded);
    TEST_ASSERT_EQUAL_STRING("secret123", buffer);
}

void test_has_settings_empty(void) {
    settings->begin(nullptr);
    TEST_ASSERT_FALSE(settings->hasSettings());
}

void test_has_settings_after_wifi(void) {
    settings->begin(nullptr);
    settings->saveWiFiSSID("MyWiFi");
    TEST_ASSERT_TRUE(settings->hasSettings());
}

void test_clear_all(void) {
    settings->begin(nullptr);
    settings->saveWiFiSSID("MyWiFi");
    settings->clearAll();
    char buffer[64];
    bool loaded = settings->loadWiFiSSID(buffer, sizeof(buffer));
    TEST_ASSERT_FALSE(loaded);
}

// ---------------------------------------------------------------------------
// MQTT config
// ---------------------------------------------------------------------------

void test_mqtt_config_defaults(void) {
    settings->begin(nullptr);
    SettingsController::MQTTConfig config;
    bool result = settings->loadMQTTConfig(&config, nullptr);
    TEST_ASSERT_FALSE(result); // not enabled, no broker
    TEST_ASSERT_FALSE(config.enabled);
    TEST_ASSERT_EQUAL_INT(1883, config.port);
    TEST_ASSERT_EQUAL_INT(1, config.qosCommands);
    TEST_ASSERT_EQUAL_INT(0, config.qosStatus);
    TEST_ASSERT_EQUAL_INT(60, config.keepalive);
}

void test_mqtt_config_null_pointer(void) {
    settings->begin(nullptr);
    TEST_ASSERT_FALSE(settings->loadMQTTConfig(nullptr, nullptr));
}

void test_mqtt_config_save_load(void) {
    settings->begin(nullptr);

    SettingsController::MQTTConfig config;
    config.enabled = true;
    config.port = 8883;
    config.qosCommands = 2;
    config.qosStatus = 1;
    config.keepalive = 30;
    config.cleanSession = true;
    strncpy(config.broker, "mqtt.example.com", sizeof(config.broker));
    strncpy(config.username, "user", sizeof(config.username));
    strncpy(config.password, "pass", sizeof(config.password));
    strncpy(config.deviceId, "kinemachina01", sizeof(config.deviceId));
    strncpy(config.baseTopic, "kinemachina", sizeof(config.baseTopic));

    settings->saveMQTTConfig(&config);

    SettingsController::MQTTConfig loaded;
    bool result = settings->loadMQTTConfig(&loaded, nullptr);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(loaded.enabled);
    TEST_ASSERT_EQUAL_INT(8883, loaded.port);
    TEST_ASSERT_EQUAL_STRING("mqtt.example.com", loaded.broker);
    TEST_ASSERT_EQUAL_STRING("user", loaded.username);
    TEST_ASSERT_EQUAL_STRING("kinemachina01", loaded.deviceId);
    TEST_ASSERT_EQUAL_STRING("kinemachina", loaded.baseTopic);
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    UNITY_BEGIN();

    // getLEDEffectFromName — strip effects
    RUN_TEST(test_led_effect_atomic_breath);
    RUN_TEST(test_led_effect_atomic_alias);
    RUN_TEST(test_led_effect_fire_breath);
    RUN_TEST(test_led_effect_rainbow);
    RUN_TEST(test_led_effect_radial_gradient);

    // getLEDEffectFromName — matrix effects
    RUN_TEST(test_led_effect_beam_attack);
    RUN_TEST(test_led_effect_beam_alias);
    RUN_TEST(test_led_effect_explosion);
    RUN_TEST(test_led_effect_transporter_tos);
    RUN_TEST(test_led_effect_transporter_alias);

    // getLEDEffectFromName — case insensitivity
    RUN_TEST(test_led_effect_case_insensitive);
    RUN_TEST(test_led_effect_mixed_case);

    // getLEDEffectFromName — invalid inputs
    RUN_TEST(test_led_effect_unknown_name);
    RUN_TEST(test_led_effect_empty_string);
    RUN_TEST(test_led_effect_null_name);
    RUN_TEST(test_led_effect_null_out_params);
    RUN_TEST(test_led_effect_whitespace_trimmed);

    // In-memory state
    RUN_TEST(test_volume_default);
    RUN_TEST(test_volume_save_load);
    RUN_TEST(test_volume_save_zero);
    RUN_TEST(test_brightness_default);
    RUN_TEST(test_brightness_save_load);
    RUN_TEST(test_brightness_save_zero);
    RUN_TEST(test_brightness_save_max);
    RUN_TEST(test_demo_delay_default);
    RUN_TEST(test_demo_delay_save_load);
    RUN_TEST(test_demo_mode_default);
    RUN_TEST(test_demo_mode_save_load);
    RUN_TEST(test_demo_order_default);
    RUN_TEST(test_demo_order_save_load);

    // Effects — initial/empty state
    RUN_TEST(test_effect_count_initial);
    RUN_TEST(test_get_effect_empty);
    RUN_TEST(test_get_effect_negative_index);
    RUN_TEST(test_get_effect_null_pointer);
    RUN_TEST(test_get_effect_by_name_null_name);
    RUN_TEST(test_get_effect_by_name_null_effect);
    RUN_TEST(test_get_effect_by_name_not_found);
    RUN_TEST(test_get_effects_by_category_empty);
    RUN_TEST(test_get_effects_by_category_null_safe);

    // begin() integration
    RUN_TEST(test_begin_without_sd);
    RUN_TEST(test_begin_with_mutex);

    // Preferences-backed operations
    RUN_TEST(test_wifi_ssid_save_load);
    RUN_TEST(test_wifi_ssid_default);
    RUN_TEST(test_wifi_password_save_load);
    RUN_TEST(test_has_settings_empty);
    RUN_TEST(test_has_settings_after_wifi);
    RUN_TEST(test_clear_all);

    // MQTT config
    RUN_TEST(test_mqtt_config_defaults);
    RUN_TEST(test_mqtt_config_null_pointer);
    RUN_TEST(test_mqtt_config_save_load);

    return UNITY_END();
}
