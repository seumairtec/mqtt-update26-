#include <stdint.h>
#include <stdbool.h>

#ifndef _COMMON_
#define _COMMON_

#define idCmd 0
#define idReadReg 1
#define idWriteReg 2

#define idCmdStat 0
#define idCmdRPM 1
#define idCmdPIN 2
#define idCmdClearFaults 3
#define idCmdGetFaults 4
#define idCmdGetParams 5
#define idCmdGetID	6
#define idCmdChangePIN 7
#define idCmdGetLinkDiag 8
#define idCmdResetAtmega 9

/*#define idRegRatedRPM 0
#define idRegRetedCurrent  1
#define idRegRumpUp  2
#define idRegRumpDown  3
#define idRegDir  4
#define idRegHallSensing  5
#define idRegMinTemp  6
#define idRegMaxTemp  7
#define idRegSuppression1low  8
#define idRegSuppression1high  9
#define idRegSuppression2low 10
#define idRegSuppression2high  11
#define idRegSuppression3low  12
#define idRegSuppression3high  13*/

#define ITEM_ARRAY_SIZE 64

#define idRegRatedRPM 0
#define idRegMBPollPeriod  1
#define idRegSensorID  2
#define idRegVSP  3
#define idRegDir  4
#define idRegMBMaster  5
#define idRegMinTemp  6
#define idRegMaxTemp  7
#define idRegMinHum	8
#define idRegMaxHum 9
#define idRegSuppression1low  10
#define idRegSuppression1high  11
#define idRegSuppression2low 12
#define idRegSuppression2high  13
#define idRegSuppression3low  14
#define idRegSuppression3high  15
#define idRegVSPTop 16
#define idRegVSPBottom 17
#define idRegSlavesCnt 18
#define idRegCurrentLim 19

#define RPMTopScale 16383.0
//---------- queue item --------------
struct queue_item_t;
//---------- queue item --------------
typedef struct{
    uint8_t data[ITEM_ARRAY_SIZE];
    uint8_t data_len;
} queue_item_t;

typedef struct{
    uint8_t ID;
    uint16_t faults;
    uint16_t taskRPM;
    uint16_t RPM;
    uint16_t current;
    uint16_t voltage;
    int16_t temperature;
    uint16_t VSP;
    uint8_t link;
    uint8_t sequencer;
    int16_t humidity;
    bool atmegaLink;
    uint16_t diagState1;
    uint16_t diagState2;
    uint16_t diagState3;
    uint16_t diagState4;
}status_data_t;

typedef struct{
    int16_t max_rpm;
    int16_t max_current;
    char pin[10];
    int user_level;
    int report_interval; //ms
    bool subscribed;
}esp_settings_t;

void setparam(int i, int16_t value);
int16_t getparam(int i);

#endif
