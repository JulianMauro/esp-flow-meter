/*
 * Copyright (c) 2025 Autoplants - All Rights Reserved.
 * Autoplants gratefully acknowledges that this software was originally authored
 * and developed by Julian Mauro (Julian.m@autoplants.com.ar).
 */

#ifndef FLOW_METER_H
#define FLOW_METER_H
#pragma once
 
#include "driver/pulse_cnt.h"

typedef struct {
    pcnt_unit_handle_t *unit;
    pcnt_channel_handle_t *channel;
} *flow_meter_handle_t;

typedef struct {
    uint8_t pin;
    double required_liters;
    QueueHandle_t cb_queue;
} flow_meter_config;

flow_meter_handle_t flow_meter_init(flow_meter_config *config);
double flow_meter_get_liters(flow_meter_handle_t handle);
double flow_meter_get_rate(flow_meter_handle_t handle, bool *no_flow_detected);
esp_err_t flow_meter_deinit(flow_meter_handle_t handle);
 
#endif // FLOW_METER_H
