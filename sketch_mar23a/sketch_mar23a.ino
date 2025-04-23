#include <DHT.h>
#include "MQ135.h" 

#define DHTPIN 2
#define DHTTYPE DHT11    
#define SOUND_SENSOR_PIN A1
#define DUST_LED_PIN 3
#define DUST_SENSOR_PIN A3
#define GAS_SENSOR_PIN A2

DHT dht(DHTPIN, DHTTYPE);
MQ135 gasSensor = MQ135(GAS_SENSOR_PIN); // инициализация объекта датчика

// Тайминги и интервалы
unsigned long lastSoundTime = 0;
unsigned long lastDHTTime = 0;
unsigned long lastDustTime = 0;
unsigned long lastGasTime = 0;
unsigned long lastOutputTime = 0;
const unsigned long intervals[] = {
  100,    // soundInterval
  2000,   // DHTInterval
  2000,   // dustInterval
  500,    // gasInterval
  5000    // outputInterval
};

// Глобальные переменные данных
struct SensorData {
  float temperature = 0.0;
  float humidity = 0.0;
  float sound = 0.0;
  float dust = 0.0;
  float gas = 0.0;
  String name = "Соколиная гора(Проспект буденного)";
} sensorData;

// Буферы для усреднения
int soundReadings[20] = {0};
int soundIndex = 0;
int soundSum = 0;
float dustSum = 0;
int dustCount = 0;
float gasSum = 0;        
int gasCount = 0;        

void setup() {
  Serial.begin(115200);
  dht.begin();
  pinMode(DUST_LED_PIN, OUTPUT);
  
  // Добавление задержки для прогрева датчиков
  Serial.println("System initializing... Warming up sensors (2 seconds)");
  delay(2000); // 20 секунд задержки для прогрева датчиков
  Serial.println("System initialized");
}

void loop() {
  unsigned long currentTime = millis();

  // Обновление данных с датчиков
  updateSoundData(currentTime);
  updateDHTData(currentTime);
  updateDustData(currentTime);
  updateGasData(currentTime);

  // Отправка данных
  if (currentTime - lastOutputTime >= intervals[4]) {
    lastOutputTime = currentTime;
    sendSensorData();
  }
}

void updateSoundData(unsigned long currentTime) {
  if (currentTime - lastSoundTime >= intervals[0]) {
    lastSoundTime = currentTime;
    
    soundSum -= soundReadings[soundIndex];
    int newReading = analogRead(SOUND_SENSOR_PIN);
    float voltage = newReading * (5.0 / 1023);
    soundReadings[soundIndex] = 20 * log10(voltage / 0.00631);
    soundSum += newReading;
    
    soundIndex = (soundIndex + 1) % 20;
    sensorData.sound = soundSum / 20.0;
  }
}

void updateDHTData(unsigned long currentTime) {
  if (currentTime - lastDHTTime >= intervals[1]) {
    lastDHTTime = currentTime;
    
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    
    if (!isnan(h) && !isnan(t)) {
      sensorData.humidity = h;
      sensorData.temperature = t;
    }
  }
}

void updateDustData(unsigned long currentTime) {
  if (currentTime - lastDustTime >= intervals[2]) {
    lastDustTime = currentTime;
    
    digitalWrite(DUST_LED_PIN, LOW);
    delayMicroseconds(280);
    float raw = analogRead(DUST_SENSOR_PIN);
    delayMicroseconds(40);
    digitalWrite(DUST_LED_PIN, HIGH);
    delayMicroseconds(9680);

    float voltage = raw * (5.0 / 1024.0);
    float density = (voltage - 0.1) / 0.17;
    
    dustSum += max(density, 0.0);
    dustCount++;
    sensorData.dust = dustCount > 0 ? dustSum / dustCount : 0;
    
    dustSum = 0;
    dustCount = 0;
  }
}

void updateGasData(unsigned long currentTime) {
  if (currentTime - lastGasTime >= intervals[3]) {
    lastGasTime = currentTime;
    
    int raw = gasSensor.getPPM();
    float voltage = raw * (5.0 / 1024.0);
    float ppm = voltage * 200;
    
    gasSum += ppm;
    gasCount++;
    sensorData.gas = gasCount > 0 ? gasSum / gasCount : 0;
    
    gasSum = 0;
    gasCount = 0;
  }
}

void sendSensorData() {
  // Формируем строку в формате URL-encoded
  String payload = "name="+sensorData.name;
  payload += "&temperature=" + String(sensorData.temperature, 1);
  payload += "&humidity=" + String(sensorData.humidity, 1);
  payload += "&sound=" + String(sensorData.sound, 1);
  payload += "&dust=" + String(sensorData.dust, 3);
  payload += "&gas=" + String(sensorData.gas, 1);

  // Отправляем через Serial.write()
  Serial.println(payload);
  Serial.write(payload.c_str());  // Отправляем как сырые байты
  Serial.write('\n');             // Добавляем символ конца строки
}