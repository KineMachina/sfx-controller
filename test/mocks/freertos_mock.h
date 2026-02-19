#ifndef FREERTOS_MOCK_H
#define FREERTOS_MOCK_H

#include <stdint.h>
#include <stddef.h>

// FreeRTOS type stubs
typedef void* SemaphoreHandle_t;
typedef void* QueueHandle_t;
typedef void* TaskHandle_t;
typedef int BaseType_t;
typedef uint32_t TickType_t;

#define pdTRUE  1
#define pdFALSE 0
#define pdPASS  pdTRUE
#define pdFAIL  pdFALSE
#define NULL_HANDLE ((void*)0)

#define portMAX_DELAY 0xFFFFFFFF

// Convert milliseconds to ticks (identity in mock)
#define pdMS_TO_TICKS(xTimeInMs) ((TickType_t)(xTimeInMs))

#ifdef __cplusplus

// Semaphore stubs — always succeed
inline SemaphoreHandle_t xSemaphoreCreateMutex() { return (SemaphoreHandle_t)1; }
inline BaseType_t xSemaphoreTake(SemaphoreHandle_t, TickType_t) { return pdTRUE; }
inline BaseType_t xSemaphoreGive(SemaphoreHandle_t) { return pdTRUE; }

// Queue stubs
inline QueueHandle_t xQueueCreate(unsigned int, unsigned int) { return (QueueHandle_t)1; }
inline BaseType_t xQueueSend(QueueHandle_t, const void*, TickType_t) { return pdTRUE; }
inline BaseType_t xQueueSendToFront(QueueHandle_t, const void*, TickType_t) { return pdTRUE; }
inline BaseType_t xQueueReceive(QueueHandle_t, void*, TickType_t) { return pdFALSE; }
inline unsigned int uxQueueMessagesWaiting(QueueHandle_t) { return 0; }

// Task stubs
inline BaseType_t xTaskCreatePinnedToCore(void (*)(void*), const char*, uint32_t, void*, int, TaskHandle_t*, int) { return pdPASS; }
inline TickType_t xTaskGetTickCount() { return 0; }
inline void vTaskDelay(TickType_t) {}

#endif // __cplusplus

#endif // FREERTOS_MOCK_H
