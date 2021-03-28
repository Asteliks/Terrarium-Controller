/* Multi-Device Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <nvs_flash.h>

#include <esp_rmaker_core.h>
#include <esp_rmaker_standard_params.h>
#include <esp_rmaker_standard_devices.h>
#include <esp_rmaker_schedule.h>

#include <app_wifi.h>

#include "app_priv.h"

static const char *TAG = "app_main";

esp_rmaker_device_t *temp_sensor_device1;
esp_rmaker_device_t *temp_sensor_device2;
esp_rmaker_device_t *humi_sensor_device1;
esp_rmaker_device_t *humi_sensor_device2;

esp_rmaker_device_t *switch_device;
esp_rmaker_device_t *light_device;
esp_rmaker_device_t *temp_sensor_device;

/* Callback to handle commands received from the RainMaker cloud */
static esp_err_t write_cb(const esp_rmaker_device_t *device, const esp_rmaker_param_t *param,
            const esp_rmaker_param_val_t val, void *priv_data, esp_rmaker_write_ctx_t *ctx)
{
    if (ctx) {
        ESP_LOGI(TAG, "Received write request via : %s", esp_rmaker_device_cb_src_to_str(ctx->src));
    }
    const char *device_name = esp_rmaker_device_get_name(device);
    const char *param_name = esp_rmaker_param_get_name(param);
    if (strcmp(device_name, "Switch") == 0) {
            app_driver_set_state(val.val.b);
    } else if (strcmp(param_name, ESP_RMAKER_DEF_POWER_NAME) == 0) {
        ESP_LOGI(TAG, "Received value = %s for %s - %s",
                val.val.b? "true" : "false", device_name, param_name);
        app_light_set_power(val.val.b);
    } else if (strcmp(param_name, ESP_RMAKER_DEF_BRIGHTNESS_NAME) == 0) {
        ESP_LOGI(TAG, "Received value = %d for %s - %s",
                val.val.i, device_name, param_name);
        app_light_set_brightness(val.val.i);
    } else if (strcmp(param_name, ESP_RMAKER_DEF_HUE_NAME) == 0) {
        ESP_LOGI(TAG, "Received value = %d for %s - %s",
                val.val.i, device_name, param_name);
        app_light_set_hue(val.val.i);
    } else if (strcmp(param_name, ESP_RMAKER_DEF_SATURATION_NAME) == 0) {
        ESP_LOGI(TAG, "Received value = %d for %s - %s",
                val.val.i, device_name, param_name);
        app_light_set_saturation(val.val.i);
    } else {
        /* Silently ignoring invalid params */
        return ESP_OK;
    }
    esp_rmaker_param_update_and_report(param, val);
    return ESP_OK;
}

void app_main()
{
    /* Initialize Application specific hardware drivers and
     * set initial state.
     */
    app_driver_init();
    app_driver_set_state(DEFAULT_SWITCH_POWER);

    /* Initialize NVS. */
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );

    /* Initialize Wi-Fi. Note that, this should be called before esp_rmaker_init()
     */
    app_wifi_init();
    
    /* Initialize the ESP RainMaker Agent.
     * Note that this should be called after app_wifi_init() but before app_wifi_start()
     * */
    esp_rmaker_config_t rainmaker_cfg = {
        .enable_time_sync = false,
    };
    esp_rmaker_node_t *node = esp_rmaker_node_init(&rainmaker_cfg, "ESP RainMaker Multi Device", "Multi Device");
    if (!node) {
        ESP_LOGE(TAG, "Could not initialise node. Aborting!!!");
        vTaskDelay(5000/portTICK_PERIOD_MS);
        abort();
    }

    /* Create a Switch device and add the relevant parameters to it */
    switch_device = esp_rmaker_switch_device_create("Switch", NULL, DEFAULT_SWITCH_POWER);
    esp_rmaker_device_add_cb(switch_device, write_cb, NULL);
    esp_rmaker_node_add_device(node, switch_device);

    /* Create a Light device and add the relevant parameters to it */
    light_device = esp_rmaker_lightbulb_device_create("Light", NULL, DEFAULT_POWER);
    esp_rmaker_device_add_cb(light_device, write_cb, NULL);

    esp_rmaker_device_add_param(light_device, esp_rmaker_brightness_param_create(ESP_RMAKER_DEF_BRIGHTNESS_NAME, DEFAULT_BRIGHTNESS));
    esp_rmaker_device_add_param(light_device, esp_rmaker_hue_param_create(ESP_RMAKER_DEF_HUE_NAME, DEFAULT_HUE));
    esp_rmaker_device_add_param(light_device, esp_rmaker_saturation_param_create(ESP_RMAKER_DEF_SATURATION_NAME, DEFAULT_SATURATION));

    esp_rmaker_node_add_device(node, light_device);
    
    /* Create a Temperature Sensor1 device and add the relevant parameters to it */
    temp_sensor_device1 = esp_rmaker_temp_sensor_device_create("Temperature Sensor 1", NULL, app_get_current_temperature(1));
    esp_rmaker_node_add_device(node, temp_sensor_device1);

    /* Create a Temperature Sensor2 device and add the relevant parameters to it */
    temp_sensor_device2 = esp_rmaker_temp_sensor_device_create("Temperature Sensor 2", NULL, app_get_current_temperature(2));
    esp_rmaker_node_add_device(node, temp_sensor_device2);

    /* Create a Humidity Sensor1 device and add the relevant parameters to it */
            // humi_sensor_device1 = esp_rmaker_device_create("Humidity Sensor 1", "esp.device.temperature-sensor", NULL);
            // esp_rmaker_device_add_param(humi_sensor_device1, esp_rmaker_temperature_param_create("Humidity", app_get_current_humidity(1)));
            // esp_rmaker_node_add_device(node, humi_sensor_device1);
    humi_sensor_device1 = esp_rmaker_temp_sensor_device_create("Humidity Sensor 1", NULL, app_get_current_humidity(1));
    esp_rmaker_node_add_device(node, humi_sensor_device1);

    /* Create a Humidity Sensor2 device and add the relevant parameters to it */
    humi_sensor_device2 = esp_rmaker_temp_sensor_device_create("Humidity Sensor 2", NULL, app_get_current_humidity(2));
    esp_rmaker_node_add_device(node, humi_sensor_device2);

    /* Enable scheduling.
     * Please note that you also need to set the timezone for schedules to work correctly.
     * Simplest option is to use the CONFIG_ESP_RMAKER_DEF_TIMEZONE config option.
     * Else, you can set the timezone using the API call `esp_rmaker_time_set_timezone("Asia/Shanghai");`
     * For more information on the various ways of setting timezone, please check
     * https://rainmaker.espressif.com/docs/time-service.html.
     */
    esp_rmaker_schedule_enable();

    /* Start the ESP RainMaker Agent */
    esp_rmaker_start();

    /* Start the Wi-Fi.
     * If the node is provisioned, it will start connection attempts,
     * else, it will start Wi-Fi provisioning. The function will return
     * after a connection has been successfully established
     */
    err = app_wifi_start(POP_TYPE_RANDOM);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Could not start Wifi. Aborting!!!");
        vTaskDelay(5000/portTICK_PERIOD_MS);
        abort();
    }
}
