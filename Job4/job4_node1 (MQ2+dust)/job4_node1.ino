#include <Wire.h>
#include <painlessMesh.h>
#include <AsyncTCP.h>
#include <LiquidCrystal_I2C.h>

#define LED 2
#define BLINK_PERIOD 3000
#define BLINK_DURATION 100
#define MESH_SSID "Realme 7"
#define MESH_PASSWORD "liverpoolfc150502"
#define MESH_PORT 5555

//LCD I2C
int lcdColumns = 16;
int lcdRows = 2;
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);

//Sensor MQ2
#define MQ2 34

//Sensor Dust
int sensePin = 35;
int ledPin = 13;
int Tsampling = 280;
int Tdelta = 40;
int Tsleep = 9680;
float outVo = 0;
float sigVolt = 0;
float dustLevel = 0;

// Prototypes
void sendMessage();

void receivedCallback(uint32_t from, String &msg);
void newConnectionCallback(uint32_t nodeId);
void changedConnectionCallback();
void nodeTimeAdjustedCallback(int32_t offset);
void delayReceivedCallback(uint32_t from, int32_t delay);

Scheduler userScheduler; // untuk mengontrol tugas pribadi Anda
painlessMesh mesh;
bool calc_delay = false;
SimpleList<uint32_t> nodes;

void sendMessage(); // Prototipe
Task taskSendMessage(TASK_SECOND * 1, TASK_FOREVER, &sendMessage); // mulai dengan interval satu detik

// Task untuk berkedip jumlah node
Task blinkNoNodes;
bool onFlag = false;

void setup()
{
  Serial.begin(115200);

  // Set up soil moisture
  digitalWrite(ledPin, LOW);
  delayMicroseconds(Tsampling);
  outVo = analogRead(sensePin);
  delayMicroseconds(Tdelta);
  digitalWrite(ledPin, HIGH);
  delayMicroseconds(Tsleep);
  sigVolt = outVo * (3.3 / 1024);
  dustLevel = 1.7 * sigVolt - 0.1;

  // Tampilan di LCD i2C
  pinMode(ledPin, OUTPUT);
  Wire.begin();
  lcd.begin();
  lcd.backlight();

  mesh.setDebugMsgTypes(ERROR | DEBUG); // set sebelum init() agar Anda dapat melihat pesan kesalahan
  mesh.init(MESH_SSID, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
  mesh.onNodeDelayReceived(&delayReceivedCallback);

  userScheduler.addTask(taskSendMessage);
  taskSendMessage.enable();

  blinkNoNodes.set(BLINK_PERIOD, (mesh.getNodeList().size() + 1) * 2, []()
  {
    // Jika nyala, matikan. Jika mati, nyalakan
    if (onFlag)
      onFlag = false;
    else
      onFlag = true;

    blinkNoNodes.delay(BLINK_DURATION);

    if (blinkNoNodes.isLastIteration())
    {
      // Selesai berkedip. Reset tugas untuk jalankan selanjutnya
      // berkedip jumlah node (termasuk node ini) kali
      blinkNoNodes.setIterations((mesh.getNodeList().size() + 1) * 2);

      // Hitung jeda berdasarkan waktu mesh saat ini dan BLINK_PERIOD
      // Ini menghasilkan berkedip antara node yang disinkronkan
      blinkNoNodes.enableDelayed(BLINK_PERIOD - (mesh.getNodeTime() % (BLINK_PERIOD * 1000)) / 1000);
    }
  });

  userScheduler.addTask(blinkNoNodes);
  blinkNoNodes.enable();

  randomSeed(analogRead(A0));
}

void loop()
{
  mesh.update();
  digitalWrite(LED, !onFlag);
}

void sendMessage()
{
  //Sensor Dust
  digitalWrite(ledPin, LOW);
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
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Voltage: ");
  lcd.print(sigVolt);
  lcd.setCursor(0, 1);
  lcd.print("Dust: ");
  lcd.print(dustLevel);

  lcd.setCursor(0, 0);
  lcd.print("CO: ");
  lcd.print(sensor_Aout);
  lcd.setCursor(0, 1);
  lcd.print("KEL.3");

  String msg = "Hello from node 1 ";
  msg += mesh.getNodeId();
  msg += " myFreeMemory: " + String(ESP.getFreeHeap()) + " Voltage : " + String(sigVolt) + " Dust : " + String(dustLevel) + " CO : " + String(sensor_Aout);
  mesh.sendBroadcast(msg);

  if (calc_delay)
  {
    SimpleList<uint32_t>::iterator node = nodes.begin();
    while (node != nodes.end())
    {
      mesh.startDelayMeas(*node);
      node++;
    }
    calc_delay = false;
  }

  Serial.printf("Sending message: %s\n", msg.c_str());
  taskSendMessage.setInterval(random(TASK_SECOND * 1, TASK_SECOND * 5)); // antara 1 hingga 5 detik
}

void receivedCallback(uint32_t from, String &msg)
{
  Serial.printf("startHere: Received from %u msg=%s\n", from, msg.c_str());
}

void newConnectionCallback(uint32_t nodeId)
{
  // Reset tugas berkedip
  onFlag = false;
  blinkNoNodes.setIterations((mesh.getNodeList().size() + 1) * 2);
  blinkNoNodes.enableDelayed(BLINK_PERIOD - (mesh.getNodeTime() % (BLINK_PERIOD * 1000)) / 1000);
  Serial.printf("--> startHere: New Connection, nodeId = %u\n", nodeId);
  Serial.printf("--> startHere: New Connection, %s\n", mesh.subConnectionJson(true).c_str());
}

void changedConnectionCallback()
{
  Serial.printf("Changed connections\n");
  // Reset tugas berkedip
  onFlag = false;
  blinkNoNodes.setIterations((mesh.getNodeList().size() + 1) * 2);
  blinkNoNodes.enableDelayed(BLINK_PERIOD - (mesh.getNodeTime() % (BLINK_PERIOD * 1000)) / 1000);
}

void nodeTimeAdjustedCallback(int32_t offset)
{
  Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(), offset);
}

void delayReceivedCallback(uint32_t from, int32_t delay)
{
  Serial.printf("Delay to node %u is %d us\n", from, delay);
}
