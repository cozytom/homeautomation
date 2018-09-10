/**
 * Using the DHT 22 Sensor, get the current temprature and humidity. 
 * Send these values out on the MQTT topics:
 *    sunroom/temperature
 *    sunroom/humidity
 *    
 * Using the touchscreen, display the current time, temprature, humidity, 
 * and thermostat settings. The touch screen allows changing the thermostat
 * setting and sending the value on the MQTT topic:
 *     sunroom/thermostat
 * The thermostat can be changed by listening to the MQTT topic
 *     heater/thermostat
 * The on board LED is controlled by the MQTT topic
 *     heater/light
 * The time is received on the MQTT topic
 *     time/ISO-8601
 *     
 * The code is copyright 2018, Tom Brusehaver 
 * It can be copied using the OSF v3 license. Do what you want with this, but share.
 * 
 * The touchscreen connections are on:   https://nailbuster.com/?page_id=341
 * 
 * Note the input (touch) is the XPT2046, the graphics are the ILI9341
 * Many of the adafruit modules use a different touch input (most of the 
 * ILI9341 touch screen input examples don't work). 
 *
 * The board selected in the IDE is WeMos D1 R2 & Mini. Standard everything 
 * 
 */

#include <SimpleDHT.h>

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <PubSubClientTools.h>

/**
#include <Thread.h>             // https://github.com/ivanseidel/ArduinoThread
#include <ThreadController.h>
**/

#include <XPT2046_Touchscreen.h>
#include <SPI.h>
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"

const char* ssid = "raspi-webgui";
const char* password = "MQTTraspi";
const char* MQTT_SERVER = "10.3.141.1";

String state="off";

// For the Adafruit shield, these are the default.
#define TFT_DC 2
#define TFT_CS 15

#define TCS_PIN  4
// MOSI=11, MISO=12, SCK=13

#define DHTTYPE DHT12   // DHT 22

// DHT Sensor
const int DHTpin = 16;
SimpleDHT22 dht22(DHTpin);

//XPT2046_Touchscreen ts(CS_PIN);
#define TIRQ_PIN  5
//XPT2046_Touchscreen ts(CS_PIN);  // Param 2 - NULL - No interrupts
//XPT2046_Touchscreen ts(CS_PIN, 255);  // Param 2 - 255 - No interrupts
XPT2046_Touchscreen ts(TCS_PIN, TIRQ_PIN);  // Param 2 - Touch IRQ Pin - interrupt enabled polling

// Use hardware SPI 
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);




WiFiClient espClient;
PubSubClient client(MQTT_SERVER, 1883, espClient);
PubSubClientTools mqtt(client);

/**
ThreadController threadControl = ThreadController();
Thread thread = Thread();
**/

/** Global Values **/
long lastMsg = 0;
String s = "";
int    thermostatSetting=70;
String currTime="2018-01-01T00:00";
String eraseTime="2018-01-01T00:00";
String currTemp="0";
String eraseTemp="0";
String currHumid="0";
String eraseHumid="0";
String currThermo="70";
String eraseThermo="70";
String publishedThermo="70";

const int width=240;
const int height=320;

void (* resetFunc)(void)=0;  // have a reset function, in case reconnect fails

void setup() {
  Serial.begin(115200);

  // graphics start
  tft.begin();
  // touch start
  ts.begin();

  // read diagnostics (optional but can help debug problems)
  uint8_t x = tft.readcommand8(ILI9341_RDMODE);
  Serial.print("Display Power Mode: 0x"); Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDMADCTL);
  Serial.print("MADCTL Mode: 0x"); Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDPIXFMT);
  Serial.print("Pixel Format: 0x"); Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDIMGFMT);
  Serial.print("Image Format: 0x"); Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDSELFDIAG);
  Serial.print("Self Diagnostic: 0x"); Serial.println(x, HEX); 

  tft.fillScreen(ILI9341_BLUE);
  displayThermoButtons();

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
  if (client.connect("ESP8266Client")) {
    Serial.println("connected");

    mqtt.subscribe("heater/light", topic1_subscriber);
    mqtt.subscribe("heater/thermostat", topic2_subscriber);
    mqtt.subscribe("time/ISO-8601", topic3_subscriber);
  } else {
    Serial.println(s+"failed, rc="+client.state());
  }

  /* Enable Thread
  thread.onRun(publisher);
  thread.setInterval(2000);
  threadControl.add(&thread);
  */
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    ESP.reset();
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
      ESP.reset();
    }
  }
}

void loop() {
  if (ts.touched()) {
    TS_Point p = ts.getPoint();
    Serial.print("Pressure = ");
    Serial.print(p.z);
    Serial.print(", x = ");
    Serial.print(p.x);
    Serial.print(", y = ");
    Serial.print(p.y);
    Serial.println();

    if ((p.y > 3200) && (p.y < 3800))
    {
      Serial.println("** Y zone **");
      if ((p.x > 1100) && (p.x < 1600)) // UP button
      {
        currThermo = String(++thermostatSetting);
      }
      if ((p.x > 500) && (p.x < 1000)) // Down button
      {
        currThermo = String(--thermostatSetting);
      }
    }
  }

  if (!client.connected()) {
    reconnect();
  }

  client.loop();
  //threadControl.run();
  publisher();           // send out current values.
  displayTime(2);
  displayTemp(8);
  displayHumid(2);
  displayThermostat(4);
}

void displayTime(int fontSize)
{
  if (!currTime.equals(eraseTime))
  {
    tft.setCursor(1, 1);
    tft.fillRect(1, 1, width, fontSize*10, ILI9341_BLACK);
    tft.setTextColor(ILI9341_BLACK);
    tft.setTextSize(fontSize);
    tft.println(eraseTime);
  }
  tft.setCursor(1, 1);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(fontSize);
  tft.println(currTime);
  eraseTime = currTime;
}

void displayTemp(int fontSize)
{
  int X=60; 
  int Y=100;
  if (!currTemp.equals(eraseTemp))
  {
    tft.fillRect(X, Y, fontSize*12, fontSize*10, ILI9341_BLUE);
  }
  tft.setCursor(X, Y);
  tft.setTextColor(ILI9341_GREEN);
  tft.setTextSize(fontSize);
  tft.println(currTemp);
  eraseTemp = currTemp;
}

void displayHumid(int fontSize)
{
  int X=50; 
  int Y=80;
  if (!currHumid.equals(eraseHumid))
  {
    tft.fillRect(0, Y, width, fontSize*10, ILI9341_BLUE);
  }
  tft.setCursor(X, Y);
  tft.setTextColor(ILI9341_GREEN);
  tft.setTextSize(fontSize);
  tft.print("Humid: ");
  tft.print(currHumid);
  tft.println("%");
  eraseHumid = currHumid;
}

void displayThermostat(int fontSize)
{
  int X=25; 
  int Y=220;
  if (!currThermo.equals(eraseThermo))
  {
    tft.fillRect(0, Y, width, fontSize*10, ILI9341_BLUE);
    displayThermoButtons();
  }
  tft.setCursor(X, Y);
  tft.setTextColor(ILI9341_GREEN);
  tft.setTextSize(2);
  tft.print("Heat at: ");
  tft.setTextSize(fontSize);
  tft.print(currThermo);
  eraseThermo = currThermo;
}

void displayThermoButtons()
{
   int X = width-60;
   int Y = 180;

   tft.fillRect(X,Y, 50, 50, ILI9341_BLACK);
   tft.fillTriangle(X+5,Y+40, X+25,Y+5, X+45,Y+40, ILI9341_BLUE); 
   Y+=60;
   tft.fillRect(X, Y, 50, 50, ILI9341_BLACK);
   tft.fillTriangle(X+5,Y+10, X+25,Y+45, X+45,Y+10, ILI9341_BLUE);
}

float degCtoF(float deg)
{
  return (deg * 9.0 / 5.0) + 32.0;
}

void publisher() {
  long now = millis();
  if (now - lastMsg > 2000) 
  {
    lastMsg = now;
    Serial.print("Publish message: ");
    float humid = 0;
    float tempC = 0;
    if (dht22.read2(&tempC, &humid, NULL) == 0)
    {
      float tempFl = degCtoF(tempC);
      String val = String(tempFl);
      mqtt.publish("sunroom/temperature", val);
      int tempF = (int)tempFl;
      currTemp=String(tempF);
      Serial.print("Temp: ");  Serial.println(val);
      val = String(humid);
      mqtt.publish("sunroom/humidity", val);
      currHumid = val;
    }
    else
    {
      Serial.println("Error Reading DHT device");
    }
  }
  if (!currThermo.equals(publishedThermo))
  {
    mqtt.publish("sunroom/thermostat", currThermo);
    publishedThermo = currThermo;
  }
}

void topic1_subscriber(String topic, String message) {
  Serial.println(s+"Message arrived in function 1 ["+topic+"] "+message);
  if (message.equals("on"))
  {
    digitalWrite(BUILTIN_LED, LOW);   // LED on 
    state = "on";
  }
  else
  {
    digitalWrite(BUILTIN_LED, HIGH);
    state="off";
  }
}

/**
 * Read number value of thermostat setting.
 */
void topic2_subscriber(String topic, String message) {
  Serial.println(s+"Message arrived in function 2 ["+topic+"] "+message);
  int thermSetting = message.toInt();
  if ((thermSetting > 55) && (thermSetting < 99))
  {
    thermostatSetting = thermSetting;
    currThermo = String(thermostatSetting);
  }
}

/**
 * Read current time.  (ISO-8601 format: 2016-06-01T21:34:12.629Z)
 */
void topic3_subscriber(String topic, String message) {
  Serial.println(s+"Message arrived in function 3 ["+topic+"] "+message);
  currTime = message;
}
