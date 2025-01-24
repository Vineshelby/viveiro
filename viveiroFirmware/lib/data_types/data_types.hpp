#ifndef _DATA_TYPES_HPP_
#define _DATA_TYPES_HPP_

#include <Arduino.h>
#include <time.h>

typedef struct
{
  String login;
  String password;
}Credentials;

typedef struct
{
  float sensorValue;
  int id;
  int maxValueAdc; // use in analog sensors
  int minValueAdc; // use in analog sensors
}Sensor;

typedef struct 
{ 
  String linkToAuthenticate;
  String linkToSensorsReading;
  String linkToValveState;
  String linkToTimeValve;
  String linkToWaterFlow;
}ApiLinks;

typedef struct 
{
  struct tm initialTime;
  struct tm finalTime;

}TimeIrrigation;

#endif