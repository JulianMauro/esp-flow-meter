/*
 * Copyright (c) 2025 Autoplants - All Rights Reserved.
 * Autoplants gratefully acknowledges that this software was originally authored
 * and developed by Julian Mauro (Julian.m@autoplants.com.ar).
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include "flow_meter.h"

#define PCNT_UNIT_PRIORITY (3)

#define LITERS_PER_COUNT ((double) 1./(7.5*60))
// YF-S201
// Freq[Hz] = 7.5 * flux[L/min]

#define LITERS_TO_PULSES(liters) (liters / LITERS_PER_COUNT)

const char* TAG = "[flow_meter]";

flow_meter_handle_t flow_meter_init(flow_meter_config *config);
double flow_meter_get_liters(flow_meter_handle_t handle);
double flow_meter_get_rate(flow_meter_handle_t handle, bool *no_flow_detected);
esp_err_t flow_meter_deinit(flow_meter_handle_t handle);

static IRAM_ATTR bool on_pcnt_reach(pcnt_unit_handle_t unit, const pcnt_watch_event_data_t *edata, void *user_ctx);

bool on_pcnt_reach(pcnt_unit_handle_t unit, const pcnt_watch_event_data_t *edata, void *user_ctx)
{
    BaseType_t high_task_wakeup;
    QueueHandle_t queue = (QueueHandle_t)user_ctx;
    // send watch point to queue, from this interrupt callback
    xQueueSendFromISR(queue, &(edata->watch_point_value), &high_task_wakeup);
    // return whether a high priority task has been waken up by this function
    return (high_task_wakeup == pdTRUE);
}

flow_meter_handle_t flow_meter_init(flow_meter_config *config)
{
    esp_err_t err;
    flow_meter_handle_t handle;

    pcnt_unit_config_t pcnt_config = 
    {
        .high_limit = INT_MAX,
        .low_limit = -1,
        .intr_priority = PCNT_UNIT_PRIORITY,
    };

    err = pcnt_new_unit(&pcnt_config, &handle->unit);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Cannot create unit for flow meter");
        return NULL;
    }

    pcnt_chan_config_t pcnt_chan_config = {
        .level_gpio_num = -1,
        .edge_gpio_num = config->pin,
    };

    err = pcnt_new_channel(handle->unit, &pcnt_chan_config, &handle->channel);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Cannot create channel for flow meter");
        return NULL;
    }

    // increase the counter on rising edge, hold the counter on falling edge
    err = pcnt_channel_set_edge_action(handle->channel, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_HOLD);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Cannot set edge action for flow meter");
        return NULL;
    }

    ESP_ERROR_CHECK(pcnt_unit_add_watch_point(handle->unit, 0));
    ESP_ERROR_CHECK(pcnt_unit_add_watch_point(handle->unit, LITERS_TO_PULSES(config->required_liters)));
    ESP_ERROR_CHECK(pcnt_unit_clear_count(handle->unit));

    pcnt_event_callbacks_t cbs = {
        .on_reach = on_pcnt_reach,
    };

    config->cb_queue = xQueueCreate(10, sizeof(int));

    ESP_ERROR_CHECK(pcnt_unit_register_event_callbacks(handle->unit, &cbs, config->cb_queue));

    ESP_ERROR_CHECK(pcnt_unit_enable(handle->unit));
    ESP_ERROR_CHECK(pcnt_unit_start(handle->unit));

    return handle;
}

double flow_meter_get_liters(flow_meter_handle_t handle)
{
    uint64_t cnt;
    esp_err_t err = pcnt_unit_get_count(handle->unit, &cnt);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Cannot get count for flow meter");
        return -1.0;
    }
    
    return LITERS_PER_COUNT * cnt;
}

// This function is blocking!
double flow_meter_get_rate(flow_meter_handle_t handle, bool *no_flow_detected)
{
    esp_err_t err;
    uint64_t first_cnt, last_cnt;
    
    err = pcnt_unit_get_count(handle->unit, &first_cnt);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Cannot get count for flow meter");
        return -1.0;
    }

    vTaskDelay(pdMS_TO_TICKS(60000)); // Wait for a minute

    err = pcnt_unit_get_count(handle->unit, &last_cnt);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Cannot get count for flow meter");
        return -1.0;
    }

    if ((last_cnt - first_cnt) == 0)
    {
        no_flow_detected = true;
    }

    return LITERS_PER_COUNT * (last_cnt - first_cnt);
}

esp_err_t flow_meter_deinit(flow_meter_handle_t handle)
{
    pcnt_unit_stop(handle->unit);
    pcnt_unit_clear_count(handle->unit);
    pcnt_unit_disable(handle->unit);
    pcnt_del_channel(handle->channel);
    return pcnt_del_unit(handle->unit);
}
