#ifndef _UTILS_
#define _UTILS_

#include <arduino.h>
#include <Stdio.h>

#define kMILLI  0.001
#define kMICRO  0.000001

void bin_read(int offset,int len, byte *bptr);
void bin_write(int offset, int len, byte *bptr);

class DigitalLed {
  private:
    byte _pin;
    boolean _reverse;
    boolean _state;
  public:
    DigitalLed(byte pin,boolean state,boolean reverse);
    void set(boolean);
};

class Button {
  private:
    byte _pin;
    boolean _state;
    boolean _changed;
    unsigned long _t;
  public:
    Button(byte pin);
    void update();
    boolean changed();
    boolean get();
} ;

class Timer {
  private:
    unsigned long _last_t;    // current timer in ms (copy of millis() or micros()), will overflow after 50 days
    unsigned long _delta_t;   // timer from last update in ms
    float _coef;              // coef between raw and seconds timer (milli()=0.001, micro=0.000001)
    float _tsec;              // precise seconds timing, only the last second
    unsigned long _rsec;      // seconds elapsed (integer positive value) : no overflow
   
  public:
    Timer();
    void update();
    unsigned long get_sec();
    unsigned long get_raw();
    unsigned long get_dt();
    float get_precise();
    void display();
    void set_precise(float);
};

#endif
