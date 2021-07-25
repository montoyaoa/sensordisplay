#include <Arduino.h>
#include <SPI.h>
#include <stdarg.h>
#include <LinkedList.h>
//custom sensor class
#include "sensor.h"
// Definitions for our display.
#include "CFA10099_defines.h"
#include "CFAF480128xx_039T.h"
// The very simple EVE library files
#include "EVE_defines.h"
#include "EVE_base.h"
#include "EVE_draw.h"
#include "parsing.h"

//Serial Input Buffer (default is string)
char serialInput[256];

char CPUName[256];
char GPUName[256];
char RAMName[256];

boolean newData = false;

//flags for various input states
boolean component         = false;
boolean cpusensors        = false;
boolean gpusensors        = false;
boolean ramsensors        = false;
boolean sensordata        = false;
boolean screenInitialized = false;

//Linked List containing ADDRESSES to Sensor objects
LinkedList<Sensor*> allsensors = LinkedList<Sensor*>();

bool onlyOnce = false;

int orderedSensorIndex = 0;

int posx = 100;
int posy = 0;
int rectheight = 70;
int rectwidth = 120;
int rectoffset = 80;
int padding = 30;
int verticalscrolloffset = 60;
int numberofvisible = 0;
 
int upnext = 4;


//===========================================================================
void setup()
{

  //Initialize GPIO port states
  // Set CS# high to start - SPI inactive
  SET_EVE_CS_NOT;
  // Set PD# high to start
  SET_EVE_PD_NOT;
  SET_SD_CS_NOT;

  //Initialize port directions
  // EVE interrupt output (not used in this example)
  pinMode(EVE_INT, INPUT_PULLUP);
  // EVE Power Down (reset) input
  pinMode(EVE_PD_NOT, OUTPUT);
  // EVE SPI bus CS# input
  pinMode(EVE_CS_NOT, OUTPUT);
  // USD card CS
  pinMode(SD_CS, OUTPUT);
  // Optional pin used for LED or oscilloscope debugging.
  pinMode(DEBUG_LED, OUTPUT);

  // Initialize SPI
  SPI.begin();
  //Bump the clock to 8MHz. Appears to be the maximum.
  SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));

  //See if we can find the FTDI/BridgeTek EVE processor
  if (0 != EVE_Initialize())
  {
    //DBG_STAT("Failed to initialize %s8%02X. Stopping.\n", EVE_DEVICE < 0x14 ? "FT" : "BT", EVE_DEVICE);
    while (1);
  }
  else { 
    //DBG_STAT("%s8%02X initialized.\n", EVE_DEVICE < 0x14 ? "FT" : "BT", EVE_DEVICE); 
  }
} //  setup()



//===========================================================================
void loop()
{
  //Get the current write pointer from the EVE
  uint16_t FWo;
  FWo = EVE_REG_Read_16(EVE_REG_CMD_WRITE);

  //Keep track of the RAM_G memory allocation
  uint32_t RAM_G_Unused_Start;
  RAM_G_Unused_Start = 0;




  while (1)
  {
    Sensor* sample;
    recvWithStartEndMarkers();
    //========== FRAME SYNCHRONIZING ==========
    // Wait for graphics processor to complete executing the current command
    // list. This happens when EVE_REG_CMD_READ matches EVE_REG_CMD_WRITE, indicating
    // that all commands have been executed.  We have a local copy of
    // EVE_REG_CMD_WRITE in FWo.
    //
    // This appears to only occur on frame completion, which is nice since it
    // allows us to step the animation along at a reasonable rate.
    //
    // If possible, I have tweaked the timing on the Crystalfontz displays
    // to all have ~60Hz frame rate.
    FWo = Wait_for_EVE_Execution_Complete(FWo);

    //========== START THE DISPLAY LIST ==========
    // Start the display list
    FWo = EVE_Cmd_Dat_0(FWo,(EVE_ENC_CMD_DLSTART));

    // Set the default clear color to black
    FWo = EVE_Cmd_Dat_0(FWo, EVE_ENC_CLEAR_COLOR_RGB(0, 0, 0));

    // Clear the screen - this and the previous prevent artifacts between lists
    FWo = EVE_Cmd_Dat_0(FWo, EVE_ENC_CLEAR(1 /*CLR_COL*/, 1 /*CLR_STN*/, 1 /*CLR_TAG*/));

    //========== ADD GRAPHIC ITEMS TO THE DISPLAY LIST ==========
    //Fill background with black
    FWo = EVE_Cmd_Dat_0(FWo, EVE_ENC_COLOR_RGB(0, 0, 0));
    FWo = EVE_Filled_Rectangle(FWo, 0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1);



    for(int i = 0; i < allsensors.size(); i++){
      sample = allsensors.get(i);

      //white pen
      FWo = EVE_Cmd_Dat_0(FWo, EVE_ENC_COLOR_RGB(255, 255, 255));
      
      
      //do all this inside the sample sensor
      //if the sample is visible draw the sensor
      if(sample->isVisible){
        if      (sample->isFirstCPUSensor) { 
          FWo = EVE_Text(FWo, sample->xpos, 0, 31, 0, "CPU"); 
          FWo = EVE_Text(FWo, sample->xpos, 43, 18, 0, CPUName);
        }
        else if (sample->isFirstGPUSensor) { 
          FWo = EVE_Text(FWo, sample->xpos, 0, 31, 0, "GPU");
          FWo = EVE_Text(FWo, sample->xpos, 43, 18, 0, GPUName); 
        }
        else if (sample->isFirstRAMSensor) { 
          FWo = EVE_Text(FWo, sample->xpos, 0, 31, 0, "RAM"); 
          FWo = EVE_Text(FWo, sample->xpos, 43, 18, 0, RAMName);
        }
        FWo = EVE_Filled_Rectangle(FWo, sample->xpos, verticalscrolloffset, (sample->xpos)+rectwidth, rectheight+verticalscrolloffset);
        //black pen
        FWo = EVE_Cmd_Dat_0(FWo, EVE_ENC_COLOR_RGB(0, 0, 0));
        FWo = EVE_PrintF(FWo, sample->xpos, 0+verticalscrolloffset, 30, 0, "%.1f%s", sample->Value, sample->formatDatatype());
        FWo = EVE_PrintF(FWo, sample->xpos, 36+verticalscrolloffset, 23, 0, "%s", sample->Name);
        sample->updatePosition(rectwidth);
      }


      //do all this inside the list
      Sensor* ext;
      if(upnext == 0){
        ext = allsensors.get(allsensors.size()-1);
      }
      else{
        ext = allsensors.get(upnext - 1);
      }

      if(ext->xpos == (LCD_WIDTH - rectwidth - padding)){
       Sensor* nextSensor = allsensors.get(upnext);
       nextSensor->isVisible = true;
       allsensors.set(upnext, nextSensor);
       if(upnext == allsensors.size() - 1){
        upnext = 0;
       }
       else{
        upnext++;
       }
      }
    }
      
      
    //========== FINSH AND SHOW THE DISPLAY LIST ==========
    // Instruct the graphics processor to show the list
    FWo = EVE_Cmd_Dat_0(FWo, EVE_ENC_DISPLAY());
    // Make this list active
    FWo = EVE_Cmd_Dat_0(FWo, EVE_ENC_CMD_SWAP);
    // Update the ring buffer pointer so the graphics processor starts executing
    EVE_REG_Write_16(EVE_REG_CMD_WRITE, (FWo));
    
  }  // while(1)
} // loop()
//===========================================================================


void recvWithStartEndMarkers() {
  boolean recvInProgress = false;
  char startMarker = '<';
  char endMarker = '>';
  char recievedChar;
  int indexOfInput = 0;

  while (Serial.available() > 0 && newData == false) {
    //read one character from the serial input
    recievedChar = Serial.read();
    //when in recieving mode
    //check the value of the recieved character
    if (recvInProgress == true) {
      //special case: the character recieved is an end marker
      //end recieving mode
      //state that there is a complete sentence recieved
      if (recievedChar == endMarker) {
        recvInProgress = false;
        parseSerialData();
      }
      //special case: a newline is recieved before the end marker
      //end recieving mode
      //empty the input string
      else if (recievedChar == '\n') {
        recvInProgress = false;
        memset(serialInput, 0, sizeof(serialInput));
        indexOfInput = 0;
      }
      //general case
      //append the recieved character to the input string
      else { 
        serialInput[indexOfInput] = recievedChar;
        indexOfInput++;
      }
    }
    //if not in recieving mode and the character recieved is the start marker
    //begin recieving mode
    else if (recievedChar == startMarker) { recvInProgress = true; }
  }
}

void parseSerialData() {
    if (strcmp(serialInput, "componentend") == 0)   { component = false; }
    if (strcmp(serialInput, "cpusensorsend") == 0)  { cpusensors = false; onlyOnce = false; }
    if (strcmp(serialInput, "gpusensorsend") == 0)  { gpusensors = false; onlyOnce = false; }
    if (strcmp(serialInput, "ramsensorsend") == 0)  { ramsensors = false; onlyOnce = false; }

    if (component) {
      if      (strncmp(serialInput, "CPU: ", 5) == 0) { memmove(CPUName, serialInput + 5, strlen(serialInput)); }
      else if (strncmp(serialInput, "GPU: ", 5) == 0) { memmove(GPUName, serialInput + 5, strlen(serialInput)); }
      else if (strncmp(serialInput, "RAM: ", 5) == 0) { memmove(RAMName, serialInput + 5, strlen(serialInput)); }
    }

    if (cpusensors) {
      char* rawSensorType = strtok(serialInput, ",");
      if(rawSensorType != NULL) {
        enum SensorType sensorType = intToSensorType(atoi(rawSensorType));
        char* rawSensorName = strtok(NULL, ",");
        if(rawSensorName != NULL){
          if(!onlyOnce){
            Sensor* newSensor = new Sensor(rawSensorName, sensorType, 0.0, orderedSensorIndex, true, false, false);
            onlyOnce = true;
            orderedSensorIndex++;
            allsensors.add(newSensor);
          }
          else{
            Sensor* newSensor = new Sensor(rawSensorName, sensorType, 0.0, orderedSensorIndex, false, false, false);
            orderedSensorIndex++;
            allsensors.add(newSensor);
          }
          
        }
      }
    }

    if (gpusensors) {
      char* rawSensorType = strtok(serialInput, ",");
      if(rawSensorType != NULL) {
        enum SensorType sensorType = intToSensorType(atoi(rawSensorType));
        char* rawSensorName = strtok(NULL, ",");
        if(rawSensorName != NULL){
          if(!onlyOnce){
            Sensor* newSensor = new Sensor(rawSensorName, sensorType, 0.0, orderedSensorIndex, false, true, false);
            onlyOnce = true;
            orderedSensorIndex++;
            allsensors.add(newSensor);
          }
          else{
            Sensor* newSensor = new Sensor(rawSensorName, sensorType, 0.0, orderedSensorIndex, false, false, false);
            orderedSensorIndex++;
            allsensors.add(newSensor);
          }
        }
      }
    }

    if (ramsensors) {
      char* rawSensorType = strtok(serialInput, ",");
      if(rawSensorType != NULL) {
        enum SensorType sensorType = intToSensorType(atoi(rawSensorType));
        char* rawSensorName = strtok(NULL, ",");
        if(rawSensorName != NULL){
          if(!onlyOnce){
            Sensor* newSensor = new Sensor(rawSensorName, sensorType, 0.0, orderedSensorIndex, false, false, true);
            onlyOnce = true;
            orderedSensorIndex++;
            allsensors.add(newSensor);
          }
          else{
            Sensor* newSensor = new Sensor(rawSensorName, sensorType, 0.0, orderedSensorIndex, false, false, false);
            orderedSensorIndex++;
            allsensors.add(newSensor);
          }
        }
      }
    }

    if(sensordata) {
      char* rawReading = strtok(serialInput, ",");
      if(rawReading != NULL){
        for(int i = 0; i < orderedSensorIndex; i++) {
          Sensor* oldSensor = allsensors.get(i);
          if(oldSensor->OrderedSensorIndex == i){
            if(rawReading != NULL){ oldSensor->Value = atof(rawReading); }
            allsensors.set(i, oldSensor);
            rawReading = strtok(NULL, ",");
          }
        }
      }
    }

    //complete handshake
    //note that Serial.println() sends a \n\r at the end of the string,
      //which cannot be properly parsed by the host program
    if (strcmp(serialInput, "init") == 0) { 
      Serial.print("<initack>\n");
      screenInitialized = true;
    }
    if (strcmp(serialInput, "component") == 0) {
      Serial.print("<componentack>\n");
      component = true;
    }
    if (strcmp(serialInput, "cpusensors") == 0) {
      Serial.print("<cpusensorsack>\n");
      cpusensors = true;
    }
    if (strcmp(serialInput, "gpusensors") == 0) {
      Serial.print("<gpusensorsack>\n");
      gpusensors = true;
    }
    if (strcmp(serialInput, "ramsensors") == 0) {
      Serial.print("<ramsensorsack>\n");
      ramsensors = true;
    }
    if (strcmp(serialInput, "sensordata") == 0) {
      sensordata = true;
    }

    //empty the serial input buffer
    memset(serialInput, 0, sizeof(serialInput));
}
