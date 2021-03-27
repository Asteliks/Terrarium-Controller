/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <sdkconfig.h>
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>

#include <iot_button.h>
#include <esp_rmaker_core.h>
#include <esp_rmaker_standard_types.h>
#include <esp_rmaker_standard_params.h>

#include <app_reset.h>
#include <ws2812_led.h>
#include "app_priv.h"

#include <dht.h>

/* DHT sensor definition */
static const dht_sensor_type_t sensor_type = DHT_TYPE_AM2301;
static const gpio_num_t dht1_gpio = 17;
static const gpio_num_t dht2_gpio = 18;

/* This is the button that is used for toggling the power */
#define BUTTON_GPIO          CONFIG_EXAMPLE_BOARD_BUTTON_GPIO
#define BUTTON_ACTIVE_LEVEL  0
/* This is the GPIO on which the power will be set */
#define OUTPUT_GPIO    19

/* These values correspoind to H,S,V = 120,100,10 */
#define DEFAULT_RED     0
#define DEFAULT_GREEN   25
#define DEFAULT_BLUE    0

#define WIFI_RESET_BUTTON_TIMEOUT       3
#define FACTORY_RESET_BUTTON_TIMEOUT    10

static bool g_power_state = DEFAULT_SWITCH_POWER;
static float g_temperature1 = DEFAULT_TEMPERATURE, g_temperature2 = DEFAULT_TEMPERATURE;
static float g_humidity1 = DEFAULT_HUMIDITY, g_humidity2 = DEFAULT_HUMIDITY;
static TimerHandle_t sensor_timer;

static void app_sensor_update(void *priv)
{
    if (dht_read_data(sensor_type, dht1_gpio, &g_humidity1, &g_temperature1) == ESP_OK)
        printf("Humidity: %d%% Temp: %dC\n", g_humidity1 / 10, g_temperature1 / 10);
    else
        printf("Could not read data from sensor\n");

    if (dht_read_data(sensor_type, dht2_gpio, &g_humidity2, &g_temperature2) == ESP_OK)
        printf("Humidity: %d%% Temp: %dC\n", g_humidity2 / 10, g_temperature2 / 10);
    else
        printf("Could not read data from sensor\n");

    esp_rmaker_param_update_and_report(
                esp_rmaker_device_get_param_by_type(temp_sensor_device1, ESP_RMAKER_PARAM_TEMPERATURE),
                esp_rmaker_float(g_temperature1));
                esp_rmaker_device_get_param_by_type(temp_sensor_device2, ESP_RMAKER_PARAM_TEMPERATURE),
                esp_rmaker_float(g_temperature2));
                esp_rmaker_device_get_param_by_type(humi_sensor_device1, ESP_RMAKER_PARAM_TEMPERATURE),
                esp_rmaker_float(g_humidity1));
                esp_rmaker_device_get_param_by_type(humi_sensor_device2, ESP_RMAKER_PARAM_TEMPERATURE),
                esp_rmaker_float(g_humidity2));
}

float app_get_current_temperature(int thermometer)
{
    switch (thermometer)
    {
    case 1:
        return g_temperature1;
        break;

    case 2:
        return g_temperature2;
        break;
    }
    return 0;
}

float app_get_current_humidity(int hygrometer)
{
    switch (hygrometer)
    {
    case 1:
        return g_humidity1;
        break;

    case 2:
        return g_humidity2;
        break;
    }
    return 0;
}

esp_err_t app_sensor_init(void)
{
    g_temperature1 = DEFAULT_TEMPERATURE;
    sensor_timer = xTimerCreate("app_sensor_update_tm", (REPORTING_PERIOD * 1000) / portTICK_PERIOD_MS,
                            pdTRUE, NULL, app_sensor_update);
    if (sensor_timer) {
        xTimerStart(sensor_timer, 0);
        return ESP_OK;
    }
    return ESP_FAIL;
}

static void app_indicator_set(bool state)
{
    if (state) {
        ws2812_led_set_rgb(DEFAULT_RED, DEFAULT_GREEN, DEFAULT_BLUE);
    } else {
        ws2812_led_clear();
    }
}

static void app_indicator_init(void)
{
    ws2812_led_init();
    app_indicator_set(g_power_state);
}

static void push_btn_cb(void *arg)
{
    bool new_state = !g_power_state;
    app_driver_set_state(new_state);
    esp_rmaker_param_update_and_report(
                esp_rmaker_device_get_param_by_type(switch_device, ESP_RMAKER_PARAM_POWER),
                esp_rmaker_bool(new_state));
}

static void set_power_state(bool target)
{
    gpio_set_level(OUTPUT_GPIO, target);
    app_indicator_set(target);
}

void app_driver_init()
{
    button_handle_t btn_handle = iot_button_create(BUTTON_GPIO, BUTTON_ACTIVE_LEVEL);
    if (btn_handle) {
        /* Register a callback for a button tap (short press) event */
        iot_button_set_evt_cb(btn_handle, BUTTON_CB_TAP, push_btn_cb, NULL);
        /* Register Wi-Fi reset and factory reset functionality on same button */
        app_reset_button_register(btn_handle, WIFI_RESET_BUTTON_TIMEOUT, FACTORY_RESET_BUTTON_TIMEOUT);
    }

    /* Configure power */
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = 1,
    };
    io_conf.pin_bit_mask = ((uint64_t)1 << OUTPUT_GPIO);
    /* Configure the GPIO */
    gpio_config(&io_conf);
    app_indicator_init();
    app_sensor_init();
}

int IRAM_ATTR app_driver_set_state(bool state)
{
    if(g_power_state != state) {
        g_power_state = state;
        set_power_state(g_power_state);
    }
    return ESP_OK;
}

bool app_driver_get_state(void)
{
    return g_power_state;
}