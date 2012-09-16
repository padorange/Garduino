// Garduino
// Supervisor for an autonomous GrowBox using an Arduino
// ---------------------------------------------------------------------------
// Supervise 4 main handlers (sensor and driver) :
// - Temperature : measured by a thermistor and drive by a fan
// - Moisture : measured by a diy soil resistance and drive by a water pump
// - Water Level : measured by a diy resistance, drive via Moisture sensor
// - Light : measured by a LDR and drive by powerled
// Use some LED to display operations and buttons for user interface
// ---------------------------------------------------------------------------
// Open Hardware and Open Source Software : BSD and cc-by-sa licences
// tested on Arduino Uno R3 and Arduino IDE 1.0.1
// author : Pierre-Alain Dorange
//
// history :
// 02a1, 5-08-2012 : rewrote for a no-interruptive measuring
// 02a2, 12-08-2012 : timer debug for no overflow, utils and handlers became class
// 02a4, 15-08-2012 : memorize values in handler, mean'em, store last max and drive PUMP for watering
// 02a5, 24-08-2012 : use EPPROM to store moisture max/min, send status and power to serial, fix some bug with watering delay

#include <Stdio.h>
#include <EEPROM.h>
#include "utils.h"    // some utilities : button, led & timer
#include "handler.h"  // handler for analog sensor and their respective drive

// -- CONSTANTS ------------------------------
const int kSerialSpeed=9600;

// digital input-output pin assignement
const int kLed0=3;
const int kLed1=5;
const int kLed2=6;
const int kWaterLevelSensorDrivePin=4;
const int kMoistureSensorDrivePin=8;
const int kFanDrivePin=10;
const int kWaterPumpDrivePin=11;
const int kPowerLedDrivePin=12;

// analog input pin assignement
const int kTemperatureAnalogPin=0;
const int kLightAnalogPin=1;
const int kLevelAnalogPin=2;
const int kMoistAnalogPin=3;

// sensor/drive constants
const int   kWaterStandardDelay=300;
const int   kWaterWateringDelay=5;
const float kWaterFull=25.0;
const unsigned long   kWateringTimeOut=20;
const byte  kPumpPower=128;
const int   kLightDelay=300;
const float kLightMin=60.0;
const int   kTemperatureDelay=60;
const float kTemperatureMax=30.0;
const float kTemperatureMin=18.0;
const int   kMoistureDelay=300;
const float kMoistureWaterCoef=0.05;

// -- GLOBALS ------------------------------

// timer : handle time in seconds (integer) with no overflow
Timer     timer=Timer();
// buttons : handle user interact
Button      b1=Button(2),  // manual measure
            b2=Button(7);  // force water pump
            
// leds : handle user informations (digital pin, reverse mode, state)
DigitalLed  led_temp=DigitalLed(kLed0,LOW,false), 
            led_light=DigitalLed(kLed1,LOW,false), 
            led_moist=DigitalLed(kLed2,LOW,false);
            
// data handlers (analog pin, digital input drive pin, digital output drive pin)
Handler   h_temp=Handler(kTemperatureAnalogPin,-1,kFanDrivePin),
          h_light=Handler(kLightAnalogPin,-1,kPowerLedDrivePin),
          h_moist=Handler(kMoistAnalogPin,kMoistureSensorDrivePin,kWaterPumpDrivePin),
          h_water=Handler(kLevelAnalogPin,kWaterLevelSensorDrivePin,-1);

float moist_max_value=0.0;
float moist_min_value=0.0;
unsigned long moist_max_time=0;
unsigned long water_timeout=0;
boolean water_force=false;
boolean temp_status,light_status,moist_status,water_status;

// -- CODE ------------------------------

// Read parameters from EEPROM
// EEPROM is a memory that is not erased on Arduino reset
// We store important value that need to be kept
void read_parameters() {
  float *fptr;
  byte id,data[4];
  char str[50];
  
  id=EEPROM.read(0);
  Serial.println(F("Get Parameters (EEPROM)"));
  if (id!=1) {
    Serial.println(F("EEPROM never used : INITIALIZE"));
    moist_max_value=76.70;
    moist_min_value=74.14;
    write_parameters();
 }
  else {
    bin_read(1,4,data);
    fptr=(float *)data;
    moist_max_value=(*fptr);
    bin_read(5,4,data);
    fptr=(float *)data;
    moist_min_value=(*fptr);
  }
  sprintf(str,"* Max Moisture: %0d.%02d, Min target: %0d.%02d",(int)moist_max_value,(int)(100*(moist_max_value-(int)moist_max_value)),(int)moist_min_value,(int)(100*(moist_min_value-(int)moist_min_value)));
  Serial.println(str);
}

// write parameters into EEPROM
void write_parameters() {
  byte id;
  
  id=1;
  bin_write(0,1,&id);
  bin_write(1,4,(byte *)(&moist_max_value));
  bin_write(5,4,(byte *)(&moist_min_value));
}

// Arduino SetUp : called a start-up (or on reset)
void setup() {
  Serial.begin(kSerialSpeed);
  analogReference(DEFAULT);
  
  // prepare the timer (in seconds without overflow)
  timer.set_precise(kMILLI);
  timer.update();

  // configure sensor handlers
  h_moist.set_delay(kMoistureDelay);          // set time between two measure
  h_moist.set_name(String("Moisture"),'M');   // set name and code
  h_moist.set_led(&led_moist);                // link to a LED Handler
  h_moist.set_corrector(&h_temp);             // set the corrector measure (if necessary)
  h_moist.set_next(timer.get_sec());          // set the time for the first measure

  h_temp.set_delay(kTemperatureDelay);
  h_temp.set_name(String("Temperature"),'T');
  h_temp.set_led(&led_temp);
  h_temp.set_next(timer.get_sec()+2);

  h_light.set_delay(kLightDelay);
  h_light.set_name(String("Light"),'L');   
  h_light.set_led(&led_light);
  h_light.set_next(timer.get_sec()+3);

  h_water.set_delay(kWaterStandardDelay);
  h_water.set_name(String("WaterLevel"),'W');
  h_water.set_led(&led_moist);
  h_water.set_corrector(&h_temp);
  h_water.set_next(timer.get_sec()+10);
  
  // make led blink : i'm alive
  led_temp.set(HIGH);
  led_light.set(HIGH);
  led_moist.set(HIGH);
  delay(500);
  led_temp.set(LOW);
  led_light.set(LOW);
  led_moist.set(LOW);
   
  while (Serial==false) { ; }
  Serial.println(F("-- Garduino -------"));
  
  temp_status=false;
  light_status=false;
  moist_status=false;
  water_status=false;
  
  read_parameters();
}

// when reaching a new max for moisture, store it (into EEPROM) and the new min target 
// the min target is used as a goal value to activate water pump
void set_moist_max() {
  char str[50];
  
  moist_max_value=h_moist.get_value();
  moist_max_time=h_moist.get_time();
  moist_min_value=(1.0-kMoistureWaterCoef)*moist_max_value;
  write_parameters();
  sprintf(str,"* Max Moisture: %0d.%02d, Min target: %0d.%02d",(int)moist_max_value,(int)(100*(moist_max_value-(int)moist_max_value)),(int)moist_min_value,(int)(100*(moist_min_value-(int)moist_min_value)));
  Serial.println(str);
}

// the Arduino main loop : called every cycle
void loop() {
  float v;
 
  timer.update();

  // update temperature sensor and handle it
  if (h_temp.update(timer)) {   
    v=h_temp.get_average();
    if (v>kTemperatureMax) {  // if temperature too high : activate fan
      temp_status=true;
      h_temp.set_power(HIGH);
    }
    else {  // else stop fan
      h_temp.set_power(LOW);
      temp_status=false;
    }
    h_temp.display(temp_status);
  }
  
  // update light sensor and handle it
  if (h_light.update(timer)) {
    v=h_light.get_average();
    if (v<kLightMin) {      // if not enough light, switch on LED
      light_status=true;
      h_light.set_power(HIGH);
    }
    else {  // else stop light
      light_status=false;
      h_light.set_power(LOW);
    }
    h_light.display(light_status);
  }
  
  // update water level sensor and handle it
  if (h_water.update(timer)) {
    float v;
    
    v=h_water.get_value();
    if (v>kWaterFull)    // just activate/deactivate status accoring to Water level in the reservoir
      water_status=true;
    else
      water_status=false;
    h_water.display(water_status);
  }
  
  // if water pump is active, check to stop it (timeout of water level) ** SECURITY **
  if (h_moist.get_power()) { 
    if ((timer.get_sec()>water_timeout)||(h_water.get_value()>kWaterFull)) {
      char str[50];
      
      h_moist.set_power(0);
      sprintf(str,"* Stop watering after %lu sec(s)",kWateringTimeOut+timer.get_sec()-water_timeout);
      Serial.println(str);
      set_moist_max();
      h_water.set_delay(kWaterStandardDelay);  // restore default water sensor delay
      h_water.display(water_status);
    }
  }
  // update moisture sensor and handle it
  if (h_moist.update(timer)) {
    v=h_moist.get_value();
    if (v>moist_max_value) set_moist_max();  // new maximum value, update new min target
    if (!h_moist.get_power()) {  // Water pump is OFF...
      if (v<moist_min_value) {  // ...and we are below min target
        moist_status=true;
        if (h_water.get_value()>kWaterFull) {  // check for water level : do not add water if not empty and reset max/min
          Serial.println(F("* Need Water, but reservoir is FULL"));
          h_moist.set_power(0);
          h_water.set_delay(kWaterStandardDelay);  // reset normal delay
         
          set_moist_max();
        }
        else { // all is ok : soil need water and reservoir is empty : drive water pump !
          char str[60];
          h_moist.set_power(kPumpPower);
          water_timeout=timer.get_sec()+kWateringTimeOut;
          h_water.set_delay(kWaterWateringDelay);  // speed-up water level sensor
          sprintf(str,"* Start Watering at %lu for %lu secs (target %lu)",timer.get_sec(),kWateringTimeOut,water_timeout);
          Serial.println(str);
        }
        h_water.set_next(timer.get_sec());
     }
      else
        moist_status=false;
    }
    h_moist.display(moist_status);
  }

  b1.update();  // handle button 1 : activate a new measure for each handler
  if (b1.changed()) {
    if (b1.get()) {
      timer.display();
      h_moist.set_next(timer.get_sec());
      h_temp.set_next(timer.get_sec()+2);
      h_light.set_next(timer.get_sec()+3);
      h_water.set_next(timer.get_sec()+10);
    }
  }
  b2.update();  // handle button 2 : activate/deactivate manually the water pump
  if (b2.changed()) {
    if (b2.get()) {
      Serial.print(F("PUMP : Manual ON, "));
      timer.display();
      h_moist.set_power(kPumpPower);
      water_timeout=timer.get_sec()+kWateringTimeOut;
      water_force=true;
      h_water.set_delay(kWaterWateringDelay);
      h_water.set_next(timer.get_sec());
    }
    else {
      Serial.print(F("PUMP : Manual OFF, "));
      timer.display();
      h_moist.set_power(0);
      water_force=false;
      h_water.set_delay(kWaterStandardDelay);
    }
  }
}
