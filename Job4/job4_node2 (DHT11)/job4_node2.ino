#include <DHT11.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <painlessMesh.h>
#include <AsyncTCP.h>
#define LED 2 
#define BLINK_PERIOD 3000 
#define BLINK_DURATION 100 
#define MESH_SSID "Realme 7"
#define MESH_PASSWORD "liverpoolfc150502"
#define MESH_PORT 5555

// Create an instance of the DHT11 class and set the digital I/O pin.
DHT11 dht11(32);
#define dhtpin 32;

LiquidCrystal_I2C lcd(0x27, 16, 2);
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
void sendMessage() ; // Prototype
Task taskSendMessage( TASK_SECOND * 1, TASK_FOREVER, 
&sendMessage ); // start with a one second interval
// Task to blink the number of nodes
Task blinkNoNodes;
bool onFlag = false;
void setup() {
  Serial.begin(115200);
  Wire.begin();
  lcd.begin();
  lcd.backlight();
  lcd.print("DHT Sensor");
  lcd.setCursor(0, 1);
  lcd.print("Reading...");
  lcd.clear();

 pinMode(LED, OUTPUT);
 mesh.setDebugMsgTypes(ERROR | DEBUG); // set before init() so that you can see error messages
 mesh.init(MESH_SSID, MESH_PASSWORD, &userScheduler, MESH_PORT);
 mesh.onReceive(&receivedCallback);
 mesh.onNewConnection(&newConnectionCallback);
 mesh.onChangedConnections(&changedConnectionCallback);
 mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
 mesh.onNodeDelayReceived(&delayReceivedCallback);
 userScheduler.addTask( taskSendMessage );
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
 // Calculate delay based on current mesh time and 
 BLINK_PERIOD;
 // This results in blinks between nodes being synced
 blinkNoNodes.enableDelayed (BLINK_PERIOD - (mesh.getNodeTime() % (BLINK_PERIOD*1000))/1000);
 }
 });
 userScheduler.addTask(blinkNoNodes);
 blinkNoNodes.enable();
 randomSeed(analogRead(A0));
}
void loop() {
 mesh.update();
 digitalWrite(LED, !onFlag);
}
void sendMessage() {
    // Read the humidity from the sensor.
    float humidity = dht11.readHumidity();

    // Read the temperature from the sensor.
    float temperature = dht11.readTemperature();
     if (temperature != -1 && humidity != -1)
    {
        Serial.print("Temperature: ");
        Serial.print(temperature);
        Serial.println(" C");

        Serial.print("Humidity: ");
        Serial.print(humidity);
        Serial.println(" %");
          lcd.setCursor(0, 0);
  lcd.print("Humidity: ");
  lcd.print(humidity);
  lcd.print("%");

  lcd.setCursor(0, 1);
  lcd.print("Temperature: ");
  lcd.print(temperature);
  lcd.print("C");

    }
    else
    {
        // If the temperature or humidity reading failed, print an error message.
        Serial.println("Error reading data");
    }

 String msg = "Hello from node 2 ";
 msg += mesh.getNodeId();
 msg += " myFreeMemory: " + String(ESP.getFreeHeap()) + "Temperature = " + String(temperature) + "C" + " Humadity: " + String(humidity) ;
 mesh.sendBroadcast(msg);
 if (calc_delay) {
 SimpleList<uint32_t>::iterator node = nodes.begin();
 while (node != nodes.end()) {
 mesh.startDelayMeas(*node);

 node++;
 }
 calc_delay = false;
 }
 Serial.printf("Sending message: %s\n", msg.c_str());
 taskSendMessage.setInterval( random(TASK_SECOND * 1, TASK_SECOND * 5)); // between 1 and 5 seconds
}
void receivedCallback(uint32_t from, String & msg) {
 Serial.printf("startHere: Received from %u msg=%s\n", from, msg.c_str());
}
void newConnectionCallback(uint32_t nodeId) {
 // Reset blink task
 onFlag = false;
 blinkNoNodes.setIterations((mesh.getNodeList().size() + 1) * 2);
 blinkNoNodes.enableDelayed(BLINK_PERIOD -
(mesh.getNodeTime() % (BLINK_PERIOD*1000))/1000);
 Serial.printf("--> startHere: New Connection, nodeId = %u\n", nodeId);
 Serial.printf("--> startHere: New Connection, %s\n", mesh.subConnectionJson(true).c_str());
}
void changedConnectionCallback() {
 Serial.printf("Changed connections\n");
 // Reset blink task
 onFlag = false;
 blinkNoNodes.setIterations((mesh.getNodeList().size() + 1) * 2);
 blinkNoNodes.enableDelayed(BLINK_PERIOD - (mesh.getNodeTime() % (BLINK_PERIOD*1000))/1000);

 nodes = mesh.getNodeList();
 Serial.printf("Num nodes: %d\n", nodes.size());
 Serial.printf("Connection list:");
 SimpleList<uint32_t>::iterator node = nodes.begin();
 while (node != nodes.end()) {
 Serial.printf(" %u", *node);
 node++;
 }
 Serial.println();
 calc_delay = true;
}
void nodeTimeAdjustedCallback(int32_t offset) {
 Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(), offset);
}
void delayReceivedCallback(uint32_t from, int32_t delay) {
 Serial.printf("Delay to node %u is %d us\n", from, delay);
}