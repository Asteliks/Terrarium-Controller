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
#include "esp_log.h"
#include <esp_rmaker_core.h>
#include <esp_rmaker_standard_types.h>
#include <esp_rmaker_standard_params.h>

#include <app_reset.h>
#include "driver/rmt.h"
#include "led_strip.h"
#include "app_priv.h"

#include <dht.h>

static const char *TAG = "app_driver";

/* DHT sensor definition */
static const dht_sensor_type_t sensor_type = DHT_TYPE_AM2301;
static const gpio_num_t dht1_gpio = 17;
static const gpio_num_t dht2_gpio = 18;

/* LED setup */
#define RMT_TX_CHANNEL RMT_CHANNEL_0
static uint16_t g_hue = DEFAULT_HUE;
static uint16_t g_saturation = DEFAULT_SATURATION;
static uint16_t g_value = DEFAULT_BRIGHTNESS;
static bool g_power = DEFAULT_POWER;

uint32_t red = 0;
uint32_t green = 0;
uint32_t blue = 0;
uint16_t hue = 0;
uint16_t start_rgb = 0;

rmt_config_t config;
led_strip_config_t strip_config;
led_strip_t *strip;

/* This is the button that is used for toggling the power */
#define BUTTON_GPIO          CONFIG_EXAMPLE_BOARD_BUTTON_GPIO
#define BUTTON_ACTIVE_LEVEL  0
/* This is the GPIO on which the power will be set */
#define OUTPUT_GPIO    19

#define WIFI_RESET_BUTTON_TIMEOUT       3
#define FACTORY_RESET_BUTTON_TIMEOUT    10

static bool g_power_state = DEFAULT_SWITCH_POWER;
static float g_temperature1 = DEFAULT_TEMPERATURE, g_temperature2 = DEFAULT_TEMPERATURE;
static float g_humidity1 = DEFAULT_HUMIDITY, g_humidity2 = DEFAULT_HUMIDITY;
static TimerHandle_t sensor_timer;

void led_strip_hsv2rgb(uint32_t h, uint32_t s, uint32_t v, uint32_t *r, uint32_t *g, uint32_t *b)
{
    h %= 360; // h -> [0,360]
    uint32_t rgb_max = v * 2.55f;
    uint32_t rgb_min = rgb_max * (100 - s) / 100.0f;

    uint32_t i = h / 60;
    uint32_t diff = h % 60;

    // RGB adjustment amount by hue
    uint32_t rgb_adj = (rgb_max - rgb_min) * diff / 60;

    switch (i) {
    case 0:
        *r = rgb_max;
        *g = rgb_min + rgb_adj;
        *b = rgb_min;
        break;
    case 1:
        *r = rgb_max - rgb_adj;
        *g = rgb_max;
        *b = rgb_min;
        break;
    case 2:
        *r = rgb_min;
        *g = rgb_max;
        *b = rgb_min + rgb_adj;
        break;
    case 3:
        *r = rgb_min;
        *g = rgb_max - rgb_adj;
        *b = rgb_max;
        break;
    case 4:
        *r = rgb_min + rgb_adj;
        *g = rgb_min;
        *b = rgb_max;
        break;
    default:
        *r = rgb_max;
        *g = rgb_min;
        *b = rgb_max - rgb_adj;
        break;
    }
}

esp_err_t app_light_init(void)
{
    rmt_config_t l_config = RMT_DEFAULT_CONFIG_TX(CONFIG_EXAMPLE_RMT_TX_GPIO, RMT_TX_CHANNEL);
    // set counter clock to 40MHz
    l_config.clk_div = 2;

    ESP_ERROR_CHECK(rmt_config(&l_config));
    ESP_ERROR_CHECK(rmt_driver_install(l_config.channel, 0, 0));

    // install ws2812 driver
    led_strip_config_t l_strip_config = LED_STRIP_DEFAULT_CONFIG(CONFIG_EXAMPLE_STRIP_LED_NUMBER, (led_strip_dev_t)l_config.channel);
    led_strip_t *l_strip = led_strip_new_rmt_ws2812(&l_strip_config);
    config=l_config;
    strip_config=l_strip_config;
    strip=l_strip;
    if (!strip) {
        ESP_LOGE(TAG, "install WS2812 driver failed");
        return ESP_FAIL;
    }
    // Clear LED strip (turn off all LEDs)
    ESP_ERROR_CHECK(strip->clear(strip, 100));
    return ESP_OK;
}

esp_err_t led_set_hsv(uint32_t hue, uint32_t saturation, uint32_t brightness)
{
    // Show simple rainbow chasing pattern
    ESP_LOGI(TAG, "Setting up LEDs");
    for (int i = 0; i < CONFIG_EXAMPLE_STRIP_LED_NUMBER; i++) {
        // Build RGB values
        // hue = i * 360 / CONFIG_EXAMPLE_STRIP_LED_NUMBER + start_rgb;
        led_strip_hsv2rgb(hue, saturation, brightness, &red, &green, &blue);
        // Write RGB values to strip driver
        ESP_ERROR_CHECK(strip->set_pixel(strip, i, red, green, blue));
    }
    // Flush RGB values to LEDs
    ESP_ERROR_CHECK(strip->refresh(strip, 100));

    return ESP_OK;
}

esp_err_t app_light_set_led(uint32_t hue, uint32_t saturation, uint32_t brightness)
{
    /* Whenever this function is called, light power will be ON */
    if (!g_power) {
        g_power = true;
        esp_rmaker_param_update_and_report(
                esp_rmaker_device_get_param_by_type(light_device, ESP_RMAKER_PARAM_POWER),
                esp_rmaker_bool(g_power));
    }
    return led_set_hsv(hue, saturation, brightness);
}

esp_err_t led_clear(void){
    ESP_ERROR_CHECK(strip->clear(strip, 100));
    return ESP_OK;
}

esp_err_t app_light_set_power(bool power)
{
    g_power = power;
    if (power) {
        led_set_hsv(g_hue, g_saturation, g_value);
    } else {
        led_clear();
    }
    return ESP_OK;
}

esp_err_t app_light_set_brightness(uint16_t brightness)
{
    g_value = brightness;
    return app_light_set_led(g_hue, g_saturation, g_value);
}
esp_err_t app_light_set_hue(uint16_t hue)
{
    g_hue = hue;
    return app_light_set_led(g_hue, g_saturation, g_value);
}
esp_err_t app_light_set_saturation(uint16_t saturation)
{
    g_saturation = saturation;
    return app_light_set_led(g_hue, g_saturation, g_value);
}


static void app_sensor_update(void *priv)
{
    if (dht_read_float_data(sensor_type, dht1_gpio, &g_humidity1, &g_temperature1) == ESP_OK)
        printf("Humidity: %f%% Temp: %fC\n", g_humidity1 / 10, g_temperature1 / 10);
    else
        printf("Could not read data from sensor\n");

    if (dht_read_float_data(sensor_type, dht2_gpio, &g_humidity2, &g_temperature2) == ESP_OK)
        printf("Humidity: %f%% Temp: %fC\n", g_humidity2 / 10, g_temperature2 / 10);
    else
        printf("Could not read data from sensor\n");

    esp_rmaker_param_update_and_report(
                esp_rmaker_device_get_param_by_type(temp_sensor_device1, ESP_RMAKER_PARAM_TEMPERATURE),
                esp_rmaker_float(g_temperature1));
    esp_rmaker_param_update_and_report(
                esp_rmaker_device_get_param_by_type(temp_sensor_device2, ESP_RMAKER_PARAM_TEMPERATURE),
                esp_rmaker_float(g_temperature2));
    esp_rmaker_param_update_and_report(
                esp_rmaker_device_get_param_by_type(humi_sensor_device1, ESP_RMAKER_PARAM_TEMPERATURE),
                esp_rmaker_float(g_humidity1));
    esp_rmaker_param_update_and_report(
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

static void push_btn_cb(void *arg)
{
    bool new_state = !g_power_state;
    app_driver_set_state(new_state);
    esp_rmaker_param_update_and_report(
                esp_rmaker_device_get_param_by_type(switch_device, ESP_RMAKER_PARAM_POWER),
                esp_rmaker_bool(new_state));
}

void app_driver_init()
{
    app_light_init();
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
    app_sensor_init();
}

static void set_power_state(bool target)
{
    gpio_set_level(OUTPUT_GPIO, !target);
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
