#include<Arduino.h>

const int PRESSURE_SENSOR_PIN = A0;
const int ONSET_THERESHOLD = 200;  //閾値の暫定値

void pressure_init() {
  pinMode(PRESSURE_SENSOR_PIN, INPUT);
}

uint8_t pressure_read() {
  int raw_value = analogRead(PRESSURE_SENSOR_PIN);
  uint8_t frog_state = 0;
  
  if(raw_value > ONSET_THERESHOLD){
    frog_state = 1;
  } else {
    frog_state = 0;
  }

  return frog_state;
}