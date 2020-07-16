/*********
  Board settings:
    Board: NodeMCU 1.0 (ESP-12E Module)
    Flash Size: 4M (FS:1MBOTA:~1019KB)
    Debug Port: Disabled
    Debug Level: None
    IwIP Variant: v2 Lower Memory
    VTables: Flash
    CPU Frequency: 160MHz
    Upload Speed: 115200
    Erase Flash: All Flash Contents
    Builtin LED: 2
*********/

// NOTE: ****** kiln was flashed with ESP Board version 2.7.2

// sketch will write default settings if new build
//const char version[] = "build "  __DATE__ " " __TIME__; 
const char version[] = __DATE__ " " __TIME__; 

#if !defined(ARRAY_SIZE)
    #define ARRAY_SIZE(x) (sizeof((x)) / sizeof((x)[0]))
#endif

#include <ESP8266WiFi.h>
//#include <ESP8266mDNS.h>
//#include <WiFiUdp.h>
#include <ArduinoOTA.h>
//#define OUTPUT_BUFFER_SIZE 5000 // for aRest
//#define DEBUG_MODE 1 // for aRest
#include <aREST.h>

//
// modbus
//
#define COIL_VAL(v)   // get coil value
#define COIL_BOOL(v)  // assign coil value
#define ISTS_VAL(v)   // get status value
#define ISTS_BOOL(v)  // assign status value
#include <ModbusRTU.h>
#define SLAVE_ID                  1
/* coils (RW) */
#define MB_CMD_SELECT_SCHEDULE    1
#define MB_CMD_START_PROFILE      2
#define MB_CMD_STOP_PROFILE       3
#define MB_CMD_HOLD_RELEASE       4
/* input status (R) */
#define MB_STS_SSR_01             1
#define MB_STS_SSR_02             2
#define MB_STS_RELEASE_REQ        3
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
ModbusRTU mb;
int HEARTBEAT_VALUE = 0;
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

//
// pid
//
#include <PID_v1.h>
//Define Variables we'll be connecting to
double Setpoint_ch0, Setpoint_ch1, Input_01, Input_02, Output_01, Output_02;
//Specify the links and initial tuning parameters
double Kp_01=2, Ki_01=5, Kd_01=1;
double Kp_02=2, Ki_02=5, Kd_02=1;
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
#define MAXCS_CH1   D3 // D3 is GPIO0 connected to FLASH button on NodeMCU
Adafruit_MAX31855 thermocouple_ch1(MAXCS_CH1);

//
// defines
//
#define WIFI_SSID                 "Thomas_301"
#define WIFI_PASSWORD             "RS232@12"
#define WIFI_LISTENING_PORT       80
#define SERIAL_BAUD_RATE          115200
#define REST_ID                   "1"
#define REST_NAME                 "NodeMCU" // having this too long was causing problems with aRest
#define OTA_PASSWORD              "ProFiBus@12"
#define OTA_HOSTNAME              "NodeMCU" // having this too long was causing problems with aRest
#define HEARTBEAT_TIME            1000

#define AUTOMATIC_MODE            1
#define MANUAL_MODE               2
#define SIMULATE_MODE             3

//
// init variables
//
const int ESP_BUILTIN_LED = LED_BUILTIN;
//const int ANALOG_PIN = A0; // causing issues - see below
const int SSR_PIN_01 = D1; // D1 D2 D5 D6 D7 (these are recommended pins)
const int SSR_PIN_02 = D2;
unsigned long timer_heartbeat;

//
// init wifi
//
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

//
// init REST API
//
// Create an instance of the server
WiFiServer server(WIFI_LISTENING_PORT);
// Create aREST instance
aREST rest = aREST();
// create variables to expose to REST API
// these variables can be called using the following:
// http://ipaddress/variable03
double temperature_ch0, temperature_ch1;

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
#define NUMBER_OF_SEGMENTS    5
#define RATE_TIMER_PERIOD     1000
#define RAMP_TIMER_PERIOD     1000
uint16_t SOAK_TIMER_PERIOD = 0;
unsigned long RampTimer, SoakTimer, RateTimer=millis();
bool RampTimerEnabled = false;
bool SoakTimerEnabled = false;
uint16_t ProfileSequence = 0;
uint16_t ProfileSequence_Next = 0;
uint16_t SegmentIndex = 0;
bool Segment_CheckDirection = false;
bool Segment_WillIncrement_Ch0 = false;
bool Segment_WillIncrement_Ch1 = false;
bool Segment_AtTemp_Ch0 = false;
bool Segment_AtTemp_Ch1 = false;
double temperatureLast_ch0 = 0.0, temperatureLast_ch1 = 0.0;
/* user interface */
uint16_t Mode = AUTOMATIC_MODE;
bool ui_StartProfile = false;
bool ui_StopProfile = false;
bool ui_Segment_HoldReleaseRequest = false;
bool ui_Segment_HoldRelease = false;
bool ui_SelectSchedule = false;
uint16_t ui_SelectedSchedule = 0;
double ui_Setpoint = 0.0;

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

void setup() {
   
  //
  // setup pins
  //
  pinMode(ESP_BUILTIN_LED, OUTPUT);
  pinMode(SSR_PIN_01, OUTPUT);
  pinMode(SSR_PIN_02, OUTPUT);
  
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
  Setpoint_ch0 = 77.0;
  Setpoint_ch1 = 81.0;
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
  if (!thermocouple_ch1.begin()) {
    //Serial.println("ERROR.");
    while (1) delay(10);
  }

  //
  // setup schedules
  //
  readScheduleSettings();
  
  //
  // setup wifi
  //
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  
  //
  // setup REST API
  //
  // Init variables and expose them to REST API
  rest.variable("ui_SelectSchedule",&ui_SelectSchedule);
  rest.variable("Kp_01",&Kp_01);
  rest.variable("Temperature_Ch0",&temperature_ch0);
  rest.variable("Temperature_Ch1",&temperature_ch1);
  // Functions to be exposed to REST API
  rest.function("function01",function01);
  rest.function("function02",function02);
  rest.function("function03",function03);
  // Give name & ID to the device (ID should be 6 characters long)
  rest.set_id(REST_ID);
  rest.set_name(REST_NAME);
  // Start the server
  server.begin();
  //Serial.println("Server started");

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
  mb.begin(&Serial);
  mb.slave(SLAVE_ID);
  /* coils (RW) */
  mb.addCoil(MB_CMD_SELECT_SCHEDULE);
  mb.addCoil(MB_CMD_START_PROFILE);
  mb.addCoil(MB_CMD_STOP_PROFILE);
  mb.addCoil(MB_CMD_HOLD_RELEASE);
  /* input status (R) */
  mb.addIsts(MB_STS_SSR_01);
  mb.addIsts(MB_STS_SSR_02);
  mb.addIsts(MB_STS_RELEASE_REQ);
  /* holding registers (RW) */
  mb.addHreg(MB_MODE,0,1);
  mb.addHreg(MB_CMD_SELECTED_SCHEDULE,0,1);
  mb.addHreg(MB_CMD_SETPOINT,0,2);
  mb.addHreg(MB_PID_P_01,Kp_01,2);
  mb.addHreg(MB_PID_I_01,Ki_01,2);
  mb.addHreg(MB_PID_D_01,Kd_01,2);
  mb.addHreg(MB_PID_P_02,Kp_02,2);
  mb.addHreg(MB_PID_I_02,Ki_02,2);
  mb.addHreg(MB_PID_D_02,Kd_02,2);
  /* input registers (R) */
  mb.addIreg(MB_HEARTBEAT,0,1);
  mb.addIreg(MB_STS_REMAINING_TIME_H,0,1);
  mb.addIreg(MB_STS_REMAINING_TIME_M,0,1);
  mb.addIreg(MB_STS_REMAINING_TIME_S,0,1);
  mb.addIreg(MB_STS_TEMPERATURE_01,1,2);
  mb.addIreg(MB_STS_TEMPERATURE_02,2,2);
  mb.addIreg(MB_STS_PID_01_OUTPUT,3,2);
  mb.addIreg(MB_STS_PID_02_OUTPUT,3,2);
  mb.addIreg(MB_NUMBER_OF_SCHEDULES,NUMBER_OF_SCHEDULES,1);
  mb.addIreg(MB_NUMBER_OF_SEGMENTS,NUMBER_OF_SEGMENTS,1);
  mb.addIreg(MB_STS_SEGMENT_STATE,0,1);
  mb.addIreg(MB_STS_SEGMENT_NAME,0,8);
  mb.addIreg(MB_STS_SCHEDULE_NAME,0,8);
  
  //mb.onGetCoil(COIL_BASE, cbRead, LEN); // Add callback on Coils value get
  //mb.onSetCoil(MB_CMD_SELECT_SCHEDULE, cbSchedule);      // Add callback on Coil LED_COIL value set
  
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

union floatAsBytes {
  float fval;
  uint16_t ui[2];
  byte bval[4];
};

union charAsUnit16 {
  uint16_t reg;
  char c[2];
};

void handleModbus() {
  /* coils (RW) */
  mb.Coil(MB_CMD_SELECT_SCHEDULE, ui_SelectSchedule);
  mb.Coil(MB_CMD_START_PROFILE, ui_StartProfile);
  mb.Coil(MB_CMD_STOP_PROFILE, ui_StopProfile);
  mb.Coil(MB_CMD_HOLD_RELEASE, ui_Segment_HoldRelease);
  /* input status (R) */
  mb.Ists(MB_STS_RELEASE_REQ, ui_Segment_HoldReleaseRequest);
  mb.Ists(MB_STS_SSR_01, SSR_PIN_01);
  mb.Ists(MB_STS_SSR_02, SSR_PIN_02);
  /* holding registers (RW) */
  mb.Hreg(MB_MODE, Mode);
  mb.Hreg(MB_CMD_SELECTED_SCHEDULE, ui_SelectedSchedule);
  mb.Hreg(MB_CMD_SETPOINT, ui_Setpoint);
  mb.Hreg(MB_PID_P_01,Kp_01);
  mb.Hreg(MB_PID_I_01,Ki_01);
  mb.Hreg(MB_PID_D_01,Kd_01);
  mb.Hreg(MB_PID_P_02,Kp_02);
  mb.Hreg(MB_PID_I_02,Ki_02);
  mb.Hreg(MB_PID_D_02,Kd_02);
  /* input registers (R) */
  mb.Ireg(MB_HEARTBEAT, HEARTBEAT_VALUE);// assign holding register value
  mb.Ireg(MB_STS_REMAINING_TIME_H, Segment_TimeRemaining.hours);
  mb.Ireg(MB_STS_REMAINING_TIME_M, Segment_TimeRemaining.minutes);
  mb.Ireg(MB_STS_REMAINING_TIME_S, Segment_TimeRemaining.seconds);
  floatAsBytes fltInput_01; 
  fltInput_01.fval = temperature_ch0;
  mb.Ireg(MB_STS_TEMPERATURE_01, fltInput_01.ui[0]);
  mb.Ireg(MB_STS_TEMPERATURE_01+1, fltInput_01.ui[1]);
  floatAsBytes fltInput_02; 
  fltInput_02.fval = temperature_ch1;
  mb.Ireg(MB_STS_TEMPERATURE_02, fltInput_02.ui[0]);
  mb.Ireg(MB_STS_TEMPERATURE_02+1, fltInput_02.ui[1]);
  floatAsBytes fltOutput_01; 
  fltOutput_01.fval = Output_01;
  mb.Ireg(MB_STS_PID_01_OUTPUT, fltOutput_01.ui[0]);
  mb.Ireg(MB_STS_PID_01_OUTPUT+1, fltOutput_01.ui[1]);
  floatAsBytes fltOutput_02; 
  fltOutput_02.fval = Output_02;
  mb.Ireg(MB_STS_PID_02_OUTPUT, fltOutput_02.ui[0]);
  mb.Ireg(MB_STS_PID_02_OUTPUT+1, fltOutput_02.ui[1]);
  //mb.Ireg(MB_NUMBER_OF_SCHEDULES, NUMBER_OF_SCHEDULES); // written once in setup
  //mb.Ireg(MB_NUMBER_OF_SEGMENTS, NUMBER_OF_SEGMENTS); // written once in setup
  mb.Ireg(MB_STS_SEGMENT_STATE, Schedules[0].Segments[SegmentIndex].State );
  int j = 0;
  for (int i=0; i<sizeof(Schedules[ui_SelectedSchedule].Segments[SegmentIndex].Name);i=i+2) {
    charAsUnit16 temp;
    temp.c[0] = Schedules[ui_SelectedSchedule].Segments[SegmentIndex].Name[i];
    temp.c[1] = Schedules[ui_SelectedSchedule].Segments[SegmentIndex].Name[i + 1];
    mb.Ireg(MB_STS_SEGMENT_NAME + j, temp.reg);
    j++;
  }
  int k = 0;
  for (int i=0; i<sizeof(Schedules[ui_SelectedSchedule].Name);i=i+2) {
    charAsUnit16 temp;
    temp.c[0] = Schedules[ui_SelectedSchedule].Name[i];
    temp.c[1] = Schedules[ui_SelectedSchedule].Name[i+1];
    mb.Ireg(MB_STS_SCHEDULE_NAME + k, temp.reg);
    k++;
  }
  
  mb.task();
  
  /* coils (RW) */
  ui_SelectSchedule = mb.Coil(MB_CMD_SELECT_SCHEDULE);
  ui_StartProfile = mb.Coil(MB_CMD_START_PROFILE);
  ui_StopProfile = mb.Coil(MB_CMD_STOP_PROFILE);
  ui_Segment_HoldRelease = mb.Coil(MB_CMD_HOLD_RELEASE);
  /* holding registers (RW) */
  Mode = mb.Hreg(MB_MODE);
  ui_SelectedSchedule = mb.Hreg(MB_CMD_SELECTED_SCHEDULE);
  ui_Setpoint = mb.Hreg(MB_CMD_SETPOINT);
  Kp_01 = mb.Hreg(MB_PID_P_01);
  Ki_01 = mb.Hreg(MB_PID_I_01);
  Kd_01 = mb.Hreg(MB_PID_D_01);
  Kp_02 = mb.Hreg(MB_PID_P_02);
  Ki_02 = mb.Hreg(MB_PID_I_02);
  Kd_02 = mb.Hreg(MB_PID_D_02);
}
  
void loop() {

  //
  // handle logic
  //
  setSchedule();
  handleProfileSequence();

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
  // handle thermocouple sensors
  //                   
  temperature_ch0 = map(thermocouple_ch0.readFahrenheit(),70.2,3000.0,76.9,3000.0); //map(value,fromlow,fromhigh,tolow,tohigh);
  if (isnan(temperature_ch0)) {
    temperature_ch0 = 0.0;
   //Serial.println("Something wrong with thermocouple!");
  } else {
   //Serial.print("C = ");
   //Serial.println(c);
  }
  temperature_ch1 = map(thermocouple_ch1.readFahrenheit(),62.6,3000.0,76.9,3000.0);
  if (isnan(temperature_ch1)) {
    temperature_ch1 = 0.0;
   //Serial.println("Something wrong with thermocouple!");
  } else {
   //Serial.print("C = ");
   //Serial.println(c);
  }
  if (Mode == SIMULATE_MODE) {
    temperature_ch0 = Setpoint_ch0;
    temperature_ch1 = Setpoint_ch1;
  }

  //
  // handle PID
  //
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
  if (Output_01 < millis() - windowStartTime_01) digitalWrite(SSR_PIN_01, HIGH);
  else digitalWrite(SSR_PIN_01, LOW); // esp outputs reversed
  
  if (millis() - windowStartTime_02 > WindowSize)
  { //time to shift the Relay Window
    windowStartTime_02 += WindowSize;
  }
  if (Output_02 < millis() - windowStartTime_02) digitalWrite(SSR_PIN_02, HIGH);
  else digitalWrite(SSR_PIN_02, LOW); // esp outputs reversed

  //
  // handle modbus
  //
  handleModbus();
  
  //
  // handle OTA requests
  //
  ArduinoOTA.handle();
    
  //
  // handle REST calls
  //
  WiFiClient client = server.available();
  if (!client) {
    return;
  }
  while(!client.available()){
    delay(1);
  }
  rest.handle(client);

}

void handleProfileSequence(){
  //
  // manage sequencing the profile
  //
  if (ui_StopProfile) {
    ui_StopProfile = false;
    ProfileSequence = 0000;
  }
  switch (ProfileSequence) {
    case 0000: /* idle */
      // TODO: set schedule name
      Schedules[0].Segments[SegmentIndex].State = SEGMENT_STATE_IDLE;
      RampTimer = millis(); RampTimerEnabled = false;
      SoakTimer = millis(); SoakTimerEnabled = false;
      if (ui_StartProfile == true and (Mode == AUTOMATIC_MODE || Mode == SIMULATE_MODE)) {
        ui_StartProfile = false;
        SegmentIndex = 0;
        ProfileSequence = 0100;
      }
      break;
      
    case 0100: /* initialize */
      // TODO: set segment name
      Schedules[0].Segments[SegmentIndex].State = SEGMENT_STATE_INIT;
      // TODO: put PIDs in auto
      Setpoint_ch0 = temperature_ch0;
      Setpoint_ch1 = temperature_ch1;
      // TODO: check if PIDs are in auto before moving to next sequence
      // TODO: add watchdog timer for PIDs in auto
      ProfileSequence = 1000;
      break;
      
    case 1000: /* first segment start */
      if (Schedules[0].Segments[SegmentIndex].Enabled) {
        Schedules[0].Segments[SegmentIndex].State = SEGMENT_STATE_START;
        Segment_CheckDirection      = true;
        Segment_WillIncrement_Ch0   = false;
        Segment_WillIncrement_Ch1   = false;
        Segment_AtTemp_Ch0          = false;
        Segment_AtTemp_Ch1          = false;
        ui_Segment_HoldReleaseRequest  = false;
        ui_Segment_HoldRelease       = false;
        ProfileSequence = ProfileSequence + 100;
      } else {
        if (SegmentIndex < NUMBER_OF_SEGMENTS) {
          SegmentIndex = SegmentIndex++;
        } else {
          ProfileSequence = 0000;
        }
      }
      break;
      
    case 1100: /* first segment ramp to temp */
      Schedules[0].Segments[SegmentIndex].State = SEGMENT_STATE_RAMP;
      RampTimerEnabled = true;
      ProfileSequence_Next = ProfileSequence + 100;
      break;
      
    case 1200: /* first segment soak at temp */
      Schedules[0].Segments[SegmentIndex].State = SEGMENT_STATE_SOAK;
      SoakTimerEnabled = true;
      ProfileSequence_Next = 1000;
      break;
      
    default:
      ProfileSequence = 0000;
      break;
  }

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
  if (Segment_AtTemp_Ch0 and Segment_AtTemp_Ch1) {
    Segment_AtTemp_Ch0 = false;
    Segment_AtTemp_Ch1 = false;
    Setpoint_ch0 = Schedules[0].Segments[SegmentIndex].Setpoint;
    Setpoint_ch1 = Schedules[0].Segments[SegmentIndex].Setpoint;
    ProfileSequence = ProfileSequence_Next;
  }

  //
  // handle soak timer
  //
  unsigned long SoakTimer_ElapsedTime = millis() - SoakTimer;
  if ((SoakTimer_ElapsedTime > SOAK_TIMER_PERIOD*60*1000) || !SoakTimerEnabled) { // in minutes - convert to milli
    if (SoakTimerEnabled) {
      if (Schedules[0].Segments[SegmentIndex].HoldEnabled) {
        ui_Segment_HoldReleaseRequest = true;
        Schedules[0].Segments[SegmentIndex].State = SEGMENT_STATE_HOLD;
        if (ui_Segment_HoldRelease) {
          ui_Segment_HoldRelease = false;
          ui_Segment_HoldReleaseRequest = false;
          SoakTimer = millis();
          SoakTimerEnabled = false;
          ProfileSequence = ProfileSequence_Next;
        }
      } else {
        SoakTimer = millis();
        SoakTimerEnabled = false;
        if (SegmentIndex < NUMBER_OF_SEGMENTS) {
          SegmentIndex = SegmentIndex++;
          ProfileSequence = ProfileSequence_Next;
        } else {
          SegmentIndex = 0;
          ProfileSequence = 0000;
        }
      }
    } else {
      SoakTimer = millis();
    }
  }

  //
  // measure current rate
  //
  double MeasuredRatePerHour_ch0, MeasuredRatePerHour_ch1;
  unsigned long RateTimer_ElapsedTime = millis() - RateTimer;
  if (RateTimer_ElapsedTime > RATE_TIMER_PERIOD) {
    RateTimer - millis();
    /* calculate rate ch0 */
    double MeasuredRatePerSecond_ch0 = abs(temperature_ch0 - temperatureLast_ch0)*((double)RateTimer_ElapsedTime/1000.0);
    double MeasuredRatePerMinute_ch0 = MeasuredRatePerSecond_ch0*60.0;
    MeasuredRatePerHour_ch0 = MeasuredRatePerMinute_ch0*60.0;
    temperatureLast_ch0 = temperature_ch0;
    /* calculate rate ch1 */
    double MeasuredRatePerSecond_ch1 = abs(temperature_ch1 - temperatureLast_ch1)*((double)RateTimer_ElapsedTime/1000.0);
    double MeasuredRatePerMinute_ch1 = MeasuredRatePerSecond_ch1*60.0;
    MeasuredRatePerHour_ch1 = MeasuredRatePerMinute_ch1*60.0;
    temperatureLast_ch1 = temperature_ch1;
  }

  //
  // calculate time remaining
  //
  double dbl_hours, dbl_minutes, dbl_seconds, dbl_milli;
  switch (ProfileSequence) {
    case 0000:
      dbl_hours = 0.0;
      dbl_minutes = 0.0;
      dbl_seconds = 0.0;
      dbl_milli = 0.0;
      break;
    case 1100:
      // calculate time remaining ch0
      if (MeasuredRatePerHour_ch0 > 0.0) {
        double temperatureDifference = abs(Schedules[0].Segments[SegmentIndex].Setpoint - temperature_ch0);
        dbl_hours = temperatureDifference / MeasuredRatePerHour_ch0;
        dbl_minutes = (dbl_hours - (uint16_t)dbl_hours)*60.0;
        dbl_seconds = (dbl_minutes - (uint16_t)dbl_minutes)*60.0;
        dbl_milli = (dbl_seconds - (uint16_t)dbl_seconds)*1000;
      } else {
        dbl_hours = 0.0;
        dbl_minutes = 0.0;
        dbl_seconds = 0.0;
        dbl_milli = 0.0;
      }
      break;
    case 1200:
      // set time remaining
      dbl_hours = Schedules[0].Segments[SegmentIndex].SoakTime/60.0 - ((((double)SoakTimer_ElapsedTime/1000.0)/60.0)/60.0);
      dbl_minutes = (dbl_hours - (uint16_t)dbl_hours)*60.0;
      dbl_seconds = (dbl_minutes - (uint16_t)dbl_minutes)*60.0;
      dbl_milli = (dbl_seconds - (uint16_t)dbl_seconds)*1000;
      break;
  }
  Segment_TimeRemaining.hours = (uint16_t)dbl_hours;
  Segment_TimeRemaining.minutes = (uint16_t)dbl_minutes;
  Segment_TimeRemaining.seconds = (uint16_t)dbl_seconds;
  Segment_TimeRemaining.milliseconds = (uint16_t)dbl_milli;

  if (Mode == SIMULATE_MODE) {
    ui_Setpoint = Setpoint_ch0;
  } else {
    ui_Setpoint = Schedules[0].Segments[SegmentIndex].Setpoint;
  }
}

float RampChange (uint16_t RampRate, unsigned long ElapsedTime) {
  return (((float)RampRate/60.0)/(60.0/((float)ElapsedTime/1000.0)));
}

void setSchedule () {
  int idxSelectedSchedule = 0;
  bool setSchedule = false;

  if (ui_SelectSchedule && ui_SelectedSchedule < NUMBER_OF_SCHEDULES) {
    ui_SelectSchedule = false;
    Schedules[ui_SelectedSchedule].CMD_Select = true;
  }
  
  if (ProfileSequence == 0) {
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

void readScheduleSettings() {
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
    // enabled
    for (int k=0;k<NUMBER_OF_SEGMENTS;k++) {
      Schedules[j].Segments[k].Enabled = true;
    }
    // hold enabled
    for (int k=0;k<NUMBER_OF_SEGMENTS;k++) {
      Schedules[j].Segments[k].HoldEnabled = false;
    }
    // setpoint
    Schedules[j].Segments[0].Setpoint = 80.0;
    Schedules[j].Segments[1].Setpoint = 500.0;
    Schedules[j].Segments[2].Setpoint = 200.0;
    Schedules[j].Segments[3].Setpoint = 400.0;
    Schedules[j].Segments[4].Setpoint = 5.0;
    // ramprate
    Schedules[j].Segments[0].RampRate = 100;
    Schedules[j].Segments[1].RampRate = 200;
    Schedules[j].Segments[2].RampRate = 200;
    Schedules[j].Segments[3].RampRate = 100;
    Schedules[j].Segments[4].RampRate = 200;
    // soaktime
    for (int k=0;k<NUMBER_OF_SEGMENTS;k++) {
      Schedules[j].Segments[k].SoakTime = 20;
    }
    // state
    for (int k=0;k<NUMBER_OF_SEGMENTS;k++) {
      Schedules[j].Segments[k].State = SEGMENT_STATE_IDLE;
    }
  }
}

// Custom function accessible by the API
// any function exposed to the API must accept a string and return an int
// this function can be called using the following:
// http://ipaddress/function01?params=99
int function01(String command) {

  // Get integer from command
  //int integer = command.toInt();
  int integer = HEARTBEAT_VALUE;
  
  return integer;

}

// Custom function accessible by the API
// any function exposed to the API must accept a string and return an int
// http://ipaddress/function02?params=99
int function02(String command) {

  // Get integer from command
  //int integer = command.toInt();
  int integer = (int)Output_01;
  
  return integer;

}

// Custom function accessible by the API
// any function exposed to the API must accept a string and return an int
// http://ipaddress/function03?params=99
int function03(String command) {

  // Get integer from command
  //int integer = command.toInt();
  int integer = (int)temperature_ch0;
  
  return integer;

}
