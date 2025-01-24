#include "esp32-hal-gpio.h"
#include "peripheral_control.hpp"

//volatile uint64_t Peripheral::fluxPulses = 0;
std::atomic<uint32_t> Peripheral::fluxPulses = {0};

void IRAM_ATTR Peripheral::fluxCounter()
{
  //taskENTER_CRITICAL_ISR(&mux);
  //fluxPulses++;
  //taskEXIT_CRITICAL_ISR(&mux);
  fluxPulses.fetch_add(1,std::memory_order_relaxed);
}

void Peripheral::analogReadAbsolute(int absoluteHumiArray[], int numSensors)
{
  int startPin = 3;  // ports 0, 1 and 2 of the multiplex are disabled (pcb limits)
  numSensors = min(numSensors, 5); // max num sensors (pcb limits)
  
  for (int port = startPin; port < numSensors + startPin; port++ ) // ports 0, 1 and 2 of the multiplex are disabled (pcb limits)
  {
    absoluteHumiArray[mapSensors[port-startPin]] = 0;
    for (int numberBit = 0; numberBit < 3; numberBit++)
    {
      digitalWrite(MultiplexPins[numberBit], multiplexBitLevel[port][numberBit]);
    }
    for(int i = 0; i<numSamplesAnalogRead; i++)
    {
      delay(analogReadingTimeInterval);
      absoluteHumiArray[mapSensors[port-startPin]] += analogRead(analogHumidityPin);
    }
    absoluteHumiArray[mapSensors[port-startPin]] = absoluteHumiArray[mapSensors[port-startPin]]/numSamplesAnalogRead;
  }
}

void Peripheral::readTempSensors(float tempArray[], int numSensors)
{
  for (int sensor = 0; sensor < numSensors; sensor++)
  {
    tempSensors[sensor].requestTemperatures();
    tempArray[sensor] = tempSensors[sensor].getTempCByIndex(0);
    delay(500);
  }
}


void Peripheral::humiCalibration(Sensor sensors[], int numSensors, bool op)
{
  analogReadAbsolute(humidityValues, numSensors);
  
  for(int i = 0; i<numSensors; i++)
  {
    if(op) 
    {
      sensors[i].maxValueAdc = humidityValues[i];
    }
    else
    {
      sensors[i].minValueAdc = humidityValues[i];
    }
  }
}

void Peripheral::loadTempSensor(Sensor sensors[],  int numSensors)
{
  readTempSensors(tempValues, numSensors);

  for(int i = 0; i<numSensors;i++)
  {
    sensors[i].sensorValue = tempValues[i];
  }
}
void Peripheral::loadHumiSensor(Sensor sensors[], int numSensors)
{
  analogReadAbsolute(humidityValues, numSensors);

  for(int i = 0; i<numSensors;i++)
  {
    sensors[i].sensorValue = map(humidityValues[i], sensors[i].maxValueAdc, sensors[i].minValueAdc, 0, 100); // 0-100% 
  }
}

void Peripheral::initPeripheral()
{
  pinMode(2, OUTPUT);
  pinMode(MultiplexPins[0], OUTPUT);
  pinMode(MultiplexPins[1], OUTPUT);
  pinMode(MultiplexPins[2], OUTPUT);
  pinMode(analogHumidityPin, INPUT);
  pinMode(configPin, INPUT);
  pinMode(flowSensorPin, INPUT);
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);

  for (int i = 0; i < hardwareLimit; i++)
  {
    oneWire[i] = OneWire(tempSensorsPins[i]);
    tempSensors[i] = DallasTemperature(&oneWire[i]);
    tempSensors[i].begin();
  }
  attachInterrupt(digitalPinToInterrupt(flowSensorPin), Peripheral::fluxCounter, RISING);
}
void Peripheral::powerValve(bool state)
{
  digitalWrite(relayPin, state);
  digitalWrite(2, state);
}
void Peripheral::resetWaterVolume()
{
  //taskENTER_CRITICAL_ISR(&mux);
  //fluxPulses = 0;
  //taskEXIT_CRITICAL_ISR(&mux);
  fluxPulses.store(0,std::memory_order_relaxed);
}

double Peripheral::getWaterVolume()
{
  uint64_t pulses  = 0;
  //taskENTER_CRITICAL_ISR(&mux);
  //pulses = fluxPulses;
  //taskEXIT_CRITICAL_ISR(&mux);
  pulses = fluxPulses.load(std::memory_order_acquire);

  return static_cast<double>(pulses)/pulsesPerLiter;
}