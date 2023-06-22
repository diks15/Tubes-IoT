#include <LiquidCrystal_I2C.h>
#include <WiFi.h>

// WiFi credentials
const char* ssid = "Realme 7";
const char* password = "liverpoolfc150502";

int SensorPin = 32; // Deklarasi pin analog yang digunakan
int RainSensorPin = 33; // Pin digital untuk sensor hujan
int soilMoistureValue; // Menyimpan nilai analog dari sensor ke ESP32
int soilMoisturePercent; // Nilai yang diperoleh dalam bentuk persen setelah dimapping

LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
  Serial.begin(115200); // Baudrate komunikasi dengan serial monitor
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Set the soil moisture pin as an input
  pinMode(SensorPin, INPUT);

  lcd.begin(); // initialize the lcd 
  lcd.backlight(); // Hidupkan backlight LCD
  lcd.setCursor(0, 0);
  lcd.print("Kelembaban Tanah");
  lcd.setCursor(0, 1);
  lcd.print("Membaca...");
  delay(2000);
  lcd.clear();

  pinMode(RainSensorPin, INPUT); // Set pin sensor hujan sebagai input
}

void loop() {
  soilMoistureValue = analogRead(SensorPin);
  soilMoisturePercent = map(soilMoistureValue, 0, 4095, 0, 100);

  int rainSensorValue = digitalRead(RainSensorPin); // Baca nilai sensor hujan

  Serial.print("Nilai analog = ");
  Serial.print(soilMoistureValue);
  Serial.print(", Persentase kelembaban tanah = ");
  Serial.print(soilMoisturePercent);
  Serial.print("%");

  if (rainSensorValue == HIGH) {
    Serial.println(", Tidak Hujan");
    lcd.setCursor(0, 0);
    lcd.print("Kelembaban: ");
    lcd.print(soilMoisturePercent);
    lcd.print("% (Hujan)");
  } else {
    Serial.println(", Hujan");
    lcd.setCursor(0, 0);
    lcd.print("Kelembaban: ");
    lcd.print(soilMoisturePercent);
    lcd.print("%");
  }

  lcd.setCursor(0, 1);
  lcd.print("Nilai: ");
  lcd.print(soilMoistureValue);

  delay(500);
}
