#include <arduino.h>
#include "utils.h"
//#define _DEBUG_

Handler::Handler(int apin,int dipin, int dopin) {
  _analogInput=apin;
  _driveInput=dipin;
  _driveOutput=dopin;
  if (_driveInput>=0)
    pinMode(_driveInput,OUTPUT);
  if (_driveOutput>=0)
    pinMode(_driveOutput,OUTPUT);
  _next_s=0.0;
  _v=0;
  _nb_value=10;
  _index_value=-1;
  _value=(float *)malloc(sizeof(float)*_nb_value);
  _nb=-1;
  _led=NULL;
  _corrector=NULL;
  _nb_mes=20;
  _d_mes=0.01;
  _power=LOW;
}

void Handler::add_value(Timer timer) {
  float v,t;
  
  _seconds=timer.get_sec();
  switch (_code) {
    case 'T': // simple conversion using a linear approximation (coef by experimentation)
      v=100.0*0.88383*_raw_value/1024.0-18.29787;
      break;
    case 'M':
      if (_corrector==NULL)
        t=0.0;
      else
        t=_corrector->get_average();
      v=(100.0*_raw_value/1024.0)-0.2*t;
      break;
    case 'W':
      v=100.0*_raw_value/1024.0;
      break;
    default:
      v=100.0*_raw_value/1024.0;
  }
  if (_value!=NULL) {
    _index_value++;
    if (_index_value>=_nb_value) {
      _index_value=_nb_value-1;
      for(int i=0;i<_nb_value-1;i++)
        _value[i]=_value[i+1];
    }
    _value[_index_value]=v;
  }

#ifdef _DEBUG_
  Serial.print("    Handler ");
  Serial.print(_analogInput);
  Serial.print(" add index ");
  Serial.print(_index_value);
  Serial.print(" :");
  for(int i=0;i<_nb_value-1;i++) {
    Serial.print(" ");
    Serial.print(_value[i],2);
  }
  Serial.println("");
#endif
}

float Handler::get_value() {
  if ((_index_value<0)||(_index_value>=_nb_value)||(_value==NULL)) 
    return 0.0;
  else
    return _value[_index_value];
}

unsigned long Handler::get_time() {
  return _seconds;
}

float Handler::get_average() {
  float v;
  
  if ((_nb_value==0)||(_value==NULL)) return 0.0;
  v=0.0;
  for(int i=0;i<=_index_value;i++)
    v+=_value[i];
  v=v/(_index_value+1);
  return v;
}

boolean Handler::update(Timer t) {
  boolean done=false;
  
  // check timer to start (or not) a measure
  if ((t.get_sec()>_next_s)&&(_nb<0)) {
#ifdef _DEBUG_
    Serial.print("    Handler Call ");
    Serial.print(_analogInput);
    Serial.print(" at ");
    Serial.println(t.get_sec(),2);
#endif
    _nb=0;  // initialize measure counter to start process
    _v=0;   // cumulative value reset
    _next_it=t.get_raw();  // next measure : now
    _raw_value=0;
    _lrv=-1; // prepare for measure stability 
    _next_blink=t.get_raw()+kDefaultBlink;  // prepare led for blinking during measure
    if (_power==LOW)
      _blink_state=HIGH;
    else
      _blink_state=LOW;
  }
  
  // if a measure is started, handle it
  if (_nb>=0) {
    if (_driveInput>=0)  // if measure driven, start drive pin
      digitalWrite(_driveInput,HIGH);
    if (t.get_raw()>_next_it) {  // do a mean measure every d_mes
      _v+=analogRead(_analogInput);
      _nb+=1;
      _next_it=t.get_raw()+(_d_mes/t.get_precise());
    }
    if (_nb>=_nb_mes) {   // after nb_mes measures mean result (we've finished)
      boolean stable=false;
      
      _raw_value=_v/_nb;
      _v=0;
      if (_driveInput>=0) {  // if measure drive, check for stability (avoid capacitance effect)
        if (abs(_raw_value-_lrv)<1) {
          if (t.get_raw()-_lt>1.0) {
            stable=true;
#ifdef _DEBUG_
            Serial.print("    Handler ");
            Serial.print(_analogInput);
            Serial.print(" total stabilize: ");
            Serial.println(t.get_sec()-_next_s,2);
#endif
          }
#ifdef _DEBUG_
          else
          {
            Serial.print("    Handler ");
            Serial.print(_analogInput);
            Serial.print(" wait stabilize, value: ");
            Serial.print(_raw_value);
            Serial.print(" for ");
            Serial.println(t.get_raw()-_lt,2);
          }
#endif
        }
        else {
#ifdef _DEBUG_
            Serial.print("    Handler ");
            Serial.print(_analogInput);
            Serial.print(" unstabilized: ");
            Serial.println(_raw_value);
#endif
          _lrv=_raw_value;
          _lt=t.get_raw();
        }
      }
      else
        stable=true;
        
      if (t.get_sec()-_next_s>5.0)  // timeout (5 seconds) if can't stabilize
        stable=true;
        
      if (stable) {
        if (_driveInput>=0)  // if measure driven, stop drive pin
          digitalWrite(_driveInput,LOW);
        add_value(t); // store value and time
        done=true;
        _nb=-1;  // stop measure
        _next_s=t.get_sec()+_dt;
#ifdef _DEBUG_
        Serial.print("    Handler ");
        Serial.print(_analogInput);
        Serial.print(", value: ");
        Serial.print(_raw_value);
        Serial.print(" at ");
        Serial.print(_seconds);
        Serial.print(", next call at ");
        Serial.println(_next_s);
#endif
      }
      else {
        _nb=0;  // redo measure (not stable)
      }
    }
  }
  // handle LED : ON when power is ON and blinking during measure
  if (_led!=NULL) { // set LED if necessary
    if (_nb<0) {  // no measure, adujst LED according to Power
      if (_power==LOW)
        _blink_state=LOW;
      else
        _blink_state=HIGH;
    }
    else {  // measuring is on going : blink led
      if (t.get_raw()>_next_blink) {
        if (_blink_state==LOW)
          _blink_state=HIGH;
        else
          _blink_state=LOW;
        _next_blink=t.get_raw()+kDefaultBlink;
      }  
    }
    _led->set(_blink_state);
  }
  
  return done;
}

void Handler::set_delay(int d) {
  _dt=d;
}

void Handler::set_name(String name,char code) {
    _name=name;
    _code=code;
}

void Handler::set_led(DigitalLed *led) {
  _led=led;
}

void Handler::set_corrector(Handler *corr) {
  _corrector=corr;
}

void Handler::set_next(unsigned long t) {
  _next_s=t;
}

void Handler::set_power(byte p) {
  if (_driveOutput>=0) {
    _power=p;
    analogWrite(_driveOutput,_power);
  }
  else
    _power=0;
}

boolean Handler::get_power() {
  return _power;
}

void Handler::display(boolean s) {
  char str[40];
  float v,m;
  int sd,pd;

  if (s)
    sd=1;
  else
    sd=0;  
  if (_power)
    pd=1;
  else
    pd=0;  
  v=get_value();
  m=get_average();
  
  sprintf(str,"> %c = %0d.%02d @ %lu, mean: %0d.%02d status=%d power=%d",_code,(int)v,(int)(100*(v-(int)v)),get_time(),(int)m,(int)(100*(m-(int)m)),sd,pd);
  Serial.println(str);
}

