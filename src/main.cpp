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
#define   TIMER_INTERRUPT   100 // ms
#define   LCD
#define   STEPMOTOR_I2C_ADDR 0x70
// #define  STEPMOTOR_I2C_ADDR 0x71

//Global
//------------------------------------------------------------------//
int     pattern = 0;
bool    hover_flag = false;

byte    counter;

BluetoothSerial bts;

// MPU9250
MPU9250 IMU; 

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


//Setup
//------------------------------------------------------------------//
void setup() {

  M5.begin();
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
  

}


//Main
//------------------------------------------------------------------//
void loop() {
   
  switch (pattern) {
    case 0:
      break;

    case 11:
      break;
    
  }

  bts.println(counter);
  counter++;
  delay(100);

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
    SendCommand(STEPMOTOR_I2C_ADDR, "G1 X100 F1000");
    SendCommand(STEPMOTOR_I2C_ADDR, "G1 X0 F400");
  }

 // Timer Interrupt
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
    
    case 10:
      iTimer10 = 0;
      break;

    }

  }

}

