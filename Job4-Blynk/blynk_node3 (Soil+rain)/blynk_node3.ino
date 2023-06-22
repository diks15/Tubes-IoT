#include <painlessMesh.h>
#include <AsyncTCP.h>
#include <LiquidCrystal_I2C.h>
#define LED 2 
#define BLINK_PERIOD 3000 
#define BLINK_DURATION 100 
#define MESH_SSID "JOB4"
#define MESH_PASSWORD "1234567890"
#define MESH_PORT 5555

/* Fill-in information from Blynk Device Info here */
#define BLYNK_TEMPLATE_ID           "TMPL6CXCKOlhR"
#define BLYNK_TEMPLATE_NAME         "Project Perancangan Elektronika"
#define BLYNK_AUTH_TOKEN            "tQ3YpKkhhZlJddbNTYQh8-qt4p1bIHvA"

/* Comment this out to disable prints and save space */
#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>


int SoilMoisturePin = 32;    // Pin analog untuk sensor kelembaban tanah
int RainSensorPin = 33;      // Pin digital untuk sensor hujan
int soilMoistureValue;       // Menyimpan nilai analog dari sensor kelembaban tanah
int soilMoisturePercent;     // Nilai kelembaban tanah dalam persen setelah dimapping
bool isRaining;              // Menyimpan status hujan (true jika hujan, false jika tidak)

// Your WiFi credentials.
// Set password to "" for open networks.
char ssid[] = "Realme 7";
char pass[] = "liverpoolfc150502";


// Prototypes
void sendMessage(); 

void receivedCallback(uint32_t from, String & msg);
void newConnectionCallback(uint32_t nodeId);
void changedConnectionCallback(); 
void nodeTimeAdjustedCallback(int32_t offset); 
void delayReceivedCallback(uint32_t from, int32_t delay);
Scheduler userScheduler; // to control your personal task
painlessMesh mesh;
bool calc_delay = false;
SimpleList<uint32_t> nodes;
void sendMessage(); // Prototype
Task taskSendMessage( TASK_SECOND * 1, TASK_FOREVER, &sendMessage ); // start with a one second interval
// Task to blink the number of nodes
Task blinkNoNodes;
bool onFlag = false;
void setup()
{
  // Debug console
  Serial.begin(115200);

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  
  pinMode(LED, OUTPUT);
  pinMode(RainSensorPin, INPUT_PULLUP);  // Mengatur pin sensor hujan sebagai input dengan pull-up internal
  
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
    // If on, switch off, else switch on
    if (onFlag)
      onFlag = false;
    else
      onFlag = true;
    
    blinkNoNodes.delay(BLINK_DURATION);
    
    if (blinkNoNodes.isLastIteration()) {
      // Finished blinking. Reset task for next run
      // blink number of nodes (including this node) times
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
  digitalWrite(LED, !onFlag);
  // You can inject your own code or combine it with other sketches.
  // Check other examples on how to communicate with Blynk. Remember
  // to avoid delay() function!
}

void sendMessage()
{
  soilMoistureValue = analogRead(SoilMoisturePin);
  soilMoisturePercent = map(soilMoistureValue, 4095, 0, 0, 100);

  isRaining = !digitalRead(RainSensorPin);  // Invert the value since the sensor is pulled up
  
  Blynk.virtualWrite(A4, soilMoisturePercent);  // Mengirim nilai kelembaban tanah ke pin virtual V1 di Blynk
  Blynk.virtualWrite(1, isRaining);            // Mengirim status hujan ke pin virtual V2 di Blynk
  
  String msg = "NODE 3 ";
  msg += mesh.getNodeId();
  msg += String(ESP.getFreeHeap()) + " Kelembaban: " + String(soilMoisturePercent) + "% Hujan: " + String(isRaining);
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
  taskSendMessage.setInterval(random(TASK_SECOND * 1, TASK_SECOND * 5)); // between 1 and 5 seconds
}

void receivedCallback(uint32_t from, String &msg)
{
  Serial.printf("startHere: Received from %u msg=%s\n", from, msg.c_str());
}

void newConnectionCallback(uint32_t nodeId)
{
  // Reset blink task
  onFlag = false;
  blinkNoNodes.setIterations((mesh.getNodeList().size() + 1) * 2);
  blinkNoNodes.enableDelayed(BLINK_PERIOD - (mesh.getNodeTime() % (BLINK_PERIOD * 1000)) / 1000);
  Serial.printf("--> startHere: New Connection, nodeId = %u\n", nodeId);
  Serial.printf("--> startHere: New Connection, %s\n", mesh.subConnectionJson(true).c_str());
}

void changedConnectionCallback()
{
  Serial.printf("Changed connections\n");
  // Reset blink task
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
