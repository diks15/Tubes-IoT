#include <painlessMesh.h>
#include <AsyncTCP.h>
#include <LiquidCrystal_I2C.h>

#define LED 2
#define BLINK_PERIOD 3000
#define BLINK_DURATION 100
#define MESH_SSID "Realme 7"
#define MESH_PASSWORD "liverpoolfc150502"
#define MESH_PORT 5555

int SensorPin = 32; // Deklarasi pin analog yang digunakan
int RainSensorPin = 33;
int soilMoistureValue; // Menyimpan nilai analog dari sensor ke ESP32
int soilMoisturePercent; // Nilai yang diperoleh dalam bentuk persen setelah dimapping

LiquidCrystal_I2C lcd(0x27, 16, 2);
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
  pinMode(SensorPin, INPUT);

  // Tampilan di LCD i2C
  lcd.begin();         // inisialisasi lcd
  lcd.backlight();     // Hidupkan backlight LCD
  lcd.setCursor(0, 0);
  lcd.print("Kelembaban Tanah");
  lcd.setCursor(0, 1);
  lcd.print("Membaca...");
  lcd.clear();

  // Set up Rain Sensor
  pinMode(RainSensorPin, INPUT);

  pinMode(LED, OUTPUT);

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
  soilMoistureValue = analogRead(SensorPin);
  soilMoisturePercent = map(soilMoistureValue, 0, 4095, 0, 100);

  int rainSensorValue = digitalRead(RainSensorPin); // Baca nilai sensor hujan

  Serial.print("Nilai analog = ");
  Serial.print(soilMoistureValue);
  Serial.print(", Persentase kelembaban tanah = ");
  Serial.print(soilMoisturePercent);
  Serial.print("%");

  if (rainSensorValue == HIGH)
  {
    Serial.println(", Tidak Hujan");
    lcd.setCursor(0, 0);
    lcd.print("Kelembaban: ");
    lcd.print(soilMoisturePercent);
    lcd.print("% (Hujan)");
  }
  else
  {
    Serial.println(", Hujan");
    lcd.setCursor(0, 0);
    lcd.print("Kelembaban: ");
    lcd.print(soilMoisturePercent);
    lcd.print("%");
  }

  lcd.setCursor(0, 1);
  lcd.print("Nilai: ");
  lcd.print(soilMoistureValue);

  String msg = "Hello from node 3 ";
  msg += mesh.getNodeId();
  msg += " myFreeMemory: " + String(ESP.getFreeHeap()) + " kelembaman : " + String(soilMoisturePercent) + "%" + " Nilai" + String(soilMoistureValue) + " Status_hujan : " + String(rainSensorValue);
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

  nodes = mesh.getNodeList();
  Serial.printf("Num nodes: %d\n", nodes.size());
  Serial.printf("Connection list:");

  SimpleList<uint32_t>::iterator node = nodes.begin();
  while (node != nodes.end())
  {
    Serial.printf(" %u", *node);
    node++;
  }

  Serial.println();
  calc_delay = true;
}

void nodeTimeAdjustedCallback(int32_t offset)
{
  Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(), offset);
}

void delayReceivedCallback(uint32_t from, int32_t delay)
{
  Serial.printf("Delay to node %u is %d us\n", from, delay);
}
