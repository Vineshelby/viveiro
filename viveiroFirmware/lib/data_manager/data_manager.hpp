#ifndef _DATA_MANAGER_
#define _DATA_MANAGER_

#include <Preferences.h>
#include <nvs_flash.h>
#include "esp_system.h"
#include <Arduino.h>

#include "serial_io_manager.hpp"
#include "data_types.hpp"

extern const int numModules;
extern const int maxIrrigationSlots;

class DataManager
{
  private:
    char masterkeyHumi[5] = "humi";
    char masterkeyTemp[5] = "temp";
    char masterkeyWifi[5] = "wifi";
    char masterkeyApi[4] = "api";
    char masterkeyLinksApi[9] = "apiLinks";
    char masterKeyWaterFlowSensor[11] = "flowSensor";
    char masterKeyIrrigation[15] = "timeIrrigation";
    char keyLogin[6] = "login";
    char keyPassword[9] = "password";
    
    char keylinkToAuthenticate[13] = "authenticate";
    char keylinkToSensorsReading[14] = "sensorReading";
    char keylinkToValveState[6] = "valve";
    char keylinkToWaterFlow[10] = "waterFlow";
    char keylinkToTimeValve[10] = "timeValve";
    char keyFlowValue[10] = "FlowValue";
    String keys[10] = {"k1", "k2", "k3", "k4", "k5", "k6", "k7","k8","k9","k10"}; //for data arrays,  max = 10;

    Preferences nvs;
  
    bool storageIDsData(char masterKey[], Sensor sensor[]);
    bool storageHumiCalibrationData(Sensor sensor[]);
    bool storageCredentials(Credentials &credentials, char key[]);
    bool storageWaterFlow(uint64_t &flow);
    bool storageApiLinks(ApiLinks &apiLinks);
    bool storageIrrigationSchedules(TimeIrrigation timeIrrigation[]);

    bool loadIDsData(char masterKey[], Sensor sensor[]);
    bool loadHumiCalibrationData(Sensor sensor[]);
    bool loadCredentials(Credentials &credentials, char key[]);
    bool loadWaterFlow(uint64_t &flow); //consultar pessoal do front;
    bool loadApiLinks(ApiLinks &apiLinks);
    bool loadIrrigationSchedules(TimeIrrigation timeIrrigation[]);
    bool areSchedulesEqual(TimeIrrigation savedSchedules[], TimeIrrigation apiSchedules[]);
  public:
    bool loadTempIDs(Sensor sensorT[]);
    bool loadHumiIDs(Sensor sensorH[]);
    bool loadHumiCalibration(Sensor sensorH[]);
    bool loadWiFiCredentials(Credentials &wifi);
    bool loadApiCredentials(Credentials &api);
    bool loadApiLinkData(ApiLinks &apiLinks);
    bool loadIrrigationSchedulesData(TimeIrrigation timeIrrigation[]);

    bool loadAllData(Sensor sensorH[], Sensor sensorT[], Credentials &wifi, Credentials &api, ApiLinks &apiLinks, TimeIrrigation timeIrrigation[]);

    bool storeTempIDs(Sensor sensorT[]);
    bool storeHumiIDs(Sensor sensorH[]);
    bool storeHumiCalibration(Sensor sensorH[]);
    bool storeWiFiCredentials(Credentials &wifi);
    bool storeApiCredentials(Credentials &api);
    bool storeApiLinkData(ApiLinks &apiLinks);
    bool storeWaterFlowData(uint64_t &flow);
    bool compareAndStoreIrrigationSchedulesData(TimeIrrigation savedSchedules[], TimeIrrigation apiSchedules[]);

    void clearSchedulesArray(TimeIrrigation timeIrrigation[]);
    void clearNvs();
};
#endif