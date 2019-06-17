#include <Arduino.h>


#include <ESP8266WiFi.h> 
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <FS.h>
#include <DNSServer.h>
#include <WiFiManager.h>

#include <set>

#include "C17GH3.h"
#include "Webserver.h"

WiFiManager wifiManager;
C17GH3MessageBuffer msgBuffer;
C17GH3State state;
Webserver webserver;


String sBuffer;

bool inSerial = false;
uint32_t lastMS = 0;


wl_status_t wifiStatus = WL_NO_SHIELD;

void setup()
{
	String devName = String("Thermostat-") + String(ESP.getChipId(),HEX);
	WiFi.hostname(devName);
	WiFi.enableAP(false);
	WiFi.enableSTA(true);
	WiFi.begin();
	wifiManager.setConfigPortalTimeout(120);
	wifiManager.setDebugOutput(false);

	Serial.begin(9600);

	state.setWifiConfigCallback([devName]() {
		Serial.println("Configuration portal opened");
		webserver.stop();
        wifiManager.startConfigPortal(devName.c_str());
		webserver.start();
		WiFi.enableAP(false);
		Serial.println("Configuration portal closed");
    });

	webserver.init(&state);
}


void loop()
{
	
	if (WiFi.status() != wifiStatus)
	{
		wifiStatus = WiFi.status();
		String status;
		switch(wifiStatus)
		{
			case WL_NO_SHIELD:
				status = "no shield";
				break;
			case WL_IDLE_STATUS:
				status = "idle";
				break;
			case WL_NO_SSID_AVAIL:
				status = "no ssid available";
				break;
			case WL_SCAN_COMPLETED:
				status = "scan completed";
				break;
			case WL_CONNECTED:
				status = "connected. SSID: ";
				status += WiFi.SSID();
				status += ", IP address: ";
				status += WiFi.localIP().toString();
				break;
			case WL_CONNECT_FAILED:
				status = "connect failed";
				break;
			case WL_CONNECTION_LOST:
				status = "connection lost";
				break;
			case WL_DISCONNECTED:
				status = "disconnected";
				break;
		}

		Serial.println("WiFi Status changed: " + status);
	}

	if (Serial.available())
	{
		if (!inSerial)
		{
			sBuffer = String();
			Serial.println("Serial data available");
		}
		inSerial = true;
		while (Serial.available())
		{
			sBuffer += String((char)Serial.read());
			sBuffer.trim();
		}
		
		if (sBuffer.length() == 32)
		{
			Serial.println("Got a proper length string");
			while (sBuffer.length() > 0)
			{
				String v = sBuffer.substring(0,2);
				sBuffer = sBuffer.substring(2);
				uint8_t byte = (uint8_t)strtol(v.c_str(), nullptr, 16);
				bool hasMsg = msgBuffer.addbyte(byte);
				if (hasMsg)
				{
					Serial.printf("!! found a message\n");
					state.processMessage(C17GH3MessageBase(msgBuffer.getBytes()));

				}
				
			}
			//Serial.printf("\n");

			inSerial = false;

		}
		else
		{
			//Serial.printf("Size was %d\n", sBuffer.length());
		}
		lastMS = millis();
	}

	uint32_t newLast = millis();
	if (newLast - lastMS > 500)
	{
		inSerial = false;
	}

    webserver.process();
}

/*

#define  "192.168.31.107" // Enter your MQTT server adderss or IP. I use my DuckDNS adddress (yourname.duckdns.org) in this field
#define mqtt_user "DVES_USER" //enter your MQTT username
#define mqtt_password "DVES_PASS" //enter your password

#define topic_base "livingroom/wifiremote/"
#define send_raw topic_base"send/raw"
#define send_samsung topic_base"send/samsung"
#define send_rc5 topic_base"send/rc5"
#define send_rc6 topic_base"send/rc6"
#define send_nec topic_base"send/nec"
#define send_sony topic_base"send/sony"
#define send_jvc topic_base"send/jvc"
#define send_whynter topic_base"send/whynter"
#define send_aiwarct501 topic_base"send/aiwar"
#define send_lg topic_base"send/lg"
#define send_dish topic_base"send/dish"
#define send_sharp topic_base"send/sharp"
#define send_sharpraw topic_base"send/sharpraw"
#define send_denon topic_base"send/denon"
#define send_pronto topic_base"send/pronto"
#define send_legopower topic_base"send/legopower"

WiFiClient espClient;
PubSubClient mqttClient(espClient); //this needs to be unique for each controller


void mqttCallback(char* topic, byte* payload, unsigned int length)
{
	char buffer[64] = {0};

	if (length >= 64)
		return; // message too big

	memcpy(buffer, payload, length);

	// sendsamsung: data=aaeeff00,bits=32,repeat=5
	// raw data=aa00ff,hz=300
	// ...
	uint32_t bits = 0;
	uint32_t repeat = 0;
	//uint32_t data[16] = {0};
	uint64_t data64 = 0;

	char *saveptr1 = NULL, *saveptr2 = NULL;

	char* token1 = strtok_r(buffer, ",", &saveptr1); 

	while (token1 != NULL)
	{
		char* name = strtok_r(token1,"=",&saveptr2);
		if (name != NULL)
		{
			char* value = strtok_r(NULL,"=",&saveptr2);
			if (NULL != value)
			{
				Serial.print(name);
				Serial.print(" = ");
				Serial.print(value);
				Serial.println();
				if (String(name) == "data")
				{
						// convert hex string
						char* endptr = NULL;
						uint32_t val = strtoul (value, &endptr, 16);
						Serial.print("value in hex: ");
						Serial.println(val, HEX);
						data64 = val;
						

				}
				else if (String(name) == "bits")
				{
					// convert int string to int
					bits = atoi(value);
					
				}
				else if (String(name) == "repeat")
				{
					repeat = atoi(value);
				}

			}
			token1 = strtok_r(NULL,",",&saveptr1);
		}
	} 

	if (String(topic) == send_samsung)
	{
		if (0 != data64 && bits == 32 && repeat <= 20)
		{
			Serial.print("Sending data to samsung data=");
			Serial.print((uint32_t)data64,HEX);
			Serial.print(", bits=");
			Serial.print(bits);
			Serial.print(", repeat=");
			Serial.println(repeat);
			irsend.sendSAMSUNG(data64, bits, repeat);
		}
		else
		{
			Serial.print("Data Error: Not sending data to samsung data=");
			Serial.print((uint32_t)data64,HEX);
			Serial.print(", bits=");
			Serial.print(bits);
			Serial.print(", repeat=");
			Serial.println(repeat);
		}
		
	}
}

uint32_t mqttNextConnectAttempt = 0;

void mqttReconnect()
{

	uint32_t now = millis();
	if (now > mqttNextConnectAttempt)
	{
		// Loop until we're reconnected
		if (!mqttClient.connected())
		{
			Serial.print("Attempting MQTT connection...");
			// Create a random client ID
			String clientId = "devname-";
			clientId += String(random(0xffff), HEX);
			// Attempt to connect
			if (mqttClient.connect(devname, mqtt_user, mqtt_password))
			{
				Serial.println("connected");
				// Once connected, publish an announcement...
				//mqttClient.publish("outTopic", "hello world");
				// ... and resubscribe
				mqttClient.subscribe(send_samsung);
				mqttClient.subscribe(send_raw);
				mqttClient.subscribe(send_rc5);
				mqttClient.subscribe(send_rc6);
				mqttClient.subscribe(send_nec);
				mqttClient.subscribe(send_sony);
				mqttClient.subscribe(send_jvc);
				mqttClient.subscribe(send_whynter);
				mqttClient.subscribe(send_aiwarct501);
				mqttClient.subscribe(send_lg);
				mqttClient.subscribe(send_dish);
				mqttClient.subscribe(send_sharp);
				mqttClient.subscribe(send_sharpraw);
				mqttClient.subscribe(send_denon);
				mqttClient.subscribe(send_pronto);
				mqttClient.subscribe(send_legopower);
				mqttClient.subscribe(send_samsung);
			}
			else
			{
				Serial.print("failed, rc=");
				Serial.print(mqttClient.state());
				Serial.println(" try again in 5 seconds");
				// Wait 5 seconds before retrying
				//delay(5000);
				mqttNextConnectAttempt = now + 2000;
			}
		}
	}
}

*/