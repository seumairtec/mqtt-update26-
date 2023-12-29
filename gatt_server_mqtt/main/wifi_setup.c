#include "wifi_setup.h"

//char  wifi_conf_ssid[32];//"Galaxy S10eb79d"
//char wifi_conf_pwd[64];
//#define CONFIG_WIFI_PASSWORD "appt9700"
#define CONFIG_WIFI_SCAN_METHOD WIFI_FAST_SCAN
#define CONFIG_WIFI_CONNECT_AP_SORT_METHOD WIFI_CONNECT_AP_BY_SIGNAL
#define CONFIG_EXAMPLE_WIFI_SCAN_RSSI_THRESHOLD -127
#define CONFIG_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK

static const char *TAG = "WIFI_STATION";

//static esp_ip4_addr_t s_ip_addr;
static xSemaphoreHandle s_semph_get_ip_addrs;

//static esp_netif_t *s_example_esp_netif = NULL;
wifi_config_t wifi_config = {
        .sta = {
         //   .ssid = wifi_conf_ssid,
        //    .password = wifi_conf_pwd,
          
            .threshold.authmode = CONFIG_WIFI_SCAN_AUTH_MODE_THRESHOLD,
        },
    };

void wifi_setup_ssid(char *ssid)
{
    if (strlen(ssid)>1)
    {
        //strcpy(wifi_config.sta.ssid, dummy);
        //wifi_config.sta.ssid = ssid;
        for (int i=0; i<strlen(ssid)+1; i++)
            wifi_config.sta.ssid[i] = ssid[i];

//        xSemaphoreGive(s_semph_credentials);
    }
}

void wifi_setup_password(char *pwd)
{
    if (strlen(pwd)>1)
    {
        //strcpy(wifi_config.sta.password, pwd);
        for (int i=0; i<strlen(pwd)+1; i++)
            wifi_config.sta.password[i] = pwd[i];

 //       xSemaphoreGive(s_semph_credentials);
    }
}
void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

/*static bool is_our_netif(const char *prefix, esp_netif_t *netif)
{
    return strncmp(prefix, esp_netif_get_desc(netif), strlen(prefix) - 1) == 0;
}*/

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "WIFI_EVENT_STA_START");
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "WIFI_EVENT_STA_DISCONNECTED");
        esp_wifi_connect();
    }else if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED){
        ESP_LOGI(TAG, "WIFI_EVENT_STA_CONNECTED");
    
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        xSemaphoreGive(s_semph_get_ip_addrs);
    } 
}

void wifi_start(void)
{
   // char *desc;
  //  esp_err_t err_code;
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));


    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    log_error_if_nonzero("esp_wifi_set_storage", esp_wifi_set_storage(WIFI_STORAGE_RAM));

    
    ESP_LOGI(TAG, "Connecting to %s...", wifi_config.sta.ssid);
    log_error_if_nonzero("esp_wifi_set_mode",esp_wifi_set_mode(WIFI_MODE_STA));
    log_error_if_nonzero("esp_wifi_set_config", esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    log_error_if_nonzero("esp_wifi_start", esp_wifi_start());
   // log_error_if_nonzero("esp_wifi_connect", esp_wifi_connect());
   s_semph_get_ip_addrs = xSemaphoreCreateCounting(1, 0);
   

    tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_STA ,"[SEUM] EC MOTOR");
    
     xSemaphoreTake(s_semph_get_ip_addrs, portMAX_DELAY);
}

