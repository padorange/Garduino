
#ifndef _HANDLER_
#define _HANDLER_

#include <arduino.h>
#include <Stdio.h>
#include "utils.h"

#define kDefaultBlink  100

class Handler {
  private:
    int _analogInput;  // analog input pin
    int _driveInput;   // digital output pin to drive analog (-1 if no drive pin)
    int _driveOutput;  // output digital pin to drive something related to measure
    unsigned long _dt;          // delay (in seconds) between 2 measures
    String _name;
    char _code;
    unsigned long _next_it;      // next timer for an intermediate measure
    unsigned long _next_s;      // next timer for a fresh measure
    int _nb,_v;          // nb measure and cumulative value
    int _lrv;
    unsigned long _lt;
    int _nb_mes;
    float _d_mes;
    int _raw_value;     // last raw value
    int _nb_value,_index_value;
    float *_value;       // last values (raw value converted into human readable measure)
    unsigned long _seconds;    // last time associated to value
    DigitalLed *_led;  // pointer to a led (or not = NULL)
    Handler *_corrector;  // pointer to a corrector (or not = NULL)
    boolean _power;
    byte _blink_state;
    unsigned long _next_blink;
    
  public:
    Handler(int apin,int dipin, int dopin);
    boolean update(Timer);
    void add_value(Timer);
    float get_average();   
    float get_value();   
    unsigned long get_time();   
    void set_delay(int);
    void set_name(String,char);
    void set_led(DigitalLed *);
    void set_corrector(Handler *);
    void set_next(unsigned long);
    void set_power(byte);
    boolean get_power();
    void display(boolean);
} ;

#endif
