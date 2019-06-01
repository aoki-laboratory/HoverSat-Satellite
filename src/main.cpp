//------------------------------------------------------------------//
//Supported MCU:   ESP32 (M5Stack)
//File Contents:   HoverSat Satellite
//Version number:  Ver.1.0
//Date:            2019.05.13
//------------------------------------------------------------------//
 
//This program supports the following boards:
//* M5Stack(Gray version)
 
//Include
//------------------------------------------------------------------//
#include <M5Stack.h>
#include <Servo.h>
#include "utility/MPU9250.h"
#include <Wire.h>
#include "BluetoothSerial.h"


//Define
//------------------------------------------------------------------//
#define   TIMER_INTERRUPT   10 // ms
#define   LCD
#define   STEPMOTOR_I2C_ADDR 0x70
// #define  STEPMOTOR_I2C_ADDR 0x71

#define HX711_DOUT  2
#define HX711_SCLK  5
#define OUT_VOL     0.0007f
#define LOAD        500.0f



//Global
//------------------------------------------------------------------//
int     pattern = 0;
bool    hover_flag = false;

byte    counter;

BluetoothSerial bts;

// MPU9250
MPU9250 IMU; 

// HX711
float hx711_offset;
float hx711_data;

// DuctedFan
static const int DuctedFanPin = 15;
unsigned char hover_val = 0;
Servo DuctedFan;

// Timer Interrupt
volatile int interruptCounter;
int totalInterruptCounter;
int iTimer10;

hw_timer_t * timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;


//Global
//------------------------------------------------------------------//
void AE_HX711_Init(void);
void AE_HX711_Reset(void);
long AE_HX711_Read(void);
long AE_HX711_Averaging(long adc,char num);
float AE_HX711_getGram(char num);

void IRAM_ATTR onTimer(void);
void SendByte(byte addr, byte b);
void SendCommand(byte addr, char *c);
void Timer_Interrupt( void );


//Setup
//------------------------------------------------------------------//
void setup() {

  M5.begin();
  Wire.begin();
  M5.Lcd.clear();
  M5.Lcd.drawJpgFile(SD, "/Image/Picture.jpg");
  M5.Lcd.setTextColor(BLACK);
  M5.Lcd.setTextSize(3);
  M5.Lcd.setCursor(88, 160);
  M5.Lcd.println("HoverSat");
  M5.Lcd.setCursor(82, 200);
  M5.Lcd.println("Sattelite");
  
  delay(3000);

  bts.begin("M5Stack");

  
  // Initialize Timer Interrupt
  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, TIMER_INTERRUPT * 1000, true);
  timerAlarmEnable(timer);

  M5.Lcd.setTextSize(3);
  M5.Lcd.setTextColor(GREEN ,BLACK);
  M5.Lcd.fillScreen(BLACK);

  AE_HX711_Init();
  AE_HX711_Reset();
  hx711_offset = AE_HX711_getGram(30); 
  
}


//Main
//------------------------------------------------------------------//
void loop() {

  Timer_Interrupt();

  
  switch (pattern) {
    case 0:
      break;

    case 11:
      break;
    
  }


  bts.print(interruptCounter);
  bts.print(", ");
  bts.println(hx711_data);
  
  // Get Data from Module.
  Wire.requestFrom(STEPMOTOR_I2C_ADDR, 1);
  if (Wire.available() > 0) {
    int u = Wire.read();
    if (u != 0) Serial.write(u);
  }
  delay(1);
  // Send Data to Module.
  while (Serial.available() > 0) {
    int inByte = Serial.read();
    SendByte(STEPMOTOR_I2C_ADDR, inByte);
  }
      
  // Button Control
  M5.update();
  if (M5.BtnA.wasPressed()) {
    hover_flag = !hover_flag;
    // Hover Control
    if(hover_flag) {
      M5.Lcd.clear();
      DuctedFan.attach(DuctedFanPin);
      DuctedFan.write(0);
    } else {
      M5.Lcd.clear();
      DuctedFan.detach();
      hover_val = 0;
    }
  } else if (M5.BtnB.wasPressed()) {
      if(hover_val<100) hover_val+=5;
      DuctedFan.write(hover_val);
  } else if (M5.BtnC.wasPressed()) {
      if(hover_val>0) hover_val-=5;
      DuctedFan.write(hover_val);
  } else if (M5.BtnA.pressedFor(700)) {
    SendCommand(STEPMOTOR_I2C_ADDR, "G1 X10 F500");
  }  else if (M5.BtnB.pressedFor(700)) {
    SendCommand(STEPMOTOR_I2C_ADDR, "G1 X-10 F500");
  }

}


// Timer Interrupt
//------------------------------------------------------------------//
void Timer_Interrupt( void ){
  if (interruptCounter > 0) {

    portENTER_CRITICAL(&timerMux);
    interruptCounter--;
    portEXIT_CRITICAL(&timerMux);

    totalInterruptCounter++;

    iTimer10++;
    switch( iTimer10 ) {
    case 1:
      if(hover_flag) {
        M5.Lcd.fillScreen(BLACK);
        M5.Lcd.setCursor(140, 105);
        M5.Lcd.println(hover_val);
      } else {
        M5.Lcd.setCursor(100, 105);
        M5.Lcd.println("Disable");
      }
    break;

    case 2:
      hx711_data = AE_HX711_getGram(5) - hx711_offset;
      break;
    
    case 10:
      iTimer10 = 0;
      break;

    }

  }
}


// IRAM
//------------------------------------------------------------------//
void IRAM_ATTR onTimer() {
  portENTER_CRITICAL_ISR(&timerMux);
  interruptCounter++;
  portEXIT_CRITICAL_ISR(&timerMux);
 }

  
//SendByte
//------------------------------------------------------------------//
void SendByte(byte addr, byte b) {
  Wire.beginTransmission(addr);
  Wire.write(b);
  Wire.endTransmission();
}

//SendCommand
//------------------------------------------------------------------//
void SendCommand(byte addr, char *c) {
  Wire.beginTransmission(addr);
  while ((*c) != 0) {
    Wire.write(*c);
    c++;
  }
  Wire.write(0x0d);
  Wire.write(0x0a);
  Wire.endTransmission();
}

//AE HX711 Init
//------------------------------------------------------------------//
void AE_HX711_Init(void)
{
  pinMode(HX711_SCLK, OUTPUT);
  pinMode(HX711_DOUT, INPUT);
}

//AE HX711 Reset
//------------------------------------------------------------------//
void AE_HX711_Reset(void)
{
  digitalWrite(HX711_SCLK,1);
  delayMicroseconds(100);
  digitalWrite(HX711_SCLK,0);
  delayMicroseconds(100); 
}

//AE HX711 Read
//------------------------------------------------------------------//
long AE_HX711_Read(void)
{
  long data=0;
  while(digitalRead(HX711_DOUT)!=0);
  delayMicroseconds(10);
  for(int i=0;i<24;i++)
  {
    digitalWrite(HX711_SCLK,1);
    delayMicroseconds(5);
    digitalWrite(HX711_SCLK,0);
    delayMicroseconds(5);
    data = (data<<1)|(digitalRead(HX711_DOUT));
  }
  //Serial.println(data,HEX);   
  digitalWrite(HX711_SCLK,1);
  delayMicroseconds(10);
  digitalWrite(HX711_SCLK,0);
  delayMicroseconds(10);
  return data^0x800000; 
}


long AE_HX711_Averaging(long adc,char num)
{
  long sum = 0;
  for (int i = 0; i < num; i++) sum += AE_HX711_Read();
  return sum / num;
}

float AE_HX711_getGram(char num)
{
  #define HX711_R1  20000.0f
  #define HX711_R2  8200.0f
  #define HX711_VBG 1.25f
  #define HX711_AVDD      4.2987f//(HX711_VBG*((HX711_R1+HX711_R2)/HX711_R2))
  #define HX711_ADC1bit   HX711_AVDD/16777216 //16777216=(2^24)
  #define HX711_PGA 128
  #define HX711_SCALE     (OUT_VOL * HX711_AVDD / LOAD *HX711_PGA)
  
  float data;

  data = AE_HX711_Averaging(AE_HX711_Read(),num)*HX711_ADC1bit; 
  //Serial.println( HX711_AVDD);   
  //Serial.println( HX711_ADC1bit);   
  //Serial.println( HX711_SCALE);   
  //Serial.println( data);   
  data =  data / HX711_SCALE;


  return data;
}
