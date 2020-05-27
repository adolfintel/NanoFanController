/**
 * Arduino Nano based PWM computer fan controller.
 * Supports up to 3 PWM fans with individual curves on pins 9,10,3
 * 
 * Developed by Federico Dossena
 * License: GNU GPLv3
 */

#include <avr/wdt.h>
#include <EEPROM.h>

#define SENSOR A7 //Change this to A0 if using an Arduino Uno

const char MAGIC[]="FANC3";

float SENSOR_CALIBRATION=7.55f,
      TEMP_MIN_1=30.0f, TEMP_MAX_1=50.0f, CURVE_1=0.7f,
      TEMP_MIN_2=30.0f, TEMP_MAX_2=50.0f, CURVE_2=0.85f,
      TEMP_MIN_3=30.0f, TEMP_MAX_3=55.0f, CURVE_3=0.75f;
bool debugging=false, manual=false;
float temp=0,f1=0,f2=0,f3;
uint16_t raw=0;
unsigned long ts=0;

void printConfig(){
  Serial.print(SENSOR_CALIBRATION);
  Serial.print(F(" "));
  Serial.print(TEMP_MIN_1);
  Serial.print(F(" "));
  Serial.print(TEMP_MAX_1);
  Serial.print(F(" "));
  Serial.print(CURVE_1);
  Serial.print(F(" "));
  Serial.print(TEMP_MIN_2);
  Serial.print(F(" "));
  Serial.print(TEMP_MAX_2);
  Serial.print(F(" "));
  Serial.print(CURVE_2);
  Serial.print(F(" "));
  Serial.print(TEMP_MIN_3);
  Serial.print(F(" "));
  Serial.print(TEMP_MAX_3);
  Serial.print(F(" "));
  Serial.println(CURVE_3);
}

void printStatus(){
  Serial.print(F("Raw:"));
  Serial.print(raw);
  Serial.print(F(",Temp:"));
  Serial.print(temp);
  Serial.print(F(",Fan1:"));
  Serial.print(f1*100.0f);
  Serial.print(F(",Fan2:"));
  Serial.print(f2*100.0f);
  Serial.print(F(",Fan3:"));
  Serial.println(f3*100.0f);
}

bool loadSettings(){
  if(debugging) Serial.print(F("Loading settings... "));
  uint16_t eadr=0;
  auto eepromReadFloat=[&](){
      float f;
      EEPROM.get(eadr,f);
      eadr+=sizeof(f);
      return f;
  };
  bool magicOk=true;
  char m[sizeof(MAGIC)];
  EEPROM.get(eadr,m);
  if(strcmp(m,MAGIC)!=0){
    if(debugging) Serial.println(F("default"));
    return false;
  }
  eadr+=sizeof(MAGIC);
  SENSOR_CALIBRATION=eepromReadFloat();
  TEMP_MIN_1=eepromReadFloat();
  TEMP_MAX_1=eepromReadFloat();
  CURVE_1=eepromReadFloat();
  TEMP_MIN_2=eepromReadFloat();
  TEMP_MAX_2=eepromReadFloat();
  CURVE_2=eepromReadFloat();
  TEMP_MIN_3=eepromReadFloat();
  TEMP_MAX_3=eepromReadFloat();
  CURVE_3=eepromReadFloat();
  EEPROM.get(eadr,debugging);
  eadr+=sizeof(debugging);
  if(debugging){
    Serial.println(F("loaded"));
    printConfig();
  }
  return true;
}

void saveSettings(){
  if(debugging) Serial.print(F("Saving settings... "));
  uint16_t eadr=0;
  auto eepromWriteFloat=[&](float f){
    EEPROM.put(eadr,f);
    eadr+=sizeof(f);
  };
  EEPROM.put(eadr,MAGIC);
  eadr+=sizeof(MAGIC);
  eepromWriteFloat(SENSOR_CALIBRATION);
  eepromWriteFloat(TEMP_MIN_1);
  eepromWriteFloat(TEMP_MAX_1);
  eepromWriteFloat(CURVE_1);
  eepromWriteFloat(TEMP_MIN_2);
  eepromWriteFloat(TEMP_MAX_2);
  eepromWriteFloat(CURVE_2);
  eepromWriteFloat(TEMP_MIN_3);
  eepromWriteFloat(TEMP_MAX_3);
  eepromWriteFloat(CURVE_3);
  EEPROM.put(eadr,debugging);
  eadr+=sizeof(debugging);
  if(debugging) Serial.println(F("done"));
}


void setup() {
  pinMode(LED_BUILTIN,OUTPUT);
  digitalWrite(LED_BUILTIN,HIGH);
  wdt_enable(WDTO_2S);
  Serial.begin(9600);
  if(debugging) Serial.println(F("FAN CONTROLLER"));
  pinMode(9,OUTPUT);
  pinMode(10,OUTPUT);
  pinMode(3,OUTPUT);
  loadSettings();
  //Set PWM frequency to about 25khz on pins 9,10 (timer 1 mode 10, no prescale, count to 320)
  TCCR1A = (1 << COM1A1) | (1 << COM1B1) | (1 << WGM11);
  TCCR1B = (1 << CS10) | (1 << WGM13);
  ICR1 = 320;
  //Set PWM frequency to about 25khz on pin 3 (timer 2 mode 5, prescale 8, count to 79)
  TIMSK2 = 0;
  TIFR2 = 0;
  TCCR2A = (1 << COM2B1) | (1 << WGM21) | (1 << WGM20);
  TCCR2B = (1 << WGM22) | (1 << CS21);
  OCR2A = 79;
  //startup pulse
  if(debugging) Serial.print(F("Startup pulse... "));
  OCR1A = 320;
  OCR1B = 320;
  OCR2B = 79;
  delay(1000);
  OCR1A = 0;
  OCR1B = 0;
  OCR2B = 0;
  if(debugging) Serial.println(F("done"));
  digitalWrite(LED_BUILTIN,LOW);
}

void loop() {
  wdt_reset();
  while(Serial.available()){
    switch(Serial.read()){
      case 'p':printStatus(); break;
      case 'g':printConfig(); break;
      case 's':{
        SENSOR_CALIBRATION=Serial.parseFloat();
        TEMP_MIN_1=Serial.parseFloat();
        TEMP_MAX_1=Serial.parseFloat();
        CURVE_1=Serial.parseFloat();
        TEMP_MIN_2=Serial.parseFloat();
        TEMP_MAX_2=Serial.parseFloat();
        CURVE_2=Serial.parseFloat();
        TEMP_MIN_3=Serial.parseFloat();
        TEMP_MAX_3=Serial.parseFloat();
        CURVE_3=Serial.parseFloat();
        saveSettings();
      }break;
      case 'd':{
        Serial.println(F("Debug OFF"));
        debugging=false;
        saveSettings();
      }break;
      case 'D':{
        Serial.println(F("Debug ON"));
        debugging=true;
        saveSettings();
      }break;
      case 'm':{
        manual=false;
      }break;
      case 'M':{
        manual=true;
        temp=Serial.parseFloat();
      }break;
      case 'R':{
        if(debugging){Serial.println(F("Resetting...")); delay(100);}
        void(*r)()=0;r();
      }break;
      default: break;
    }
  }
  if(ts==0||millis()-ts>=1000){
    raw=analogRead(SENSOR);
    if(!manual) temp=(float)raw/SENSOR_CALIBRATION;
    f1=(float)(temp-TEMP_MIN_1)/(TEMP_MAX_1-TEMP_MIN_1);
    f2=(float)(temp-TEMP_MIN_2)/(TEMP_MAX_2-TEMP_MIN_2);
    f3=(float)(temp-TEMP_MIN_3)/(TEMP_MAX_3-TEMP_MIN_3);
    f1=pow(f1<0?0:f1>1?1:f1,CURVE_1);
    f2=pow(f2<0?0:f2>1?1:f2,CURVE_2);
    f3=pow(f3<0?0:f3>1?1:f3,CURVE_3);
    OCR1A = (uint16_t)(320*f1); //set PWM width on pin 9
    OCR1B = (uint16_t)(320*f2); //set PWM width on pin 10
    OCR2B = (uint8_t)(79*f3); //set PWM width on pin 3
    if(debugging) printStatus();
    ts=millis();
  }else delay(100);
}
