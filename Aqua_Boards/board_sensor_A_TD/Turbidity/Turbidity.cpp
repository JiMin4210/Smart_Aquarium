#include "Turbidity.h"

void turcheck(int *env) {
  int sensorValue = analogRead(0);// read the input on analog pin 0:
  *env = sensorValue /1024 + (1024-sensorValue); // Convert the analog reading (which goes from 0 - 1023) to a voltage (0 - 5V):
  Serial.println(*env); // print out the value you read:
}