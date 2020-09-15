
/*     
 * The code is copyright 2018, Tom Brusehaver 
 * It can be copied using the OSF v3 license. Do what you want with this, but share.
 * 
 * This uses a ublox 7m GPS device at 9600-baud serial hooked up on pins 2(rx) and 0(tx).
 *
 * The board selected in the IDE is WeMos D1 R2 & Mini. Standard everything 
 * 
 */
 
#include <SoftwareSerial.h>

#include <TinyGPS.h>

#include <ESP8266WiFi.h>m
#include <PubSubClient.h>
#include <PubSubClientTools.h>

#include <ArduinoJson.h>

TinyGPS gps;
SoftwareSerial ss(2, 0);

const char* ssid = "raspi-webgui";
const char* password = "MQTTraspi";
const char* MQTT_SERVER = "10.3.141.1";

static void smartdelay(unsigned long ms);
static void print_float(float val, float invalid, int len, int prec);
static void print_int(unsigned long val, unsigned long invalid, int len);
static void print_date(TinyGPS &gps);
static void print_str(const char *str, int len);

WiFiClient espClient;
PubSubClient client(MQTT_SERVER, 1883, espClient);
PubSubClientTools mqtt(client);



void setup()
{
  Serial.begin(115200);
  
  ss.begin(9600);

  setup_wifi();
  String s = "";
    // Connect to MQTT
  Serial.print(s+"Connecting to MQTT: "+MQTT_SERVER+" ... ");
  if (client.connect("GPSClient")) {
    Serial.println("connected");
  } else {
    Serial.println(s+"failed, rc="+client.state());
  }
}


void setup_wifi() 
{
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

//////////////////////////////////////////////////////////////////////////
//////////  MQTT Stuff ///////////////////////////////////////////////////

void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("GPSClient")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic", "GPS connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void publisher(char *gpsMSG) {
  StaticJsonDocument<300> doc;
  int year;
  byte month, day, hour, minute, second, hundredths;
  unsigned long age;
  float flat, flon;
  //unsigned long age, date, time, chars = 0;
  gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths, &age);
  
  doc["sensor"] = "gps";
  JsonArray time = doc.createNestedArray("time");
  time.add(year);
  time.add(month);
  time.add(day);
  time.add(hour);
  time.add(minute);
  time.add(second); 
  time.add(hundredths);

  gps.f_get_position(&flat, &flon, &age);
  doc["Latitude"] = flat;
  doc["Longitude"] = flon;
  doc["Satellites"] = gps.satellites();
  doc["HDOP"] = gps.hdop();
  doc["Altitude"] = gps.f_altitude();
  doc["Course"] = gps.f_course();
  doc["Speed"] = gps.f_speed_kmph();

  // Generate the minified JSON and send it to the Serial port.
  //
  //serializeJson(doc, Serial);
  // The above line prints

  // Start a new line
  Serial.println();

  // Generate the prettified JSON and send it to the Serial port.
  serializeJsonPretty(doc, Serial);

  char buffer[256];
  serializeJson(doc, buffer);
  mqtt.publish("gps/location", buffer);
  Serial.println("11111111111111111111111111111111111111111111111");
  Serial.println(buffer);
  Serial.println("22222222222222222222222222222222222222222222222");
}

void loop()
{
  float flat, flon;
  unsigned long age, date, time, chars = 0;
  unsigned short sentences = 0, failed = 0;
  static const double LONDON_LAT = 51.508131, LONDON_LON = -0.128002;
  char gpsMsg[300];
  
  gps.stats(&chars, &sentences, &failed);

  publisher(gpsMsg);
  client.loop();
  smartdelay(1000);
}

static void smartdelay(unsigned long ms)
{
  unsigned long start = millis();
  do 
  {
    while (ss.available())
      gps.encode(ss.read());
  } while (millis() - start < ms);
}

static void print_str(const char *str, int len)
{
  int slen = strlen(str);
  for (int i=0; i<len; ++i)
    Serial.print(i<slen ? str[i] : ' ');
  smartdelay(0);
}
