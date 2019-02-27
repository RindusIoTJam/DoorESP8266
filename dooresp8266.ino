#define AUTOCONNECT_APID "dooropener"
#define AUTOCONNECT_PSK "RindusIoTdooropener"

#define ring_detectpin 5
#define open_doorpin 4

const uint32_t timeout = 10000;

#include <ESP8266WiFi.h>          // Replace with WiFi.h for ESP32
#include <ESP8266WebServer.h>     // Replace with WebServer.h for ESP32
#include <AutoConnect.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ArduinoJson.h>
#include <Ticker.h>

const char* host = AUTOCONNECT_APID;
const char* ap = AUTOCONNECT_APID;
const char* password = AUTOCONNECT_PSK;
const char* update_path = "/firmware";
const char* update_username = "RindusIoT";
const char* update_password = AUTOCONNECT_PSK;

ESP8266WebServer Server;          // Replace with WebServer for ESP32
AutoConnect      Portal(Server);
ESP8266HTTPUpdateServer httpUpdater;
WiFiServer wifiServer(8266);
Ticker OpendoorTimer;




void rootPage()
{
    char content[] = "<html> <head><style>body {background-color: gray;}h1   {color: blue;}p    {color: white;}</style></head><body><h1>Rindus IoT door opener</h1><p>Configure the WIFI settings by clicking in the following link: <a href=\"/_ac\">Wifi settings</a> <br/> Flash firmaware by clicking in the following link: <a href=\"/firmware\">Flash firmware</a> </p></body></html>";
    Server.send(200, "text/html", content);
}

void OpendoorDisactivate()
{
	digitalWrite(open_doorpin,LOW);
	Serial.println("Ticker time out");
}

void setup()
{


    Serial.begin(115200);
    Portal.config(AUTOCONNECT_APID,AUTOCONNECT_PSK);
    pinMode(ring_detectpin,INPUT);
    pinMode(open_doorpin,OUTPUT);
    Serial.println();

    Server.on("/", rootPage);
    httpUpdater.setup(&Server, update_path, update_username, update_password);
    Serial.printf("[OTA] HTTPUpdateServer ready! Open http://%s.local%s in your browser and login with username '%s' and password '%s'\n", host, update_path, update_username, update_password);
    Serial.println("[OTA] OTA Ready");

    if (Portal.begin())
    {
      Serial.println("WiFi connected: " + WiFi.localIP().toString());
      wifiServer.begin();
      Serial.println("[door server] Command server ready");
      MDNS.begin(host);
      MDNS.addService("http", "tcp", 80);
    }
  else
  {
      Serial.println("Mode AP iniciated");
  }

    Serial.println("Setup finished");
}





void loop() {
    Portal.handleClient();
    //Serial.println("loop?");


    WiFiClient client = wifiServer.available();
    static bool ring_detected = false;
    static bool open_door = false;
    uint32_t currenttime = millis();
    static uint32_t waittime = 0;
    static const size_t bufferSize = 1024;
    //DynamicJsonBuffer jsonBuffer(bufferSize);


    if (client)
    {

		if (client.connected()) {
			delay(100);
		  if (client.available()>0)
		  {
			String str = client.readString();
			Serial.println("read " + str);

			DynamicJsonDocument rootrev(bufferSize);
			deserializeJson(rootrev,str);
			open_door = rootrev["open_request"];
		  }
      DynamicJsonDocument rootsend(bufferSize);
      rootsend["open_request"] = open_door;
      rootsend["ring_detect"] = ring_detected;
      String output;
      serializeJson(rootsend, output);
      client.write(output.c_str());
      Serial.println("send " + output);
      ring_detected =false;
      delay(10);
      }

    client.stop();
    Serial.println("Client disconnected");


    }
    //Serial.println(ring_detected,DEC);
    if((!ring_detected) || (currenttime>waittime))
    {
		ring_detected = digitalRead(ring_detectpin);
		waittime = ring_detected ? currenttime + timeout : 0;
		//waittime = currenttime + timeout;
		Serial.print("pin state ");
		Serial.println(ring_detected,DEC);
    }

    if(open_door)
    {
		digitalWrite(open_doorpin,HIGH);
		OpendoorTimer.once(1, OpendoorDisactivate);
		open_door = false;
    }



}

