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
  Sensor(char *newName, enum SensorType newSensorType, float newValue, int newOrderedSensorIndex, boolean newIsFirstCPUSensor, boolean newIsFirstGPUSensor, boolean newIsFirstRAMSensor);

  char Name[128];
  enum SensorType sensorType;
  float Value;
  int OrderedSensorIndex;
  boolean isVisible;
  boolean isFirstCPUSensor;
  boolean isFirstGPUSensor;
  boolean isFirstRAMSensor;
  int xpos;

  char* formatDatatype();
  void updatePosition(int);

  
  
};

enum SensorType intToSensorType(int);
