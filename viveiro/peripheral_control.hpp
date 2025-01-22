#ifndef _peripheral_control_
#define _peripheral_control_

#include <OneWire.h>
#include <DallasTemperature.h>
#include "data_types.hpp"
#include <Arduino.h>
#include <atomic>

class Peripheral
{
  private:
    static const int hardwareLimit = 5;
    const int mapSensors[hardwareLimit] = {0,1,4,2,3};
    const int tempSensorsPins[hardwareLimit] = {18, 19, 21, 22, 23};
    const int MultiplexPins[3] = {25, 26, 27}; // bit 2, 1, 0
    const int configPin = 34;
    const int relayPin = 13;
    const int flowSensorPin = 35;
    const int analogHumidityPin = 32;
    const int pulsesPerLiter = 450;

    const int debaucingTime = 100;
    //static volatile uint64_t fluxPulses;
    static std::atomic<uint32_t> fluxPulses;

    int numSamplesAnalogRead = 3;
    unsigned int analogReadingTimeInterval = 100; // ms

    OneWire oneWire[hardwareLimit];
    DallasTemperature tempSensors[hardwareLimit];

    float tempValues[hardwareLimit] = {};
    int humidityValues[hardwareLimit] = {};

    uint8_t multiplexBitLevel[8][3] = {
      {0, 0, 0}, // 0
      {0, 0, 1}, // 1
      {0, 1, 0}, // 2
      {0, 1, 1}, // 3
      {1, 0, 0}, // 4
      {1, 0, 1}, // 5
      {1, 1, 0}, // 6
      {1, 1, 1}  // 7
    };
    
    void readTempSensors(float tempArray[], int numSensors);

    static void IRAM_ATTR fluxCounter();
  public:
    void initPeripheral();
    void analogReadAbsolute(int absoluteHumiArray[] , int numSensors);
    void humiCalibration(Sensor sensors[], int numSensors, bool op);
    void loadTempSensor(Sensor sensors[], int numSensors);
    void loadHumiSensor(Sensor sensors[], int numSensors);
    
    double getWaterVolume();
    void powerValve(bool state);
    void resetWaterVolume();
};
#endif