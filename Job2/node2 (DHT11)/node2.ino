// Libraries
#include <WiFi.h>
#include <DHT.h>

// WiFi credentials
const char* ssid = "Realme 7";
const char* password = "liverpoolfc150502";

// DHT11 Sensor
#define DHTPIN 32
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// MQ-2 Sensor
const int mq2Pin = 34;

// Buzzer
const int buzzerPin = 27;

void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Initialize DHT11 sensor
  dht.begin();

  // Set the MQ-2 pin as an input
  pinMode(mq2Pin, INPUT);

  // Set the buzzer pin as an output
  pinMode(buzzerPin, OUTPUT);
}

void loop() {
  // Read temperature and humidity from DHT11 sensor
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  // Read gas concentration from MQ-2 sensor
  int gasValue = analogRead(mq2Pin);

  // Print sensor readings
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.print(" °C\t");
  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.print(" %\t");
  Serial.print("Gas Value: ");
  Serial.print(gasValue);
  Serial.println();

  // Check gas concentration
  if (gasValue > 500) {
    // Turn on the buzzer
    digitalWrite(buzzerPin, HIGH);
    delay(1000);
    // Turn off the buzzer
    digitalWrite(buzzerPin, LOW);
    delay(1000);
  }

  delay(2000);
}
