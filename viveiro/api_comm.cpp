#include "Arduino.h"
#include "esp32-hal.h"
#include "api_comm.hpp"


bool ApiComm::initApiComm(HardwareSerial &serialObj, Credentials &wifiObj, Credentials &apiObj, ApiLinks &links)
{
  serial = &serialObj;
  wifiAuth = &wifiObj;
  apiAuth = &apiObj;
  apiLinks = &links;
  if(!initWifi()) return false;
  loadWebTime();
  if(!tokenUpdate()) return false;
  return true;
}

bool ApiComm::initWifi()
{
  WiFi.begin(wifiAuth->login,wifiAuth->password);
  //serial->println("Conectando");
  uint64_t initMillis = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    serial->println(".");
    if(millis() - initMillis > maxWifiReconnectTime) return false;
  }
  //serial->println("");
  //serial->print("Conectado à rede WiFi com o endereço IP: ");
  serial->println(WiFi.localIP());
  return true;
}

bool ApiComm::checkAndReconnectWiFi()
{
  if (WiFi.status() != WL_CONNECTED) 
  {
    serial->println("ReconnectWiFi");
    if(!initWifi()) return false;
  }
  return true;
}
void ApiComm::turnOffWifi()
{
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
}

bool ApiComm::httpPost(String &link, String &data)
{
  HTTPClient http;

  uint64_t initMillis = millis();
  int responseCode = -1;

  http.begin(link);
  http.setConnectTimeout(10000);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", token.c_str());

  while(1)
  {
    responseCode = http.POST(data);

    if(responseCode == 401 || responseCode == 403) 
    {
      tokenUpdate(true);
    }

    if (responseCode == 201)
    {
      http.end(); 
      return true;
    }

    if(responseCode != -1) 
    {
      break;
    }

    if(millis()-initMillis> maxReconnectTime) 
    {
      break;
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
  http.end();
  return false;
}

String ApiComm::httpGet(String &link)
{
  int responseCode = -1;
  uint64_t initMillis = millis();

  HTTPClient http;

  http.begin(link);
  http.setConnectTimeout(10000);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", token.c_str());

  while(1)
  {
    //serial->println(">Buscando Estado Valvula<");
    int responseCode = http.GET();

    //serial->print("responseCode: ");
    //serial->println(responseCode);

    if(responseCode == 401 || responseCode == 403) 
    {
      tokenUpdate(true);
    }

    if (responseCode == 200) 
    {
      String payload = http.getString();
      http.end(); 
      return payload; 
    }

    if(millis()-initMillis > maxReconnectTime) 
    {
      break;
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
  http.end(); 
  return defaultResponse;
}


bool ApiComm::tokenUpdate(bool ignoreTimeTokenUpdade)
{
  static bool isFirstFunctionCall = true;
  
  if(!isFirstFunctionCall)
  {
    uint64_t currentMillis = millis();
    if(!(currentMillis-lastMillisTokenUpdate >= timeTokenUpdade) || !ignoreTimeTokenUpdade)
    { 
      return true;
    }
  }

  if (WiFi.status() != WL_CONNECTED) 
  {
    return false;
  }

  DynamicJsonDocument loginApi(128);

  loginApi["username"] = apiAuth->login;
  loginApi["password"] = apiAuth->password;

  String jsonStringAuth;
  serializeJson(loginApi, jsonStringAuth);
  jsonStringAuth.trim();

  HTTPClient http;

  uint64_t initMillis = millis();
  int responseCode = -1;

  while(1)
  {
    http.begin(apiLinks->linkToAuthenticate);
    http.setConnectTimeout(10000);
    http.addHeader("Content-Type", "application/json");

    const char *headerKeys[] = {"Authorization"};
    const size_t headerKeysCount = sizeof(headerKeys) / sizeof(headerKeys[0]);
    
    http.collectHeaders(headerKeys, headerKeysCount);

    responseCode = http.POST(jsonStringAuth);

    if (responseCode == 200) 
    {
      String payload = http.getString(); 
      token = http.header("Authorization");
      http.end();
      lastMillisTokenUpdate = millis();
      isFirstFunctionCall = false;
      serial->println("apiTokenOK");
      return true;
    }
    if(responseCode != -1) break;

    if(millis()-initMillis> maxReconnectTime) 
    {
      break;
    }
    delay(20);
  }
  http.end();
  return false;
}

bool ApiComm::sendAllSensorsData(Sensor humi[], Sensor temp[], size_t sizeArrayHumi, size_t sizeArrayTemp)
{
  if (WiFi.status() != WL_CONNECTED) 
  {
    return false;
  }

  if(!tokenUpdate()) 
  { 
    return false;
  }

  size_t jsonArraySize = sizeArrayTemp + sizeArrayHumi;
  const size_t capacity = JSON_ARRAY_SIZE(jsonArraySize) + jsonArraySize * JSON_OBJECT_SIZE(2);

  DynamicJsonDocument dataSensors(capacity);
  JsonArray sensorsArray = dataSensors.to<JsonArray>();

  for (int i = 0; i < sizeArrayTemp; i++) 
  { 
    JsonObject sensor = sensorsArray.createNestedObject(); 
    sensor["sensorId"] = temp[i].id; 
    sensor["value"] = temp[i].sensorValue;
  }

  for (int i = 0; i < sizeArrayHumi; i++) 
  { 
    JsonObject sensor = sensorsArray.createNestedObject(); 
    sensor["sensorId"] = humi[i].id; 
    sensor["value"] = humi[i].sensorValue;
  }

  String jsonStringdataSensors;
  serializeJson(dataSensors, jsonStringdataSensors);
  jsonStringdataSensors.trim();

  return httpPost(apiLinks->linkToSensorsReading , jsonStringdataSensors);
}

int ApiComm::getValveState()
{
  if (WiFi.status() != WL_CONNECTED) 
  {
    return -1;
  }
  
  if(!tokenUpdate()) 
  { 
    return -1;
  }

  String payload = httpGet(apiLinks->linkToValveState);

  if(payload == "true") 
  {
    return 1;
  }
  if(payload == "false") 
  {
    return 0;
  }

  return -1;
}
bool ApiComm::searchForIrrigationTime(TimeIrrigation timeIrragation[])
{
  if (WiFi.status() != WL_CONNECTED) 
  {
    return false;
  }

  if(!tokenUpdate()) 
  { 
    return false;
  }

  String payload = httpGet(apiLinks->linkToTimeValve);

  if(payload == defaultResponse)
  {
    return false;
  }

  int i = 0;
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, payload);

  for (JsonObject obj : doc.as<JsonArray>()) 
  {
    if(i>=maxIrrigationSlots) break;

    String initialTime = obj["initialTime"];
    String finalTime = obj["finalTime"];

    passStringToTm(timeIrragation[i].initialTime, initialTime);
    passStringToTm(timeIrragation[i].finalTime, finalTime);
    i++;
  }

  return true;
}
bool ApiComm::sendWaterVolume(double &volumeRead)
{
  if (WiFi.status() != WL_CONNECTED) 
  {
    return false;
  }

  if(!tokenUpdate()) 
  { 
    return false;
  }

  DynamicJsonDocument jsonSensor(128);
  jsonSensor["value"] = volumeRead;
  String waterVolume;
  serializeJson(jsonSensor, waterVolume);
  waterVolume.trim();

  return httpPost(apiLinks->linkToWaterFlow , waterVolume);
}

void ApiComm::passStringToTm(struct tm &tmStruct, String &time)
{
  tmStruct.tm_hour = time.substring(0, 2).toInt(); //hh:mm:ss  -> ignore second
  tmStruct.tm_min = time.substring(3, 5).toInt(); 
}

void ApiComm::loadWebTime()
{
  if (WiFi.status() != WL_CONNECTED) 
  {
    return;
  }
  configTime(3600*timezone, daysavetime*3600, "time.nist.gov", "0.pool.ntp.org", "1.pool.ntp.org");
}