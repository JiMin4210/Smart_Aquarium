#include <OneWire.h>
#include <DallasTemperature.h>
#include <TempSensor.h>

extern float temp;

// GPIO where the DS18B20 is connected to
const int oneWireBus = 4;     

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(oneWireBus);

// Pass our oneWire reference to Dallas Temperature sensor 
DallasTemperature sensors(&oneWire);

void tempcheck () {
  sensors.requestTemperatures(); 
  Serial.print(sensors.getTempCByIndex(0));
  Serial.println("ºC");
  delay(5000);
}
