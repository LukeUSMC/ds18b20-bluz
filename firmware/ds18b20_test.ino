#include "DS18B20/Particle-OneWire.h"
#include "DS18B20/DS18B20.h"

DS18B20 ds18b20 = DS18B20(D2); //Sets Pin D2 for Temp Sensor
int led = D7;
char szInfo[64];
float pubTemp;
double celsius;
double fahrenheit;
unsigned int Metric_Publish_Rate = 30000;
unsigned int MetricnextPublishTime = 5000; //Need time to get the first result
int DS18B20nextSampleTime;
int DS18B20_SAMPLE_INTERVAL = 2500;
int dsAttempts = 0;

void setup() {
    pinMode(D2, INPUT);
    Particle.variable("tempDS18B20", &fahrenheit, DOUBLE);
    Serial1.begin(38400);
    Serial1.println("Starting up...");
}

void loop() {

if (millis() > DS18B20nextSampleTime){
  getTemp();
  }

if (millis() > MetricnextPublishTime){
    publishData();
  }
}


void publishData(){
  if(!ds18b20.crcCheck()){
    return;
  }
  sprintf(szInfo, String(fahrenheit,2));
  Serial1.println("Publishing now: " + String(szInfo));
  Particle.publish("dsTmp", szInfo, PRIVATE);
  MetricnextPublishTime = millis() + Metric_Publish_Rate;
}

void getTemp(){
    if(!ds18b20.search()){
      ds18b20.resetsearch();
      celsius = ds18b20.getTemperature();
      while (!ds18b20.crcCheck() && dsAttempts < 5){
        dsAttempts++;
        if (dsAttempts == 3){
          delay(1000);
        }
        ds18b20.resetsearch();
        celsius = ds18b20.getTemperature();
      }
      fahrenheit = ds18b20.convertToFahrenheit(celsius);
      if (ds18b20.crcCheck())
        Serial1.print("Valid ");
      else
        Serial1.print("Invalid ");
      Serial1.println("result: " + String(fahrenheit,1) + "F or " + String(celsius,1) + "C after " + String(dsAttempts) + " attempt(s)");
      dsAttempts = 1;
      DS18B20nextSampleTime = millis() + DS18B20_SAMPLE_INTERVAL;
    }
}
