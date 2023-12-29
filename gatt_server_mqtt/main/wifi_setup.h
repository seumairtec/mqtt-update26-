#include "esp_wifi.h"
#include "esp_wifi_default.h"
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "esp_log.h"

void wifi_start(void);


void wifi_setup_password(char *pwd);
void wifi_setup_ssid(char *ssid);

void log_error_if_nonzero(const char *message, int error_code);