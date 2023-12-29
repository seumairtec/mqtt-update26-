
#include "uart.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_gap_ble_api.h"
#include "common.h"

#define ECHO_TEST_TXD 17
#define ECHO_TEST_RXD 16
#define ECHO_TEST_RTS (UART_PIN_NO_CHANGE)
#define ECHO_TEST_CTS (UART_PIN_NO_CHANGE)

#define ECHO_UART_PORT_NUM 2
#define ECHO_UART_BAUD_RATE 19200

#define BUF_SIZE (128)

extern QueueHandle_t UARTQueue; // queue to UART
extern QueueHandle_t MQTTQueue;

// queue_item_t itemToBLE;
extern esp_settings_t esp_settings;

char debug_str[255];
uint16_t pstr;
uint16_t crc;
uint16_t crcFromPackage;

bool flUARTReady = false;
uint16_t CRC16 (const uint8_t *nData, uint8_t wLength)
{
	static const uint16_t wCRCTable[] = {
		0X0000, 0XC0C1, 0XC181, 0X0140, 0XC301, 0X03C0, 0X0280, 0XC241,
		0XC601, 0X06C0, 0X0780, 0XC741, 0X0500, 0XC5C1, 0XC481, 0X0440,
		0XCC01, 0X0CC0, 0X0D80, 0XCD41, 0X0F00, 0XCFC1, 0XCE81, 0X0E40,
		0X0A00, 0XCAC1, 0XCB81, 0X0B40, 0XC901, 0X09C0, 0X0880, 0XC841,
		0XD801, 0X18C0, 0X1980, 0XD941, 0X1B00, 0XDBC1, 0XDA81, 0X1A40,
		0X1E00, 0XDEC1, 0XDF81, 0X1F40, 0XDD01, 0X1DC0, 0X1C80, 0XDC41,
		0X1400, 0XD4C1, 0XD581, 0X1540, 0XD701, 0X17C0, 0X1680, 0XD641,
		0XD201, 0X12C0, 0X1380, 0XD341, 0X1100, 0XD1C1, 0XD081, 0X1040,
		0XF001, 0X30C0, 0X3180, 0XF141, 0X3300, 0XF3C1, 0XF281, 0X3240,
		0X3600, 0XF6C1, 0XF781, 0X3740, 0XF501, 0X35C0, 0X3480, 0XF441,
		0X3C00, 0XFCC1, 0XFD81, 0X3D40, 0XFF01, 0X3FC0, 0X3E80, 0XFE41,
		0XFA01, 0X3AC0, 0X3B80, 0XFB41, 0X3900, 0XF9C1, 0XF881, 0X3840,
		0X2800, 0XE8C1, 0XE981, 0X2940, 0XEB01, 0X2BC0, 0X2A80, 0XEA41,
		0XEE01, 0X2EC0, 0X2F80, 0XEF41, 0X2D00, 0XEDC1, 0XEC81, 0X2C40,
		0XE401, 0X24C0, 0X2580, 0XE541, 0X2700, 0XE7C1, 0XE681, 0X2640,
		0X2200, 0XE2C1, 0XE381, 0X2340, 0XE101, 0X21C0, 0X2080, 0XE041,
		0XA001, 0X60C0, 0X6180, 0XA141, 0X6300, 0XA3C1, 0XA281, 0X6240,
		0X6600, 0XA6C1, 0XA781, 0X6740, 0XA501, 0X65C0, 0X6480, 0XA441,
		0X6C00, 0XACC1, 0XAD81, 0X6D40, 0XAF01, 0X6FC0, 0X6E80, 0XAE41,
		0XAA01, 0X6AC0, 0X6B80, 0XAB41, 0X6900, 0XA9C1, 0XA881, 0X6840,
		0X7800, 0XB8C1, 0XB981, 0X7940, 0XBB01, 0X7BC0, 0X7A80, 0XBA41,
		0XBE01, 0X7EC0, 0X7F80, 0XBF41, 0X7D00, 0XBDC1, 0XBC81, 0X7C40,
		0XB401, 0X74C0, 0X7580, 0XB541, 0X7700, 0XB7C1, 0XB681, 0X7640,
		0X7200, 0XB2C1, 0XB381, 0X7340, 0XB101, 0X71C0, 0X7080, 0XB041,
		0X5000, 0X90C1, 0X9181, 0X5140, 0X9301, 0X53C0, 0X5280, 0X9241,
		0X9601, 0X56C0, 0X5780, 0X9741, 0X5500, 0X95C1, 0X9481, 0X5440,
		0X9C01, 0X5CC0, 0X5D80, 0X9D41, 0X5F00, 0X9FC1, 0X9E81, 0X5E40,
		0X5A00, 0X9AC1, 0X9B81, 0X5B40, 0X9901, 0X59C0, 0X5880, 0X9841,
		0X8801, 0X48C0, 0X4980, 0X8941, 0X4B00, 0X8BC1, 0X8A81, 0X4A40,
		0X4E00, 0X8EC1, 0X8F81, 0X4F40, 0X8D01, 0X4DC0, 0X4C80, 0X8C41,
		0X4400, 0X84C1, 0X8581, 0X4540, 0X8701, 0X47C0, 0X4680, 0X8641,
	0X8201, 0X42C0, 0X4380, 0X8341, 0X4100, 0X81C1, 0X8081, 0X4040 };

	uint8_t nTemp;
	uint16_t wCRCWord = 0xFFFF;

	while (wLength--)
	{
		nTemp = *nData++ ^ wCRCWord;
		wCRCWord >>= 8;
		wCRCWord ^= wCRCTable[nTemp];
	}
	return wCRCWord;

}
void uart_stop()
{
    if (flUARTReady)
    {
        uart_driver_delete(ECHO_UART_PORT_NUM);
        flUARTReady = false;
        gpio_set_pull_mode(ECHO_TEST_TXD, GPIO_FLOATING);
        gpio_set_direction(ECHO_TEST_TXD, GPIO_MODE_DISABLE);

        gpio_set_pull_mode(ECHO_TEST_RXD, GPIO_FLOATING);
        gpio_set_direction(ECHO_TEST_RXD, GPIO_MODE_DISABLE);
    }
}
void uart_task(void *arg)
{
    queue_item_t ble_item;
    /* Configure parameters of an UART driver,
        * communication pins and install the driver */
    uart_config_t uart_config = {
        .baud_rate = ECHO_UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    int intr_alloc_flags = 0;

#if CONFIG_UART_ISR_IN_IRAM
    intr_alloc_flags = ESP_INTR_FLAG_IRAM;
#endif

    ESP_ERROR_CHECK(uart_driver_install(ECHO_UART_PORT_NUM, BUF_SIZE * 2, 0, 0, NULL, intr_alloc_flags));
    ESP_ERROR_CHECK(uart_param_config(ECHO_UART_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(ECHO_UART_PORT_NUM, ECHO_TEST_TXD, ECHO_TEST_RXD, ECHO_TEST_RTS, ECHO_TEST_CTS));
    flUARTReady = true;
    // Configure a temporary buffer for the incoming data
    uint8_t *data = (uint8_t *)malloc(BUF_SIZE);

    //-- at start send request ID -- 
    ble_item.data[0] = idCmd;
    ble_item.data[1] = idCmdGetID;
    ble_item.data_len = 2;

    vTaskDelay( 20 );
    uart_write_bytes(ECHO_UART_PORT_NUM, (const char *)ble_item.data, ble_item.data_len);

    vTaskDelay(200/ portTICK_PERIOD_MS);
    //-- at start send request current settings -- 
    ble_item.data[0] = idCmd;
    ble_item.data[1] = idCmdGetParams;
    ble_item.data_len = 2;

    
    uart_write_bytes(ECHO_UART_PORT_NUM, (const char *)ble_item.data, ble_item.data_len);

    while (1)
    {
        // Read data from the UART
        int len =0;
        if (flUARTReady) 
            len = uart_read_bytes(ECHO_UART_PORT_NUM, data, BUF_SIZE, 1);

        if (len > 0)
        {
            // --- check CRC --
            crc = CRC16(data, len-2);
            crcFromPackage = (data[len-2]<<8) + data[len-1];
            //   ESP_LOGI("UART", "UART package received (%d)\n", len);

            pstr=0;
            for(int i=0; i<len; i++)
                pstr+= sprintf(&debug_str[pstr], "%X ", data[i]);

           // ESP_LOGI("UART:","%s",debug_str);
            if (crc != crcFromPackage)
            {
             //   ESP_LOGE("UART:", "BAD CRC");
                //   uart_write_bytes(ECHO_UART_PORT_NUM, (const char *)ble_item.data, ble_item.data_len);                    
            }
            else
            {

                for (int i = 0; i < (len-2); i++)
                    ble_item.data[i] = data[i];

                ble_item.data_len = len-2;
                

                if (xQueueSend(MQTTQueue, &ble_item, 1) == pdPASS)
                {
   //                 ESP_LOGI("UART", "Written to queue\n");
                }
            }
        }
        // Write data back to the UART
        if (xQueueReceive(UARTQueue, &ble_item, 10) == pdPASS)
        {
            if (flUARTReady)
            {
                uart_write_bytes(ECHO_UART_PORT_NUM, (const char *)ble_item.data, ble_item.data_len);
    //            ESP_LOGI("UART", "Writing to UART\n");
            }
        }
    }
}

void task_repeat_id(void *arg)
{
    queue_item_t ble_item;

        vTaskDelay(100/ portTICK_PERIOD_MS);
      
    while(1)
    {
        if (esp_settings.report_interval>0)
        {
            vTaskDelay(esp_settings.report_interval/ portTICK_PERIOD_MS);
        }
        else
        {
            vTaskDelay(200/ portTICK_PERIOD_MS);
        }

        ble_item.data[0] = idCmd;
        ble_item.data[1] = idCmdStat;
        ble_item.data_len = 2;
        uart_write_bytes(ECHO_UART_PORT_NUM, (const char *)ble_item.data, ble_item.data_len);
        
        
    }
    return;
}