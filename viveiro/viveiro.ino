/*
 *  @file Viveiro.ino
 *  @author Marcus Vinícius (marcaovini07@gmail.com)
 *  @brief Gerenciar Placa Projeto Viveiro
 *  @version 0.8
 *  @date 22-10-224
 *
 */
#include "esp_system.h"
#include <esp_task_wdt.h>
#include <Preferences.h>
#include <nvs_flash.h>
#include <time.h>
#include <atomic>

#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h" 
#include <rtc_wdt.h>

#include "serial_io_manager.hpp"
#include "data_manager.hpp"
#include "data_types.hpp"
#include "api_comm.hpp"
#include "peripheral_control.hpp"

const esp_task_wdt_config_t configWDTtask = {30000,true};


const int numModules = 5; //global reference
const int maxIrrigationSlots = 10;  //global reference

SemaphoreHandle_t xMutexIrrigationData = nullptr;
SemaphoreHandle_t xMutexSensorData = nullptr;
SemaphoreHandle_t xMutexCriticalApiSend = nullptr;

TimerHandle_t irrigationScheduleUpdateTimer = nullptr;
TimerHandle_t sensorDataSendingTimer = nullptr;
//TimerHandle_t resetTimer = nullptr;
TimerHandle_t apiValveTimer = nullptr;

std::atomic<bool> flagScheduleCheck = {0};
std::atomic<bool> flagSendSensors = {0};
std::atomic<bool> flagSendFlow = {0};
std::atomic<bool> flagRestartPermission = {0};
std::atomic<bool> flagCheckValveStatusApi = {0};
std::atomic<bool> valveActivated = {0};
std::atomic<bool> hourUnavailable = {1};

const uint32_t systemCheckTime = 5000;

//all times in ms
const uint32_t timeBetweenSensorReads = 10000;
const uint32_t valveStateCheckInterval = 10000;
const uint32_t delayTaskCommunication = 10000;

const uint32_t timeSendSensorReadingsApi = 180000;
const uint32_t timeToCheckAPiIrrigationSchedules = 3600000;
const uint32_t timeCheckValveStatusApi = 60000;
const uint32_t minSystemRestartTime = 21600000;

struct tm currentTime;

Peripheral sensorsDevices;
SerialIOManager serialIOManager(&Serial, &sensorsDevices);
DataManager dataManager;
ApiComm apiClient;

Credentials wifiCredentials = {}, apiCredentials = {};
ApiLinks apiLinks = {};
Sensor humiSensors[numModules] = {{}}, tempSensors[numModules] = {{}};
TimeIrrigation irrigationSchedulesNvs[maxIrrigationSlots] = {{}};
TimeIrrigation irrigationSchedulesApi[maxIrrigationSlots] = {{}};

void setup()
{
  serialIOManager.begin(115200);
  sensorsDevices.initPeripheral();

  if(serialIOManager.waitforPowerMode()) settings(); //use pin configuration in production

  if(!dataManager.loadAllData(humiSensors, tempSensors, wifiCredentials, apiCredentials, apiLinks, irrigationSchedulesNvs)) Serial.println("nvs_fail");

  xMutexIrrigationData = xSemaphoreCreateMutex();
  xMutexSensorData = xSemaphoreCreateMutex();
  xMutexCriticalApiSend = xSemaphoreCreateMutex();

  irrigationScheduleUpdateTimer = xTimerCreate("scheduleUpdate", pdMS_TO_TICKS(timeToCheckAPiIrrigationSchedules), pdTRUE, (void *) 1, timerCallbackScheduleUpdate);
  sensorDataSendingTimer = xTimerCreate("sensorSending", pdMS_TO_TICKS(timeSendSensorReadingsApi), pdTRUE, (void *) 2, timerCallbacksensorSending);
  //resetTimer = xTimerCreate("espRestart", minSystemRestartTime, pdFALSE, (void*) 3, timerCallbackReset);
  apiValveTimer = xTimerCreate("valveCheck", pdMS_TO_TICKS(timeCheckValveStatusApi), pdTRUE,(void*) 4, timerCallbackValveState);

  xTimerStart(irrigationScheduleUpdateTimer, 0);
  xTimerStart(sensorDataSendingTimer, 0);
  //xTimerStart(resetTimer, 0);
  xTimerStart(apiValveTimer,0);

  rtc_wdt_protect_off();      //Disable RTC WDT write protection
  rtc_wdt_disable(); 
  rtc_wdt_set_stage(RTC_WDT_STAGE0, RTC_WDT_STAGE_ACTION_RESET_RTC); //Set stage 0 to trigger a system reset after 1000ms
  rtc_wdt_set_time(RTC_WDT_STAGE0, 60000); // time for reset
  rtc_wdt_enable();           //Start the RTC WDT timer
  rtc_wdt_protect_on();       //Enable RTC WDT write protection

  xTaskCreatePinnedToCore(
    taskApiCommunication,   
    "apiTask",             
    10000,                   
    NULL,                  
    1,                      // Priority
    NULL,                   // Handle
    0                       // Core
  );
  xTaskCreatePinnedToCore(
    taskValve,  
    "valveTask",             
    4096,                      
    NULL,                      
    1,                         // Priority
    NULL,                      // Handle
    1                          // Core
  );
  xTaskCreatePinnedToCore(
    taskReadSensors,  
    "sensorsTask",             
    4096,                      
    NULL,                      
    1,                         // Priority
    NULL,                      // Handle
    1                          // Core
  );
  xTaskCreatePinnedToCore(
    taskSystemMaintenance,  
    "systemMaintenance",             
    4096,                      
    NULL,                      
    2,                         // Priority
    NULL,                      // Handle
    1                          // Core
  );
}

void loop()
{
  delay(10);
}

void taskReadSensors(void *pvParameters) {
  const TickType_t delayBetweenSensorReads = pdMS_TO_TICKS(timeBetweenSensorReads);
  //usando uma task só pra isso com a possibilidade de fazer uma média móvel/gaussiana ou algo do tipo,
  //caso não, as funções de leitura podem ser chamadas na task api antes do momento de envio
  for(;;) 
  {
    if (xSemaphoreTake(xMutexSensorData, portMAX_DELAY)) 
    {
      Serial.println("sensors_read");
      
      sensorsDevices.loadTempSensor(tempSensors, numModules); 
      sensorsDevices.loadHumiSensor(humiSensors, numModules); 

      xSemaphoreGive(xMutexSensorData);
    }
    vTaskDelay(delayBetweenSensorReads);
  }
}

void taskValve(void *pvParameters) {
  const TickType_t stateDelay = pdMS_TO_TICKS(valveStateCheckInterval);
  bool valveState = false;
  bool lastValveState = false;
  int apiResponse = -1;
  bool timeOK = false;


  for(;;)
  {
    if(!timeOK)
    {
      if(hourUnavailable.load()) timeOK = true;
    }

    if(timeOK)
    {
      if(xSemaphoreTake(xMutexIrrigationData, portMAX_DELAY))
      {
        valveState = checkValveStatusIrrigationSchedules();
        xSemaphoreGive(xMutexIrrigationData);
        if(valveState) 
        { 
          Serial.println("schTurnON");
        }
        else
        {
          Serial.println("schTurnOFF");
        }
      }   

      if(valveState == false && lastValveState == true)
      {
        flagSendFlow.store(true);  
      }

      lastValveState = valveState;
      
      valveActivated.store(valveState);
      sensorsDevices.powerValve(valveState);
    }
    vTaskDelay(stateDelay);
  }
}

void taskApiCommunication(void *pvParameters)
{
  const TickType_t referenceDelay = pdMS_TO_TICKS(delayTaskCommunication);
  Serial.println(esp_task_wdt_reconfigure(&configWDTtask));
  bool firstExecution = true;
  bool inconsistentIrrigationSchedules = false;
  int valveStateApi = -1;
  int lastValveStateApi = -1;

  Serial.print("initApi");
  if(apiClient.initApiComm(*(serialIOManager.serial), wifiCredentials, apiCredentials, apiLinks)) 
  {
    Serial.println("initApi_OK");
  }
  else
  {
    Serial.println("initApi_Fail");
  }
  hourUnavailable.store(!getLocalTime(&currentTime, 5000));

  sensorsDevices.loadTempSensor(tempSensors, numModules);
  sensorsDevices.loadHumiSensor(humiSensors, numModules);
  
  Serial.println("senAllSensorsData_first");
  apiClient.sendAllSensorsData(humiSensors, tempSensors, numModules, numModules);

  if(apiClient.searchForIrrigationTime(irrigationSchedulesApi))
  {
    dataManager.compareAndStoreIrrigationSchedulesData(irrigationSchedulesNvs, irrigationSchedulesApi);
  }

  for(;;)
  {
    if(xSemaphoreTake(xMutexCriticalApiSend,portMAX_DELAY))
    {
      if(flagCheckValveStatusApi.load())
      {

        if(hourUnavailable.load())
        {
          Serial.println("hourUnavailable_newAttempt");
          apiClient.loadWebTime();
          vTaskDelay(referenceDelay);
          hourUnavailable.store(!getLocalTime(&currentTime, 5000));
        }

        flagCheckValveStatusApi.store(false);

        apiClient.checkAndReconnectWiFi();

        valveStateApi = apiClient.getValveState();
        vTaskDelay(pdMS_TO_TICKS(100));

        if(valveStateApi == -1) 
        {
          Serial.println("apiNotResponse");
        }
        else
        {
          Serial.println("apiValveState");
        }

        if(valveStateApi >= 0)
        {
          bool schedulesStatus;
          
          if(xSemaphoreTake(xMutexIrrigationData, portMAX_DELAY))
          {
            schedulesStatus = checkValveStatusIrrigationSchedules();
            xSemaphoreGive(xMutexIrrigationData);
          }

          if(!firstExecution && schedulesStatus!=valveStateApi) 
          {
            if(schedulesStatus!=lastValveStateApi)
            { 
              inconsistentIrrigationSchedules = true;
              Serial.println("inconsistentIrrigationTime"); //debug
            }
          }
        }
        lastValveStateApi = valveStateApi;
        firstExecution = false;
      }
      
      if(flagSendFlow.load())
      {
        flagSendFlow.store(false);
        Serial.println("sendFlow");
        double waterVolume = sensorsDevices.getWaterVolume();
        apiClient.sendWaterVolume(waterVolume);
        sensorsDevices.resetWaterVolume();
      }
      
      if(flagSendSensors.load())
      {
        flagSendSensors.store(false);
        
        if(xSemaphoreTake(xMutexSensorData, portMAX_DELAY))
        {
          Serial.println("sendSensors");
          apiClient.sendAllSensorsData(humiSensors, tempSensors, numModules, numModules);
          xSemaphoreGive(xMutexSensorData);
        }
      }

      if(flagScheduleCheck.load() || inconsistentIrrigationSchedules)
      {
        flagScheduleCheck.store(false);

        if(inconsistentIrrigationSchedules) xTimerReset(irrigationScheduleUpdateTimer, 0);
        inconsistentIrrigationSchedules = false;      
        
        if(xSemaphoreTake(xMutexIrrigationData, portMAX_DELAY))
        {
          dataManager.clearSchedulesArray(irrigationSchedulesApi);
          if(apiClient.searchForIrrigationTime(irrigationSchedulesApi))
          {
            Serial.println("api_irrigationSchedules");
            dataManager.compareAndStoreIrrigationSchedulesData(irrigationSchedulesNvs, irrigationSchedulesApi);
          }
          
          Serial.print("Time:"); //debug
          Serial.print(currentTime.tm_hour);//debug
          Serial.print(":"); //debug
          Serial.println(currentTime.tm_min); //debug

          Serial.println("uptadeTime"); //debug

          apiClient.loadWebTime(); 
          hourUnavailable.store(!getLocalTime(&currentTime, 5000));
          
          Serial.print("New time:"); //debug
          Serial.print(currentTime.tm_hour); //debug
          Serial.print(":"); //debug
          Serial.println(currentTime.tm_min); //debug

          xSemaphoreGive(xMutexIrrigationData);
        }
      }
      xSemaphoreGive(xMutexCriticalApiSend);
    }
    vTaskDelay(referenceDelay);
  }
}

void taskSystemMaintenance(void *pvParameters)
{
  const TickType_t delayForCheck = pdMS_TO_TICKS(systemCheckTime);
  for(;;)
  {
    rtc_wdt_feed();
    vTaskDelay(delayForCheck);
  }
}

void timerCallbackScheduleUpdate(TimerHandle_t xTimer)
{
  flagScheduleCheck.store(true);
}

void timerCallbacksensorSending(TimerHandle_t xTimer)
{
  flagSendSensors.store(true);
}

void timerCallbackReset(TimerHandle_t xTimer)
{
  Serial.println("restartCommand");
  flagRestartPermission.store(true);
}

void timerCallbackValveState(TimerHandle_t xTimer)
{
  flagCheckValveStatusApi.store(true);
}

bool checkValveStatusIrrigationSchedules()
{
  getLocalTime(&currentTime, 5000);
  
  Serial.print("getLocalTime:"); // debug
  Serial.print(currentTime.tm_hour); // debug
  Serial.print(":"); // debug
  Serial.println(currentTime.tm_min); // debug

  int currentTimeMinutes = currentTime.tm_hour * 60 + currentTime.tm_min;

  for (int i = 0; i < maxIrrigationSlots; i++)
  {
    int initialTimeMinutes = irrigationSchedulesNvs[i].initialTime.tm_hour * 60 + irrigationSchedulesNvs[i].initialTime.tm_min;
    int finalTimeMinutes = irrigationSchedulesNvs[i].finalTime.tm_hour * 60 + irrigationSchedulesNvs[i].finalTime.tm_min;

    if (!(initialTimeMinutes == 0 && finalTimeMinutes == 0))
    {
      if (finalTimeMinutes < initialTimeMinutes)
      {
        if (currentTimeMinutes >= initialTimeMinutes || currentTimeMinutes < finalTimeMinutes)
        {
          return true;
        }
      }
      else
      {
        if (currentTimeMinutes >= initialTimeMinutes && currentTimeMinutes < finalTimeMinutes)
        {
          return true;
        }
      }
    }
  }
  return false;
}

void settings()
{
  Sensor tempSensorsToChange[numModules] = {{}};
  Sensor humiSensorsToChange[numModules] = {{}};
  Credentials wifiToChange = {};
  Credentials apiToChange = {};
  ApiLinks apiLinksToChange = {};
  TimeIrrigation irrigationSchedulesToChange[maxIrrigationSlots] = {{}};

  dataManager.loadAllData(humiSensorsToChange, tempSensorsToChange, wifiToChange, apiToChange, apiLinksToChange, irrigationSchedulesToChange);
  
  while(1)
  {
    int option;
    serialIOManager.menuConfig();

    option = serialIOManager.waitForInt(8, 0);

    switch(option)
    {
      case 1:
        serialIOManager.showAllData(humiSensorsToChange, tempSensorsToChange, wifiToChange, apiToChange, apiLinksToChange);
      break;
      
      case 2:
        if(serialIOManager.readUserIDs(humiSensorsToChange, INITIAL_TEXT_HUMI_IDs, IDS_HUMI_TEXT))
        {
          if(!dataManager.storeHumiIDs(humiSensorsToChange)) serialIOManager.errorNvs();
          if(!dataManager.loadHumiIDs(humiSensorsToChange)) serialIOManager.errorNvs();
        }
        else
        {
          serialIOManager.operationCancelled();
          if(!dataManager.loadHumiIDs(humiSensorsToChange)) serialIOManager.errorNvs();
        }
      break;

      case 3:
        if(serialIOManager.readUserIDs(tempSensorsToChange, INITIAL_IDS_TEMP_TEXT, IDS_TEMP_TEXT))
        {
          if(!dataManager.storeTempIDs(tempSensorsToChange)) serialIOManager.errorNvs();
          if(!dataManager.loadTempIDs(tempSensorsToChange)) serialIOManager.errorNvs();
        }
        else
        {
          serialIOManager.operationCancelled();
          if(!dataManager.loadTempIDs(tempSensorsToChange)) serialIOManager.errorNvs();
        }
      break;

      case 4:
        if(serialIOManager.readCredentials(wifiToChange, INITIAL_WIFI_TEXT, WIFI_TEXT, WIFI_CONFIRMATION_TEXT))
        {
          if(!dataManager.storeWiFiCredentials(wifiToChange)) serialIOManager.errorNvs();
          if(!dataManager.loadWiFiCredentials(wifiToChange)) serialIOManager.errorNvs();
        }
        else
        {
          serialIOManager.operationCancelled();
          if(!dataManager.loadWiFiCredentials(wifiToChange)) serialIOManager.errorNvs();
        }
      break;

      case 5:
        if(serialIOManager.readCredentials(apiToChange, INITIAL_API_TEXT, WIFI_TEXT, API_CONFIRMATION_TEXT))
        {
          if(!dataManager.storeApiCredentials(apiToChange)) serialIOManager.errorNvs();
          if(!dataManager.loadApiCredentials(apiToChange)) serialIOManager.errorNvs();
        }
        else
        {
          serialIOManager.operationCancelled();
          if(!dataManager.loadApiCredentials(apiToChange)) serialIOManager.errorNvs();
        }
      break;
      
      case 6:
        if(serialIOManager.readHumiCalibration(humiSensorsToChange))
        {
          if(!dataManager.storeHumiCalibration(humiSensorsToChange)) serialIOManager.errorNvs();
          if(!dataManager.loadHumiCalibration(humiSensorsToChange)) serialIOManager.errorNvs();
        }
        else
        {
          serialIOManager.operationCancelled();
          if(!dataManager.loadHumiCalibration(humiSensorsToChange)) serialIOManager.errorNvs();
        }
      break; 

      case 7:
        if(serialIOManager.readLinks(apiLinksToChange))
        {
          if(!dataManager.storeApiLinkData(apiLinksToChange)) serialIOManager.errorNvs();
          if(!dataManager.loadApiLinkData(apiLinksToChange)) serialIOManager.errorNvs();
        }
        else
        {
          serialIOManager.operationCancelled();
          if(!dataManager.loadApiLinkData(apiLinksToChange)) serialIOManager.errorNvs();
        }
      break;

      case 8:
        if(serialIOManager.confirmationClearAllStorage())
        {
          dataManager.clearNvs();
          esp_restart();
        }
        else
        {
          serialIOManager.operationCancelled();
        }
      break;
    }  
  }
}