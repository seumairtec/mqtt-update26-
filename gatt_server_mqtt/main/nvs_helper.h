#include "nvs_flash.h"


#define NVS_KEY_SSID "wifi_strg_ssid"
#define NVS_KEY_PWD "wifi_strg_pwd"
#define NVS_KEY_BROKER "mqtt_broker"
#define NVS_KEY_CURRENT "max_current"
#define NVS_KEY_RPM     "max_rpm"
#define NVS_KEY_PIN "pin_code"
#define NVS_KEY_NAME    "device_name"
void save_string(char* key, char *str);
void load_string(char *key, char *str);
void load_int(char *key, int16_t *value);
void save_int(char *key, int16_t *value);