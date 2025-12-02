#include <RedBot.h>

RedBotMotors motors;

void setup() {
  Serial.begin(9600);
  Serial.println("HELLO from setup!");
}

#define STOPV 0.8
#define LEFTV 1.6
#define RIGHTV 2.4

void loop() {
  // Read voltage from A0 (analog 1023 max, 5V ref).
  float voltage = analogRead(A0) * (5.0 / 1023.0);

  Serial.print("Voltage: ");
  Serial.print(voltage);
  Serial.print("V  ");

  if (voltage < STOPV) {
    motors.stop();
    Serial.println("S");
  }
  else if (voltage < LEFTV) {
    motors.leftDrive(-100);
    motors.rightDrive(100);
    Serial.println("L");
  }
  else if (voltage < RIGHTV) {
    motors.leftDrive(100);
    motors.rightDrive(-100);
    Serial.println("R");
  }
  else {
    motors.drive(100);
    Serial.println("G");
  }
  delay(100);
}
