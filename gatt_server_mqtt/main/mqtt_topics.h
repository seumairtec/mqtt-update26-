#include "mqtt_client.h"

void setup_topic(char* device_name);
void subscribe_all_topics(uint8_t id);
void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
void mqtt_app_start(char* broker);
void sendReport();
void sendConfig();
void sendFaultReport(void);
void sendLinkDiagReport(void);
