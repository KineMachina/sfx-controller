#include <unity.h>
#include "AudioController.h"

static Audio audioObj;
static AudioController* controller = nullptr;

void setUp(void) {
    // Configure SD mock for successful begin()
    SD.setBeginResult(true);
    SD.setFileContent("dummy");
    audioObj.reset();
    controller = new AudioController(&audioObj, 7, 15, 16, 10, 11, 13, 12);
}

void tearDown(void) {
    delete controller;
    controller = nullptr;
    SD.setBeginResult(false);
    SD.setFileContent(nullptr);
}

// ---------------------------------------------------------------------------
// Constructor defaults (pre-begin)
// ---------------------------------------------------------------------------

void test_default_volume(void) {
    TEST_ASSERT_EQUAL_INT(21, controller->getVolume());
}

void test_default_current_file(void) {
    TEST_ASSERT_EQUAL_STRING("/sounds/roar.mp3",
                             controller->getCurrentFile().c_str());
}

// ---------------------------------------------------------------------------
// Pre-begin guards (queues not initialized)
// ---------------------------------------------------------------------------

void test_play_file_returns_false_before_begin(void) {
    TEST_ASSERT_FALSE(controller->playFile("/sounds/test.mp3"));
}

void test_stop_safe_before_begin(void) {
    controller->stop();  // Should not crash
    TEST_ASSERT_TRUE(true);
}

void test_set_volume_safe_before_begin(void) {
    controller->setVolume(10);  // Should not crash
    TEST_ASSERT_TRUE(true);
}

void test_is_playing_false_before_begin(void) {
    // Falls back to audio->isRunning() when queue is NULL
    TEST_ASSERT_FALSE(controller->isPlaying());
}

void test_file_exists_false_before_begin(void) {
    // sdCardMutex is NULL before begin()
    TEST_ASSERT_FALSE(controller->fileExists("/sounds/test.mp3"));
}

// ---------------------------------------------------------------------------
// Volume range validation (checked before queue, so works pre-begin)
// ---------------------------------------------------------------------------

void test_set_volume_rejects_negative(void) {
    controller->setVolume(-1);
    TEST_ASSERT_EQUAL_INT(21, controller->getVolume());
}

void test_set_volume_rejects_above_max(void) {
    controller->setVolume(22);
    TEST_ASSERT_EQUAL_INT(21, controller->getVolume());
}

// ---------------------------------------------------------------------------
// begin() initialization
// ---------------------------------------------------------------------------

void test_begin_succeeds(void) {
    TEST_ASSERT_TRUE(controller->begin());
}

void test_begin_fails_when_sd_fails(void) {
    SD.setBeginResult(false);
    TEST_ASSERT_FALSE(controller->begin());
}

// ---------------------------------------------------------------------------
// Post-begin: playFile path handling
// ---------------------------------------------------------------------------

void test_play_file_returns_true(void) {
    controller->begin();
    TEST_ASSERT_TRUE(controller->playFile("/sounds/test.mp3"));
}

void test_play_file_normalizes_path(void) {
    controller->begin();
    controller->playFile("sounds/test.mp3");
    TEST_ASSERT_EQUAL_STRING("/sounds/test.mp3",
                             controller->getCurrentFile().c_str());
}

void test_play_file_preserves_leading_slash(void) {
    controller->begin();
    controller->playFile("/sounds/test.mp3");
    TEST_ASSERT_EQUAL_STRING("/sounds/test.mp3",
                             controller->getCurrentFile().c_str());
}

void test_play_file_trims_whitespace(void) {
    controller->begin();
    controller->playFile("  /sounds/test.mp3  ");
    TEST_ASSERT_EQUAL_STRING("/sounds/test.mp3",
                             controller->getCurrentFile().c_str());
}

void test_play_file_empty_returns_false(void) {
    controller->begin();
    TEST_ASSERT_FALSE(controller->playFile(""));
}

void test_play_file_whitespace_only_returns_false(void) {
    controller->begin();
    TEST_ASSERT_FALSE(controller->playFile("   "));
}

void test_play_file_updates_current_file(void) {
    controller->begin();
    controller->playFile("/sounds/new.mp3");
    TEST_ASSERT_EQUAL_STRING("/sounds/new.mp3",
                             controller->getCurrentFile().c_str());
}

// ---------------------------------------------------------------------------
// Post-begin: fileExists
// ---------------------------------------------------------------------------

void test_file_exists_with_content(void) {
    controller->begin();
    // SD mock has file content set, so exists() returns true
    TEST_ASSERT_TRUE(controller->fileExists("/sounds/test.mp3"));
}

void test_file_exists_without_content(void) {
    controller->begin();
    SD.setFileContent(nullptr);
    TEST_ASSERT_FALSE(controller->fileExists("/sounds/test.mp3"));
}

// ---------------------------------------------------------------------------
// Post-begin: isPlaying
// ---------------------------------------------------------------------------

void test_is_playing_false_after_begin(void) {
    controller->begin();
    TEST_ASSERT_FALSE(controller->isPlaying());
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    UNITY_BEGIN();

    // Constructor defaults
    RUN_TEST(test_default_volume);
    RUN_TEST(test_default_current_file);

    // Pre-begin guards
    RUN_TEST(test_play_file_returns_false_before_begin);
    RUN_TEST(test_stop_safe_before_begin);
    RUN_TEST(test_set_volume_safe_before_begin);
    RUN_TEST(test_is_playing_false_before_begin);
    RUN_TEST(test_file_exists_false_before_begin);

    // Volume validation
    RUN_TEST(test_set_volume_rejects_negative);
    RUN_TEST(test_set_volume_rejects_above_max);

    // begin()
    RUN_TEST(test_begin_succeeds);
    RUN_TEST(test_begin_fails_when_sd_fails);

    // playFile path handling
    RUN_TEST(test_play_file_returns_true);
    RUN_TEST(test_play_file_normalizes_path);
    RUN_TEST(test_play_file_preserves_leading_slash);
    RUN_TEST(test_play_file_trims_whitespace);
    RUN_TEST(test_play_file_empty_returns_false);
    RUN_TEST(test_play_file_whitespace_only_returns_false);
    RUN_TEST(test_play_file_updates_current_file);

    // fileExists
    RUN_TEST(test_file_exists_with_content);
    RUN_TEST(test_file_exists_without_content);

    // isPlaying
    RUN_TEST(test_is_playing_false_after_begin);

    return UNITY_END();
}
