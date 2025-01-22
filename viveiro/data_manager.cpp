#include "data_manager.hpp"

void DataManager::clearNvs()
{
  nvs_flash_erase();
  nvs_flash_init();
}

bool DataManager::storageIDsData(char masterKey[], Sensor sensor[])
{
  if (!nvs.begin(masterKey, false)) // false -> writing and read
  {
    return false;
  }
  for(int i = 0; i<numModules; i++)
  {
    nvs.putInt(keys[i].c_str(), sensor[i].id);
  }
  nvs.end();

  return true;
}

bool DataManager::storageHumiCalibrationData(Sensor sensor[])
{
  if (!nvs.begin("calibrationMax", false)) // false -> write and read
  {
    return false;
  }
  for(int i = 0; i<numModules; i++)
  {
    nvs.putInt(keys[i].c_str(), sensor[i].maxValueAdc);
  }
  nvs.end();

  if (!nvs.begin("calibrationMin", false)) // false -> write and read
  {
    return false;
  }
  for(int i = 0; i<numModules; i++)
  {
    nvs.putInt(keys[i].c_str(), sensor[i].minValueAdc);
  }
  nvs.end();

  return true;
}

bool DataManager::storageCredentials(Credentials &credentials, char key[])
{
  if (!nvs.begin(key, false)) // false -> write and read
  {
    return false;
  }
  nvs.putString(keyLogin, credentials.login);
  nvs.putString(keyPassword, credentials.password);
  
  nvs.end();

  return true;
}

bool DataManager::storageApiLinks(ApiLinks &apiLinks)
{
  if (!nvs.begin(masterkeyLinksApi , false)) // false -> write and read
  {
    return false;
  }

  nvs.putString(keylinkToAuthenticate, apiLinks.linkToAuthenticate);
  nvs.putString(keylinkToSensorsReading, apiLinks.linkToSensorsReading);
  nvs.putString(keylinkToValveState, apiLinks.linkToValveState);
  nvs.putString(keylinkToTimeValve, apiLinks.linkToTimeValve);
  nvs.putString(keylinkToWaterFlow, apiLinks.linkToWaterFlow);

  nvs.end();

  return true;
}

bool DataManager::storageWaterFlow(uint64_t &flow)
{
  if(!nvs.begin(masterKeyWaterFlowSensor, false)) // false -> write and read
  {
    return false;
  }
  nvs.putULong64(keyFlowValue, flow);

  nvs.end();
  return true;
}

bool DataManager::storageIrrigationSchedules(TimeIrrigation timeIrrigation[])
{
  if (!nvs.begin(masterKeyIrrigation, false)) // false -> writing and read
  {
    return false;
  }
  for(int i = 0; i<maxIrrigationSlots; i++)
  {
    char buffer[12];
    sprintf(buffer, "%02d:%02d;%02d:%02d",timeIrrigation[i].initialTime.tm_hour, timeIrrigation[i].initialTime.tm_min, timeIrrigation[i].finalTime.tm_hour, timeIrrigation[i].finalTime.tm_min);
    nvs.putString(keys[i].c_str(), buffer);
  }
  nvs.end();
  return true;
}

//====================================================================

bool DataManager::loadIDsData(char masterKey[], Sensor sensor[])
{
  if (!nvs.begin(masterKey, true))
  {
    return false;
  }
  for(int i = 0; i<numModules; i++)
  {
    sensor[i].id = nvs.getInt(keys[i].c_str(), 0);
  }
  nvs.end();

  return true;
}

bool DataManager::loadHumiCalibrationData(Sensor sensor[])
{
  if (!nvs.begin("calibrationMax", true)) 
  {
    return false;
  }
  for(int i = 0; i<numModules; i++)
  {
   sensor[i].maxValueAdc = nvs.getInt(keys[i].c_str(), 0);
  }
  nvs.end();

  if (!nvs.begin("calibrationMin", true)) // false -> write and read
  {
    return false;
  }
  for(int i = 0; i<numModules; i++)
  {
    sensor[i].minValueAdc = nvs.getInt(keys[i].c_str(), 4095);
  }
  nvs.end();

  return true;
}

bool DataManager::loadCredentials(Credentials &credentials, char key[])
{
  if (!nvs.begin(key, true))
  {
    return false;
  }
  credentials.login = nvs.getString(keyLogin, "");
  credentials.password = nvs.getString(keyPassword, "");
  
  nvs.end();

  return true;
}

bool DataManager::loadApiLinks(ApiLinks &apiLinks)
{
  if (!nvs.begin(masterkeyLinksApi , true))
  {
    return false;
  }

  apiLinks.linkToAuthenticate = nvs.getString(keylinkToAuthenticate, "");
  apiLinks.linkToSensorsReading = nvs.getString(keylinkToSensorsReading, "");
  apiLinks.linkToValveState = nvs.getString(keylinkToValveState, "");
  apiLinks.linkToTimeValve = nvs.getString(keylinkToTimeValve, "");
  apiLinks.linkToWaterFlow = nvs.getString(keylinkToWaterFlow, "");

  nvs.end();

  return true;
}

bool DataManager::loadWaterFlow(uint64_t &flow)
{
  if(!nvs.begin(masterKeyWaterFlowSensor, true))
  {
    return false;
  }
  
  flow = nvs.getULong64(keyFlowValue, 0);

  nvs.end();

  return true;
}

bool DataManager::loadIrrigationSchedules(TimeIrrigation timeIrrigation[])
{
  if (!nvs.begin(masterKeyIrrigation, true)) 
  {
    return false;
  }
  for(int i = 0; i<maxIrrigationSlots; i++)
  {
    String scheduleString = nvs.getString(keys[i].c_str(), "");
    
    if (scheduleString.length()> 0)
    {
      sscanf(scheduleString.c_str(),  "%02d:%02d;%02d:%02d", &timeIrrigation[i].initialTime.tm_hour, &timeIrrigation[i].initialTime.tm_min, &timeIrrigation[i].finalTime.tm_hour, &timeIrrigation[i].finalTime.tm_min);
    }
  }
  nvs.end();

  return true;
}

bool DataManager::loadTempIDs(Sensor sensorT[]) {
  return loadIDsData(masterkeyTemp, sensorT);
}

bool DataManager::loadHumiIDs(Sensor sensorH[]) {
  return loadIDsData(masterkeyHumi, sensorH);
}

bool DataManager::loadHumiCalibration(Sensor sensorH[]) {
  return loadHumiCalibrationData(sensorH);
}

bool DataManager::loadWiFiCredentials(Credentials &wifi) {
  return loadCredentials(wifi, masterkeyWifi);
}

bool DataManager::loadApiCredentials(Credentials &api) {
  return loadCredentials(api, masterkeyApi);
}

bool DataManager::loadApiLinkData(ApiLinks &apiLinks) {
  return loadApiLinks(apiLinks);
}
bool DataManager::loadIrrigationSchedulesData(TimeIrrigation timeIrrigation[])
{
  return loadIrrigationSchedules(timeIrrigation);
}

bool DataManager::areSchedulesEqual(TimeIrrigation savedSchedules[], TimeIrrigation apiSchedules[])
{
  for (int i = 0; i < maxIrrigationSlots; i++) 
  {
    if (savedSchedules[i].initialTime.tm_hour != apiSchedules[i].initialTime.tm_hour) 
    {
      return false;    
    }
    if (savedSchedules[i].initialTime.tm_min != apiSchedules[i].initialTime.tm_min)
    { 
      return false;
    }

    if (savedSchedules[i].finalTime.tm_hour != apiSchedules[i].finalTime.tm_hour)
    {
      return false;
    }
    if (savedSchedules[i].finalTime.tm_min != apiSchedules[i].finalTime.tm_min) 
    {
      return false;
    }
  }
  return true; 
}


bool DataManager::loadAllData(Sensor sensorH[], Sensor sensorT[], Credentials &wifi, Credentials &api, ApiLinks &apiLinks, TimeIrrigation timeIrrigation[]) 
{
  bool nvsOK = true;

  if (!loadTempIDs(sensorT))
  {
    nvsOK = false;
  }

  if (!loadHumiIDs(sensorH))
  {
    nvsOK = false;
  }

  if (!loadHumiCalibration(sensorH))
  {
    nvsOK = false;
  }

  if (!loadWiFiCredentials(wifi))
  {
    nvsOK = false;
  }

  if (!loadApiCredentials(api))
  {
    nvsOK = false;
  }

  if (!loadApiLinkData(apiLinks))
  {
    nvsOK = false;
  }

  if(!loadIrrigationSchedulesData(timeIrrigation))
  {
    nvsOK = false;
  }

  return nvsOK;  // Retorna true se todas as operações foram bem-sucedidas, false se alguma falhar
}

bool DataManager::storeTempIDs(Sensor sensorT[]) {
  return storageIDsData(masterkeyTemp, sensorT);
}
bool DataManager::storeHumiIDs(Sensor sensorH[]) {
  return storageIDsData(masterkeyHumi, sensorH);
}

bool DataManager::storeHumiCalibration(Sensor sensorH[]) {
  return storageHumiCalibrationData(sensorH);
}

bool DataManager::storeWiFiCredentials(Credentials &wifi) {
  return storageCredentials(wifi, masterkeyWifi);
}

bool DataManager::storeApiCredentials(Credentials &api) {
  return storageCredentials(api, masterkeyApi);
}

bool DataManager::storeApiLinkData(ApiLinks &apiLinks) {
  return storageApiLinks(apiLinks);
}

bool DataManager::storeWaterFlowData(uint64_t &flow) {
  return storageWaterFlow(flow);
}

bool DataManager::compareAndStoreIrrigationSchedulesData(TimeIrrigation savedSchedules[], TimeIrrigation apiSchedules[])
{
  if(!areSchedulesEqual(savedSchedules, apiSchedules))
  {
    if(storageIrrigationSchedules(apiSchedules))
    {
      for(int i = 0; i<maxIrrigationSlots; i++)
      {
        savedSchedules[i] = apiSchedules[i];
      }
      return true;
    }
  }
  return false;
}
void DataManager::clearSchedulesArray(TimeIrrigation timeIrrigation[])
{
  TimeIrrigation clearTimeIrrigation = {};
  for(int i = 0; i<maxIrrigationSlots; i++)
  {
    timeIrrigation[i] = clearTimeIrrigation;
  }
}

