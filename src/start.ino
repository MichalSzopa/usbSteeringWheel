#include <Joystick.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
//#include <Wire.h>
#include <math.h>
#include <TM1638plus.h>

/// Joystick
Joystick_ Joystick;

/// Accelerometer
Adafruit_MPU6050 mpu;

/// tm1638 display
#define  STROBE_TM 7 // strobe = GPIO connected to strobe line of module
#define  CLOCK_TM 9  // clock = GPIO connected to clock line of module
#define  DIO_TM 8 // data = GPIO connected to data line of module
bool high_freq = false; //default false, If using a high freq CPU > ~100 MHZ set to true. 
TM1638plus tm(STROBE_TM, CLOCK_TM , DIO_TM, high_freq);

/// Serial in
char income;
String buff = "";

// Constant that maps the phyical pin to the joystick button.
const int pinToButtonMap = 4;

// Last state of the button
int lastButtonState[9] = {0,0,0,0,0,0,0,0,0};
byte keys = 0;
byte oldKeys = keys;
byte leds = 1;
int i=0, d = 0;
int steeringWheelPosition = 0;
sensors_event_t a, g, temp;
unsigned long StartTime;
unsigned long ElapsedTime;


void readSteeringData()
{
  keys = tm.readButtons();
  mpu.getEvent(&a, &g, &temp);
}

void setJoystick()
{
  { // button 0
    int currentButtonState = !digitalRead(pinToButtonMap);
    if (currentButtonState != lastButtonState[0])
    {
      Joystick.setButton(0, currentButtonState);
      lastButtonState[0] = currentButtonState;
    }
  }

  if(keys != oldKeys)
  {
    for(int i=0; i<8; i++) // buttons 1-8
    {
      int currentButtonState = (keys >> i) & 0b00000001;
      if (currentButtonState != lastButtonState[i+1])
      {
        Joystick.setButton(i+1, currentButtonState);
        lastButtonState[i+1] = currentButtonState;
      }
    }
    oldKeys = keys;
  }
  
  //if(fabs(g.gyro.z) > 0.15)
  {
    ElapsedTime = micros() - StartTime;
    //steeringWheelPosition = steeringWheelPosition - (g.gyro.z*ElapsedTime/20);
    steeringWheelPosition = -255 * atan(a.acceleration.y / sqrt(a.acceleration.x * a.acceleration.x + a.acceleration.z * a.acceleration.z))/(M_PI/2);
    if(steeringWheelPosition < -255)
      Joystick.setXAxis(-255);
    else if(steeringWheelPosition > 255)
      Joystick.setXAxis(255);
    else Joystick.setXAxis(steeringWheelPosition);
    //Serial.println(steeringWheelPosition);
  }

  
}

void readAndPrintTelemetry()
{
  if (Serial.available() > 0) {
    // read incoming serial data:
    income = Serial.read();
    if(income == 's')
      {
          do
          {
            income = Serial.read();
            if(income >= '0' && income <= '9')
              buff += income;
              if(buff.length() == 3) break;
          } while(income >= '0' && income <= '9');
          for(int i=0; i<3; i++)
          {
            if(i>buff.length())tm.displayASCII(i, ' ');
            else tm.displayASCII(i, buff[i]);
          }
          buff = "";
      }
    if(income == 'g')
    {
          income = Serial.read();
          if((income >= '0' && income <= '9') || income == 'R' || income == 'N')
           { 
              buff += income;
              tm.displayASCII(4, buff[0]);
           } 
          buff = "";
    }
    if(income == 'p')
    {
      do
          {
            income = Serial.read();
            if(income >= '0' && income <= '9')
              buff += income;
              if(buff.length() == 2) break;
          } while(income >= '0' && income <= '9');
          if(buff.length() == 2)
          {
            tm.displayASCII(6, buff[0]);
            tm.displayASCII(7, buff[1]);
          } 
          else if(buff.length() == 1) 
          {
            if(buff != "0")
            {
              tm.displayASCII(6, ' ');
              tm.displayASCII(7, buff[0]);
            }
          }
          buff = "";
    }
    if(income == 'r')
    {
      do
          {
            income = Serial.read();
            if(income >= '0' && income <= '9')
              buff += income;
              if(buff.length() == 3) break;
          } while(income >= '0' && income <= '9');
          byte revs = buff.toInt();
          for(int i=0; i<8*revs/100; i++)
          {
            tm.setLED(i, 1);
          }

          for(int i=8*revs/100 + 1; i<8; i++)
          {
            tm.setLED(i, 0); 
          }
          buff = "";
    }
  }
}

void setup() {
  
  /// tm1638 part
  tm.displayBegin();

  /// Joystick part
  // Initialize Button Pins
  pinMode(4, INPUT_PULLUP);
  
  // Initialize Joystick Library
  Joystick.begin();
  Joystick.setXAxisRange(-255, 255);
  Joystick.setYAxisRange(-255, 255);

  /// Accelerometer part
    
  mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  mpu.begin();

  if(a.acceleration.x > 0)
  {
    steeringWheelPosition = -255 * atan(a.acceleration.y / a.acceleration.x)/(M_PI/2);
    //steeringWheelPosition = 255 * atan(a.acceleration.x / sqrt(a.acceleration.y * a.acceleration.y + a.acceleration.z * a.acceleration.z))/(M_PI/2);
    Joystick.setXAxis(steeringWheelPosition);
  }

  //StartTime = micros();
}


void loop() {
  
for(int j=0; j<3; j++)
 {
  readSteeringData();
  //StartTime = micros();
  setJoystick();
 }

  readAndPrintTelemetry();
 /*%Aif((a.acceleration.x > 0) && (fabs(steeringWheelPosition - 255 * atan(a.acceleration.y / a.acceleration.x)/(M_PI/2)) > 5) && (a.acceleration.x < 8))
  {
    steeringWheelPosition = 255 * atan(a.acceleration.y / a.acceleration.x)/(M_PI/2);
    Joystick.setXAxis(steeringWheelPosition);
  }/*
  
  

/*
  for(int j=0; j<3; j++)
 {
  mpu.getEvent(&a, &g, &temp);
  ElapsedTime = millis() - StartTime;
  steeringWheelPosition = steeringWheelPosition + (g.gyro.z*ElapsedTime/1000)*255/2,36;
  if(steeringWheelPosition < -255)
    steeringWheelPosition = -255;
  if(steeringWheelPosition > 255)
    steeringWheelPosition = 255;
  Joystick.setXAxis(steeringWheelPosition);
  
  StartTime = millis();
 }

 Serial.println(ElapsedTime);
mpu.getEvent(&a, &g, &temp);
 Serial.println(g.gyro.z); // w spoczynku odczyty max 0.1 - trzeba to uciąć
 delay(300);*/

}
