#include <painlessMesh.h>
#include <AsyncTCP.h>
#include <LiquidCrystal_I2C.h>

#define LED_PIN 2
#define BLINK_PERIOD 3000
#define BLINK_DURATION 100
#define MESH_SSID "JOB4"
#define MESH_PASSWORD "1234567890"
#define MESH_PORT 5555

/* Fill-in information from Blynk Device Info here */
#define BLYNK_TEMPLATE_ID           "TMPL6CXCKOlhR"
#define BLYNK_TEMPLATE_NAME         "Project Perancangan Elektronika"
#define BLYNK_AUTH_TOKEN            "tQ3YpKkhhZlJddbNTYQh8-qt4p1bIHvA"

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

// Sensor Pins
#define MQ2_PIN 34
#define DUST_PIN 35
#define LED_DUST_PIN 13
#define DUST_TSAMPLING 280
#define DUST_TDELTA 40
#define DUST_TSLEEP 9680

// Your WiFi credentials
// Set password to "" for open networks
char ssid[] = "Realme 7";
char pass[] = "liverpoolfc150502";

// Mesh Network
Scheduler userScheduler; // to control your personal task
painlessMesh mesh;
bool calc_delay = false;
SimpleList<uint32_t> nodes;

// Prototypes
void sendMessage();
void receivedCallback(uint32_t from, String &msg);
void newConnectionCallback(uint32_t nodeId);
void changedConnectionCallback();
void nodeTimeAdjustedCallback(int32_t offset);
void delayReceivedCallback(uint32_t from, int32_t delay);

// Task to send messages
Task taskSendMessage(TASK_SECOND * 1, TASK_FOREVER, &sendMessage);

// Task to blink the number of nodes
Task blinkNoNodes;
bool onFlag = false;

void setup()
{
  // Debug console
  Serial.begin(115200);

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  pinMode(LED_PIN, OUTPUT);
  pinMode(DUST_PIN, INPUT_PULLUP);
  pinMode(LED_DUST_PIN, OUTPUT);

  mesh.setDebugMsgTypes(ERROR | DEBUG); // set before init() so that you can see error messages
  mesh.init(MESH_SSID, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
  mesh.onNodeDelayReceived(&delayReceivedCallback);

  userScheduler.addTask(taskSendMessage);
  taskSendMessage.enable();

  blinkNoNodes.set(BLINK_PERIOD, (mesh.getNodeList().size() + 1) * 2, []() {
    // If on, switch off; else, switch on
    if (onFlag)
      onFlag = false;
    else
      onFlag = true;

    blinkNoNodes.delay(BLINK_DURATION);

    if (blinkNoNodes.isLastIteration()) {
      // Finished blinking. Reset task for next run
      // Blink number of nodes (including this node) times
      blinkNoNodes.setIterations((mesh.getNodeList().size() + 1) * 2);
      // Calculate delay based on current mesh time and BLINK_PERIOD
      // This results in blinks between nodes being synced
      blinkNoNodes.enableDelayed(BLINK_PERIOD - (mesh.getNodeTime() % (BLINK_PERIOD * 1000)) / 1000);
    }
  });

  userScheduler.addTask(blinkNoNodes);
  blinkNoNodes.enable();

  randomSeed(analogRead(A0));
}

void loop()
{
  Blynk.run();
  mesh.update();
  digitalWrite(LED_PIN, !onFlag);
}

void sendMessage()
{
  // Read MQ2 sensor
  int sensor_Aout = analogRead(MQ2_PIN);
  Blynk.virtualWrite(V0, sensor_Aout);

  // Read Dust sensor
  digitalWrite(LED_DUST_PIN, LOW);
  delayMicroseconds(DUST_TSAMPLING);
  float outVo = analogRead(DUST_PIN);
  delayMicroseconds(DUST_TDELTA);
  digitalWrite(LED_DUST_PIN, HIGH);
  delayMicroseconds(DUST_TSLEEP);
  float sigVolt = outVo * (3.3 / 1024);
  float dustLevel = 1.7 * sigVolt - 0.1;
  Blynk.virtualWrite(V0, sigVolt);
  Blynk.virtualWrite(V1, dustLevel);

  String msg = "NODE 1 ";
  msg += mesh.getNodeId();
  msg += " FreeHeap: " + String(ESP.getFreeHeap()) + " MQ2: " + String(sensor_Aout) + " Voltage: " + String(sigVolt) + " Dust: " + String(dustLevel);
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
  taskSendMessage.setInterval(random(TASK_SECOND * 1, TASK_SECOND * 5)); // Between 1 and 5 seconds
}

void receivedCallback(uint32_t from, String &msg)
{
  Serial.printf("Received from %u: %s\n", from, msg.c_str());
}

void newConnectionCallback(uint32_t nodeId)
{
  // Reset blink task
  onFlag = false;
  blinkNoNodes.setIterations((mesh.getNodeList().size() + 1) * 2);
  blinkNoNodes.enableDelayed(BLINK_PERIOD - (mesh.getNodeTime() % (BLINK_PERIOD * 1000)) / 1000);
  Serial.printf("--> New Connection, nodeId = %u\n", nodeId);
  Serial.printf("--> New Connection, %s\n", mesh.subConnectionJson(true).c_str());
}

void changedConnectionCallback()
{
  Serial.println("Changed connections");
  // Reset blink task
  onFlag = false;
  blinkNoNodes.setIterations((mesh.getNodeList().size() + 1) * 2);
  blinkNoNodes.enableDelayed(BLINK_PERIOD - (mesh.getNodeTime() % (BLINK_PERIOD * 1000)) / 1000);
  nodes = mesh.getNodeList();
  Serial.printf("Num nodes: %d\n", nodes.size());
  Serial.print("Connection list:");
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
