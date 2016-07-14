#include "Particle-OneWire.h"
#include "DS18B20.h"

/*NOTE:In my experience when testing the DS18B20 with Bluz I found that using a
resolution higher than 10bits (9 and 10 bits give you increments of 0.5°C,
0.25°C respectively) leads to a high rate of failed read attempts.  This
is due to the timing sensitive nature of the DS18B20 and the Bluz BLE radio
interrupts.  If anyone finds a solution that allows a higher resolution please
contact me on GitHub and I will update the lib and example.
https://github.com/LukeUSMC/ds18b20-bluz
*/

DS18B20 ds18b20 = DS18B20(D2);

float pubTemp;
double celsius;
double fahrenheit;
unsigned long metricPublishRate = 30000;
unsigned long lastMetricPublishTime = 35000;
unsigned long lastDSSampleTime = 20000;
unsigned long dsSampleRate = 2000;
bool firstPass = true;
bool radioON;
int failedAttempts;
int successAttempts;
float readPercentage;

void getTemp();
void publishData();
void radioCallbackHandler(bool radio_active);

/* executes once at startup */
void setup() {
  pinMode(D2, INPUT);
  Serial1.begin(38400);
  Serial1.println("Staring up...");
  Particle.variable("tempDS18B20", &fahrenheit, DOUBLE);
  ds18b20.setResolution(9); //See Note above before modifying
  BLE.registerNotifications(radioCallbackHandler);
}

/* executes continuously after setup() runs */
void loop() {

  if (millis() - lastDSSampleTime > dsSampleRate && !radioON){
    getTemp();
    lastDSSampleTime = millis();
    }

  if (Particle.connected()){
  /*NOTE: See Bluz Docs for details on SLEEP_MODE_CPU, if commented battery
  usage will be high. Wait for a cloud connection before entering SLEEP*/
      System.sleep(SLEEP_MODE_CPU);
      if (millis() - lastMetricPublishTime > metricPublishRate && !radioON){
        publishData();
        lastMetricPublishTime = millis();
        }
    }
}

void radioCallbackHandler(bool radio_active){
  radioON = radio_active;
}

void publishData(){
if(!ds18b20.crcCheck()){
    return;
  }
  Serial1.println("DS18B20 Temp is: " + String(fahrenheit,2) + "F. Success Rate is: " + String(readPercentage,2) + "%.");
  Particle.publish("bluzTemp", "DS18B20 Temp is: " + String(fahrenheit,2) + "F. Success Rate is: " + String(readPercentage,2) + "%.", PRIVATE);
}

void getTemp(){
  if(firstPass){
    ds18b20.setResolution(9);
    firstPass = false;
  }
  if(!ds18b20.search()){
    ds18b20.resetsearch();
    celsius = ds18b20.getTemperature();
    if (!ds18b20.crcCheck()){
      failedAttempts++;
    }else{
      successAttempts++;
    }
    fahrenheit = ds18b20.convertToFahrenheit(celsius);
    int totalAttempts = successAttempts + failedAttempts;
    readPercentage = ((float)successAttempts /  (float)totalAttempts) * 100.0;
  }

}
