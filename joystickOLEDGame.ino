#include <Wire.h>
#include <U8g2lib.h>
#include <Servo.h>
#include "SR04.h"


const byte xPin = A3;
const byte yPin = A2;
const byte swPin = 2;
const byte trigPin = 4;
const byte echoPin = 5;
const byte buzzerPin = 12;
int servoPos = 0;
int servoDirection = 1;
int buzzerDelay = 50;
int maxDist = 0;
int minDist;
int radarMagnitude;
long uSensorDist;
long input;
unsigned long previousTime = 0;
bool buzzerState = false;


U8G2_SSD1309_128X64_NONAME0_1_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
Servo myServo;
SR04 usSensor = SR04(echoPin, trigPin);


void drawRadar() {
  for(int angle = 0; angle <= 180; angle += 1) {
  float rad = angle * 3.14159265 / 180;
  int x = 80 + 30*cos(rad);
  int y = 40 - 30 * sin(rad);
  u8g2.drawPixel(x,y);
  }

  if(uSensorDist > 100) {
    radarMagnitude = 100;
  } else {
    radarMagnitude = uSensorDist;
  }
  u8g2.drawLine(80, 40,
    80 + 30*cos(servoPos * 3.14159265 / 180.0) * radarMagnitude / 100,
    40 - 30 * sin(servoPos * 3.14159265 / 180.0) * radarMagnitude / 100
  );

  int blipX = 80 + 30*cos(servoPos * 3.14159265 / 180.0) * radarMagnitude / 100;
  int blipY = 40 - 30 * sin(servoPos * 3.14159265 / 180.0) * radarMagnitude / 100;
  u8g2.drawCircle(blipX, blipY, 2);
}


void setup() {
  u8g2.begin();
  int initialDist = usSensor.Distance();
  pinMode(swPin, INPUT);
  digitalWrite(swPin, HIGH);
  Serial.begin(9600);
  myServo.attach(3);
  myServo.write(0);
  Wire.begin();
  Wire.setClock(100000);  // force standard I2C speed
  pinMode(buzzerPin, OUTPUT);
  maxDist = initialDist;
  minDist = initialDist;
}

void loop() {
  int xInput = analogRead(xPin);
  int yInput = analogRead(yPin);
  uSensorDist = usSensor.Distance();
  unsigned long currentTime = millis();

  if(uSensorDist < 5) {
    buzzerDelay = 5;
  } else if (uSensorDist < 10) {
    buzzerDelay = 10;
  } else if (uSensorDist < 25) {
    buzzerDelay = 50;
  } else if (uSensorDist < 50) {
    buzzerDelay = 100;
  } else {
    buzzerDelay = 0;
  }

  if((currentTime - previousTime >= buzzerDelay) && buzzerDelay != 0) {
    previousTime = currentTime;
    buzzerState = !buzzerState;
    digitalWrite(buzzerPin, buzzerState);
  } else {
    digitalWrite(buzzerPin, LOW);
  }




  input = uSensorDist;

  if(xInput > 600) {
    servoPos -= 8;
  }

  if(xInput < 400) {
    servoPos += 8;
  }

  myServo.write(servoPos);

  if (servoPos > 180) {
    servoPos = 180;
  }
  if (servoPos < 0) {
    servoPos = 0;
  }

  char buffer[20];
  sprintf(buffer, "%ld", input);

  if(uSensorDist < minDist) {
    minDist = uSensorDist;
  }
  if(uSensorDist > maxDist) {
    maxDist = uSensorDist;
  }

  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_ncenB08_tr);



    u8g2.drawStr(0, 20, buffer);
    u8g2.drawStr(20, 20, "cm" );
    u8g2.setFont(u8g2_font_5x8_tr);
    char minDistBuffer[20];
    sprintf(minDistBuffer, "Min: %dcm", minDist);
    u8g2.drawStr(0, 60, minDistBuffer);
    char maxDistBuffer[20];
    sprintf(maxDistBuffer, "Max: %dcm", maxDist);
    u8g2.drawStr(80, 60, maxDistBuffer);


    drawRadar();



  } while (u8g2.nextPage());

  delay(2);  // 👈 slower = more accurate readings
}