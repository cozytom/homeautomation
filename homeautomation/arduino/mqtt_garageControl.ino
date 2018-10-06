/**
 * Simple action node. Read sensors and send out status
 * 
 * garage/bigDoor to get status of garage door
 * garage/littleDoor to get status of garage door.
 * garage/outsideTemp to read ds18b20 temperature sensor. 
 * 
 * They are separate devices, but using Node Red can be connected.
 * 
 * 
 * TGB 9/30/18
 */

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <PubSubClientTools.h>
#include <OneWire.h>
#include <DallasTemperature.h>
/*
#include <Thread.h>             // https://github.com/ivanseidel/ArduinoThread
#include <ThreadController.h>
*/

const char* ssid = "raspi-webgui";
const char* password = "MQTTraspi";
const char* MQTT_SERVER = "10.3.141.1";

// Data wire is plugged into port D3 on the ESP8266
#define ONE_WIRE_BUS D3

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

//DS18B20
#define ONE_WIRE_BUS D3 //Pin to which is attached a temperature sensor
#define ONE_WIRE_MAX_DEV 15 //The maximum number of devices

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

  pinMode(D1, INPUT_PULLUP);      // big door
  pinMode(D2, INPUT_PULLUP);      // little door
  
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
  if (client.connect("GarageControl")) {
    Serial.println("connected");
    digitalWrite(BUILTIN_LED, HIGH); // turn LED off

    mqtt.subscribe("garage/light", topic1_subscriber);
  } else {
    Serial.println(s+"failed, rc="+client.state());
  }

  // Start up the library
  sensors.begin();
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
      mqtt.subscribe("garage/light", topic1_subscriber);
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
  // threadControl.run();
  publisher();
}

void publisher() {
  long now = millis();
  if (now - lastMsg > 5000)  // 10 second loop.
  {
    lastMsg = now;
    ++value;
    Serial.println("Publish messages: ");


    int bd = digitalRead(D1);
    int ld = digitalRead(D2);

    String bigDoor="closed";
    String littleDoor="closed";

    if (bd) {
      bigDoor = "open";
    }
    if (ld) {
      littleDoor = "open";
    }
    Serial.print(" bigDoor: ");
    Serial.println(bigDoor);
    Serial.print(" littleDoor: ");
    Serial.println(littleDoor);
            
    mqtt.publish("garage/bigDoor", bigDoor);
    mqtt.publish("garage/littleDoor", littleDoor);

    sensors.requestTemperatures(); // Send the command to get temperatures
    int temp = sensors.getTempCByIndex(0); 
    Serial.print(" temperature: ");
    Serial.println(temp);
    String tempS = String(temp);
    mqtt.publish("garage/outsideTemperature", tempS);
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

