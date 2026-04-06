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

//Instantiate compnent instances
U8G2_SSD1309_128X64_NONAME0_1_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
Servo myServo;
SR04 usSensor = SR04(echoPin, trigPin);

//prototypes
void drawRadar();
void updateOLED();
void updateServoPos();
void updateBuzzer();


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

  //Update each component using functional programming
  updateServoPos(xInput);
  updateOLED();
  updateBuzzer();

}


void updateOLED() {
  char buffer[20];
  sprintf(buffer, "%d", uSensorDist);

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
}

void drawRadar() {
  for(int angle = 0; angle <= 180; angle += 5) {
  float rad = radians(angle);
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
    80 + 30*cos(radians(servoPos)) * radarMagnitude / 100,
    40 - 30 * sin(radians(servoPos)) * radarMagnitude / 100
  );

  int blipX = 80 + 30*cos(radians(servoPos)) * radarMagnitude / 100;
  int blipY = 40 - 30 * sin(radians(servoPos)) * radarMagnitude / 100;
  u8g2.drawCircle(blipX, blipY, 2);
}

void updateServoPos(int x) {
  if(x > 600) servoPos-= 3;
  if(x < 400) servoPos+= 3;


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
}