/**
 * Simple action node. Received MQTT message "on" or "off" on topics
 * 
 * heater/gas to control the heater
 * heater/light to control the LED in the box.
 * 
 * They are separate devices, but using Node Red can be connected.
 * (there is a dimmer LED when the relay is engaged).
 * 
 * TGB 9/9/18
 */



#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <PubSubClientTools.h>


#include <Thread.h>             // https://github.com/ivanseidel/ArduinoThread
#include <ThreadController.h>
/**/

const char* ssid = "raspi-webgui";
const char* password = "MQTTraspi";
const char* MQTT_SERVER = "10.3.141.1";

String state="off";

WiFiClient espClient;
PubSubClient client(MQTT_SERVER, 1883, espClient);
PubSubClientTools mqtt(client);


ThreadController threadControl = ThreadController();
Thread thread = Thread();

#define relayPin  4 // Relay Control

long lastMsg = 0;
int value = 0;
String s = "";

void setup() {
  Serial.begin(115200);

  pinMode(relayPin, OUTPUT);
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  
  // Connect to WiFi
  Serial.print(s+"Connecting to WiFi: "+ssid+" ");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(s+" connected with IP: "+WiFi.localIP());

  // Connect to MQTT
  Serial.print(s+"Connecting to MQTT: "+MQTT_SERVER+" ... ");
  if (client.connect("SRHeatControl")) {
    Serial.println("connected");

    mqtt.subscribe("heater/light", topic1_subscriber);
    mqtt.subscribe("heater/gas", topic2_subscriber);
  } else {
    Serial.println(s+"failed, rc="+client.state());
  }
  
  // Enable Thread
  thread.onRun(publisher);
  thread.setInterval(2000);
  threadControl.add(&thread);
  
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("arduinoClient")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic","hello world");
      // ... and resubscribe
      client.subscribe("inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" Resetting");
      // Wait 5 seconds before retrying
    }
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  
  client.loop();
  threadControl.run();
  publisher();
}

void publisher() {
  long now = millis();
  if (now - lastMsg > 2000) 
  {
    lastMsg = now;
    ++value;
    Serial.print("Publish message: ");
    Serial.println(state);
    mqtt.publish("heater/state", state);
  }
}

void topic1_subscriber(String topic, String message) {
  Serial.println(s+"Message arrived in function 1 ["+topic+"] "+message);
  if (message.equals("on"))
  {
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
  }
  else
  {
    digitalWrite(BUILTIN_LED, HIGH);
  }
}
void topic2_subscriber(String topic, String message) {
  Serial.println(s+"Message arrived in function 2 ["+topic+"] "+message);
  if (message.equals("on"))
  {
    digitalWrite(relayPin, HIGH);   // Turn the heater on 
    state = "on";
  }
  else
  {
    digitalWrite(relayPin, LOW);
    state="off";
  }
  Serial.println(s+" function 2 ["+topic+"] "+message+"  Done.");
}
