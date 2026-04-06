#include <Wire.h>
#include <U8g2lib.h>
#include <Servo.h>
#include "SR04.h"

//Joystick pins
const byte xPin = A3;
const byte yPin = A2;
const byte swPin = 2;
//Ultrasonic Sensor pins
const byte trigPin = 4;
const byte echoPin = 5;
//Ultrasonic Sensor readings: Max, Min, Current
int maxDist;
int minDist;
int uSensorDist;
//Servo variables
const byte servoPin = 3;
int servoPos = 0;
//Buzzer variables
const byte buzzerPin = 12;
unsigned long previousTime = 0;
bool buzzerState = false;
byte buzzerDelay = 50;
int radarMagnitude;
bool trackingMode = false;
//Used to switch between manual and auto
bool lastButtonState = HIGH;
bool currentButtonState;
//Auto tracking variables
int trackingDirection = 1;
int servoSpeed = 4;
int closestDist = 999;
int closestAngle = 0;

//Instantiate compnent instances
U8G2_SSD1309_128X64_NONAME0_1_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
Servo myServo;
SR04 usSensor = SR04(echoPin, trigPin);

//prototypes
void drawRadar();
void updateOLED();
void updateServoPos();
void updateBuzzer();
void autoTrack();


void setup() {
  int initialDist = usSensor.Distance();//Calibrate max,min distances
  maxDist = initialDist;
  minDist = initialDist;
  u8g2.begin();//Start writing to OLED
  pinMode(swPin, INPUT);//Setup joystick click pin
  digitalWrite(swPin, HIGH);
  Serial.begin(9600);
  myServo.attach(servoPin);//Setup servo motor
  myServo.write(0);
  Wire.setClock(100000);  // force standard I2C speed
  pinMode(buzzerPin, OUTPUT);
}

void loop() {
  //Initiate dynamic variables
  int xInput = analogRead(xPin);//Get Joystick x input
  uSensorDist = usSensor.Distance();

  currentButtonState = digitalRead(swPin);

  //Detect button press
  if(lastButtonState == HIGH && currentButtonState == LOW) trackingMode = !trackingMode;
  lastButtonState = currentButtonState;

  //Update each component using functional programming
  if(!trackingMode) updateServoPos(xInput);
  if(trackingMode) autoTrack();
  updateOLED();
  updateBuzzer();

}


void updateOLED() {
  char buffer[20];
  sprintf(buffer, "%d", uSensorDist);
  //Set min,max readings
  if(uSensorDist < minDist) {
    minDist = uSensorDist;
  }
  if(uSensorDist > maxDist) {
    maxDist = uSensorDist;
  }

  //begin OLED writing
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_ncenB08_tr);

    u8g2.drawStr(0, 20, buffer);
    u8g2.drawStr(20, 20, "cm" );

    u8g2.setFont(u8g2_font_5x8_tr);
    //write min, max readings
    char minDistBuffer[20];
    sprintf(minDistBuffer, "Min: %dcm", minDist);
    u8g2.drawStr(0, 60, minDistBuffer);

    char maxDistBuffer[20];
    sprintf(maxDistBuffer, "Max: %dcm", maxDist);
    u8g2.drawStr(80, 60, maxDistBuffer);
    //write current mode
    u8g2.drawStr(
      0,
      48,
      trackingMode ? "Auto-Track" : "Manual"
    );

    drawRadar();

  } while (u8g2.nextPage());
}

void drawRadar() {
  //draw radar arc one pixel at a time
  for(int angle = 0; angle <= 180; angle += 5) {
  float rad = radians(angle);
  int x = 80 + 30*cos(rad);
  int y = 40 - 30 * sin(rad);
  u8g2.drawPixel(x,y);
  }

  //clamp distance reading for radar line
  if(uSensorDist > 100) {
    radarMagnitude = 100;
  } else {
    radarMagnitude = uSensorDist;
  }

  //draw radar line - size is proportional to current distance
  u8g2.drawLine(80, 40,
    80 + 30*cos(radians(servoPos)) * radarMagnitude / 100,
    40 - 30 * sin(radians(servoPos)) * radarMagnitude / 100
  );

  //draw blip at end of line to signal object
  int blipX = 80 + 30*cos(radians(servoPos)) * radarMagnitude / 100;
  int blipY = 40 - 30 * sin(radians(servoPos)) * radarMagnitude / 100;
  u8g2.drawCircle(blipX, blipY, 2);
}

void updateServoPos(int x) {
  if(x > 600) servoPos-= servoSpeed;
  if(x < 400) servoPos+= servoSpeed;

  //clamp servo position
  if (servoPos > 180) {
    servoPos = 180;
  }
  if (servoPos < 0) {
    servoPos = 0;
  }

  myServo.write(servoPos);
}

void updateBuzzer() {
  unsigned long currentTime = millis();
  if(uSensorDist < 20) {
    tone(buzzerPin, 1000);
  } else {
    noTone(buzzerPin);
  }

 
}

int getStableDistance() {
  int sum = 0;
  int count = 3;

  for(int i = 0; i < count; i++) {
    sum += usSensor.Distance();
    delay(5);
  }
  return sum / count;
}

void autoTrack() {
  servoPos += trackingDirection * servoSpeed;
  if(servoPos > 180) {
    servoPos = 180;
    trackingDirection = -1;
  }
  if(servoPos < 0) {
    servoPos = 0;
    trackingDirection = 1;

    //New sweep started - Reset tracking
    closestDist = 999;

  }
    
    if(uSensorDist > 2 && uSensorDist < closestDist && uSensorDist < 150) {
      closestDist = uSensorDist;
      closestAngle = servoPos;
    }
  if(closestDist < 999) {
    myServo.write(closestAngle);
    servoPos = closestAngle;
  }

}