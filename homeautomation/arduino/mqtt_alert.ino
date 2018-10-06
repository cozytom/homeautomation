/**
 * Simple action node. makes noise when receives alert
 * 
 * home/alert
 * 
 * They are separate devices, but using Node Red can be connected.
 * 
 * 
 * TGB 9/30/18
 */

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <PubSubClientTools.h>

/*
#include <Thread.h>             // https://github.com/ivanseidel/ArduinoThread
#include <ThreadController.h>
*/

const char* ssid = "raspi-webgui";
const char* password = "MQTTraspi";
const char* MQTT_SERVER = "10.3.141.1";


WiFiClient espClient;
PubSubClient client(MQTT_SERVER, 1883, espClient);
PubSubClientTools mqtt(client);

/*
ThreadController threadControl = ThreadController();
Thread thread = Thread();
*/

long lastMsg = 0;
int value = 0;
String s = "";

void setup() {
  Serial.begin(115200);

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
  if (client.connect("HomeAlert")) {
    Serial.println("connected");
    digitalWrite(BUILTIN_LED, HIGH); // turn LED off

    mqtt.subscribe("home/alarm", topic1_subscriber);
    mqtt.subscribe("alarm/light", topic2_subscriber);
  } else {
    Serial.println(s+"failed, rc="+client.state());
  }

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("arduinoClient")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic","garageControl Reconnect");
      mqtt.subscribe("home/alarm", topic1_subscriber);
      mqtt.subscribe("alarm/light", topic2_subscriber);
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
}


void topic1_subscriber(String topic, String message) {
  Serial.println(s+"Message arrived in function 1 ["+topic+"] "+message);
  if (message.equals("on"))
  {
    tone(D5, 1000, 0);
  }
  else
  {
    noTone(D5);
  }
}

void topic2_subscriber(String topic, String message) {
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

