/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_twai.h"
#include "esp_twai_onchip.h"

#define TWAI_SENDER_TX_GPIO     CONFIG_EXAMPLE_TWAI_TX_GPIO
#define TWAI_SENDER_RX_GPIO     CONFIG_EXAMPLE_TWAI_RX_GPIO
#define TWAI_QUEUE_DEPTH        10
#define TWAI_BITRATE            500000

// Message IDs
#define TWAI_DATA_ID            0x100 // (must match sender)
#define TWAI_HEARTBEAT_ID       0x7FF
#define TWAI_DATA_LEN           1000

// Buffer for burst data handling
#define POLL_DEPTH              200 // from twai_listen_only

static const char *TAG_SEND = "twai_sender";
static const char *TAG_LISTEN = "twai_listen"; // from twai_listen_only

typedef struct {
    twai_frame_t frame;
    uint8_t data[TWAI_FRAME_MAX_LEN];
} twai_sender_data_t;

typedef struct {
    twai_frame_t frame;
    uint8_t data[TWAI_FRAME_MAX_LEN];
} twai_listener_data_t;

// from twai_listen_only
typedef struct {
    twai_node_handle_t node_hdl;
    twai_listener_data_t *rx_pool;
    SemaphoreHandle_t free_pool_semaphore; // free_pool_semaphore = available buffer slots (ISR uses one and then RX gives it back after leaving)
    SemaphoreHandle_t rx_result_semaphore; // rx_result_semaphore = number of received frames (ISR notifies frame is here, RX then takes it)
    int write_idx;
    int read_idx;
} twai_listener_ctx_t;

// Shared handle for the single TWAI node
twai_node_handle_t twai_node = NULL;
twai_listener_ctx_t listener_ctx = {0};

// Node state
static bool IRAM_ATTR twai_listener_on_state_change_callback(twai_node_handle_t handle, const twai_state_change_event_data_t *edata, void *user_ctx)
{
    const char *twai_state_name[] = {"error_active", "error_warning", "error_passive", "bus_off"};
    ESP_EARLY_LOGI(TAG_LISTEN, "state changed: %s -> %s", twai_state_name[edata->old_sta], twai_state_name[edata->new_sta]);
    return false;
}

// TWAI receive callback - store data and signal
static bool IRAM_ATTR twai_listener_rx_callback(twai_node_handle_t handle, const twai_rx_done_event_data_t *edata, void *user_ctx)
{
    BaseType_t woken;
    twai_listener_ctx_t *ctx = (twai_listener_ctx_t *)user_ctx;

    if (xSemaphoreTakeFromISR(ctx->free_pool_semaphore, &woken) != pdTRUE) {
        ESP_EARLY_LOGI(TAG_LISTEN, "Pool full, dropping frame");
        return (woken == pdTRUE);
    }
    if (twai_node_receive_from_isr(handle, &ctx->rx_pool[ctx->write_idx].frame) == ESP_OK) {
        ctx->write_idx = (ctx->write_idx + 1) % POLL_DEPTH; // circular buffer design
        xSemaphoreGiveFromISR(ctx->rx_result_semaphore, &woken);
    }
    return (woken == pdTRUE);
}

// Transmission completion callback
static bool IRAM_ATTR twai_sender_tx_done_callback(twai_node_handle_t handle, const twai_tx_done_event_data_t *edata, void *user_ctx)
{
    if (!edata->is_tx_success) {
        ESP_EARLY_LOGW(TAG_SEND, "Failed to transmit message, ID: 0x%X", edata->done_tx_frame->header.id);
    }
    return false; // No task wake required
}

// Bus error callback
static bool IRAM_ATTR twai_on_error_callback(twai_node_handle_t handle, const twai_error_event_data_t *edata, void *user_ctx)
{
    ESP_EARLY_LOGW(TAG_SEND, "TWAI node error: 0x%x", edata->err_flags.val);
    
    return false; // No task wake required
}

// Separating tx and rx for free RTOS tasks

void tx_task(void *pvParameters) {
    while (1) {
        uint64_t timestamp = esp_timer_get_time();
        twai_frame_t tx_frame = {
            .header.id = TWAI_HEARTBEAT_ID,
            .buffer = (uint8_t *) &timestamp,
            .buffer_len = sizeof(timestamp),
        };
        
        // Transmit using the shared node handle
        if (twai_node_transmit(twai_node, &tx_frame, pdMS_TO_TICKS(500)) == ESP_OK) {
            ESP_LOGI("TX_TASK", "Heartbeat sent: %lld", timestamp);
        }

        // Logic for burst data (every 10s) goes here...

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void rx_task(void *pvParameters) {
    while (1) {
        // Wait for the callback to signal a new message
        if (xSemaphoreTake(listener_ctx.rx_result_semaphore, portMAX_DELAY) == pdTRUE) {
            twai_frame_t *frame = &listener_ctx.rx_pool[listener_ctx.read_idx].frame;
            
            ESP_LOGI("RX_TASK", "Received ID: 0x%03X", frame->header.id);
            
            listener_ctx.read_idx = (listener_ctx.read_idx + 1) % POLL_DEPTH;
            xSemaphoreGive(listener_ctx.free_pool_semaphore);
        }
    }
}

void app_main(void) {
    // 1. Initialize Semaphores and Buffers for Listener

    // RX pool & buffer setup
    listener_ctx.free_pool_semaphore = xSemaphoreCreateCounting(POLL_DEPTH, POLL_DEPTH);
    listener_ctx.rx_result_semaphore = xSemaphoreCreateCounting(POLL_DEPTH, 0);
    assert(listener_ctx.free_pool_semaphore != NULL);
    assert(listener_ctx.rx_result_semaphore != NULL);

    listener_ctx.rx_pool = calloc(POLL_DEPTH, sizeof(twai_listener_data_t));
    assert(listener_ctx.rx_pool != NULL);
    for (int i = 0; i < POLL_DEPTH; i++) {
        listener_ctx.rx_pool[i].frame.buffer = listener_ctx.rx_pool[i].data;
        listener_ctx.rx_pool[i].frame.buffer_len = sizeof(listener_ctx.rx_pool[i].data);
    }
    ESP_LOGI(TAG_LISTEN, "Buffer initialized: %d slots for burst data", POLL_DEPTH);

    // 2. Single Node Configuration
    twai_onchip_node_config_t node_config = {
        .io_cfg = { .tx = TWAI_SENDER_TX_GPIO, .rx = TWAI_SENDER_RX_GPIO },
        .bit_timing = { .bitrate = TWAI_BITRATE },
        .fail_retry_cnt = 3,
        .tx_queue_depth = TWAI_QUEUE_DEPTH,
        .timestamp_resolution_hz = 1000000,
    };

    // 3. Create ONE node
    ESP_ERROR_CHECK(twai_new_node_onchip(&node_config, &twai_node));

    // 4. Set Acceptance Filter (if needed)
    twai_mask_filter_config_t data_filter = {
        .id = TWAI_DATA_ID, 
        .mask = 0x000, // accept all masks
        .is_ext = false,
    };
    ESP_ERROR_CHECK(twai_node_config_mask_filter(twai_node, 0, &data_filter));

    // 5. Register ALL callbacks in one struct
    twai_event_callbacks_t callbacks = {
        .on_tx_done = twai_sender_tx_done_callback,
        .on_rx_done = twai_listener_rx_callback, // Listener callback
        .on_error = twai_on_error_callback,
        .on_state_change = twai_listener_on_state_change_callback,
    };
    ESP_ERROR_CHECK(twai_node_register_event_callbacks(twai_node, &callbacks, &listener_ctx));

    // 6. Start the Node
    ESP_ERROR_CHECK(twai_node_enable(twai_node));

    // 7. Launch Tasks
    xTaskCreate(tx_task, "tx_task", 4096, NULL, 5, NULL);
    xTaskCreate(rx_task, "rx_task", 4096, NULL, 5, NULL);
}
