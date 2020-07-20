/*********
  Board settings:
    Board: NodeMCU 1.0 (ESP-12E Module)
    Flash Size: 4M (FS:1MBOTA:~1019KB)
    Debug Port: Disabled
    Debug Level: None
    IwIP Variant: v2 Lower Memory
    VTables: Flash
    CPU Frequency: 160MHz
    Upload Speed: 115200¿
    Erase Flash: All Flash Contents
    Builtin LED: 2
*********/

// NOTE: ****** kiln was flashed with ESP Board version 2.7.2

// sketch will write default settings if new build
//const char version[] = "build "  __DATE__ " " __TIME__; 
const char version[] = __DATE__ " " __TIME__; 
const char Initialized[] = {"Initialized02"};

#if !defined(ARRAY_SIZE)
    #define ARRAY_SIZE(x) (sizeof((x)) / sizeof((x)[0]))
#endif

#include <ESP8266WiFi.h>
//#include <ESP8266mDNS.h>
//#include <WiFiUdp.h>
#include <ArduinoOTA.h>
//#define OUTPUT_BUFFER_SIZE 5000 // for aRest
//#define DEBUG_MODE 1 // for aRest
//#include <aREST.h>
#include "LittleFS.h"
#include <EEPROM.h>

//
// modbus
//
#define COIL_VAL(v)   // get coil value
#define COIL_BOOL(v)  // assign coil value
#define ISTS_VAL(v)   // get status value
#define ISTS_BOOL(v)  // assign status value
#include <ModbusRTU.h> // https://github.com/emelianov/modbus-esp8266
#include <ModbusIP_ESP8266.h>
#define SLAVE_ID                  1
/* coils (RW) */
#define MB_CMD_SELECT_SCHEDULE    1
#define MB_CMD_START_PROFILE      2
#define MB_CMD_STOP_PROFILE       3
#define MB_CMD_HOLD_RELEASE       4
#define MB_CMD_THERM_OVERRIDE     5
#define MB_CMD_WRITE_EEPROM       50
#define MB_SCH_SEG_ENABLED        100
#define MB_SCH_SEG_HOLD_EN        101
/* input status (R) */
#define MB_STS_SSR_01             1
#define MB_STS_SSR_02             2
#define MB_STS_RELEASE_REQ        3
#define MB_STS_SAFETY_OK          4
#define MB_STS_IN_PROCESS         5
#define MB_STS_THERMAL_RUNAWAY    6
#define MB_STS_EEPROM_WRITTEN     50
/* holding registers (RW) 16 bit*/
#define MB_MODE                   1
#define MB_CMD_SELECTED_SCHEDULE  2
#define MB_CMD_SETPOINT           3
#define MB_PID_P_01               5
#define MB_PID_I_01               7
#define MB_PID_D_01               9
#define MB_PID_P_02               11
#define MB_PID_I_02               13
#define MB_PID_D_02               15
#define MB_SCH_NAME               100
#define MB_SCH_SEG_NAME           108
#define MB_SCH_SEG_SETPOINT       116
#define MB_SCH_SEG_RAMP_RATE      118
#define MB_SCH_SEG_SOAK_TIME      119
#define MB_SCH_SEG_SELECTED       120
#define MB_SCH_SELECTED           121
/* input registers (R) 16 bit*/
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
/* instance */
ModbusRTU mb_rtu;
ModbusIP mb_ip;
int HEARTBEAT_VALUE = 0;
bool ui_EepromWritten = false;
/*
// Callback function to read corresponding DI
uint16_t cbRead(TRegister* reg, uint16_t val) {
  if(reg->address.address < COIL_BASE)
    return 0;
  uint8_t offset = reg->address.address - COIL_BASE;
  if(offset >= LEN)
    return 0; 
  return COIL_VAL(digitalRead(pinList[offset]));
}
// Callback function to write-protect DI
uint16_t cbWrite(TRegister* reg, uint16_t val) {
  return reg->value;
}*/

uint16_t cbWriteEeprom(TRegister* reg, uint16_t val) {
  writeScheduleToEeeprom();
  ui_EepromWritten = true;
  return reg->value;
}

//
// pid
//
#include <PID_v1.h>
//Define Variables we'll be connecting to
double Setpoint_ch0, Setpoint_ch1, Input_01, Input_02, Output_01, Output_02;
//Specify the links and initial tuning parameters
double Kp_01=2.0, Ki_01=1.0, Kd_01=0.0;
double Kp_02=2.0, Ki_02=1.0, Kd_02=0.0;
PID PID_01(&Input_01, &Output_01, &Setpoint_ch0, Kp_01, Ki_01, Kd_01, DIRECT); // REVERSE - PROCESS lowers as OUTPUT rises
PID PID_02(&Input_02, &Output_02, &Setpoint_ch1, Kp_02, Ki_02, Kd_02, DIRECT); 
int WindowSize = 1000;
unsigned long windowStartTime_01, windowStartTime_02;

//
// temperature sensors
//
#include <SPI.h>
#include "Adafruit_MAX31855.h"
#define MAXCS_CH0   D8 // D8 is GPIO15 SPI (CS) on NodeMCU
Adafruit_MAX31855 thermocouple_ch0(MAXCS_CH0);
#define MAXCS_CH1   D7 // D3 is GPIO0 connected to FLASH button on NodeMCU
Adafruit_MAX31855 thermocouple_ch1(MAXCS_CH1);

//
// defines
//
#define WIFI_SSID                 "Thomas_301"
#define WIFI_PASSWORD             "RS232@12"
#define WIFI_LISTENING_PORT       80
#define SERIAL_BAUD_RATE          115200
#define OTA_PASSWORD              "ProFiBus@12"
#define OTA_HOSTNAME              "KilnControls" // having this too long was causing problems with aRest
#define HEARTBEAT_TIME            1000

#define AUTOMATIC_MODE            1
#define MANUAL_MODE               2
#define SIMULATE_MODE             3

#define SAFETY_CIRCUIT_INPUT      D0
#define MAIN_CONTACTOR_OUTPUT     D4

#define EEPROM_SCH_START_ADDR     100

//
// init variables
//
const int ESP_BUILTIN_LED = LED_BUILTIN;
//const int ANALOG_PIN = A0; // causing issues - see below
const int SSR_PIN_01 = D1; // D1 D2 D5 D6 D7 (these are recommended pins)
const int SSR_PIN_02 = D2;
unsigned long timer_heartbeat;
bool Safety_Ok = false;
int SafetyInputLast = 0;

//
// init wifi
//
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

//
// profile sequence
//
#define SEGMENT_STATE_IDLE    0
#define SEGMENT_STATE_RAMP    1
#define SEGMENT_STATE_SOAK    2
#define SEGMENT_STATE_HOLD    3
#define SEGMENT_STATE_INIT    4
#define SEGMENT_STATE_START   5
#define NUMBER_OF_SCHEDULES   5
#define NUMBER_OF_SEGMENTS    10
#define RATE_TIMER_PERIOD     1000
#define RAMP_TIMER_PERIOD     1000
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
double temperature_ch0, temperature_ch1;
double temperatureLast_ch0 = 0.0, temperatureLast_ch1 = 0.0;
double MeasuredRatePerHour_ch0, MeasuredRatePerHour_ch1;
/* thermal runaway */
bool ThermalRunawayDetected = false, ui_ThermalRunawayOverride = false;
/* user interface */
uint16_t Mode = AUTOMATIC_MODE;
bool ui_StartProfile = false;
bool ui_StopProfile = false;
bool ui_Segment_HoldReleaseRequest = false;
bool ui_Segment_HoldRelease = false;
bool ui_SelectSchedule = false;
uint16_t ui_SelectedSchedule = 0, ui_SelectedScheduleLast = 0;
double ui_Setpoint = 0.0;
bool ui_StsSSRPin_01, ui_StsSSRPin_02;
uint16_t ui_ChangeSelectedSchedule = 1, ui_ChangeSelectedSegment = 0;

/*
uint16_t cbSchedule(TRegister* reg, uint16_t val) {
  ui_SelectSchedule = COIL_BOOL(val);
  return val;
}
*/

struct TIME {
  uint16_t hours;
  uint16_t minutes;
  uint16_t seconds;
  uint16_t milliseconds;
} Segment_TimeRemaining;
      
struct SCHEDULE_SEGMENT {
  char Name[15] = {'\0'};
  bool Enabled;
  bool HoldEnabled;
  double Setpoint;    // in degrees(C)
  uint16_t RampRate;  // in degrees(C)/hour
  uint16_t SoakTime;  // in minutes
  uint8_t State;
};

struct SCHEDULE {
  char Name[15] = {'\0'};
  bool CMD_Select;
  bool STS_Select;
  SCHEDULE_SEGMENT Segments[NUMBER_OF_SEGMENTS];
} Schedules[NUMBER_OF_SCHEDULES];

union floatAsBytes {
  float fval;
  uint16_t ui[2];
  byte bval[4];
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

unsigned long EepromWritten_Timer = millis();

void handleModbus() {
  if (ui_SelectedSchedule >= NUMBER_OF_SCHEDULES) ui_SelectedSchedule = NUMBER_OF_SCHEDULES - 1;
  if (SegmentIndex >= NUMBER_OF_SEGMENTS) SegmentIndex = NUMBER_OF_SEGMENTS - 1;
  if (ui_ChangeSelectedSchedule >= NUMBER_OF_SCHEDULES) ui_ChangeSelectedSchedule = NUMBER_OF_SCHEDULES - 1;
  if (ui_ChangeSelectedSchedule >= NUMBER_OF_SEGMENTS) ui_ChangeSelectedSegment = NUMBER_OF_SEGMENTS - 1;
  /* coils (RW) */
  mb_rtu.Coil(MB_CMD_SELECT_SCHEDULE, ui_SelectSchedule);
  mb_rtu.Coil(MB_CMD_START_PROFILE, ui_StartProfile);
  mb_rtu.Coil(MB_CMD_STOP_PROFILE, ui_StopProfile);
  mb_rtu.Coil(MB_CMD_HOLD_RELEASE, ui_Segment_HoldRelease);
  mb_rtu.Coil(MB_CMD_THERM_OVERRIDE, ui_ThermalRunawayOverride);
  //mb_rtu.Coil(MB_CMD_WRITE_EEPROM, ???);
  mb_rtu.Coil(MB_SCH_SEG_ENABLED, Schedules[ui_ChangeSelectedSchedule].Segments[ui_ChangeSelectedSegment].Enabled);
  mb_rtu.Coil(MB_SCH_SEG_HOLD_EN, Schedules[ui_ChangeSelectedSchedule].Segments[ui_ChangeSelectedSegment].HoldEnabled);
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
  int y = 0;
  for (int i=0; i<sizeof(Schedules[ui_ChangeSelectedSchedule].Name);i=i+2) {
    charAsUnit16 temp;
    temp.c[0] = Schedules[ui_ChangeSelectedSchedule].Name[i];
    temp.c[1] = Schedules[ui_ChangeSelectedSchedule].Name[i + 1];
    mb_rtu.Hreg(MB_SCH_NAME + y, temp.reg);
    y++;
  }
  int x = 0;
  for (int i=0; i<sizeof(Schedules[ui_ChangeSelectedSchedule].Segments[ui_ChangeSelectedSegment].Name);i=i+2) {
    charAsUnit16 temp;
    temp.c[0] = Schedules[ui_ChangeSelectedSchedule].Segments[ui_ChangeSelectedSegment].Name[i];
    temp.c[1] = Schedules[ui_ChangeSelectedSchedule].Segments[ui_ChangeSelectedSegment].Name[i + 1];
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
  mb_rtu.Ireg(MB_STS_SEGMENT_STATE, Schedules[0].Segments[SegmentIndex].State );
  int j = 0;
  for (int i=0; i<sizeof(Schedules[ui_SelectedSchedule].Segments[SegmentIndex].Name);i=i+2) {
    charAsUnit16 temp;
    temp.c[0] = Schedules[ui_SelectedSchedule].Segments[SegmentIndex].Name[i];
    temp.c[1] = Schedules[ui_SelectedSchedule].Segments[SegmentIndex].Name[i + 1];
    mb_rtu.Ireg(MB_STS_SEGMENT_NAME + j, temp.reg);
    j++;
  }
  int k = 0;
  for (int i=0; i<sizeof(Schedules[ui_SelectedSchedule].Name);i=i+2) {
    charAsUnit16 temp;
    temp.c[0] = Schedules[ui_SelectedSchedule].Name[i];
    temp.c[1] = Schedules[ui_SelectedSchedule].Name[i+1];
    mb_rtu.Ireg(MB_STS_SCHEDULE_NAME + k, temp.reg);
    k++;
  }
  
  mb_rtu.task();
  mb_ip.task();
  
  /* coils (RW) */
  ui_SelectSchedule = mb_rtu.Coil(MB_CMD_SELECT_SCHEDULE);
  ui_StartProfile = mb_rtu.Coil(MB_CMD_START_PROFILE);
  ui_StopProfile = mb_rtu.Coil(MB_CMD_STOP_PROFILE);
  ui_Segment_HoldRelease = mb_rtu.Coil(MB_CMD_HOLD_RELEASE);
  ui_ThermalRunawayOverride = mb_rtu.Coil(MB_CMD_THERM_OVERRIDE);
  //????? = mb_rtu.Coil(MB_CMD_WRITE_EEPROM);
  Schedules[ui_ChangeSelectedSchedule].Segments[ui_ChangeSelectedSegment].Enabled = mb_rtu.Coil(MB_SCH_SEG_ENABLED);
  Schedules[ui_ChangeSelectedSchedule].Segments[ui_ChangeSelectedSegment].HoldEnabled = mb_rtu.Coil(MB_SCH_SEG_HOLD_EN);
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
    Schedules[ui_ChangeSelectedSchedule].Name[i + 1] = temp.c[1];
    a++;
  }
  int b = 0;
  for (int i=0; i<sizeof(Schedules[ui_ChangeSelectedSchedule].Segments[ui_ChangeSelectedSegment].Name);i=i+2) {
    charAsUnit16 temp;
    temp.reg = mb_rtu.Hreg(MB_SCH_SEG_NAME + b);
    Schedules[ui_ChangeSelectedSchedule].Segments[ui_ChangeSelectedSegment].Name[i] = temp.c[0];
    Schedules[ui_ChangeSelectedSchedule].Segments[ui_ChangeSelectedSegment].Name[i + 1] = temp.c[1];
    b++;
  }
  Schedules[ui_ChangeSelectedSchedule].Segments[ui_ChangeSelectedSegment].Setpoint = HregToDouble(MB_SCH_SEG_SETPOINT);
  Schedules[ui_ChangeSelectedSchedule].Segments[ui_ChangeSelectedSegment].RampRate = mb_rtu.Hreg(MB_SCH_SEG_RAMP_RATE);
  Schedules[ui_ChangeSelectedSchedule].Segments[ui_ChangeSelectedSegment].SoakTime = mb_rtu.Hreg(MB_SCH_SEG_SOAK_TIME);
  ui_ChangeSelectedSegment = mb_rtu.Hreg(MB_SCH_SEG_SELECTED);
  ui_ChangeSelectedSchedule = mb_rtu.Hreg(MB_SCH_SELECTED);

  // reset the saved indicator on the hmi
  if (millis() - EepromWritten_Timer > 3000 || !ui_EepromWritten) {
    EepromWritten_Timer = millis();
    if (ui_EepromWritten) ui_EepromWritten = false;
  }
}

#define TEMP_AVG_ARR_SIZE 100
int idx_ch0Readings = 0, idx_ch1Readings;
double t_ch0Readings[TEMP_AVG_ARR_SIZE] = {0.0};
double t_ch1Readings[TEMP_AVG_ARR_SIZE] = {0.0};
double t_ch0Tot = 0.0, t_ch1Tot = 0.0;

void handleTemperature() {
  double t_ch0 = map(thermocouple_ch0.readFahrenheit(),78.0,3000.0,75.9,3000.0); //map(value,fromlow,fromhigh,tolow,tohigh);
  if (isnan(t_ch0) || t_ch0 < -32.0 || t_ch0 > 5000.0) {
    //temperature_ch0 = 0.0;
  } else {
    /* smoothing */
    // subtract the last reading:
    t_ch0Tot = t_ch0Tot - t_ch0Readings[idx_ch0Readings];
    // read from the sensor:
    t_ch0Readings[idx_ch0Readings] = t_ch0;
    // add the reading to the total:
    t_ch0Tot = t_ch0Tot + t_ch0Readings[idx_ch0Readings];
    // advance to the next position in the array:
    idx_ch0Readings++;
    // if we're at the end of the array...
    if (idx_ch0Readings >= TEMP_AVG_ARR_SIZE) {
      // ...wrap around to the beginning:
      idx_ch0Readings = 0;
    }
    // calculate the average:
    temperature_ch0 = t_ch0Tot / TEMP_AVG_ARR_SIZE;
  }
  double t_ch1 = map(thermocouple_ch1.readFahrenheit(),89.0,3000.0,75.9,3000.0);
  if (isnan(t_ch1) || t_ch1 < -32.0 || t_ch1 > 5000.0) {
    //temperature_ch1 = 0.0;
  } else {
    /* smoothing */
    // subtract the last reading:
    t_ch1Tot = t_ch1Tot - t_ch1Readings[idx_ch1Readings];
    // read from the sensor:
    t_ch1Readings[idx_ch1Readings] = t_ch1;
    // add the reading to the total:
    t_ch1Tot = t_ch1Tot + t_ch1Readings[idx_ch1Readings];
    // advance to the next position in the array:
    idx_ch1Readings++;
    // if we're at the end of the array...
    if (idx_ch1Readings >= TEMP_AVG_ARR_SIZE) {
      // ...wrap around to the beginning:
      idx_ch1Readings = 0;
    }
    // calculate the average:
    temperature_ch1 = t_ch1Tot / TEMP_AVG_ARR_SIZE;
  }
  if (Mode == SIMULATE_MODE) {
    temperature_ch0 = Setpoint_ch0;
    temperature_ch1 = Setpoint_ch1;
  }
}

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
  if (millis() - windowStartTime_01 > WindowSize)
  { //time to shift the Relay Window
    windowStartTime_01 += WindowSize;
  }
  if (Output_01 < millis() - windowStartTime_01) {
    digitalWrite(SSR_PIN_01, HIGH);
    ui_StsSSRPin_01 = false;
  } else {
      if (Safety_Ok && !ThermalRunawayDetected) {
        digitalWrite(SSR_PIN_01, LOW); // esp has sinking outputs
        ui_StsSSRPin_01 = true;
      }
  }
  
  if (millis() - windowStartTime_02 > WindowSize)
  { //time to shift the Relay Window
    windowStartTime_02 += WindowSize;
  }
  if (Output_02 < millis() - windowStartTime_02) {
    digitalWrite(SSR_PIN_02, HIGH);
    ui_StsSSRPin_02 = false;
  } else {
      if (Safety_Ok && !ThermalRunawayDetected) {
        digitalWrite(SSR_PIN_02, LOW); // esp has sinking outputs
        ui_StsSSRPin_02 = true;
      }
  }
  if (!Safety_Ok || ThermalRunawayDetected) { // turn off sinking outputs
    digitalWrite(SSR_PIN_01, HIGH);
    ui_StsSSRPin_02 = false;
    digitalWrite(SSR_PIN_02, HIGH);
    ui_StsSSRPin_02 = false;
  }
}

void handleSafetyCircuit() {
  SafetyInputLast = digitalRead(SAFETY_CIRCUIT_INPUT);
  if (SafetyInputLast == HIGH) Safety_Ok = true;
  else Safety_Ok = false;
}

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
          (Mode == AUTOMATIC_MODE || Mode == SIMULATE_MODE) && 
          !ThermalRunawayDetected) {
        SegmentIndex = 0;
        ProfileSequence = SEGMENT_STATE_INIT;
      }
      ui_StartProfile = false;
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
      if (Schedules[0].Segments[SegmentIndex].Enabled) {
        Segment_CheckDirection          = true;
        Segment_WillIncrement_Ch0       = false;
        Segment_WillIncrement_Ch1       = false;
        Segment_AtTemp_Ch0              = false;
        Segment_AtTemp_Ch1              = false;
        ui_Segment_HoldReleaseRequest   = false;
        ui_Segment_HoldRelease          = false;
        ProfileSequence = SEGMENT_STATE_RAMP;
      } else {
        if (SegmentIndex < NUMBER_OF_SEGMENTS) {
          SegmentIndex++;
        } else {
          SegmentIndex = 0;
          ProfileSequence = SEGMENT_STATE_IDLE;
        }
      }
      break;
      
    case SEGMENT_STATE_RAMP: /* first segment ramp to temp */
      RampTimerEnabled = true;
      break;
      
    case SEGMENT_STATE_SOAK: /* first segment soak at temp */
      SOAK_TIMER_PERIOD = Schedules[0].Segments[SegmentIndex].SoakTime;
      SoakTimerEnabled = true;
      break;
      
    default:
      ProfileSequence = SEGMENT_STATE_IDLE;
      break;
  }
  
  Schedules[0].Segments[SegmentIndex].State = ProfileSequence;
  
  // 
  // check the direction the temp needs to go to get to setpoint
  //
  if (Segment_CheckDirection) {
    Segment_CheckDirection = false;
    if (temperature_ch0 < Schedules[0].Segments[SegmentIndex].Setpoint) {
      Segment_WillIncrement_Ch0 = true;
    } else {
      Segment_WillIncrement_Ch0 = false;
    }
    if (temperature_ch1 < Schedules[0].Segments[SegmentIndex].Setpoint) {
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
        if (temperature_ch0 >= Schedules[0].Segments[SegmentIndex].Setpoint) {
          Segment_AtTemp_Ch0 = true;
        } else {
          if (Setpoint_ch0 < Schedules[0].Segments[SegmentIndex].Setpoint) {
            Setpoint_ch0 = Setpoint_ch0 + RampChange(Schedules[0].Segments[SegmentIndex].RampRate,RampTimer_ElapsedTime);
          }
        }
      } else {
        if (temperature_ch0 <= Schedules[0].Segments[SegmentIndex].Setpoint) {
          Segment_AtTemp_Ch0 = true;
        } else {
          if (Setpoint_ch0 > Schedules[0].Segments[SegmentIndex].Setpoint) {
            Setpoint_ch0 = Setpoint_ch0 - RampChange(Schedules[0].Segments[SegmentIndex].RampRate,RampTimer_ElapsedTime);
          }
        }
      }
      
      if (Segment_WillIncrement_Ch1) {
        if (temperature_ch1 >= Schedules[0].Segments[SegmentIndex].Setpoint) {
          Segment_AtTemp_Ch1 = true;
        } else {
          if (Setpoint_ch1 < Schedules[0].Segments[SegmentIndex].Setpoint) {
            Setpoint_ch1 = Setpoint_ch1 + RampChange(Schedules[0].Segments[SegmentIndex].RampRate,RampTimer_ElapsedTime);
          }
        }
      } else {
        if (temperature_ch1 <= Schedules[0].Segments[SegmentIndex].Setpoint) {
          Segment_AtTemp_Ch1 = true;
        } else {
          if (Setpoint_ch1 > Schedules[0].Segments[SegmentIndex].Setpoint) {
            Setpoint_ch1 = Setpoint_ch1 - RampChange(Schedules[0].Segments[SegmentIndex].RampRate,RampTimer_ElapsedTime);
          }
        }
      } 
    }
  }
  /* advance profile once ramp temp reached */
  if ((Segment_AtTemp_Ch0 == true) && (Segment_AtTemp_Ch1 == true)) {
    Segment_AtTemp_Ch0 = false;
    Segment_AtTemp_Ch1 = false;
    Setpoint_ch0 = Schedules[0].Segments[SegmentIndex].Setpoint;
    Setpoint_ch1 = Schedules[0].Segments[SegmentIndex].Setpoint;
    ProfileSequence = SEGMENT_STATE_SOAK;
  }

  //
  // handle soak timer
  //
  // TODO: see if unsigned long and millis is big enough here - seconds?
  unsigned long SoakTimer_ElapsedTime = millis() - SoakTimer;
  if ((SoakTimer_ElapsedTime > SOAK_TIMER_PERIOD*60*1000) || !SoakTimerEnabled) { // in minutes - convert to milli
    if (SoakTimerEnabled) {
      if (Schedules[0].Segments[SegmentIndex].HoldEnabled) {
        ui_Segment_HoldReleaseRequest = true;
        Schedules[0].Segments[SegmentIndex].State = SEGMENT_STATE_HOLD;
        if (ui_Segment_HoldRelease) {
          ui_Segment_HoldRelease = false;
          ui_Segment_HoldReleaseRequest = false;
          SoakTimerEnabled = false;
          SegmentIndex++;
          ProfileSequence = SEGMENT_STATE_START;
        }
      } else {
        SoakTimerEnabled = false;
        if (SegmentIndex < NUMBER_OF_SEGMENTS) {
          SegmentIndex++;
          ProfileSequence = SEGMENT_STATE_START;
        } else {
          SegmentIndex = 0;
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
    double MeasuredRatePerSecond_ch0 = fabs(temperature_ch0 - temperatureLast_ch0)*((double)RateTimer_ElapsedTime/1000.0); // use fabs for floats/double instead of abs
    double MeasuredRatePerMinute_ch0 = MeasuredRatePerSecond_ch0*60.0;
    MeasuredRatePerHour_ch0 = MeasuredRatePerMinute_ch0*60.0;
    temperatureLast_ch0 = temperature_ch0;
    /* calculate rate ch1 */
    double MeasuredRatePerSecond_ch1 = fabs(temperature_ch1 - temperatureLast_ch1)*((double)RateTimer_ElapsedTime/1000.0);
    double MeasuredRatePerMinute_ch1 = MeasuredRatePerSecond_ch1*60.0;
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
      break;
    case SEGMENT_STATE_RAMP:
      // calculate time remaining ch0
      double dbl_hours_ch0, dbl_minutes_ch0, dbl_seconds_ch0, dbl_milli_ch0;
      if (MeasuredRatePerHour_ch0 > 0.0) {
        double temperatureDifference = fabs(Schedules[0].Segments[SegmentIndex].Setpoint - temperature_ch0);
        dbl_hours_ch0 = temperatureDifference / MeasuredRatePerHour_ch0;
      } else {
        dbl_hours_ch0 = 0.0;
      }
      // calculate time remaining ch1
      double dbl_hours_ch1, dbl_minutes_ch1, dbl_seconds_ch1, dbl_milli_ch1;
      if (MeasuredRatePerHour_ch1 > 0.0) {
        double temperatureDifference = fabs(Schedules[0].Segments[SegmentIndex].Setpoint - temperature_ch1);
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
        dbl_hours = Schedules[0].Segments[SegmentIndex].SoakTime/60.0 - ((((double)SoakTimer_ElapsedTime/1000.0)/60.0)/60.0);
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
        ui_Setpoint = Schedules[0].Segments[SegmentIndex].Setpoint;
      }
      break;
    case MANUAL_MODE:
      Setpoint_ch0 = ui_Setpoint;
      Setpoint_ch1 = ui_Setpoint;
      break;
    case SIMULATE_MODE:
      ui_Setpoint = Setpoint_ch0;
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
    if (ProfileSequence == SEGMENT_STATE_IDLE && ui_SelectedSchedule < NUMBER_OF_SCHEDULES){
      ui_SelectedScheduleLast = ui_SelectedSchedule;
      Schedules[ui_SelectedSchedule].CMD_Select = true;
    } else {
      ui_SelectedSchedule = ui_SelectedScheduleLast;
    }
  }
  
  if (ProfileSequence == SEGMENT_STATE_IDLE) {
    for (int i=1;i<NUMBER_OF_SCHEDULES;i++) { // skip 0 index
      if (Schedules[i].CMD_Select){
        idxSelectedSchedule = i;
        setSchedule = true;
      }
    }
    if (setSchedule) {
      setSchedule = false;
      for (int i=1;i<NUMBER_OF_SCHEDULES;i++) { // skip 0 index
        Schedules[i].CMD_Select = false;
        if (i != idxSelectedSchedule) {
          Schedules[i].STS_Select = false;
        } else {
          Schedules[i].STS_Select = true;
        }
      }

      /* copy schedule to 0 index */
      for (int i=0; i<sizeof(Schedules[idxSelectedSchedule].Name);i++) {
         Schedules[0].Name[i] = Schedules[idxSelectedSchedule].Name[i];
      }
      for (int i=0;i<NUMBER_OF_SEGMENTS;i++) {
        for (int j=0; j<sizeof(Schedules[idxSelectedSchedule].Segments[i].Name);j++) {
           Schedules[0].Segments[i].Name[j] = Schedules[idxSelectedSchedule].Segments[i].Name[j];
        }
        Schedules[0].Segments[i].Enabled = Schedules[idxSelectedSchedule].Segments[i].Enabled;
        Schedules[0].Segments[i].HoldEnabled = Schedules[idxSelectedSchedule].Segments[i].HoldEnabled;
        Schedules[0].Segments[i].Setpoint = Schedules[idxSelectedSchedule].Segments[i].Setpoint;
        Schedules[0].Segments[i].RampRate = Schedules[idxSelectedSchedule].Segments[i].RampRate;
        Schedules[0].Segments[i].SoakTime = Schedules[idxSelectedSchedule].Segments[i].SoakTime;
        Schedules[0].Segments[i].State = Schedules[idxSelectedSchedule].Segments[i].State;
      }
    }
  }
  
}

void checkInit(){
  //Serial.println(F("Checking initialization..."));
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
    applyDefaultScheduleSettings();
    makeInitialized();
  }
}

void makeInitialized(){
  //Serial.println(F("Initializing..."));
  for (byte i=0;i<sizeof(Initialized)-1;i++){
    EEPROM.write(i, Initialized[i]);
  }
  /*for (byte i=0;i<sizeof(version)-1;i++){
    EEPROM.write(i, version[i]);
  }*/
}

void applyDefaultScheduleSettings() {
  // cmd select
  Schedules[1].CMD_Select = true;
  Schedules[2].CMD_Select = false;
  Schedules[3].CMD_Select = false;
  Schedules[4].CMD_Select = false;
  // sts select
  Schedules[1].STS_Select = false;
  Schedules[2].STS_Select = false;
  Schedules[3].STS_Select = false;
  Schedules[4].STS_Select = false;
  // name
  char strTemp_Test[] = {"Test"};
  for (int i=0; i<sizeof(strTemp_Test);i++) {
     Schedules[0].Name[i] = strTemp_Test[i];
  }
  char strTemp1_1_1[] = {"Bisque 1"};
  for (int i=0; i<sizeof(strTemp1_1_1);i++) {
     Schedules[1].Name[i] = strTemp1_1_1[i];
  }
  char strTemp1_1_2[] = {"Bisque 2"};
  for (int i=0; i<sizeof(strTemp1_1_2);i++) {
     Schedules[2].Name[i] = strTemp1_1_2[i];
  }
  char strTemp1_1_3[] = {"Glaze 1"};
  for (int i=0; i<sizeof(strTemp1_1_3);i++) {
     Schedules[3].Name[i] = strTemp1_1_3[i];
  }
  char strTemp1_1_4[] = {"Glaze 2"};
  for (int i=0; i<sizeof(strTemp1_1_4);i++) {
     Schedules[4].Name[i] = strTemp1_1_4[i];
  }
  // segments
  for (int j=1; j<NUMBER_OF_SCHEDULES; j++) {
    char strTemp1_1[] = {"Candle"};
    for (int i=0; i<sizeof(strTemp1_1);i++) {
       Schedules[j].Segments[0].Name[i] = strTemp1_1[i];
    }
    char strTemp1_2[] = {"Work Temp 1"};
    for (int i=0; i<sizeof(strTemp1_2);i++) {
       Schedules[j].Segments[1].Name[i] = strTemp1_2[i];
    }
    char strTemp1_3[] = {"Cool Down 1"};
    for (int i=0; i<sizeof(strTemp1_3);i++) {
       Schedules[j].Segments[2].Name[i] = strTemp1_3[i];
    }
    char strTemp1_4[] = {"Work Temp 2"};
    for (int i=0; i<sizeof(strTemp1_4);i++) {
       Schedules[j].Segments[3].Name[i] = strTemp1_4[i];
    }
    char strTemp1_5[] = {"Cool Down 2"};
    for (int i=0; i<sizeof(strTemp1_5);i++) {
       Schedules[j].Segments[4].Name[i] = strTemp1_5[i];
    }
    char strTemp1_6[] = {"Cool Down 6"};
    for (int i=0; i<sizeof(strTemp1_6);i++) {
       Schedules[j].Segments[5].Name[i] = strTemp1_6[i];
    }
    char strTemp1_7[] = {"Cool Down 7"};
    for (int i=0; i<sizeof(strTemp1_7);i++) {
       Schedules[j].Segments[6].Name[i] = strTemp1_7[i];
    }
    char strTemp1_8[] = {"Cool Down 8"};
    for (int i=0; i<sizeof(strTemp1_8);i++) {
       Schedules[j].Segments[7].Name[i] = strTemp1_8[i];
    }
    char strTemp1_9[] = {"Cool Down 9"};
    for (int i=0; i<sizeof(strTemp1_9);i++) {
       Schedules[j].Segments[8].Name[i] = strTemp1_9[i];
    }
    // enabled
    for (int k=0;k<NUMBER_OF_SEGMENTS;k++) {
      Schedules[j].Segments[k].Enabled = true;
    }
    // hold enabled
    Schedules[j].Segments[0].HoldEnabled = true; // first segment is true
    for (int k=1;k<NUMBER_OF_SEGMENTS;k++) {
      Schedules[j].Segments[k].HoldEnabled = false;
    }
    // setpoint
    Schedules[j].Segments[0].Setpoint = 80.0;
    Schedules[j].Segments[1].Setpoint = 500.0;
    Schedules[j].Segments[2].Setpoint = 200.0;
    Schedules[j].Segments[3].Setpoint = 400.0;
    Schedules[j].Segments[4].Setpoint = 5.0;
    Schedules[j].Segments[5].Setpoint = 5.0;
    Schedules[j].Segments[6].Setpoint = 5.0;
    Schedules[j].Segments[7].Setpoint = 5.0;
    Schedules[j].Segments[8].Setpoint = 5.0;
    Schedules[j].Segments[9].Setpoint = 5.0;
    // ramprate
    Schedules[j].Segments[0].RampRate = 100;
    Schedules[j].Segments[1].RampRate = 200;
    Schedules[j].Segments[2].RampRate = 200;
    Schedules[j].Segments[3].RampRate = 100;
    Schedules[j].Segments[4].RampRate = 200;
    Schedules[j].Segments[5].RampRate = 200;
    Schedules[j].Segments[6].RampRate = 200;
    Schedules[j].Segments[7].RampRate = 200;
    Schedules[j].Segments[8].RampRate = 200;
    Schedules[j].Segments[9].RampRate = 200;
    // soaktime
    for (int k=0;k<NUMBER_OF_SEGMENTS;k++) {
      Schedules[j].Segments[k].SoakTime = 1;
    }
    // state
    for (int k=0;k<NUMBER_OF_SEGMENTS;k++) {
      Schedules[j].Segments[k].State = SEGMENT_STATE_IDLE;
    }
  }

  writeScheduleToEeeprom();
}

void writeScheduleToEeeprom() {
  
  int address = EEPROM_SCH_START_ADDR;

  for (int i=0; i<NUMBER_OF_SCHEDULES; i++) {
    /* schedule name */
    for (int j=0; j<sizeof(Schedules[i].Name); j++) {
      EEPROM.put(address, Schedules[i].Name[j]);
      address = address + sizeof(address, Schedules[i].Name[j]);
    }
    for (int k=0; k<NUMBER_OF_SEGMENTS; k++) {
      /* segment name */
      for (int x=0; x<sizeof(Schedules[i].Segments[k].Name); x++) {
        EEPROM.put(address, Schedules[i].Segments[k].Name[x]);
        address = address + sizeof(Schedules[i].Segments[k].Name[x]);
      }
      /* segment enabled */
      EEPROM.put(address, Schedules[i].Segments[k].Enabled);
      address = address + sizeof(address, Schedules[i].Segments[k].Enabled);
      /* hold enabled */
      EEPROM.put(address, Schedules[i].Segments[k].HoldEnabled);
      address = address + sizeof(address, Schedules[i].Segments[k].HoldEnabled);
      /* setpoint */
      EEPROM.put(address, Schedules[i].Segments[k].Setpoint);
      address = address + sizeof(address, Schedules[i].Segments[k].Setpoint);
      /* ramp rate */
      EEPROM.put(address, Schedules[i].Segments[k].RampRate);
      address = address + sizeof(address, Schedules[i].Segments[k].RampRate);
      /* soak time */
      EEPROM.put(address, Schedules[i].Segments[k].SoakTime);
      address = address + sizeof(address, Schedules[i].Segments[k].SoakTime);
    }
  }
  /* commit to simulated eeprom (flash) */
  EEPROM.commit();
}

void readScheduleFromEeeprom() {
  
  int address = EEPROM_SCH_START_ADDR;

  for (int i=0; i<NUMBER_OF_SCHEDULES; i++) {
    /* schedule name */
    for (int j=0; j<sizeof(Schedules[i].Name); j++) {
      EEPROM.get(address, Schedules[i].Name[j]);
      address = address + sizeof(address, Schedules[i].Name[j]);
    }
    for (int k=0; k<NUMBER_OF_SEGMENTS; k++) {
      /* segment name */
      for (int x=0; x<sizeof(Schedules[i].Segments[k].Name); x++) {
        EEPROM.get(address, Schedules[i].Segments[k].Name[x]);
        address = address + sizeof(Schedules[i].Segments[k].Name[x]);
      }
      /* segment enabled */
      EEPROM.get(address, Schedules[i].Segments[k].Enabled);
      address = address + sizeof(address, Schedules[i].Segments[k].Enabled);
      /* hold enabled */
      EEPROM.get(address, Schedules[i].Segments[k].HoldEnabled);
      address = address + sizeof(address, Schedules[i].Segments[k].HoldEnabled);
      /* setpoint */
      EEPROM.get(address, Schedules[i].Segments[k].Setpoint);
      address = address + sizeof(address, Schedules[i].Segments[k].Setpoint);
      /* ramp rate */
      EEPROM.get(address, Schedules[i].Segments[k].RampRate);
      address = address + sizeof(address, Schedules[i].Segments[k].RampRate);
      /* soak time */
      EEPROM.get(address, Schedules[i].Segments[k].SoakTime);
      address = address + sizeof(address, Schedules[i].Segments[k].SoakTime);
    }
  }
}

#define THERMAL_RUNAWAY_TEMPERATURE_TIMER 10000 // 10000 is 10 seconds
#define THERMAL_RUNAWAY_RATE_TIMER 120000 // 120000 i2 2 min
bool TemperatureDifferenceDetected = false;
bool RateDifferenceDetected = false;
double Tolerance_Rate = 20.0, Tolerance_Temperature = 35.0;
unsigned int ThermalRunawayTemperature_Timer = millis();
unsigned int ThermalRunawayRate_Timer = millis();

void handleThermalRunaway() {

  switch (ProfileSequence) {
    case SEGMENT_STATE_IDLE:
      break;
    case SEGMENT_STATE_INIT:
      break;
    case SEGMENT_STATE_START:
      break;
    case SEGMENT_STATE_RAMP:
      /* ch0 */
      if ((MeasuredRatePerHour_ch0 > Schedules[0].Segments[SegmentIndex].RampRate + Tolerance_Rate) || 
          (MeasuredRatePerHour_ch0 < Schedules[0].Segments[SegmentIndex].RampRate - Tolerance_Rate)) {
            RateDifferenceDetected = true;
          }
      /* ch1 */
      if ((MeasuredRatePerHour_ch1 > Schedules[0].Segments[SegmentIndex].RampRate + Tolerance_Rate) || 
          (MeasuredRatePerHour_ch1 < Schedules[0].Segments[SegmentIndex].RampRate - Tolerance_Rate)) {
            RateDifferenceDetected = true;
          }
      break;
    case SEGMENT_STATE_SOAK:
      /* ch0 */
      if ((temperature_ch0 > Schedules[0].Segments[SegmentIndex].Setpoint + Tolerance_Temperature) || 
          (temperature_ch0 < Schedules[0].Segments[SegmentIndex].Setpoint - Tolerance_Temperature)) {
            TemperatureDifferenceDetected = true;
          }
      /* ch1 */
      if ((temperature_ch1 > Schedules[0].Segments[SegmentIndex].Setpoint + Tolerance_Temperature) || 
          (temperature_ch1 < Schedules[0].Segments[SegmentIndex].Setpoint - Tolerance_Temperature)) {
            TemperatureDifferenceDetected = true;
          }
      break;
  }
  // check temperature difference
  unsigned int ThermalRunawayTemperatureTimer_Elapsed = millis() - ThermalRunawayTemperature_Timer;
  if (ThermalRunawayTemperatureTimer_Elapsed > THERMAL_RUNAWAY_TEMPERATURE_TIMER || !TemperatureDifferenceDetected) {
    if (TemperatureDifferenceDetected) {
      ThermalRunawayDetected = true;
    } else {
      ThermalRunawayTemperature_Timer = millis();
    }
  }
  TemperatureDifferenceDetected = false;
  // check rate difference
  unsigned int ThermalRunawayRateTimer_Elapsed = millis() - ThermalRunawayRate_Timer;
  if (ThermalRunawayRateTimer_Elapsed > THERMAL_RUNAWAY_RATE_TIMER || !RateDifferenceDetected) {
    if (RateDifferenceDetected) {
      ThermalRunawayDetected = true;
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

void handleMainContactor() {
  if (Safety_Ok && !ThermalRunawayDetected) {
    digitalWrite(MAIN_CONTACTOR_OUTPUT, LOW);
  } else {
    digitalWrite(MAIN_CONTACTOR_OUTPUT, HIGH);
  }
}

void setup() {
  
  EEPROM.begin(512);  //Initialize EEPROM 
  /* check if this is a new device */
  checkInit();
  readScheduleFromEeeprom();
   
  //
  // setup pins
  //
  //pinMode(ESP_BUILTIN_LED, OUTPUT);
  pinMode(SSR_PIN_01, OUTPUT);
  pinMode(SSR_PIN_02, OUTPUT);
  pinMode(SAFETY_CIRCUIT_INPUT, INPUT);
  pinMode(MAIN_CONTACTOR_OUTPUT, OUTPUT);
  //digitalWrite(MAIN_CONTACTOR_OUTPUT, LOW);
  digitalWrite(MAIN_CONTACTOR_OUTPUT, HIGH);
  
  //
  // setup timers
  //
  timer_heartbeat = millis();

  //
  // setup PID
  //
  windowStartTime_01 = millis();
  windowStartTime_02 = millis();
  //initialize the variables we're linked to
  Setpoint_ch0 = 0.0;
  Setpoint_ch1 = 0.0;
  //tell the PID to range between 0 and the full window size
  PID_01.SetOutputLimits(0, WindowSize);
  PID_02.SetOutputLimits(0, WindowSize);
  //turn the PID on
  PID_01.SetMode(AUTOMATIC);
  PID_02.SetMode(AUTOMATIC);

  //
  // setup thermocouple sensors
  //
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
  
  //
  // setup wifi
  //
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  int TryCount = 0;
  while (WiFi.waitForConnectResult() != WL_CONNECTED && TryCount < 3) {
    TryCount++;
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  //
  // setup OTA
  //
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);
  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(OTA_HOSTNAME);
  // No authentication by default
  ArduinoOTA.setPassword((const char *)OTA_PASSWORD);
  ArduinoOTA.onStart([]() {
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

  //
  // setup modbus
  //
  Serial.begin(115200, SERIAL_8N1);
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
  
  //mb_rtu.onGetCoil(COIL_BASE, cbRead, LEN); // Add callback on Coils value get
  //mb_rtu.onSetCoil(MB_CMD_SELECT_SCHEDULE, cbSchedule);      // Add callback on Coil LED_COIL value set
  mb_rtu.onSetCoil(MB_CMD_WRITE_EEPROM, cbWriteEeprom);
  
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
  
  //
  // done with setup
  //
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {

  //
  // handle logic
  //
  handleSafetyCircuit();
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
      HEARTBEAT_VALUE = -100;
    } else {
      HEARTBEAT_VALUE = HEARTBEAT_VALUE + 1;
    }
  }
  
  //
  // handle OTA requests
  //
  ArduinoOTA.handle();
    
  //
  // handle REST calls
  //
  //WiFiClient client = server.available();
  //if (!client) {
  //  return;
  //}
  //while(!client.available()){
  //  delay(1);
  //}
  //rest.handle(client);
  
  yield();
}
