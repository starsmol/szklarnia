#include <BluetoothSerial.h>
#include <string>
#include <DHT.h>

BluetoothSerial SerialBT;

#define PIN_LIGHT 27
#define PIN_DHT 33

bool mistStatus, pompStatus, lightStatus, heatingStatus, manualStatus;
int soilHumidity, sun;
int temperature, airHumidity;
int airHumidityTarget, soilHumidityTarget, sunTarget, temperatureTarget;
int step1, step2, step3, step4;

DHT dht(PIN_DHT, DHT11);


// Zmienne do timera
unsigned long previousMillis = 0;
const long interval = 100;  // Interwał 5 sekun

void setup() {
  Serial.begin(115200);
  SerialBT.begin("ESP32_BT");
  Serial.println("ESP32 gotowy do łączenia!");

  pinMode(PIN_LIGHT, OUTPUT);
  dht.begin();
  
  // Inicjalizacja wartości
  soilHumidity = 0;
  sun = 0;

  manualStatus = mistStatus = pompStatus = lightStatus = heatingStatus = 0;

  airHumidityTarget = -999;
  soilHumidityTarget = -999;
  sunTarget = -999;
  temperatureTarget = -999;


}

void loop() {
  // Część odbierająca dane (działa ciągle)
  if (SerialBT.available()) {
    handleBluetoothData();
  }

  temperature = dht.readTemperature();
  airHumidity = dht.readHumidity();

  // Część wysyłająca dane co 5 sekund
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    sendSensorData();  // Wywołaj funkcję wysyłającą dane
  }

  if(lightStatus == true){
    digitalWrite(PIN_LIGHT, HIGH);
  }else if(lightStatus == false){
    digitalWrite(PIN_LIGHT, LOW);
  }
}

void handleBluetoothData() {
  String data = SerialBT.readStringUntil('\n');
  Serial.print("Odebrano: ");
  Serial.println(data);
  
  char mode = data[0];
  char parameter = data[1];
  
  if (mode == 'P') {
    step1 = data.indexOf(';');
    step2 = data.indexOf(';', step1 + 1);
    step3 = data.indexOf(';', step2 + 1);

    temperatureTarget =  data.substring(1,step1).toInt();
    airHumidityTarget = data.substring(step1 + 1, step2).toInt();
    soilHumidityTarget = data.substring(step2 + 1, step3).toInt();
    sunTarget = data.substring(step3 + 1).toInt();
  } else if(mode == 'M'){
      if(parameter == 'x' && manualStatus){
        manualStatus = false;
      }else if(parameter == 'x' && !manualStatus){
        manualStatus = true;
      }
      if(manualStatus){
        if(parameter == 'h' && heatingStatus){
          heatingStatus = false;
        }else if(parameter == 'h' && !heatingStatus){
          heatingStatus = true;
        }

        if(parameter == 'm' && mistStatus){
          mistStatus = false;
        }else if(parameter == 'm' && !mistStatus){
          mistStatus = true;
        }
        if(parameter == 'p' && pompStatus){
          pompStatus = false;
        }else if(parameter == 'p' && !pompStatus){
          pompStatus = true;
        }
        if(parameter == 'l' && lightStatus){
          lightStatus = false;
        }else if(parameter == 'l' && !lightStatus){
          lightStatus = true;
        }
      }
  }
}

void sendSensorData() {
  // Wysłanie sformatowanych danych do aplikacji
  String message = 
    String(temperature) +
    ";"+String(airHumidity) +
    ";"+String(soilHumidity) +
    ";" + String(sun) +
    ";" + String(heatingStatus) +
    ";" + String(mistStatus) +
    ";" + String(pompStatus) +
    ";" + String(lightStatus) +
    ";" + String(manualStatus) +
    ";" + String(temperatureTarget) +
    ";"+String(airHumidityTarget) +
    ";"+String(soilHumidityTarget) +
    ";" + String(sunTarget);
  
  SerialBT.println(message);
  Serial.println("Wysłano dane: " + message);
}