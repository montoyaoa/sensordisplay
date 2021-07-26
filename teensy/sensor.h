//#include "Arduino.h"
#pragma once

enum SensorType {VOLTAGE, CLOCK, TEMPERATURE, LOAD, FAN, FLOW, CONTROL, LEVEL, FACTOR, POWER, DATA, SMALLDATA, THROUGHPUT};

struct STRUCTSENSOR {
  enum SensorType sensorType;
  char Name[128];
  float Value;
  int OrderedSensorIndex;
};

class Sensor {
  public:
  Sensor(char *newName, enum SensorType newSensorType, float newValue, int newOrderedSensorIndex);

  char Name[128];
  enum SensorType sensorType;
  float Value;
  int OrderedSensorIndex;
  char sensorArray[256];

  void updateSensorArray();
  char* formatDatatype();
  
};

enum SensorType intToSensorType(int);
