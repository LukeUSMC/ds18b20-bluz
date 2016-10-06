#include "Particle-OneWire.h"
#include "DS18B20.h"

#define ledPin D7
#define oneWirePin D2
#define dsResolution 9
// Higher resolution takes longer to calculate but should give increase precision
// Bit | Precision (F) | Time to calculate (+ ~30-50ms overhead)
//  9  |  .9F |  94ms
// 10  | .45F |  188ms
// 11  | .23F |  375ms
// 12  | .11F |  750ms
// In my testing it doesn't seem like the resolution setting is working, 
// 9-bit and 12-bit give the exact same result but the latter takes much longer to process

DS18B20 ds18b20 = DS18B20(oneWirePin, dsResolution);

float pubTemp;
double celsius;
double fahrenheit;
unsigned long metricPublishRate = 30000;
unsigned long lastMetricPublishTime = 35000;
unsigned long lastDSSampleTime = 20000;
unsigned long dsSampleRate = 2000;
bool foundSensor = false;

int failedAttempts = 0;
int successAttempts = 0;
int totalAttempts = 0;
float readPercentage = 0;
unsigned long totalTime = 0;
float averageTime = 0;


/* executes once at startup */
void setup() {
    pinMode(ledPin, OUTPUT);
    pinMode(oneWirePin, INPUT);
    Serial1.begin(38400);
    Serial1.println("Staring up...");
    Particle.variable("tempDS18B20", &fahrenheit, DOUBLE);

}

/* executes continuously after setup() runs */
void loop() {
    
    if (millis() - lastDSSampleTime > dsSampleRate){
        getTemp();
        lastDSSampleTime = millis();
        if (foundSensor) 
            Serial1.println("Temp is: " + String(fahrenheit,2) + "F. Success: " + String(readPercentage,2) + "%; Attempts: " + String(totalAttempts) + "; Avg time for success: " + String(averageTime, 1) + "ms" );

        if (Particle.connected()){
            /*NOTE: See Bluz Docs for details on SLEEP_MODE_CPU, if commented battery
            usage will be high. Wait for a cloud connection before entering SLEEP*/
            System.sleep(SLEEP_MODE_CPU);
            if (millis() - lastMetricPublishTime > metricPublishRate){
                publishData();
                lastMetricPublishTime = millis();
            }
        }
    }
}

void publishData()
{
    if(foundSensor){
        Particle.publish("bluzTemp", "Temp is: " + String(fahrenheit,2) + "F. Success: " + String(readPercentage,2) + "%; Attempts: " + String(totalAttempts) + "; Avg time for success: " + String(averageTime, 1) + "ms", 60, PRIVATE);
        lastMetricPublishTime = millis();
    }
}

void getTemp(){
    digitalWrite(ledPin, HIGH);
    if(!foundSensor){
        //Takes several tries to find a device successfully but after that we save and re-use it
        bool success = ds18b20.search();
        if (success)
        {
            Serial1.println("successful search, sensor found");
            foundSensor = true; 
            char szRom[24];
            ds18b20.getROM(szRom); //get address
            Serial1.print("Rom: ");
            Serial1.println(szRom);
            Serial1.println("Resolution: " + String(ds18b20.getResolution()) + "bit");
        }
        else
            Serial1.println("failed search, will try again next time");
    }
    if (foundSensor)
    {
        unsigned long before=millis();
        celsius = ds18b20.getTemperature();
        if (!ds18b20.crcCheck()){
            failedAttempts++;
            Serial1.println("failed attempt");
            }else{
            fahrenheit = ds18b20.convertToFahrenheit(celsius);
            successAttempts++;
        }
        unsigned long delta = millis()-before;
        totalTime += delta;
        averageTime = (totalTime/successAttempts);

        totalAttempts = successAttempts + failedAttempts;
        readPercentage = ((float)successAttempts /  (float)totalAttempts) * 100.0;
    }
    digitalWrite(ledPin, LOW);
    
}
