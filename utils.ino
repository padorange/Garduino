#include <arduino.h>
#include "utils.h"

// basic binary read into EEPROM
void bin_read(int offset,int len, byte *bptr) {
  for(int i=0;i<len;i++)
    bptr[i]=EEPROM.read(i+offset);
}

// basic binary write into EEPROM
void bin_write(int offset, int len, byte *bptr) {
  for(int i=0;i<len;i++)
    EEPROM.write(i+offset,bptr[i]);
}

// DigitalLED

DigitalLed::DigitalLed(byte pin,boolean s,boolean r) {
  _pin=pin;
  _state=s;
  _reverse=r;
  pinMode(_pin,OUTPUT);
  set(_state);
}

void DigitalLed::set(boolean state) {
  if (_reverse)
    _state=(!state);
  else
    _state=state;
  digitalWrite(_pin,_state);
}

// Button

Button::Button(byte pin) {
  _pin=pin;
 _t=0;
  pinMode(_pin,INPUT);
}

void Button::update() {
  boolean s;
  
  s=digitalRead(_pin);
  if (_state!=s) {
    unsigned long t0;
    
    t0=millis();
    if ((t0-_t)>50) {  // debounce 50 ms
      _changed=true;
      _state=s;
      _t=t0;
    }
  }
  else
    _changed=false;
}

boolean Button::changed() {
  return _changed;
}

boolean Button::get() {
  return _state;
}

// Timer
// _last_t is the last call to raw timing measure (milli of micro)
// _tsec keep last seconds precisly (with float)
// _rsec is the interger rounded seconds elapsed since timer start

Timer::Timer() {
  _delta_t=0;
  _tsec=0.0;
  _coef=kMILLI;
  _rsec=0;
  if (_coef==kMILLI)
    _last_t=millis();
  else
    _last_t=micros();
}

void Timer::set_precise(float p) {
  if (p==kMILLI) _coef=p;
  else
    if (p==kMICRO) _coef=p;
    else _coef=kMILLI;

  _delta_t=0;
  _tsec=0.0;
  _rsec=0;
  if (_coef==kMILLI)
    _last_t=millis();
  else
    _last_t=micros();
}

float Timer::get_precise() {
   return _coef;
}

void Timer::update() {
  unsigned long t0;
  float  dt,v;
  
   if (_coef==kMILLI)
    t0=millis();
  else
    t0=micros();
  _delta_t=t0-_last_t;
  dt=_coef*_delta_t;
/*  if (_last_t>t0) {
    Serial.print("Timer uint overflow : t0=");
    Serial.print(t0);
    Serial.print(" last=");
    Serial.print(_last_t);
    Serial.print(" delta=");
    Serial.print(_delta_t);
    Serial.print(" dt=");
    Serial.println(dt,8);
  }*/
  _last_t=t0;
  v=_tsec+dt;
/*  if (2.0*abs(v-_tsec)<dt) {
    Serial.print("Timer float overflow : t+=");
    Serial.print(_tsec,8);
    Serial.print(" t+dt=");
    Serial.print(v,8);
    Serial.print(" dt=");
    Serial.println(dt,8);
  }*/
  _tsec=v;
  // adjust _tsec and _rsec to avoid float mistake computation
  while (_tsec>1.0) {
    _tsec-=1.0;
    _rsec+=1;
  }
}

unsigned long Timer::get_sec() {
  return _rsec;
}

unsigned long Timer::get_raw() {
  return _last_t;
}

unsigned long Timer::get_dt() {
  return _delta_t;
}

void Timer::display() {
  char str[40];
  
  sprintf(str,"Timer: %lu sec(s)",_rsec);
  Serial.println(str);
}

