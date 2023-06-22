#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

const char* ssid = "Realme 7";
const char* password = "liverpoolfc150502";

#define BLYNK_TEMPLATE_ID       "TMPL6CXCKOlhR"
#define BLYNK_TEMPLATE_NAME     "Project Perancangan Elektronika"
#define BLYNK_AUTH_TOKEN        "tQ3YpKkhhZlJddbNTYQh8-qt4p1bIHvA"
/* Comment this out to disable prints and save space */
#define BLYNK_PRINT Serial

//LCD I2C
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
int lcdColumns = 16;
int lcdRows = 2;
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);

//SENSOR  MQ2
#define MQ2 34

//SENSOR DUST 
int sensePin = 35;
int ledPin = 13;
int Tsampling = 280;
int Tdelta = 40;
int Tsleep = 9680;
float outVo = 0;
float sigVolt = 0;
float dustLevel = 0;

BlynkTimer timer;
void setup() {
  Serial.begin(115200);
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, password);
 
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  pinMode (ledPin, OUTPUT);
  Wire.begin();
  lcd.begin();
  lcd.backlight();

  // You can also specify server:
  //Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass, "blynk.cloud", 80);
  //Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass, IPAddress(192,168,1,100), 8080);
  timer.setInterval(1000L,sendSensor);
  
}

void sendSensor()
{

  //Sensor Dust
  digitalWrite (ledPin, LOW);
  delayMicroseconds(Tsampling);
  outVo = analogRead(sensePin);
  delayMicroseconds(Tdelta);
  digitalWrite(ledPin, HIGH);
  delayMicroseconds(Tsleep);
  sigVolt = outVo * (3.3 / 1024);
  dustLevel = 1.7 * sigVolt - 0.1;

  //Sensor MQ2
  int sensor_Aout = analogRead(MQ2);
  

  Blynk.virtualWrite(V0, sensor_Aout);
  Blynk.virtualWrite(V1, sigVolt);
  Blynk.virtualWrite(V2, dustLevel);
}

void loop() {
  Blynk.run();
  timer.run();
  //Sensor Dust
  digitalWrite (ledPin, LOW);
  delayMicroseconds(Tsampling);
  outVo = analogRead(sensePin);
  delayMicroseconds(Tdelta);
  digitalWrite(ledPin, HIGH);
  delayMicroseconds(Tsleep);
  sigVolt = outVo * (3.3 / 1024);
  dustLevel = 1.7 * sigVolt - 0.1;

  //Sensor MQ-2
  int sensor_Aout = analogRead(MQ2);

  //Tampilan Serial Monitor
  Serial.print("Measured Signal Value (0-1023): ");
  Serial.print(outVo);
  Serial.print(" Voltage: ");
  Serial.print(sigVolt);
  Serial.print(" Dust Density Level: ");
  Serial.println(dustLevel);
  Serial.print(" Kadar CO: ");
  Serial.println(sensor_Aout);
  //Tampilan LCD
  lcd.setCursor(0,0);
  lcd.print("Voltage: ");
  lcd.setCursor(9,0);
  lcd.print(sigVolt);

  lcd.setCursor(0,1);
  lcd.print("Dust: ");
  lcd.setCursor(6,1);
  lcd.print(dustLevel);

  delay(3000);
  lcd.clear();

  lcd.setCursor(0,0);
  lcd.print("CO :");
  lcd.setCursor(6,0);
  lcd.print(sensor_Aout);

  lcd.setCursor(0,1);
  lcd.print("KEL.3");

  delay(3000);

}