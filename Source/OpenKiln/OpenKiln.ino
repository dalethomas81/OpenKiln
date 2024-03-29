/*

  https://github.com/dalethomas81/OpenKiln
  Copyright (C) 2021  Dale Thomas

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.

  DaleThomas@me.com

*/

/****************************************
  Board settings:
    Board: NodeMCU 1.0 (ESP-12E Module)
    Flash Size: 4M (FS:1MBOTA:~1019KB)
    Debug Port: Disabled
    Debug Level: None
    IwIP Variant: v2 Lower Memory
    VTables: Flash
    CPU Frequency: 160MHz
    Upload Speed: 115200
    Erase Flash: Only Sketch (if using emulated EEPROM 'All Flash Contents' will overwrite it!)
    Builtin LED: 2


    "board": "esp8266:esp8266:nodemcuv2",
    "configuration": "xtal=160,vt=flash,exception=legacy,ssl=all,eesz=4M1M,led=2,ip=lm2f,dbg=Disabled,lvl=None____,wipe=none,baud=115200",


*****************************************/

// NOTE: ****** kiln was flashed with ESP Board version 2.7.2

/* global defines */
#define WIFI_LISTENING_PORT       80
#define SERIAL_BAUD_RATE          115200
#define HEARTBEAT_TIME            1000
#define CONST_DEVICE_ID           "Kiln-"

#define AUTOMATIC_MODE            1
#define MANUAL_MODE               2
#define SIMULATION_MODE           3
#define NUMER_OF_MODES            3 // make this equal to the last mode to trap errors

/* global variables */
bool Safety_Ok = false;
uint16_t Mode = AUTOMATIC_MODE,  Mode_Last = AUTOMATIC_MODE;
bool ThermalRunawayDetected = false, ui_ThermalRunawayOverride = false;
String DEVICE_ID(CONST_DEVICE_ID);

/* wifi */
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>  
void checkWifi() {
  if ((WiFi.status() != WL_CONNECTED) || (WiFi.localIP().toString() == "0.0.0.0")) { 
    //Serial.println("Reconnecting to Wifi");
    WiFi.reconnect();
    //connectWifi(0);
  }
}
void setupWifiManager() {   
  // The extra parameters to be configured (can be either global or just in the setup)
  // After connecting, parameter.getValue() will get you the configured value
  // id/name placeholder/prompt default length
  // Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;

  //reset settings - for testing
  //wifiManager.resetSettings();

  //set minimum quality of signal so it ignores AP's under that quality
  //defaults to 8%
  wifiManager.setMinimumSignalQuality();
  
  // set custom ip for portal
  //wifiManager.setAPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));
  
  /*
  if (BUTTON_STATE == LOW) {
    if (!wifiManager.startConfigPortal(DEVICE_ID.c_str())) {
      Serial.println("failed to connect and hit timeout");
      delay(3000);
      ESP.reset();
      delay(5000);
    }
  } else {
    wifiManager.autoConnect(DEVICE_ID.c_str());
  }
  */

    wifiManager.autoConnect(DEVICE_ID.c_str());
    //WiFi.mode(WIFI_STA);
    //WiFi.begin();
    //WiFi.setAutoReconnect(true);
    //WiFi.persistent(true);
}

/* flash */
#include "LittleFS.h" // need the upload tool here https://github.com/earlephilhower/arduino-esp8266littlefs-plugin
void initLittleFS() {
 
    Serial.println(F("Inizializing FS..."));
    if (LittleFS.begin()){
        Serial.println(F("done."));
    }else{
        Serial.println(F("fail."));
    }
 
    // To format all space in LittleFS
     //LittleFS.format();
 
    // Get all information of your LittleFS
    /*FSInfo fs_info;
    LittleFS.info(fs_info);
 
    Serial.println("File system info.");
 
    Serial.print("Total space:      ");
    Serial.print(fs_info.totalBytes);
    Serial.println("byte");
 
    Serial.print("Total space used: ");
    Serial.print(fs_info.usedBytes);
    Serial.println("byte");
 
    Serial.print("Block size:       ");
    Serial.print(fs_info.blockSize);
    Serial.println("byte");
 
    Serial.print("Page size:        ");
    Serial.print(fs_info.totalBytes);
    Serial.println("byte");
 
    Serial.print("Max open files:   ");
    Serial.println(fs_info.maxOpenFiles);
 
    Serial.print("Max path length:  ");
    Serial.println(fs_info.maxPathLength);
 
    Serial.println();
 
    // Open dir folder
    Dir dir = LittleFS.openDir("/");
    // Cycle all the content
    while (dir.next()) {
        // get filename
        Serial.print(dir.fileName());
        Serial.print(" - ");
        // If element have a size display It else write 0
        if(dir.fileSize()) {
            File f = dir.openFile("r");
            Serial.println(f.size());
            f.close();
        }else{
            Serial.println("0");
        }
    }
    */
}

/* ota */
#include <ArduinoOTA.h>
#define OTA_PASSWORD    "OpenKiln@12"
void setupOTA(){
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);
  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(DEVICE_ID.c_str());
  // No authentication by default
  //ArduinoOTA.setPassword((const char *)OTA_PASSWORD);
  ArduinoOTA.onStart([]() {
    LittleFS.end();
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
}

/* i/o */
#define SAFETY_CIRCUIT_INPUT      D0
#define MAIN_CONTACTOR_OUTPUT     D4
#define SSR_PIN_01                D1
#define SSR_PIN_02                D2
void setupPins() {
  pinMode(SSR_PIN_01, OUTPUT);
  pinMode(SSR_PIN_02, OUTPUT);
  pinMode(SAFETY_CIRCUIT_INPUT, INPUT);
  pinMode(MAIN_CONTACTOR_OUTPUT, OUTPUT);
  digitalWrite(MAIN_CONTACTOR_OUTPUT, HIGH);
}

/* pid */
#include <PID_v1.h>
#define PID_RELAY_DUTY_CYCLE_PERIOD 1000
double Setpoint_ch0, Setpoint_ch1, Input_01, Input_02, Output_01, Output_02;
double temperature_ch0, temperature_ch1;
bool ui_StsSSRPin_01, ui_StsSSRPin_02;
double Kp_01=5.0, Ki_01=0.001, Kd_01=0.0;
double Kp_02=5.0, Ki_02=0.001, Kd_02=0.0;
PID PID_01(&Input_01, &Output_01, &Setpoint_ch0, Kp_01, Ki_01, Kd_01, DIRECT); // REVERSE - PROCESS lowers as OUTPUT rises
PID PID_02(&Input_02, &Output_02, &Setpoint_ch1, Kp_02, Ki_02, Kd_02, DIRECT); 
unsigned long windowStartTime_01, windowStartTime_02;
void handlePID() {
  //Input = analogRead(ANALOG_PIN); // this is causing wifi issues?? wtf?? // esp uses A0 to read input voltage and adjust power - maybe thats the problem?
  Input_01 = temperature_ch0;
  Input_02 = temperature_ch1;
  PID_01.SetTunings(Kp_01, Ki_01, Kd_01);
  PID_02.SetTunings(Kp_02, Ki_02, Kd_02);
  PID_01.Compute();
  PID_02.Compute();
  /************************************************
   * turn the output pin on/off based on pid output
   ************************************************/
   // TODO: enable manual mode to set the pulse width
  if (millis() - windowStartTime_01 > PID_RELAY_DUTY_CYCLE_PERIOD)
  { //time to shift the Relay Window
    windowStartTime_01 += PID_RELAY_DUTY_CYCLE_PERIOD;
  }
  if (Output_01 < millis() - windowStartTime_01) {
    digitalWrite(SSR_PIN_01, HIGH);
    ui_StsSSRPin_01 = false;
  } else {
      if (Safety_Ok && !ThermalRunawayDetected && Mode != SIMULATION_MODE) {
        digitalWrite(SSR_PIN_01, LOW); // esp has sinking outputs
        ui_StsSSRPin_01 = true;
      }
  }
  
  if (millis() - windowStartTime_02 > PID_RELAY_DUTY_CYCLE_PERIOD)
  { //time to shift the Relay Window
    windowStartTime_02 += PID_RELAY_DUTY_CYCLE_PERIOD;
  }
  if (Output_02 < millis() - windowStartTime_02) {
    digitalWrite(SSR_PIN_02, HIGH);
    ui_StsSSRPin_02 = false;
  } else {
      if (Safety_Ok && !ThermalRunawayDetected && Mode != SIMULATION_MODE) {
        digitalWrite(SSR_PIN_02, LOW); // esp has sinking outputs
        ui_StsSSRPin_02 = true;
      }
  }
  if (!Safety_Ok || ThermalRunawayDetected || Mode == SIMULATION_MODE) { // turn off sinking outputs
    digitalWrite(SSR_PIN_01, HIGH);
    ui_StsSSRPin_02 = false;
    digitalWrite(SSR_PIN_02, HIGH);
    ui_StsSSRPin_02 = false;
  }
}
void setupPID() {
  windowStartTime_01 = millis();
  windowStartTime_02 = millis();
  //initialize the variables we're linked to
  Setpoint_ch0 = 0.0;
  Setpoint_ch1 = 0.0;
  //tell the PID to range between 0 and the full window size
  PID_01.SetOutputLimits(0, PID_RELAY_DUTY_CYCLE_PERIOD);
  PID_02.SetOutputLimits(0, PID_RELAY_DUTY_CYCLE_PERIOD);
  //turn the PID on
  PID_01.SetMode(AUTOMATIC);
  PID_02.SetMode(AUTOMATIC);
}

/* temperature */
#include <SPI.h>
#include "Adafruit_MAX31855.h"
#define TEMP_AVG_ARR_SIZE 100
#define MAX_ALLOWED_TEMPERATURE 3500.0
#define MIN_ALLOWED_TEMPERATURE -32.0
#define TEMPERATURE_SAMPLE_RATE 1000
#define MAXCS_CH0   D8 // D8 is GPIO15 SPI (CS) on NodeMCU
#define MAXCS_CH1   D7 // D3 is GPIO0 connected to FLASH button on NodeMCU
Adafruit_MAX31855 thermocouple_ch0(MAXCS_CH0);
Adafruit_MAX31855 thermocouple_ch1(MAXCS_CH1);
int idx_ch0Readings = 0, idx_ch1Readings;
double t_ch0Readings[TEMP_AVG_ARR_SIZE] = {0.0};
double t_ch1Readings[TEMP_AVG_ARR_SIZE] = {0.0};
double t_ch0_raw, t_ch1_raw;
double temperatureLast_ch0 = 0.0, temperatureLast_ch1 = 0.0;
double t_ch0Tot = 0.0, t_ch1Tot = 0.0;
double t_ch0_fromLow = 1.0, t_ch0_fromHigh = 3000.0, t_ch0_toLow = 1.0, t_ch0_toHigh = 3000.0;
double t_ch1_fromLow = 1.0, t_ch1_fromHigh = 3000.0, t_ch1_toLow = 1.0, t_ch1_toHigh = 3000.0;
unsigned long SampleTemperature_Timer = millis();
int NumberOfSamples_ch0 = 0, NumberOfSamples_ch1 = 0; // will need this because array is full of zeros on boot
void handleTemperature() {
  t_ch0_raw = thermocouple_ch0.readFahrenheit();
  t_ch1_raw = thermocouple_ch1.readFahrenheit();
  if (millis()-SampleTemperature_Timer > TEMPERATURE_SAMPLE_RATE) {
    SampleTemperature_Timer = millis();
    double t_ch0 = map(t_ch0_raw,t_ch0_fromLow,t_ch0_fromHigh,t_ch0_toLow,t_ch0_toHigh); //map(value,fromlow,fromhigh,tolow,tohigh);
    if (isnan(t_ch0) || t_ch0 < MIN_ALLOWED_TEMPERATURE || t_ch0 > MAX_ALLOWED_TEMPERATURE) {
      //temperature_ch0 = MAX_ALLOWED_TEMPERATURE; // fail high - nevermind, if it fails high briefly and comes back down the segment thinks its reached its setpoint. god dammit.
    } else {
      if (NumberOfSamples_ch0 < TEMP_AVG_ARR_SIZE) NumberOfSamples_ch0++;
      /* smoothing */
      t_ch0Tot = t_ch0Tot - t_ch0Readings[idx_ch0Readings];
      t_ch0Readings[idx_ch0Readings] = t_ch0;
      t_ch0Tot = t_ch0Tot + t_ch0Readings[idx_ch0Readings];
      idx_ch0Readings++;
      if (idx_ch0Readings >= TEMP_AVG_ARR_SIZE) idx_ch0Readings = 0;
      temperature_ch0 = t_ch0Tot / NumberOfSamples_ch0;
    }
    double t_ch1 = map(t_ch1_raw,t_ch1_fromLow,t_ch1_fromHigh,t_ch1_toLow,t_ch1_toHigh);
    if (isnan(t_ch1) || t_ch1 < MIN_ALLOWED_TEMPERATURE || t_ch1 > MAX_ALLOWED_TEMPERATURE) {
      //temperature_ch1 = MAX_ALLOWED_TEMPERATURE; // fail high
    } else {
      if (NumberOfSamples_ch1 < TEMP_AVG_ARR_SIZE) NumberOfSamples_ch1++;
      /* smoothing */
      t_ch1Tot = t_ch1Tot - t_ch1Readings[idx_ch1Readings];
      t_ch1Readings[idx_ch1Readings] = t_ch1;
      t_ch1Tot = t_ch1Tot + t_ch1Readings[idx_ch1Readings];
      idx_ch1Readings++;
      if (idx_ch1Readings >= TEMP_AVG_ARR_SIZE) idx_ch1Readings = 0;
      temperature_ch1 = t_ch1Tot / NumberOfSamples_ch1;
    }
  }
  if (Mode == SIMULATION_MODE) {
    temperature_ch0 = Setpoint_ch0;
    temperature_ch1 = Setpoint_ch1;
  }
}
void setupThermocouples() {
  if (!thermocouple_ch0.begin()) {
    //Serial.println("ERROR.");
    while (1) delay(10);
  }
  pinMode(MAXCS_CH0,OUTPUT);
  if (!thermocouple_ch1.begin()) {
    //Serial.println("ERROR.");
    while (1) delay(10);
  }
  pinMode(MAXCS_CH1,OUTPUT); //  this is needed AFTER thermocouple.begin because we are using a SPI pin
  
}

/* calibration */
bool t_ch0_cal_low = false, t_ch1_cal_low = false;
bool t_ch0_cal_high = false, t_ch1_cal_high = false;
double t_ch0_actual = 100.0, t_ch1_actual = 100.0;
void handleCal() {
  if (t_ch0_cal_low) {
    t_ch0_cal_low = false;
    t_ch0_fromLow = t_ch0_raw;
    t_ch0_toLow = t_ch0_actual;
  }
  if (t_ch1_cal_low) {
    t_ch1_cal_low = false;
    t_ch1_fromLow = t_ch1_raw;
    t_ch1_toLow = t_ch1_actual;
  }
  if (t_ch0_cal_high) {
    t_ch0_cal_high = false;
    t_ch0_fromHigh = t_ch0_raw;
    t_ch0_toHigh = t_ch0_actual;
  }
  if (t_ch1_cal_high) {
    t_ch1_cal_high = false;
    t_ch1_fromHigh = t_ch1_raw;
    t_ch1_toHigh = t_ch1_actual;
  }
}

/* profile */
#define SEGMENT_STATE_IDLE    0
#define SEGMENT_STATE_RAMP    1
#define SEGMENT_STATE_SOAK    2
#define SEGMENT_STATE_HOLD    3
#define SEGMENT_STATE_INIT    4
#define SEGMENT_STATE_START   5
#define NUMBER_OF_SCHEDULES   5
#define NUMBER_OF_SEGMENTS    10
#define RATE_TIMER_PERIOD     60000
#define RAMP_TIMER_PERIOD     1000 //1000
#define MAX_STRING_LENGTH     16 // space is wasted when this is an odd number because a modbus register is 2 bytes and fits 2 characters
uint16_t SOAK_TIMER_PERIOD = 0;
unsigned long RampTimer, SoakTimer, RateTimer;
bool RampTimerEnabled = false;
bool SoakTimerEnabled = false;
uint16_t ProfileSequence = SEGMENT_STATE_IDLE;
uint16_t SegmentIndex = 0;
bool Segment_CheckDirection = false;
bool Segment_WillIncrement_Ch0 = false;
bool Segment_WillIncrement_Ch1 = false;
bool Segment_AtTemp_Ch0 = false;
bool Segment_AtTemp_Ch1 = false;
double MeasuredRatePerHour_ch0, MeasuredRatePerHour_ch1;
bool ui_StartProfile = false;
bool ui_StopProfile = false;
bool ui_Segment_HoldReleaseRequest = false;
bool ui_Segment_HoldRelease = false;
bool ui_SelectSchedule = false;
uint16_t ui_SelectedSchedule = 0, ui_SelectedScheduleLast = 1;
uint16_t ui_ChangeSelectedSchedule = 0, ui_ChangeSelectedSegment = 0;
double ui_Setpoint = 0.0;
struct TIME {
  uint16_t hours;
  uint16_t minutes;
  uint16_t seconds;
  uint16_t milliseconds;
} Segment_TimeRemaining; 
struct SCHEDULE_SEGMENT { // 1 segment is 30 bytes when MAX_STRING_LENGTH = 15
  char Name[MAX_STRING_LENGTH] = {'\0'}; // char is 1 byte 15
  bool Enabled; // bool is 1 byte
  bool HoldEnabled; // bool is 1 byte
  double Setpoint;    // in degrees(C) // double is 8 bytes
  uint16_t RampRate;  // in degrees(C)/hour // uint16_t is 2 bytes
  uint16_t SoakTime;  // in minutes // uint16_t is 2 bytes
  uint8_t State; // uint8_t is 1 bytes
};
struct SCHEDULE { // each schedule is 303 bytes when NUMBER_OF_SEGMENTS = 10 and MAX_STRING_LENGTH = 15
  char Name[MAX_STRING_LENGTH] = {'\0'}; // char is 1 byte
  bool CMD_Select; // bool is 1 byte
  bool STS_Select; // bool is 1 byte
  SCHEDULE_SEGMENT Segments[NUMBER_OF_SEGMENTS]; // 1 segment is 30 - when NUMBER_OF_SEGMENTS = 10 this is 300
} Schedules[NUMBER_OF_SCHEDULES],LoadedSchedule;
void handleProfileSequence(){
  //
  // manage sequencing the profile
  //
  if (ui_StopProfile || !Safety_Ok || ThermalRunawayDetected) {
    ProfileSequence = SEGMENT_STATE_IDLE;
  }
  ui_StopProfile = false;
  
  switch (ProfileSequence) {
    case SEGMENT_STATE_IDLE: /* idle */
      // TODO: set schedule name
      RampTimer = millis(); RampTimerEnabled = false;
      SoakTimer = millis(); SoakTimerEnabled = false;
      RateTimer = millis();
      if (Mode != MANUAL_MODE) {
        Setpoint_ch0 = 0.0;
        Setpoint_ch1 = 0.0;
      }
      if (Safety_Ok && ui_StartProfile == true && 
          (Mode == AUTOMATIC_MODE || Mode == SIMULATION_MODE) && 
          !ThermalRunawayDetected) {
        ProfileSequence = SEGMENT_STATE_INIT;
      }
      SegmentIndex = 0;
      ui_StartProfile = false;
      ui_Segment_HoldReleaseRequest = false;
      break;
      
    case SEGMENT_STATE_INIT: /* initialize */
      // TODO: set segment name
      // TODO: put PIDs in auto
      Setpoint_ch0 = temperature_ch0;
      Setpoint_ch1 = temperature_ch1;
      // TODO: check if PIDs are in auto before moving to next sequence
      // TODO: add watchdog timer for PIDs in auto
      ProfileSequence = SEGMENT_STATE_START;
      break;
      
    case SEGMENT_STATE_START: /* first segment start */
      if (LoadedSchedule.Segments[SegmentIndex].Enabled) {
        Segment_CheckDirection          = true;
        Segment_WillIncrement_Ch0       = false;
        Segment_WillIncrement_Ch1       = false;
        Segment_AtTemp_Ch0              = false;
        Segment_AtTemp_Ch1              = false;
        ui_Segment_HoldReleaseRequest   = false;
        ui_Segment_HoldRelease          = false;
        ProfileSequence = SEGMENT_STATE_RAMP;
      } else {
        if (SegmentIndex < NUMBER_OF_SEGMENTS - 1) {
          SegmentIndex++;
        } else {
          ProfileSequence = SEGMENT_STATE_IDLE;
        }
      }
      break;
      
    case SEGMENT_STATE_RAMP: /* first segment ramp to temp */
      RampTimerEnabled = true;
      break;
      
    case SEGMENT_STATE_SOAK: /* first segment soak at temp */
      SOAK_TIMER_PERIOD = LoadedSchedule.Segments[SegmentIndex].SoakTime;
      SoakTimerEnabled = true;
      break;
      
    default:
      ProfileSequence = SEGMENT_STATE_IDLE;
      break;
  }
  
  LoadedSchedule.Segments[SegmentIndex].State = ProfileSequence;
  
  // 
  // check the direction the temp needs to go to get to setpoint
  //
  if (Segment_CheckDirection) {
    Segment_CheckDirection = false;
    if (temperature_ch0 < LoadedSchedule.Segments[SegmentIndex].Setpoint) {
      Segment_WillIncrement_Ch0 = true;
    } else {
      Segment_WillIncrement_Ch0 = false;
    }
    if (temperature_ch1 < LoadedSchedule.Segments[SegmentIndex].Setpoint) {
      Segment_WillIncrement_Ch1 = true;
    } else {
      Segment_WillIncrement_Ch1 = false;
    }
  }

  //
  // calculate the amount of change to make per the ramp rate
  //
  unsigned long RampTimer_ElapsedTime = millis() - RampTimer;
  if ((RampTimer_ElapsedTime > RAMP_TIMER_PERIOD) || !RampTimerEnabled){
    RampTimer = millis();
    if (RampTimerEnabled) {
      RampTimerEnabled = false;
      
      if (Segment_WillIncrement_Ch0) {
        if (temperature_ch0 >= LoadedSchedule.Segments[SegmentIndex].Setpoint) {
          Segment_AtTemp_Ch0 = true;
        } else {
          if (Setpoint_ch0 < LoadedSchedule.Segments[SegmentIndex].Setpoint) {
            Setpoint_ch0 = Setpoint_ch0 + RampChange(LoadedSchedule.Segments[SegmentIndex].RampRate,RampTimer_ElapsedTime);
          }
        }
      } else {
        if (temperature_ch0 <= LoadedSchedule.Segments[SegmentIndex].Setpoint) {
          Segment_AtTemp_Ch0 = true;
        } else {
          if (Setpoint_ch0 > LoadedSchedule.Segments[SegmentIndex].Setpoint) {
            Setpoint_ch0 = Setpoint_ch0 - RampChange(LoadedSchedule.Segments[SegmentIndex].RampRate,RampTimer_ElapsedTime);
          }
        }
      }
      
      if (Segment_WillIncrement_Ch1) {
        if (temperature_ch1 >= LoadedSchedule.Segments[SegmentIndex].Setpoint) {
          Segment_AtTemp_Ch1 = true;
        } else {
          if (Setpoint_ch1 < LoadedSchedule.Segments[SegmentIndex].Setpoint) {
            Setpoint_ch1 = Setpoint_ch1 + RampChange(LoadedSchedule.Segments[SegmentIndex].RampRate,RampTimer_ElapsedTime);
          }
        }
      } else {
        if (temperature_ch1 <= LoadedSchedule.Segments[SegmentIndex].Setpoint) {
          Segment_AtTemp_Ch1 = true;
        } else {
          if (Setpoint_ch1 > LoadedSchedule.Segments[SegmentIndex].Setpoint) {
            Setpoint_ch1 = Setpoint_ch1 - RampChange(LoadedSchedule.Segments[SegmentIndex].RampRate,RampTimer_ElapsedTime);
          }
        }
      } 
    }
  }
  /* advance profile once ramp temp reached */
  if ((Segment_AtTemp_Ch0 == true) && (Segment_AtTemp_Ch1 == true)) {
    Segment_AtTemp_Ch0 = false;
    Segment_AtTemp_Ch1 = false;
    Setpoint_ch0 = LoadedSchedule.Segments[SegmentIndex].Setpoint;
    Setpoint_ch1 = LoadedSchedule.Segments[SegmentIndex].Setpoint;
    ProfileSequence = SEGMENT_STATE_SOAK;
  }

  //
  // handle soak timer
  //
  // TODO: see if unsigned long and millis is big enough here - seconds?
  unsigned long SoakTimer_ElapsedTime = millis() - SoakTimer;
  if ((SoakTimer_ElapsedTime > SOAK_TIMER_PERIOD*60*1000) || !SoakTimerEnabled) { // in minutes - convert to milli
    if (SoakTimerEnabled) {
      if (LoadedSchedule.Segments[SegmentIndex].HoldEnabled) {
        ui_Segment_HoldReleaseRequest = true;
        LoadedSchedule.Segments[SegmentIndex].State = SEGMENT_STATE_HOLD;
        if (ui_Segment_HoldRelease) {
          ui_Segment_HoldRelease = false;
          ui_Segment_HoldReleaseRequest = false;
          SoakTimerEnabled = false;
          if (SegmentIndex < NUMBER_OF_SEGMENTS - 1) {
            SegmentIndex++;
            ProfileSequence = SEGMENT_STATE_START;
          } else {
            ProfileSequence = SEGMENT_STATE_IDLE;
          }
        }
      } else {
        SoakTimerEnabled = false;
        if (SegmentIndex < NUMBER_OF_SEGMENTS - 1) {
          SegmentIndex++;
          ProfileSequence = SEGMENT_STATE_START;
        } else {
          ProfileSequence = SEGMENT_STATE_IDLE;
        }
      }
    } else {
      SoakTimer = millis();
    }
  }

  //
  // measure current rate
  //
  unsigned long RateTimer_ElapsedTime = millis() - RateTimer;
  if (RateTimer_ElapsedTime > RATE_TIMER_PERIOD) {
    RateTimer = millis();
    /* calculate rate ch0 */
    //double MeasuredRatePerSecond_ch0 = fabs(temperature_ch0 - temperatureLast_ch0)*((double)RateTimer_ElapsedTime/1000.0); // use fabs for floats/double instead of abs
    //double MeasuredRatePerMinute_ch0 = MeasuredRatePerSecond_ch0*60.0;
    double MeasuredRatePerMinute_ch0 = fabs(temperature_ch0 - temperatureLast_ch0)*((double)RateTimer_ElapsedTime/RATE_TIMER_PERIOD); // use fabs for floats/double instead of abs
    MeasuredRatePerHour_ch0 = MeasuredRatePerMinute_ch0*60.0;
    temperatureLast_ch0 = temperature_ch0;
    /* calculate rate ch1 */
    //double MeasuredRatePerSecond_ch1 = fabs(temperature_ch1 - temperatureLast_ch1)*((double)RateTimer_ElapsedTime/1000.0);
    //double MeasuredRatePerMinute_ch1 = MeasuredRatePerSecond_ch1*60.0;
    double MeasuredRatePerMinute_ch1 = fabs(temperature_ch1 - temperatureLast_ch1)*((double)RateTimer_ElapsedTime/RATE_TIMER_PERIOD);
    MeasuredRatePerHour_ch1 = MeasuredRatePerMinute_ch1*60.0;
    temperatureLast_ch1 = temperature_ch1;
  }

  //
  // calculate time remaining
  //
  double dbl_hours, dbl_minutes, dbl_seconds, dbl_milli;
  switch (ProfileSequence) {
    case SEGMENT_STATE_IDLE:
      dbl_hours = 0.0;
      dbl_minutes = 0.0;
      dbl_seconds = 0.0;
      dbl_milli = 0.0;
      MeasuredRatePerHour_ch0 = 0.0;
      MeasuredRatePerHour_ch1 = 0.0;
      break;
    case SEGMENT_STATE_RAMP:
      // calculate time remaining ch0
      double dbl_hours_ch0, dbl_minutes_ch0, dbl_seconds_ch0, dbl_milli_ch0;
      if (MeasuredRatePerHour_ch0 > 0.0) {
        double temperatureDifference = fabs(LoadedSchedule.Segments[SegmentIndex].Setpoint - temperature_ch0);
        dbl_hours_ch0 = temperatureDifference / MeasuredRatePerHour_ch0;
      } else {
        dbl_hours_ch0 = 0.0;
      }
      // calculate time remaining ch1
      double dbl_hours_ch1, dbl_minutes_ch1, dbl_seconds_ch1, dbl_milli_ch1;
      if (MeasuredRatePerHour_ch1 > 0.0) {
        double temperatureDifference = fabs(LoadedSchedule.Segments[SegmentIndex].Setpoint - temperature_ch1);
        dbl_hours_ch1 = temperatureDifference / MeasuredRatePerHour_ch1;
      } else {
        dbl_hours_ch1 = 0.0;
      }
      // see which has the most remaining time and use it for calculation
      if (dbl_hours_ch0 > dbl_hours_ch1) {
        dbl_hours = dbl_hours_ch0;
      } else {
        dbl_hours = dbl_hours_ch1;
      }
      // calc min, sec, and milli based off hours - note that casting a double will truncate it. this is how we get the whole number.
      dbl_minutes = (dbl_hours - (uint16_t)dbl_hours)*60.0;
      dbl_seconds = (dbl_minutes - (uint16_t)dbl_minutes)*60.0;
      dbl_milli = (dbl_seconds - (uint16_t)dbl_seconds)*1000;
      break;
    case SEGMENT_STATE_SOAK:
      // set time remaining
      if (ui_Segment_HoldReleaseRequest) {
          dbl_hours = 0.0;
          dbl_minutes = 0.0;
          dbl_seconds = 0.0;
          dbl_milli = 0.0;
      } else {
        dbl_hours = LoadedSchedule.Segments[SegmentIndex].SoakTime/60.0 - ((((double)SoakTimer_ElapsedTime/1000.0)/60.0)/60.0);
        dbl_minutes = (dbl_hours - (uint16_t)dbl_hours)*60.0;
        dbl_seconds = (dbl_minutes - (uint16_t)dbl_minutes)*60.0;
        dbl_milli = (dbl_seconds - (uint16_t)dbl_seconds)*1000;
      }
      break;
  }
  Segment_TimeRemaining.hours = (uint16_t)dbl_hours;
  Segment_TimeRemaining.minutes = (uint16_t)dbl_minutes;
  Segment_TimeRemaining.seconds = (uint16_t)dbl_seconds;
  Segment_TimeRemaining.milliseconds = (uint16_t)dbl_milli;

  switch (Mode){
    case AUTOMATIC_MODE:
      if (ProfileSequence == SEGMENT_STATE_IDLE) {
        // just display which is greater. should both be 0.0
        if (Setpoint_ch0 > Setpoint_ch1) {
          ui_Setpoint = Setpoint_ch0;
        } else {
          ui_Setpoint = Setpoint_ch1;
        }
      } else {
        ui_Setpoint = LoadedSchedule.Segments[SegmentIndex].Setpoint;
      }
      break;
    case MANUAL_MODE:
      Setpoint_ch0 = ui_Setpoint;
      Setpoint_ch1 = ui_Setpoint;
      break;
    case SIMULATION_MODE:
      if (ProfileSequence == SEGMENT_STATE_IDLE) {
        // just display which is greater. should both be 0.0
        if (Setpoint_ch0 > Setpoint_ch1) {
          ui_Setpoint = Setpoint_ch0;
        } else {
          ui_Setpoint = Setpoint_ch1;
        }
      } else {
        ui_Setpoint = LoadedSchedule.Segments[SegmentIndex].Setpoint;
      }
      break;
    default:
      break;
  }
}
float RampChange (uint16_t RampRate, unsigned long ElapsedTime) {
  return (((float)RampRate/60.0)/(60.0/((float)ElapsedTime/1000.0)));
}
void setSchedule () {
  int idxSelectedSchedule = 0;
  bool setSchedule = false;

  // if select schedule is set
  if (ui_SelectSchedule && ui_SelectedSchedule < NUMBER_OF_SCHEDULES) {
    Schedules[ui_SelectedSchedule].CMD_Select = true;
    ui_SelectedScheduleLast = ui_SelectedSchedule;
  }
  ui_SelectSchedule = false;

  // if selected schedule changes
  if (ui_SelectedSchedule != ui_SelectedScheduleLast) {
    if (ProfileSequence == SEGMENT_STATE_IDLE && ui_SelectedSchedule < NUMBER_OF_SCHEDULES && ui_SelectedSchedule >= 0){
      ui_SelectedScheduleLast = ui_SelectedSchedule;
      Schedules[ui_SelectedSchedule].CMD_Select = true;
    } else {
      ui_SelectedSchedule = ui_SelectedScheduleLast;
    }
  }
  
  if (ProfileSequence == SEGMENT_STATE_IDLE) {
    for (int i=0;i<NUMBER_OF_SCHEDULES;i++) {
      if (Schedules[i].CMD_Select){
        idxSelectedSchedule = i;
        setSchedule = true;
      }
    }
    if (setSchedule) {
      setSchedule = false;
      for (int i=0;i<NUMBER_OF_SCHEDULES;i++) {
        Schedules[i].CMD_Select = false;
        if (i != idxSelectedSchedule) {
          Schedules[i].STS_Select = false;
        } else {
          Schedules[i].STS_Select = true;
        }
      }

      /* copy schedule to 0 index */
      for (int i=0; i<sizeof(Schedules[idxSelectedSchedule].Name);i++) {
         LoadedSchedule.Name[i] = Schedules[idxSelectedSchedule].Name[i];
      }
      for (int i=0;i<NUMBER_OF_SEGMENTS;i++) {
        for (int j=0; j<sizeof(Schedules[idxSelectedSchedule].Segments[i].Name);j++) {
           LoadedSchedule.Segments[i].Name[j] = Schedules[idxSelectedSchedule].Segments[i].Name[j];
        }
        LoadedSchedule.Segments[i].Enabled = Schedules[idxSelectedSchedule].Segments[i].Enabled;
        LoadedSchedule.Segments[i].HoldEnabled = Schedules[idxSelectedSchedule].Segments[i].HoldEnabled;
        LoadedSchedule.Segments[i].Setpoint = Schedules[idxSelectedSchedule].Segments[i].Setpoint;
        LoadedSchedule.Segments[i].RampRate = Schedules[idxSelectedSchedule].Segments[i].RampRate;
        LoadedSchedule.Segments[i].SoakTime = Schedules[idxSelectedSchedule].Segments[i].SoakTime;
        LoadedSchedule.Segments[i].State = Schedules[idxSelectedSchedule].Segments[i].State;
      }
    }
  }
  
}

/* safety */
#define THERMAL_RUNAWAY_TEMPERATURE_TIMER       600000 // 600000 is 10 min
#define THERMAL_RUNAWAY_RATE_TIMER              600000 // should not be set less that the sample period of the rate (RATE_TIMER_PERIOD)
#define THERMAL_RUNAWAY_TOLERANCE_TEMPERATURE   0.03 // percentage
#define THERMAL_RUNAWAY_TOLERANCE_RATE          0.30
bool TemperatureDifferenceDetected = false;
bool RateDifferenceDetected = false;
unsigned long ThermalRunawayTemperature_Timer = millis();
unsigned long ThermalRunawayRate_Timer = millis();
unsigned long ThermalRunawayTemperatureTimer_Elapsed = 0;
unsigned long ThermalRunawayRateTimer_Elapsed = 0;
int SafetyInputLast = 0;
void handleThermalRunaway() {

  double RateTolerance = LoadedSchedule.Segments[SegmentIndex].RampRate*THERMAL_RUNAWAY_TOLERANCE_RATE;
  double TempTolerance = LoadedSchedule.Segments[SegmentIndex].Setpoint*THERMAL_RUNAWAY_TOLERANCE_TEMPERATURE;

  switch (ProfileSequence) {
    case SEGMENT_STATE_IDLE:
      break;
    case SEGMENT_STATE_INIT:
      break;
    case SEGMENT_STATE_START:
      break;
    case SEGMENT_STATE_RAMP:
      /* ch0 */
      if ((MeasuredRatePerHour_ch0 > LoadedSchedule.Segments[SegmentIndex].RampRate + RateTolerance) || 
          (MeasuredRatePerHour_ch0 < LoadedSchedule.Segments[SegmentIndex].RampRate - RateTolerance)) {
            RateDifferenceDetected = true;
          }
      /* ch1 */
      if ((MeasuredRatePerHour_ch1 > LoadedSchedule.Segments[SegmentIndex].RampRate + RateTolerance) || 
          (MeasuredRatePerHour_ch1 < LoadedSchedule.Segments[SegmentIndex].RampRate - RateTolerance)) {
            RateDifferenceDetected = true;
          }
      /* ch0 */
      if ((temperature_ch0 > Setpoint_ch0 + TempTolerance) || 
          (temperature_ch0 < Setpoint_ch0 - TempTolerance)) {
            TemperatureDifferenceDetected = true;
          }
      /* ch1 */
      if ((temperature_ch1 > Setpoint_ch1 + TempTolerance) || 
          (temperature_ch1 < Setpoint_ch1 - TempTolerance)) {
            TemperatureDifferenceDetected = true;
          }
      break;
    case SEGMENT_STATE_SOAK:
      /* ch0 */
      if ((temperature_ch0 > LoadedSchedule.Segments[SegmentIndex].Setpoint + TempTolerance) || 
          (temperature_ch0 < LoadedSchedule.Segments[SegmentIndex].Setpoint - TempTolerance)) {
            TemperatureDifferenceDetected = true;
          }
      /* ch1 */
      if ((temperature_ch1 > LoadedSchedule.Segments[SegmentIndex].Setpoint + TempTolerance) || 
          (temperature_ch1 < LoadedSchedule.Segments[SegmentIndex].Setpoint - TempTolerance)) {
            TemperatureDifferenceDetected = true;
          }
      break;
  }
  // check temperature difference
  ThermalRunawayTemperatureTimer_Elapsed = millis() - ThermalRunawayTemperature_Timer;
  if (ThermalRunawayTemperatureTimer_Elapsed > THERMAL_RUNAWAY_TEMPERATURE_TIMER || !TemperatureDifferenceDetected) {
    if (TemperatureDifferenceDetected) {
      ThermalRunawayDetected = true;
    } else {
      ThermalRunawayTemperature_Timer = millis();
    }
  }
  TemperatureDifferenceDetected = false;
  // check rate difference
  ThermalRunawayRateTimer_Elapsed = millis() - ThermalRunawayRate_Timer;
  if (ThermalRunawayRateTimer_Elapsed > THERMAL_RUNAWAY_RATE_TIMER || !RateDifferenceDetected) {
    if (RateDifferenceDetected) {
      //ThermalRunawayDetected = true;
    } else {
      ThermalRunawayRate_Timer = millis();
    }
  }
  RateDifferenceDetected = false;
  // need to physically press the stop button to reset
  if (!Safety_Ok || ui_ThermalRunawayOverride) {
    ThermalRunawayTemperature_Timer = millis();
    ThermalRunawayRate_Timer = millis();
    ThermalRunawayDetected = false;
  }

}
void handleSafetyCircuit() {
  SafetyInputLast = digitalRead(SAFETY_CIRCUIT_INPUT);
  if (SafetyInputLast == HIGH) Safety_Ok = true;
  else Safety_Ok = false;
}
void handleMainContactor() {
  if (Safety_Ok && !ThermalRunawayDetected && Mode != SIMULATION_MODE) {
    digitalWrite(MAIN_CONTACTOR_OUTPUT, LOW);
  } else {
    digitalWrite(MAIN_CONTACTOR_OUTPUT, HIGH);
  }
}

/* modbus */
#include <ModbusRTU.h> // https://github.com/emelianov/modbus-esp8266
#include <ModbusIP_ESP8266.h>
#define SLAVE_ID                  1
/* coils (RW) */
#define MB_CMD_SELECT_SCHEDULE    1
#define MB_CMD_START_PROFILE      2
#define MB_CMD_STOP_PROFILE       3
#define MB_CMD_HOLD_RELEASE       4
#define MB_CMD_THERM_OVERRIDE     5
#define MB_CMD_WRITE_EEPROM       6
#define MB_SCH_SEG_ENABLED        7
#define MB_SCH_SEG_HOLD_EN        8
#define MB_CMD_CAL_CH0_LOW        9
#define MB_CMD_CAL_CH1_LOW        10
#define MB_CMD_CAL_CH0_HIGH       11
#define MB_CMD_CAL_CH1_HIGH       12
/* input status (R) */
#define MB_STS_SSR_01             1
#define MB_STS_SSR_02             2
#define MB_STS_RELEASE_REQ        3
#define MB_STS_SAFETY_OK          4
#define MB_STS_IN_PROCESS         5
#define MB_STS_THERMAL_RUNAWAY    6
#define MB_STS_EEPROM_WRITTEN     7 //50
/* holding registers (RW) 16 bit */
#define MB_MODE                   1
#define MB_CMD_SELECTED_SCHEDULE  2
#define MB_CMD_SETPOINT           3
#define MB_PID_P_01               5
#define MB_PID_I_01               7
#define MB_PID_D_01               9
#define MB_PID_P_02               11
#define MB_PID_I_02               13
#define MB_PID_D_02               15
#define MB_SCH_NAME               17
#define MB_SCH_SEG_NAME           25
#define MB_SCH_SEG_SETPOINT       33
#define MB_SCH_SEG_RAMP_RATE      35
#define MB_SCH_SEG_SOAK_TIME      36
#define MB_SCH_SEG_SELECTED       37
#define MB_SCH_SELECTED           38
#define MB_CAL_TEMP_ACT_CH0       39 
#define MB_CAL_TEMP_ACT_CH1       41
/* input registers (R) 16 bit */
#define MB_HEARTBEAT              1
#define MB_STS_REMAINING_TIME_H   2
#define MB_STS_REMAINING_TIME_M   3
#define MB_STS_REMAINING_TIME_S   4
#define MB_STS_TEMPERATURE_01     5
#define MB_STS_TEMPERATURE_02     7
#define MB_STS_PID_01_OUTPUT      9
#define MB_STS_PID_02_OUTPUT      11
#define MB_NUMBER_OF_SCHEDULES    13
#define MB_NUMBER_OF_SEGMENTS     14
#define MB_STS_SEGMENT_STATE      15
#define MB_STS_SEGMENT_NAME       16
#define MB_STS_SCHEDULE_NAME      24 // this is 8 regs long - next should start at 32
#define MB_STS_TEMP_01_RAW        32
#define MB_STS_TEMP_02_RAW        34
#define MB_STS_RUNAWAY_TEMP_T     36
#define MB_STS_RUNAWAY_RATE_T     38
#define MB_STS_MEAS_RATE_CH0      40
#define MB_STS_MEAS_RATE_CH1      42
ModbusRTU mb_rtu;
ModbusIP mb_ip;
int HEARTBEAT_VALUE = 0;
bool ui_EepromWritten = false;
unsigned long EepromWritten_Timer = millis();
bool ui_WriteEeprom = false;
union floatAsBytes {
  float fval;
  uint16_t ui[2];
  byte bval[4];
};
union uintLongAsUint {
  unsigned long uLongVal;
  uint16_t ui[2];
};
union charAsUnit16 {
  uint16_t reg;
  char c[2];
};
void DoubleToIreg(uint16_t reg, double val) {
  floatAsBytes flt; 
  flt.fval = val;
  mb_rtu.Ireg(reg, flt.ui[0]);
  mb_rtu.Ireg(reg+1, flt.ui[1]);
}
void UnsignedLongToIreg(uint16_t reg, unsigned long val) {
  uintLongAsUint uLong; 
  uLong.uLongVal = val;
  mb_rtu.Ireg(reg, uLong.ui[0]);
  mb_rtu.Ireg(reg+1, uLong.ui[1]);
}
void DoubleToHreg(uint16_t reg, double val) {
  floatAsBytes flt; 
  flt.fval = val;
  mb_rtu.Hreg(reg, flt.ui[0]);
  mb_rtu.Hreg(reg+1, flt.ui[1]);
}
double HregToDouble(uint16_t reg) {
  floatAsBytes flt; 
  flt.ui[0] = mb_rtu.Hreg(reg);
  flt.ui[1] = mb_rtu.Hreg(reg+1);
  return flt.fval;
}
void handleModbus() {
  /* prevent arrays from going out of bounds from ui */
  if (ui_SelectedSchedule >= NUMBER_OF_SCHEDULES) ui_SelectedSchedule = NUMBER_OF_SCHEDULES - 1;
  if (ui_SelectedSchedule < 0) ui_SelectedSchedule = 0;
  if (SegmentIndex >= NUMBER_OF_SEGMENTS) SegmentIndex = NUMBER_OF_SEGMENTS - 1;
  if (SegmentIndex < 0) SegmentIndex = 0;
  if (ui_ChangeSelectedSchedule >= NUMBER_OF_SCHEDULES) ui_ChangeSelectedSchedule = NUMBER_OF_SCHEDULES - 1;
  if (ui_ChangeSelectedSchedule < 0) ui_ChangeSelectedSchedule = 0;
  if (ui_ChangeSelectedSegment >= NUMBER_OF_SEGMENTS) ui_ChangeSelectedSegment = NUMBER_OF_SEGMENTS - 1;
  if (ui_ChangeSelectedSegment < 0) ui_ChangeSelectedSegment = 0;
  /* coils (RW) */
  mb_rtu.Coil(MB_CMD_SELECT_SCHEDULE, ui_SelectSchedule);
  mb_rtu.Coil(MB_CMD_START_PROFILE, ui_StartProfile);
  mb_rtu.Coil(MB_CMD_STOP_PROFILE, ui_StopProfile);
  mb_rtu.Coil(MB_CMD_HOLD_RELEASE, ui_Segment_HoldRelease);
  mb_rtu.Coil(MB_CMD_THERM_OVERRIDE, ui_ThermalRunawayOverride);
  mb_rtu.Coil(MB_CMD_WRITE_EEPROM, ui_WriteEeprom);
  mb_rtu.Coil(MB_SCH_SEG_ENABLED, Schedules[ui_ChangeSelectedSchedule].Segments[ui_ChangeSelectedSegment].Enabled);
  mb_rtu.Coil(MB_SCH_SEG_HOLD_EN, Schedules[ui_ChangeSelectedSchedule].Segments[ui_ChangeSelectedSegment].HoldEnabled);
  mb_rtu.Coil(MB_CMD_CAL_CH0_LOW, t_ch0_cal_low);
  mb_rtu.Coil(MB_CMD_CAL_CH1_LOW, t_ch1_cal_low);
  mb_rtu.Coil(MB_CMD_CAL_CH0_HIGH, t_ch0_cal_high);
  mb_rtu.Coil(MB_CMD_CAL_CH1_HIGH, t_ch1_cal_high);
  /* input status (R) */
  mb_rtu.Ists(MB_STS_RELEASE_REQ, ui_Segment_HoldReleaseRequest);
  mb_rtu.Ists(MB_STS_SSR_01, ui_StsSSRPin_01);
  mb_rtu.Ists(MB_STS_SSR_02, ui_StsSSRPin_02);
  mb_rtu.Ists(MB_STS_SAFETY_OK, Safety_Ok);
  mb_rtu.Ists(MB_STS_IN_PROCESS, ProfileSequence);
  mb_rtu.Ists(MB_STS_THERMAL_RUNAWAY, ThermalRunawayDetected);
  mb_rtu.Ists(MB_STS_EEPROM_WRITTEN, ui_EepromWritten);
  /* holding registers (RW) */
  mb_rtu.Hreg(MB_MODE, Mode);
  mb_rtu.Hreg(MB_CMD_SELECTED_SCHEDULE, ui_SelectedSchedule);
  DoubleToHreg(MB_CMD_SETPOINT, ui_Setpoint);
  DoubleToHreg(MB_PID_P_01, Kp_01);
  DoubleToHreg(MB_PID_I_01, Ki_01);
  DoubleToHreg(MB_PID_D_01, Kd_01);
  DoubleToHreg(MB_PID_P_02, Kp_02);
  DoubleToHreg(MB_PID_I_02, Ki_02);
  DoubleToHreg(MB_PID_D_02, Kd_02);
  DoubleToHreg(MB_CAL_TEMP_ACT_CH0, t_ch0_actual);
  DoubleToHreg(MB_CAL_TEMP_ACT_CH1, t_ch1_actual);
  int y = 0;
  for (int i=0; i<sizeof(Schedules[ui_ChangeSelectedSchedule].Name);i=i+2) {
    charAsUnit16 temp;
    temp.c[0] = Schedules[ui_ChangeSelectedSchedule].Name[i];
    if (i+1<sizeof(Schedules[ui_ChangeSelectedSchedule].Name)) { // fix struct overwrite issue when MAX_STRING_LENGTH is odd
      temp.c[1] = Schedules[ui_ChangeSelectedSchedule].Name[i + 1];
    } else {
      temp.c[1] = ' ';
    }
    mb_rtu.Hreg(MB_SCH_NAME + y, temp.reg);
    y++;
  }
  int x = 0;
  for (int i=0; i<sizeof(Schedules[ui_ChangeSelectedSchedule].Segments[ui_ChangeSelectedSegment].Name);i=i+2) {
    charAsUnit16 temp;
    temp.c[0] = Schedules[ui_ChangeSelectedSchedule].Segments[ui_ChangeSelectedSegment].Name[i];
    if (i+1<sizeof(Schedules[ui_ChangeSelectedSchedule].Segments[ui_ChangeSelectedSegment].Name)) { // fix struct overwrite issue when MAX_STRING_LENGTH is odd
      temp.c[1] = Schedules[ui_ChangeSelectedSchedule].Segments[ui_ChangeSelectedSegment].Name[i + 1];
    } else {
      temp.c[1] = ' ';
    }
    mb_rtu.Hreg(MB_SCH_SEG_NAME + x, temp.reg);
    x++;
  }
  DoubleToHreg(MB_SCH_SEG_SETPOINT, Schedules[ui_ChangeSelectedSchedule].Segments[ui_ChangeSelectedSegment].Setpoint);
  mb_rtu.Hreg(MB_SCH_SEG_RAMP_RATE, Schedules[ui_ChangeSelectedSchedule].Segments[ui_ChangeSelectedSegment].RampRate);
  mb_rtu.Hreg(MB_SCH_SEG_SOAK_TIME, Schedules[ui_ChangeSelectedSchedule].Segments[ui_ChangeSelectedSegment].SoakTime);
  mb_rtu.Hreg(MB_SCH_SEG_SELECTED, ui_ChangeSelectedSegment);
  mb_rtu.Hreg(MB_SCH_SELECTED, ui_ChangeSelectedSchedule);
  /* input registers (R) */
  mb_rtu.Ireg(MB_HEARTBEAT, HEARTBEAT_VALUE);
  mb_rtu.Ireg(MB_STS_REMAINING_TIME_H, Segment_TimeRemaining.hours);
  mb_rtu.Ireg(MB_STS_REMAINING_TIME_M, Segment_TimeRemaining.minutes);
  mb_rtu.Ireg(MB_STS_REMAINING_TIME_S, Segment_TimeRemaining.seconds);
  DoubleToIreg(MB_STS_TEMPERATURE_01, temperature_ch0);
  DoubleToIreg(MB_STS_TEMPERATURE_02, temperature_ch1);
  DoubleToIreg(MB_STS_PID_01_OUTPUT, Output_01);
  DoubleToIreg(MB_STS_PID_02_OUTPUT, Output_02);
  //mb_rtu.Ireg(MB_NUMBER_OF_SCHEDULES, NUMBER_OF_SCHEDULES); // written once in setup
  //mb_rtu.Ireg(MB_NUMBER_OF_SEGMENTS, NUMBER_OF_SEGMENTS); // written once in setup
  mb_rtu.Ireg(MB_STS_SEGMENT_STATE, LoadedSchedule.Segments[SegmentIndex].State );
  int j = 0;
  for (int i=0; i<sizeof(Schedules[ui_SelectedSchedule].Segments[SegmentIndex].Name);i=i+2) {
    charAsUnit16 temp;
    temp.c[0] = Schedules[ui_SelectedSchedule].Segments[SegmentIndex].Name[i];
    if (i+1<sizeof(Schedules[ui_SelectedSchedule].Segments[SegmentIndex].Name)) { // fix struct overwrite issue when MAX_STRING_LENGTH is odd
      temp.c[1] = Schedules[ui_SelectedSchedule].Segments[SegmentIndex].Name[i + 1];
    } else {
      temp.c[1] = ' ';
    }
    mb_rtu.Ireg(MB_STS_SEGMENT_NAME + j, temp.reg);
    j++;
  }
  int k = 0;
  for (int i=0; i<sizeof(Schedules[ui_SelectedSchedule].Name);i=i+2) {
    charAsUnit16 temp;
    temp.c[0] = Schedules[ui_SelectedSchedule].Name[i];
    if (i+1<sizeof(Schedules[ui_SelectedSchedule].Name)) { // fix struct overwrite issue when MAX_STRING_LENGTH is odd
      temp.c[1] = Schedules[ui_SelectedSchedule].Name[i+1];
    } else {
      temp.c[1] = ' ';
    }
    mb_rtu.Ireg(MB_STS_SCHEDULE_NAME + k, temp.reg);
    k++;
  }
  DoubleToIreg(MB_STS_TEMP_01_RAW, t_ch0_raw);
  DoubleToIreg(MB_STS_TEMP_02_RAW, t_ch1_raw);
  UnsignedLongToIreg(MB_STS_RUNAWAY_TEMP_T, ThermalRunawayTemperatureTimer_Elapsed);
  UnsignedLongToIreg(MB_STS_RUNAWAY_RATE_T, ThermalRunawayRateTimer_Elapsed);
  DoubleToIreg(MB_STS_MEAS_RATE_CH0, MeasuredRatePerHour_ch0);
  DoubleToIreg(MB_STS_MEAS_RATE_CH1, MeasuredRatePerHour_ch1);

  mb_rtu.task();
  mb_ip.task();
  
  /* coils (RW) */
  ui_SelectSchedule = mb_rtu.Coil(MB_CMD_SELECT_SCHEDULE);
  ui_StartProfile = mb_rtu.Coil(MB_CMD_START_PROFILE);
  ui_StopProfile = mb_rtu.Coil(MB_CMD_STOP_PROFILE);
  ui_Segment_HoldRelease = mb_rtu.Coil(MB_CMD_HOLD_RELEASE);
  ui_ThermalRunawayOverride = mb_rtu.Coil(MB_CMD_THERM_OVERRIDE);
  ui_WriteEeprom = mb_rtu.Coil(MB_CMD_WRITE_EEPROM);
  Schedules[ui_ChangeSelectedSchedule].Segments[ui_ChangeSelectedSegment].Enabled = mb_rtu.Coil(MB_SCH_SEG_ENABLED);
  Schedules[ui_ChangeSelectedSchedule].Segments[ui_ChangeSelectedSegment].HoldEnabled = mb_rtu.Coil(MB_SCH_SEG_HOLD_EN);
  t_ch0_cal_low = mb_rtu.Coil(MB_CMD_CAL_CH0_LOW);
  t_ch1_cal_low = mb_rtu.Coil(MB_CMD_CAL_CH1_LOW);
  t_ch0_cal_high = mb_rtu.Coil(MB_CMD_CAL_CH0_HIGH);
  t_ch1_cal_high = mb_rtu.Coil(MB_CMD_CAL_CH1_HIGH);
  /* holding registers (RW) */
  Mode = mb_rtu.Hreg(MB_MODE);
  ui_SelectedSchedule = mb_rtu.Hreg(MB_CMD_SELECTED_SCHEDULE);
  ui_Setpoint = HregToDouble(MB_CMD_SETPOINT);
  Kp_01 = HregToDouble(MB_PID_P_01);
  Ki_01 = HregToDouble(MB_PID_I_01);
  Kd_01 = HregToDouble(MB_PID_D_01);
  Kp_02 = HregToDouble(MB_PID_P_02);
  Ki_02 = HregToDouble(MB_PID_I_02);
  Kd_02 = HregToDouble(MB_PID_D_02);
  
  int a = 0;
  for (int i=0; i<sizeof(Schedules[ui_ChangeSelectedSchedule].Name);i=i+2) {
    charAsUnit16 temp;
    temp.reg = mb_rtu.Hreg(MB_SCH_NAME + a);
    Schedules[ui_ChangeSelectedSchedule].Name[i] = temp.c[0];
    if (i<sizeof(Schedules[ui_ChangeSelectedSchedule].Name)) { // fix struct overwrite issue when MAX_STRING_LENGTH is odd
      Schedules[ui_ChangeSelectedSchedule].Name[i + 1] = temp.c[1];
    }
    a++;
  }
  Schedules[ui_ChangeSelectedSchedule].Name[MAX_STRING_LENGTH - 1] = {'\0'}; // make sure terminator is still here!
  
  int b = 0;
  for (int i=0; i<sizeof(Schedules[ui_ChangeSelectedSchedule].Segments[ui_ChangeSelectedSegment].Name);i=i+2) {
    charAsUnit16 temp;
    temp.reg = mb_rtu.Hreg(MB_SCH_SEG_NAME + b);
    Schedules[ui_ChangeSelectedSchedule].Segments[ui_ChangeSelectedSegment].Name[i] = temp.c[0];
    if (i+1<sizeof(Schedules[ui_ChangeSelectedSchedule].Segments[ui_ChangeSelectedSegment].Name)) { // fix struct overwrite issue when MAX_STRING_LENGTH is odd
      Schedules[ui_ChangeSelectedSchedule].Segments[ui_ChangeSelectedSegment].Name[i + 1] = temp.c[1];
    }
    b++;
  }
  Schedules[ui_ChangeSelectedSchedule].Segments[ui_ChangeSelectedSegment].Name[MAX_STRING_LENGTH - 1] = {'\0'}; // make sure terminator is still here!
  
  Schedules[ui_ChangeSelectedSchedule].Segments[ui_ChangeSelectedSegment].Setpoint = HregToDouble(MB_SCH_SEG_SETPOINT);
  Schedules[ui_ChangeSelectedSchedule].Segments[ui_ChangeSelectedSegment].RampRate = mb_rtu.Hreg(MB_SCH_SEG_RAMP_RATE);
  Schedules[ui_ChangeSelectedSchedule].Segments[ui_ChangeSelectedSegment].SoakTime = mb_rtu.Hreg(MB_SCH_SEG_SOAK_TIME);
  ui_ChangeSelectedSegment = mb_rtu.Hreg(MB_SCH_SEG_SELECTED);
  ui_ChangeSelectedSchedule = mb_rtu.Hreg(MB_SCH_SELECTED);
  t_ch0_actual = HregToDouble(MB_CAL_TEMP_ACT_CH0);
  t_ch1_actual = HregToDouble(MB_CAL_TEMP_ACT_CH1);

  /*************** should create a new routine for things below this line ****************/
  // do not let user change modes while running
  if (Mode != Mode_Last) {
    if (ProfileSequence != SEGMENT_STATE_IDLE) {
      Mode = Mode_Last;
    } else {
      Mode_Last = Mode;
    }
  }

  // could use callback here....
  if (ui_WriteEeprom) {
    ui_WriteEeprom = false;
    writeSettingsToEeeprom();
    ui_EepromWritten = true;
    ui_SelectSchedule = true; // loaded schedule may have changed - go ahead and reload it
  }
  // reset the eeprom saved indicator on the hmi
  if (millis() - EepromWritten_Timer > 3000 || !ui_EepromWritten) {
    EepromWritten_Timer = millis();
    if (ui_EepromWritten) ui_EepromWritten = false;
  }
}
void setupModbus() {
  //Serial.begin(SERIAL_BAUD_RATE, SERIAL_8N1);
  mb_ip.server();   //Start Modbus IP
  mb_rtu.begin(&Serial);
  mb_rtu.slave(SLAVE_ID);
  /* coils (RW) */
  mb_rtu.addCoil(MB_CMD_SELECT_SCHEDULE);
  mb_rtu.addCoil(MB_CMD_START_PROFILE);
  mb_rtu.addCoil(MB_CMD_STOP_PROFILE);
  mb_rtu.addCoil(MB_CMD_HOLD_RELEASE);
  mb_rtu.addCoil(MB_CMD_THERM_OVERRIDE);
  mb_rtu.addCoil(MB_CMD_WRITE_EEPROM);
  mb_rtu.addCoil(MB_SCH_SEG_ENABLED);
  mb_rtu.addCoil(MB_SCH_SEG_HOLD_EN);
  mb_rtu.addCoil(MB_CMD_CAL_CH0_LOW);
  mb_rtu.addCoil(MB_CMD_CAL_CH1_LOW);
  mb_rtu.addCoil(MB_CMD_CAL_CH0_HIGH);
  mb_rtu.addCoil(MB_CMD_CAL_CH1_HIGH);
  /* input status (R) */
  mb_rtu.addIsts(MB_STS_SSR_01);
  mb_rtu.addIsts(MB_STS_SSR_02);
  mb_rtu.addIsts(MB_STS_RELEASE_REQ);
  mb_rtu.addIsts(MB_STS_SAFETY_OK);
  mb_rtu.addIsts(MB_STS_IN_PROCESS);
  mb_rtu.addIsts(MB_STS_THERMAL_RUNAWAY);
  mb_rtu.addIsts(MB_STS_EEPROM_WRITTEN);
  /* holding registers (RW) */
  mb_rtu.addHreg(MB_MODE,0,1);
  mb_rtu.addHreg(MB_CMD_SELECTED_SCHEDULE,0,1);
  mb_rtu.addHreg(MB_CMD_SETPOINT,0,2);
  mb_rtu.addHreg(MB_PID_P_01,Kp_01,2);
  mb_rtu.addHreg(MB_PID_I_01,Ki_01,2);
  mb_rtu.addHreg(MB_PID_D_01,Kd_01,2);
  mb_rtu.addHreg(MB_PID_P_02,Kp_02,2);
  mb_rtu.addHreg(MB_PID_I_02,Ki_02,2);
  mb_rtu.addHreg(MB_PID_D_02,Kd_02,2);
  mb_rtu.addHreg(MB_SCH_NAME,0,8);
  mb_rtu.addHreg(MB_SCH_SEG_NAME,0,8);
  mb_rtu.addHreg(MB_SCH_SEG_SETPOINT,0,2);
  mb_rtu.addHreg(MB_SCH_SEG_RAMP_RATE,0,1);
  mb_rtu.addHreg(MB_SCH_SEG_SOAK_TIME,0,1);
  mb_rtu.addHreg(MB_SCH_SEG_SELECTED,0,1);
  mb_rtu.addHreg(MB_SCH_SELECTED,1,1);
  mb_rtu.addHreg(MB_CAL_TEMP_ACT_CH0,0,2);
  mb_rtu.addHreg(MB_CAL_TEMP_ACT_CH1,0,2);
  /* input registers (R) */
  mb_rtu.addIreg(MB_HEARTBEAT,0,1);
  mb_rtu.addIreg(MB_STS_REMAINING_TIME_H,0,1);
  mb_rtu.addIreg(MB_STS_REMAINING_TIME_M,0,1);
  mb_rtu.addIreg(MB_STS_REMAINING_TIME_S,0,1);
  mb_rtu.addIreg(MB_STS_TEMPERATURE_01,1,2);
  mb_rtu.addIreg(MB_STS_TEMPERATURE_02,2,2);
  mb_rtu.addIreg(MB_STS_PID_01_OUTPUT,3,2);
  mb_rtu.addIreg(MB_STS_PID_02_OUTPUT,3,2);
  mb_rtu.addIreg(MB_NUMBER_OF_SCHEDULES,NUMBER_OF_SCHEDULES,1);
  mb_rtu.addIreg(MB_NUMBER_OF_SEGMENTS,NUMBER_OF_SEGMENTS,1);
  mb_rtu.addIreg(MB_STS_SEGMENT_STATE,0,1);
  mb_rtu.addIreg(MB_STS_SEGMENT_NAME,0,8);
  mb_rtu.addIreg(MB_STS_SCHEDULE_NAME,0,8);
  mb_rtu.addIreg(MB_STS_TEMP_01_RAW,0,2);
  mb_rtu.addIreg(MB_STS_TEMP_02_RAW,0,2);
  mb_rtu.addIreg(MB_STS_RUNAWAY_TEMP_T,0,2);
  mb_rtu.addIreg(MB_STS_RUNAWAY_RATE_T,0,2);
  mb_rtu.addIreg(MB_STS_MEAS_RATE_CH0,0,2);
  mb_rtu.addIreg(MB_STS_MEAS_RATE_CH1,0,2);
  
  //mb_rtu.onGetCoil(COIL_BASE, cbRead, LEN); // Add callback on Coils value get
  //mb_rtu.onSetCoil(MB_CMD_SELECT_SCHEDULE, cbSchedule);      // Add callback on Coil LED_COIL value set
  //mb_rtu.onSetCoil(MB_CMD_WRITE_EEPROM, cbWriteEeprom);
  
  /* add multiple
  bool addHreg(uint16_t offset, uint16_t value = 0, uint16_t numregs = 1);
  bool addCoil(uint16_t offset, bool value = false, uint16_t numregs = 1);
  bool addIsts(uint16_t offset, bool value = false, uint16_t numregs = 1);
  bool addIreg(uint16_t offset, uint16_t value = 0, uint16_t nemregs = 1);
   */
  /* write local
  bool Hreg(uint16_t offset, uint16_t value);
  bool Coil(uint16_t offset, bool value);
  bool Ists(uint16_t offset, bool value);
  bool Ireg(uint16_t offset, uint16_t value);
   */
  /* read local
  uint16_t Hreg(uint16_t offset);
  bool Coil(uint16_t offset);
  bool Ists(uint16_t offset);
  uint16_t Ireg(uint16_t offset);
   */
  /* remove
  bool removeHreg(uint16_t offset, uint16_t numregs = 1);
  bool removeCoil(uint16_t offset, uint16_t numregs = 1);
  bool removeIsts(uint16_t offset, uint16_t numregs = 1);
  bool removeIreg(uint16_t offset, uint16_t numregs = 1);
   */
  /* callbacks
  bool onSetCoil(uint16_t address, cbModbus cb = nullptr, uint16_t numregs = 1);
  bool onSetHreg(uint16_t address, cbModbus cb = nullptr, uint16_t numregs = 1);
  bool onSetIsts(uint16_t address, cbModbus cb = nullptr, uint16_t numregs = 1);
  bool onSetIreg(uint16_t address, cbModbus cb = nullptr, uint16_t numregs = 1);

  bool onGetCoil(uint16_t address, cbModbus cb = nullptr, uint16_t numregs = 1);
  bool onGetHreg(uint16_t address, cbModbus cb = nullptr, uint16_t numregs = 1);
  bool onGetIsts(uint16_t address, cbModbus cb = nullptr, uint16_t numregs = 1);
  bool onGetIreg(uint16_t address, cbModbus cb = nullptr, uint16_t numregs = 1);
   */
  /* get value
  uint16_t Hreg(uint16_t offset);
  bool Coil(uint16_t offset);
  bool Ists(uint16_t offset);
  uint16_t Ireg(uint16_t offset);
  */
  
}

/* eeprom */
#include <EEPROM.h>
#define EEPROM_SCH_START_ADDR     100
#define EEPROM_SIZE               3000 // can be between 4 and 4096 -schedules take up around 1515 bytes when MAX_STRING_LENGTH=16, NUMBER_OF_SCHEDULES=5, and NUMBER_OF_SEGMENTS=10
void writeSettingsToEeeprom() {
  //Serial.println(F("Writing Schedule to EEPROM..."));
  int address = EEPROM_SCH_START_ADDR;

  for (int i=0; i<NUMBER_OF_SCHEDULES; i++) {
    /* cmd select */
    EEPROM.put(address, Schedules[i].CMD_Select);
    address = address + sizeof(Schedules[i].CMD_Select);
    /* sts select */
    EEPROM.put(address, Schedules[i].STS_Select);
    address = address + sizeof(Schedules[i].STS_Select);
    /* schedule name */
    for (int j=0; j<sizeof(Schedules[i].Name); j++) {
      EEPROM.put(address, Schedules[i].Name[j]);
      address = address + sizeof(Schedules[i].Name[j]);
    }
    Schedules[i].Name[MAX_STRING_LENGTH - 1] = '\0'; // make sure terminator is still there!
    for (int k=0; k<NUMBER_OF_SEGMENTS; k++) {
      /* segment name */
      for (int x=0; x<sizeof(Schedules[i].Segments[k].Name); x++) {
        EEPROM.put(address, Schedules[i].Segments[k].Name[x]);
        address = address + sizeof(Schedules[i].Segments[k].Name[x]);
      }
      Schedules[i].Segments[k].Name[MAX_STRING_LENGTH - 1] = '\0'; // make sure terminator is still there!
      /* segment enabled */
      EEPROM.put(address, Schedules[i].Segments[k].Enabled);
      address = address + sizeof(Schedules[i].Segments[k].Enabled);
      /* hold enabled */
      EEPROM.put(address, Schedules[i].Segments[k].HoldEnabled);
      address = address + sizeof(Schedules[i].Segments[k].HoldEnabled);
      /* setpoint */
      EEPROM.put(address, Schedules[i].Segments[k].Setpoint);
      address = address + sizeof(Schedules[i].Segments[k].Setpoint);
      /* ramp rate */
      EEPROM.put(address, Schedules[i].Segments[k].RampRate);
      address = address + sizeof(Schedules[i].Segments[k].RampRate);
      /* soak time */
      EEPROM.put(address, Schedules[i].Segments[k].SoakTime);
      address = address + sizeof(Schedules[i].Segments[k].SoakTime);
      /* state */
      EEPROM.put(address, Schedules[i].Segments[k].State);
      address = address + sizeof(Schedules[i].Segments[k].State);
    }
  }

  // PID settings
  /* upper */
  EEPROM.put(address, Kp_01);
  address = address + sizeof(Kp_01);
  EEPROM.put(address, Ki_01);
  address = address + sizeof(Ki_01);
  EEPROM.put(address, Kd_01);
  address = address + sizeof(Kd_01);
  /* lower */
  EEPROM.put(address, Kp_02);
  address = address + sizeof(Kp_02);
  EEPROM.put(address, Ki_02);
  address = address + sizeof(Ki_02);
  EEPROM.put(address, Kd_02);
  address = address + sizeof(Kd_02);

  // calibration settings
  /* upper */
  EEPROM.put(address, t_ch0_fromLow);
  address = address + sizeof(t_ch0_fromLow);
  EEPROM.put(address, t_ch0_fromHigh);
  address = address + sizeof(t_ch0_fromHigh);
  EEPROM.put(address, t_ch0_toLow);
  address = address + sizeof(t_ch0_toLow);
  EEPROM.put(address, t_ch0_toHigh);
  address = address + sizeof(t_ch0_toHigh);
  /* lower */
  EEPROM.put(address, t_ch1_fromLow);
  address = address + sizeof(t_ch1_fromLow);
  EEPROM.put(address, t_ch1_fromHigh);
  address = address + sizeof(t_ch1_fromHigh);
  EEPROM.put(address, t_ch1_toLow);
  address = address + sizeof(t_ch1_toLow);
  EEPROM.put(address, t_ch1_toHigh);
  address = address + sizeof(t_ch1_toHigh);

  /* commit to simulated eeprom (flash) */
  EEPROM.commit();
  //EEPROM.end(); // will also commit, but will release the RAM copy of EEPROM contents
}
void readSettingsFromEeeprom() {
  //Serial.println(F("Reading Schedule from EEPROM..."));
  int address = EEPROM_SCH_START_ADDR;

  for (int i=0; i<NUMBER_OF_SCHEDULES; i++) {
    /* cmd select */
    EEPROM.get(address, Schedules[i].CMD_Select);
    address = address + sizeof(Schedules[i].CMD_Select);
    /* sts select */
    EEPROM.get(address, Schedules[i].STS_Select);
    address = address + sizeof(Schedules[i].STS_Select);
    /* schedule name */
    for (int j=0; j<sizeof(Schedules[i].Name); j++) {
      EEPROM.get(address, Schedules[i].Name[j]);
      address = address + sizeof(Schedules[i].Name[j]);
    }
    for (int k=0; k<NUMBER_OF_SEGMENTS; k++) {
      /* segment name */
      for (int x=0; x<sizeof(Schedules[i].Segments[k].Name); x++) {
        EEPROM.get(address, Schedules[i].Segments[k].Name[x]);
        address = address + sizeof(Schedules[i].Segments[k].Name[x]);
      }
      /* segment enabled */
      EEPROM.get(address, Schedules[i].Segments[k].Enabled);
      address = address + sizeof(Schedules[i].Segments[k].Enabled);
      /* hold enabled */
      EEPROM.get(address, Schedules[i].Segments[k].HoldEnabled);
      address = address + sizeof(Schedules[i].Segments[k].HoldEnabled);
      /* setpoint */
      EEPROM.get(address, Schedules[i].Segments[k].Setpoint);
      address = address + sizeof(Schedules[i].Segments[k].Setpoint);
      /* ramp rate */
      EEPROM.get(address, Schedules[i].Segments[k].RampRate);
      address = address + sizeof(Schedules[i].Segments[k].RampRate);
      /* soak time */
      EEPROM.get(address, Schedules[i].Segments[k].SoakTime);
      address = address + sizeof(Schedules[i].Segments[k].SoakTime);
      /* state */
      EEPROM.get(address, Schedules[i].Segments[k].State);
      address = address + sizeof(Schedules[i].Segments[k].State);
    }
  }

  // PID settings
  /* upper */
  EEPROM.get(address, Kp_01);
  address = address + sizeof(Kp_01);
  EEPROM.get(address, Ki_01);
  address = address + sizeof(Ki_01);
  EEPROM.get(address, Kd_01);
  address = address + sizeof(Kd_01);
  /* lower */
  EEPROM.get(address, Kp_02);
  address = address + sizeof(Kp_02);
  EEPROM.get(address, Ki_02);
  address = address + sizeof(Ki_02);
  EEPROM.get(address, Kd_02);
  address = address + sizeof(Kd_02);

  // calibration settings
  /* upper */
  EEPROM.get(address, t_ch0_fromLow);
  address = address + sizeof(t_ch0_fromLow);
  EEPROM.get(address, t_ch0_fromHigh);
  address = address + sizeof(t_ch0_fromHigh);
  EEPROM.get(address, t_ch0_toLow);
  address = address + sizeof(t_ch0_toLow);
  EEPROM.get(address, t_ch0_toHigh);
  address = address + sizeof(t_ch0_toHigh);
  /* lower */
  EEPROM.get(address, t_ch1_fromLow);
  address = address + sizeof(t_ch1_fromLow);
  EEPROM.get(address, t_ch1_fromHigh);
  address = address + sizeof(t_ch1_fromHigh);
  EEPROM.get(address, t_ch1_toLow);
  address = address + sizeof(t_ch1_toLow);
  EEPROM.get(address, t_ch1_toHigh);
  address = address + sizeof(t_ch1_toHigh);

}

/* initialization */
const char version[] = "build "  __DATE__ " " __TIME__; 
const char Initialized[] = {"Initialized02"};
void checkInit(){
  Serial.println();
  Serial.println(F("Checking initialization..."));
  bool writeDefaults = false;
  for (byte i=0;i<sizeof(Initialized)-1;i++){
    if (Initialized[i] != EEPROM.read(i)) {
      writeDefaults = true;
      i = sizeof(Initialized); // get out
    }
  }
  /*for (byte i=0;i<sizeof(version)-1;i++){
    if (version[i] != EEPROM.read(i)) {
      writeDefaults = true;
      i = sizeof(version); // get out
    }
  }*/
  if (writeDefaults) {
    applyDefaultSettings();
    makeInitialized();
  }
}
void makeInitialized(){
  Serial.println(F("Initializing board..."));
  for (byte i=0;i<sizeof(Initialized)-1;i++){
    EEPROM.write(i, Initialized[i]);
  }
  /*for (byte i=0;i<sizeof(version)-1;i++){
    EEPROM.write(i, version[i]);
  }*/

  EEPROM.commit();

  // To format all space in LittleFS
  LittleFS.format();
}
void applyDefaultSettings() {
  Serial.println(F("Applying default  settings..."));
  char strEmpty[MAX_STRING_LENGTH] = {'\0'};
  char strSchedule[] = {"Schedule"};
  char strSegment[] = {"Segment"};
  
  for (int i=0; i<NUMBER_OF_SCHEDULES; i++) {
    /* cmd select */
    Schedules[i].CMD_Select = false;
    /* sts select */
    Schedules[i].STS_Select = false;
    /* schedule name */
    for (int j=0; j<sizeof(Schedules[i].Name); j++) {
     Schedules[i].Name[j] = strEmpty[j];
    }
    for (int a=0; a<sizeof(strSchedule) && a<MAX_STRING_LENGTH; a++) {
     Schedules[i].Name[a] = strSchedule[a];
    }
    for (int k=0; k<NUMBER_OF_SEGMENTS; k++) {
      /* segment name */
      for (int x=0; x<sizeof(Schedules[i].Segments[k].Name); x++) {
        Schedules[i].Segments[k].Name[x] = strEmpty[x];
      }
      for (int e=0; e<sizeof(strSegment) && e<MAX_STRING_LENGTH; e++) {
       Schedules[i].Segments[k].Name[e] = strSegment[e];
      }
      /* segment enabled */
      Schedules[i].Segments[k].Enabled = true;
      /* hold enabled */
      Schedules[i].Segments[k].HoldEnabled = false;
      /* setpoint */
      Schedules[i].Segments[k].Setpoint = 100.0;
      /* ramp rate */
      Schedules[i].Segments[k].RampRate = 200;
      /* soak time */
      Schedules[i].Segments[k].SoakTime = 1;
      /* state */
      Schedules[i].Segments[k].State = SEGMENT_STATE_IDLE;
    }
  }

  // PID settings
  /* upper */
  Kp_01 = 0.01;
  Ki_01 = 0.01;
  Kd_01 = 0.0;
  /* lower */
  Kp_02 = 0.01;
  Ki_02 = 0.01;
  Kd_02 = 0.0;
  
  // calibration settings
  /* upper */
  t_ch0_fromLow = 1.0, t_ch0_fromHigh = 2000.0, t_ch0_toLow = 1.0, t_ch0_toHigh = 2000.0;
  /* lower */
  t_ch1_fromLow = 1.0, t_ch1_fromHigh = 2000.0, t_ch1_toLow = 1.0, t_ch1_toHigh = 2000.0;

  writeSettingsToEeeprom();
}

/* setup */
unsigned long timer_heartbeat;
void setup() {
      
  DEVICE_ID += String(ESP.getChipId(), HEX);

  Serial.begin(SERIAL_BAUD_RATE, SERIAL_8N1);
  delay(500); // give the serial port time to start up

  EEPROM.begin(EEPROM_SIZE);

  //checkInit();
  readSettingsFromEeeprom();
  initLittleFS();
  setupPins();
  timer_heartbeat = millis();
  setupPID();
  setupThermocouples();
  setupWifiManager();
  setupOTA();
  setupModbus();

  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

/* loop */
bool HeartbeatOn = false;
void loop() {

  //
  // handle logic
  //
  //checkWifi();
  handleSafetyCircuit();
  handleCal();
  handleTemperature();
  handlePID();
  setSchedule();
  handleProfileSequence();
  handleThermalRunaway();
  handleModbus();
  handleMainContactor();

  //
  // handle heartbeat
  //
  if (millis() - timer_heartbeat > HEARTBEAT_TIME ){
    timer_heartbeat = millis();
    if (HEARTBEAT_VALUE>=100) {
      HEARTBEAT_VALUE = 0;
    } else {
      HEARTBEAT_VALUE = HEARTBEAT_VALUE + 1;
    }
    if (HeartbeatOn) {
      HeartbeatOn = false;
      //Serial.println("Heartbeat Off");
    } else {
      HeartbeatOn = true;
      //Serial.println("Heartbeat On");
    }
  }
  
  //
  // handle OTA requests
  //
  ArduinoOTA.handle();
  
  yield();
}