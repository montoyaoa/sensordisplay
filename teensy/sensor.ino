#include "sensor.h"


Sensor::Sensor(char *newName, enum SensorType newSensorType, float newValue, int newOrderedSensorIndex){
  strcpy(Name, newName);
  sensorType = newSensorType;
  Value = newValue;
  OrderedSensorIndex = newOrderedSensorIndex;
}

void Sensor::updateSensorArray(){
  sprintf(sensorArray, "\"%s\",%s,%.2f", Name, "test", Value);
  
}

char* Sensor::formatDatatype(){
  switch(this->sensorType){
    case VOLTAGE:
      return "V";
    case CLOCK:
      return "MHz";
    case TEMPERATURE:
      return "test";
    case LOAD:
      return "%";
    case FAN:
      return "RPM";
    case FLOW:
      return "LPM";
    case CONTROL:
      return "?";
    case LEVEL:
      return "%";
    case FACTOR:
      return "?";
    case POWER:
      return "W";
    case DATA:
      return "GB";
    case SMALLDATA:
      return "MB";
    case THROUGHPUT:
      return "MB/s";
    default:
      return "?";
  }
}

enum SensorType intToSensorType(int input){
  switch(input){
    case 0:
      return VOLTAGE;
    case 1:
      return CLOCK;
    case 2:
      return TEMPERATURE;
    case 3:
      return LOAD;
    case 4:
      return FAN;
    case 5:
      return FLOW;
    case 6:
      return CONTROL;
    case 7:
      return LEVEL;
    case 8:
      return FACTOR;
    case 9:
      return POWER;
    case 10:
      return DATA;
    case 11:
      return SMALLDATA;
    case 12:
      return THROUGHPUT;
    default:
      return VOLTAGE;
  }
}
