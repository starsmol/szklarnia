#include <DHT22.h>
#include <DS18B20.h>
#include <BH1750FVI.h>
#include <Wire.h>

#define DHT_PIN 2
#define DS18_PIN 15
#define SCL 19
#define SDA 21
#define SOIL_PIN 4

// PLACEHOLDER VALUES!!!!

#define HEATING_PIN      1
#define FAN_PIN          3
#define MIST_PIN         5
#define PUMP_PIN         6
#define LED_LIGHT_PIN    7
#define HEAT_METER_PIN   8

class SoilMeter {
  int input;

  public:
  SoilMeter(int inputPin) {
    input = inputPin;
  }

  void begin() {
    pinMode(input, INPUT);
  }

  int readHumidity() {
    return analogRead(input);
  }
};

class HeaterTemperatureMeter {
  private:
    int pin;
    float R1;
    float Beta;
    float To;
    float Ro;
    float adcMax;
    float Vs;

  public:
    HeaterTemperatureMeter(int analogPin, float voltage = 3.3, float seriesResistor = 10000.0, float beta = 3950.0, float nominalRes = 10000.0, float adcResolution = 4095.0) {
      pin = analogPin;
      Vs = voltage;
      R1 = seriesResistor;
      Beta = beta;
      Ro = nominalRes;
      To = 298.15;
      adcMax = adcResolution;
    }

    float readTemperature() {
      int adc = analogRead(pin);
      float Vout = adc * Vs / adcMax;
      float Rt = R1 * Vout / (Vs - Vout);
      float T = 1.0 / (1.0 / To + log(Rt / Ro) / Beta);
      return T - 273.15;
    }
};

class Sensors {
  DHT22 humidityMeter;
  DS18B20 temperatureMeter;
  BH1750FVI lightMeter;
  SoilMeter soilMeter;
  HeaterTemperatureMeter heaterTempMeter;

  public:
  Sensors() :
    humidityMeter(DHT_PIN),
    temperatureMeter(DS18_PIN),
    lightMeter(0x23),
    soilMeter(SOIL_PIN),
    heaterTempMeter(HEATER_METER_PIN)
  {}

  void begin() {
    Wire.begin(SDA, SCL);
    delay(100);
    lightMeter.begin();
    lightMeter.setContHighRes();
    soilMeter.begin();
  }

  float readHumidity() {
    return humidityMeter.getHumidity();
  }

  float readTemperature() {
    return temperatureMeter.getTempC();
  }

  float readHeaterTemperature() {
    return heaterTempMeter.readTemperature();
  }

  float readLightLevel() {
    return lightMeter.getLux();
  }

  float getHumidityMeterLastError() {
    return humidityMeter.getLastError();
  }

  float readSoilHumidity() {
    return soilMeter.readHumidity();
  }
};

class Controller {
  private:
    int heatingPin;
    int fanPin;
    int mistPin;
    int pumpPin;
    int ledLightPin;

  public:
    Controller(int heating, int fan, int mist, int pump, int led) {
      heatingPin = heating;
      fanPin = fan;
      mistPin = mist;
      pumpPin = pump;
      ledLightPin = led;

      pinMode(heatingPin, OUTPUT);
      pinMode(fanPin, OUTPUT);
      pinMode(mistPin, OUTPUT);
      pinMode(pumpPin, OUTPUT);
      pinMode(ledLightPin, OUTPUT);

      digitalWrite(heatingPin, LOW);
      digitalWrite(fanPin, LOW);
      digitalWrite(mistPin, LOW);
      digitalWrite(pumpPin, LOW);
      digitalWrite(ledLightPin, LOW);
    }

    void heatingOn() { 
      digitalWrite(heatingPin, HIGH); 
    }
    void heatingOff() { 
      digitalWrite(heatingPin, LOW); 
    }

    void fanOn() { 
      igitalWrite(fanPin, HIGH); 
    }
    void fanOff() { 
      digitalWrite(fanPin, LOW); 
    }

    void mistOn() { 
      digitalWrite(mistPin, HIGH); 
    }
    void mistOff() { 
      digitalWrite(mistPin, LOW); 
    }

    void pumpOn() { 
      digitalWrite(pumpPin, HIGH); 
    }
    void pumpOff() { 
      digitalWrite(pumpPin, LOW); 
    }

    void ledOn() { 
      digitalWrite(ledLightPin, HIGH); 
    }
    void ledOff() { 
      digitalWrite(ledLightPin, LOW); 
    }
};

class Greenhouse {
  public:
    float targetTemperature = 23.0;
    float targetHumidity = 50.0;
    float targetLight = 200.0;
    int   targetSoil = 400;

    float tempTolerance = 2.0;
    float humidityTolerance = 5.0;
    float lightTolerance = 50.0;
    int soilTolerance = 50;

    unsigned long lastWateringTime = 0;
    unsigned long pumpStartTime = 0;
    bool pumpActive = false;
    const unsigned long pumpDuration = 5000;        
    const unsigned long soilCheckDelay = 300000;    

    Sensors sensors;
    Controller controller;

    Greenhouse(Controller ctrl) : controller(ctrl) {}
    void begin() {
      sensors.begin();
    }

  void update() {
      float temp = sensors.readTemperature();
      float hum = sensors.readHumidity();
      float light = sensors.readLightLevel();
      int soil = sensors.readSoilHumidity();
      float heaterTemp = sensors.readHeaterTemperature();

      Serial.println("=== Greenhouse Check ===");
      Serial.print("Temp: "); Serial.println(temp);
      Serial.print("Hum: "); Serial.println(hum);
      Serial.print("Light: "); Serial.println(light);
      Serial.print("Soil: "); Serial.println(soil);
      Serial.print("Heater temp: "); Serial println(heaterTemp);

      if (temp < targetTemperature - tempTolerance) {
        controller.heatingOn();
        controller.fanOff();
        Serial.println("TEMP CHECK - HEAT ON, FAN OFF");
      } else if (temp > targetTemperature + tempTolerance) {
        controller.heatingOff();
        controller.fanOn();
        Serial.println("TEMP CHECK - HEAT OFF, FAN ON");
      } else {
        controller.heatingOff();
        controller.fanOff();
        Serial.println("TEMP CHECK - HEAT OFF, FAN OFF");
      }

      if (hum < targetHumidity - humidityTolerance) {
        controller.mistOn();
        Serial.println("HUM CHECK - MIST ON");
      } else if (hum > targetHumidity + humidityTolerance) {
        controller.mistOff();
        Serial.println("HUM CHECK - MIST OFF");
      }

      if (light < targetLight - lightTolerance) {
        controller.ledOn();
        Serial.println("LIGHT CHECK - LED ON");
      } else if (light > targetLight + lightTolerance) {
        controller.ledOff();
        Serial.println("LIGHT CHECK - LED OFF");
      }

      unsigned long currentTime = millis();

      if (pumpActive) {
        if (currentTime - pumpStartTime >= pumpDuration) {
          controller.pumpOff();
          pumpActive = false;
          lastWateringTime = currentTime;
          Serial.println("SOIL PUMP - OFF after 5s");
        }
      } else if (currentTime - lastWateringTime >= soilCheckDelay) {
        if (soil < targetSoil - soilTolerance) {
          controller.pumpOn();
          pumpStartTime = currentTime;
          pumpActive = true;
          Serial.println("SOIL CHECK - PUMP ON for 5s");
        } else {
          Serial.println("SOIL CHECK - OK");
        }
      } else {
        Serial.println("SOIL CHECK - Skipped (delay active)");
      }

      Serial.println("========================\n");
    }

    void setTargets(float temp, float hum, float light, int soil) {
      targetTemperature = temp;
      targetHumidity = hum;
      targetLight = light;
      targetSoil = soil;
    }

    void setTolerances(float tTol, float hTol, float lTol, int sTol) {
      tempTolerance = tTol;
      humidityTolerance = hTol;
      lightTolerance = lTol;
      soilTolerance = sTol;
    }

}

Controller controller(HEATING_PIN, FAN_PIN, MIST_PIN, PUMP_PIN, LED_LIGHT_PIN);
Greenhouse greenhouse(controller);


void setup() {
  Serial.begin(9600);
  sensors.begin();
  greenhouse.setTargets(23.0, 50.0, 200.0, 400);            
  greenhouse.setTolerances(2.0, 5.0, 50.0, 50);       
}

void loop() {
  greenhouse.update();
  delay(5000);
}