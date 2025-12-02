#include <RedBot.h>

RedBotMotors motors;

void setup() {
  Serial.begin(9600);
  Serial.println("HELLO from setup!");
}

#define FORWARDV 0.6
#define LEFTV 1.2
#define RIGHTV 1.8
#define BACKWARDV 2.4


void loop() {
  // Read voltage from A0 (analog 1023 max, 5V ref).
  float voltage = analogRead(A0) * (5.0 / 1023.0);

  Serial.print("Voltage: ");
  Serial.print(voltage);
  Serial.print("V  ");

  if (voltage < FORWARDV) {
    motors.drive(100);
    Serial.println("W");
  }
  else if (voltage < LEFTV) {
    motors.leftDrive(-100);
    motors.rightDrive(100);
    Serial.println("A");
  }
  else if (voltage < RIGHTV) {
    motors.leftDrive(100);
    motors.rightDrive(-100);
    Serial.println("D");
  }
  else if (voltage < BACKWARDV) {
    motors.leftDrive(-100);
    motors.rightDrive(-100);
    Serial.println("S");
  }
  else {
    motors.stop();
    Serial.println("Stop");
  }
  delay(100);
}
