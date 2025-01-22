#ifndef _API_COMM_HPP_
#define _API_COMM_HPP_

#include "HardwareSerial.h"
#include "data_types.hpp"

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>

extern const int maxIrrigationSlots;

class ApiComm {
  private:
    String token;
    Credentials *wifiAuth, *apiAuth;
    ApiLinks *apiLinks;

    HardwareSerial *serial;

    long timezone = -4;
    byte daysavetime = 1;

    const unsigned long maxReconnectTime = 60000;
    const unsigned long maxWifiReconnectTime = 30000;

    const unsigned long timeTokenUpdade = 2700000;
    unsigned long lastMillisTokenUpdate = 0;
    
    const String defaultResponse = "unresponsive";
  
    bool initWifi();
    bool tokenUpdate(bool ignoreTimeTokenUpdade = false);   
    void passStringToTm(struct tm &tmStruct, String &time);
    bool httpPost(String &link, String &data);
    String httpGet(String &link);
  public:
    bool initApiComm(HardwareSerial &serialObj, Credentials &wifiObj, Credentials &apiObj, ApiLinks &links); // serialObj need for DEBUG
    bool sendAllSensorsData(Sensor humi[], Sensor temp[], size_t sizeArrayHumi, size_t sizeArrayTemp);
    int getValveState();
    bool searchForIrrigationTime(TimeIrrigation timeIrragation[]);
    void loadWebTime();
    bool sendWaterVolume(double &volumeRead);
    bool checkAndReconnectWiFi();
    void turnOffWifi();
};

#endif