/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

/****************************************************************************
*
* This demo showcases BLE GATT server. It can send adv data, be connected by client.
* Run the gatt_client demo, the client demo will automatically connect to the gatt_server demo.
* Client demo will enable gatt_server's notify after connection. The two devices will then exchange
* data.
*
****************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_log.h"

#include "nvs_helper.h"
#include "common.h"

#include "sdkconfig.h"
#include "uart.h"
#include "mqtt_topics.h"

#include "wifi_setup.h"
#define min(a,b)(a<b?a:b)

#define ATMEGA_LINK_TIMEOUT 5000

static const char *TAG = "MQTT_CLIENT";

char HOST_NAME[32];
char broker[128] = "mqtt://192.168.39.51:1883";

char mqtt_device_name[128] = "[SEUM] EC MOTOR";
queue_item_t itemFromUART;
queue_item_t itemToUART;

QueueHandle_t MQTTQueue;     //queue to BLE
QueueHandle_t UARTQueue;    //queue to UART



int restart_counter=0;


status_data_t actualReport;
esp_settings_t esp_settings =
{
    .max_rpm = 1000,
    .max_current = 10.0f,
    .pin = "0",
    .user_level = 0,
    .report_interval = 1000,
    .subscribed = false
};


void readline(char *p_line)
{
    
    int count= 0;

    while (count<128){

        int c = fgetc(stdin);
        if (c == '\n')
        {
            p_line[count] = '\0';
            break;
        }else if (c > 0 && c < 127) {
                p_line[count] = c;
                ++count;
        }

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}
void process_menu(void *arg)
{
    char line[32];
   // nvs_handle_t my_nvs_handle;
    while(true)
    {
        readline(line);

        if (strncmp("ssid", line,strlen(line)-1)==0)
        {
            printf("enter new SSID:\n");
            readline(line);
            save_string(NVS_KEY_SSID,line);
            printf("\nnew SSID: %s\n", line);
        }else if(strncmp("pwd", line,strlen(line)-1)==0)
        {
            printf("enter new wi-fi password:\n");
            readline(line);
            save_string(NVS_KEY_PWD, line);
            printf("\nnew WI-FI password:%s\n", line);
        }else if(strncmp("broker", line,strlen(line)-1)==0)
        {
            
            
            printf("enter MQTT broker url:\n");
            readline(line);
            save_string(NVS_KEY_BROKER, line);
            printf("\nmqtt broker configured:%s\n", line);
        }else if(strncmp("reset", line,strlen(line)-1)==0){
        /*    nvs_open("storage", NVS_READWRITE, &my_nvs_handle);
            restart_counter+=10;
            nvs_set_i32(my_nvs_handle, "restart_counter", restart_counter);
            nvs_commit(my_nvs_handle);
            nvs_close(my_nvs_handle);
            printf("counter increased");*/
            esp_restart();
        }else if(strncmp("disableuart", line,strlen(line)-1)==0){
            uart_stop();
            printf("uart deleted\n");
        }else if(strncmp("setid", line,strlen(line)-1)==0){
            printf("new device name:\n");
            readline(line);
            save_string(NVS_KEY_NAME, line);
            printf("\nnew device name configured:%s\n", line);
        }
        
    }
}
uint32_t noAtmegaLinkTime;

void MQTTTask(void *arg)
{
    int msg_id=0;
    mqtt_app_start(broker);
    uint8_t *pData;
    while(1)
    {
         if (xQueueReceive(MQTTQueue, &itemFromUART,10) == pdPASS)
        {
            noAtmegaLinkTime = 0;
            actualReport.atmegaLink = true;

            pData = itemFromUART.data;
            if ((pData[0] == 0) && (pData[1] == 6) && !esp_settings.subscribed)
            {
                sprintf(HOST_NAME, "[SEUM] EC MOTOR (%d)", itemFromUART.data[2]);
//                test_manufacturer[2] = itemFromUART.data[3];    //master/slave flag
                printf("setup host name %s\n", HOST_NAME);
                log_error_if_nonzero("tcpip_adapter_set_hostname ",tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_STA ,HOST_NAME));

                
                actualReport.ID = itemFromUART.data[2];
                esp_settings.subscribed = true;
            }

            if ((pData[0] == idCmd) && (pData[1] == idCmdStat))
            {
                /*
                0-1 cmd
                2-3 faults
                4-5 task rpm
                6-7 rpm
                8-9 current
                10-11 output
                12-13 temperature
                14-15 vsp
                16 - link
                17 - sequencer
                18-19 - humidity
                */
                actualReport.faults = (pData[2]<<8) + pData[3];
                actualReport.taskRPM  = (pData[4]<<8) + pData[5];
                actualReport.RPM = (pData[6]<<8) + pData[7];
                actualReport.current = (pData[8]<<8) + pData[9];
                actualReport.voltage = (pData[10]<<8) + pData[11];
                actualReport.temperature = (pData[12]<<8) + pData[13];
                actualReport.VSP = (pData[14]<<8) + pData[15];
                actualReport.link = pData[16];
                actualReport.sequencer = pData[17];
                actualReport.humidity = (pData[18]<<8) + pData[19];

                if (esp_settings.report_interval>0)
                    sendReport();
            }

            if ((pData[0] == idCmd) && (pData[1] == idCmdGetParams))
            {
                for(int i = 0; i<8; i++)
                {
                    setparam(i + pData[2],  (pData[i*2+3]<<8) + pData[i*2+4]);

                    printf("param %d=%d\n" , i + pData[2], getparam(i + pData[2]));
                }
                printf("received parametes set: %d\n", pData[2]);

                if (pData[2] == 16)  //last package received - send to MQTT
                {
                   
                    sendConfig();
                }
            }

            // if((pData[0] == idCmd) && (pData[1] ==  idCmdClearFaults))
            // {
            //     // There is no data response for idCmdClearFaults comamnd
            // }

            if ((pData[0] == idCmd) && (pData[1] == idCmdGetFaults))
            {
                actualReport.faults = (pData[2]<<8) + pData[3];
                sendFaultReport();
            }

            if ((pData[0] == idCmd) && (pData[1] == idCmdGetLinkDiag))
            {
                actualReport.diagState1 = (pData[3] << 8) | pData[2];
                actualReport.diagState2 = (pData[5] << 8) | pData[4];
                actualReport.diagState3 = (pData[7] << 8) | pData[6];
                actualReport.diagState4 = (pData[9] << 8) | pData[8];
                sendLinkDiagReport();
            }
        }

        vTaskDelay(100 / portTICK_PERIOD_MS);
        noAtmegaLinkTime += 100;
        if (noAtmegaLinkTime> ATMEGA_LINK_TIMEOUT)
            {
                noAtmegaLinkTime = ATMEGA_LINK_TIMEOUT;
                actualReport.atmegaLink = false;
            }
   //     msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
   //     ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
    }

}

void app_main(void)
{
    esp_err_t ret;
    
    
    char ssid[128] = "dummy ssid";
    char pwd[128] = "dummy pwd";
    

    // Initialize NVS.
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );


    load_string(NVS_KEY_SSID, ssid);
    load_string(NVS_KEY_PWD, pwd);
    load_string(NVS_KEY_BROKER, broker);
    load_string(NVS_KEY_PIN, esp_settings.pin);
    load_string(NVS_KEY_NAME, mqtt_device_name);


    load_int(NVS_KEY_CURRENT, &esp_settings.max_current);
    load_int(NVS_KEY_RPM, &esp_settings.max_rpm);
    
    wifi_setup_password(pwd);
    wifi_setup_ssid(ssid);

    printf("wifi ssid:%s  pass: %s\n", ssid, pwd);
    printf("mqtt device name:%s  \n", mqtt_device_name);

    setup_topic(mqtt_device_name);

    MQTTQueue = xQueueCreate(5, sizeof(queue_item_t));
    UARTQueue = xQueueCreate(5, sizeof(queue_item_t));

  
    
    if (UARTQueue != NULL)
    {
        if(xTaskCreate(uart_task, "uart_echo_task", ECHO_TASK_STACK_SIZE, NULL, 10, NULL)!=pdPASS)
        {
          
        }
        
        if(xTaskCreate(process_menu, "menu_task", ECHO_TASK_STACK_SIZE, NULL, 11, NULL)!=pdPASS)
        {
        
        }

        xTaskCreate(task_repeat_id, "id_request", ECHO_TASK_STACK_SIZE, NULL, 12, NULL);
    }else
    {
        
    }
    


    wifi_start();

    ESP_LOGI(TAG, "Got IP");

    xTaskCreate(MQTTTask, "mqtt_task", ECHO_TASK_STACK_SIZE, NULL, 9, NULL);
    


    
    return;
}
