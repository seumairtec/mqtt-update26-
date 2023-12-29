#include "mqtt_topics.h"
#include "esp_log.h"
#include "common.h"
#include "nvs_helper.h"
#include "cJSON.h"

const char* MQTT_TOPIC_WIDEREQUEST = "ec_motor/wide_request";
const char* MQTT_TOPIC_WIDERESPONCE = "ec_motor/wide_response";
static const char *TAG = "MQTT_CLIENT";

#define TOPIC_COMMAND_RUN "command/run"
#define TOPIC_COMMAND_RESETATMEGA "command/reset_atmega"
#define TOPIC_COMMAND_LOGIN "command/login"
#define TOPIC_COMMAND_LOGOUT "command/logout"
#define TOPIC_COMMAND_SET_REPORT_INTERVAL "command/set_report_interval"
#define TOPIC_COMMAND_REPORT "command/report"
#define TOPIC_COMMAND_SETPIN "command/setpin"
#define TOPIC_COMMAND_SET_RATED_RPM "command/set_rated_rpm"
#define TOPIC_COMMAND_CLEAR_FAULT   "command/clear_fault"
#define TOPIC_COMMAND_GET_FAULT     "command/get_fault"
#define TOPIC_COMMAND_GET_LINK_DIAG "command/get_link_diag"

#define TOPIC_REPORT_MEASURES "/report/measures"
#define TOPIC_REPORT_CONFIG "/report/config"
#define TOPIC_REPORT_FAULT "/report/fault"
#define TOPIC_REPORT_LINK_DIAG "/report/link_diag"
esp_mqtt_client_handle_t client;
bool bMQTTConnected = false;
char send_buf[2048];

char topic_prefix[48];
/*char **command_topics=
{
    {"set_rated_rpm"},
    {"set_poll_period"},
    {"set_sensor_id"},
    {"set_use_vsp"},
    {"set_switch_direction"},
    {"set_modbus_master"},
    {"set_min_temp"},
    {"set_max_temp"},
    {"set_min_hum"},
    {"set_max_hum"},
    {"set_suppr_1_bottom"},
    {"set_suppr_1_top"},
    {"set_suppr_2_bottom"},
    {"set_suppr_2_top"},
    {"set_suppr_3_bottom"},
    {"set_suppr_3_top"},
    {"set_vsp_top"},
    {"set_vsp_bottom"},
    {"set_slaves_count"},
    {"set_current_lim"},
    {"set_max_rpm"},
    {"set_max_current"}
};*/
// static char* sequencer_states[]= {
//     "power on", 
//     "stop", 
//     "calc offset current", 
//     "changing bootstrap", 
//     "running", 
//     "fault", 
//     "catch spin", 
//     "parking", 
//     "open loop", 
//     "angle sensing",
//     "11", "12", "13", "14", "15"};

char *report_template = "{\n"
	"\"id\":%d,\n "
	"\"attributes\":{\n"
	"\"RPM\":%d,\n"
	"\"Task RPM\":%d, \n"
	"\"Current\":%.1f, \n" 
	"\"Output\":%.1f, \n"
	"\"Temperature\":%.1f, \n"
	"\"Humidity\":%.1f,\n"

	"\"VSP\":%d, \n"
	"\"Sequencer state\": %d, \n"
	"\"Sensor link\": \"%s\", \n"
	"\"Driver link\": \"%s\", \n"
	"\"Atmega link\": \"%s\", \n"
    "\"User level\": \"%d\" \n"
	"}, \n"
	"\"faults\":{ \n"
	"\"Motor gatekill\":%s, \n"
	"\"DC bus Critical overvoltage\":%s, \n"
	"\"DC bus overvoltage\":%s, \n"
	"\"DC bus undervoltage\":%s, \n"
	"\"Flux PLL out of control\":%s, \n"
	"\"Over Temperature\":%s, \n"
	"\"Rotor lock\":%s, \n"
	"\"Phase Loss\":%s,\n"
	"\"CPU load over 95\":%s, \n"
	"\"Parameter load\":%s, \n"
	"\"UART link break\":%s, \n"
	"\"Hall timeout\":%s, \n"
	"\"Hall invalid\":%s \n}\n}";

char * fault_report_template ="{\n"
	"\"faults\":{ \n"
	"\"Motor gatekill\":%s, \n"
	"\"DC bus Critical overvoltage\":%s, \n"
	"\"DC bus overvoltage\":%s, \n"
	"\"DC bus undervoltage\":%s, \n"
	"\"Flux PLL out of control\":%s, \n"
	"\"Over Temperature\":%s, \n"
	"\"Rotor lock\":%s, \n"
	"\"Phase Loss\":%s,\n"
	"\"CPU load over 95\":%s, \n"
	"\"Parameter load\":%s, \n"
	"\"UART link break\":%s\n}\n}";

char *param_report_template= "{\n"
		"\"Rated RPM\":%d,\n"
        "\"Modus poll period\":%d,\n"
        "\"Sensor modbus ID\":%d,\n"
		"\"Use VSP\":%s,\n"
		"\"Switch direction\":%s,\n"
		"\"Modbus master\":%s,\n"
		"\"Min temperature\":%d,\n"
		"\"Max temperature\":%d,\n"
		"\"Min humidity\":%d,\n"
		"\"Max humidity\":%d,\n"
		"\"Suppression 1 bottom\":%d,\n"
		"\"Suppression 1 top\":%d,\n"
		"\"Suppression 2 bottom\":%d,\n"
		"\"Suppression 2 top\":%d,\n"
		"\"Suppression 3 bottom\":%d,\n"
		"\"Suppression 3 top\":%d,\n"
		"\"VSP Top value\":%d,\n"
		"\"VSP Bottom value\":%d,\n"
        "\"Slaves cnt\": %d,\n"
        "\"Current limit\": %d,\n"
		"\"Max RPM\":%d,\n"
		"\"Max Current\":%d,\n"
		"\"Report interval\":%d\n}";


//------------------------- EXTERN DEFINITIONS --------------------------------------
extern status_data_t actualReport;
extern esp_settings_t esp_settings;
extern QueueHandle_t UARTQueue;    //queue to UART
//------------------------- METHODS -----------------------------------------------
void setup_topic(char* device_name)
{
    strncpy(topic_prefix, device_name, strlen(device_name));
}
void subscribe_all_topics(uint8_t id)
{
    char wide_topic[64];

    sprintf(&topic_prefix[strlen(topic_prefix)], "(%d)", id);
    sprintf(wide_topic, "%s/#", topic_prefix);

    esp_mqtt_client_subscribe(client, wide_topic, 0);
}
bool isTopicEqual(char *receivedTopic, char* expectedTopic)
{
    return (strncmp(receivedTopic, expectedTopic, strlen(expectedTopic))==0);
}

int16_t RPMToRealValue(int16_t rpm)
{
    return (int16_t)((float)rpm* (float)esp_settings.max_rpm / 16383.0f);

}

float CurrentToReal(uint16_t current)
{
    return (float)((float)current* (float)esp_settings.max_current / 4096.0f);
}

void sendParamToAtmegaInt(uint8_t nParam, char *data)
{
    queue_item_t itemToUART;
    int16_t intParam=0;
    intParam = atoi(data);
    itemToUART.data[0] = idWriteReg;
    itemToUART.data[1] = nParam;
    itemToUART.data[2] = intParam>>8;
    itemToUART.data[3] = intParam&0xff;
    itemToUART.data_len = 4;

    if (esp_settings.user_level == 0)
        return;

    if (xQueueSend(UARTQueue, &itemToUART, 1) == pdPASS)
    {
        ESP_LOGI("UART", "Written to queue\n");
    }
}

void sendParamToAtmegaTemperature(uint8_t nParam, char *data)
{
    queue_item_t itemToUART;
    int16_t intParam=0;
    intParam = atoi(data) + 50;
    itemToUART.data[0] = idWriteReg;
    itemToUART.data[1] = nParam;
    itemToUART.data[2] = intParam>>8;
    itemToUART.data[3] = intParam&0xff;
    itemToUART.data_len = 4;

    if (esp_settings.user_level == 0)
        return;

    if (xQueueSend(UARTQueue, &itemToUART, 1) == pdPASS)
    {
        ESP_LOGI("UART", "Written to queue\n");
    }
}

bool login(char *recpin, uint8_t len)
{
    bool res = true;
    for(int i=0; i< strlen(esp_settings.pin) || (i< len); i++)
    {
        if (recpin[i] != esp_settings.pin[i])
            res = false;
    }
    if (res)
        esp_settings.user_level = 1;
    return res;
}

void logout()
{
    esp_settings.user_level = 0;
}

void sendParamToAtmegaRPM(uint8_t nParam, char *data, uint8_t len)
{
    queue_item_t itemToUART;
    uint16_t intParam=0;
    char buf[16];
   
    memset(buf,0,16);
    strncpy(buf, data,len);
    int rpm = atoi(buf);

    intParam = (uint16_t)((float)rpm * RPMTopScale / (float)esp_settings.max_rpm);
    printf("saving rpm: %d, %d\n", rpm, intParam);
    //ivalue = (ivalue * GlobalConsts.RPMTopScale / (Storage.appParams["MAXRPM"] ?? 1)).round();
    itemToUART.data[0] = idWriteReg;
    itemToUART.data[1] = nParam;
    itemToUART.data[2] = intParam>>8;
    itemToUART.data[3] = intParam&0xff;
    itemToUART.data_len = 4;

    if (esp_settings.user_level == 0)
        return;

    if (xQueueSend(UARTQueue, &itemToUART, 1) == pdPASS)
    {
        ESP_LOGI("UART", "Written to queue\n");
    }
}

void sendParamToAtmegaBool(uint8_t nParam, char *data)
{
    queue_item_t itemToUART;
    int16_t intParam=0;
 /*   for(int i=0; i < strlen(data); i++)
    {
        data[i] = tolower(data[i]);
    }*/
    if (data[0]=='t' && data[1]=='r' && data[2]=='u' && data[3]=='e')
    {
        intParam = 1;
    }else if(data[0]=='f' && data[1]=='a' && data[2]=='l' && data[3]=='s')
    {
        intParam = 0;
    }else
    {
        return;
    }
    itemToUART.data[0] = idWriteReg;
    itemToUART.data[1] = nParam;
    itemToUART.data[2] = intParam>>8;
    itemToUART.data[3] = intParam&0xff;
    itemToUART.data_len = 4;

    if (esp_settings.user_level == 0)
        return;

    if (xQueueSend(UARTQueue, &itemToUART, 1) == pdPASS)
    {
        ESP_LOGI("UART", "Written to queue\n");
    }
}
void sendReport()
{
    char full_topic[128];
    bool flFaults[16];
    
    if (!bMQTTConnected)
    {
        return;
    }

    for (int i = 0; i<16; i++)
        flFaults[i] = (actualReport.faults & (1 << i)) ? true : false;

    bool sensorLinkStat = (actualReport.link & 2) >0;
    bool driverLinkStat = (actualReport.link & 1) >0;

    float humidity = ((float)actualReport.humidity)/10.f;
    float temperature = ((float)actualReport.temperature)/10.f;
    float VDC = ((float)actualReport.voltage)*2013.3f/13.3f /4096.0f * 5.0f;
    float theCurrent = CurrentToReal(actualReport.current);
    float theOutput = VDC * theCurrent;
    sprintf(full_topic, "%s%s", topic_prefix, TOPIC_REPORT_MEASURES);

    //printf("preparing to send\n");
    sprintf(send_buf, report_template, 
        actualReport.ID,     //id
        RPMToRealValue(actualReport.RPM),    //rpm
        RPMToRealValue(actualReport.taskRPM),   //task rpm
        theCurrent,    //current
        theOutput,    //output
        temperature,    //temperature
        humidity,    //humidity
        actualReport.VSP,    //vsp
        actualReport.sequencer, //sequencer
        sensorLinkStat ? "ok":"fault", //sensor link
        driverLinkStat ? "ok":"fault",   //driver link
        actualReport.atmegaLink ? "ok":"fault",  //atmega link
        esp_settings.user_level,
        flFaults[0]? "true": "false", //faults
        flFaults[1]? "true": "false", //faults
        flFaults[2]? "true": "false", //faults
        flFaults[3]? "true": "false", //faults
        flFaults[4]? "true": "false", //faults
        flFaults[5]? "true": "false", //faults
        flFaults[6]? "true": "false", //faults
        flFaults[7]? "true": "false", //faults
        flFaults[8]? "true": "false", //faults
        flFaults[9]? "true": "false", //faults
        flFaults[10]? "true": "false", //faults
        flFaults[11]? "true": "false", //faults
        flFaults[12]? "true": "false" //faults
        );
   // sprintf("size to send %d\n", strlen(send_buf));
    //esp_mqtt_client_publish(client,full_topic,send_buf, strlen(send_buf),0,0 );
    esp_mqtt_client_publish(client,full_topic,send_buf, strlen(send_buf),0,0 );
}

void sendConfig()
{
    char full_topic[128];

    if (!bMQTTConnected)
    {
        return;
    }


    sprintf(full_topic, "%s%s", topic_prefix, TOPIC_REPORT_CONFIG);
    /*sprintf(send_buf, param_report_template, 
        RPMToRealValue(rawParams[idRegRatedRPM]),
        rawParams[idRegMBPollPeriod],
        rawParams[idRegSensorID],
        rawParams[idRegVSP]>0 ? "true" : "false",
        rawParams[idRegDir]>0 ? "true" : "false",
        rawParams[idRegMBMaster]>0 ? "true" : "false",
        rawParams[idRegMinTemp] - 50,
        rawParams[idRegMaxTemp] - 50,
        rawParams[idRegMinHum],
        rawParams[idRegMaxHum],
        RPMToRealValue(rawParams[idRegSuppression1low]),
        RPMToRealValue(rawParams[idRegSuppression1high]),
        RPMToRealValue(rawParams[idRegSuppression2low]),
        RPMToRealValue(rawParams[idRegSuppression2high]),
        RPMToRealValue(rawParams[idRegSuppression3low]),
        RPMToRealValue(rawParams[idRegSuppression3high]),
        rawParams[idRegVSPTop],
        rawParams[idRegVSPBottom],
        rawParams[idRegSlavesCnt],
        rawParams[idRegCurrentLim],
        (int)esp_settings.max_rpm,
        esp_settings.max_current,
        esp_settings.report_interval
    );*/
    sprintf(send_buf, param_report_template, 
        RPMToRealValue(getparam(idRegRatedRPM)),
        getparam(idRegMBPollPeriod),
        getparam(idRegSensorID),
        getparam(idRegVSP)>0 ? "true" : "false",
        getparam(idRegDir)>0 ? "true" : "false",
        getparam(idRegMBMaster)>0 ? "true" : "false",
        getparam(idRegMinTemp) - 50,
        getparam(idRegMaxTemp) - 50,
        getparam(idRegMinHum),
        getparam(idRegMaxHum),
        RPMToRealValue(getparam(idRegSuppression1low)),
        RPMToRealValue(getparam(idRegSuppression1high)),
        RPMToRealValue(getparam(idRegSuppression2low)),
        RPMToRealValue(getparam(idRegSuppression2high)),
        RPMToRealValue(getparam(idRegSuppression3low)),
        RPMToRealValue(getparam(idRegSuppression3high)),
        getparam(idRegVSPTop),
        getparam(idRegVSPBottom),
        getparam(idRegSlavesCnt),
        getparam(idRegCurrentLim),
        esp_settings.max_rpm,
        esp_settings.max_current,
        esp_settings.report_interval
    );

    esp_mqtt_client_publish(client,full_topic,send_buf, strlen(send_buf),0,0 );
}

void sendFaultReport(void)
{
    char full_topic[128];
    bool flFaults[16];
    if (!bMQTTConnected)
    {
        return;
    }

    for (int i = 0; i < 16; i++)
    {
        flFaults[i] = (actualReport.faults & (1 << i)) ? true : false;
    }

    sprintf(full_topic, "%s%s", topic_prefix, TOPIC_REPORT_FAULT);

    //printf("preparing to send\n");
    sprintf(send_buf, fault_report_template,
            flFaults[0] ? "true" : "false",  /** [0] Motor gatekill fault */
            flFaults[1] ? "true" : "false",  /** [1] DC bus Critical overvoltage fault */
            flFaults[2] ? "true" : "false",  /** [2] DC bus overvoltage fault */
            flFaults[3] ? "true" : "false",  /** [3] DC bus under voltage fault */
            flFaults[4] ? "true" : "false",  /** [4] Flux PLL out of control fault */
            flFaults[6] ? "true" : "false",  /** [6] Over Temperature fault */
            flFaults[7] ? "true" : "false",  /** [7] Rotor lock fault */
            flFaults[8] ? "true" : "false",  /** [8] Phase Loss fault */
            flFaults[10] ? "true" : "false", /** [10] Execution fault (CPU load is more than 95%) */
            flFaults[12] ? "true" : "false", /** [12] Parameter load fault */
            flFaults[13] ? "true" : "false" /** [13] UART link break fault */
    );
    esp_mqtt_client_publish(client,full_topic,send_buf, strlen(send_buf),0,0 );
}

void sendLinkDiagReport(void)
{
    char full_topic[128];
    bool diags[64];
    if (!bMQTTConnected)
    {
        return;
    }

    sprintf(full_topic, "%s%s", topic_prefix, TOPIC_REPORT_LINK_DIAG);
    for(int i = 0; i < 16; i++)
    {
        diags[i +  0] = (actualReport.diagState1 & (1 << i)) ? true : false;
        diags[i + 16] = (actualReport.diagState2 & (1 << i)) ? true : false;
        diags[i + 32] = (actualReport.diagState3 & (1 << i)) ? true : false;
        diags[i + 48] = (actualReport.diagState4 & (1 << i)) ? true : false;
    }

    cJSON* root = cJSON_CreateObject();
    cJSON* diag = cJSON_AddObjectToObject(root, "diagnostics");
    char jsonNames[32];
    for(int i = 0; i < 64; i++)
    {
        snprintf(jsonNames, sizeof(jsonNames), "Channel-%d", (int)(i + 1));
        cJSON_AddStringToObject(diag, jsonNames, diags[i] ? "true" : "false");
    }

    char* jsonStr = cJSON_PrintUnformatted(root);
    if(jsonStr)
    {
        esp_mqtt_client_publish(client,full_topic,jsonStr, strlen(jsonStr),0,0 );
    }
    cJSON_free(jsonStr);
    cJSON_Delete(root);
}

void request_config()
{
    queue_item_t itemToUART;

    itemToUART.data[0] = idCmd;
    itemToUART.data[1] = idCmdGetParams;
    itemToUART.data_len = 2;

    if (xQueueSend(UARTQueue, &itemToUART, 1) == pdPASS)
    {
        ESP_LOGI("UART", "Written to queue\n");
    }
}

void process_run_command(char *data, uint8_t len)
{
    queue_item_t itemToUART;

    itemToUART.data[0] = idCmd;
    itemToUART.data[1] = idCmdRPM;
    
    itemToUART.data_len = 4;

    

    if (strncmp(data, "start", len)==0)
    {
        itemToUART.data[2] = getparam(idRegRatedRPM) >>8;
        itemToUART.data[3] = getparam(idRegRatedRPM) & 0xff;
    }else if(strncmp(data, "stop", len)==0)
    {
        itemToUART.data[2] = 0;
        itemToUART.data[3] = 0;
    }else
        {
            int rpm = atoi(data);
            uint16_t intParam = (uint16_t)((float)rpm * RPMTopScale / (float)esp_settings.max_rpm);
            itemToUART.data[2] = intParam >>8 ;
            itemToUART.data[3] = intParam & 0xFF;
        }

    if (xQueueSend(UARTQueue, &itemToUART, 1) == pdPASS)
    {
        ESP_LOGI("UART", "Written to queue\n");
    }
}
void process_topic(char *topic, char *data, uint8_t len)
{
    if(topic == NULL || data == NULL)
    {
        return;
    }
    
    queue_item_t itemToUART;
    char topic_cmp[64];
    if (strncmp(topic, MQTT_TOPIC_WIDEREQUEST, strlen(MQTT_TOPIC_WIDEREQUEST)-1)==0)
    {
        esp_mqtt_client_publish(client, MQTT_TOPIC_WIDERESPONCE ,topic_prefix, strlen(topic_prefix),0,0);
    }

    //compare first part
    sprintf(topic_cmp, "%s/%s/", topic_prefix, "command");
    if (strncmp(topic_cmp, topic, strlen(topic_cmp))!=0)
        return;

    if(strncmp(&topic[strlen(topic_cmp)], "run", strlen("run"))==0)
    {
       process_run_command(data, len);
       // esp_mqtt_client_publish(client, MQTT_TOPIC_WIDERESPONCE ,topic_prefix, strlen(topic_prefix),0,0);
    }
    else if(strncmp(&topic[strlen(topic_cmp)], "reset_atmega", strlen("reset_atmega"))==0)
    {
        itemToUART.data[0] = idCmd;
        itemToUART.data[1] = idCmdResetAtmega;
        itemToUART.data_len = 2;

        if (xQueueSend(UARTQueue, &itemToUART, 1) == pdPASS)
        {
            ESP_LOGI("UART", "Written to queue\n");
        }
    }
    else if(isTopicEqual(&topic[strlen(topic_cmp)], "login"))
    {
        login(data, len);
    }
    else if(isTopicEqual(&topic[strlen(topic_cmp)], "logout"))
    {
        logout();
    }
    else if(isTopicEqual(&topic[strlen(topic_cmp)], "set_report_interval"))
    {
        esp_settings.report_interval = atoi(data);
    }
    else if(isTopicEqual(&topic[strlen(topic_cmp)], "report"))
    {
        if (data[0] == 'm')
            sendReport();
        else if(data[0] == 'c')
            request_config();//sendConfig();
    }
    else if(isTopicEqual(&topic[strlen(topic_cmp)], "setpin"))
    {
        if (esp_settings.user_level > 0)
        {
            strncpy(esp_settings.pin, data, len+1);
            save_string(NVS_KEY_PIN, esp_settings.pin);
        }
    }
    else if(isTopicEqual(&topic[strlen(topic_cmp)], "set_rated_rpm"))
    {
        sendParamToAtmegaRPM(idRegRatedRPM, data, len);
    }
    else if(isTopicEqual(&topic[strlen(topic_cmp)], "set_poll_period"))
    {
        sendParamToAtmegaInt(idRegMBPollPeriod, data);
    }
    else if(isTopicEqual(&topic[strlen(topic_cmp)], "set_sensor_id"))
    {
        sendParamToAtmegaInt(idRegSensorID, data);
    }
    else if(isTopicEqual(&topic[strlen(topic_cmp)], "set_use_vsp"))
    {
        sendParamToAtmegaBool(idRegVSP, data);
    }
    else if(isTopicEqual(&topic[strlen(topic_cmp)], "set_switch_direction"))
    {
        sendParamToAtmegaBool(idRegDir, data);
    }
    else if(isTopicEqual(&topic[strlen(topic_cmp)], "set_modbus_master"))
    {
        sendParamToAtmegaBool(idRegMBMaster, data);
    }
    else if(isTopicEqual(&topic[strlen(topic_cmp)], "set_min_temp"))
    {
        sendParamToAtmegaTemperature(idRegMinTemp, data);
    }
    else if(isTopicEqual(&topic[strlen(topic_cmp)], "set_max_temp"))
    {
        sendParamToAtmegaTemperature(idRegMaxTemp, data);
    }
    else if(isTopicEqual(&topic[strlen(topic_cmp)], "set_min_hum"))
    {
        sendParamToAtmegaInt(idRegMinHum, data);
    }
    else if(isTopicEqual(&topic[strlen(topic_cmp)], "set_max_hum"))
    {
        sendParamToAtmegaInt(idRegMaxHum, data);
    }
    else if(isTopicEqual(&topic[strlen(topic_cmp)], "set_suppr_1_bottom"))
    {
        sendParamToAtmegaRPM(idRegSuppression1low, data, len);
    }
    else if(isTopicEqual(&topic[strlen(topic_cmp)], "set_suppr_1_top"))
    {
        sendParamToAtmegaRPM(idRegSuppression1high, data, len);
    }
    else if(isTopicEqual(&topic[strlen(topic_cmp)], "set_suppr_2_bottom"))
    {
        sendParamToAtmegaRPM(idRegSuppression2low, data, len);
    }
    else if(isTopicEqual(&topic[strlen(topic_cmp)], "set_suppr_2_top"))
    {
        sendParamToAtmegaRPM(idRegSuppression2high, data, len);
    }
    else if(isTopicEqual(&topic[strlen(topic_cmp)], "set_suppr_3_bottom"))
    {
        sendParamToAtmegaRPM(idRegSuppression3low, data, len);
    }
    else if(isTopicEqual(&topic[strlen(topic_cmp)], "set_suppr_3_top"))
    {
        sendParamToAtmegaRPM(idRegSuppression3high, data, len);
    }
    else if(isTopicEqual(&topic[strlen(topic_cmp)], "set_vsp_top"))
    {
        sendParamToAtmegaInt(idRegVSPTop, data);
    }
    else if(isTopicEqual(&topic[strlen(topic_cmp)], "set_vsp_bottom"))
    {
        sendParamToAtmegaInt(idRegVSPBottom, data);
    }
    else if(isTopicEqual(&topic[strlen(topic_cmp)], "set_slaves_count"))
    {
        sendParamToAtmegaInt(idRegSlavesCnt, data);
    }
    else if(isTopicEqual(&topic[strlen(topic_cmp)], "set_current_lim"))
    {
        sendParamToAtmegaInt(idRegCurrentLim, data);
    }
    else if(isTopicEqual(&topic[strlen(topic_cmp)], "set_max_rpm"))
    {
        esp_settings.max_rpm = atoi(data);
        save_int(NVS_KEY_RPM, &esp_settings.max_rpm);
    }
    else if(isTopicEqual(&topic[strlen(topic_cmp)], "set_max_current"))
    {
        esp_settings.max_current = atoi(data);
        save_int(NVS_KEY_CURRENT, &esp_settings.max_current);
    }
    else if(isTopicEqual(&topic[strlen(topic_cmp)], "clear_fault"))
    {
        itemToUART.data[0] = idCmd;
        itemToUART.data[1] = idCmdClearFaults;
        itemToUART.data_len = 2;

        if (xQueueSend(UARTQueue, &itemToUART, 1) == pdPASS)
        {
            ESP_LOGI("UART", "Written to queue clear fault");
        }
    }
    else if(isTopicEqual(&topic[strlen(topic_cmp)], "get_fault"))
    {
        ESP_LOGI("MQTT_TOPIC", "Get command: %s", topic);
        itemToUART.data[0] = idCmd;
        itemToUART.data[1] = idCmdGetFaults;
        itemToUART.data_len = 2;

        if (xQueueSend(UARTQueue, &itemToUART, 1) == pdPASS)
        {
            ESP_LOGI("UART", "Written to queue get fault");
        }
    }
    else if(isTopicEqual(&topic[strlen(topic_cmp)], "get_link_diag"))
    {
        ESP_LOGI("MQTT_TOPIC", "Get command: %s", topic);
        itemToUART.data[0] = idCmd;
        itemToUART.data[1] = idCmdGetLinkDiag;
        itemToUART.data_len = 2;

        if (xQueueSend(UARTQueue, &itemToUART, 1) == pdPASS)
        {
            ESP_LOGI("UART", "Written to queue get link diagnostic");
        }
        sendLinkDiagReport();
    }
}
/*
 * @brief Event handler registered to receive MQTT events
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */
char data_buf[32];

void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
  //  ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        bMQTTConnected = true;
    /*
        msg_id = esp_mqtt_client_publish(client, "/topic/qos1", "data_3", 0, 1, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_subscribe(client, "/topic/qos0", 0);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_subscribe(client, "/topic/qos1", 1);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_unsubscribe(client, "/topic/qos1");
        ESP_LOGI(TAG, "sent unsubscribe successful, msg_id=%d", msg_id);*/
        esp_mqtt_client_subscribe(client, MQTT_TOPIC_WIDEREQUEST, 0);
        subscribe_all_topics(actualReport.ID);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        bMQTTConnected = false;
        esp_settings.subscribed = false;
    
        break;

    case MQTT_EVENT_SUBSCRIBED:
      /*  ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);*/
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
   //     ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
   //     ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);

    
   
        memset(data_buf,0,32);
        strncpy(data_buf, event->data, sizeof(data_buf));
        process_topic(event->topic, data_buf, event->data_len);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
      /*  if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));

        }*/
        break;
    default:
        //ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

void mqtt_app_start(char* broker)
{
    esp_err_t err=0;
    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = broker,
    };

    ESP_LOGI(TAG,"STARTING MQTT");
    client = esp_mqtt_client_init(&mqtt_cfg);
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    err = esp_mqtt_client_start(client);

    ESP_LOGI(TAG,"CLIENT START STATUS: %x",err);

}
