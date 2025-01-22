#include "serial_io_manager.hpp"

void SerialIOManager::begin(int baudRate)
{
  serial->begin(baudRate);
}

void SerialIOManager::clearSerialBuffer() {
  while (serial->available() > 0) {
    serial->read(); 
  }
}

bool SerialIOManager::readUserIDs(Sensor sensor[], const String &INITIAL, const String &PRESENTATION)
{
  while(1)
  {
    int argSerial = -1;
    int IDsRead = 0;
    unsigned long currentMillis = millis();
    unsigned long LastActionMillis = currentMillis;

    clearSerialBuffer();
    serial->println(INITIAL);
    
    while(IDsRead<numModules)
    {
      if(serial->available()>0)
      {
        sensor[IDsRead].id = serial->parseInt();
        clearSerialBuffer();
        LastActionMillis = currentMillis;
        serial->println(sensor[IDsRead].id);
        IDsRead++;
      }
      currentMillis = millis();
      if((currentMillis - LastActionMillis) > timeoutArgSerial) return false;
    }

    serial->println(PRESENTATION);
    showIDsArray(sensor);

    serial->println(ID_CONFIRMATION_TEXT);

    clearSerialBuffer();
    currentMillis = millis();
    LastActionMillis = currentMillis;
    
    while(argSerial != 2 && argSerial != 1 && argSerial != 0 && ((currentMillis-LastActionMillis) < timeoutArgSerial))
    {
      if(serial->available()>0) 
      {
        argSerial = serial->parseInt();
        LastActionMillis = currentMillis;
      }
      currentMillis = millis();
    }
    if(argSerial == 1) 
    {
      return true;
    }
    if(argSerial == 2) 
    {
      return false;
    }
  }
}

bool SerialIOManager::readHumiCalibration(Sensor sensor[])
{
  while(1)
  {
    int argSerial = -1;
    unsigned long currentMillis = millis();
    unsigned long LastActionMillis = currentMillis;

    clearSerialBuffer();
    
    serial->println(INITIAL_CALIBRATION_TEXT);
 
    while(argSerial != 1 && argSerial != 0 && (currentMillis-LastActionMillis) < timeoutArgSerial)
    {
      if(serial->available()>0) 
      {
        argSerial = serial->parseInt();
        LastActionMillis = currentMillis;
        clearSerialBuffer();
      }
      currentMillis = millis();
      peripheral->humiCalibration(sensor, numModules, true);
      showCurrentCalibrationValue(sensor, true);
    }

    if(argSerial != 1) 
    {
      return false;
    } 
    argSerial = -1;

    clearSerialBuffer();
    
    serial->println(CALIBRATION_TEXT);
    while(argSerial != 1)
    {
      if(serial->available()>0) 
      {
        argSerial = serial->parseInt();
        LastActionMillis = currentMillis;
        clearSerialBuffer();
      }
      currentMillis = millis();
      if((currentMillis-LastActionMillis) > timeoutArgSerial) return false;
      peripheral->humiCalibration(sensor, numModules, false);
      showCurrentCalibrationValue(sensor, false);
    }
    argSerial = -1;    
    showCalibration(sensor);

    clearSerialBuffer();
    currentMillis = millis();
    LastActionMillis = currentMillis;
    
    serial->println(CALIBRATION_CONFIRMATION_TEXT);
    while(argSerial != 2 && argSerial != 1 && argSerial != 0 && (currentMillis-LastActionMillis) < timeoutArgSerial)
    {
      if(serial->available()>0) 
      {
        argSerial = serial->parseInt();
        LastActionMillis = currentMillis;
        clearSerialBuffer();
      }
      currentMillis = millis();
    }
    
    if(argSerial == 1) 
    {
      return true;
    }
    if(argSerial == 2) 
    {
      return false;
    }
  }
}

bool SerialIOManager::readCredentials(Credentials &credentials, const String &initialText, const String &mainText, const String &confirmationText)
{
  while(1)
  {
    int argSerial = -1;
    unsigned long currentMillis = millis();
    unsigned long LastActionMillis = currentMillis;
    
    clearSerialBuffer();
    
    serial->println(initialText);
    
    while(1)
    {
      if(serial->available()>0)
      {
        credentials.login = serial->readStringUntil('\n');
        LastActionMillis = currentMillis;
        serial->println(credentials.login);
        break;
      }
      currentMillis = millis();
      if((currentMillis-LastActionMillis) > timeoutArgSerial) return false;
    }
    
    clearSerialBuffer();

    currentMillis = millis();
    LastActionMillis = currentMillis;
    
    serial->println(mainText);
    while(1)
    {
      if(serial->available()>0)
      {
        credentials.password = serial->readStringUntil('\n');
        LastActionMillis = currentMillis;
        credentials.password.trim();
        serial->println(credentials.password); //remover no codigo final
        break;
      }
      currentMillis = millis();
      if((currentMillis-LastActionMillis) > timeoutArgSerial) return false;
    }

    clearSerialBuffer();

    currentMillis = millis();
    LastActionMillis = currentMillis;

    serial->println(confirmationText);
    while(argSerial != 2 && argSerial != 1 && argSerial != 0 && (currentMillis-LastActionMillis) < timeoutArgSerial)
    {
      if(serial->available()>0) 
      {
        argSerial = serial->parseInt();
        LastActionMillis = currentMillis;
      }
      currentMillis = millis();
    }
    if(argSerial == 1) 
    {
      return true;
    }
    if(argSerial == 2) 
    {
      return false;
    }
  }
}

bool SerialIOManager::readLinks(ApiLinks &apiLinks)
{
  while(1)
  {
    int argSerial = -1;
    unsigned long currentMillis = millis();
    unsigned long LastActionMillis = currentMillis;
    
    clearSerialBuffer();
    
    serial->println(INITIAL_LINK_TEXT);
    serial->println(LINK_AUTH_TEXT);
    
    while(1)
    {
      if(serial->available()>0)
      {
        apiLinks.linkToAuthenticate = serial->readStringUntil('\n');
        apiLinks.linkToAuthenticate.trim();
        LastActionMillis = currentMillis;
        serial->println(apiLinks.linkToAuthenticate);
        break;
      }
      currentMillis = millis();
      if((currentMillis-LastActionMillis) > timeoutArgSerial) return false;
    }
    
    clearSerialBuffer();
    currentMillis = millis();
    LastActionMillis = currentMillis;

    serial->println(LINK_SENSOR_READING_TEXT);
    while(1)
    {
      if(serial->available()>0)
      {
        apiLinks.linkToSensorsReading = serial->readStringUntil('\n');
        apiLinks.linkToSensorsReading.trim();
        LastActionMillis = currentMillis;
        serial->println(apiLinks.linkToSensorsReading);
        break;
      }
      currentMillis = millis();
      if((currentMillis-LastActionMillis) > timeoutArgSerial) return false;
    }
    
    clearSerialBuffer();
    currentMillis = millis();
    LastActionMillis = currentMillis;

    serial->println(LINK_VALVE_TEXT);
    while(1)
    {
      if(serial->available()>0)
      {
        apiLinks.linkToValveState = serial->readStringUntil('\n');
        apiLinks.linkToValveState.trim();
        LastActionMillis = currentMillis;
        serial->println(apiLinks.linkToValveState);
        break;
      }
      currentMillis = millis();
      if((currentMillis-LastActionMillis) > timeoutArgSerial) return false;
    }
    
    clearSerialBuffer();
    currentMillis = millis();
    LastActionMillis = currentMillis;

    serial->println(LINK_TIME_VALVE_TEXT);
    while(1)
    {
      if(serial->available()>0)
      {
        apiLinks.linkToTimeValve = serial->readStringUntil('\n');
        apiLinks.linkToTimeValve.trim();
        LastActionMillis = currentMillis;
        serial->println(apiLinks.linkToTimeValve);
        break;
      }
      currentMillis = millis();
      if((currentMillis-LastActionMillis) > timeoutArgSerial) return false;
    }
    
    clearSerialBuffer();
    currentMillis = millis();
    LastActionMillis = currentMillis;


    serial->println(LINK_WATER_FLOW_TEXT);
    
    while(1)
    {
      if(serial->available()>0)
      {
        apiLinks.linkToWaterFlow = serial->readStringUntil('\n');
        apiLinks.linkToWaterFlow.trim();
        LastActionMillis = currentMillis;
        serial->println(apiLinks.linkToWaterFlow);
        break;
      }
      currentMillis = millis();
      if((currentMillis-LastActionMillis) > timeoutArgSerial) return false;
    }

    showApiLinks(apiLinks);

    clearSerialBuffer();
    currentMillis = millis();
    LastActionMillis = currentMillis;

    serial->println(LINK_CONFIRMATION_TEXT);
    while(argSerial != 2 && argSerial != 1 && argSerial != 0 && (currentMillis-LastActionMillis) < timeoutArgSerial)
    {
      if(serial->available()>0) 
      {
        argSerial = serial->parseInt();
        LastActionMillis = currentMillis;
        clearSerialBuffer();
      }
      currentMillis = millis();
    }
    if(argSerial == 1) 
    {
      return true;
    }
    if(argSerial == 2) 
    {
      return false;
    }
  }
}

bool SerialIOManager::confirmationClearAllStorage()
{
  while(1)
  {
    int argSerial = -1;
    unsigned long currentMillis = millis();
    unsigned long LastActionMillis = currentMillis;

    clearSerialBuffer();

    serial->println(CLEAR_CONFIRMATION_TEXT);
    while(argSerial != 1 && argSerial != 0 && (currentMillis-LastActionMillis) < timeoutArgSerial)
    {
      if(serial->available()>0) 
      {
        argSerial = serial->parseInt();
        LastActionMillis = currentMillis;
      }
      currentMillis = millis();
    }
    if(argSerial == 1) 
    {
      return true;
    }
    if(argSerial == 0)
    {
      return false;
    }
  }
}

void SerialIOManager::showIDsArray(Sensor sensor[])
{
  for(int i = 0; i<numModules; i++)
  {
    serial->println("\"" + String(sensor[i].id) +"\"");
  }
}
void SerialIOManager::showCalibration(Sensor sensor[])
{
  for (int i = 0; i < numModules; i++) 
  {
    serial->print("A");
    serial->print(i + 1);
    serial->print(" max: ");
    serial->print(sensor[i].maxValueAdc);
    serial->print("; min: ");
    serial->println(sensor[i].minValueAdc);
  }
}

void SerialIOManager::showCurrentCalibrationValue(Sensor sensor[], bool op)
{
  for(int i = 0; i<numModules; i++)
  {
    if(op)
    {
      serial->print(sensor[i].maxValueAdc);
    }
    else
    {
      serial->print(sensor[i].minValueAdc);  
    }
    serial->print("  ");
  }
  serial->println();
}

void SerialIOManager::showCredentials(String &login)
{
  serial->print("Identificador: ");
  serial->println(login);
}

void SerialIOManager::showApiLinks(ApiLinks &apiLinks)
{
  serial->println(apiLinks.linkToAuthenticate);
  serial->println(apiLinks.linkToSensorsReading);
  serial->println(apiLinks.linkToValveState);
  serial->println(apiLinks.linkToTimeValve);
  serial->println(apiLinks.linkToWaterFlow);
}


void SerialIOManager::showAllData(Sensor sensorH[], Sensor sensorT[], Credentials &wifi, Credentials &api, ApiLinks &apiLinks) 
{
  serial->println(IDS_HUMI_TEXT);
  showIDsArray(sensorH);

  serial->println(IDS_TEMP_TEXT);
  showIDsArray(sensorT);

  showCalibration(sensorH);

  showCredentials(wifi.login);

  showCredentials(api.login);

  showApiLinks(apiLinks);
}

void SerialIOManager::menuConfig()
{
  serial->println(TEXT_MENU);
}

int SerialIOManager::waitForInt(int max, int min)
{
  int argSerial;

  while(!(argSerial>min && argSerial<(max+1)))
  {
    if(serial->available()>0) 
    {
      argSerial = serial->parseInt();
      clearSerialBuffer();
    }
  }

  return argSerial;
}


void SerialIOManager::errorNvs()
{
  serial->println(TEXT_ERRO_NVS);
}

void SerialIOManager::operationCancelled()
{
  serial->println(ABORT_SERIAL_READ);
}

bool SerialIOManager::waitforPowerMode()
{

  int argSerial = -1;
  unsigned long currentMillis = millis();
  unsigned long LastActionMillis = currentMillis;

  clearSerialBuffer();

  serial->println(POWER_MODE_TEXT);
  while(argSerial != 1 && argSerial != 0 && (currentMillis-LastActionMillis) < 10000)
  {
    if(serial->available()>0) 
    {
      argSerial = serial->parseInt();
      LastActionMillis = currentMillis;
    }
    currentMillis = millis();
  }
  if(argSerial == 1) 
  {
    return true;
  }
  if(argSerial == 0)
  {
    return false;
  }
  return false;
}